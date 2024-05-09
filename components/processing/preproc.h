#ifndef __PREPROC__
#define __PREPROC__

#include "esp_err.h"

#define ACCEL_ARR_SIZE 3


static const float STEP_COEFF_A[] = {0.8277396};
static const float STEP_COEFF_B[] = {0.0861302, 0.0861302};


float running_sum;
float mean;
int mean_counter;
float prev_magnitude_input;


float preproc_magnitude(float *input, float *outputs, int index, int order);
float magnitude(float x, float y, float z);
float apply_filter(float prev_output, float curr_input, float prev_input, float* a, float* b);
void init_preproc();

#endif /* __PREPROC__ */
