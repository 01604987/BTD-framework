#include "postproc.h"
#include <stdbool.h>
#include <math.h>

#define MIN_SWIPE_DURATION 5  // Minimum number of data points to consider a swipe valid


int count_steps(float mag_array[], int size, float threshold) {
    int step_count = 0;
    bool above_threshold = false; // Flag to track if the magnitude is above the threshold

    for (int i = 1; i < size - 1; i++) {
        if (mag_array[i] > threshold) {

			if (mag_array[i-1] < mag_array[i]){
				// If magnitude is above the threshold
				if (!above_threshold) {
					// If not already above threshold, increment step count
					step_count++;
					above_threshold = true; // Set the flag
				}
			}
            
        } else {
            above_threshold = false; // Reset the flag if magnitude falls below threshold
        }
    }

    return step_count;
}

// Swipe detection function
/*
int detect_swipe(float accel_data[][3], int size, float threshold) {
    for (int i = 1; i < size; i++) {
        float delta_x = accel_data[i][0] - accel_data[i - 1][0];
        float delta_y = accel_data[i][1] - accel_data[i - 1][1];
        float delta_z = accel_data[i][2] - accel_data[i - 1][2];

        if (fabs(delta_x) > threshold || fabs(delta_y) > threshold || fabs(delta_z) > threshold) {
            return true; // Swipe detected
        }
    }
    return false; // No swipe detected
}*/


// Swipe detection function with direction
swipe_direction_t detect_swipe(float accel_data[][3], int size, float threshold) {
    float delta_x_sum = 0;
    int valid_points = 0;

    for (int i = 1; i < size; i++) {
        float delta_x = accel_data[i][0] - accel_data[i - 1][0];
        delta_x_sum += delta_x;
        valid_points++;

        // Ensure a minimum swipe duration
        if (valid_points >= MIN_SWIPE_DURATION && fabs(delta_x_sum) > threshold) {
            if (delta_x_sum > 0) {
                return SWIPE_LEFT_TO_RIGHT;
            } else {
                return SWIPE_RIGHT_TO_LEFT;
            }
        }
    }
    return SWIPE_NONE; // No swipe detected
}