# SmartWatch V3 — Bug Audit & Findings

**Date:** 2026-02-15
**Scope:** Full review of `SmartWatchV3/` firmware source, touch driver (`bsp_cst816/`), and LVGL/esp-brookesia configuration files.
**Purpose:** Catalog all known defects ahead of a full overhaul.

---

## Summary

| Severity | Count |
|----------|-------|
| Critical | 3 |
| Significant | 3 |
| Minor | 3 |
| **Total** | **9** |

---

## Critical Issues

These will cause visible malfunction on hardware. The firmware cannot work correctly until all three are resolved.

### C-1: `LV_COLOR_16_SWAP` is `0` — display colors are byte-swapped

| | |
|---|---|
| **File** | `SmartWatchV3/lv_conf.h:30` |
| **Current value** | `#define LV_COLOR_16_SWAP 0` |
| **Required value** | `1` |

The ST7789 is connected over SPI. SPI displays receive pixel data one byte at a time and expect big-endian RGB565. The ESP32 is little-endian. With `LV_COLOR_16_SWAP 0`, every pixel has its two bytes reversed. The result is garbled colors across the entire UI — reds and blues are swapped, and most colors appear wrong.

**Fix:** Set `LV_COLOR_16_SWAP` to `1`.

---

### C-2: `NotificationApp` bypasses esp-brookesia screen management

| | |
|---|---|
| **File** | `SmartWatchV3/src/notification_app.h:21, 33` |

```cpp
// Line 21 — creates a raw LVGL screen outside the framework
screen = lv_obj_create(nullptr);
// ...
// Line 33 — force-loads it, overriding esp-brookesia's internal screen stack
lv_scr_load(screen);
```

`WatchFaceApp` does this correctly by using `lv_scr_act()` — the screen that esp-brookesia has already prepared for the app. `NotificationApp` instead creates its own detached screen and force-loads it with `lv_scr_load()`. This fights with the framework's internal screen lifecycle, causing display corruption or hard crashes when the user navigates away from or back to the notification app.

**Fix:** Replace `lv_obj_create(nullptr)` + `lv_scr_load(screen)` with `lv_scr_act()`, matching the pattern used in `WatchFaceApp`.

---

### C-3: `LV_MEM_SIZE` is 48 KB with `LV_MEM_CUSTOM 0` — LVGL will run out of memory

| | |
|---|---|
| **File** | `SmartWatchV3/lv_conf.h:49-52` |
| **Current value** | `LV_MEM_CUSTOM 0`, `LV_MEM_SIZE (48U * 1024U)` |

With `LV_MEM_CUSTOM 0`, LVGL uses a fixed-size internal heap (48 KB) for all widget, style, and draw-buffer allocations. An esp-brookesia phone UI (status bar, navigation bar, app launcher, recents screen, plus installed apps) will easily exceed 48 KB. The board has 8 MB of PSRAM that is currently unused by LVGL.

When the pool is exhausted LVGL silently fails allocations and the UI corrupts or crashes.

**Fix:** Either:
- Set `LV_MEM_CUSTOM 1` and point `LV_MEM_CUSTOM_ALLOC` / `LV_MEM_CUSTOM_FREE` at `ps_malloc` / `free` to use PSRAM, or
- Keep the built-in allocator but raise `LV_MEM_SIZE` significantly (e.g. 256 KB+), placing the pool in PSRAM via `LV_MEM_ADR`.

---

## Significant Issues

Functional bugs that cause incorrect behavior but are not immediately catastrophic.

### S-1: Watch face displays seconds but updates every 60 seconds

| | |
|---|---|
| **File** | `SmartWatchV3/src/watch_apps.h:33, 68` |

```cpp
// Line 33 — timer fires once per minute
update_timer = lv_timer_create(timerCallback, 60000, this);
// Line 68 — format string includes seconds
strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
```

The `:SS` portion of the clock will freeze for up to 59 seconds between refreshes, making the watch face look broken.

**Context:** `docs/COMMON_PROBLEMS_AND_FIXES.md` (issue #1) documents that the timer was deliberately slowed from 1 s to 60 s to reduce UI redraws. The format string was not updated to match.

**Fix:** Either change the format to `"%H:%M"` or restore the timer interval to `1000` ms.

---

### S-2: Backlight PWM API mismatch — brightness control breaks after `setup()`

| | |
|---|---|
| **Files** | `SmartWatchV3/SmartWatchV3.ino:109-110` (init) and `SmartWatchV3/src/hardware_config.h:91-127` (runtime) |

During `setup()`, the backlight is configured with the **Arduino LEDC wrapper**:

```cpp
ledcAttach(EXAMPLE_PIN_NUM_LCD_BL, LEDC_FREQ, LEDC_TIMER_10_BIT);
ledcWrite(EXAMPLE_PIN_NUM_LCD_BL, ...);
```

At runtime, `setDisplayBrightness()` (called by `wakeDisplay()` from the sensor task) uses the **ESP-IDF LEDC API**:

```cpp
ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, duty);
ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL);
```

These two APIs configure the LEDC peripheral through different internal paths. The Arduino wrapper sets up the channel/timer its own way; the ESP-IDF calls then target a channel/timer configuration that was never established through the ESP-IDF structs. The result is that `setDisplayBrightness()` has no effect — or worse, corrupts the PWM state.

Additionally, `configureBacklightPWM()` in `hardware_config.h` (which would correctly set up the ESP-IDF side) is **never called** from anywhere.

**Fix:** Pick one API and use it consistently. Either:
- Call `configureBacklightPWM()` in `setup()` instead of `ledcAttach`/`ledcWrite`, and use `setDisplayBrightness()` everywhere, or
- Rewrite `setDisplayBrightness()` to use the Arduino `ledcWrite()` API.

---

### S-3: Wrong error message in touch initialization

| | |
|---|---|
| **File** | `bsp_cst816/bsp_cst816.cpp:68` |

```cpp
Serial.println("QMI8658 read data fail!");
```

This line runs when the **CST816 touch controller** ID check fails. It prints the name of the IMU sensor (QMI8658) instead. A copy-paste error that will mislead anyone debugging touch problems.

**Fix:** Change to `"CST816 read ID fail!"` or similar.

---

## Minor Issues

Code quality problems that increase fragility or confusion but do not directly cause runtime failures in the current build.

### M-1: Unreachable dead code in touch driver

| | |
|---|---|
| **File** | `bsp_cst816/bsp_cst816.cpp:28, 45` |

Both `bsp_touch_i2c_reg8_read()` and `bsp_touch_i2c_reg8_write()` have a `return false;` statement after a `return true;`. The second return is unreachable.

```cpp
// Line 26-28 in bsp_touch_i2c_reg8_read
  return true;

  return false;   // <-- unreachable
```

Same pattern at line 44-46 in `bsp_touch_i2c_reg8_write`.

**Fix:** Remove the dead `return false;` lines.

---

### M-2: Global variables defined (not just declared) in a header file

| | |
|---|---|
| **File** | `SmartWatchV3/src/ble_notifications.h:42-47` |

```cpp
BLEServer *pServer = NULL;
BLEClient *pClient = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
String connectedDeviceName = "";
```

These are full definitions in a header file. The project compiles today only because the header is included from a single translation unit. If a second `.cpp` file ever includes this header (directly or transitively), the linker will fail with multiple-definition errors.

**Fix:** Move definitions to a `.cpp` file and replace them with `extern` declarations in the header.

---

### M-3: Sensor task stack size is only 2048 bytes

| | |
|---|---|
| **File** | `SmartWatchV3/SmartWatchV3.ino:175` |

```cpp
xTaskCreatePinnedToCore(
    sensorTask, "Sensor_Task",
    2048,       // <-- very tight
    ...
```

2048 bytes is marginal for a FreeRTOS task that calls `analogRead()` (which pulls in ADC calibration internals) and will eventually perform I2C reads for the IMU. A stack overflow on the ESP32 causes silent, hard-to-diagnose crashes or watchdog resets.

**Fix:** Increase to at least 4096 bytes.

---

## Notes

- Issues are numbered within each severity tier: **C** = Critical, **S** = Significant, **M** = Minor.
- The existing `docs/COMMON_PROBLEMS_AND_FIXES.md` documents five previously resolved issues. Some of those fixes introduced new bugs listed above (e.g., the 60 s timer change that created S-1).
- This audit covers source-level defects only. It does not cover hardware integration testing, BLE interoperability, or power-management behavior.
