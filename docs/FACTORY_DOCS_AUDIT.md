# Factory / OEM Audit — Waveshare ESP32-S3-Touch-LCD-2

**Date:** 2026-06-13
**Question answered:** Does this firmware program the board the way the OEM
(Waveshare) does?

## Authoritative sources used
- **Waveshare wiki / docs** (ESP32-S3-Touch-LCD-2): confirms the silicon —
  ESP32-S3, 512KB SRAM + 384KB ROM, **stacked 8MB PSRAM**, **external 16MB
  Flash**, 2" 240×320 IPS, **ST7789T3** display, **CST816D** touch, **QMI8658**
  6-axis IMU, 3.7V MX1.25 battery, microSD, camera interface.
- **Bundled OEM reference code** — `examples/01_factory/` is the source for the
  shipped `ESP32-S3-Touch-LCD-2-factory.bin`; `examples/04/06/07/08` are
  Waveshare's demo sketches. For this board these sketches + BSP *are* the OEM's
  prescribed programming method (Arduino framework, `Arduino_GFX` for the
  ST7789, `bsp_cst816` for touch, `Wire` for I2C, LEDC for backlight).

## Pin map / peripheral APIs — match the OEM exactly ✅

| Item | OEM (factory BSP / demos) | Project | Status |
|---|---|---|---|
| LCD SCLK/MOSI/MISO | 39 / 38 / 40 | 39 / 38 / 40 | ✅ |
| LCD DC / CS / RST | 42 / 45 / -1 | 42 / 45 / -1 | ✅ |
| LCD backlight | GPIO 1, LEDC 5000Hz 10-bit | 1, 5000/10 | ✅ |
| Display driver | ST7789, IPS, 240×320, rotation 1 | same | ✅ |
| Touch | CST816 @0x15, I2C SDA48/SCL47 | same | ✅ |
| IMU | QMI8658 @0x6B | same | ✅ |
| Battery ADC | GPIO5, `3.3/4096*adc*3` | GPIO5, same formula | ✅ |
| Framework | Arduino + Arduino_GFX + bsp_cst816 | same | ✅ |

The hardware bring-up is faithful to the OEM. The findings below are the few
places the code diverged from the OEM reference, plus the fixes applied.

## Findings & fixes applied

### 1. Framebuffer was NOT done the OEM way — FIXED (high impact)
The OEM factory BSP (`examples/01_factory/bsp_lv_port.cpp`) allocates **two
full-screen LVGL buffers in PSRAM** (`MALLOC_CAP_SPIRAM`). The board carries
8MB PSRAM specifically for this. The project had instead been changed to a
**1/10-screen partial buffer in internal RAM**, under the mistaken assumption
that a full framebuffer "won't fit" — it fits trivially in PSRAM.
**Fix:** `lvgl_port_v8.cpp` now allocates two full-screen buffers from
`MALLOC_CAP_SPIRAM` (factory pattern), with a graceful fall-back to the partial
internal buffer only if PSRAM is genuinely unavailable.

### 2. `TFT_SPI_HOST` set to HSPI — FIXED (low)
Factory uses **FSPI** (`SPIClass bsp_spi(FSPI)`, `Arduino_ESP32SPI(..., FSPI)`).
The constant was `HSPI` (and unused). **Fix:** set to `FSPI` to match the OEM.

### 3. Battery divider used `/4095` — FIXED (trivial)
OEM `06_lvgl_battery` formula is `3.3/4096*adc*3`; the code used `4095.0f`.
**Fix:** changed to `4096.0f` to match the OEM formula exactly.

## Note on the README (not yet changed)
`README.md` still instructs builders to install **esp-brookesia /
ESP32_Display_Panel / ESP32_IO_Expander** and describes a brookesia
app-launcher UI. The firmware was deliberately moved off those libraries to the
OEM `Arduino_GFX` + `bsp_cst816` path (see
`COMMON_PROBLEMS_AND_FIXES.md` §2). The README should be rewritten to match the
shipped code and the OEM method. Flagged for a follow-up.
