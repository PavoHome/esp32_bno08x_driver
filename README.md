# ESP-IDF C BNO08x IMU Driver

This project is inspired by [myles-parfeniuk/esp32_BNO08x](https://github.com/myles-parfeniuk/esp32_BNO08x), a C++ BNO08x driver.

The driver has been modified to support multiple BNO08x sensors on the same ESP32.

# Examples

## Reading data via polling with `BNO08x_data_available()`:
```cpp
#include <stdio.h>
#include "bno08x_driver.h"

void app_main(void)
{
    BNO08x imu;                               // imu object handle
    BNO08x_config_t cfg = DEFAULT_IMU_CONFIG; // default IMU config modifiable via idf.py menuconfig esp32_BNO08x

    // initialize SPI peripheral and gpio
    BNO08x_init(&imu, &cfg);
    // initialize BNO08x
    BNO08x_initialize(&imu);

    // enable reports for any data we want to receive
    BNO08x_enable_game_rotation_vector(&imu, 100000UL); // 100,000us == 100ms report interval
    BNO08x_enable_gyro(&imu, 150000UL);                 // 150,000us == 150ms report interval

    while (1)
    {
        //blocks until a valid packet is received or HOST_INT_TIMEOUT_MS occurs
        if (BNO08x_data_available(&imu))
        {
            float x, y, z = 0;

            // angular velocity (Rad/s)
            x = BNO08x_get_gyro_calibrated_velocity_X(&imu);
            y = BNO08x_get_gyro_calibrated_velocity_Y(&imu);
            z = BNO08x_get_gyro_calibrated_velocity_Z(&imu);
            ESP_LOGW("IMU_data", "Angular Velocity: x: %.3f y: %.3f z: %.3f", x, y, z);

            // absolute heading (degrees)
            x = BNO08x_get_roll_deg(&imu);
            y = BNO08x_get_pitch_deg(&imu);
            z = BNO08x_get_yaw_deg(&imu);
            ESP_LOGI("IMU_data", "Euler Angle: x (roll): %.3f y (pitch): %.3f z (yaw): %.3f", x, y, z);
        }
    }
}

```

## Reading data with an automatically invoked callback using `BNO08x_register_cb()`:
```cpp
#include <stdio.h>
#include "bno08x_driver.h"

//imu data callback function, invoked whenever new data is arrived
void imu_data_cb(void *arg)
{
    BNO08x *imu = (BNO08x *)arg;
    if (imu != NULL)
    {
        float x, y, z = 0;

        // angular velocity (Rad/s)
        x = BNO08x_get_gyro_calibrated_velocity_X(imu);
        y = BNO08x_get_gyro_calibrated_velocity_Y(imu);
        z = BNO08x_get_gyro_calibrated_velocity_Z(imu);
        ESP_LOGW("IMU_cb", "Angular Velocity: x: %.3f y: %.3f z: %.3f", x, y, z);

        // absolute heading (degrees)
        x = BNO08x_get_roll_deg(imu);
        y = BNO08x_get_pitch_deg(imu);
        z = BNO08x_get_yaw_deg(imu);
        ESP_LOGI("IMU_cb", "Euler Angle: x (roll): %.3f y (pitch): %.3f z (yaw): %.3f", x, y, z);
    }
}

void app_main(void)
{
    BNO08x imu;                               // imu object handle
    BNO08x_config_t cfg = DEFAULT_IMU_CONFIG; // default IMU config modifiable via idf.py menuconfig esp32_BNO08x

    // initialize SPI peripheral and gpio
    BNO08x_init(&imu, &cfg);
    // initialize BNO08x
    BNO08x_initialize(&imu);

    // register a callback to be executed when new data is available
    BNO08x_register_cb(&imu, imu_data_cb);

    // enable reports for any data we want to receive
    BNO08x_enable_game_rotation_vector(&imu, 100000UL); // 100,000us == 100ms report interval
    BNO08x_enable_gyro(&imu, 150000UL);                 // 150,000us == 150ms report interval

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Keep main task alive
    }
}

```

## Using Two BNO08x Sensors:
To use two (or more) BNO08x sensors, you need to:
1. Define separate `BNO08x` structs for each sensor.
2. Define separate `BNO08x_config_t` structs for each sensor. Ensure that the GPIO pins (especially `io_cs` and `io_int`) are unique for each sensor. They can share the same SPI bus (`spi_peripheral`) or use different ones. If sharing an SPI bus, ensure the `spi_peripheral`, `io_mosi`, `io_miso`, and `io_sclk` are the same for both configurations.
3. Call `BNO08x_init()` and `BNO08x_initialize()` for each sensor.
4. Enable reports and read data (or register callbacks) for each sensor independently.

```cpp
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bno08x_driver.h"

static const char *TAG_MAIN = "APP_MAIN";

// Define GPIOs for the sensors based on pin.md
// Shared SPI pins
#define BNO08X_GPIO_MOSI  GPIO_NUM_23
#define BNO08X_GPIO_MISO  GPIO_NUM_18
#define BNO08X_GPIO_SCLK  GPIO_NUM_19

// BASE Sensor Pins
#define BNO08X_BASE_SENSOR_GPIO_CS    GPIO_NUM_5
#define BNO08X_BASE_SENSOR_GPIO_INT   GPIO_NUM_21
#define BNO08X_BASE_SENSOR_GPIO_RST   GPIO_NUM_22
#define BNO08X_BASE_SENSOR_GPIO_WAKE  GPIO_NUM_NC // Assuming WAKE is not used

// HEAD Sensor Pins
#define BNO08X_HEAD_SENSOR_GPIO_CS    GPIO_NUM_4
#define BNO08X_HEAD_SENSOR_GPIO_INT   GPIO_NUM_27
#define BNO08X_HEAD_SENSOR_GPIO_RST   GPIO_NUM_26
#define BNO08X_HEAD_SENSOR_GPIO_WAKE  GPIO_NUM_NC // Assuming WAKE is not used

// Declare BNO08x device handles
BNO08x imu_base;
BNO08x imu_head;

// Callback for BASE Sensor
void imu_base_data_cb(void *arg) {
    BNO08x *imu = (BNO08x *)arg;
    // Example: Get rotation vector data
    float i, j, k, real, acc_rad;
    uint8_t acc_int;
    BNO08x_get_quat(imu, &i, &j, &k, &real, &acc_rad, &acc_int);
    ESP_LOGI("IMU_BASE_CB", "Quat I: %.3f J: %.3f K: %.3f Real: %.3f Acc: %d", i, j, k, real, acc_int);
}

// Callback for HEAD Sensor
void imu_head_data_cb(void *arg) {
    BNO08x *imu = (BNO08x *)arg;
    // Example: Get rotation vector data
    float i, j, k, real, acc_rad;
    uint8_t acc_int;
    BNO08x_get_quat(imu, &i, &j, &k, &real, &acc_rad, &acc_int);
    ESP_LOGI("IMU_HEAD_CB", "Quat I: %.3f J: %.3f K: %.3f Real: %.3f Acc: %d", i, j, k, real, acc_int);
}

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
        .sclk_speed = 3000000UL,     // 3 MHz
        .cpu_spi_intr_affinity = CONFIG_ESP32_BNO08X_SPI_INTR_CPU_AFFINITY // Or 0 or 1 or a specific core
    };

    // Configuration for HEAD Sensor
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
        .cpu_spi_intr_affinity = CONFIG_ESP32_BNO08X_SPI_INTR_CPU_AFFINITY // Or 0 or 1 or a specific core
    };

    ESP_LOGI(TAG_MAIN, "Initializing BASE IMU...");
    BNO08x_init(&imu_base, &cfg_base);
    if (BNO08x_initialize(&imu_base)) {
        ESP_LOGI(TAG_MAIN, "BASE IMU Initialized.");
        BNO08x_register_cb(&imu_base, imu_base_data_cb);
        // Enable a report for BASE IMU, e.g., Rotation Vector at 50ms (20Hz)
        BNO08x_enable_rotation_vector(&imu_base, 50000UL);
    } else {
        ESP_LOGE(TAG_MAIN, "Failed to initialize BASE IMU.");
    }

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

    // Your main application loop can go here
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}

```