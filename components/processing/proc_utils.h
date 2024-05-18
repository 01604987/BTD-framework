#ifndef __PROC_UTILS__
#define __PROC_UTILS__

#include "esp_err.h"



int16_t* init_1d_buffer_int(int row);
float* init_1d_buffer(int row);
void deinit_1d_buffer(float * buf);

float** init_2d_buffer(int row, int col);
void deinit_2d_buffer(float** buf,int row);


#endif /* __PROC_UTILS__ */
