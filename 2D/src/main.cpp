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

float dot_product(float arr1[], float arr2[], int dim)
{
	float sum = 0;
	for (int i = 0; i < dim; i++) {
		sum += arr1[i] * arr2[i];
	}
	return sum;
}

float magnitude(float x, float y) {
    return sqrt(x*x+y*y);
}

constexpr int screenWidth = 1200;
constexpr int screenHeight = 900;

const int dims = 2;
const int directions = 4;

const float dt = 1.0 / 60;
const float width = 1;
const float height = 0.5; //in meters
const int numX = 50; //num of cells
const int numY = float(numX) / width * height; //num of cells
const float thickness = 5;
const float cellSize = 1.0 * float(screenWidth/numX);
const float deltaX = width / float(numX);



const float StefanBoltzmannConstant = 5.67*pow(10, -8); //in W/(m^2*K^4)

//Constants for Steel AlSI 1010
float thermalDiffusivity = 18.8 * pow(10, -6);      //In m^2/s
float density = 7.87 * pow(10, 3);                 //In kg/m^3
float specificHeatCapacity = 448;                   //In J/(kg*K)


float temperature[numX][numY];	//2D array for Temperatures
float lastTemperature[numX][numY];
float temp[numX][numY];          //Temporary storage
float heatInlet[numX][numY];

int e_arr[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

int isMaterial[numX][numY];      //Array to set which points are heat conductive material.

int timings[10];


//general variables for GUI interaction
bool running = true;
bool reset = true;
bool walls_with_mouse = false;
int maxDrawTemp = 2000;
bool showAsKelvin = true;

void init()
{

    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            temperature[x][y] = 293.0;
        }
        
    }
    
    //memset(temperature, 293.0, sizeof(temperature));
    memcpy(temp, temperature, sizeof(temp));
    memset(isMaterial, 1, sizeof(isMaterial));
    memset(heatInlet, 0.0, sizeof(heatInlet));

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    //unsigned seed = 10;
    default_random_engine generator(seed);
    normal_distribution<float> distribution(0.0, 1.0);

    for (int y = 4; y < numY - 4; y++)
    {
        heatInlet[numX / 2][y] = 200;
        heatInlet[numX / 2 - 1][y] = 200;
        heatInlet[numX / 2 + 1][y] = 200;
    }
    
    

    memcpy(lastTemperature, temperature, sizeof(lastTemperature));
    reset = false;
}

void heatConduction(float dt)
{
    int x_p;
    int x_n;
    int y_p;
    int y_n;

    for (int x = 0; x < numX; x++) {
		for (int y = 0; y < numY; y++) {

            //Central Differential quotient. 
            
            x_p = x + 1;
            x_n = x - 1;
            y_p = y + 1;
            y_n = y - 1;

            if (x_p >= numX || (isMaterial[x_p][y] == 0)) x_p = x;
            if (x_n < 0 || (isMaterial[x_n][y] == 0)) x_n = x;
            if (y_p >= numY || (isMaterial[x][y_p] == 0)) y_p = y;
            if (y_n < 0 || (isMaterial[x][y_n] == 0)) y_n = y;

            float dudx_2 = temperature[x_p][y] - 2 * temperature[x][y] + temperature[x_n][y];
            float dudy_2 = temperature[x][y_p] - 2 * temperature[x][y] + temperature[x][y_n];
            

            float laplaceU = 0.5 / pow(deltaX, 2) * (dudx_2 + dudy_2);

            float dudt = 100.0 * thermalDiffusivity * laplaceU + heatInlet[x][y];

            temp[x][y] = 2 * dt * dudt + temperature[x][y];
        }
    }
    memcpy(temperature, temp, sizeof(temperature));
    memcpy(lastTemperature, temp, sizeof(lastTemperature));
    
}

void radiationDissipation(float dt)
{
    float emissivity = 0.3;
    float surface = 2 * deltaX * deltaX;
    float mass = deltaX * deltaX * thickness;

    for (int x = 0; x < numX; x++) {
		for (int y = 0; y < numY; y++) {

            float emittedHeat = emissivity * surface * StefanBoltzmannConstant * pow(temperature[x][y], 4) * dt;
            temperature[x][y] -= emittedHeat / (specificHeatCapacity * mass);
            //temperature[x][y] -= 0.001 * StefanBoltzmannConstant * pow(temperature[x][y], 4);
            if (temperature[x][y] < 0) temperature[x][y] = 0;
            

        }
    }
}


float maxTemperature = 1;
Color pixels[numX*numY];

Texture2D CalculatePixels() {
    auto start = high_resolution_clock::now();
    
    for (int x = 0; x < numX; x++) {
		for (int y = 0; y < numY; y++) {
            if (isMaterial[x][y] == 1 && temperature[x][y] > maxTemperature)
            {
                maxTemperature = temperature[x][y];
            }
        }
    }

    for (int x = 0; x < numX; x++) {
		for (int y = 0; y < numY; y++) {
            float displayTemp = showAsKelvin ? temperature[x][y] : temperature[x][y] - 273;
            //pixels[x + numX * y] = ColorFromHSV(temperature[x][y]/maxTemperature*360.0f, 1, 1);
            pixels[x + numX * y] = ColorFromHSV(displayTemp / maxDrawTemp * 360, 1, 1);


        }
    }
    Image screenImage = { .data = pixels, .width = numX, .height = numY, .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture2D texture = LoadTextureFromImage(screenImage);

    auto stop = high_resolution_clock::now();
    timings[4] = duration_cast<chrono::microseconds>(stop - start).count();

    return texture;
}

void DrawWall()
{
    for (int x = 0; x < numX; x++) {
		for (int y = 0; y < numY; y++) {
            if (isMaterial[x][y] == false)
                DrawRectangle(x * cellSize, y * cellSize, cellSize, cellSize, BLACK);
        }
    }
}

void DrawTemperatureNumbers()
{
    for (int x = 0; x < numX; x++) {
		for (int y = 0; y < numY; y++) {
            int displayTemp = showAsKelvin ? temperature[x][y] : temperature[x][y] - 273;
            DrawText(TextFormat("%2i", displayTemp), (x + 0.2) * cellSize, (y + 0.5) * cellSize, cellSize / 5, BLACK);
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

    ImGui::SliderInt("Max Temperature Color", &maxDrawTemp, 300, 10000);

    if (ImGui::Button("Kelvin/Celsius"))                           
        showAsKelvin = !showAsKelvin;

    ImGui::SameLine();
    if (showAsKelvin) {
        ImGui::Text("Kelvin");
    }
    else {
        ImGui::Text("Celsius");
    }
    
    
    ImGui::End();

    rlImGuiEnd();
}

int main() 
{   

    InitWindow(screenWidth, screenHeight, "Heat Transfer 2D");
    SetTargetFPS(60);
    SetTraceLogCallback(CustomTraceLog);
    
    rlImGuiSetup(true);
    init();
    
    int i = 0;
    int numIter = 5;
    float subdt = dt / float(numIter);

    while (!WindowShouldClose())
    {        
        if (reset) init();
        
        
        for (int x = 0; x < numIter; x++) {
            if (!running) break;
            
            heatConduction(subdt);
            radiationDissipation(subdt);
            
        }
        
        
        Texture2D texture = CalculatePixels();

        BeginDrawing();

            
            ClearBackground(WHITE);
            //DrawDensityAsColor();
            
            

            
            DrawTexturePro(
                texture,
                (Rectangle){ 0, 0, (float)texture.width, (float)texture.height }, 
                (Rectangle){ 0, 0, (float)texture.width*cellSize, (float)texture.height*cellSize },
                (Vector2) { 0, 0 }, 0, WHITE);
            
            DrawTemperatureNumbers();

            DrawWall();

            

            DrawFPS(10, screenHeight - 30);

            MakeImGui();	

        EndDrawing();

        
        
        

        Vector2 mouse_pos = GetMousePosition();
        Vector2 mouse_index = {floor(mouse_pos.x / cellSize), floor(mouse_pos.y / cellSize)};
        if (mouse_index.x < numX && mouse_index.y < numY && walls_with_mouse)
        {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                isMaterial[int(mouse_index.x)][int(mouse_index.y)] = false;
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && isMaterial[int(mouse_index.x)][int(mouse_index.y)] == false)
            {
                isMaterial[int(mouse_index.x)][int(mouse_index.y)] = true;
            }
        }
        
        printf("\rTimings: collision(%d), makros(%d), streaming(%d), drawSpeed(%d)       ", timings[0], timings[1], timings[3], timings[4]);
        fflush(stdout);
        
        i++;
    }
    rlImGuiShutdown();	

    CloseWindow();
}





