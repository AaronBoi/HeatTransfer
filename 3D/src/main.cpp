#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>
#include <raymath.h>
#include <cstdio>


#include "logging.h"
#include "material.h"
#include "grid_state.h"
#include "app_state.h"
#include "geometry.h"
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
        strcmp(e->key, "Control") == 0 ||
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
    int screenWidth = 2400;
    int screenHeight = 1800;
#else
    int screenWidth = 1200;
    int screenHeight = 900;
#endif


Camera3D camera;

int numCubes;
Mesh mesh;
Model model;

vector<float> time_arr;
vector<float> sensor_temperatures;

float speedUp = 0.5;
float simdt = dt * speedUp;
int numIter = 1;



void init()
{   
    ConfigState state = guiState;

    updateFromConfigState(state);
    timePassed = 0;

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                temperature[idx(x, y, z)] = roomTemperature;
                isMaterial[idx(x, y, z)] = 0;
                heatInlet[idx(x, y, z)] = 0;
            }
        }
    }

    temp = temperature;

    //memcpy(temp, temperature, sizeof(temp));
    //memset(heatInlet, 0, sizeof(heatInlet));

    setCylinderHollow();

    // setPlane(); // swap in for a simpler flat-slab validation setup

    numCubes = getNumberCubes();
    mesh = GenMeshIdenticalCubes(drawSize);
    model = LoadModelFromMesh(mesh);

    reset = false;
}

int SelectCubeTargeted(Camera3D camera, Mesh mesh)
{
    Ray ray;

    if (activateMouse) ray = GetScreenToWorldRay(GetMousePosition(), camera);
    else ray = GetScreenToWorldRay({0.5f * GetScreenWidth(), 0.5f * GetScreenHeight()}, camera);

    RayCollision collision = GetRayCollisionMesh(ray, mesh, MatrixIdentity());
    if (!collision.hit) return -1; // sentinel: no cube under the crosshair

    // Nudge the hit point slightly into the cube along -normal so floating-point
    // noise at the exact face boundary can't push it into the neighboring cell.
    Vector3 nudged = Vector3Subtract(collision.point, Vector3Scale(collision.normal, 0.01f * drawSize));
    Vector3 hitPosition = Vector3Scale(nudged, 1.0f / drawSize);

    int x = (int)floorf(hitPosition.x);
    int y = (int)floorf(hitPosition.y);
    int z = (int)floorf(hitPosition.z);

    // Defensive clamp in case the nudge pushes a boundary-adjacent hit outside the grid.
    x = Clamp(x, 0, numX - 1);
    y = Clamp(y, 0, numY - 1);
    z = Clamp(z, 0, numZ - 1);

    return idx(x, y, z);
}

void loop()
{
    if (reset) init();

    if (IsKeyPressed(KEY_C))
    {
        activateMouse = !activateMouse;

        if (activateMouse)
        {
            DisableCursor(); // Capture/lock cursor
        }
        else
        {
            EnableCursor();  // Release cursor
        }
    }

    if (activateMouse)
    {
        UpdateCamera(&camera, CAMERA_FREE);
    }

    if (!activateMouse)
    {
        //UpdateCamera(&camera, CAMERA_FREE);
    }

    if (selectMode)
    {
        guiState.selectedSensor = SelectCubeTargeted(camera, mesh);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) selectMode = false;
    
    }
    

    for (int x = 0; x < numIter; x++) {
        if (!running) break;

        if (timePassed > stopTime && stopTime != 0)
        {
            running = false;
            SaveSensorData(time_arr, sensor_temperatures);
        }

        rotateAndApplyInlet(simdt);
        heatConductionCrankNicolson(simdt, material);
        radiationLoss(simdt);

        timePassed += simdt;

        time_arr.push_back(timePassed);
        sensor_temperatures.push_back(temperature[guiState.selectedSensor]);

    }

    BeginDrawing();

        ClearBackground(WHITE);

        BeginMode3D(camera);

            DrawGrid(50, 1.0f);
            DrawModel(model, {0, 0, 0}, 1, WHITE);
            DrawModelWires(model, {0, 0, 0}, 1, BLACK);

            SetAllCubeColors(mesh, numCubes, 36);

        EndMode3D();

        //Crosshair
        if (selectMode)
        {
            activateMouse = false;
            int centerX = GetScreenWidth() / 2;
            int centerY = GetScreenHeight() / 2;
            DrawCircle(centerX, centerY, 4, WHITE);
        }

        DrawFPS(10, screenHeight - 30);

        MakeImGui();

    EndDrawing();

    printf("\rTimings: heatConduction(%d), Draw(%d), radiationLoss(%d)      ", timings[0], timings[1], timings[2]);
    fflush(stdout);
}

int main()
{
    InitWindow(screenWidth, screenHeight, "Heat Transfer in Pipe");

    #ifdef __EMSCRIPTEN__
        emscripten_set_keydown_callback(
        "#canvas",
        nullptr,
        EM_TRUE,
        KeyCallback
        );
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
