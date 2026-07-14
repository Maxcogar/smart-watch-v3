# Change Traceability — Factory-Docs Audit Branch

Every change on `claude/project-factory-docs-audit-wlty9g`, traced to its
source. This reflects the **final** state after correcting earlier mistakes
(see the "Corrections" note at the end).

**Guiding principle:** reproduce the manufacturer's validated environment
exactly — same code patterns, same library versions, same core version — rather
than substitute newer versions or invent workarounds.

Source types: **[OEM-DEMO]** Waveshare's official demo package
(files.waveshare.com/wiki/ESP32-S3-Touch-LCD-2/ESP32-S3-Touch-LCD-2-Demo.zip);
**[OEM-CODE]** the factory example code in this repo (`examples/01_factory` =
factory firmware source); **[OEM-WIKI]** waveshare.com / docs.waveshare.com;
**[BUILD]** verified by actually compiling; **[TOOL]** arduino-cli
`board details` (Espressif's own board definition).

---

## The manufacturer's validated stack (the anchor for everything below)

From Waveshare's demo package `library.properties` files **[OEM-DEMO]**:
- `GFX Library for Arduino` = **1.5.0** (defines `BLACK` etc. at `Arduino_GFX.h:57`)
- `lvgl` = **8.4.0**
- touch driver = **bsp_cst816** (bundled at `Arduino/libraries/bsp_cst816/`)

ESP32 core = **3.1.3** **[BUILD]**: GFX 1.5.0 calls the pre-3.2.0
`spiFrequencyToClockDiv(freq)`; core ≥3.2.0 (incl. 3.3.x) added a `spi_t*`
parameter and fails to compile GFX 1.5.0. The demo is dated Dec-2024/Jan-2025 —
the 3.1.x line.

---

## 1. `SmartWatchV3/sketch.yaml` (new) — pins the whole stack

**Why it exists [TOOL]:** `arduino-cli board details -b esp32:esp32:esp32s3`
shows the ESP32-S3 defaults are `PSRAM=disabled` + `FlashSize=4M`. The build
command in `COMMON_PROBLEMS_AND_FIXES.md:82` set neither, so PSRAM was never
enabled at runtime — the root cause of the blank screen.

| Pin | Value | Source |
|-----|-------|--------|
| `PSRAM=opi` | OPI PSRAM | **[OEM-WIKI]** "8MB PSRAM"; **[TOOL]** id `PSRAM=opi`; README:70 |
| `FlashSize=16M` | 16MB | **[OEM-WIKI]** "16MB Flash"; **[TOOL]** id `FlashSize=16M` |
| `PartitionScheme=huge_app` | 3MB app | `COMMON_PROBLEMS_AND_FIXES.md:52,82`; **[TOOL]** id |
| `CDCOnBoot=cdc`,`USBMode=hwcdc`,`CPUFreq=240`,`FlashMode=qio`,`UploadSpeed=921600` | — | README:63-74 board settings; **[TOOL]** ids |
| core `esp32:esp32 3.1.3` | — | **[BUILD]** GFX 1.5.0 needs core <3.2.0; demo era = 3.1.x |
| `GFX Library for Arduino 1.5.0`, `lvgl 8.4.0`, `ArduinoJson 7.4.3` | — | **[OEM-DEMO]** `library.properties` |

---

## 2. `src/lvgl_port_v8.cpp` — framebuffers + display driver

**Change:** two full-screen buffers in PSRAM + `full_refresh = 1`; no fallback,
no `direct_mode`.

**Source [OEM-CODE]** `examples/01_factory/bsp_lv_port.cpp`:
- L157/159: `heap_caps_malloc(H_RES*V_RES*sizeof(lv_color_t), MALLOC_CAP_SPIRAM)` ×2
- L162: `lv_disp_draw_buf_init(&disp_buf, buf1, buf2, H_RES*V_RES)`
- L170: `disp_drv.full_refresh = 1;`

The factory `assert()`s the buffers — it *requires* PSRAM. My code logs a hard
error and returns if PSRAM alloc fails; no partial-buffer fallback.

**[BUILD]** corroboration: 140 KB internal RAM free after globals — the two
150 KB buffers cannot live there, which is why the factory puts them in PSRAM
and why the pre-PSRAM build crashed on a NULL buffer.

---

## 3. `SmartWatchV3.ino` — SPI bus + display constructor

**Change:** `Arduino_ESP32SPI(DC,CS,SCLK,MOSI,MISO, FSPI, true)` and
`Arduino_ST7789(bus, RST, ROTATION, true, 240, 320)`.

**Source [OEM-CODE]** `examples/01_factory/bsp_lv_port.cpp:22-29` — verbatim the
factory's bus (FSPI + `is_shared=true`) and display (explicit `H_RES,V_RES`)
construction. Previously the sketch omitted `FSPI`/`is_shared` and the explicit
dimensions, letting Arduino_GFX pick defaults.

---

## 4. `SmartWatchV3/lv_conf.h` — `LV_COLOR_16_SWAP 0 → 1`

**Source [OEM-CODE]** `examples/01_factory/lv_conf.h:30` `#define LV_COLOR_16_SWAP 1`.
The flush callback already switches `draw16bitBeRGBBitmap`/`draw16bitRGBBitmap`
on this macro, so SWAP=1 selects the byte order the factory firmware ships with.

---

## 5. `src/hardware_config.h` (two lines)

- `TFT_SPI_HOST HSPI → FSPI`: **[OEM-CODE]** `bsp_spi.cpp:5`, `bsp_lv_port.cpp:24`.
  **Honesty:** this macro is unused; the real SPI-host fix is in change #3.
- battery divider `4095.0f → 4096.0f`: **[OEM-CODE]** `06_lvgl_battery.ino:196`
  `voltage = 3.3 / 4096 * analogValue * 3`. ~0.02% — consistency only.

---

## 6. `SmartWatchV3/{lib/bsp_cst816 → }/bsp_cst816.{h,cpp}` — driver to sketch root

**Source [BUILD]** compile error `bsp_cst816.h: No such file`. Arduino compiles
the sketch root + `src/` recursively, never a `lib/` folder (PlatformIO
convention). All consumers use bare `#include "bsp_cst816.h"`, so sketch-root
placement resolves it. (The OEM ships bsp_cst816 as an installed library
`Arduino/libraries/bsp_cst816/` **[OEM-DEMO]**; vendoring it into the sketch
root is the self-contained equivalent.) Verified by the passing compile.

---

## 7. Docs
- `README.md` — rewritten to the real Arduino_GFX/LVGL architecture, the
  manufacturer's pinned versions, and required board settings.
  Sources: **[OEM-WIKI]** + **[OEM-CODE]** pin/spec values;
  `COMMON_PROBLEMS_AND_FIXES.md §2` for the esp-brookesia removal.
- `FACTORY_DOCS_AUDIT.md`, this file — the audit trail.

---

## Corrections to my earlier commits (full honesty)

Two things I did earlier were themselves the mistake you called out, and are now
reverted:

1. **I pinned newer library/core versions than the manufacturer uses**
   (GFX 1.6.6, lvgl 8.3.11, core 3.3.10) and then **rewrote OEM code to fit
   them** — `gfx->fillScreen(BLACK)` → `RGB565_BLACK`. Wrong direction.
   Corrected: pin the manufacturer's versions (GFX 1.5.0, lvgl 8.4.0, core
   3.1.3), and the OEM code compiles unchanged (`BLACK` is defined in GFX 1.5.0).
2. **I invented a partial-buffer fallback** with no OEM basis and **omitted
   `full_refresh = 1`**. Corrected: match `bsp_lv_port.cpp` exactly — PSRAM
   required, `full_refresh = 1`, no fallback.

---

## Verified vs. not

- **Verified [BUILD]:** compiles clean on the manufacturer-matched stack
  (core 3.1.3 / GFX 1.5.0 / lvgl 8.4.0) — 2.13 MB app (67% of huge_app).
- **NOT verified:** runtime on physical hardware — I cannot flash your board
  from here. The boot serial log prints
  `LVGL: 2x full-screen buffers in PSRAM …`, which confirms PSRAM is live when
  you flash it.
