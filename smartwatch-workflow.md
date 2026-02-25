# ESP32 Smartwatch — Agentic Build Workflow

This document defines the mandatory workflow for building a custom smartwatch on a Waveshare ESP32 with touchscreen display. It is designed to be followed by an AI coding agent (Claude Code, Cursor, etc.) or used as instructions for a human-agent pair programming session.

**The core rule: nothing advances until the previous step compiles, flashes, and is verified on hardware.**

---

## Philosophy

Embedded development has zero tolerance for assumptions. A wrong pin number, a wrong driver, or a wrong bus configuration doesn't produce an error message — it produces silence. The device does nothing and tells you nothing about why.

This means:

1. **Every milestone must be verified on real hardware before the next milestone begins.** "It should work" is not verification. Flashing and seeing the expected result on the physical device is verification.
2. **The agent must never generate configuration values from training data.** Pin mappings, driver ICs, bus speeds, and peripheral addresses come exclusively from the hardware documentation provided. If the documentation doesn't specify something, the agent must ask — not guess.
3. **Each milestone is a standalone, compilable, flashable sketch.** Not a diff. Not a partial file. A complete sketch that can be opened in Arduino IDE or compiled with Arduino CLI and flashed immediately.
4. **Scope is enforced per milestone.** The agent works on exactly one milestone at a time. It does not "prepare for later milestones" by adding unused code, abstractions, or file structures.

---

## Pre-Flight: Hardware Manifest

Before any code is written, the agent must create a hardware manifest file from the provided documentation. This file becomes the single source of truth for every hardware interaction in the project.

### Required: `HARDWARE_MANIFEST.md`

The agent must extract and document ALL of the following from the provided hardware docs. If any field cannot be determined from the docs, the agent must stop and ask — not fill in a default.

```markdown
# Hardware Manifest

## Board
- Board model: [exact Waveshare model name and version]
- MCU: [exact ESP32 variant — ESP32-S3, ESP32-C3, etc.]
- Flash size: [MB]
- PSRAM: [yes/no, size]
- USB: [native USB or USB-UART bridge, chip if applicable]

## Display
- Driver IC: [exact chip — ST7789, GC9A01, etc.]
- Resolution: [width x height]
- Interface: [SPI / parallel / I2C]
- Color depth: [16-bit RGB565, etc.]
- Pin assignments:
  - MOSI/SDA: GPIO [N]
  - SCLK/SCL: GPIO [N]
  - CS: GPIO [N]
  - DC/RS: GPIO [N]
  - RST: GPIO [N]
  - BL/Backlight: GPIO [N]
  - Bus speed: [MHz]

## Touch Controller
- Controller IC: [exact chip — CST816S, FT6336, GT911, etc.]
- Interface: [I2C / SPI]
- Pin assignments:
  - SDA: GPIO [N]
  - SCL: GPIO [N]
  - INT: GPIO [N]
  - RST: GPIO [N]
  - I2C address: [hex]

## Power
- Battery connector: [yes/no, type]
- Charging IC: [if applicable]
- Power management IC: [if applicable]
- Voltage regulator: [if applicable]
- Battery monitoring: [ADC pin if applicable]

## Other Peripherals
- IMU/accelerometer: [model, interface, pins]
- RTC: [model, interface, pins]
- Buzzer/motor: [pin]
- Buttons: [pins, active high/low]
- Any other onboard peripherals

## Arduino Board Configuration
- Board package: [exact URL for boards manager]
- Board selection: [exact board name in Arduino IDE]
- Partition scheme: [if non-default]
- Flash mode: [if non-default]
- Upload speed: [if specific]
```

**This manifest is referenced by every milestone. If a milestone needs a pin number, it reads it from this file. The agent never hardcodes a pin number from memory.**

---

## Pre-Flight: Reference Code Baseline

If Waveshare provides example/demo code for this board:

1. **Compile it exactly as provided.** No modifications.
2. **Flash it to the device.**
3. **Confirm it runs.** Document what it does — screen color, touch response, serial output.
4. **This becomes the known-good baseline.** If something breaks later, we diff against this.

The agent must not "improve" or "clean up" the reference code. It exists to prove the toolchain and hardware work. It is not the starting point for the project — it is the verification that the environment is functional.

---

## Milestone Sequence

Each milestone is a complete, standalone Arduino sketch. The agent produces the full sketch file(s) for each milestone. The user compiles, flashes, and reports the result before the next milestone begins.

### Milestone 0: Bare Compile

**Goal:** Confirm the Arduino toolchain builds for this board with zero functionality.

**Sketch:**
- Empty `setup()` and `loop()`
- Correct board selected in Arduino IDE / platformio / CLI
- Compiles without errors
- Flashes without errors
- Serial monitor shows boot messages (if applicable)

**Verification:** "It compiled and flashed. I see [boot output / nothing expected] on serial."

**Why this exists:** If the toolchain is broken, misconfigured, or the board isn't recognized, we find out now — not after writing 300 lines of display code.

---

### Milestone 1: Backlight Only

**Goal:** Prove we can control a GPIO pin on this board.

**Sketch:**
- Sets the backlight pin (from manifest) as OUTPUT
- Turns it HIGH
- That's it

**Verification:** "The backlight turned on." or "Nothing happened" (which means the backlight pin is wrong, active-low, or needs PWM).

**Why this exists:** If we can't toggle the backlight, the display driver doesn't matter yet. This isolates the simplest possible hardware interaction.

---

### Milestone 2: Display — Solid Color Fill

**Goal:** Prove the display driver, SPI bus, and pin configuration are correct.

**Sketch:**
- Initializes the display using the exact driver IC from the manifest
- Uses the exact pin assignments from the manifest
- Fills the screen with a solid color (e.g., red)
- Uses whichever library the reference code uses (TFT_eSPI, Adafruit GFX, LovyanGFX, or Waveshare's own library)
- If using TFT_eSPI: the `User_Setup.h` must be generated from the manifest, not from a template

**Verification:** "The screen is solid red." or "The screen is white/black/garbled" (which means driver or pin config is wrong).

**What the agent must NOT do:**
- Add touch handling
- Add any UI elements
- Add any abstraction layers
- Include any unused library headers
- "Prepare" for future milestones

---

### Milestone 3: Display — Text Rendering

**Goal:** Prove we can draw text at specific coordinates.

**Sketch:**
- Everything from Milestone 2
- Draws "Hello" at coordinates (10, 10) in white on black background
- Draws the current `millis()` value updating once per second to prove the loop is running

**Verification:** "I see 'Hello' and a counter incrementing."

---

### Milestone 4: Touch — Raw Coordinates to Serial

**Goal:** Prove the touch controller is communicating.

**Sketch:**
- Initializes the touch controller using the exact IC and pins from the manifest
- On touch event, prints X/Y coordinates to Serial
- Display can show the solid color from Milestone 2 (no touch visualization yet)

**Verification:** "I touch the screen and see coordinates in serial monitor." Note the coordinate ranges — are they 0-319, 0-169, or something else? Are they inverted? This matters for the next milestone.

**What the agent must NOT do:**
- Map touch coordinates to display coordinates yet
- Add gesture detection
- Add any UI interaction

---

### Milestone 5: Touch — Visual Feedback

**Goal:** Prove touch coordinates map correctly to display coordinates.

**Sketch:**
- Combines display + touch
- When the screen is touched, draw a small circle at the touch point
- If coordinates are inverted or swapped (common), apply the correction here

**Verification:** "I touch the screen and a dot appears where my finger is, not mirrored/offset."

**This is the critical integration point.** If touch and display coordinates don't agree, everything built on top will be broken. Do not proceed until touch accurately maps to display.

---

### Milestone 6: Basic UI — Static Watchface

**Goal:** Display a static clock face layout.

**Sketch:**
- Draws a watchface layout: time display area, date, battery indicator position
- Uses hardcoded time values (e.g., "12:34") — no RTC yet
- No touch interaction yet
- Establishes the visual layout that will become the real watchface

**Verification:** "I see a watchface layout with fake time displayed."

---

### Milestone 7: Touch Zones — Button Interaction

**Goal:** Prove we can detect taps in defined screen regions.

**Sketch:**
- Draws 2-3 simple buttons on screen (rectangles with labels)
- Touch in a button region changes something visible — background color, text, counter
- Touch outside buttons does nothing
- Print which button was pressed to Serial as confirmation

**Verification:** "I tap Button A and see the expected response. Button B does something different. Tapping empty space does nothing."

---

### Milestone 8: Screen Navigation

**Goal:** Prove we can switch between multiple screens.

**Sketch:**
- Implements 2-3 distinct screens (e.g., watchface, menu, settings placeholder)
- Swipe gesture or button tap navigates between them
- Each screen draws its own content and handles its own touch zones
- Back navigation works

**Verification:** "I can swipe/tap to move between screens and back."

---

### Milestone 9+: Feature Milestones

From here, milestones depend on what features the watch needs. Each follows the same rules:

- **One feature per milestone**
- **Complete compilable sketch**
- **Hardware verification before advancing**
- **No speculative code for future milestones**

Likely sequence:
- RTC integration (real time display)
- Battery monitoring (ADC read, display percentage)
- BLE connectivity (phone notifications or data sync)
- IMU/accelerometer (step counting, wrist wake)
- Power management (deep sleep, wake on tilt/tap)
- Watch apps (timer, stopwatch, alarm)
- Persistent settings (NVS storage)

Each of these gets its own milestone document following the same pattern.

---

## Rules for the Agent

These rules are non-negotiable. They exist because every one of them has been violated in past sessions, resulting in wasted hours and scrapped work.

### Rule 1: Use the Hardware Manifest

Every pin number, driver name, bus configuration, and peripheral address comes from `HARDWARE_MANIFEST.md`. If you need a value that isn't in the manifest, ask. Do not infer it from similar boards, example code for other boards, or training data.

### Rule 2: One Milestone at a Time

When asked to work on Milestone N, produce only the code for Milestone N. Do not add code "in preparation" for Milestone N+1. Do not refactor for future extensibility. Do not add abstraction layers that aren't needed yet.

### Rule 3: Complete Sketches Only

Every milestone output is a complete, compilable sketch. Not a code snippet. Not a diff. Not "add this to your existing code." A full `.ino` file (and any required headers) that can be compiled and flashed as-is.

### Rule 4: No Guessing on Hardware

If you are unsure about any hardware detail — pin, protocol, timing, register value, library compatibility — say "I don't know, check [specific thing]." Do not provide a plausible-sounding value. A wrong guess on hardware wastes an entire debug cycle.

### Rule 5: When Using Libraries, Match the Reference Code

If the Waveshare reference code uses LovyanGFX, use LovyanGFX. If it uses TFT_eSPI, use TFT_eSPI. If it uses their own custom library, use that. Do not substitute a different library because you're "more familiar with it" or it's "more popular." The reference code compiled and ran. Use what works.

### Rule 6: If It Doesn't Compile, Nothing Else Matters

If the user reports a compile error, the only task is fixing that compile error. Do not simultaneously try to add features, refactor, or "improve" the code. Fix the error. Confirm it compiles. Then resume the milestone.

### Rule 7: Respect Previous Milestones

Code from previous verified milestones is known-good. When building Milestone N, start from the verified Milestone N-1 code. Do not rewrite working display initialization because you "prefer a different approach." The existing code works. Extend it, don't replace it.

### Rule 8: Keep the Sketch Flat Until Complexity Demands Otherwise

For early milestones, everything can live in a single `.ino` file. Do not split into multiple files, create class hierarchies, or add a "proper project structure" until the complexity genuinely requires it. Premature structure adds surface area for bugs in embedded development.

---

## Failure Recovery

When a milestone fails (doesn't compile, doesn't flash, or doesn't behave correctly on hardware):

1. **Get the exact error.** Compile error? Copy the full error output. Runtime failure? Describe exactly what happens (white screen, garbled display, no touch response, crash/reboot loop).

2. **Diff against the last working state.** What changed between the last working milestone and this one? The bug is in the diff.

3. **Do not add debug code to a broken sketch.** First, revert to the last known-good milestone and confirm it still works. Then re-apply changes incrementally until the failure reappears.

4. **If the display doesn't work, check the basics first.** Wrong rotation? Wrong color inversion? Wrong SPI mode? These are the most common display issues and they're all single-line configuration changes.

5. **If touch doesn't work, check I2C.** Run an I2C scanner sketch. Does the touch controller appear at the expected address? If not, the wiring or pin config is wrong. No amount of driver code fixes wrong hardware config.

---

## Directory Structure

Once the project grows past the early milestones, use a flat, minimal structure:

```
smartwatch/
├── HARDWARE_MANIFEST.md          # Single source of truth for hardware
├── milestones/
│   ├── m0_bare_compile/
│   │   └── m0_bare_compile.ino
│   ├── m1_backlight/
│   │   └── m1_backlight.ino
│   ├── m2_display_color/
│   │   └── m2_display_color.ino
│   ├── m3_display_text/
│   │   └── m3_display_text.ino
│   ├── m4_touch_serial/
│   │   └── m4_touch_serial.ino
│   ├── m5_touch_visual/
│   │   └── m5_touch_visual.ino
│   └── ...
├── current/                       # The active working sketch (copy of latest verified milestone)
│   └── smartwatch.ino
├── reference/                     # Unmodified Waveshare example code
│   └── [whatever they provided]
└── docs/
    ├── [Waveshare datasheets]
    ├── [pin mapping diagrams]
    └── [display/touch controller datasheets]
```

Each milestone directory preserves the exact code that was verified on hardware. The `current/` directory is always a copy of the latest verified milestone, which becomes the starting point for the next one.

---

## Checklist Template

For each milestone, the agent and user follow this checklist:

```
## Milestone [N]: [Name]

### Goal
[One sentence describing what this milestone proves]

### Prerequisites
- [ ] Milestone [N-1] verified on hardware

### Agent Output
- [ ] Complete sketch provided (compiles as-is)
- [ ] All hardware values sourced from HARDWARE_MANIFEST.md
- [ ] No code beyond this milestone's scope
- [ ] No library substitutions from reference code

### User Verification
- [ ] Compiled without errors
- [ ] Flashed without errors
- [ ] Hardware behavior matches expected result: [describe expected result]
- [ ] Milestone code saved to milestones/mN_name/

### Notes
[Any observations, quirks, coordinate corrections, etc. discovered during verification]
```

---

## Summary

The entire strategy is: **prove one thing at a time on real hardware, and never let an agent skip ahead.** Every failed smartwatch attempt has come from trying to build too much at once, using assumed hardware configs, or letting the agent generate a "complete solution" that doesn't compile. This workflow makes that impossible by requiring physical verification at every step.
