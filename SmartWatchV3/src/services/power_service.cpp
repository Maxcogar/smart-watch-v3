/**
 * Power Service — Implementation
 *
 * Idle tracking:
 * - After screenTimeout seconds: dim to BRIGHTNESS_MIN
 * - After screenTimeout + 30 seconds: light sleep
 * - Touch wakes and restores brightness
 * - Focus mode: always-on at 40%, no timeout
 */

#include "power_service.h"
#include "../hardware_config.h"
#include <Arduino.h>

static uint32_t _lastActivityMs = 0;
static uint16_t _timeoutSeconds = 30;
static bool _focusMode = false;
static bool _dimmed = false;
static bool _sleeping = false;
static uint8_t _activeBrightness = BRIGHTNESS_HIGH;

void PowerService::init(void) {
    _lastActivityMs = millis();
    _dimmed = false;
    _sleeping = false;
}

void PowerService::update(void) {
    // Focus mode: always on, no timeout
    if (_focusMode) {
        if (_dimmed) {
            _dimmed = false;
            setDisplayBrightness(BRIGHTNESS_LOW);  // 40% during focus
        }
        return;
    }

    if (_timeoutSeconds == 0) return;  // timeout disabled

    uint32_t idleMs = millis() - _lastActivityMs;
    uint32_t timeoutMs = (uint32_t)_timeoutSeconds * 1000UL;

    if (!_dimmed && idleMs > timeoutMs) {
        // Dim screen
        _dimmed = true;
        setDisplayBrightness(BRIGHTNESS_MIN);
        Serial.println("Power: screen dimmed");
    }

    if (_dimmed && !_sleeping && idleMs > timeoutMs + 30000UL) {
        // Enter light sleep
        _sleeping = true;
        Serial.println("Power: entering light sleep");
        enableLightSleep();
        // After wake:
        _sleeping = false;
        _dimmed = false;
        _lastActivityMs = millis();
        setDisplayBrightness(_activeBrightness);
        Serial.println("Power: woke from sleep");
    }
}

void PowerService::resetIdleTimer(void) {
    _lastActivityMs = millis();
    if (_dimmed) {
        _dimmed = false;
        if (_focusMode) {
            setDisplayBrightness(BRIGHTNESS_LOW);
        } else {
            setDisplayBrightness(_activeBrightness);
        }
    }
}

void PowerService::setScreenTimeout(uint16_t seconds) {
    _timeoutSeconds = seconds;
}

void PowerService::setFocusMode(bool active) {
    _focusMode = active;
    if (active) {
        _dimmed = false;
        setDisplayBrightness(BRIGHTNESS_LOW);  // 40%
    } else {
        setDisplayBrightness(_activeBrightness);
    }
}

uint8_t PowerService::getCurrentBrightness(void) {
    return hardware_detail::g_backlightLevel;
}

void PowerService::setActiveBrightness(uint8_t brightness) {
    _activeBrightness = brightness;
    if (!_dimmed && !_focusMode) {
        setDisplayBrightness(brightness);
    }
}

bool PowerService::isDimmed(void) {
    return _dimmed;
}
