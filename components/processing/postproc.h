#ifndef __POSTPROC__
#define __POSTPROC__

#include "esp_err.h"

# define ACCEL_ARR_SIZE 3



int count_steps(float mag_array[], int size, float threshold);


#endif /* __POSTPROC__ */
