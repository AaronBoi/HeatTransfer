#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>
#include <raymath.h>
#include <cstdio>


#include "logging.h"
#include "material.h"
#include "grid_config.h"
#include "grid_state.h"
#include "app_state.h"
#include "geometry.h"
#include "inlet.h"
#include "heat_solver.h"
#include "mesh_builder.h"
#include "gui.h"

constexpr int screenWidth = 1200;
constexpr int screenHeight = 900;


void init()
{
    timePassed = 0;

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                temperature[x][y][z] = roomTemperature;
                isMaterial[x][y][z] = 0;
            }
        }
    }

    memcpy(temp, temperature, sizeof(temp));
    memset(heatInlet, 0, sizeof(heatInlet));

    setCylinderHollow();
    // setPlane(); // swap in for a simpler flat-slab validation setup

    reset = false;
}


void WriteConfiguration()
{
    
}

int main()
{
    InitWindow(screenWidth, screenHeight, "Heat Transfer 2D");

    Camera3D camera = { 0 };
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

    float speedUp = 1;
    float simdt = dt * speedUp;
    int numIter = 1;

    int numCubes = getNumberCubes();

    Mesh mesh = GenMeshIdenticalCubes(drawSize);
    Model model = LoadModelFromMesh(mesh);

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_C))
        {
            activateMouse = !activateMouse;
        }

        if (activateMouse)
        {
            ShowCursor();
        }
        else
        {
            UpdateCamera(&camera, CAMERA_FREE);
            DisableCursor();
            HideCursor();
        }

        if (reset) init();

        

        for (int x = 0; x < numIter; x++) {
            if (!running) break;

            rotateAndApplyInlet(simdt);
            heatConductionCrankNicolson(simdt, material);

            timePassed += simdt;
        }

        BeginDrawing();

            ClearBackground(WHITE);

            BeginMode3D(camera);

                DrawGrid(50, 1.0f);
                DrawModel(model, {0, 0, 0}, 1, WHITE);
                DrawModelWires(model, {0, 0, 0}, 1, BLACK);

                SetAllCubeColors(mesh, numCubes, 36);

            EndMode3D();

            DrawFPS(10, screenHeight - 30);

            MakeImGui();

        EndDrawing();

        printf("\rTimings: heatConduction(%d), Draw(%d)      ", timings[0], timings[1]);
        fflush(stdout);
    }

    rlImGuiShutdown();
    CloseWindow();
}
