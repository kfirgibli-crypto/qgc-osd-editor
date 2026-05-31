# Task 07 — Hardware Validation & Upstream PR

**Estimated:** 1–2 sessions hardware testing + 2–6 weeks PR review
**Type:** Real-world validation
**Preconditions:** Task 06 complete (SITL tests all pass).

## Goal

The component works on real hardware against real DJI O3 goggles, and
ultimately ships as a PR to QGC upstream.

## Phase 1 — Bench test (Tier 4)

Walk through every item in **Tier 4** of `docs/test-plan.md`:

1. FC connected to QGC via USB
2. O3 air unit connected to a FC UART
3. `OSD_TYPE = 5` and `SERIALn_PROTOCOL = 42`
4. Goggle settings → Display → Custom OSD enabled
5. Move an element in QGC → element moves on goggles
6. Switch screens via RC → goggle shows different layout
7. Resolution mismatch handled gracefully

Capture every failure with:
- Action taken in QGC
- What appeared on goggles
- Relevant param values via Mission Planner or MAVProxy

## Phase 2 — Field test

Once bench tests pass:

1. Mount FC + O3 on the airframe
2. Power up, arm, hover briefly
3. Verify telemetry updates in real-time on goggles (RSSI, battery, altitude)
4. Land, review

Don't fly aggressively until you've confirmed all critical elements
(BAT_VOLT, ALTITUDE, FLTMODE, RSSI, ARMING) are stable.

## Phase 3 — Documentation

Update or create:

- `qgroundcontrol/src/AutoPilotPlugins/APM/APMOSDComponent.qml` header
  comment with usage notes
- ArduPilot wiki / DJI section if your testing surfaces gotchas
- Screenshots for the PR description (annotated)

## Phase 4 — PR to upstream

```bash
cd qgroundcontrol
git checkout -b feat/osd-layout-editor-apm
git add src/AutoPilotPlugins/APM/APMOSDComponent* src/qgcunittest/APMOSD*
git commit -m "Add APM OSD layout editor component

Adds a new VehicleComponent for editing ArduPilot OSD layouts directly
in QGC, replacing the need to switch to Mission Planner for OSD config.

- Supports all OSD1..OSD4 screens
- SD 30x16 and HD 50x18/60x22 (MSP DisplayPort) resolutions
- 25 most common OSD elements (extensible to full AP_OSD catalogue)
- Mission Planner .param file import/export round-trips losslessly
- Offline editing supported
- Qt Test coverage for parameter file IO and overlap detection
- Tested against ArduCopter SITL and DJI O3 hardware

Closes #..."
git push -u origin feat/osd-layout-editor-apm
```

Open the PR on GitHub. Expected review process:

- Initial CI run (auto)
- Maintainer comments within 1–2 weeks
- 1–3 review rounds, each ~1 week
- Merge after maintainer approval + clean CI

## Prompt to give Claude Code

> Read `CLAUDE.md`, then `tasks/07-hardware-and-pr.md`.
> 
> [Status: bench-tested / field-tested / drafting PR]
> 
> Help me [next concrete action]. Don't skip ahead — each phase has its
> own loop of test → fix → re-test.

## Acceptance

- All Tier 4 tests pass on hardware
- Field-tested with no critical bugs
- PR opened, CI green
- Tracking PR review comments in this task or a successor task

## Tips for the review

- **Pre-empt the obvious questions** in the PR description: "Why APM
  only? Because PX4 lacks the equivalent OSD param schema; that's a
  separate follow-up issue." This saves a review round.
- **Include a 30-second video** of the editor in action, dragging
  elements while connected to a vehicle. Static screenshots
  undersell this feature.
- **Link to the prototype** so reviewers can play with the UX before
  diving into code.
- **Be responsive but not rushed.** Maintainers have day jobs.
  Reply within 1–2 days, but it's fine to take a week per round if
  the requested changes are substantial.
