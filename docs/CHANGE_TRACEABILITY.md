# Change Traceability — Factory-Docs Audit Branch

Every change made on `claude/project-factory-docs-audit-wlty9g`, traced to its
source. Source types:

- **[OEM-CODE]** — Waveshare's own reference code bundled in this repo
  (`examples/01_factory/*` = the factory firmware source; `examples/04/06/07` =
  Waveshare demo sketches). This is the authoritative "how the OEM programs it."
- **[OEM-WIKI]** — Waveshare published docs (waveshare.com / docs.waveshare.com).
- **[TOOLCHAIN]** — Output of the actual build tools (`arduino-cli board
  details`, compiler errors). Authoritative for build syntax + behavior.
- **[ARDUINO-SPEC]** — Documented Arduino sketch build-system behavior.
- **[JUDGMENT]** — My engineering decision where no single source dictates the
  answer. Flagged explicitly so you can challenge it.

---

## 1. `src/lvgl_port_v8.cpp` — framebuffers moved to PSRAM

**Change:** replace the 1/10-screen partial buffer in internal RAM with **two
full-screen buffers in PSRAM** (`MALLOC_CAP_SPIRAM`), with a partial-internal
fallback.

**Source [OEM-CODE]:** `examples/01_factory/bsp_lv_port.cpp`
- L157: `lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);`
- L159: `lv_color_t *buf2 = … MALLOC_CAP_SPIRAM …`
- L162: `lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES);`

This is the factory doing exactly two full-screen buffers in PSRAM.

**Corroborating [OEM-WIKI]:** the board has "stacked 8MB PSRAM" — so the
framebuffers have somewhere to live. (waveshare.com/wiki/ESP32-S3-Touch-LCD-2)

**Corroborating [TOOLCHAIN]:** my verified compile reported
`Global variables use 167712 bytes … leaving 159968 bytes` of internal RAM.
Two 240×320×2-byte buffers = 307 KB, which cannot fit in ~160 KB — proving the
buffers *must* be in PSRAM, and explaining the original NULL-buffer crash
(commit `1c57842`) when PSRAM wasn't available.

**[JUDGMENT] — disclosed divergence from the factory:** the factory also sets
`disp_drv.full_refresh = 1` (`bsp_lv_port.cpp:170`). I did **not**. I kept
partial-refresh double-buffering (`full_refresh` unset, area-based flush). Both
render correctly; partial refresh is more efficient. This is my call, not the
factory's — flag it if you want an exact factory match.

**[JUDGMENT]:** the PSRAM-unavailable fallback (revert to partial internal
buffer) is mine — it preserves the previous working behavior if a build ever
ships without PSRAM, rather than crashing.

---

## 2. `SmartWatchV3/sketch.yaml` — new pinned build profile

**Change:** new file pinning the FQBN board options and library/core versions.

**FQBN:** `esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,FlashMode=qio,PartitionScheme=huge_app,CDCOnBoot=cdc,USBMode=hwcdc,CPUFreq=240,UploadSpeed=921600`

Per-option source:

| Option | Value | Source |
|--------|-------|--------|
| `PSRAM=opi` | OPI PSRAM | **[OEM-WIKI]** "8MB PSRAM" + board is OPI; **[TOOLCHAIN]** `arduino-cli board details -b esp32:esp32:esp32s3` lists `OPI PSRAM → PSRAM=opi`, and shows the default is `PSRAM=disabled` (the trap). Also README.md:70 "PSRAM: OPI PSRAM". |
| `FlashSize=16M` | 16MB | **[OEM-WIKI]** "external 16MB Flash"; **[TOOLCHAIN]** board details `16MB (128Mb) → FlashSize=16M`; README.md:67. |
| `PartitionScheme=huge_app` | 3MB app | **[OEM-CODE/PROJECT]** `docs/COMMON_PROBLEMS_AND_FIXES.md:52,82`; README.md:68; **[TOOLCHAIN]** board details id `huge_app`. |
| `CDCOnBoot=cdc` | USB CDC on boot | README.md:64 / `SmartWatchV3.ino:16`; **[TOOLCHAIN]** id `CDCOnBoot=cdc`. |
| `USBMode=hwcdc` | Hardware CDC and JTAG | README.md:74; **[TOOLCHAIN]** id `USBMode=hwcdc`. |
| `CPUFreq=240` | 240MHz | README.md:65 / PRD §7.1; **[TOOLCHAIN]** id `CPUFreq=240`. |
| `FlashMode=qio` | QIO 80MHz | README.md:66; **[TOOLCHAIN]** id `FlashMode=qio`. |
| `UploadSpeed=921600` | — | README.md:73. |

**Core / lib versions** (`esp32:esp32 3.3.10`, `GFX Library for Arduino 1.6.6`,
`lvgl 8.3.11`, `ArduinoJson 7.4.3`): **[TOOLCHAIN]** — the exact versions
`arduino-cli` installed and that I compiled against (`core list` / `lib list`).
`lvgl 8.3.11` and `ArduinoJson 7.x` are also required by README.md:53-54 and
`SmartWatchV3.ino:10-12`.

**Why this file exists at all [TOOLCHAIN]:** board details showed the ESP32-S3
default is `PSRAM=disabled` + `FlashSize=4M`, and
`COMMON_PROBLEMS_AND_FIXES.md:82` documented a build command with neither set —
so PSRAM was never enabled. This is the root-cause fix.

---

## 3. `SmartWatchV3/{lib/bsp_cst816 → }/bsp_cst816.{h,cpp}` — driver moved to sketch root

**Change:** moved the vendored CST816 touch driver out of `lib/`.

**Source [TOOLCHAIN]:** the compile failed with
`fatal error: bsp_cst816.h: No such file or directory` (SmartWatchV3.ino:27).

**Source [ARDUINO-SPEC]:** the Arduino build system compiles the sketch root
plus the `src/` subfolder (recursively) and puts them on the include path; a
`lib/` subfolder is a **PlatformIO** convention and is not compiled. All three
consumers include it bare — `SmartWatchV3.ino:27`, `src/lvgl_port_v8.cpp:24`,
`bsp_cst816.cpp:2` (`#include "bsp_cst816.h"`) — so placing it in the sketch
root resolves the include everywhere.

**Verification [TOOLCHAIN]:** after the move the sketch compiled.

**[JUDGMENT]:** root placement (vs. `src/` with rewritten include paths, or a
`build_opt.h` `-I` flag) — chosen because it needs zero edits to the existing
bare includes and works identically in Arduino IDE and arduino-cli.

---

## 4. `SmartWatchV3.ino` — `BLACK` → `RGB565_BLACK`

**Change:** one line, `gfx->fillScreen(BLACK)` → `gfx->fillScreen(RGB565_BLACK)`.

**Source [TOOLCHAIN]:** compiler error `'BLACK' was not declared in this scope`
(SmartWatchV3.ino:116), and grep of the installed Arduino_GFX 1.6.6
(`src/Arduino_GFX.h:43`) shows only `#define RGB565_BLACK RGB565(0,0,0)` — the
legacy `BLACK` alias was removed in Arduino_GFX ≥1.5. The OEM demo sketches
(e.g. `06_lvgl_battery.ino:109 gfx->fillScreen(BLACK)`) were written against an
older Arduino_GFX; on the pinned 1.6.6 the new macro name is required.

**Verification [TOOLCHAIN]:** compiles clean after the change.

---

## 5. `src/hardware_config.h` — two changes

### 5a. `TFT_SPI_HOST HSPI` → `FSPI`
**Source [OEM-CODE]:** `examples/01_factory/bsp_spi.cpp:5` `SPIClass bsp_spi(FSPI);`
and `bsp_lv_port.cpp:24` `Arduino_ESP32SPI(…, FSPI /* spi_num */, true)`.
**Caveat (honesty):** this macro is **not referenced anywhere** in the build
(grep found only its definition), so this is a correctness/consistency fix with
**no runtime effect** — the live display bus in `SmartWatchV3.ino:84` uses the
Arduino_ESP32SPI default.

### 5b. Battery divider `/4095.0f` → `/4096.0f`
**Source [OEM-CODE]:** `examples/06_lvgl_battery/06_lvgl_battery.ino:196`
`voltage = 3.3 / 4096 * analogValue * 3;` (pin from L16 `EXAMPLE_PIN_NUM_BAT 5`).
The file's own comments (`hardware_config.h:53,117`) already cited `/4096`; the
code used `4095.0f`. Numerically ~0.02% — a consistency fix, not a behavior fix.

---

## 6. `README.md` — rewritten

Not a functional change; rewritten to match reality. Claim sources:

- Hardware table (8MB PSRAM, 16MB flash, ST7789T3, CST816D @0x15, QMI8658 @0x6B):
  **[OEM-WIKI]** + **[OEM-CODE]** (`04_qmi8658_output.ino:4` `IMU_ADDRESS 0x6B`;
  touch addr `0x15` from `hardware_config.h:41`).
- Pin table: **[OEM-CODE]** `bsp_lv_port.h:7-12` (SCLK39/MOSI38/MISO40/DC42/
  RST-1/CS45), `bsp_i2c.h:6-7` (SDA48/SCL47), `06_lvgl_battery.ino:16` (BAT5),
  BL1 from `bsp_lv_port.cpp:19` `#define GFX_BL 1`.
- "No esp-brookesia; plain LVGL": **[PROJECT]** `docs/COMMON_PROBLEMS_AND_FIXES.md`
  §2 (the documented refactor away from ESP_Panel/brookesia) + the actual
  `src/` tree.
- Board settings table: mirrors the sources in §2 above.
- Build commands: **[TOOLCHAIN]** — I ran them; the compile succeeded.

---

## 7. `.gitignore` — add `.build/`
**[JUDGMENT]:** my scratch toolchain dir (~1GB of downloaded cores) must not be
committed. (Turned out already covered by an existing rule; no net change.)

---

## What is verified vs. not (honesty)

- **Verified [TOOLCHAIN]:** the firmware compiles clean against the OEM
  toolchain via `sketch.yaml` (esp32 3.3.10, Arduino_GFX 1.6.6, lvgl 8.3.11) —
  1.75 MB app (55% of huge_app), 167 KB DRAM globals.
- **NOT verified:** runtime behavior on physical hardware. I cannot flash your
  board from here. The PSRAM root cause is fixed at the build-config level and
  the code matches the factory allocation pattern, but final confirmation is you
  flashing it. On boot the serial log prints which buffer path was taken
  (`LVGL: 2x full-screen buffers in PSRAM …` vs the fallback), which will
  confirm PSRAM is live.
