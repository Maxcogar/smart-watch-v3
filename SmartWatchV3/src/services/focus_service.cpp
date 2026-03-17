/**
 * Focus Service — Implementation
 *
 * Uses esp_timer_get_time() for microsecond precision.
 * Records absolute start time; remaining = target - elapsed.
 * No drift from FreeRTOS task scheduling jitter.
 */

#include "focus_service.h"
#include <esp_timer.h>
#include <Arduino.h>

static FocusState _state = FOCUS_IDLE;
static uint32_t _totalDurationUs = 0;     // total in microseconds
static int64_t _startTimeUs = 0;          // when timer started (absolute)
static int64_t _pausedElapsedUs = 0;      // elapsed before pause

void FocusService::init(void) {
    _state = FOCUS_IDLE;
    _totalDurationUs = 0;
    _startTimeUs = 0;
    _pausedElapsedUs = 0;
}

void FocusService::startTimer(uint16_t minutes) {
    _totalDurationUs = (uint32_t)minutes * 60UL * 1000000UL;
    _startTimeUs = esp_timer_get_time();
    _pausedElapsedUs = 0;
    _state = FOCUS_ACTIVE;
    Serial.printf("Focus: started %d min timer\n", minutes);
}

void FocusService::pauseTimer(void) {
    if (_state != FOCUS_ACTIVE) return;
    // Accumulate elapsed time so far
    _pausedElapsedUs += (esp_timer_get_time() - _startTimeUs);
    _state = FOCUS_PAUSED;
    Serial.println("Focus: paused");
}

void FocusService::resumeTimer(void) {
    if (_state != FOCUS_PAUSED) return;
    // Reset start to now; pausedElapsed holds time before pause
    _startTimeUs = esp_timer_get_time();
    _state = FOCUS_ACTIVE;
    Serial.println("Focus: resumed");
}

void FocusService::stopTimer(void) {
    _state = FOCUS_IDLE;
    _totalDurationUs = 0;
    _pausedElapsedUs = 0;
    Serial.println("Focus: stopped");
}

FocusState FocusService::getState(void) {
    return _state;
}

uint32_t FocusService::getRemainingSeconds(void) {
    if (_state == FOCUS_IDLE || _state == FOCUS_COMPLETE) return 0;

    int64_t elapsedUs;
    if (_state == FOCUS_PAUSED) {
        elapsedUs = _pausedElapsedUs;
    } else {
        elapsedUs = _pausedElapsedUs + (esp_timer_get_time() - _startTimeUs);
    }

    if (elapsedUs >= (int64_t)_totalDurationUs) return 0;

    return (uint32_t)((_totalDurationUs - elapsedUs) / 1000000UL);
}

uint32_t FocusService::getTotalSeconds(void) {
    return _totalDurationUs / 1000000UL;
}

uint32_t FocusService::getElapsedSeconds(void) {
    if (_state == FOCUS_IDLE) return 0;

    int64_t elapsedUs;
    if (_state == FOCUS_PAUSED) {
        elapsedUs = _pausedElapsedUs;
    } else {
        elapsedUs = _pausedElapsedUs + (esp_timer_get_time() - _startTimeUs);
    }

    uint32_t elapsed = (uint32_t)(elapsedUs / 1000000UL);
    uint32_t total = _totalDurationUs / 1000000UL;
    return elapsed > total ? total : elapsed;
}

bool FocusService::isFocusShieldActive(void) {
    return _state == FOCUS_ACTIVE;
}

void FocusService::update(void) {
    if (_state != FOCUS_ACTIVE) return;

    int64_t elapsedUs = _pausedElapsedUs + (esp_timer_get_time() - _startTimeUs);
    if (elapsedUs >= (int64_t)_totalDurationUs) {
        _state = FOCUS_COMPLETE;
        Serial.println("Focus: timer complete!");
    }
}
