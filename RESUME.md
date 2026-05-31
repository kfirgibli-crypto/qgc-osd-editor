# RESUME — pick-up notes for the next session

**Snapshot date:** 2026-05-30 (session ran 2026-05-27 through 2026-05-30)
**Cube state at end of session:** Cube Orange running ArduCopter V4.6.3 (flashed during this session, BARO1 board-validation error present but not blocking QGC integration)
**QGC build state:** `F:\APPS\qgroundcontrol\build\Release\QGroundControl.exe` rebuilt successfully; loads cleanly; OSD entry appears in sidebar; **full editor page now RENDERS CORRECTLY (blank-page bug SOLVED 2026-05-30)** — palette, canvas+grid, inspector, status bar all visible against a real Cube.

---

## TL;DR — BLANK PAGE SOLVED (2026-05-30)

The full OSD editor now renders correctly. Root cause of the blank page was **invalid QML module imports** — the same failure mode as the earlier ScreenTools fix. The file imported THREE non-existent module URIs, each of which silently kills the entire file load (blank page, no capturable error on the Windows GUI build):
- `import QGroundControl.ScreenTools` (found earlier) — ScreenTools is a singleton from `.Controls`
- `import QGroundControl.FactSystem` (found 2026-05-30) — `Fact` comes from `.FactControls`
- `import QGroundControl.Palette` (found 2026-05-30) — `QGCPalette` comes from `.Controls`

Correct/sufficient imports: `QtCore, QtQuick, QtQuick.Controls, QtQuick.Layouts, QtQuick.Dialogs` + `QGroundControl, QGroundControl.FactControls, QGroundControl.Controls`.

How it was found (the prior session's stderr-grep was a dead end — QGC's Windows GUI build writes QML load errors to NO capturable log): on-screen bisect with a minimal diagnostic page, then validating every `import QGroundControl.X` against the real `qt_add_qml_module()` URI list (`grep -rhoE "URI[[:space:]]+QGroundControl[A-Za-z.]*" src/`). The `width/height: availableWidth/Height` structural fix from before is still in place and still needed.

A backup of the pre-fix full version is at `src/AutoPilotPlugins/APM/APMOSDComponent.qml.fullbak` (can be deleted).

### Build source is CLEAN (verified 2026-05-30)
`src/AutoPilotPlugins/APM/APMAutoPilotPlugin.cc` is a clean 281-line file (an earlier note in this session wrongly called it "corrupted" — that was a misread of a garbled UTF-16 tool dump; retracted). `git status` shows only the intentional OSD-wiring edits: APMAutoPilotPlugin.cc/.h, CMakeLists.txt (x2), SettingsPages.json modified, plus untracked APMOSDComponent* files. The OSD component is constructed/appended once at lines 177-179; Sensors at 164-166. Safe to rebuild.

### Sensor calibration fails on TWO different Cubes (Orange AND Black)
Can't calibrate accel / level horizon / compass on either board. The plugin's Sensors component is constructed normally and APMSensorsComponent* files are pristine upstream (not modified by us), so this is NOT our build. Most likely firmware/board state. NEXT: test with STOCK (official) QGroundControl from qgroundcontrol.com — if stock also fails, it's firmware (reflash correct board target); if stock works, investigate our build. NOTE: this QGC has a built-in `_checkForBadCubeBlack()` (APMAutoPilotPlugin.cc ~line 242) that warns about the Cube Black critical service bulletin SB-0000002 when INS_ACC3_ID==0 && INS_GYR3_ID==0 && INS_ENABLE_MASK>=7 — relevant since the Black Cube is now connected.

### Still open / next
- **Whole-window flicker** was the broken Orange Cube spamming errors — gone with the Black Cube. NOT a GPU/graphics issue. No action needed beyond fixing Cube firmware.
- **Cube reports no gyro/accel + BARO1 board-validation error** — firmware/board-target mismatch; needs a re-flash of the correct exact board target. Does NOT block editor work (use the real Cube only for final goggle testing).
- **Project goal:** MSP OSD on DJI goggles (OSD_TYPE MSP DisplayPort) — the editor edits the same OSDn_* element params used by MSP DisplayPort.

---

## What is verified to work

- ✅ Cube Orange runs ArduCopter V4.6.3
- ✅ QGC dev build (`QGroundControl Daily`) connects to Cube via USB COM9, params download fully
- ✅ `APMAutoPilotPlugin::vehicleComponents()` constructs and appends our `APMOSDComponent`
- ✅ `APMOSDComponent` reports its name, icon, setupSource, summaryQmlSource correctly
- ✅ `OSD` entry appears in Vehicle Configuration sidebar with the battery placeholder icon
- ✅ Clicking the entry navigates to our page (page is selected, "OSD" highlighted)
- ✅ A minimal `Rectangle { color: "red"; width: availableWidth; height: 600 }` inside our SetupPage's pageComponent renders perfectly with all the dimension labels
- ✅ Reported dimensions in the minimal page were `availableWidth=1391, availableHeight=863` — plenty of space
- ✅ JS tests still pass: 27/27 osd-params + 6/6 parity
- ✅ Distributable zip exists at `F:\APPS\QGroundControl-Daily-OSD.zip` (170 MB) — runs on other machines

## What is NOT verified to work

- ❌ The actual editor UI rendering inside `APMOSDComponent.qml`. The page area is blank even after both known fixes.

---

## Two real bugs found and fixed (don't reintroduce)

1. **`width: availableWidth` and `height: availableHeight` on the root ColumnLayout** were missing. Without them, the Loader inside QGC's `SetupPage` doesn't size the loaded page and the ColumnLayout collapses to zero. APMAirframeComponent.qml has the same pattern as the model.
2. **`import QGroundControl.ScreenTools` is NOT a valid module.** ScreenTools is a singleton auto-available via `import QGroundControl.Controls`. Including the bad import line caused Qt to silently fail to load the entire .qml file and the page rendered blank. A comment in the current `APMOSDComponent.qml` warns about this — leave it.

Both fixes are in place in `F:\APPS\qgroundcontrol\src\AutoPilotPlugins\APM\APMOSDComponent.qml` and synced back to `F:\APPS\qgc-osd-editor\qgc\qml\APMOSDComponent.qml`.

---

## The remaining bug (unsolved)

**Symptom:** With Cube Orange running ArduCopter V4.6.3 connected, clicking OSD in the Vehicle Configuration sidebar shows ONLY the yellow "Config Error: fix problem then reboot" banner at the top of the page area. The rest of the page area is blank. Other components (Power, Motors, etc.) render their content fine in the same QGC instance against the same vehicle.

**What we know:**
- The same `SetupPage` scaffold and ColumnLayout structure renders a minimal red rectangle perfectly (we tested this).
- No QML errors are emitted to stderr when the bad ScreenTools import is removed.
- The page entry IS visible in the sidebar (Task 05 wiring proven).
- The user reproduced the same blank-page behavior on a second computer with the same build — so it's not a Qt-cache or local-machine issue.

**The bug lives somewhere in the body of `APMOSDComponent.qml`** — between the toolbar `RowLayout`, the workspace `RowLayout` (palette/canvas/inspector), and the status bar `RowLayout`. Specifically the things that change between the minimal version and the full version.

---

## Debug ideas to try next (in priority order)

### 1. Bisect by content (highest signal, lowest effort)

Replace the body of the root ColumnLayout incrementally. Start with the simplest version that includes our controller:

```qml
ColumnLayout {
    width:  availableWidth
    height: availableHeight

    APMOSDComponentController { id: controller }

    QGCLabel { text: "controller exists: " + (controller !== null) }
    QGCLabel { text: "elementKeys count: " + controller.elementKeys.length }
    QGCLabel { text: "activeScreen: " + controller.activeScreen }
    QGCLabel { text: "maxX: " + controller.maxX + " maxY: " + controller.maxY }
}
```

If this renders, controller is fine and the bug is in one of the sub-blocks. Then add blocks back one at a time (toolbar first, then palette ListView, then canvas Rectangle with its grid Canvas, then inspector) and find the one that breaks rendering.

### 2. Check FactCheckBox / FactComboBox with null Fact

The toolbar has `FactComboBox { fact: controller.screenTxtResFact() }` and the inspector has `FactCheckBox { fact: controller.elEnabledFact(osdPage.selectedKey) }`. When `selectedKey === ""` (the default), `elEnabledFact("")` is called and may return nullptr. Some FactControls may throw a QML error when handed a null fact even though we set `visible:` to gate them.

Suggested fix: change `fact: controller.elEnabledFact(osdPage.selectedKey)` to `fact: osdPage.selectedKey !== "" ? controller.elEnabledFact(osdPage.selectedKey) : null` so we don't even *call* the lookup with empty string.

### 3. Watch for binding loops

The `recomputeOverlaps()` function reassigns `overlapKeys = s`. If a binding somewhere ends up calling `recomputeOverlaps()` and that change triggers another binding evaluation that calls it again, you get a loop. The `Connections` block fires it on every controller signal — if a controller signal fires during overlap recomputation, infinite loop. Qt usually prints "QML binding loop detected" to stderr; check for that after the next attempt.

### 4. Test the controller's `elementKeys` and `elementInfo` from QML

The palette `ListView { model: controller.elementKeys }` and `text: controller.elementInfo(modelData)["label"]` are the first place the page tries to actually USE controller data. Add a `Component.onCompleted: { console.log("elementKeys:", controller.elementKeys); console.log("info:", JSON.stringify(controller.elementInfo("ALTITUDE"))); }` to log values to stderr.

### 5. Try the page against an offline-editing APM vehicle

Open QGC with no Cube connected, set offline-editing firmware class to ArduPilot (or use the MockLink APM ArduCopter button at Settings → Mock Link). If the bug repeats against a synthetic vehicle, no parameters are involved — it's a pure-QML issue. If it doesn't, the bug is triggered by something specific to the real Cube's parameter set or the BARO1 error state.

---

## Files involved

### Staging (the source of truth)

- `F:\APPS\qgc-osd-editor\qgc\src\APMOSDComponent.h` / `.cc` — VehicleComponent subclass
- `F:\APPS\qgc-osd-editor\qgc\src\APMOSDComponentController.h` / `.cc` — controller with `importParamText`, `exportParamText`, `detectOverlaps`, `clampActiveScreen` + file I/O helpers + `overlapsChanged` signal
- `F:\APPS\qgc-osd-editor\qgc\qml\APMOSDComponent.qml` — editor page (the file with the open bug)
- `F:\APPS\qgc-osd-editor\qgc\qml\APMOSDComponentSummary.qml` — sidebar summary card
- `F:\APPS\qgc-osd-editor\qgc\tests\APMOSDControllerTest.cc/.h` — Qt Test cases, currently QSKIP-guarded

### Modifications inside qgroundcontrol/

- `src\AutoPilotPlugins\APM\CMakeLists.txt` — added 4 source files + 2 QML files
- `src\AutoPilotPlugins\APM\APMAutoPilotPlugin.h` — forward declare + `_osdComponent` member
- `src\AutoPilotPlugins\APM\APMAutoPilotPlugin.cc` — construct + setupTriggerSignals + append
- `src\Comms\CMakeLists.txt` — `if(TARGET Qt6::Bluetooth)` guard around Bluetooth subdir
- `src\AppSettings\pages\SettingsPages.json` — MockLink page made visible in Release (`"visible": "true"` instead of `"ScreenTools.isDebug"`)

### Distribution

- `F:\APPS\QGroundControl-Daily-OSD.zip` (170 MB compressed, 436 MB extracted) — portable QGC build with the broken-page version of the editor; usable on other Windows machines

---

## How to rebuild from scratch

```bash
# In F:\APPS\qgroundcontrol\build\
build.bat
# Output: Release\QGroundControl.exe (~45 MB)
```

Or for a full clean rebuild:

```bash
rm -rf F:\APPS\qgroundcontrol\build\Release\*
# then rerun configure.bat then build.bat
```

To run with stderr captured (essential for QML debugging):

```powershell
$env:QT_FORCE_STDERR_LOGGING = "1"
$env:PATH = "F:\QTt\6.10.3\msvc2022_64\bin;F:\APPS\qgroundcontrol\.cache\CPM\gstreamer-win-x86_64-1.28.1\sdk\bin;" + $env:PATH
Start-Process -FilePath 'F:\APPS\qgroundcontrol\build\Release\QGroundControl.exe' `
  -WorkingDirectory 'F:\APPS\qgroundcontrol\build\Release' `
  -RedirectStandardError 'F:\APPS\qgroundcontrol\build\qgc-stderr.log'
```

Then grep for OSD-related errors:
```powershell
Select-String -Path 'F:\APPS\qgroundcontrol\build\qgc-stderr.log' -Pattern 'OSD|APMOSD|module.*not installed|TypeError|binding loop'
```

---

## How to revert the QGC modifications cleanly

If you want to restore upstream QGC behavior:

```bash
cd F:\APPS\qgroundcontrol
git status                 # see what's modified
git diff src/AutoPilotPlugins/APM   # review our changes
git checkout -- src/AutoPilotPlugins/APM/CMakeLists.txt \
                 src/AutoPilotPlugins/APM/APMAutoPilotPlugin.{h,cc} \
                 src/Comms/CMakeLists.txt \
                 src/AppSettings/pages/SettingsPages.json
rm src/AutoPilotPlugins/APM/APMOSDComponent*.{h,cc,qml}
# then rebuild
```

The staging dir at `F:\APPS\qgc-osd-editor\qgc\` still has the canonical source, so nothing is lost.
