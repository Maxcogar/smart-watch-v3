/**
 * Hardware configuration for the Waveshare ESP32-S3 Touch LCD 2.0".
 */

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#ifdef __cplusplus
#include <chrono>
#include <type_traits>
#endif

#include <Arduino.h>
#include "esp32-hal-ledc.h"
#include <esp_err.h>
#include <driver/i2c.h>
#include <driver/ledc.h>
#include <esp_sleep.h>

// Display (ST7789 over SPI)
#define TFT_WIDTH    240
#define TFT_HEIGHT   320
#define TFT_SPI_HOST HSPI
#define TFT_MOSI     38
#define TFT_SCLK     39
#define TFT_MISO     40
#define TFT_CS       45
#define TFT_DC       42
#define TFT_RST      -1
#define TFT_BL       1

// Touch controller (CST816 family)
#define TOUCH_SDA    48
#define TOUCH_SCL    47
#define TOUCH_INT    -1
#define TOUCH_RST    -1
#define TOUCH_I2C_ADDR 0x15

// IMU (QMI8658 shares the same I2C bus)
#define IMU_SDA      TOUCH_SDA
#define IMU_SCL      TOUCH_SCL
#define IMU_INT1     -1
#define IMU_INT2     -1
#define IMU_I2C_ADDR 0x6B

// Battery monitoring (voltage divider ~2:1 on GPIO5)
#define BATTERY_PIN  5
#define CHARGE_PIN   -1
#define BATTERY_DIVIDER_RATIO 2.00f
#define BATTERY_MAX_VOLTAGE   4.20f
#define BATTERY_MIN_VOLTAGE   3.00f

// Buttons
#define BUTTON_POWER 0
#define BUTTON_USER  -1

// Backlight PWM
#define BACKLIGHT_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define BACKLIGHT_LEDC_CHANNEL    LEDC_CHANNEL_0
#define BACKLIGHT_LEDC_TIMER      LEDC_TIMER_0
#define BACKLIGHT_LEDC_FREQ       5000
#define BACKLIGHT_LEDC_RES_BITS   LEDC_TIMER_10_BIT
#define BACKLIGHT_LEDC_RESOLUTION_BITS LEDC_TIMER_10_BIT

#define BRIGHTNESS_MIN    8
#define BRIGHTNESS_LOW    32
#define BRIGHTNESS_MEDIUM 128
#define BRIGHTNESS_HIGH   200
#define BRIGHTNESS_MAX    255

// Function declarations
void initializeHardware();
void setDisplayBrightness(uint8_t brightness);
float readBatteryVoltage();
uint8_t getBatteryPercentage();
bool isCharging();
void updateBatteryStatus();
void updateStepCount();
bool checkWristRaise();
void enableLightSleep();
void wakeDisplay();
void vibrateMotor(uint16_t duration_ms);

namespace hardware_detail {
    inline uint8_t g_backlightLevel = BRIGHTNESS_MEDIUM;
    inline float g_lastBatteryVoltage = 0.0f;
    inline uint8_t g_lastBatteryPercent = 0;
}


inline void configureBacklightPWM()
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .duty_resolution = BACKLIGHT_LEDC_RES_BITS,
        .timer_num = BACKLIGHT_LEDC_TIMER,
        .freq_hz = BACKLIGHT_LEDC_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t channel_cfg = {
        .gpio_num = TFT_BL,
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .channel = BACKLIGHT_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BACKLIGHT_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .flags = {.output_invert = 0},
    };
    ledc_channel_config(&channel_cfg);

    setDisplayBrightness(hardware_detail::g_backlightLevel);
}

void initializeHardware() { /* Now handled by ESP_Panel_Library */ }

inline void setDisplayBrightness(uint8_t brightness)
{
    hardware_detail::g_backlightLevel = constrain(brightness, 0, 255);
    const uint16_t maxDuty = (1u << BACKLIGHT_LEDC_RESOLUTION_BITS) - 1;
    const uint32_t duty = map(hardware_detail::g_backlightLevel, 0, 255, 0, maxDuty);
    ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, duty);
    ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL);

}

inline float readBatteryVoltage()
{
    if (BATTERY_PIN < 0) {
        return BATTERY_MAX_VOLTAGE;
    }

    uint32_t accum = 0;
    constexpr uint8_t samples = 8;
    for (uint8_t i = 0; i < samples; ++i) {
        accum += analogRead(BATTERY_PIN);
    }
    const float avg = static_cast<float>(accum) / samples;
    const float voltage = (avg * 3.30f / 4095.0f) * BATTERY_DIVIDER_RATIO;
    return voltage;
}

inline uint8_t getBatteryPercentage()
{
    float voltage = readBatteryVoltage();
    voltage = constrain(voltage, BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE);
    const float percent = ((voltage - BATTERY_MIN_VOLTAGE) /
                           (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0f;
    return static_cast<uint8_t>(roundf(percent));
}

inline bool isCharging()
{
    if (CHARGE_PIN < 0) {
        return false;
    }
    pinMode(CHARGE_PIN, INPUT_PULLUP);
    return digitalRead(CHARGE_PIN) == LOW;
}

inline void updateBatteryStatus()
{
    const unsigned long now = millis();
    static unsigned long last = 0;
    if (now - last < 10000UL) {
        return;
    }
    last = now;

    hardware_detail::g_lastBatteryVoltage = readBatteryVoltage();
    hardware_detail::g_lastBatteryPercent = getBatteryPercentage();
}

inline void updateStepCount()
{
    // TODO: Integrate FastIMU QMI8658 driver for step counting.
}

inline bool checkWristRaise()
{
    // TODO: Implement gesture detection once IMU pipeline is in place.
    return false;
}

inline void enableLightSleep()
{
    setDisplayBrightness(BRIGHTNESS_MIN);
    esp_sleep_enable_timer_wakeup(30ULL * 1000ULL * 1000ULL);
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(BUTTON_POWER), 0);
    esp_light_sleep_start();
    setDisplayBrightness(hardware_detail::g_backlightLevel);
}

inline void wakeDisplay()
{
    setDisplayBrightness(hardware_detail::g_backlightLevel);
}

inline void vibrateMotor(uint16_t duration_ms)
{
    constexpr int MOTOR_PIN = -1;
    if (MOTOR_PIN < 0) {
        return;
    }
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, HIGH);
    delay(duration_ms);
    digitalWrite(MOTOR_PIN, LOW);
}

#endif // HARDWARE_CONFIG_H



