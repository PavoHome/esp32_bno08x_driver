#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bno08x_driver.h"

static const char *TAG_MAIN = "APP_MAIN";

// Define GPIOs for the sensors based on pin.md
// Shared SPI pins
#define BNO08X_GPIO_MOSI  GPIO_NUM_23
#define BNO08X_GPIO_MISO  GPIO_NUM_19 // Corrected
#define BNO08X_GPIO_SCLK  GPIO_NUM_18 // Corrected

// BASE Sensor Pins
#define BNO08X_BASE_SENSOR_GPIO_CS    GPIO_NUM_5
#define BNO08X_BASE_SENSOR_GPIO_INT   GPIO_NUM_21
#define BNO08X_BASE_SENSOR_GPIO_RST   GPIO_NUM_22
#define BNO08X_BASE_SENSOR_GPIO_WAKE  GPIO_NUM_NC // Assuming WAKE is not used

// HEAD Sensor Pins (Keep defines but don't use them yet)
#define BNO08X_HEAD_SENSOR_GPIO_CS    GPIO_NUM_4
#define BNO08X_HEAD_SENSOR_GPIO_INT   GPIO_NUM_27
#define BNO08X_HEAD_SENSOR_GPIO_RST   GPIO_NUM_26
#define BNO08X_HEAD_SENSOR_GPIO_WAKE  GPIO_NUM_NC // Assuming WAKE is not used

// Declare BNO08x device handles
BNO08x imu_base;
// BNO08x imu_head; // Commented out

// Callback for BASE Sensor
void imu_base_data_cb(void *arg) {
    BNO08x *imu = (BNO08x *)arg;
    // Example: Get rotation vector data
    float i, j, k, real, acc_rad;
    uint8_t acc_int;
    BNO08x_get_quat(imu, &i, &j, &k, &real, &acc_rad, &acc_int);
    ESP_LOGI("IMU_BASE_CB", "Quat I: %.3f J: %.3f K: %.3f Real: %.3f Acc: %d", i, j, k, real, acc_int);
}

/* // Callback for HEAD Sensor - Commented out
void imu_head_data_cb(void *arg) {
    BNO08x *imu = (BNO08x *)arg;
    // Example: Get rotation vector data
    float i, j, k, real, acc_rad;
    uint8_t acc_int;
    BNO08x_get_quat(imu, &i, &j, &k, &real, &acc_rad, &acc_int);
    ESP_LOGI("IMU_HEAD_CB", "Quat I: %.3f J: %.3f K: %.3f Real: %.3f Acc: %d", i, j, k, real, acc_int);
}
*/

void app_main(void) {
    // Configuration for BASE Sensor
    BNO08x_config_t cfg_base = {
        .spi_peripheral = SPI2_HOST, // Or SPI3_HOST, ensure this is a valid SPI host
        .io_mosi = BNO08X_GPIO_MOSI,
        .io_miso = BNO08X_GPIO_MISO,
        .io_sclk = BNO08X_GPIO_SCLK,
        .io_cs = BNO08X_BASE_SENSOR_GPIO_CS,
        .io_int = BNO08X_BASE_SENSOR_GPIO_INT,
        .io_rst = BNO08X_BASE_SENSOR_GPIO_RST,
        .io_wake = BNO08X_BASE_SENSOR_GPIO_WAKE,
        .sclk_speed = 1000000UL,     // 1 MHz
        .cpu_spi_intr_affinity = 0
    };

    /* // Configuration for HEAD Sensor - Commented out
    BNO08x_config_t cfg_head = {
        .spi_peripheral = SPI2_HOST, // Using the same SPI host as the BASE sensor
        .io_mosi = BNO08X_GPIO_MOSI,
        .io_miso = BNO08X_GPIO_MISO,
        .io_sclk = BNO08X_GPIO_SCLK,
        .io_cs = BNO08X_HEAD_SENSOR_GPIO_CS,
        .io_int = BNO08X_HEAD_SENSOR_GPIO_INT,
        .io_rst = BNO08X_HEAD_SENSOR_GPIO_RST,
        .io_wake = BNO08X_HEAD_SENSOR_GPIO_WAKE,
        .sclk_speed = 3000000UL,     // 3 MHz
        .cpu_spi_intr_affinity = 0
    };
    */

    ESP_LOGI(TAG_MAIN, "Initializing BASE IMU...");
    BNO08x_init(&imu_base, &cfg_base);
    if (BNO08x_initialize(&imu_base)) {
        ESP_LOGI(TAG_MAIN, "BASE IMU Initialized.");
        BNO08x_register_cb(&imu_base, imu_base_data_cb);
        // Enable a report for BASE IMU, e.g., Rotation Vector at 50ms (20Hz)
        BNO08x_enable_rotation_vector(&imu_base, 50000UL);
    } else {
        ESP_LOGE(TAG_MAIN, "Failed to initialize BASE IMU.");
        // Optional: Enter a loop or halt if base fails, to prevent proceeding
        // while(1) { vTaskDelay(portMAX_DELAY); }
    }

    /* // Initializing HEAD IMU - Commented out
    ESP_LOGI(TAG_MAIN, "Initializing HEAD IMU...");
    BNO08x_init(&imu_head, &cfg_head);
    if (BNO08x_initialize(&imu_head)) {
        ESP_LOGI(TAG_MAIN, "HEAD IMU Initialized.");
        BNO08x_register_cb(&imu_head, imu_head_data_cb);
        // Enable a report for HEAD IMU, e.g., Rotation Vector at 50ms (20Hz)
        BNO08x_enable_rotation_vector(&imu_head, 50000UL);
    } else {
        ESP_LOGE(TAG_MAIN, "Failed to initialize HEAD IMU.");
    }
    */

    ESP_LOGI(TAG_MAIN, "Entering main loop...");
    // Your main application loop can go here
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}