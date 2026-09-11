#pragma once

// Mutable runtime/UI state shared between the simulation loop, the renderer,
// and the ImGui panel.
extern bool running;
extern bool reset;
extern bool walls_with_mouse;
extern int  maxDrawTemp;
extern bool showAsKelvin;
extern bool activateMouse;
extern float timePassed;
extern float rotationSpeed; // degrees/second
extern float heatPerDistancekJcm;                          // kJ/cm
