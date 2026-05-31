# QGC Integration

This directory is **staging** for files that ultimately live in
`qgroundcontrol/src/AutoPilotPlugins/APM/`. Work in this repo, copy/link
into a QGC checkout for compilation.

## Final placement

```
qgroundcontrol/
└── src/AutoPilotPlugins/APM/
    ├── APMAutoPilotPlugin.cc           ← modified to register component
    ├── APMOSDComponent.cc              ← from qgc/src/
    ├── APMOSDComponent.h
    ├── APMOSDComponentController.cc
    ├── APMOSDComponentController.h
    └── qml/
        ├── APMOSDComponent.qml         ← from qgc/qml/
        └── APMOSDComponentSummary.qml

qgroundcontrol/src/qgcunittest/
├── APMOSDControllerTest.cc             ← from qgc/tests/
└── APMOSDControllerTest.h
```

You'll also need to:

1. Add the new sources to QGC's build system. Look at how
   `APMPowerComponent` is listed in `qgroundcontrol.pro` (or the CMake
   equivalent in recent versions) and add `APMOSDComponent*` alongside.
2. Add the QML files to `APMResources.qrc` (or whatever resource file
   the APM plugin uses). Pattern:
   ```xml
   <file alias="APMOSDComponent.qml">qml/APMOSDComponent.qml</file>
   ```
3. Register the unit test class via `UT_REGISTER_TEST(APMOSDControllerTest)`
   in the test file and confirm it appears in the runner.

## What's scaffolded vs what's TODO

| File                              | State                                       |
| --------------------------------- | ------------------------------------------- |
| `APMOSDComponent.h/.cc`           | Complete enough to compile. May need adjustment to match your QGC branch's `VehicleComponent` API. |
| `APMOSDComponentController.h`     | Complete API surface, all `Q_PROPERTY` / `Q_INVOKABLE` declared. |
| `APMOSDComponentController.cc`    | Skeleton with element catalogue, Fact lookups, and active-screen logic done. `importParamText`, `exportParamText`, `detectOverlaps`, and `clampActiveScreen` are stubs with reference algorithms in comments. |
| `APMOSDComponent.qml`             | Three-column layout with palette/canvas/inspector structure and Fact bindings. File-dialog handlers are stubs. Canvas drag-and-drop is wired but uses basic `drag.target` — port the prototype's snap-to-cell logic to QML. |
| `APMOSDComponentSummary.qml`      | Done. |
| `APMOSDControllerTest.{h,cc}`     | All 12 test cases declared, currently `QSKIP`'d. Replace each `QSKIP` with the actual test as the matching controller method is implemented. |

## How to work with Claude Code on this

Start each session by referencing the task you're picking up:

```
Read tasks/03-port-import-export.md and CLAUDE.md, then start work.
```

When you finish a task, update `CLAUDE.md`'s "Current state" table.

## When stuck on Qt API

The most likely friction points and where to look:

| Question                                          | Reference                                                   |
| ------------------------------------------------- | ----------------------------------------------------------- |
| "How do I declare a setup component?"             | `qgroundcontrol/src/AutoPilotPlugins/APM/APMPowerComponent.h` |
| "How do FactPanelController + QML bind together?" | `qgroundcontrol/src/FactSystem/FactPanelController.h`       |
| "How does a Fact get written?"                    | `qgroundcontrol/src/FactSystem/Fact.h` — `setRawValue()`    |
| "How do unit tests run?"                          | `qgroundcontrol/src/qgcunittest/` — look at any `*Test.cc`  |
| "Where do new components register?"               | `APMAutoPilotPlugin::vehicleComponents()` — already wired pattern for Power, Sensors, etc. |
