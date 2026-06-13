# Factory Docs Audit — Waveshare ESP32-S3-Touch-LCD-2

**Date:** 2026-06-13
**Scope:** Audit the SmartWatchV3 firmware and project docs against the
manufacturer (OEM/"factory") reference material shipped in this repo:
`examples/01_factory/*` (the factory BSP), `examples/04_qmi8658_output`,
`examples/06_lvgl_battery`, `examples/07_lvgl_brightness`, and the factory
binary `Firmware/ESP32-S3-Touch-LCD-2-factory.bin`.

**Verdict:** The hardware abstraction (`SmartWatchV3/src/hardware_config.h`
and the display/touch init in `SmartWatchV3.ino`) matches the factory pin map
and init APIs almost exactly. The remaining findings are a few stale
constants/comments, a framebuffer strategy that ignores the board's PSRAM,
and project docs (README/PRD) that describe a different architecture and an
incorrect memory model than what the factory hardware and the actual code use.

---

## 1. Pin map & peripheral config — verified against factory BSP

Sources of truth: `examples/01_factory/bsp_spi.h`, `bsp_i2c.h`,
`bsp_lv_port.h/.cpp`, `examples/06_lvgl_battery`, `examples/07_lvgl_brightness`,
`examples/04_qmi8658_output`.

| Signal        | Factory value                  | Project value            | Status |
|---------------|--------------------------------|--------------------------|--------|
| LCD SCLK      | 39                             | 39                       | ✅ |
| LCD MOSI      | 38                             | 38                       | ✅ |
| LCD MISO      | 40                             | 40                       | ✅ |
| LCD DC        | 42                             | 42                       | ✅ |
| LCD CS        | 45                             | 45                       | ✅ |
| LCD RST       | -1                             | -1                       | ✅ |
| LCD backlight | GPIO 1                         | 1                        | ✅ |
| Touch SDA     | 48                             | 48                       | ✅ |
| Touch SCL     | 47                             | 47                       | ✅ |
| Touch ctrl    | CST816, addr 0x15              | 0x15                     | ✅ |
| IMU           | QMI8658, addr 0x6B             | 0x6B                     | ✅ |
| Battery ADC   | GPIO 5                         | 5                        | ✅ |
| Panel         | ST7789, IPS, 240×320 native    | 240×320 IPS              | ✅ |
| Rotation      | 1 (landscape 320×240)          | 1                        | ✅ |
| Backlight PWM | `ledcAttach(pin,5000,10)`      | 5000 Hz / 10-bit         | ✅ |

The pin map is correct and the backlight uses the same Arduino LEDC API as
every factory example. No action needed here.

---

## 2. Findings — code vs factory

### 2.1 `TFT_SPI_HOST` is wrong *and* unused (low severity)
`hardware_config.h:27` defines `TFT_SPI_HOST HSPI`. The factory uses **FSPI**
(`SPIClass bsp_spi(FSPI)` in `bsp_spi.cpp`; `Arduino_ESP32SPI(..., FSPI, true)`
in `bsp_lv_port.cpp`). The macro is never referenced anywhere — the live
display bus in `SmartWatchV3.ino` is constructed with the `Arduino_ESP32SPI`
default — so there is no runtime effect, but the constant is misleading.
**Fix:** change to `FSPI` or delete the macro.

### 2.2 Battery ADC: `4095` vs factory `4096`, and comment/code disagree (low)
`hardware_config.h:118` computes `voltage = avg * 3.30f / 4095.0f * ratio`,
while the comments on lines 53 and 117 (and the factory `06_lvgl_battery`)
say `3.3 / 4096 * adc * 3`. Numerically negligible (~0.02 %), but the comment
contradicts the code. **Fix:** use `4096.0f` to match the OEM formula exactly,
or correct the comment. Battery uses default `analogRead` (12-bit / 11 dB
attenuation), consistent with the factory example.

### 2.3 LVGL framebuffer ignores the board's 8 MB PSRAM (medium)
`lvgl_port_v8.cpp` allocates a **1/10-screen partial buffer** and tries
`MALLOC_CAP_INTERNAL` first, with a comment that the "full-screen buffer
(150 KB) exceeds available internal RAM." The **factory** `bsp_lv_port.cpp`
allocates **two full-screen buffers in PSRAM** (`MALLOC_CAP_SPIRAM`,
`240*320*sizeof(lv_color_t)` each) — the board has 8 MB PSRAM precisely so the
framebuffer doesn't compete for internal RAM. **Recommendation:** allocate the
LVGL draw buffer(s) in PSRAM like the factory BSP to restore full/large
buffering and remove the internal-RAM pressure that forced the partial-buffer
workaround.
Also: the file's header comment is now **stale** — it still claims
"Full-screen framebuffer (240*320)" and "`direct_mode = true`," but the code
uses a partial buffer with `direct_mode = false`.

### 2.4 `LV_COLOR_16_SWAP` — consistent, but note the divergence (info)
Project `lv_conf.h` sets `LV_COLOR_16_SWAP 0`, matching the simple LVGL
examples (06/07/08) and the project's `draw16bitRGBBitmap` flush path. The full
`01_factory` firmware uses `LV_COLOR_16_SWAP 1` (paired with a shared-SPI bus).
No bug — but this is the knob to check first if colors ever render inverted/byte-swapped.

---

## 3. Findings — documentation vs hardware reality

### 3.1 README describes an abandoned architecture (medium)
`README.md` instructs builders to install **esp-brookesia**,
**ESP32_Display_Panel**, and **ESP32_IO_Expander**, and documents a
brookesia app-launcher UI (`watch_apps.h`, `notification_app.h`, swipe-up
launcher). Per `docs/COMMON_PROBLEMS_AND_FIXES.md` §2, the firmware was
deliberately **refactored away** from those libraries to `Arduino_GFX` +
`bsp_cst816` + raw LVGL with a services/screens architecture. The README is
stale and will mislead a new builder. **Fix:** rewrite the README to match
`docs/BUILD_GUIDE.md` and the actual `src/` tree. (Harmless historical mentions
of brookesia remain only in comments in `hardware_config.h` and
`screen_manager.h`.)

### 3.2 PRD memory model contradicts the board and the factory examples (medium)
`docs/PRD.md` §7.1 lists "512 KB SRAM, 384 KB ROM — strict optimization
required" and AC 1.1.5 demands ">400 KB available heap," with **no mention of
the 8 MB PSRAM** that the README, the factory examples, and the factory BSP all
rely on. This under-specs the device and is effectively the root cause of the
"full-screen buffer won't fit" issue in §2.3. **Fix:** add 8 MB PSRAM to the
PRD hardware constraints and direct large allocations (framebuffers, JSON
buffers) to PSRAM.

### 3.3 PRD specifies ESP-IDF, project is Arduino (low)
PRD AC 1.1.1 requires building with "ESP-IDF v5.1+ toolchain." The entire
project and **all** factory examples are Arduino framework (`.ino`,
`Arduino_GFX`, `Wire`, `ledc`), built with `arduino-cli`
(`COMMON_PROBLEMS_AND_FIXES.md`). **Fix:** update the PRD to state the Arduino
framework / arduino-cli toolchain.

---

## 4. Secondary behavioral notes (vs PRD, not factory hardware)

- **Focus-mode brightness mismatch.** `power_service.cpp` comments say "40 %"
  but sets `BRIGHTNESS_LOW = 32` (~12.5 %). PRD AC 2.4.3 specifies 40 %
  (~102/255). Either the constant or the comment is wrong.
- **BLE + WiFi run concurrently.** `initializeBLE()` runs unconditionally and
  uses **Bluedroid** (`BLEDevice.h`), while the control panel can bring up WiFi
  STA + a `WebServer` for FR8/FR9. README's "Known Limitations" claim that BLE
  and WiFi "cannot be used simultaneously" is inaccurate (coexistence is
  supported), but it is RAM-heavy on this device. Consider **NimBLE** to cut
  BLE RAM, relevant to stability NFR2 and battery NFR3.
- **FR9 priority-alert HTTP server is off by default.** `handleAlertServer()`
  is polled every loop, but `startAlertServer()` only runs after the user
  manually toggles WiFi on in the control panel
  (`control_panel.cpp`). Out of the box, the HTTP priority alert (FR9) is
  inactive until WiFi is connected.

---

## 5. Recommended priority order

1. (§2.3 / §3.2) Move the LVGL framebuffer to PSRAM and fix the PRD memory
   model — highest impact on stability/performance.
2. (§3.1) Rewrite the README to match the shipped Arduino_GFX architecture.
3. (§2.1, §2.2) Tidy the `TFT_SPI_HOST` and battery-divider constant/comment
   discrepancies.
4. (§4) Reconcile focus-mode brightness, BLE/WiFi coexistence, and the FR9
   server-start path.
