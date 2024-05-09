#include "postproc.h"
#include <stdbool.h>


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
