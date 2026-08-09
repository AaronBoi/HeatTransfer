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
//#include <raymath.h>
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

const float dt = 1.0 / 60;
const int width = 250; //num of cells
const int height = 125; //num of cells
const float cellSize = 1.0 * float(screenWidth/width);

//float c = gridsize / dt;
float c = 1;

float tau = 0.6f; //Standard 0.6

int dims = 9;
constexpr float inletVelocity = 0.05f;
constexpr float rho0 = 2.7f;

constexpr float velocityDrawScale = 10.0f;

float n_arr[width][height][9];	//2D array in which cells are densities of the 9 vectors to neighbor cells.
float n_temp[width][height][9];


float w_arr[9] = {4.0/9.0f, 1.0/9.0f, 1.0/36.0f, 1.0/9.0f, 1.0/36.0f, 1.0/9.0f, 1.0/36.0f, 1.0/9.0f, 1.0/36.0f};	//Boltzmann Distribution weights for the neighbor vectors.
float e_arr[9][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};


float rho_arr[width][height];
float u_arr[width][height][2];

int wall_arr[width][height];

int timings[10];

float test = 0;

Vector2 obj_pos;
float obj_radius;


//For collision on GPU
uint32_t compute_shader_id;
uint32_t position_buffer;
uint32_t velocity_buffer;


//general variables for GUI interaction
bool running = true;
bool reset = true;
int periodic_border = false;
bool walls_with_mouse = false;
bool walls_top_bottom = true; 
float cylinder_radius = height / 4;
bool gravity = false;
bool collision_GPU = true;

void init()
{
    memset(n_arr, 1.0, sizeof(n_arr));
    memset(wall_arr, 0.0, sizeof(wall_arr));
    memset(rho_arr, 1.0, sizeof(rho_arr));
    memset(u_arr, 0.0, sizeof(u_arr));

    obj_pos = {5.0, height / 5};
    obj_radius = 1;

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    //unsigned seed = 10;
    default_random_engine generator(seed);
    normal_distribution<float> distribution(0.0, 1.0);

    const float ux = inletVelocity;
    const float uy = 0.0f;

    

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {

            if (walls_top_bottom && (y == 0 || y == height - 1))
            {
                wall_arr[x][y] = 1;
            }
            if (x == 5 && y == 5)
            {
                //wall_arr[x][y] = true;
            }

            if (wall_arr[x][y])
                continue;

            for (int i = 0; i < dims; i++){
                //n_arr[x][y][i] = w_arr[i] * (1.0 + 0.01 * distribution(generator));
                

                const float eDotU = e_arr[i][0] * ux * (1.0 + 0.02 * distribution(generator)) + e_arr[i][0] * uy;
                const float u2 = ux * ux + uy * uy;
                n_arr[x][y][i] = rho0 * w_arr[i] * (1.0f + 3.0f * eDotU + 4.5f * eDotU * eDotU - 1.5f * u2);
            }

        }
    }  
    reset = false;
}




float abs_u_arr[width][height];
float max_u = 0;
Color pixels[width*height];

Texture2D CalculatePixels() {
    auto start = high_resolution_clock::now();
    
    for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
            abs_u_arr[x][y] = magnitude(u_arr[x][y][0], u_arr[x][y][1]);
            if (wall_arr[x][y] == 0 && abs_u_arr[x][y] > max_u)
            {
                max_u = abs_u_arr[x][y];
            }
        }
    }

    for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
            pixels[x + width * y] = ColorFromHSV(abs_u_arr[x][y]/max_u*360.0f, 1, 1);
            //pixels[x + width * y] = ColorFromHSV(abs_u_arr[x][y] * 360 *20, 1, 1);
        }
    }
    Image screenImage = { .data = pixels, .width = width, .height = height, .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture2D texture = LoadTextureFromImage(screenImage);

    auto stop = high_resolution_clock::now();
    timings[4] = duration_cast<chrono::microseconds>(stop - start).count();

    return texture;
}

void DrawDensityAsColor()
{
    float max = 0;
    for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
            //cout << rho_arr[x][y];
            if (rho_arr[x][y] > max)
            {
                //cout << max << endl;
                max = rho_arr[x][y];
            }
        }
    }
    if (max == 0)
        return;
    
    for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
            Color color = ColorFromHSV(rho_arr[x][y]/max*360, 1, 1);
            DrawRectangle(x * cellSize, y * cellSize, cellSize, cellSize, color);
        }
    }
    return;
}

void DrawWall()
{
    for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
            if (wall_arr[x][y] == true)
                DrawRectangle(x * cellSize, y * cellSize, cellSize, cellSize, BLACK);
        }
    }
}

void MakeImGui()
{
    rlImGuiBegin();	            
            
    //IMGUI

    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

    ImGui::SliderFloat("Cylinder radius", &cylinder_radius, 1.0f, 32.0f);            // Edit 1 float using a slider from 0.0f to 1.0f


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

    if (ImGui::Button("Periodic border"))   
    {  
        periodic_border = !periodic_border;
    }
    ImGui::SameLine();
    ImGui::Text("%d", periodic_border);

    if (ImGui::Button("Top and bottom walls on init"))   
    {  
        walls_top_bottom = !walls_top_bottom;
    }
    ImGui::SameLine();
    ImGui::Text("%d", walls_top_bottom);

    ImGui::End();

    rlImGuiEnd();
}

int main() 
{   

    InitWindow(screenWidth, screenHeight, "LBM");
    SetTargetFPS(60);
    SetTraceLogCallback(CustomTraceLog);
    
    rlImGuiSetup(true);
    init();
    
    int i = 0;
    while (!WindowShouldClose())
    {        
        if (reset) init();
        
        for (int x = 0; x < 10; x++) {
            if (!running) break;
            
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
            
            DrawWall();

            DrawCircle(obj_pos.x * cellSize, obj_pos.y * cellSize, obj_radius * cellSize, BLUE);

            DrawFPS(10, screenHeight - 30);

            MakeImGui();	

        EndDrawing();

        
        
        

        Vector2 mouse_pos = GetMousePosition();
        Vector2 mouse_index = {floor(mouse_pos.x / cellSize), floor(mouse_pos.y / cellSize)};
        if (mouse_index.x < width && mouse_index.y < height && walls_with_mouse)
        {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                wall_arr[int(mouse_index.x)][int(mouse_index.y)] = true;
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && wall_arr[int(mouse_index.x)][int(mouse_index.y)] == true)
            {
                wall_arr[int(mouse_index.x)][int(mouse_index.y)] = false;
            }
        }
        
        printf("\rTimings: collision(%d), makros(%d), streaming(%d), drawSpeed(%d)       ", timings[0], timings[1], timings[3], timings[4]);
        fflush(stdout);
        
        i++;
    }
    rlImGuiShutdown();	

    CloseWindow();
}





