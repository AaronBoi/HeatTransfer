#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>
#include <rlgl.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <stdio.h> 
#include <random>
#include <chrono>
#include <raymath.h>
#include <string.h>

using namespace std;
using namespace std::chrono;

void CustomTraceLog(int msgType, const char *text, va_list args)
{
    char timeStr[64] = { 0 };
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s] ", timeStr);

    switch (msgType)
    {
        case LOG_INFO:
            if (string(text).rfind("TEXT", 0) == 0)
                return;
            
             
            printf("[INFO] : "); 
            break;
        case LOG_ERROR: printf("[ERROR]: "); break;
        case LOG_WARNING: printf("[WARN] : "); break;
        case LOG_DEBUG: printf("[DEBUG]: "); break;
        default: break;
    }

    vprintf(text, args);
    printf("\n");
}

struct MaterialConstants {   //Material Constants
    float thermalDiffusivity = 18.8 * pow(10, -6);      //In m^2/s
    float density = 7.87 * pow(10, 3);                 //In kg/m^3
    float specificHeatCapacity = 448;                   //In J/(kg*K)
    float thermalEmissivity = 0.3;
};

MaterialConstants Steel = { //Constants for Steel AlSI 1010
    .thermalDiffusivity = 18.8 * pow(10, -6),
    .density = 7.87 * pow(10, 3),
    .specificHeatCapacity = 448,
    .thermalEmissivity = 0.3,
};

MaterialConstants material = Steel;

constexpr int screenWidth = 1200;
constexpr int screenHeight = 900;

const int dims = 3;
const int directions = 6;

const float dt = 1.0 / 60;
const float cylinderLength = 0.5;
const float cylinderRadiusInside = 0.011 / 2; //in meters
const float cylinderRadiusOutside = 0.014 / 2;
const int numY = 12; //num of cells
const int numX = float(numY) * cylinderLength / cylinderRadiusOutside / 2;
const int numZ = numY;
float cellSize = cylinderLength / numX; //in meters
float cellVolume = pow(cellSize, 3);
float drawSize = 10.0 / numX;

float InletPositionAngle = 0.0; //On which angle the Inlet is currently on the Pipe
float InletPositionX = numX/2;  //Where on the X Axis the Inlet is

//Heating Power
const float heatPerDistancekJcm = 0.15;    //  kJ/cm
const float heatPerDistance = heatPerDistancekJcm * 1000 * 100;      //  J/m
float heatPerSecond = 100;

float roomTemperature = 293.0;


const float StefanBoltzmannConstant = 5.67*pow(10, -8); //in W/(m^2*K^4)

float temperature[numX][numY][numZ];	//2D array for Temperatures
float temp[numX][numY][numZ];          //Temporary storage
float heatInlet[numX][numY][numZ];

int isMaterial[numX][numY][numZ];      //Array to set which points are heat conductive material.

int timings[10];


//general variables for GUI interaction
bool running = true;
bool reset = true;
bool walls_with_mouse = false;
int maxDrawTemp = 300;
bool showAsKelvin = true;
bool activateMouse = false;
float timePassed = 0.0;
float rotationSpeed = 0.0; //degree/s

void setCylinderHollow() {

    float middle_y = numY / 2.0;
    float middle_z = numZ / 2.0;
    float radius = numY / 2.0 - 1; //So there is 1 layer air

    for (int x = 0; x < numX; x++) {
        
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                float midpoint_sq = pow(y - middle_y + 0.5, 2) + pow(z - middle_z + 0.5, 2);
                if (midpoint_sq <= pow(radius, 2))
                {
                    isMaterial[x][y][z] = 1;
                }
            }
        }
    }

    radius = radius * cylinderRadiusInside / cylinderRadiusOutside - 1;
    for (int x = 0; x < numX; x++) {
        
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                float midpoint_sq = pow(y - middle_y + 0.5, 2) + pow(z - middle_z + 0.5, 2);
                if (midpoint_sq <= pow(radius, 2))
                {
                    isMaterial[x][y][z] = 0;
                }
            }
        }
    }
}

Color getGridColor(int x, int y, int z)
{
    float displayTemp = showAsKelvin ? temperature[x][y][z] : temperature[x][y][z] - 273.15;
    return ColorFromHSV(displayTemp / float(maxDrawTemp) * 360, 1, 1);
}

int getNumberCubes()
{
    int num = 0;
    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                if (isMaterial[x][y][z] != 0 && isMaterial[x][y][z] != 1) continue;
                
                
                num += isMaterial[x][y][z];
            }
        }
    }
    return num;
}

static Mesh GenMeshIdenticalCubes(int cubePositions[], int numCubes, float sideLength)
{
    Mesh singleMesh = GenMeshCube(sideLength, sideLength, sideLength);

    int vertsPerCube = singleMesh.triangleCount * 3; // 36 for a cube — one vertex per index entry

    Mesh mesh = { 0 };
    mesh.vertexCount   = vertsPerCube * numCubes;
    mesh.triangleCount = singleMesh.triangleCount * numCubes;

    mesh.vertices  = (float *)RL_MALLOC(3 * mesh.vertexCount * sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(2 * mesh.vertexCount * sizeof(float));
    mesh.normals   = (float *)RL_MALLOC(3 * mesh.vertexCount * sizeof(float));
    mesh.colors    = (unsigned char *)RL_MALLOC(4 *3 * mesh.vertexCount * sizeof(unsigned char));
    mesh.indices   = nullptr;

    for (int i = 0; i < numCubes; i++)
    {
        int vOffset = i * vertsPerCube;

        for (int j = 0; j < vertsPerCube; j++)
        {
            unsigned short srcVert = singleMesh.indices[j]; // which original vertex this slot needs

            mesh.vertices[3 * (vOffset + j) + 0] =
                singleMesh.vertices[3 * srcVert + 0] + cubePositions[3 * i + 0] * sideLength;
            mesh.vertices[3 * (vOffset + j) + 1] =
                singleMesh.vertices[3 * srcVert + 1] + cubePositions[3 * i + 1] * sideLength;
            mesh.vertices[3 * (vOffset + j) + 2] =
                singleMesh.vertices[3 * srcVert + 2] + cubePositions[3 * i + 2] * sideLength;

            mesh.normals[3 * (vOffset + j) + 0] = singleMesh.normals[3 * srcVert + 0];
            mesh.normals[3 * (vOffset + j) + 1] = singleMesh.normals[3 * srcVert + 1];
            mesh.normals[3 * (vOffset + j) + 2] = singleMesh.normals[3 * srcVert + 2];

            mesh.texcoords[2 * (vOffset + j) + 0] = singleMesh.texcoords[2 * srcVert + 0];
            mesh.texcoords[2 * (vOffset + j) + 1] = singleMesh.texcoords[2 * srcVert + 1];
        }
    }

    UnloadMesh(singleMesh);

    UploadMesh(&mesh, false);
    return mesh;
}


void SetAllCubeColors(Mesh &mesh, int numCubes, int vertsPerCube)
{
    auto start = high_resolution_clock::now();

    int i = 0;

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                if (isMaterial[x][y][z] == 0) continue;

                int vOffset = i * vertsPerCube;
                Color c = getGridColor(x, y, z);

                if (heatInlet[x][y][z] != 0)
                {
                    c = ColorFromHSV(0, 0, 0);
                }
                

                for (int k = 0; k < vertsPerCube; k++)
                {
                    mesh.colors[4 * (vOffset + k) + 0] = c.r;
                    mesh.colors[4 * (vOffset + k) + 1] = c.g;
                    mesh.colors[4 * (vOffset + k) + 2] = c.b;
                    mesh.colors[4 * (vOffset + k) + 3] = c.a;
                }


                i++;
            }
        }
    }

    UpdateMeshBuffer(mesh, 3, mesh.colors, 4 * mesh.vertexCount * sizeof(unsigned char), 0);


    auto stop = high_resolution_clock::now();
    timings[1] = duration_cast<chrono::microseconds>(stop - start).count();
}

void init()
{

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                
                temperature[x][y][z] = roomTemperature;
                isMaterial[x][y][z] = 0;
            }
        }
        
    }
    
    //memset(temperature, 293.0, sizeof(temperature));
    memcpy(temp, temperature, sizeof(temp));
    //memset(isMaterial, 0, sizeof(isMaterial));

    memset(heatInlet, 0.0, sizeof(heatInlet));

    setCylinderHollow();

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    //unsigned seed = 10;
    default_random_engine generator(seed);
    normal_distribution<float> distribution(0.0, 1.0);

    /*
    int x = numX/2;
    for (int y = 0; y < numY; y++) {
        for (int z = 0; z < numZ; z++) {
            if (isMaterial[x][y][z] == 0) continue;
            if (y != 0) continue;
            heatInlet[x][y][z] = heatPerSecond / (material.specificHeatCapacity * material.density * cellVolume);
            //temperature[x][y][z] = 2000;
        }
    }
    */
    

    reset = false;
}

bool findSurfaceCell(float angleDeg, int x, int &yOut, int &zOut)
{
    float angleRad = angleDeg * DEG2RAD;
    float middle_y = numY / 2.0f;
    float middle_z = numZ / 2.0f;
    float outerRadius = numY / 2.0f;

    // Step inward from just past the outer edge until we hit real material
    for (float r = outerRadius; r > 0; r -= 0.25f)
    {
        int y = (int)floorf(middle_y + r * sinf(angleRad));
        int z = (int)floorf(middle_z + r * cosf(angleRad));

        y = std::max(0, std::min(numY - 1, y));
        z = std::max(0, std::min(numZ - 1, z));

        if (isMaterial[x][y][z] == 1)
        {
            yOut = y;
            zOut = z;
            return true;
        }
    }
    return false; // shouldn't happen if x is a valid cylinder cross-section
}

float setHeatInletWithRotation(float dt)
{
    int x = InletPositionX;
    int y, z;

    findSurfaceCell(InletPositionAngle, x, y, z);
    heatInlet[x][y][z] = 0;

    InletPositionAngle += rotationSpeed * dt;

    findSurfaceCell(InletPositionAngle, x, y, z);
    heatInlet[x][y][z] = heatPerSecond / (material.specificHeatCapacity * material.density * cellVolume);
    isMaterial[x][y][z] = 1.0;

    return InletPositionAngle;
}

void heatConduction(float dt)
{
    auto start = high_resolution_clock::now();
    int x_p, x_n, y_p, y_n, z_p, z_n;

    float faceSurface = pow(cellSize, 2);
    float mass = pow(cellSize, 3) * material.density;

    for (int x = 0; x < numX; x++) {
		for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {

                if (isMaterial[x][y][z] == 0) continue;
                
                
                //Central Differential quotient. 
                
                x_p = x + 1;
                x_n = x - 1;
                y_p = y + 1;
                y_n = y - 1;
                z_p = z + 1;
                z_n = z - 1;

                if (x_p >= numX) x_p = x;
                if (x_n < 0)     x_n = x;
                if (y_p >= numY) y_p = y;
                if (y_n < 0)     y_n = y;
                if (z_p >= numZ) z_p = z;   
                if (z_n < 0)     z_n = z;

                //Intersurface Thermal Radiation Effects
                //Penetration depth of Steel is only about 120nm, so no intermetallic "fake" conduction effect by thermal radiation.
                float var = 0;

                if (isMaterial[x_p][y][z] == 0) var += pow(temperature[x_p][y][z], 4) - pow(temperature[x][y][z], 4);
                if (isMaterial[x_n][y][z] == 0) var += pow(temperature[x_n][y][z], 4) - pow(temperature[x][y][z], 4);
                if (isMaterial[x][y_p][z] == 0) var += pow(temperature[x][y_p][z], 4) - pow(temperature[x][y][z], 4);
                if (isMaterial[x][y_n][z] == 0) var += pow(temperature[x][y_n][z], 4) - pow(temperature[x][y][z], 4);
                if (isMaterial[x][y][z_p] == 0) var += pow(temperature[x][y][z_p], 4) - pow(temperature[x][y][z], 4);
                if (isMaterial[x][y][z_n] == 0) var += pow(temperature[x][y][z_n], 4) - pow(temperature[x][y][z], 4);

                float radiationHeat = material.thermalEmissivity * faceSurface * StefanBoltzmannConstant * var;
                float radiationTempChange = radiationHeat / (material.specificHeatCapacity * mass);
            

                if (isMaterial[x_p][y][z] == 0) x_p = x;
                if (isMaterial[x_n][y][z] == 0) x_n = x;
                if (isMaterial[x][y_p][z] == 0) y_p = y;
                if (isMaterial[x][y_n][z] == 0) y_n = y;
                if (isMaterial[x][y][z_p] == 0) z_p = z;
                if (isMaterial[x][y][z_n] == 0) z_n = z;

                //Normal Heat Conduction
                float dudx_2 = temperature[x_p][y][z] - 2 * temperature[x][y][z] + temperature[x_n][y][z];
                float dudy_2 = temperature[x][y_p][z] - 2 * temperature[x][y][z] + temperature[x][y_n][z];
                float dudz_2 = temperature[x][y][z_p] - 2 * temperature[x][y][z] + temperature[x][y][z_n];
                
                float laplaceU = 0.5 / pow(cellSize, 2) * (dudx_2 + dudy_2 + dudz_2);

                float dudt = material.thermalDiffusivity * laplaceU + heatInlet[x][y][z];

                temp[x][y][z] = temperature[x][y][z] + dt * (dudt + radiationTempChange);
            }
        }
    }
    memcpy(temperature, temp, sizeof(temperature));

    auto stop = high_resolution_clock::now();
    timings[0] = duration_cast<chrono::microseconds>(stop - start).count();
}


// void radiationDissipation(float dt)
// {
//     float emissivity = 0.3;
//     int numCubes = getNumberCubes();
//     float surface = 2 * PI * (cylinderRadiusInside + cylinderRadiusOutside) * cylinderLength / numCubes;
//     float mass = cylinderLength * PI * (pow(cylinderRadiusInside, 2) + pow(cylinderRadiusOutside, 2)) / numCubes;

//     for (int x = 0; x < numX; x++) {
// 		for (int y = 0; y < numY; y++) {
//             for (int z = 0; z < numZ; z++) {
//                 float emittedHeat = emissivity * surface * StefanBoltzmannConstant * pow(temperature[x][y][z], 4) * dt;
                
//                 temperature[x][y][z] -= emittedHeat / (material.specificHeatCapacity * mass);
//                 //temperature[x][y][z] -= 0.0001;
//                 if (temperature[x][y][z] < 0) temperature[x][y][z] = 0;
//             }

//         }
//     }
// }

vector<float> ThomasAlgorithm()
{

}

void heatConductionCrankNicolson(float dt, MaterialConstants material)
{
    float r1 = material.thermalDiffusivity * dt / cellSize;
    float r2 = r1;
    float r3 = r1;

    
    
}


void DrawCubes()
{

    for (int x = 0; x < numX; x++) {
		for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                if (isMaterial[x][y][z] == 0) continue;
                
                float displayTemp = showAsKelvin ? temperature[x][y][z] : temperature[x][y][z] - 273.15;
                Color color = ColorFromHSV(displayTemp / float(maxDrawTemp) * 360, 1, 1);

                //DrawCube({drawSize * x, drawSize * y , drawSize * z}, drawSize, drawSize, drawSize, color);
                //DrawCubeWires({drawSize * x, drawSize * y , drawSize * z}, drawSize, drawSize, drawSize, BLACK);

            }
        }
    }

}

void MakeImGui()
{
    rlImGuiBegin();	            
            
    //IMGUI

    ImGui::Begin("Simulation GUI");                          // Create a window called "Hello, world!" and append into it.

    if (ImGui::Button("Start"))                           
        running = true;
    if (ImGui::Button("Stop"))                            
        running = false;
    if (ImGui::Button("Reset"))                           
        reset = true;
    if (ImGui::Button("Walls with mouseclick"))   
    {  
        walls_with_mouse = !walls_with_mouse;
    }
    ImGui::SameLine();
    ImGui::Text("%d", walls_with_mouse);

    ImGui::Spacing();
    ImGui::Text("Max Temperature Color: ");
    ImGui::SliderInt("Max Temp Color", &maxDrawTemp, 300, 10000);
    ImGui::Spacing();

    if (ImGui::Button("Kelvin/Celsius"))                           
        showAsKelvin = !showAsKelvin;

    ImGui::SameLine();
    if (showAsKelvin) {
        ImGui::Text("Kelvin");
    }
    else {
        ImGui::Text("Celsius");
    }

    ImGui::Text("Time passed: ");
    ImGui::SameLine();
    ImGui::Text("%f", round(timePassed));
    ImGui::SameLine();
    ImGui::Text(" seconds");

    ImGui::Spacing();
    ImGui::Text("Rotation Speed (°/s):");
    ImGui::InputFloat("Rot Speed", &rotationSpeed);
    ImGui::Spacing();

    ImGui::End();

    rlImGuiEnd();
}

int main() 
{   

    InitWindow(screenWidth, screenHeight, "Heat Transfer 2D");

    Camera3D camera = { 0 };
    camera.position = (Vector3){ -3.0f, 0.5f, 0.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 90.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE; 

    DisableCursor();
    SetTargetFPS(60);
    SetTraceLogCallback(CustomTraceLog);
    
    rlImGuiSetup(true);
    init();
    
    int i = 0;
    float speedUp = 100.0 / float(numX);
    float simdt = dt * speedUp;
    int numIter = 1;
    float subdt = dt / float(numIter);

    int numCubes = getNumberCubes();

    int cubePositions[3 * numCubes] = {};

    int index = 0;
    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                if (isMaterial[x][y][z] == 1)
                {

                    cubePositions[3 * index] = x;
                    cubePositions[3 * index + 1] = y;
                    cubePositions[3 * index + 2] = z;
                    index++;
                }
                
            }
        }
    }

    Mesh mesh = GenMeshIdenticalCubes(cubePositions, numCubes, drawSize);
    Model model = LoadModelFromMesh(mesh);


    while (!WindowShouldClose())
    {        
        

        if (IsKeyPressed(KEY_C))
        {
            activateMouse = !activateMouse;
        }

        if (activateMouse)
        {
            //UpdateCamera(&camera, CAMERA_CUSTOM);
            //EnableCursor();
            ShowCursor();
        }
        else
        {
            UpdateCamera(&camera, CAMERA_FREE);
            DisableCursor();
            HideCursor();
        }
        
        if (reset) init();
        
        if (rotationSpeed > 0.2)
        {
            //heatPerSecond = heatPerDistance / (rotationSpeed * cylinderRadiusOutside ) ;  //  J/s
        }
        
        
        
        for (int x = 0; x < numIter; x++) {
            if (!running) break;
            
            setHeatInletWithRotation(simdt);
            heatConduction(simdt);
            //radiationDissipation(simdt);
            timePassed += simdt;    
            
        }

        
        
        //Mesh mesh = GenMeshCube(drawSize, drawSize, drawSize);
        //Model model = LoadModelFromMesh(mesh);

        //model.transform = MatrixTranslate(0, 2, 0);

        

        
        BeginDrawing();

            ClearBackground(WHITE);

            BeginMode3D(camera);
                
                DrawGrid(50, 1.0f);
                DrawModel(model, {0,0,0}, 1, WHITE);
                DrawModelWires(model, {0,0,0}, 1, BLACK);
                
                //DrawCube({5.0, 0.0, 0.0}, 0.1f, 0.1f, 0.1f, RED);
                //DrawCube({0.0, 4.0, 0.0}, 2.0f, 2.0f, 2.0f, RED);
                //DrawCubes();
                SetAllCubeColors(mesh, numCubes, 36);
                
               
        

            EndMode3D();

            DrawFPS(10, screenHeight - 30);

            MakeImGui();	

        EndDrawing();

        
        printf("\rTimings: heatConduction(%d), Draw(%d)      ", timings[0], timings[1]);
        fflush(stdout);
        
        i++;
    }
    rlImGuiShutdown();	

    CloseWindow();
}





