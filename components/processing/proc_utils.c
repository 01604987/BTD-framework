#include "proc_utils.h"

static const char *TAG = "PROC_UTILS";

float* init_1d_buffer(int size){
    float * d_buffer = (float *)malloc(size * sizeof(float));
    if (d_buffer == NULL) {
        ESP_LOGE(TAG, "Error allocating mem for  1d array.");
        return NULL;
    }
    return d_buffer;
}


// example: array of size 50 (row). Each entry is another array of size 3 (col)
float** init_2d_buffer(int row, int col){
    // Allocate memory for the array dynamically using malloc

    float** dd_buffer = (float **)malloc(row * sizeof(float *));
    if (dd_buffer == NULL) {
        ESP_LOGE(TAG, "Error allocating mem for 2d array. Out of mem ?\n");
        return NULL;
    }
    for (int i = 0; i < row; i++) {
        dd_buffer[i] = (float *)malloc(col * sizeof(float));;
        if (dd_buffer[i] == NULL) {
            ESP_LOGE(TAG, "Error allocating sub mem for 2d array. Out of mem ?\n");
            return NULL;
        }
    }
    return dd_buffer;
}

void deinit_2d_buffer(float** buf,int row){

    for (int i = 0; i < row; i++){
        free(buf[i]);
    }
    free(buf);
    buf = NULL;
}

void deinit_1d_buffer(float * buf) {
    free(buf);
    buf = NULL;
}
