#include "app_state.h"

bool running = false;
bool reset = true;
bool walls_with_mouse = false;
int  maxDrawTemp = 300;
bool showAsKelvin = true;
bool activateMouse = false;
float timePassed = 0.0f;
float rotationSpeed = 0.0f;
bool selectMode = false;
int stopTime = 0;

ConfigState guiState;
