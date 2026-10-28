#pragma once
#include <cmath>
#include <vector>
using namespace std;


inline int timings[10];

struct Config {
    //float cylinderLength = 50.0f;       
    float cylinderLength_mm = 50.0f;     
    float cylinderDiameterOutside_mm = 14.0f;
    float wallThickness_mm = 2.0f;
    int cylinder_numY = 30; 
    float roomTemperature = 293.0f;
    float InletPositionAngle = 0.0f; 
    float InletPositionX = 50.0f;
    float heatPerDistancekJcm = 0.15f;
    float rotationSpeedcms = 0.3;
    int selectedSensor = -1;
    float speedUp = 1.0f;
};

struct gridState {
    //int cylinderLength;
    float cylinderRadiusOutside;
    float cylinderRadiusInside;

    int numX;
    int numY;
    int numZ;

    float cellSize;
    float cellVolume;
    float drawSize;

    
    vector<float> temperature;
    vector<float> temp;
    vector<float> heatInlet;
    vector<int>   isMaterial;

    gridState(Config& config) : 
        cylinderRadiusOutside(config.cylinderDiameterOutside_mm / 2000.0f),
        cylinderRadiusInside(cylinderRadiusOutside - config.wallThickness_mm / 1000.0f),
        
        numX(float(config.cylinder_numY) * (config.cylinderLength_mm / 1000.0f) / cylinderRadiusOutside / 2 + 4),
        numY(config.cylinder_numY + 4),
        numZ(config.cylinder_numY + 4),
        cellSize(config.cylinderLength_mm / (1000.0f * numX)),
        cellVolume(pow(cellSize, 3)),
        drawSize((config.cylinderLength_mm / 1000.0f) / numX * 50)
    {
        //reserve if memory err

        temperature.resize(numX * numY * numZ);
        temp.resize(numX * numY * numZ);
        heatInlet.resize(numX * numY * numZ);
        isMaterial.resize(numX * numY * numZ);
    }

};

struct InletState {
    float positionAngle;
    float positionX;
    float heatPerDistance;
    float heatPerSecond = 100.0f;
    float rotationSpeed;

    bool hasActiveCell = false;
    int activeX = 0, activeY = 0, activeZ = 0;

    InletState(Config& config) : 
        positionAngle(config.InletPositionAngle),
        positionX(config.InletPositionX),
        heatPerDistance(config.heatPerDistancekJcm * 1000 * 100),
        rotationSpeed(config.rotationSpeedcms / 100.0f)
        
    {
        if (this->rotationSpeed > 0.001)
        {
            heatPerSecond = heatPerDistance / (rotationSpeed); // J/s
        }
    }
};

struct GuiState {
    bool running = false;
    bool reset = true;
    bool walls_with_mouse = false;
    int  maxDrawTemp = 300;
    bool showAsKelvin = true;
    bool cameraActive = false;
    float timePassed = 0.0f;
    bool selectMode = false;
    int stopTime = 0;
    float speedUp = 1.0f;
};

class Scene
{
private:
    /* data */
public:
    Scene(Config config);
    ~Scene();
    void setCylinderHollow();
    void setPlane();
    
    

    Config config;
    gridState state;
    InletState inlet;
    GuiState gui;

    float dt = 1.0f / 60;
    float StefanBoltzmannConstant = 5.67f * pow(10, -8); // W/(m^2*K^4)

    int idx(int x, int y, int z) const {
        return (x * state.numY + y) * state.numZ + z;
    }

};  

inline Scene::Scene(Config config)
    :config(config), state(config), inlet(config)
{
    gui.speedUp = config.speedUp;

}

inline Scene::~Scene()
{
}

inline void Scene::setCylinderHollow()
{
    int numX = state.numX;
    int numY = state.numY;
    int numZ = state.numZ;

    float middle_y = numY / 2.0;
    float middle_z = numZ / 2.0;
    float radius = numY / 2.0 - 2;

    for (int x = 2; x < numX - 2; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                float midpoint_sq = pow(y - middle_y + 0.5, 2) + pow(z - middle_z + 0.5, 2);
                if (midpoint_sq <= pow(radius, 2))
                {
                    state.isMaterial[idx(x, y, z)] = 1;
                }
            }
        }
    }

    radius = radius * state.cylinderRadiusInside / state.cylinderRadiusOutside - 1;
    for (int x = 0; x < numX; x++) {
        for (int y = 0; y < numY; y++) {
            for (int z = 0; z < numZ; z++) {
                float midpoint_sq = pow(y - middle_y + 0.5, 2) + pow(z - middle_z + 0.5, 2);
                if (midpoint_sq <= pow(radius, 2))
                {
                    state.isMaterial[idx(x, y, z)] = 0;
                }
            }
        }
    }
}

inline void Scene::setPlane()
{
    int numX = state.numX;
    int numY = state.numY;
    int numZ = state.numZ;

    for (int y = 1; y < numY - 1; y++) {
        for (int z = 1; z < numZ - 1; z++) {
            state.isMaterial[idx(numX / 2, y, z)] = 1;
            state.isMaterial[idx(numX / 2 - 1, y, z)] = 1;
            state.isMaterial[idx(numX / 2 + 1, y, z)] = 1;
        }
    }
}
