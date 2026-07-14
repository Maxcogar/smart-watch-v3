# ESP32-S3 SmartWatch - Detailed Build Guide

This guide provides step-by-step instructions for building and configuring your ESP32-S3 smartwatch from scratch.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Hardware Setup](#hardware-setup)
3. [Software Installation](#software-installation)
4. [Library Configuration](#library-configuration)
5. [Code Compilation](#code-compilation)
6. [Testing & Debugging](#testing--debugging)
7. [Optimization](#optimization)
8. [Common Issues](#common-issues)

## Prerequisites

### Required Skills
- Basic Arduino programming knowledge
- Understanding of C/C++ syntax
- Familiarity with serial debugging
- Basic electronics (if adding external components)

### Required Tools
- Computer (Windows/Mac/Linux)
- USB-C cable (data capable, not charge-only)
- Arduino IDE 2.0+
- Serial monitor software
- Smartphone for BLE testing

## Hardware Setup

### ESP32-S3-Touch-LCD-2 Board Overview

```
┌─────────────────────────────┐
│  [USB-C Port]               │
│                             │
│  ┌───────────────────┐      │
│  │                   │      │
│  │   240x320 LCD     │      │
│  │   Touch Display   │      │
│  │                   │      │
│  └───────────────────┘      │
│                             │
│  [Boot Button]  [Reset]     │
│                             │
│  [GPIO Pins on sides]       │
└─────────────────────────────┘
```

### Pin Connections (Default)

| Function | Pin | Description |
|----------|-----|-------------|
| Display MOSI | GPIO38 | SPI data to display |
| Display SCLK | GPIO39 | SPI clock |
| Display CS | GPIO45 | Chip select |
| Display DC | GPIO42 | Data/Command |
| Display RST | -1 | Reset |
| Display BL | GPIO1 | Backlight PWM |
| Touch SDA | GPIO48 | I2C data |
| Touch SCL | GPIO47 | I2C clock |
| Touch INT | -1 | Touch interrupt |
| IMU SDA | GPIO48 | I2C data (shared bus) |
| IMU SCL | GPIO47 | I2C clock (shared bus) |
| Battery ADC | GPIO5 | Voltage reading |
| Boot Button | GPIO0 | Wake/User input |

### Optional Hardware Additions

#### Adding a Vibration Motor
```
Motor (+) → GPIO17 → 100Ω Resistor → NPN Transistor Collector
Motor (-) → GND
Transistor Base → GPIO17 (through 1kΩ resistor)
Transistor Emitter → GND
```

#### Adding a Battery
```
Li-Po Battery (+) → Battery connector (+)
Li-Po Battery (-) → Battery connector (-)
Battery monitoring is already onboard via a divider on GPIO5 (no wiring needed)
```

## Software Installation

### Step 1: Install Arduino IDE

1. Download from https://www.arduino.cc/en/software
2. Choose version 2.0 or later
3. Install with default settings
4. Launch Arduino IDE

### Step 2: Add ESP32 Board Support

1. Open Arduino IDE
2. Navigate to **File → Preferences**
3. In "Additional Board Manager URLs" add:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
4. Click OK
5. Go to **Tools → Board → Board Manager**
6. Search for "esp32"
7. Install "esp32 by Espressif Systems" (latest version)
8. Wait for installation to complete

### Step 3: Install Required Libraries

Open **Tools → Manage Libraries** and install these exact versions (they match
Waveshare's demo package — newer versions break the build):

1. **GFX Library for Arduino** (Arduino_GFX)
   - Search: "GFX Library for Arduino"
   - Install version **1.5.0** (≥1.6 removes the `BLACK`/`RED`/… color macros)
   - Drives the ST7789 display

2. **lvgl**
   - Search: "lvgl"
   - Install version **8.4.0**
   - Graphics library

3. **ArduinoJson**
   - Search: "ArduinoJson"
   - Install latest 7.x version
   - For parsing notifications

The **CST816 touch driver (`bsp_cst816`)** is vendored in the sketch itself
(`SmartWatchV3/bsp_cst816.h/.cpp`) — nothing to install.

> Note on the ESP32 board package: install **version 3.1.3**. Arduino_GFX 1.5.0
> does not compile against core ≥3.2.0 (a SPI API changed).

### Step 4: Configure LVGL

**No manual LVGL configuration is needed.** The project ships its own
`lv_conf.h` inside the sketch folder (`SmartWatchV3/lv_conf.h`), and
`SmartWatchV3.ino` defines `LV_CONF_INCLUDE_SIMPLE` so LVGL uses it. Do **not**
copy `lv_conf_template.h` into your Arduino `libraries` folder — that would
shadow the project's config.

Key settings already baked into `SmartWatchV3/lv_conf.h`:

```c
#define LV_COLOR_DEPTH   16
#define LV_COLOR_16_SWAP 1     // matches the factory firmware for this panel
#define LV_MEM_CUSTOM    0
#define LV_MEM_SIZE      (96U * 1024U)
```

## Library Configuration

### Display and touch

This project drives the display directly with **Arduino_GFX** and the touch
controller with the vendored **`bsp_cst816`** driver — there is no
`ESP32_Display_Panel` / `esp-brookesia` configuration to edit. The display
object is constructed in `SmartWatchV3.ino`, exactly as the factory example
(`examples/01_factory/bsp_lv_port.cpp`) does:

```cpp
// FSPI + shared bus, ST7789 IPS, native 240x320
Arduino_DataBus *bus = new Arduino_ESP32SPI(
    PIN_LCD_DC /*42*/, PIN_LCD_CS /*45*/, PIN_LCD_SCLK /*39*/,
    PIN_LCD_MOSI /*38*/, PIN_LCD_MISO /*40*/, FSPI, true);
Arduino_GFX *gfx = new Arduino_ST7789(
    bus, PIN_LCD_RST /*-1*/, LCD_ROTATION /*1*/, true /*IPS*/, 240, 320);
```

Pin values live in `SmartWatchV3/src/hardware_config.h`; do not change them
unless the hardware changes.

## Code Compilation

### Board Settings in Arduino IDE

1. Go to **Tools** menu
2. Configure each setting:

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240MHz (WiFi) |
| Core Debug Level | None |
| USB DFU On Boot | Disabled |
| Erase All Flash | Disabled |
| Events Run On | Core 1 |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB (128Mb) |
| Arduino Runs On | Core 1 |
| USB Firmware MSC | Disabled |
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) |
| PSRAM | OPI PSRAM |
| Upload Mode | UART0 / Hardware CDC |
| Upload Speed | 921600 |
| USB Mode | Hardware CDC and JTAG |

### Compilation Steps

1. Open `SmartWatchV3.ino`
2. Connect ESP32-S3 board via USB-C
3. Select correct port in **Tools → Port**
4. Click **Verify** button (checkmark) to compile
5. Check for any errors in output window
6. If successful, click **Upload** button (arrow)

### First Upload Process

```
1. Compilation starts...
   Sketch uses X bytes (X%) of program storage
   Global variables use X bytes (X%) of dynamic memory
   
2. Uploading...
   Connecting........_____....._____
   
3. Writing at 0x00010000... (10%)
   Writing at 0x00020000... (20%)
   [Progress continues to 100%]
   
4. Hard resetting via RTS pin...
   Done uploading.
```

## Testing & Debugging

### Serial Monitor Setup

1. Open **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. You should see (actual boot output from `SmartWatchV3.ino`):
   ```
   ESP32-S3 SmartWatch Starting...
   LVGL: 2x full-screen buffers in PSRAM (76800 px each)
   LVGL port initialization complete
   LVGL task started
   Initializing BLE...
   SmartWatch init complete!
   === Memory Status ===
   ```
   The `2x full-screen buffers in PSRAM` line confirms PSRAM is enabled. If you
   instead see a PSRAM allocation error, your build is missing `PSRAM=opi`.

### Basic Functionality Tests

#### 1. Display Test
- Screen should light up
- UI should appear without artifacts
- Touch response should work

#### 2. Touch Calibration
```cpp
// Add to setup() for testing
void testTouch() {
    while(1) {
        if (touchRead(TOUCH_INT) < 40) {
            Serial.println("Touch detected!");
        }
        delay(100);
    }
}
```

#### 3. BLE Test
- Open Bluetooth settings on phone
- Look for "ESP32-Watch"
- Attempt pairing
- Check Serial Monitor for connection status

### Debug Output Levels

Add to your code for different debug levels:
```cpp
#define DEBUG_NONE 0
#define DEBUG_ERROR 1
#define DEBUG_WARNING 2
#define DEBUG_INFO 3
#define DEBUG_VERBOSE 4

#define DEBUG_LEVEL DEBUG_INFO

#if DEBUG_LEVEL >= DEBUG_ERROR
  #define DEBUG_ERROR(x) Serial.print("[ERROR] "); Serial.println(x)
#else
  #define DEBUG_ERROR(x)
#endif
```

## Optimization

### Memory Optimization

#### Monitor Memory Usage
```cpp
void printMemoryStats() {
    Serial.printf("Free Heap: %d\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
    Serial.printf("Heap Size: %d\n", ESP.getHeapSize());
    Serial.printf("PSRAM Size: %d\n", ESP.getPsramSize());
}
```

#### Optimize Buffers
```cpp
// Use PSRAM for large buffers
uint8_t* large_buffer = (uint8_t*)ps_malloc(1024);

// Use smaller buffers where possible
#define NOTIFICATION_BUFFER_SIZE 256  // Instead of 1024
```

### Power Optimization

#### Implement Sleep Modes
```cpp
void enterLightSleep(uint32_t seconds) {
    // Dim display
    analogWrite(TFT_BL, 10);
    
    // Configure wake source
    esp_sleep_enable_timer_wakeup(seconds * 1000000);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    
    // Enter sleep
    esp_light_sleep_start();
    
    // Wake up - restore display
    analogWrite(TFT_BL, 128);
}
```

#### Reduce CPU Frequency When Idle
```cpp
void setLowPowerMode() {
    setCpuFrequencyMhz(80);  // Reduce from 240MHz
}

void setHighPerformanceMode() {
    setCpuFrequencyMhz(240);
}
```

### Display Optimization

#### Partial Screen Updates
```cpp
// Update only changed areas
lv_obj_invalidate(changed_object);  // Instead of full refresh
```

#### Reduce Refresh Rate
```cpp
// In lvgl_port_v8.cpp
#define LVGL_REFRESH_PERIOD_MS 50  // 20 FPS instead of 60
```

## Common Issues

### Issue: Display Shows Nothing

**Solutions:**
1. Check pin connections in `hardware_config.h`
2. Verify SPI speed isn't too high (try 10MHz)
3. Check backlight pin and brightness
4. Test with simple GFX example first

### Issue: Touch Not Responding

**Solutions:**
1. Verify I2C address (scan with I2C scanner)
2. Check pull-up resistors on SDA/SCL
3. Test touch interrupt pin
4. Reduce I2C speed to 100kHz

### Issue: BLE Won't Connect

**Solutions:**
1. Clear paired devices on phone
2. Reset ESP32 and retry
3. Check if BLE is enabled in code
4. Verify no other device is connected
5. Try different phone

### Issue: Random Resets

**Solutions:**
1. Check power supply (use powered USB hub)
2. Add capacitors near ESP32 power pins
3. Check for stack overflow (increase task stack sizes)
4. Monitor brownout detector:
   ```cpp
   #include "soc/soc.h"
   #include "soc/rtc_cntl_reg.h"
   
   void setup() {
       WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disable brownout
   }
   ```

### Issue: Out of Memory Errors

**Solutions:**
1. Enable PSRAM in board settings
2. Move large arrays to PSRAM:
   ```cpp
   uint8_t* buffer = (uint8_t*)ps_malloc(size);
   ```
3. Reduce buffer sizes
4. Delete unused objects:
   ```cpp
   lv_obj_del(unused_screen);
   ```

### Issue: Compilation Errors

**Common Fixes:**
1. Update all libraries to latest versions
2. Check for conflicting library versions
3. Clean build: **Sketch → Verify/Compile** with Shift held
4. Delete `.arduino` cache folder
5. Reinstall ESP32 board package

## Advanced Configuration

### Custom Screen Development

Screens are plain LVGL, registered with the project's `ScreenManager` (see
`src/ui/screen_manager.h` and the existing screens in `src/ui/screens/`). A
screen provides `create`/`destroy` functions and is registered in
`SmartWatchV3.ino`. Template following the existing pattern:

```cpp
// src/ui/screens/my_screen.h / .cpp
namespace MyScreen {
    lv_obj_t *create(lv_obj_t *parent) {
        lv_obj_t *root = lv_obj_create(parent);
        lv_obj_set_size(root, lv_pct(100), lv_pct(100));
        // ... build your UI on `root` ...
        return root;
    }
    void destroy(lv_obj_t *root) {
        if (root) lv_obj_del(root);
    }
}
```

Then register it in `setup()` alongside the others:
```cpp
ScreenManager::registerScreen(SCREEN_MY_SCREEN, MyScreen::create, MyScreen::destroy, "My Screen");
```

### Adding External Sensors

Example: Heart Rate Sensor (MAX30102)
```cpp
#include <MAX30105.h>
MAX30105 particleSensor;

void setupHeartRateSensor() {
    if (particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        particleSensor.setup();
        particleSensor.setPulseAmplitudeRed(0x0A);
    }
}

uint8_t getHeartRate() {
    long irValue = particleSensor.getIR();
    if (checkForBeat(irValue)) {
        // Calculate BPM
    }
    return bpm;
}
```

## Performance Metrics

Expected performance with optimizations:

| Metric | Value |
|--------|-------|
| Boot Time | < 3 seconds |
| UI Frame Rate | 30-60 FPS |
| Touch Latency | < 50ms |
| BLE Connection Time | < 2 seconds |
| Battery Life (400mAh) | 8-12 hours |
| Deep Sleep Current | < 10µA |
| Active Current | 50-150mA |

## Next Steps

1. **Customize UI**: Modify the screens in `src/ui/screens/` for your design
2. **Add Features**: Implement additional sensors
3. **Create Companion App**: Build phone app for enhanced features
4. **Design Case**: 3D print or purchase watch case
5. **Optimize Battery**: Fine-tune power management

## Support Resources

- [ESP32 Forum](https://esp32.com)
- [LVGL Forum](https://forum.lvgl.io)
- [Arduino Forum](https://forum.arduino.cc)
- [Project GitHub](https://github.com/yourusername/smartwatch)

---

Document Version: 1.0
Last Updated: January 2025
