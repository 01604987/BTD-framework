#include "preproc.h"
#include "esp_log.h"
#include <math.h>

float running_sum;
float mean;
int mean_counter;
float prev_magnitude_input;


float magnitude(float x, float y, float z) {
    return sqrt(x*x + y*y + z*z);
}

float apply_filter(float prev_output, float curr_input, float prev_input, float* a, float* b){
	float output = a[0] * prev_output + b[0] * curr_input + b[1] * prev_input;
	return output;
}

float preproc_magnitude(float *input, float *outputs, int index, int order){
    // magnitude
    float mag = magnitude(input[0], input[1], input[2]);
    float curr_output;
    float curr_input;
    running_sum += mag;
    // calculate running mean for normalization
    mean = (running_sum) / mean_counter;

    curr_input = mag - mean;

    // TODO calculate prev outputs from end of array if value exists
    if (index < 1) {
        curr_output = curr_input;
    } else {
        curr_output = apply_filter(outputs[index -1], curr_input, prev_magnitude_input, STEP_COEFF_A, STEP_COEFF_B);
    }
 
    mean_counter += 1;
    prev_magnitude_input = curr_input;
    
    return curr_output;
    // TODO finish switch case

    // switch(order - 1){
    //     case 1:
    //         // consider current + last input

    // }
}

void init_preproc(){
    running_sum = 0.0f;
    mean = 0.0f;
    mean_counter = 1;
}
