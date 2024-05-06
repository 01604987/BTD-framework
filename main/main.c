#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"

#include "axp192.h"
#include "sgm2578.h"
#include "st7789.h"
#include "fontx.h"
#include "tft/bmpfile.h"
#include "tft/decode_jpeg.h"
#include "tft/decode_png.h"
#include "tft/pngle.h"
#include <math.h>
#include <driver/i2c.h>

#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)

// M5stickC-Plus stuff
#if CONFIG_M5STICK_C_PLUS
#define CONFIG_WIDTH 135
#define CONFIG_HEIGHT 240
#define CONFIG_MOSI_GPIO 15
#define CONFIG_SCLK_GPIO 13
#define CONFIG_CS_GPIO 5 
#define CONFIG_DC_GPIO 23
#define CONFIG_RESET_GPIO 18
#define CONFIG_BL_GPIO -1
#define CONFIG_LED_GPIO 10
#define CONFIG_OFFSETX 52
#define CONFIG_OFFSETY 40
#endif

// M5stickC-Plus2 stuff
#if CONFIG_M5STICK_C_PLUS2
#define CONFIG_WIDTH 135
#define CONFIG_HEIGHT 240
#define CONFIG_MOSI_GPIO 15
#define CONFIG_SCLK_GPIO 13
#define CONFIG_CS_GPIO 5 
#define CONFIG_DC_GPIO 14
#define CONFIG_RESET_GPIO 12
#define CONFIG_BL_GPIO -1
#define CONFIG_LED_GPIO 19
#define CONFIG_OFFSETX 52
#define CONFIG_OFFSETY 40
#endif

#define I2C_MASTER_TIMEOUT_MS       1000

# define MPU6886_SENSOR_ADDR                     0x68        /*!< Slave address of the MPU6866 sensor */
# define MPU6886_WHO_AM_I_REG_ADDR               0x75        /*!< Register addresses of the "who am I" register */
# define MPU6886_SMPLRT_DIV_REG_ADDR             0x19
# define MPU6886_CONFIG_REG_ADDR                 0x1A
# define MPU6886_ACCEL_CONFIG_REG_ADDR           0x1C
# define MPU6886_ACCEL_CONFIG_2_REG_ADDR         0x1D
# define MPU6886_FIFO_EN_REG_ADDR                0x23
# define MPU6886_INT_PIN_CFG_REG_ADDR            0x37
# define MPU6886_INT_ENABLE_REG_ADDR             0x38
# define MPU6886_ACCEL_XOUT_REG_ADDR             0x3B
# define MPU6886_USER_CRTL_REG_ADDR              0x6A
# define MPU6886_PWR_MGMT_1_REG_ADDR             0x6B
# define MPU6886_PWR_MGMT_2_REG_ADDR             0x6C

static const char *TAG = "MAIN";

static esp_err_t mpu6886_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_NUM_0, MPU6886_SENSOR_ADDR, &reg_addr, 1, data, len, 10*I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t mpu6886_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};

    ret = i2c_master_write_to_device(I2C_NUM_0, MPU6886_SENSOR_ADDR, write_buf, sizeof(write_buf), 10*I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    return ret;
}

static void init_mpu6886(void)
{
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_PWR_MGMT_1_REG_ADDR, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_PWR_MGMT_1_REG_ADDR, (0x01 << 7)));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_PWR_MGMT_1_REG_ADDR, (0x01 << 0)));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_ACCEL_CONFIG_REG_ADDR, 0x18));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_CONFIG_REG_ADDR, 0x01));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_SMPLRT_DIV_REG_ADDR, 0x05));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_INT_ENABLE_REG_ADDR, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_ACCEL_CONFIG_2_REG_ADDR, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_USER_CRTL_REG_ADDR, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_FIFO_EN_REG_ADDR, 0x00));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_INT_PIN_CFG_REG_ADDR, 0x22));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(mpu6886_register_write_byte(MPU6886_INT_ENABLE_REG_ADDR, 0x01));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "MPU6866 intialized successfully");
    vTaskDelay(10 / portTICK_PERIOD_MS);
}


static void getAccelAdc(int16_t* ax, int16_t* ay, int16_t* az)
{
    uint8_t buf[6];
    mpu6886_register_read(MPU6886_ACCEL_XOUT_REG_ADDR, buf, 6);
    // combine 2 array entries following each other into one variable
    *ax = ((int16_t)buf[0] << 8) | buf[1];
    *ay = ((int16_t)buf[2] << 8) | buf[3];
    *az = ((int16_t)buf[4] << 8) | buf[5];
}

static void getAccelData(float* ax, float* ay, float* az)
{
    int16_t accX = 0;
    int16_t accY = 0;
    int16_t accZ = 0;
    getAccelAdc(&accX, &accY, &accZ);
    float aRes = 16.0 / 32768.0;

    *ax = (float)accX * aRes;
    *ay = (float)accY * aRes;
    *az = (float)accZ * aRes;
}

FontxFile fx16G[2];
FontxFile fx24G[2];
FontxFile fx32G[2];
TFT_t dev;

void init_tft(){
	AXP192_PowerOn();
	AXP192_ScreenBreath(11);
	// set font file

	InitFontx(fx16G,"/fonts/ILGH16XB.FNT",""); // 8x16Dot Gothic
	InitFontx(fx24G,"/fonts/ILGH24XB.FNT",""); // 12x24Dot Gothic
	InitFontx(fx32G,"/fonts/ILGH32XB.FNT",""); // 16x32Dot Gothic
	
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
	lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
}


void clear_screen(){
	lcdFillScreen(&dev, BLACK);
}

void draw_steps(int count){
	char charBuffer[6];
	itoa(count, charBuffer, 10); // base 
	uint16_t color;
	color = WHITE;
	lcdSetFontDirection(&dev, 1);
	uint16_t xpos = CONFIG_WIDTH - CONFIG_WIDTH / 4;
	uint16_t ypos = CONFIG_HEIGHT / 5;
	lcdDrawString(&dev, fx24G, xpos, ypos, (uint8_t*)charBuffer, color);
}

void draw_mag(float mag){
	char buffer[20];
	sprintf(buffer, "%.3f", mag);

	uint16_t color;
	color = WHITE;
	lcdSetFontDirection(&dev, 1);
	uint16_t xpos = CONFIG_WIDTH - CONFIG_WIDTH / 4;
	uint16_t ypos = CONFIG_HEIGHT / 5;
	lcdDrawString(&dev, fx24G, xpos, ypos, (uint8_t*)buffer, color);
}

void draw_mags(float mag_input, float mag_output) {
	char buffer[20];
	sprintf(buffer, "%.7f", mag_input);

	uint16_t color;
	color = WHITE;
	lcdSetFontDirection(&dev, 1);
	uint16_t xpos = CONFIG_WIDTH - CONFIG_WIDTH / 4;
	uint16_t ypos = CONFIG_HEIGHT / 5;
	lcdDrawString(&dev, fx24G, xpos, ypos, (uint8_t*)buffer, color);

	sprintf(buffer, "%.7f", mag_output);
	xpos = CONFIG_WIDTH - CONFIG_WIDTH / 2;
	lcdDrawString(&dev, fx24G, xpos, ypos, (uint8_t*)buffer, color);

}

void draw_text(char* text){
	uint8_t ascii[40];
	uint16_t color;
	color = WHITE;
	lcdSetFontDirection(&dev, 1);
	uint16_t xpos = CONFIG_WIDTH / 3;
	uint16_t ypos = CONFIG_HEIGHT / 4;
	strcpy((char *)ascii, text);
	lcdDrawString(&dev, fx32G, xpos, ypos, ascii, color);
}





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

float magnitude(float x, float y, float z) {
    return sqrt(x*x + y*y + z*z);
}

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



float apply_filter(float prev_output, float curr_input, float prev_input, float* a, float* b){
	float output = a[0] * prev_output + b[0] * curr_input + b[1] * prev_input;
	return output;
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
	i2c_master_init();

	init_mpu6886();
	init_tft();

	float accBuf[3];


	uint8_t Fs = 100;
	uint8_t interval = 50;
	float mag_output[interval];
	float mag_input[2];

	float a[] = {0.8277396};
	float b[] = {0.0861302, 0.0861302};
	
	// based on fft hz
	float threshold = 0.30;


	float running_sum = 0;
	float mean = 0;

	int mag_output_counter = 0; 
	int mag_input_counter = 0;
	int step_counter = 0;
	int interval_steps = 0;
	int feature = 0;

	int mean_counter = 1;
	

	char* walking = "WALKING";
	char* idle = "IDLE";
	while (1) {
		if (mag_output_counter >= interval){
			interval_steps = count_steps(mag_output, interval, threshold);
			if (interval_steps > 0){
				step_counter += interval_steps;
				feature = 1;
				clear_screen();
				draw_steps(step_counter);
				draw_text(walking);
			} else {
				if (feature == 1){
					feature = 0;
					clear_screen();
					draw_steps(step_counter);
					draw_text(idle);
				}
			}
			mag_output_counter = 0;
			continue;
		}


		vTaskDelay(10 / portTICK_PERIOD_MS);
		getAccelData(&accBuf[0], &accBuf[1], &accBuf[2]);
		// calculate magnitude
		float mag = magnitude(accBuf[0], accBuf[1], accBuf[2]);
		running_sum += mag;
		// calculate running mean for normalization
		mean = (running_sum) / mean_counter;

		// calculate normalized input magnitude
		mag_input[mag_input_counter] = mag - mean;

		// output array must be already length = 1 in order to apply current filter
		if (mag_output_counter < 1){
			mag_output[mag_output_counter] = mag_input[mag_input_counter];
		} else {
			// pass in previous input, current input, previous input and both coefficients
			mag_output[mag_output_counter] = apply_filter(mag_output[mag_output_counter - 1], mag_input[mag_input_counter], mag_input[mag_input_counter ^ 1], a, b);
		}
		//clear_screen();
		//draw_mags(mag_input[mag_input_counter], mag_output[mag_output_counter]);
		
		//draw_mag(mean);

		mag_output_counter += 1;
		mag_input_counter = mag_input_counter ^ 1;
		mean_counter += 1;
	}

}
