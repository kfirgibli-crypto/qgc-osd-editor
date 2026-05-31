# Task 06 — SITL Integration Validation (Tier 3)

**Estimated:** 1–2 sessions (4–6 hours)
**Type:** Integration testing
**Preconditions:** Task 05 complete (component is in QGC).

## Goal

The component works against a live ArduPilot SITL. Param reads, writes,
multi-screen, resolution changes, file import — all validated against
real MAVLink traffic.

## Setup

### Terminal A — ArduPilot SITL

```bash
cd ardupilot
./Tools/autotest/sim_vehicle.py -v ArduCopter --console --map
```

Wait for `GPS_GLITCH cleared` and other init messages to settle.

### Terminal B — QGC

```bash
cd qgroundcontrol/build
./Release/qgroundcontrol
```

Connect via UDP 14550. The vehicle should appear and load parameters.

## Test cases

Walk through every item in **Tier 3** of `docs/test-plan.md`:

1. Component appears in sidebar
2. Reads existing OSD1_* params correctly
3. Drag an element → param updates in SITL (`param show OSD1_BAT_VOLT_X`)
4. Switch to Screen 2, configure it, switch back — Screen 1 state preserved
5. Change resolution → elements clamped
6. Import a `.param` file → layout updates → params written
7. Disconnect QGC → page still works in offline mode
8. Build-flag-conditional elements absent → palette hides them gracefully

For each test, capture:
- The exact action you took in QGC
- The expected SITL state (run `param show OSDx_...` to verify)
- The actual SITL state
- Pass/fail

## Prompt to give Claude Code

> Read `CLAUDE.md`, then `tasks/06-sitl-validation.md`.
> 
> I'm running ArduCopter SITL on UDP 14550 with QGC connected. The OSD
> component is visible in the sidebar. Help me walk through the 8 Tier 3
> tests in `docs/test-plan.md`.
> 
> For each test: tell me exactly what to do in QGC, what to look for in
> SITL, and what to do if it fails. When something fails, suggest the
> most likely cause based on the controller code in
> `qgc/src/APMOSDComponentController.cc`.

## Acceptance

All 8 Tier 3 tests pass. Bugs found are filed as their own follow-up
tasks or fixed inline if trivial.

## Likely failure modes & where to look

| Symptom                                         | Most likely cause                                     |
| ----------------------------------------------- | ----------------------------------------------------- |
| "Param doesn't update in SITL after drag"       | `Fact::setRawValue` not called, or wrong component ID |
| "Page is blank, no elements"                    | `elementKeys` empty, or `elEnabledFact` returns null  |
| "Element vanishes when I enable it"             | X/Y are 0 and stay 0 — `DEFAULT_LAYOUT` not consulted on first-enable |
| "Drag is jittery"                               | `setRawValue` per pixel — batch on release            |
| "Wrong screen's params written"                 | `_paramName` building wrong screen prefix             |
| "Can't import .param"                           | `importParamText` not implemented yet (Task 03)       |

## When you find a bug

1. Reproduce it with minimal steps
2. Add a failing Qt Test case in `APMOSDControllerTest.cc`
3. Fix until the test passes
4. Re-verify the SITL scenario

This is the loop that produces a shippable feature.
