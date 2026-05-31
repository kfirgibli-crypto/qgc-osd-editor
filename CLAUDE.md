# CLAUDE.md

> This file is read by Claude Code at the start of every session. Keep it tight, current, and accurate. Stale instructions here will mislead future sessions.

## What this project is

A **QGroundControl SetupView component** that lets users edit ArduPilot OSD layouts (the elements rendered on DJI O3 / Walksnail / HDZero goggles) directly inside QGC, instead of having to switch to Mission Planner. It targets ArduPilot's existing `OSDn_*` parameter schema — no firmware work is required.

Two halves to the project:

- **`prototype/`** — A working browser-based editor that already implements the full UX and the parameter file format. Treated as an **executable spec** for the QGC port. Every behavior decision should be made here first, then ported.
- **`qgc/`** — The actual QGC component (C++ controller + QML page + Qt tests). Currently a scaffolded skeleton with TODO markers — this is where most new work happens.

## Repo map

```
qgc-osd-editor/
├── CLAUDE.md                         ← you are here
├── README.md                         ← human-facing intro
├── package.json                      ← npm test, npm run serve
├── prototype/
│   ├── index.html                    ← full editor, open in browser
│   ├── osd-params.js                 ← pure data layer (the spec)
│   └── tests/
│       ├── test-osd-params.js        ← 27 tests, all green
│       └── test-inline-parity.js     ← inlined-vs-module check
├── qgc/
│   ├── README.md                     ← integration overview
│   ├── src/
│   │   ├── APMOSDComponent.h         ← VehicleComponent subclass
│   │   ├── APMOSDComponent.cc
│   │   ├── APMOSDComponentController.h
│   │   └── APMOSDComponentController.cc   ← bulk of new C++ work
│   ├── qml/
│   │   ├── APMOSDComponent.qml       ← the editor page
│   │   └── APMOSDComponentSummary.qml
│   └── tests/
│       └── APMOSDControllerTest.cc   ← Qt Test cases
├── docs/
│   ├── parameters-spec.md            ← AP_OSD param inventory (from source)
│   ├── qgc-integration-notes.md      ← architecture + porting guide
│   └── test-plan.md                  ← four-tier test plan
├── fixtures/                         ← real .param files for testing
├── scripts/
│   ├── test.sh                       ← run all JS tests
│   └── serve.sh                      ← serve prototype on localhost:8000
└── tasks/                            ← ordered task prompts for Claude Code
    └── 01..07_*.md
```

## Working norms

### Test-driven, in this exact order

1. Every JS data-layer change must keep `npm test` green (27 tests + 6 parity checks).
2. Every C++ port of a JS function must come with a Qt Test case that mirrors a JS test. The mapping is in `docs/qgc-integration-notes.md`. **Don't write controller code without writing the test first.**
3. UX changes must be made in `prototype/` first, validated by Liat in a browser, then ported to QML.
4. Never silently change behavior in `osd-params.js` without updating both the tests and the matching C++ implementation in lock-step.

### Param naming is authoritative

ArduPilot's `libraries/AP_OSD/AP_OSD_Screen.cpp` is the source of truth for element names. If a param name conflict ever appears, re-read that file via web search; don't trust assumptions. `docs/parameters-spec.md` is a cache of that source — verify it's current before relying on it for new elements.

### Inlining discipline

`prototype/index.html` has a copy of `osd-params.js` inlined inside a `<script>` block (for zero-dependency browser use). The `test-inline-parity.js` test enforces that the inlined version matches the module byte-for-byte after function extraction. **If you change `osd-params.js`, re-inline it into `index.html` and run the parity test.** Use `scripts/sync-inline.sh` (TODO: create).

### Style and conventions

- **JS**: vanilla, no build step, no framework. ES2020+, `'use strict'` implicit. Pure functions in `osd-params.js`; DOM-touching code lives in `index.html` only.
- **C++**: Qt 6 conventions. `Q_OBJECT` macro, `Q_PROPERTY` for QML-visible state, `Q_INVOKABLE` for QML-callable methods. Return `Fact*` from invokables, never `Fact` by value.
- **QML**: Match existing QGC patterns from `src/AutoPilotPlugins/APM/qml/APMPowerComponent.qml`. Use `FactComboBox`, `FactTextField`, `QGCLabel`, `QGCButton`. Never write a raw `Item` when a QGC widget exists.
- **No TypeScript, no React, no build tooling** in the prototype. The point is that it runs anywhere with no setup.

### What NOT to do

- Don't add npm dependencies to the prototype. The test runner is a hand-rolled 30-line harness on purpose — Liat needs to be able to run tests on any machine in any condition.
- Don't refactor the prototype into a framework "to make porting easier." The QGC port doesn't share runtime; the prototype's job is to make decisions easy, not to share code.
- Don't write QGC code that requires a vehicle connection. The component must work in offline editing mode (`_vehicle->isOfflineEditingVehicle()`).
- Don't expand the element set past the 25 in `osd-params.js` before the QGC port works end-to-end with those 25. ArduPilot has ~55 elements; adding the rest is mechanical and goes last.
- Don't introduce file-watching, hot-reload, or dev servers. `python3 -m http.server` is fine.

## Current state (update this when it changes)

| Component                            | Status       | Notes                                         |
| ------------------------------------ | ------------ | --------------------------------------------- |
| Local QGC build (Win MSVC)           | ✅ Built     | Against upstream `d0a32dc7c` (2026-05-26) on Qt 6.10.3 / MSVC 19.44 / GStreamer 1.28.1. See "Environment expectations" for launch PATH. Local patch: `src/Comms/CMakeLists.txt` gates `Bluetooth` subdir with `if(TARGET Qt6::Bluetooth)`. |
| `osd-params.js` data layer           | ✅ Complete  | 27 tests passing                              |
| `index.html` interactive editor      | ✅ Complete  | Needs Liat's UX validation pass               |
| Inline-vs-module parity              | ✅ Enforced  | 6 parity checks passing                       |
| Test fixtures                        | ✅ Drafted   | 3 .param files in `fixtures/`                 |
| `APMOSDComponentController.{h,cc}`   | 🟢 Methods done | `importParamText`, `exportParamText`, `detectOverlaps`, `clampActiveScreen` implemented (mirror `osd-params.js`). Plus QML-facing `importParamTextFromUrl`/`exportParamTextToUrl` (QFile-backed) and `overlapsChanged` signal added in Task 04. Execution unverified pending Task 05 plugin wiring. |
| `APMOSDComponent.{h,cc}`             | 🟢 Wired     | Fixed to current QGC API (3-arg VehicleComponent ctor with `UnknownVehicleComponent`, `summaryQmlSource()` not `setupSummaryItem()`, qrc paths under `QGroundControl.AutoPilotPlugins.APM` module). Copied into `qgroundcontrol/src/AutoPilotPlugins/APM/` and added to `APMAutoPilotPlugin::vehicleComponents()`. Visible only when connected to an actual ArduPilot vehicle - offline-editing mode in this QGC version doesn't populate components. |
| `APMOSDComponent.qml`                | 🟠 Loads but renders blank | Sidebar entry visible, page selectable, but the body renders empty against a real ArduCopter vehicle. Two real bugs found + fixed: missing `width: availableWidth`/`height: availableHeight` on root ColumnLayout, and `import QGroundControl.ScreenTools` (not a valid module — silently killed the file). Minimal red-rectangle diagnostic version renders perfectly with the same scaffold, so structure is fine; bug is in the body. **See `RESUME.md` for full debug ideas list.** |
| `APMOSDComponentSummary.qml`         | 🟢 Done      | Item-based sidebar summary; shows OSD_TYPE backend + per-screen enabled count. Original scaffold used legacy `FactPanel` (removed from QGC) - rewrote to current `VehicleSummaryRow` pattern. |
| `APMOSDControllerTest.cc`            | 🟢 Bodies staged | All 13 test bodies ported from JS, guarded by `if (!_controller) QSKIP(...)`. Auto-activate when Task 05 wires `init()` to a real Vehicle + Controller per `src/qgcunittest/FactSystemTestGeneric.cc`. |
| Wiring into `APMAutoPilotPlugin`     | ✅ Live-verified | Cube Orange flashed to ArduCopter V4.6.3 (2026-05-30); OSD entry confirmed visible in Vehicle Configuration sidebar between Motors and Power. Wiring works end-to-end against a real ArduPilot vehicle. |
| QGC SITL integration validation      | 🟠 Partial   | Real-vehicle integration verified for the sidebar entry. Editor UI rendering blocked on the open QML bug — see `RESUME.md`. SITL itself was never set up (skipped in favor of live Cube test). |
| Hardware loop validation             | ⬜ TODO      | Tier 4 in `docs/test-plan.md`                 |
| Upstream QGC PR                      | ⬜ TODO      | Last step                                     |

## How sessions usually go

Pick a task file from `tasks/`, read it end to end, then say "let's do task N." Each task file states its preconditions, the work itself, and the acceptance criteria. Don't combine tasks — they're sized to be one focused session each.

When in doubt about ArduPilot behavior, web-search `libraries/AP_OSD/AP_OSD_Screen.cpp` on github.com/ArduPilot/ardupilot rather than guessing.

When in doubt about QGC plugin conventions, look at `src/AutoPilotPlugins/APM/APMPowerComponent.*` — it's the closest existing analogue to what we're building.

## Environment expectations

- Node 18+ for tests (`npm test`)
- Python 3 for `scripts/serve.sh` (only if Liat wants to serve via http instead of `file://`)
- A QGC source checkout at `../qgroundcontrol/` (sibling to this repo). The `qgc/` directory in this repo is **staging**; final files end up under `qgroundcontrol/src/AutoPilotPlugins/APM/`.
- ArduPilot SITL for Tier 3 testing
- A flight controller + DJI O3 + goggles for Tier 4 (Liat owns)

### Where to resume

Read `RESUME.md` in this repo first. It contains the full session recap, the two bugs already fixed (don't reintroduce them), and a prioritized list of debug ideas for the remaining "page renders blank against real ArduPilot vehicle" issue. The user verified the bug reproduces on a second computer — it's not a local environment problem.

### Windows build/launch (current dev box)

Project moved from `C:\Users\kfir\Desktop\APPS` → **`F:\APPS`** (C: was full). Toolchain installed entirely on F:\ for the same reason:

- **MSVC**: `F:\BuildTools` (vcvars: `F:\BuildTools\VC\Auxiliary\Build\vcvars64.bat`)
- **Qt 6.10.3**: `F:\QTt\6.10.3\msvc2022_64` (yes, the folder is `QTt` with two t's — typo at install time, not worth renaming)
- **Build dir**: `F:\APPS\qgroundcontrol\build`
- **Binary**: `F:\APPS\qgroundcontrol\build\Release\QGroundControl.exe`

Re-build = `F:\APPS\qgroundcontrol\build\build.bat`. Re-configure = `configure.bat` in the same dir.

To **launch the built QGC**, three dirs must be on PATH (Qt bin, GStreamer CPM cache, optional MSVC redist):
```
F:\QTt\6.10.3\msvc2022_64\bin
F:\APPS\qgroundcontrol\.cache\CPM\gstreamer-win-x86_64-1.28.1\sdk\bin
```
Without GStreamer on PATH the .exe loads but never shows a window (silent `STATUS_DLL_NOT_FOUND` at init). `windeployqt` already ran once and bundled Qt QML plugins into `Release/`.

Python 3.13 (not 3.12) is what CMake picks for codegen — `jinja2` and `defusedxml` must be installed in 3.13.

## When you finish a task

Update the "Current state" table above before ending the session. Stale state lies to the next session.
