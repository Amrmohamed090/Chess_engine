#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef WIN64
    #include <windows.h>
#else
    #include <sys/time.h>
#endif


#define U64 unsigned long long

// function to create a normal feed forwared layer of neural network

// the layer is simply a matrix, with a dimension of input x output
// for example, if the input is [1,40], and the output is [1,20], the matrix should be [40,20]
// since [1,40]x[40,20] = [1,20]
// do not forget about the bias, which have the same dimension of the output



// define a structure to compact all information about the layer
typedef struct Layer{
    int8_t  dim_row;
    int8_t  dim_col;
    int8_t* iterator;
}Layer;




// function to generate a random uniform distribution from range
static inline float uniform_float(float low, float high) {
    return low + (high - low) * (rand() / (RAND_MAX + 1.0f));
}


float sample_from_normal_distribution(float mean, float std) {
    double u1 = uniform_0_1();
    double u2 = uniform_0_1();

    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);

    return (float)(z0 * std + mean);
}


Layer* make_layer(int input_dim, int output_dim) {

    int size = input_dim * output_dim;
    
    int8_t* weights = (int8_t*)malloc(sizeof(int8_t) * size);
    if (!weights) return NULL;

    // Xavier uniform limit
    float limit = sqrtf(6.0f / (input_dim + output_dim));

    for (int row = 0; row < input_dim; row++) {
        for (int col = 0; col < output_dim; col++) {

            int index = row * output_dim + col;

            // sample Xavier float
            float w = uniform_float(-limit, limit);

            // scale to int8 range
            int scaled = (int)roundf(w / limit * 127.0f);

            // clamp
            if (scaled > 127) scaled = 127;
            if (scaled < -127) scaled = -127;

            weights[index] = (int8_t)scaled;
        }
    }

    return weights;
}

// helper function to print the matrix
void print_matrix_2d(Layer * layer){
    
    // iterate over the first dimension (rows)
    for (int i = 0; i < layer->dim_row; i++){
        
        // iterate over the second dimension (columns)
        for (int j = 0; j < layer->dim_col; j++){
            // print the value by indexing with current_row * dim_col +  current_col
            printf("%d ", layer->iterator[(i*layer->dim_col) + j]);
            }
        // print a new line for the next row
        printf("\n");

    }

}


int main(){

    
    int8_t *layer = make_layer(4,3);
    for (int row = 0; row < 4; row++){
        for (int col = 0; col< 3; col++){
            int index = (3 * row) + col;
            printf("%d ", layer[index]);


        }
        printf("\n");
    }

    return 0;
}