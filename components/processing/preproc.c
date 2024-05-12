#include "preproc.h"
#include "esp_log.h"
#include <math.h>

float running_sum = 0.0f;
float mean = 0.0f;
int mean_counter = 1;
float curr_input = 0.0f;

float magnitude(float x, float y, float z) {
    return sqrt(x*x + y*y + z*z);
}

float apply_filter(float prev_output, float curr_input, float prev_input, float a[], float b[]){
	float output = a[0] * prev_output + b[0] * curr_input + b[1] * prev_input;
	return output;
}

float preproc_magnitude(float *input, float *outputs, int index, int order){
    // magnitude
    float prev_input = curr_input;
    float mag = magnitude(input[0], input[1], input[2]);
    float curr_output;
    // keep track of running sum
    running_sum += mag;
    // calculate running mean for normalization
    mean = running_sum / mean_counter;

    curr_input = mag - mean;

    // TODO calculate prev outputs from end of array if value exists
    if (index < 1) {
        curr_output = curr_input;
    } else {
        curr_output = apply_filter(outputs[index -1], curr_input, prev_input, STEP_COEFF_A, STEP_COEFF_B);
    }
 
    mean_counter += 1;
    
    return curr_output;
    // TODO finish switch case for higher order

    // switch(order - 1){
    //     case 1:
    //         // consider current + last input

    // }

}




void reset_preproc() {
    running_sum = 0.0f;
    mean = 0.0f;
    mean_counter = 1;
    curr_input = 0;
}

