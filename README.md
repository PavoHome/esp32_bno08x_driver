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

// Define GPIOs for the second sensor
// Ensure these pins are not conflicting with other peripherals or the first sensor
#define BNO08X_SENSOR2_SPI_HOST   SPI2_HOST // Or SPI3_HOST, or the same as sensor 1 if sharing bus
#define BNO08X_SENSOR2_GPIO_MOSI  GPIO_NUM_23 // Example, ensure this matches your SPI host's MOSI
#define BNO08X_SENSOR2_GPIO_MISO  GPIO_NUM_19 // Example, ensure this matches your SPI host's MISO
#define BNO08X_SENSOR2_GPIO_SCLK  GPIO_NUM_18 // Example, ensure this matches your SPI host's SCLK
#define BNO08X_SENSOR2_GPIO_CS    GPIO_NUM_2  // Example - MUST BE DIFFERENT FROM SENSOR 1's CS
#define BNO08X_SENSOR2_GPIO_INT   GPIO_NUM_4  // Example - MUST BE DIFFERENT FROM SENSOR 1's INT
#define BNO08X_SENSOR2_GPIO_RST   GPIO_NUM_15 // Example
#define BNO08X_SENSOR2_GPIO_WAKE  GPIO_NUM_NC // Example, or a valid GPIO if used

// Declare BNO08x device handles
BNO08x imu1;
BNO08x imu2;

// Callback for IMU1
void imu1_data_cb(void *arg) {
    BNO08x *imu = (BNO08x *)arg;
    // Example: Get rotation vector data
    float i, j, k, real, acc_rad;
    uint8_t acc_int;
    BNO08x_get_quat(imu, &i, &j, &k, &real, &acc_rad, &acc_int);
    ESP_LOGI("IMU1_CB", "Quat I: %.3f J: %.3f K: %.3f Real: %.3f Acc: %d", i, j, k, real, acc_int);
}

// Callback for IMU2
void imu2_data_cb(void *arg) {
    BNO08x *imu = (BNO08x *)arg;
    // Example: Get accelerometer data
    float x, y, z;
    uint8_t acc;
    BNO08x_get_accel(imu, &x, &y, &z, &acc);
    ESP_LOGI("IMU2_CB", "Accel X: %.3f Y: %.3f Z: %.3f Acc: %d", x, y, z, acc);
}


void app_main(void)
{
    ESP_LOGI(TAG_MAIN, "Starting application...");

    // Configuration for Sensor 1 (using defaults from menuconfig, or define manually)
    // If using DEFAULT_IMU_CONFIG, ensure menuconfig settings are for your first sensor.
    BNO08x_config_t cfg1 = DEFAULT_IMU_CONFIG;
    // Example of manual configuration for sensor 1 if not using menuconfig defaults:
    /*
    BNO08x_config_t cfg1 = {
        .spi_peripheral = SPI2_HOST, // or SPI3_HOST
        .io_mosi = GPIO_NUM_13,      // ESP32 default VSPI MOSI
        .io_miso = GPIO_NUM_12,      // ESP32 default VSPI MISO
        .io_sclk = GPIO_NUM_14,      // ESP32 default VSPI SCLK
        .io_cs = GPIO_NUM_15,        // CS for sensor 1
        .io_int = GPIO_NUM_27,       // INT for sensor 1
        .io_rst = GPIO_NUM_26,       // RST for sensor 1
        .io_wake = GPIO_NUM_NC,      // WAKE for sensor 1 (if not used)
        .sclk_speed = 3000000UL,     // 3 MHz
        .cpu_spi_intr_affinity = CONFIG_ESP32_BNO08X_SPI_INTR_CPU_AFFINITY // Or 0 or 1
    };
    */


    // Configuration for Sensor 2 (manual configuration)
    BNO08x_config_t cfg2 = {
        .spi_peripheral = BNO08X_SENSOR2_SPI_HOST, // Can be same as cfg1.spi_peripheral if sharing bus
        .io_mosi = BNO08X_SENSOR2_GPIO_MOSI,       // Must be same as cfg1.io_mosi if sharing bus
        .io_miso = BNO08X_SENSOR2_GPIO_MISO,       // Must be same as cfg1.io_miso if sharing bus
        .io_sclk = BNO08X_SENSOR2_GPIO_SCLK,       // Must be same as cfg1.io_sclk if sharing bus
        .io_cs = BNO08X_SENSOR2_GPIO_CS,           // Must be unique
        .io_int = BNO08X_SENSOR2_GPIO_INT,         // Must be unique
        .io_rst = BNO08X_SENSOR2_GPIO_RST,         // Can be unique or shared if reset simultaneously
        .io_wake = BNO08X_SENSOR2_GPIO_WAKE,       // Can be unique or shared
        .sclk_speed = 3000000UL,                   // 3 MHz, must be same as cfg1.sclk_speed if sharing bus
        .cpu_spi_intr_affinity = CONFIG_ESP32_BNO08X_SPI_INTR_CPU_AFFINITY // Or specific core
    };

    ESP_LOGI(TAG_MAIN, "Initializing IMU1...");
    BNO08x_init(&imu1, &cfg1);
    if (BNO08x_initialize(&imu1)) {
        ESP_LOGI(TAG_MAIN, "IMU1 Initialized.");
        BNO08x_register_cb(&imu1, imu1_data_cb);
        // Enable a report for IMU1, e.g., Rotation Vector at 20Hz (50ms)
        BNO08x_enable_rotation_vector(&imu1, 50000UL);
    } else {
        ESP_LOGE(TAG_MAIN, "Failed to initialize IMU1.");
    }

    ESP_LOGI(TAG_MAIN, "Initializing IMU2...");
    BNO08x_init(&imu2, &cfg2);
    if (BNO08x_initialize(&imu2)) {
        ESP_LOGI(TAG_MAIN, "IMU2 Initialized.");
        BNO08x_register_cb(&imu2, imu2_data_cb);
        // Enable a different report for IMU2, e.g., Accelerometer at 10Hz (100ms)
        BNO08x_enable_accelerometer(&imu2, 100000UL);
    } else {
        ESP_LOGE(TAG_MAIN, "Failed to initialize IMU2.");
    }

    ESP_LOGI(TAG_MAIN, "Application main loop started. IMU data will be logged via callbacks.");
    while (1)
    {
        // Main loop can do other things or just delay
        // Data is handled by the callbacks in this example
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```