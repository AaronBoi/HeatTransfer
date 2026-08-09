#version 430

//layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(std430, binding=0) buffer ssbo0 { float n_arr[]; };
layout(std430, binding=1) buffer ssbo1 { int wall_arr[]; };
layout(std430, binding=4) buffer ssbo4 { float n_temp[]; };

layout(location=0) uniform int width;
layout(location=1) uniform int height;
layout(location=3) uniform int periodic_border;

const int dims = 9;
const float w_arr[9] = float[](4.0/9.0, 1.0/9.0, 1.0/36.0, 1.0/9.0, 1.0/36.0, 1.0/9.0, 1.0/36.0, 1.0/9.0, 1.0/36.0);
const vec2 e_arr[9] = vec2[](
        vec2( 0,  0),
        vec2( 1,  0),
        vec2( 1,  1),
        vec2( 0,  1),
        vec2(-1,  1),
        vec2(-1,  0),
        vec2(-1, -1),
        vec2( 0, -1),
        vec2( 1, -1)
    );

//float n_temp[width * height * 9];


void main()
{
    int x = int(gl_GlobalInvocationID[0]);
    int y = int(gl_GlobalInvocationID[1]);

    if (x >= width || y >= height)
        return;
    
    int xy = x * height + y; //convert 2d indexing to 1d indexing. For n_arr: *9 + i
    int index = dims * xy;
    
    for (int i = 0; i < dims; i++) {
        int nextX = x + int(e_arr[i][0]);
        int nextY = y + int(e_arr[i][1]);

        if (periodic_border == 1) {

            if (nextX < 0)
            {   
                nextX = width - 1;
            }
            if (nextX >= width)
                nextX = 0;
            
            if (nextY < 0)
                nextY = height - 1;
            
            if (nextY >= height)
                nextY = 0;
            
            int nextIndex = dims * (nextX * height + nextY);
            n_temp[nextIndex + i] = n_arr[index + i];
        }
        else {
            int nextIndex = dims * (nextX * height + nextY);
            if (!(nextX < 0 || nextX >= width || nextY < 0 || nextY >= height)) {
                n_temp[nextIndex + i] = n_arr[index + i];
            }
        }


    }
    
}