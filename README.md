# ESP32-S3 ADHD-Friendly SmartWatch (v3)

Firmware for the **Waveshare ESP32-S3-Touch-LCD-2**. Built directly on
**Arduino + Arduino_GFX + LVGL 8.3.11**, following the manufacturer's own
reference examples (`examples/01_factory`, `06_lvgl_battery`,
`07_lvgl_brightness`, `04_qmi8658_output`). There is **no** esp-brookesia /
ESP32_Display_Panel layer — that was removed (see
`docs/COMMON_PROBLEMS_AND_FIXES.md` §2); the UI is plain LVGL.

> ⚠️ **Critical build setting:** this board uses **OPI PSRAM**, and the LVGL
> framebuffers are allocated there. If you build without `PSRAM=opi` the
> display will be blank / crash on boot. Use the pinned profile below
> (`arduino-cli compile -p esp32s3 ...`) or set **PSRAM: "OPI PSRAM"** in the
> Arduino IDE — do not rely on the default board options.

## Hardware

| Item | Detail |
|------|--------|
| Board | Waveshare ESP32-S3-Touch-LCD-2 |
| MCU | ESP32-S3 (dual LX7 @ 240 MHz) |
| RAM | 512 KB SRAM + **8 MB OPI PSRAM** |
| Flash | **16 MB** |
| Display | 2.0″ IPS 240×320, **ST7789T3**, SPI |
| Touch | **CST816** capacitive, I²C @ `0x15` |
| IMU | **QMI8658** 6-axis, I²C @ `0x6B` (shares the touch I²C bus) |
| Battery | 3.7 V Li-Po, ADC on GPIO5 |

### Pin map (from the factory BSP — `examples/01_factory`)

| Signal | GPIO | | Signal | GPIO |
|--------|------|---|--------|------|
| LCD SCLK | 39 | | LCD MOSI | 38 |
| LCD MISO | 40 | | LCD DC | 42 |
| LCD CS | 45 | | LCD RST | -1 |
| LCD backlight | 1 | | Touch/IMU SDA | 48 |
| Touch/IMU SCL | 47 | | Battery ADC | 5 |

Pin/peripheral definitions live in `SmartWatchV3/src/hardware_config.h`.

## Software requirements

- ESP32 Arduino core **3.x** (`esp32:esp32`)
- Libraries: **lvgl 8.3.11**, **GFX Library for Arduino** (Arduino_GFX),
  **ArduinoJson 7.x**. BLE / WiFi / HTTPClient / WebServer ship with the core.
- `bsp_cst816` (the CST816 touch driver) is **vendored** in
  `SmartWatchV3/lib/bsp_cst816/` — no separate install.

## Building

### Option A — arduino-cli with the pinned profile (recommended)

A reproducible build profile is committed in `SmartWatchV3/sketch.yaml`. It
pins the FQBN board options (including `PSRAM=opi`, `FlashSize=16M`, huge-app
partition) so you can't accidentally build without PSRAM:

```bash
cd SmartWatchV3
arduino-cli compile -p esp32s3              # uses sketch.yaml profile
arduino-cli upload  -p /dev/ttyACM0 -b esp32s3   # add --port for your OS
```

First time only, install the toolchain the profile references:

```bash
arduino-cli core update-index \
  --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32
arduino-cli lib install "GFX Library for Arduino" "lvgl@8.3.11" ArduinoJson
```

### Option B — Arduino IDE

Board: **ESP32S3 Dev Module**, with these **Tools** settings:

| Setting | Value |
|---------|-------|
| PSRAM | **OPI PSRAM** |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240MHz (WiFi) |
| Flash Mode | QIO 80MHz |
| Upload Speed | 921600 |
| USB Mode | Hardware CDC and JTAG |

The LVGL config (`SmartWatchV3/lv_conf.h`) is committed with the sketch — do
**not** copy a template `lv_conf.h` into your libraries folder.

## Project layout

```
SmartWatchV3/
├── SmartWatchV3.ino          # display/touch/LVGL bring-up, BLE task, service + screen init
├── sketch.yaml               # pinned arduino-cli build profile (PSRAM=opi, 16M flash)
├── lv_conf.h                 # LVGL 8.3.11 config
├── build_opt.h
├── lib/bsp_cst816/           # vendored CST816 touch driver
└── src/
    ├── hardware_config.h     # pins, backlight (LEDC), battery ADC, power/haptics
    ├── lvgl_port_v8.{h,cpp}  # LVGL port: PSRAM framebuffers, flush/touch cb, Core-1 task
    ├── ble_notifications.{h,cpp}  # BLE server: notifications, task sync, time sync
    ├── services/             # storage, task, focus, notification, connectivity(WiFi/HTTP), time, power
    └── ui/
        ├── theme, screen_manager, gesture
        ├── widgets/status_bar
        └── screens/          # watch_face, task_list, active_task, notification_view, priority_alert, control_panel
```

## How it runs

- `setup()` brings up the ST7789 via Arduino_GFX, the CST816 touch over I²C,
  then LVGL (`lvgl_port_init`), the service layer, the screen manager (starts
  on the watch face), and BLE.
- The **LVGL task** runs on Core 1; the **BLE task** runs on Core 0.
- Phone notifications, task sync, and time sync arrive over **BLE**
  (`ble_notifications.cpp`). WiFi is brought up on demand from the control panel
  for the priority-alert HTTP server and the network-device ("Compressor")
  control.

## Status / known gaps

- IMU step-counting and wrist-raise are **stubs** in `hardware_config.h`
  (`updateStepCount`, `checkWristRaise`) — the QMI8658 driver isn't wired in
  yet. See `examples/04_qmi8658_output` / `05_lvgl_qmi8658`.
- The priority-alert HTTP server only starts after WiFi is enabled in the
  control panel.
- `docs/BUILD_GUIDE.md` still documents the old esp-brookesia flow and is being
  reworked; trust this README and `sketch.yaml` for building.

## License

Provided as-is for personal/educational use.
