#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>
#include <raymath.h>
#include <cstdio>
#include <rcamera.h>

#include "scene.h"
#include "logging.h"
#include "material.h"
#include "inlet.h"
#include "heat_solver.h"
#include "mesh_builder.h"
#include "gui.h"



#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#ifdef __EMSCRIPTEN__
EM_BOOL KeyCallback(int eventType, const EmscriptenKeyboardEvent *e, void *userData)
{
    if (strcmp(e->key, " ") == 0 ||
        strcmp(e->key, "Shift") == 0 ||
        strcmp(e->key, "w") == 0 ||
        strcmp(e->key, "a") == 0 ||
        strcmp(e->key, "s") == 0 ||
        strcmp(e->key, "d") == 0)
    {
        return EM_TRUE;
    }

    return EM_FALSE;
}
#endif

#ifdef __EMSCRIPTEN__
    int screenWidth = 1200;
    int screenHeight = 900;
#else
    int screenWidth = 1200;
    int screenHeight = 900;
#endif

Config config;
Scene scene = Scene(config);

Camera3D camera;

int numCubes;
Mesh mesh;
Model model;

vector<float> time_arr;
vector<float> sensor_temperatures;

float simdt = scene.dt;
int numIter = 1;

void init()
{   
    scene = Scene(scene.config);

    scene.gui.timePassed = 0;

    int numX = scene.state.numX;
    int numY = scene.state.numY;
    int numZ = scene.state.numZ;

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                scene.state.temperature[scene.idx(x, y, z)] = scene.config.roomTemperature;
                scene.state.isMaterial[scene.idx(x, y, z)] = 0;
                scene.state.heatInlet[scene.idx(x, y, z)] = 0;
            }
        }
    }

    scene.state.temp = scene.state.temperature;

    //memcpy(temp, temperature, sizeof(temp));
    //memset(heatInlet, 0, sizeof(heatInlet));

    scene.setCylinderHollow();

    // setPlane();
    

    numCubes = getNumberCubes(scene);
    mesh = GenMeshIdenticalCubes(scene, scene.state.drawSize);
    model = LoadModelFromMesh(mesh);

    scene.gui.reset = false;

}


void UpdateCameraCustom(Camera3D& camera, float dt)
{
    Vector2 mouseDelta = GetMouseDelta();

    float rotSpeed = 0.003f;
    float moveSpeed = 5.0f * dt;

    // Mouse look (same as raylib's free camera rotation)
    CameraYaw(&camera, -mouseDelta.x * rotSpeed, false);
    CameraPitch(&camera, -mouseDelta.y * rotSpeed, true, false, false);

    // Forward/back/strafe (WASD-style, optional — mirror whatever you already use)
    if (IsKeyDown(KEY_W)) CameraMoveForward(&camera, moveSpeed, false);
    if (IsKeyDown(KEY_S)) CameraMoveForward(&camera, -moveSpeed, false);
    if (IsKeyDown(KEY_D)) CameraMoveRight(&camera, moveSpeed, false);
    if (IsKeyDown(KEY_A)) CameraMoveRight(&camera, -moveSpeed, false);

    // Vertical, rebound to Shift/Space instead of Ctrl+wheel
    if (IsKeyDown(KEY_LEFT_SHIFT)) CameraMoveUp(&camera, -moveSpeed);
    if (IsKeyDown(KEY_SPACE))      CameraMoveUp(&camera, moveSpeed);
}

int SelectCubeTargeted(Camera3D camera, Mesh mesh)
{
    Ray ray;

    if (scene.gui.cameraActive) ray = GetScreenToWorldRay(GetMousePosition(), camera);
    else ray = GetScreenToWorldRay({0.5f * GetScreenWidth(), 0.5f * GetScreenHeight()}, camera);

    RayCollision collision = GetRayCollisionMesh(ray, mesh, MatrixIdentity());
    if (!collision.hit) return -1; // sentinel: no cube under the crosshair

    // Nudge the hit point slightly into the cube along -normal so floating-point
    // noise at the exact face boundary can't push it into the neighboring cell.
    Vector3 nudged = Vector3Subtract(collision.point, Vector3Scale(collision.normal, 0.01f * scene.state.drawSize));
    Vector3 hitPosition = Vector3Scale(nudged, 1.0f / scene.state.drawSize);

    int x = (int)floorf(hitPosition.x);
    int y = (int)floorf(hitPosition.y);
    int z = (int)floorf(hitPosition.z);

    // Defensive clamp in case the nudge pushes a boundary-adjacent hit outside the grid.
    x = Clamp(x, 0, scene.state.numX - 1);
    y = Clamp(y, 0, scene.state.numY - 1);
    z = Clamp(z, 0, scene.state.numZ - 1);

    return scene.idx(x, y, z);
}

void loop()
{
    if (scene.gui.reset) init();
    

    if (IsKeyPressed(KEY_C))
    {   
        scene.gui.cameraActive = !scene.gui.cameraActive;
        if (scene.gui.cameraActive) 
        { 
            DisableCursor();
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;

        }
        else 
        {
            EnableCursor();
            ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        }
    }

    
    if (scene.gui.cameraActive)
    {
        UpdateCameraCustom(camera, GetFrameTime());
    }

    if (scene.gui.selectMode)
    {
        scene.config.selectedSensor = SelectCubeTargeted(camera, mesh);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) scene.gui.selectMode = false;
    
    }

    simdt = scene.dt * scene.gui.speedUp;
    

    for (int x = 0; x < numIter; x++) {
        if (!scene.gui.running) break;

        if (scene.gui.timePassed > scene.gui.stopTime && scene.gui.stopTime != 0)
        {
            scene.gui.running = false;
            SaveSensorData(time_arr, sensor_temperatures);
        }

        rotateAndApplyInlet(scene, simdt);
        heatConductionCrankNicolson(scene, simdt, material, 0.5);
        radiationLoss(scene, simdt);

        scene.gui.timePassed += simdt;

        time_arr.push_back(scene.gui.timePassed);
        sensor_temperatures.push_back(scene.state.temperature[scene.config.selectedSensor]);

    }

    BeginDrawing();

        ClearBackground(WHITE);
        BeginMode3D(camera);

            DrawGrid(50, 1.0f);
            
            SetAllCubeColors(scene, mesh, numCubes, 36);
           
            DrawModelWires(model, {0, 0, 0}, 1, BLACK);
            DrawModel(model, {0, 0, 0}, 1, WHITE);
    
        EndMode3D();

        //Crosshair
        if (scene.gui.selectMode)
        {
            scene.gui.cameraActive = true;
            DisableCursor();
            int centerX = GetScreenWidth() / 2;
            int centerY = GetScreenHeight() / 2;
            DrawCircle(centerX, centerY, 4, GREEN);

            int sel = scene.config.selectedSensor;
            int x = sel / (scene.state.numY * scene.state.numZ);
            sel = sel - x * (scene.state.numY * scene.state.numZ);
            int y = sel / scene.state.numZ;
            int z = sel - y * scene.state.numZ;

            DrawText(TextFormat("x: %08i | y: %08i | z: %08i", x, y, z), GetScreenWidth() - 200, GetScreenHeight()- 10, 10, BLACK);

        }

        

        DrawFPS(10, screenHeight - 30);

        MakeImGui(scene);

    EndDrawing();

    printf("\rTimings: heatConduction(%d), Draw(%d), radiationLoss(%d)      ", timings[0], timings[1], timings[2]);
    fflush(stdout);
}

int main()
{
    
    setvbuf(stdout, NULL, _IONBF, 0);
    TraceLog(LOG_ERROR, "Tracelog for debugging :)");

    InitWindow(screenWidth, screenHeight, "Heat Transfer in Pipe");
    
    #ifdef __EMSCRIPTEN__
        emscripten_set_keydown_callback(
        "#canvas",
        nullptr,
        EM_TRUE,
        KeyCallback
        );

        
    #endif
    
    #if defined(__EMSCRIPTEN__)
        EM_ASM({
            Module.canvas.style.width = "100vw";
            Module.canvas.style.height = "100vh";
        });
    #endif  


    camera = { 0 };
    camera.position = (Vector3){ -3.0f, 0.5f, 0.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };    // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };        // Camera up vector (rotation towards target)
    camera.fovy = 90.0f;                              // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;
    

    DisableCursor();
    SetTargetFPS(60);
    SetTraceLogCallback(CustomTraceLog);

    rlImGuiSetup(true);
    init();
    

    #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(loop, 0, 1);
    #else
        while (!WindowShouldClose())
        {
            loop();
        }
    #endif

    

    rlImGuiShutdown();
    CloseWindow();
}
