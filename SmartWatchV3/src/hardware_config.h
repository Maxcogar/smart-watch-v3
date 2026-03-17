/**
 * Hardware configuration for the Waveshare ESP32-S3 Touch LCD 2.0".
 *
 * Pin assignments and peripheral constants come from the manufacturer's
 * reference examples and ESP_Panel_Board_Custom.h — NOT from guesswork.
 *
 * Backlight control uses the Arduino LEDC API (ledcAttach / ledcWrite),
 * matching every working manufacturer example.  Do NOT mix in ESP-IDF
 * ledc_set_duty / ledc_update_duty calls — they conflict with the
 * Arduino wrapper on ESP32 Arduino Core 3.x.
 */

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#ifdef __cplusplus
#include <chrono>
#include <type_traits>
#endif

#include <Arduino.h>
#include <esp_sleep.h>

// ─── Display (ST7789 over SPI) ──────────────────────────────────────
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

// ─── Touch controller (CST816S, I2C) ───────────────────────────────
#define TOUCH_SDA      48
#define TOUCH_SCL      47
#define TOUCH_INT      -1
#define TOUCH_RST      -1
#define TOUCH_I2C_ADDR 0x15

// ─── IMU (QMI8658, shares I2C bus with touch) ──────────────────────
#define IMU_SDA      TOUCH_SDA
#define IMU_SCL      TOUCH_SCL
#define IMU_INT1     -1
#define IMU_INT2     -1
#define IMU_I2C_ADDR 0x6B

// ─── Battery monitoring (voltage divider on GPIO5) ─────────────────
#define BATTERY_PIN            5
#define CHARGE_PIN             -1
#define BATTERY_DIVIDER_RATIO  2.00f
#define BATTERY_MAX_VOLTAGE    4.20f
#define BATTERY_MIN_VOLTAGE    3.00f

// ─── Buttons ────────────────────────────────────────────────────────
#define BUTTON_POWER  0
#define BUTTON_USER   -1

// ─── Backlight PWM (Arduino API — matches manufacturer examples) ───
//     Examples use: ledcAttach(pin, 5000, 10);  ledcWrite(pin, duty);
//     10-bit resolution → duty 0–1023.
#define BACKLIGHT_LEDC_FREQ       5000
#define BACKLIGHT_LEDC_BITS       10        // plain integer, NOT an enum
#define BACKLIGHT_MAX_DUTY        ((1 << BACKLIGHT_LEDC_BITS) - 1)  // 1023

// Convenience brightness levels (0-255 scale, mapped to 0-1023 duty)
#define BRIGHTNESS_MIN    8
#define BRIGHTNESS_LOW    32
#define BRIGHTNESS_MEDIUM 128
#define BRIGHTNESS_HIGH   200
#define BRIGHTNESS_MAX    255

// ─── Function declarations ──────────────────────────────────────────
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

// ─── Internal state ─────────────────────────────────────────────────
namespace hardware_detail {
    inline uint8_t g_backlightLevel = BRIGHTNESS_MEDIUM;
    inline float   g_lastBatteryVoltage = 0.0f;
    inline uint8_t g_lastBatteryPercent = 0;
}

// ─── Backlight ──────────────────────────────────────────────────────
// Uses Arduino ledcWrite — the same API every manufacturer example uses.
inline void setDisplayBrightness(uint8_t brightness)
{
    hardware_detail::g_backlightLevel = constrain(brightness, 0, 255);
    // Map 0-255 to 0-1023 (10-bit duty)
    const uint32_t duty = map(hardware_detail::g_backlightLevel, 0, 255, 0, BACKLIGHT_MAX_DUTY);
    ledcWrite(TFT_BL, duty);
}

// ─── Battery ────────────────────────────────────────────────────────
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
    // Example 06_lvgl_battery uses: voltage = 3.3 / 4096 * analogValue * 3
    // We use the documented 2:1 divider ratio for this board variant.
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

// ─── IMU stubs ──────────────────────────────────────────────────────
inline void updateStepCount()
{
    // TODO: Integrate FastIMU QMI8658 driver for step counting.
    //       See example 04_qmi8658_output and 05_lvgl_qmi8658 for
    //       working IMU init and read patterns on this exact hardware.
}

inline bool checkWristRaise()
{
    // TODO: Implement gesture detection once IMU pipeline is in place.
    return false;
}

// ─── Power management ───────────────────────────────────────────────
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

// ─── Haptics ────────────────────────────────────────────────────────
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
