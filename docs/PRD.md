# ESP32-S3 ADHD-Friendly Smartwatch — Product Requirements Document

**Hardware Target:** Waveshare ESP32-S3-Touch-LCD-2
**Architecture:** Layered Monolith (Monorepo)
**Total Estimate:** 76 Story Points across 11 Sprints

## Change Log

| Date       | Version | Description                                                                                      | Author            |
|------------|---------|--------------------------------------------------------------------------------------------------|--------------------|
| 2024-05-24 | 1.0     | Initial PRD Draft                                                                                | John, PM           |
| 2025-08-19 | 2.0     | Professional Enhancement — ADHD Design Principles, User Personas, Success Metrics, Technical Constraints, Enhanced Acceptance Criteria | BMad Orchestrator  |

---

## 1. Goals & Background

### 1.1 Goals

1. Create a high-quality, purpose-built wearable that directly addresses the user's specific productivity needs.
2. Successfully integrate with the user's existing ecosystem (Microsoft ToDo, phone, local network devices).
3. Develop a modular and expandable software architecture.
4. Achieve a measurable increase in completed focus timer sessions and a reduction in distractions.
5. Ensure the device is stable and performant enough for all-day use.

### 1.2 Background

This project addresses a key gap in the commercial smartwatch market. The device actively defends the user's focus by implementing a Focus Shield and adhering to a strict set of minimalist, ADHD-Friendly Design Principles.

---

## 2. ADHD-Friendly Design Principles

These five principles are grounded in clinical ADHD research and neurodivergent UX design best practices, addressing executive function challenges, attention regulation difficulties, and sensory processing needs.

### Principle 1 — Singular Focus

"One Thing at a Time" — Each screen presents exactly one primary task or decision point, preventing cognitive overload and reducing executive function demands. Maximum one primary call-to-action per screen with progressive disclosure for secondary options.

### Principle 2 — Immediate Clarity

"No Guessing Required" — All interface elements provide instant understanding without cognitive processing time. High contrast ratios (4.5:1 minimum), clear affordances, text labels with icons, large touch targets (44px minimum), and unambiguous states.

### Principle 3 — Gentle Consistency

"Familiar Patterns, Reduced Cognitive Load" — Consistent interaction patterns throughout eliminate relearning behaviors. Standardized gestures, visual pattern consistency, predictable navigation flow, and interaction muscle memory development.

### Principle 4 — Protective Boundaries

"Respect Focus States" — Interface clearly communicates and enforces different attention states, supporting the Focus Shield concept. Clear focus state indicators, user-controlled interruption levels, graceful queuing system, and context preservation.

### Principle 5 — Empowering Feedback

"Immediate Response and Positive Reinforcement" — All interactions provide immediate, confidence-building feedback. Sub-250ms response time, multi-modal feedback, progress visualization, celebration micro-interactions, and solution-focused error handling.

---

## 3. User Personas & Target Audience

**Primary Target:** Adults with ADHD (Age 22–45)

### Alex Chen — ADHD-I Professional (60% of target market)

- Marketing Manager, 32 years old
- **Challenges:** Time blindness, notification overwhelm, task switching costs
- **Strengths:** Hyperfocus capability, creative problem-solving, crisis performance
- **Key Needs:** Focus protection, minimal cognitive load, clear task prioritization

### Sam Rodriguez — ADHD-H Student (25% of target market)

- University student, 21 years old
- **Challenges:** Hyperfocus traps, impulse control, stimulation regulation
- **Strengths:** High energy, quick thinking, crisis management
- **Key Needs:** Gentle interruption management, energy regulation support, clear boundaries

### Jordan Kim — ADHD-C Creative (15% of target market)

- Creative professional, 28 years old
- **Challenges:** Variable creative cycles, client management, sensory overwhelm
- **Strengths:** Creative hyperfocus, innovative problem-solving
- **Key Needs:** Flexible focus periods, creative flow protection, minimal distractions

---

## 4. Requirements

### 4.1 Functional Requirements

| ID  | Requirement |
|-----|-------------|
| FR1 | The system shall synchronize with a user's Microsoft ToDo account via a companion phone app to display a list of tasks. |
| FR2 | The user shall be able to select a task from the list to set it as the "current task" on the Active Task Screen. |
| FR3 | The user shall be able to mark a task as "complete" from the watch, syncing back to Microsoft ToDo. |
| FR4 | The system shall provide a countdown timer that the user can start for the current task. |
| FR5 | The system shall display incoming SMS messages from a connected phone. |
| FR6 | SMS notifications must remain on-screen until the user explicitly dismisses them with a swipe gesture. |
| FR7 | The system shall implement a Focus Shield that queues all incoming SMS and calendar notifications while the Active Task Screen is visible. |
| FR8 | The user shall be able to toggle a "Compressor" control from a slide-out Modular Control Panel, sending HTTP requests to two predefined local IP addresses. |
| FR9 | The system shall display a Priority Alert (full-screen red flash) for 3 seconds upon receiving a signal from a predefined local IP address. This alert must override all other UI states. |

### 4.2 Non-Functional Requirements

| ID   | Category    | Requirement |
|------|-------------|-------------|
| NFR1 | Performance | All touch-based UI interactions must provide visual feedback in under 250ms. |
| NFR2 | Stability   | Firmware must operate without crashing, freezing, or requiring a reboot during a continuous 12-hour period of typical use. |
| NFR3 | Battery Life| At least 12 hours under typical usage (minimum three 25-minute focus sessions with always-on display active). |
| NFR4 | Usability   | UI must adhere to the five ADHD-Friendly Design Principles. |
| NFR5 | Hardware    | Firmware designed and optimized exclusively for the Waveshare ESP32-S3-Touch-LCD-2. |
| NFR6 | Modularity  | Software architecture shall be layered (HAL → Services → Application → UI). |
| NFR7 | Security    | No secrets (WiFi credentials, API keys) stored directly in the compiled firmware binary. |
| NFR8 | Display     | UI rendered in fixed horizontal orientation. Always-on low-power display during active focus timer; screen timeout otherwise. |

---

## 5. Success Metrics & KPIs

### User Engagement

| Metric                      | Target                              |
|-----------------------------|-------------------------------------|
| Daily Focus Sessions        | ≥ 1 per day per user                |
| Focus Completion Rate       | ≥ 80% completed without interruption|
| Task Completion Improvement | ≥ 40% increase vs. baseline         |
| 30-Day Retention            | > 75% consistent daily usage        |

### Technical Performance

| Metric                      | Target                                  |
|-----------------------------|-----------------------------------------|
| Touch Response Time         | < 250ms for all interactions            |
| BLE Connection Reliability  | > 98% successful establishment          |
| Battery Performance         | 12+ hours (3× 25-min focus sessions)    |
| Focus Shield Effectiveness  | > 95% notification blocking success     |

### Business

| Metric                      | Target                                          |
|-----------------------------|--------------------------------------------------|
| MVP Delivery                | Sprint 8, 100% core functionality                |
| User Satisfaction           | > 4.5 / 5.0 SUS score                           |
| Quality Standards           | > 90% unit test coverage, zero critical defects  |

---

## 6. UI Design Goals

**Overall UX Vision:** The experience is one of calm, focus, and deliberate action.

### Core Screens

1. **Watch Face / Launcher (Home)**
2. **Main Task List**
3. **Active Task Screen**
4. **Notification View**
5. **Modular Control Panel**
6. **Priority Alert View**

---

## 7. Technical Constraints & Limitations

### 7.1 Hardware

| Constraint | Detail |
|------------|--------|
| Processor  | ESP32-S3 dual-core Xtensa LX7 @ 240MHz |
| Memory     | 512KB SRAM, 384KB ROM, 8MB OPI PSRAM, 16MB Flash — large buffers (framebuffers, JSON) go in PSRAM |
| Display    | 2.0" IPS LCD, ST7789T3, native 240×320 (operated landscape 320×240 at rotation 1) |
| Touch      | Single-point capacitive — no multi-touch |
| Battery    | ~12 hours with aggressive power management |

### 7.2 Connectivity

| Constraint      | Detail |
|-----------------|--------|
| BLE Range       | 10m maximum reliable distance |
| WiFi            | Local network required for HTTP device control |
| Phone-as-Proxy  | All internet services require companion phone |
| Offline         | Limited to locally cached tasks and basic timer |

### 7.3 API & Integration

| Constraint         | Detail |
|--------------------|--------|
| Microsoft ToDo API | Rate limited to 10,000 req/hr per application |
| BLE MTU            | Maximum 247 bytes per characteristic notification |
| JSON Processing    | Maximum 2KB message size (memory-constrained) |
| Sync Latency       | 3–10 second delay depending on network conditions |

### 7.4 UI

| Constraint     | Detail |
|----------------|--------|
| Layout         | No split-screen or multi-window (display size) |
| Touch Targets  | 44px minimum reduces available real estate |
| Text           | Limited font sizes/styles due to memory |
| Animation      | Simple only — preserve battery and performance |

### 7.5 Scalability & Future

| Constraint         | Detail |
|--------------------|--------|
| Users              | Single user device — no multi-user support |
| Task Volume        | Optimal with < 100 active tasks |
| Notification Queue | Max 20 queued; oldest dropped beyond capacity |
| Feature Expansion  | Limited flash memory constrains additions |

### 7.6 Key Technical Assumptions

- Repository: Monorepo
- Architecture: Layered Monolith
- Phone-as-Proxy for all cloud API access
- State Persistence across reboots
- OTA Updates supported
- High-Priority Power Management throughout

---

## 8. Epics & Stories

### Epic Overview

| Epic   | Scope                                    | Story Points | Sprints |
|--------|------------------------------------------|--------------|---------|
| Epic 1 | Foundation, Core UI & Priority Systems   | 26           | 1–3     |
| Epic 2 | Task Management & The Focus Shield       | 32           | 4–7     |
| Epic 3 | External Connectivity & Notifications    | 18           | 8–11    |
| **Total** |                                       | **76**       | **11**  |

---

### Epic 1 — Foundation, Core UI & Priority Systems (26 SP)

**Dependencies:** Arduino/arduino-cli setup, hardware procurement, dev environment configuration
**Risk Factors:** Hardware delivery delays, toolchain compatibility issues
**Success Criteria:** Stable platform foundation with < 250ms UI response time

#### Story 1.1 — Project Initialization and Basic Boot (8 SP)

**Priority:** Critical | **Sprint:** 1 | **Dependencies:** Hardware delivery

| AC    | Criteria |
|-------|----------|
| 1.1.1 | **Build System** — Project compiles without errors using the Arduino ESP32 core (arduino-cli) within 60 seconds on dev machine. |
| 1.1.2 | **Boot Sequence** — Device completes power-on boot within 5 seconds and displays splash screen. |
| 1.1.3 | **Display Init** — 320×240 LCD initializes with correct orientation and 80% brightness. |
| 1.1.4 | **Touch Calibration** — Touch responds with visual feedback within 250ms across entire surface. |
| 1.1.5 | **Memory Validation** — System reports > 400KB available heap at boot completion. |
| 1.1.6 | **Error Handling** — Failed initialization displays clear error message with diagnostic info. |

#### Story 1.2 — Home Screen UI and Navigation Shell (10 SP)

**Priority:** High | **Sprint:** 1–2 | **Dependencies:** Story 1.1, LVGL integration

| AC    | Criteria |
|-------|----------|
| 1.2.1 | **Home Display** — Current time in 48px font, date in 24px font beneath. |
| 1.2.2 | **Navigation Icons** — Four icons (Tasks, Timer, Notifications, Settings) in 2×2 grid, 44px minimum touch targets. |
| 1.2.3 | **Visual Hierarchy** — Time at 7:1 contrast ratio, navigation icons at 4.5:1. |
| 1.2.4 | **Touch Response** — Tap provides color-change feedback within 100ms, navigation within 250ms. |
| 1.2.5 | **Battery Indicator** — Percentage display with amber low-battery warning at < 20%. |
| 1.2.6 | **ADHD Compliance** — Adheres to "Singular Focus" principle with time as primary element. |

#### Story 1.3 — Priority Alert System Implementation (8 SP)

**Priority:** Critical | **Sprint:** 2 | **Dependencies:** WiFi connectivity, HTTP client

| AC    | Criteria |
|-------|----------|
| 1.3.1 | **Signal Reception** — Receives HTTP POST from configurable IP within 2 seconds of transmission. |
| 1.3.2 | **Alert Override** — Overrides any current UI state within 500ms of signal reception. |
| 1.3.3 | **Visual Alert** — Full-screen red background (RGB 255,0,0) for exactly 3.0s ± 100ms. |
| 1.3.4 | **Alert Text** — "PRIORITY ALERT" in 36px white font, centered. |
| 1.3.5 | **Acknowledgment** — Touch anywhere dismisses alert and returns to previous UI state. |
| 1.3.6 | **Audio Feedback** — 100ms vibration pulse accompanies visual alert (if hardware supports). |
| 1.3.7 | **Logging** — Alert events logged with timestamp. |

---

### Epic 2 — Task Management & The Focus Shield (32 SP)

**Dependencies:** BLE stack, Microsoft ToDo API integration, companion app development
**Risk Factors:** BLE connection stability, API rate limiting, focus timer accuracy
**Success Criteria:** > 98% BLE connection reliability, > 80% focus session completion rate

#### Story 2.1 — Bluetooth Connectivity & Handshake (8 SP)

**Priority:** Critical | **Sprint:** 3 | **Dependencies:** BLE stack, pairing workflow

| AC    | Criteria |
|-------|----------|
| 2.1.1 | **BLE Init** — BLE stack initializes and begins advertising within 3 seconds of boot. |
| 2.1.2 | **Pairing** — Secure numeric comparison pairing with companion phone within 30 seconds. |
| 2.1.3 | **Connection** — > 98% success rate across 10 test attempts. |
| 2.1.4 | **Message Exchange** — Bidirectional exchange confirmed with 100% data integrity. |
| 2.1.5 | **Stability** — Connection maintained > 10 minutes with < 2% packet loss. |
| 2.1.6 | **Reconnection** — Automatic reconnection within 10 seconds after drop. |
| 2.1.7 | **Error Handling** — Clear error messages for pairing failures with retry mechanism. |

#### Story 2.2 — Task Synchronization and Display (10 SP)

**Priority:** High | **Sprint:** 4 | **Dependencies:** Story 2.1, JSON parsing

| AC    | Criteria |
|-------|----------|
| 2.2.1 | **Sync Request** — Watch initiates sync and receives response within 5 seconds. |
| 2.2.2 | **Data Format** — Tasks in standardized JSON with id, title, isComplete, priority. |
| 2.2.3 | **Task Display** — Scrollable list of up to 20 tasks, titles truncated at 30 characters. |
| 2.2.4 | **Visual Indicators** — Completed tasks with strikethrough; incomplete with checkbox icon. |
| 2.2.5 | **Priority Colors** — High = red accent, Medium = amber, Low = green. |
| 2.2.6 | **Touch Response** — Task selection feedback within 100ms. |
| 2.2.7 | **Persistence** — Tasks cached locally and survive reboot until next sync. |

#### Story 2.3 — The Active Task Screen UI (6 SP)

**Priority:** High | **Sprint:** 4 | **Dependencies:** Story 2.2, UI layout framework

| AC    | Criteria |
|-------|----------|
| 2.3.1 | **Layout** — Split-screen: task details (left 60%) and timer controls (right 40%). |
| 2.3.2 | **Task Info** — Title in 20px font with priority indicator and description. |
| 2.3.3 | **Timer Display** — Countdown in 36px font, MM:SS format. |
| 2.3.4 | **Controls** — Start, Pause, Stop buttons with 44px touch targets and clear labels. |
| 2.3.5 | **Visual States** — Timer state (idle / running / paused) indicated by color coding. |
| 2.3.6 | **Navigation** — Back button returns to task list, maintaining scroll position. |
| 2.3.7 | **ADHD Compliance** — Adheres to "Singular Focus" and "Immediate Clarity" principles. |

#### Story 2.4 — Functional Focus Timer (8 SP)

**Priority:** Critical | **Sprint:** 5 | **Dependencies:** Story 2.3, power management

| AC    | Criteria |
|-------|----------|
| 2.4.1 | **Accuracy** — Countdown accurate within ± 1 second over a 25-minute period. |
| 2.4.2 | **State Management** — Start, pause, resume work reliably with immediate UI feedback. |
| 2.4.3 | **Always-On Display** — Screen remains active during timer at reduced brightness (40%). |
| 2.4.4 | **Completion Alert** — Full-screen green flash + vibration for 3 seconds on completion. |
| 2.4.5 | **Background Operation** — Timer continues when user navigates away. |
| 2.4.6 | **Power Management** — Timer operation maintains 12+ hour battery life. |
| 2.4.7 | **Session Logging** — Sessions logged with start time, duration, and completion status. |

#### Story 2.5 — The Focus Shield Implementation (6 SP)

**Priority:** High | **Sprint:** 6 | **Dependencies:** Notification system, queue management

| AC    | Criteria |
|-------|----------|
| 2.5.1 | **Activation** — Focus Shield auto-activates when Active Task Screen is displayed. |
| 2.5.2 | **Queuing** — All incoming notifications queued in memory (max 20) during focus. |
| 2.5.3 | **Queue Overflow** — Oldest notifications dropped at capacity with logging. |
| 2.5.4 | **Indicator** — Shield icon shows Focus Shield active status. |
| 2.5.5 | **Exception** — Priority Alerts (Story 1.3) override Focus Shield immediately. |
| 2.5.6 | **Deactivation** — Shield deactivates on timer completion or Active Task Screen exit. |
| 2.5.7 | **Effectiveness** — > 95% of notifications successfully queued without interruption. |

---

### Epic 3 — External Connectivity & Notifications (18 SP)

**Dependencies:** Notification system, HTTP client, gesture recognition
**Risk Factors:** Network latency, gesture recognition accuracy, notification overflow
**Success Criteria:** > 75% notification engagement rate, < 3 second HTTP response time

#### Story 3.1 — Queued Notification Display (6 SP)

**Priority:** High | **Sprint:** 7 | **Dependencies:** Story 2.5, notification queue

| AC    | Criteria |
|-------|----------|
| 3.1.1 | **Queue Processing** — Notifications display sequentially in FIFO order after Shield deactivation. |
| 3.1.2 | **Layout** — Each shows sender, 50-character message preview, and timestamp. |
| 3.1.3 | **Swipe Dismiss** — Left swipe dismisses with smooth animation within 200ms. |
| 3.1.4 | **Feedback** — Swipe progress shown with sliding animation and fade-out. |
| 3.1.5 | **Queue Status** — Counter shows "X of Y" remaining notifications. |
| 3.1.6 | **Auto-Advance** — Next notification auto-displays after 1-second delay. |
| 3.1.7 | **Exit** — User can exit review mode and return notifications to queue. |

#### Story 3.2 — Task Completion Sync (4 SP)

**Priority:** High | **Sprint:** 8 | **Dependencies:** Story 2.3, Microsoft ToDo API

| AC    | Criteria |
|-------|----------|
| 3.2.1 | **Complete Button** — Prominent button on Active Task Screen with confirmation animation. |
| 3.2.2 | **Sync Request** — Completion signal sent to phone within 1 second of press. |
| 3.2.3 | **API Integration** — Phone marks task complete in Microsoft ToDo within 10 seconds. |
| 3.2.4 | **Confirmation** — Visual checkmark animation on watch when sync succeeds. |
| 3.2.5 | **Error Handling** — Sync failure shows retry option with clear error message. |
| 3.2.6 | **Offline** — Completion cached locally when offline; synced on reconnect. |
| 3.2.7 | **Data Integrity** — 100% accuracy in completion status across watch and ToDo. |

#### Story 3.3 — Modular Control Panel UI & Gesture (4 SP)

**Priority:** Medium | **Sprint:** 9 | **Dependencies:** Gesture recognition, overlay UI

| AC    | Criteria |
|-------|----------|
| 3.3.1 | **Right Edge Swipe** — Swipe from right edge (within 20px) reveals panel with slide-in animation. |
| 3.3.2 | **Appearance** — Translucent overlay (70% opacity) with rounded corners and blur. |
| 3.3.3 | **Layout** — Controls in vertical list with 44px minimum touch targets. |
| 3.3.4 | **Left Edge Swipe** — Swipe from left edge dismisses panel within 300ms. |
| 3.3.5 | **Touch Outside** — Tap outside panel dismisses and returns to previous screen. |
| 3.3.6 | **State Indicators** — Active controls show enabled state; inactive appear dimmed. |
| 3.3.7 | **Performance** — Panel animations maintain 30+ FPS without UI lag. |

#### Story 3.4 — Network Device Control Implementation (4 SP)

**Priority:** Low | **Sprint:** 10 | **Dependencies:** Story 3.3, HTTP client, local network config

| AC    | Criteria |
|-------|----------|
| 3.4.1 | **Compressor Button** — Toggle in control panel labeled "Compressor" with on/off states. |
| 3.4.2 | **HTTP Requests** — Button press sends HTTP POST to two predefined IPs within 3 seconds. |
| 3.4.3 | **Configuration** — IP addresses configurable through companion app settings. |
| 3.4.4 | **Request Format** — Proper headers and JSON payload with device state. |
| 3.4.5 | **Response Handling** — Responses validated; success/failure indicated on button. |
| 3.4.6 | **Timeout** — 5-second timeout with appropriate error message. |
| 3.4.7 | **Resilience** — Graceful handling of network unavailability with retry mechanism. |
