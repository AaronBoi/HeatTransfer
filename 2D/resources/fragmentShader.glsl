// speed.fs
#version 430
in vec2 fragTexCoord;
out vec4 finalColor;

layout(std430, binding = 3) readonly buffer UBuffer {
    float u_arr[]; // flattened width*height*2 floats
};

uniform int width;
uniform int height;
uniform float maxSpeed;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    int x = int(fragTexCoord.x * float(width));
    int y = int(fragTexCoord.y * float(height));
    int idx = (x + width * y) * 2;

    float ux = u_arr[idx];
    float uy = u_arr[idx + 1];
    float speed = length(vec2(ux, uy));

    float t = clamp(speed / max(maxSpeed, 0.0001), 0.0, 1.0);
    finalColor = vec4(hsv2rgb(vec3((1.0 - t) * 0.66, 1.0, 1.0)), 1.0);
    finalColor = 1;
}