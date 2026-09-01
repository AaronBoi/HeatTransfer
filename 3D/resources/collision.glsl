#version 430

//layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(std430, binding=0) buffer ssbo0 { float n_arr[]; };
layout(std430, binding=1) buffer ssbo1 { int wall_arr[]; };
layout(std430, binding=2) buffer ssbo2 { float rho_arr[]; };
layout(std430, binding=3) buffer ssbo3 { float u_arr[]; };
layout(std430, binding=4) buffer ssbo4 { float n_temp[]; };

layout(location=0) uniform int width;
layout(location=1) uniform int height;
layout(location=2) uniform float tau;
layout(location=3) uniform float periodic_border;

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
float magic_parameter = 0.2;
float tau_minus = 2.0 * magic_parameter / (2.0 * tau - 1) + 0.5;

void main()
{
    int x = int(gl_GlobalInvocationID[0]);
    int y = int(gl_GlobalInvocationID[1]);

    if (x >= width || y >= height)
        return;
    
    int xy = x * height + y; //convert 2d indexing to 1d indexing. For n_arr: *9 + i
    int index = dims * xy;

    
    //compute macroskopic
    
    rho_arr[xy] = 0;
    u_arr[2 * xy] = 0;
    u_arr[2 * xy + 1] = 0;

    if (wall_arr[xy] == 0) {

        
        for (int i = 0; i < dims; i++) {    
            rho_arr[xy] += n_arr[index + i];
            u_arr[2 * xy] += e_arr[i][0] * n_arr[index + i];
            u_arr[2 * xy + 1] += e_arr[i][1] * n_arr[index + i];
           
        }
        //rho_arr[xy] = clamp(rho_arr[xy], 0.1, 10000.0);
        if (rho_arr[xy] > 1e-4f)
        {
            u_arr[2 * xy] /= rho_arr[xy];
            u_arr[2 * xy + 1] /= rho_arr[xy];
        }
        else
        {
            u_arr[2 * xy] = 0;
            u_arr[2 * xy + 1] = 0; 
        }
    }
    
    
    //collision
    vec2 u = vec2(u_arr[2*xy], u_arr[2*xy + 1]);

    if (wall_arr[xy] == 1) //Bounceback from Wall, switch the 8 vectors
    {
        
        float temp = n_arr[index + 1];
        n_arr[index + 1] = n_arr[index + 5];
        n_arr[index + 5] = temp; 

        temp = n_arr[index + 2];
        n_arr[index + 2] = n_arr[index + 6];
        n_arr[index + 6] = temp; 

        temp = n_arr[index + 3];
        n_arr[index + 3] = n_arr[index + 7];
        n_arr[index + 7] = temp; 

        temp = n_arr[index + 4];
        n_arr[index + 4] = n_arr[index + 8];
        n_arr[index + 8] = temp; 
        
    }
    else
    {
        for (int i = 0; i < dims; i++) {
            
            //compute equilibrium distribution
            float eDotU = e_arr[i][0] * u[0] + e_arr[i][1] * u[1];
            float temp = 1.0 + 3.0 * eDotU + 4.5 * eDotU * eDotU - 1.5 * (u[0] * u[0] + u[1] * u[1]);
            float n_eq = rho_arr[xy] * w_arr[i] * temp;

            //relax distribution toward equilibrium using:

            
            //Bhatnagar-Gross-Krook model
            n_arr[index + i] = n_arr[index + i] - 1 / tau * (n_arr[index + i] - n_eq);
            
            
            //Two Relaxation Times Method
            /*
            int i_flip;
            if (i == 0)
                i_flip = i;
            else
                i_flip = ((i - 1 + 4) % 8) + 1;
                
            float n_symm = 0.5 * (n_arr[index + i] + n_arr[index + i_flip]);
            float n_antisymm = 0.5 * (n_arr[index + i] - n_arr[index + i_flip]);

            eDotU = e_arr[i_flip][0] * u[0] + e_arr[i_flip][1] * u[1];
            temp = 1.0 + 3.0 * eDotU + 4.5 * eDotU * eDotU - 1.5 * (u[0] * u[0] + u[1] * u[1]);
            float n_eq_flip = rho_arr[xy] * w_arr[i_flip] * temp;
            float n_eq_symm = 0.5 * (n_eq + n_eq_flip);
            float n_eq_antisymm = 0.5 * (n_eq - n_eq_flip);

            n_temp[index + i] = n_arr[index + i] - 1.0 / tau * (n_symm - n_eq_symm) - 1.0 / tau_minus * (n_antisymm - n_eq_antisymm);
            */
        }
        
        /*
        for (int i = 0; i < dims; i++) {
            n_arr[index + i] = n_temp[index + i];
        }
        */
    }    
    
}