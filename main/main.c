#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// #include "freertos/FreeRTOS.h" //
//#include "freertos/task.h" //
// #include "esp_err.h" //
//#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"


#include "sgm2578.h"
//#include "bmpfile.h" 
//#include "decode_jpeg.h"
//#include "decode_png.h"
//#include "pngle.h"
#include <math.h>
#include "imu.h"
#include "tft.h"
#include "esp_netif.h"
#include "ipv4.h"
#include "protocol_examples_common.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "preproc.h"
#include "postproc.h"
#include "proc_utils.h"

#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)

#define WINDOW 50
#define ORDER_1 2
#define ORDER_2 3

static const char *TAG = "MAIN";
int end = 0;


esp_err_t mountSPIFFS(char * partition_label, char * mount_point) {
	ESP_LOGI(TAG, "Initializing SPIFFS");
	esp_vfs_spiffs_conf_t conf = {
		.base_path = mount_point,
		.partition_label = partition_label,
		.max_files = 10, // maximum number of files which can be open at the same time
		.format_if_mount_failed = true
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return ret;
	}

	size_t total = 0, used = 0;
	//ret = esp_spiffs_info(NULL, &total, &used);
	ret = esp_spiffs_info(partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "Partition [%s] size: total: %d, used: %d", mount_point, total, used);
	}
	return ret;
}

static int getFileSize(char *fullPath) {
	struct stat st;
	if (stat(fullPath, &st) == 0)
		return st.st_size;
	return -1;
}

static void printDirectory(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent *pe = readdir(dir);
		if (!pe) break;
		if (pe->d_type == 1) {
			char fullPath[64];
			strcpy(fullPath, path);
			strcat(fullPath, "/");
			strcat(fullPath, pe->d_name);
			int fsize = getFileSize(fullPath);
			ESP_LOGI(__FUNCTION__,"%s d_name=%s d_ino=%d fsize=%d", path, pe->d_name, pe->d_ino, fsize);
		}
		if (pe->d_type == 2) {
			char subDir[127];
			sprintf(subDir,"%s%.64s", path, pe->d_name);
			ESP_LOGI(TAG, "subDir=[%s]", subDir);
			printDirectory(subDir);

		}
	}
	closedir(dir);
}

// float magnitude(float x, float y, float z) {
//     return sqrt(x*x + y*y + z*z);
// }

// int count_steps(float mag_array[], int size, float threshold) {
//     int step_count = 0;
//     bool above_threshold = false; // Flag to track if the magnitude is above the threshold

//     for (int i = 1; i < size - 1; i++) {
//         if (mag_array[i] > threshold) {

// 			if (mag_array[i-1] < mag_array[i]){
// 				// If magnitude is above the threshold
// 				if (!above_threshold) {
// 					// If not already above threshold, increment step count
// 					step_count++;
// 					above_threshold = true; // Set the flag
// 				}
// 			}
            
//         } else {
//             above_threshold = false; // Reset the flag if magnitude falls below threshold
//         }
//     }

//     return step_count;
// }

// float apply_filter(float prev_output, float curr_input, float prev_input, float* a, float* b){
// 	float output = a[0] * prev_output + b[0] * curr_input + b[1] * prev_input;
// 	return output;
// }

void end_callback(TimerHandle_t xTimer) {
    printf("Timer reached, sending end cmd");
	end = 1;
}

void app_main(void)
{
	// Mount SPIFFS File System on FLASH
	ESP_LOGI(TAG, "Initializing SPIFFS");
	ESP_ERROR_CHECK(mountSPIFFS("storage1", "/fonts"));
	printDirectory("/fonts");
	ESP_ERROR_CHECK(mountSPIFFS("storage2", "/images"));
	printDirectory("/images");

	// Initialize i2c
	init_i2c();
	init_mpu6886();
	init_tft();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

	TimerHandle_t end_timer;
	end_timer = xTimerCreate("EndTimer", pdMS_TO_TICKS(20000), pdFALSE, NULL, end_callback);

	init_preproc();
	while (1) {
		init_network();
		if (connect_to_sock() != 1){
			break;
		};

		// col = 3 because of xyz
		float *input_buf = init_1d_buffer(3);
		// 1D because of magnitude. WINDOW = size of array
		float *output_buf = init_1d_buffer(WINDOW);

		int output_index = 0;

		char* walking = "WALKING";
		char* idle = "IDLE";
		while(1) {

			vTaskDelay(10 / portTICK_PERIOD_MS);
			getAccelData(&input_buf[0], &input_buf[1], &input_buf[2]);

			output_buf[output_index] = preproc_magnitude(input_buf, output_buf, output_index, ORDER_1);


			
			// flip between index 0 and 1 because order_1 filter
			output_index += 1;
		}

	}


	// while (1) {
	// 	init_network();
	// 	if (connect_to_sock() != 1){
	// 		break;
	// 	};

	// 	float accBuf[3];


	// 	uint8_t Fs = 100;
	// 	uint8_t interval = 50;
	// 	float mag_output[interval];
	// 	float mag_input[2];

	// 	float a[] = {0.8277396};
	// 	float b[] = {0.0861302, 0.0861302};
		
	// 	// based on fft hz
	// 	float threshold = 0.30;


	// 	float running_sum = 0;
	// 	float mean = 0;

	// 	int mag_output_counter = 0; 
	// 	int mag_input_counter = 0;
	// 	int step_counter = 0;
	// 	int interval_steps = 0;
	// 	int feature = 0;

	// 	int mean_counter = 1;
		

	// 	char* walking = "WALKING";
	// 	char* idle = "IDLE";
		
	// 	extern int conn_err;

	// 	if (xTimerStart(end_timer, 0) != pdPASS) {
    //     // Timer start failed
    //     // Handle the error
    //     }
	// 	while (1) {

	// 		// TODO use timer instead of sleep, frequency will change due to accumulated processing time + sleep
	// 		if (mag_output_counter >= WINDOW){
	// 			interval_steps = count_steps(mag_output, interval, threshold);
	// 			if (interval_steps > 0){
	// 				step_counter += interval_steps;
	// 				feature = 1;
	// 				clear_screen();
	// 				draw_steps(step_counter);
	// 				draw_text(walking);
	// 			} else {
	// 				if (feature == 1){
	// 					feature = 0;
	// 					clear_screen();
	// 					draw_steps(step_counter);
	// 					draw_text(idle);
	// 				}
	// 			}
	// 			mag_output_counter = 0;
	// 			continue;
	// 		}


	// 		vTaskDelay(10 / portTICK_PERIOD_MS);
	// 		getAccelData(&accBuf[0], &accBuf[1], &accBuf[2]);
	// 		// calculate magnitude
	// 		float mag = magnitude(accBuf[0], accBuf[1], accBuf[2]);
	// 		running_sum += mag;
	// 		// calculate running mean for normalization
	// 		mean = (running_sum) / mean_counter;

	// 		// calculate normalized input magnitude
	// 		mag_input[mag_input_counter] = mag - mean;

	// 		// output array must be already length = 1 in order to apply current filter
	// 		if (mag_output_counter < 1){
	// 			mag_output[mag_output_counter] = mag_input[mag_input_counter];
	// 		} else {
	// 			// pass in previous input, current input, previous input and both coefficients
	// 			mag_output[mag_output_counter] = apply_filter(mag_output[mag_output_counter - 1], mag_input[mag_input_counter], mag_input[mag_input_counter ^ 1], a, b);
	// 		}
	// 		//clear_screen();
	// 		//draw_mags(mag_input[mag_input_counter], mag_output[mag_output_counter]);
			
	// 		//draw_mag(mean);

	// 		mag_output_counter += 1;
	// 		mag_input_counter = mag_input_counter ^ 1;
	// 		mean_counter += 1;

	// 		if (conn_err == 1){
	// 			break;

	// 		} else {
	// 			if (end == 1){
	// 				const char *message = "end";
	// 				send_buf(message, sizeof(message)-1);

	// 				const char *response = recv_buf();
	// 				if (strcmp(response, "Bye!")) {
	// 					break;
	// 				}	

	// 			} else {
	// 				send_buf(accBuf, sizeof(accBuf));
	// 			}
	// 		}
	// 	}

	// 	if(check_conn() == 1){
	// 		ESP_LOGE(TAG, "Shutting down socket and restarting...");
    //         shutdown_conn();
    //         close_sock();
	// 	}
	// }

}
