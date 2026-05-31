# Task 05 — Wire Into APMAutoPilotPlugin

**Estimated:** 1 session (2–3 hours)
**Type:** Plumbing / integration
**Preconditions:** Tasks 03 and 04 complete.

## Goal

The new OSD component appears in QGC's SetupView sidebar when connected
to an ArduPilot vehicle. Clicking it opens the editor page.

## What needs to happen

1. **Copy/symlink files into QGC's tree.** From this repo's `qgc/src/`
   and `qgc/qml/` to `qgroundcontrol/src/AutoPilotPlugins/APM/` and
   `.../APM/qml/`. A symlink is fine during development.

2. **Add files to QGC's build system.** Pattern depends on whether
   you're on the qmake or CMake branch. For CMake:
   ```cmake
   # In src/AutoPilotPlugins/APM/CMakeLists.txt
   target_sources(qgcppi PRIVATE
       APMOSDComponent.cc
       APMOSDComponent.h
       APMOSDComponentController.cc
       APMOSDComponentController.h
   )
   ```

3. **Register QML in the resource file.** In `APMResources.qrc` (or the
   plugin's main qrc, name varies by branch):
   ```xml
   <file alias="APMOSDComponent.qml">qml/APMOSDComponent.qml</file>
   <file alias="APMOSDComponentSummary.qml">qml/APMOSDComponentSummary.qml</file>
   ```

4. **Register the controller as a QML type.** In `QGCApplication.cc`
   (or wherever other APM controllers register — search for
   `APMPowerComponentController`):
   ```cpp
   qmlRegisterType<APMOSDComponentController>(
       "QGroundControl.Controllers", 1, 0, "APMOSDComponentController");
   ```

5. **Add to the plugin's component list.** In
   `APMAutoPilotPlugin.cc`:
   ```cpp
   QList<VehicleComponent*>& APMAutoPilotPlugin::vehicleComponents() {
       if (_components.isEmpty() && !_vehicle->isOfflineEditingVehicle()) {
           // ... existing components ...
           _osdComponent = new APMOSDComponent(_vehicle, this);
           _osdComponent->setupTriggerSignals();
           _components.append(qobject_cast<VehicleComponent*>(_osdComponent));
       }
       return _components;
   }
   ```
   And declare `_osdComponent` in the header.

6. **Build, run, observe.** The OSD component should appear in the
   sidebar. Click it. Page loads. Connected to nothing, it'll show
   empty Facts — that's fine; we test against SITL in Task 06.

## Prompt to give Claude Code

> Read `CLAUDE.md`, then `tasks/05-wire-into-plugin.md`.
> 
> I need to wire the OSD component files (now staged in `qgc/`) into a
> QGC checkout at `../qgroundcontrol/`. Walk through the 6 steps in the
> task file. At each step, run an incremental build to catch errors fast.
> 
> When the build is green and QGC launches with the OSD component in the
> sidebar, update `CLAUDE.md`'s "Current state" and stop.

## Acceptance

- QGC builds cleanly with the OSD component sources included
- Component appears in SetupView sidebar with the OSD icon
- Clicking it opens the editor page
- Page renders without QML errors in the console
- Offline-editing mode works (page doesn't crash without a vehicle)

## Gotchas

- **`setupTriggerSignals()` must be called** after construction; the
  `VehicleComponent` base relies on it to connect change signals.
- **Icon resource missing**: the `kIconResource` path in
  `APMOSDComponent.h` is `/qmlimages/OSDComponentIcon.png`, which
  doesn't exist yet. Either drop a placeholder in QGC's `qmlimages`
  resource, or change the path to an existing icon
  (`/qmlimages/PowerComponentIcon.png` works as a temporary stand-in).
- **QML import not found**: if the component appears but the page is
  blank with a `QGroundControl.Controllers` import error, the
  `qmlRegisterType` call (step 4) was missed.
