#ifndef __IMU__
#define __IMU__

#pragma once
#include <stdint.h>
#include "esp_err.h"



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
# define MPU6886_GYRO_XOUT_REG_ADDR              0x43
# define MPU6886_USER_CRTL_REG_ADDR              0x6A
# define MPU6886_PWR_MGMT_1_REG_ADDR             0x6B
# define MPU6886_PWR_MGMT_2_REG_ADDR             0x6C

esp_err_t mpu6886_register_read(uint8_t reg_addr, uint8_t *data, size_t len);
esp_err_t mpu6886_register_write_byte(uint8_t reg_addr, uint8_t data);
void init_mpu6886(void);
void getAccelAdc(int16_t* ax, int16_t* ay, int16_t* az);
void getAccelData(float* ax, float* ay, float* az);
void getRotAdc(int16_t* ax, int16_t* ay, int16_t* az);
void getRotData(float* ax, float* ay, float* az);




#endif /* __IMU__ */
