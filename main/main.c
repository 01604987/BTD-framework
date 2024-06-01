#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// #include "freertos/FreeRTOS.h" //
// #include "freertos/task.h" //
// #include "esp_err.h" //
// #include "freertos/timers.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"

#include "sgm2578.h"
// #include "bmpfile.h"
// #include "decode_jpeg.h"
// #include "decode_png.h"
// #include "pngle.h"
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
#include "buttons.h"

#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)

#define WINDOW 50
#define ORDER_1 2
#define ORDER_2 3

static const char *TAG = "MAIN";
int fetch_flag = 0;
int end = 0;

#define SWIPE_BUFFER_SIZE 5
#define SWIPE_THRESHOLD 6.0

esp_err_t mountSPIFFS(char *partition_label, char *mount_point)
{
	ESP_LOGI(TAG, "Initializing SPIFFS");
	esp_vfs_spiffs_conf_t conf = {
		.base_path = mount_point,
		.partition_label = partition_label,
		.max_files = 10, // maximum number of files which can be open at the same time
		.format_if_mount_failed = true};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
		{
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		}
		else if (ret == ESP_ERR_NOT_FOUND)
		{
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		}
		else
		{
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return ret;
	}

	size_t total = 0, used = 0;
	// ret = esp_spiffs_info(NULL, &total, &used);
	ret = esp_spiffs_info(partition_label, &total, &used);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	}
	else
	{
		ESP_LOGI(TAG, "Partition [%s] size: total: %d, used: %d", mount_point, total, used);
	}
	return ret;
}

static int getFileSize(char *fullPath)
{
	struct stat st;
	if (stat(fullPath, &st) == 0)
		return st.st_size;
	return -1;
}

static void printDirectory(char *path)
{
	DIR *dir = opendir(path);
	assert(dir != NULL);
	while (true)
	{
		struct dirent *pe = readdir(dir);
		if (!pe)
			break;
		if (pe->d_type == 1)
		{
			char fullPath[64];
			strcpy(fullPath, path);
			strcat(fullPath, "/");
			strcat(fullPath, pe->d_name);
			int fsize = getFileSize(fullPath);
			ESP_LOGI(__FUNCTION__, "%s d_name=%s d_ino=%d fsize=%d", path, pe->d_name, pe->d_ino, fsize);
		}
		if (pe->d_type == 2)
		{
			char subDir[127];
			sprintf(subDir, "%s%.64s", path, pe->d_name);
			ESP_LOGI(TAG, "subDir=[%s]", subDir);
			printDirectory(subDir);
		}
	}
	closedir(dir);
}

void end_callback(TimerHandle_t xTimer)
{
	printf("Timer reached, sending end cmd");
	end = 1;
}

void fetch_imu(TimerHandle_t xTimer)
{
	fetch_flag = 1;
}

void clear_buffer(float buffer[][3], int size)
{
	for (int i = 0; i < size; i++)
	{
		buffer[i][0] = 0;
		buffer[i][1] = 0;
		buffer[i][2] = 0;
	}
}

extern volatile button_state_t button_state_index;
extern volatile button_state_t button_state_middle;	
extern enum Switch finger;

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

	init_button();

	TimerHandle_t end_timer;
	end_timer = xTimerCreate("EndTimer", pdMS_TO_TICKS(60000), pdFALSE, NULL, end_callback);

	TimerHandle_t signal_timer;
	signal_timer = xTimerCreate("IMU-Signal", pdMS_TO_TICKS(10), pdTRUE, NULL, fetch_imu);


	while (1)
	{
		clear_screen();
		draw_text((char *)"Initialize Network");
		init_network();
		init_udp();
		if (connect_to_sock() != 1)
		{
			break;
		};
		// col = 3 because of xyz
		float *input_buf = init_1d_buffer(3);
		// 1D because of magnitude. WINDOW = size of array
		float *output_buf = init_1d_buffer(WINDOW);

		// ===================================================================
		int16_t *imu_buf = init_1d_buffer_int(6);
		uint8_t imu_buf_size = 6 * sizeof(int16_t);
		float *imu_buf_float = init_1d_buffer(6);
		uint8_t imu_buf_float_size = 6 * sizeof(float);

		// ===================================================================
		uint8_t output_index = 0;

		// TODO put these 4 following vars into postproc
		char *walking = "WALKING";
		char *idle = "IDLE";
		char *swiped = "SWIPED";

		uint8_t steps = 0;
		uint8_t feature = 0;

		extern uint8_t conn_err;

		if (xTimerStart(end_timer, 0) != pdPASS)
		{
			// Error handling here.
			ESP_LOGE(TAG, "Timer Start failure");
		}

		if (xTimerStart(signal_timer, 0) != pdPASS)
		{
			ESP_LOGE(TAG, "IMU Timer start failuer");
		}

		clear_screen();
		draw_text(idle);

		float swipe_buffer[SWIPE_BUFFER_SIZE][3];
		int swipe_index = 0;

		// main logic loop
		while (1)
		{

			if (fetch_flag == 1)
			{
				getAccelData(&input_buf[0], &input_buf[1], &input_buf[2]);

				switch (finger)
				{
				// TODO slide left right.
				case NONE:
					// Add data to swipe buffer
					swipe_buffer[swipe_index][0] = input_buf[0];
					swipe_buffer[swipe_index][1] = input_buf[1];
					swipe_buffer[swipe_index][2] = input_buf[2];
					swipe_index = (swipe_index + 1) % SWIPE_BUFFER_SIZE;

					// Detect swipe
					swipe_direction_t swipe_direction = detect_swipe(swipe_buffer, SWIPE_BUFFER_SIZE, SWIPE_THRESHOLD);
					if (swipe_direction != SWIPE_NONE)
					{
						if (swipe_direction == SWIPE_LEFT_TO_RIGHT)
						{
							ESP_LOGI(TAG, "Swipe detected: Left to Right");
							clear_screen();
							draw_text((char *)"Right Swipe");
							// Send Command via TCP
							const char *message = "CSR";
							send_buf(message, sizeof(message) - 1);
						}
						else if (swipe_direction == SWIPE_RIGHT_TO_LEFT)
						{
							ESP_LOGI(TAG, "Swipe detected: Right to Left");
							clear_screen();
							draw_text((char *)"Left Swipe");
							// Send Command via TCP
							const char *message = "CSL";
							send_buf(message, sizeof(message) - 1);
						}

						// Delay for one second
						vTaskDelay(pdMS_TO_TICKS(500));

						// Clear buffer and reset index
						clear_buffer(swipe_buffer, SWIPE_BUFFER_SIZE);
						swipe_index = 0;

						// Return to idle display
						clear_screen();
						draw_text(idle);
					}

					// need to keep track of dynamic alloc. mem block size
					// send_buf(input_buf, 3 * sizeof(float));

					break;

				// TODO stream xz acceleration via udp
				case INDEX:
					ESP_LOGE(TAG,"INDEX");
					if (button_state_index == BUTTON_PRESSED)
					{
						ESP_LOGI(TAG, "Index Finger pressed");
					} else if (button_state_index == BUTTON_HOLD) {
						ESP_LOGI(TAG, "Index Finger held");
					}	
					break;

				// TODO originally for pressing esc and fullscreen. may be repurposed for activating left right slider.
				case MIDDLE:
					if (button_state_middle == BUTTON_PRESSED)
					{
						ESP_LOGI(TAG, "Middle Finger pressed");
					} else if (button_state_middle == BUTTON_HOLD) {
						ESP_LOGI(TAG, "Middle Finger held");
					}	
                    break;

				case DOUBLE_TAP_INDEX:
					ESP_LOGE(TAG,"DOUBLE TAP INDEX");
					if (button_state_index == BUTTON_DOUBLE_TAP)
					{
						ESP_LOGI(TAG, "Index Finger DOUBLE TAP");
					} else if (button_state_index == BUTTON_HOLD) {
						ESP_LOGI(TAG, "Index Finger TAP + HOLD");
					} 
                    break;	

				case DOUBLE_TAP_MIDDLE:
					if (button_state_middle == BUTTON_DOUBLE_TAP) {
						ESP_LOGI(TAG, "Middle Finger DOUBLE TAP");
					} 
					else if (button_state_middle == BUTTON_HOLD) {
						ESP_LOGI(TAG, "Middle Finger TAP + HOLD");
					}		
				
				// for debugging purposes
				case DEBUG:
					break;

				// example step detection + raw imu streamer via tcp
				case DEV0:
					// sliding window
					if (output_index >= WINDOW)
					{
						float threshold = 0.30;
						float steps_in_window = count_steps(output_buf, WINDOW, threshold);
						if (steps_in_window > 0)
						{
							steps += steps_in_window;
							feature = 1;
							//! UPDATING SCREEN can take up alot of time and mess with the frequency of the data.
							clear_screen();
							draw_steps(steps);
							draw_text(walking);
						}
						else
						{
							if (feature == 1)
							{
								feature = 0;
								clear_screen();
								draw_steps(steps);
								draw_text(idle);
							}
						}
						output_index = 0;
					}

					// preprocess signal
					output_buf[output_index] = preproc_magnitude(input_buf, output_buf, output_index, ORDER_1);

					output_index += 1;

					// send commands
					if (conn_err == 1)
					{
						break;
					}
					else
					{
						if (end == 1)
						{
							const char *message = "end";
							send_buf(message, sizeof(message) - 1);

							const char *response = recv_buf();
							if (strcmp(response, "Bye!"))
							{
								break;
							}
						}
						else
						{
							// need to keep track of dynamic alloc. mem block size
							send_buf(input_buf, 3 * sizeof(float));
						}
					}

				// send filtered data only
				case DEV1:
					getAccelData(&imu_buf_float[0], &imu_buf_float[1], &imu_buf_float[2]);
					getRotData(&imu_buf_float[3], &imu_buf_float[4], &imu_buf_float[5]);

					if (check_conn() == 0)
					{
						break;
					}
					else
					{
						send_buf_udp(imu_buf_float, imu_buf_float_size);
					}

				default:
					break;
				}

				fetch_flag = 0;
			}
		}

		deinit_1d_buffer(input_buf);
		deinit_1d_buffer(output_buf);
		// remove the end flag
		end = 0;

		if (check_conn() == 1)
		{
			ESP_LOGE(TAG, "Shutting down socket and restarting...");
			shutdown_conn();
			close_sock();
		}
	}
}
