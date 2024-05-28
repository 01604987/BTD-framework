#ifndef __POSTPROC__
#define __POSTPROC__

#include "esp_err.h"

# define ACCEL_ARR_SIZE 3



int count_steps(float mag_array[], int size, float threshold);
//int detect_swipe(float accel_data[][3], int size, float threshold);

typedef enum {
    SWIPE_NONE,
    SWIPE_LEFT_TO_RIGHT,
    SWIPE_RIGHT_TO_LEFT
} swipe_direction_t;

swipe_direction_t detect_swipe(float accel_data[][3], int size, float threshold);



#endif /* __POSTPROC__ */
