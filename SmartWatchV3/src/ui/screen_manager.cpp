/**
 * Screen Manager — Implementation
 */

#include "screen_manager.h"
#include "theme.h"
#include <Arduino.h>

// Storage
static ScreenDef _screens[SCREEN_COUNT] = {};
static ScreenID _current = SCREEN_WATCH_FACE;
static ScreenID _history[8] = {};
static uint8_t _historyDepth = 0;
static lv_obj_t *_currentScreenObj = NULL;
static bool _overlayActive = false;

// Internal: create a screen and apply theme
static lv_obj_t *_createScreen(ScreenID id) {
    lv_obj_t *scr = lv_obj_create(NULL);
    theme_apply_screen(scr);

    if (_screens[id].create) {
        _screens[id].create(scr);
    }

    return scr;
}

void ScreenManager::init(void) {
    _historyDepth = 0;
    _current = SCREEN_WATCH_FACE;
    _overlayActive = false;

    // Start on watch face if registered
    if (_screens[SCREEN_WATCH_FACE].create) {
        _currentScreenObj = _createScreen(SCREEN_WATCH_FACE);
        lv_scr_load(_currentScreenObj);
    }
}

void ScreenManager::registerScreen(ScreenID id, ScreenCreateFn create, ScreenDestroyFn destroy, const char *name) {
    if (id >= SCREEN_COUNT) return;
    _screens[id].create = create;
    _screens[id].destroy = destroy;
    _screens[id].name = name;
}

void ScreenManager::navigate(ScreenID target, lv_scr_load_anim_t anim, uint32_t duration) {
    if (target >= SCREEN_COUNT || target == _current) return;
    if (!_screens[target].create) {
        Serial.printf("Screen %d not registered\n", target);
        return;
    }

    // Push current to history
    if (_historyDepth < 8) {
        _history[_historyDepth++] = _current;
    }

    // Destroy callback for old screen
    if (_screens[_current].destroy) {
        _screens[_current].destroy();
    }

    // Create new screen
    _current = target;
    _currentScreenObj = _createScreen(target);

    // Animate transition — auto-delete old screen
    lv_scr_load_anim(_currentScreenObj, anim, duration, 0, true);

    Serial.printf("Navigate -> %s\n", _screens[target].name ? _screens[target].name : "?");
}

void ScreenManager::goBack(void) {
    if (_historyDepth == 0) return;

    ScreenID prev = _history[--_historyDepth];

    // Destroy current
    if (_screens[_current].destroy) {
        _screens[_current].destroy();
    }

    // Create previous screen with reverse animation
    _current = prev;
    _currentScreenObj = _createScreen(prev);
    lv_scr_load_anim(_currentScreenObj, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, true);

    Serial.printf("Back -> %s\n", _screens[prev].name ? _screens[prev].name : "?");
}

ScreenID ScreenManager::currentScreen(void) {
    return _current;
}

lv_obj_t *ScreenManager::currentScreenObj(void) {
    return _currentScreenObj;
}

bool ScreenManager::isOverlayActive(void) {
    return _overlayActive;
}
