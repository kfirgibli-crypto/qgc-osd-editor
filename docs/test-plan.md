# Test Plan — End-to-End Validation

Tests in the project split into three tiers by what they need to run:

| Tier | Environment needed              | What it validates                                |
| ---- | ------------------------------- | ------------------------------------------------ |
| 1    | Node.js only                    | Parameter file parsing/serialization correctness |
| 2    | Browser (any)                   | Visual editor UX, drag/drop, import/export       |
| 3    | QGC build + ArduPilot SITL      | Live MAVLink parameter read/write integration    |
| 4    | Real FC + DJI O3 + goggles      | OSD actually renders correctly                   |

Tiers 1 and 2 I ran and verified here. Tiers 3 and 4 need your local setup.

---

## Tier 1 — Node tests (already run, 27/27 passing)

```
cd osd-editor
node test-osd-params.js
node test-inline-parity.js
```

Expected output: `27 tests, 27 passed, 0 failed` and `6 parity checks, 6 passed`.

If you modify `osd-params.js`, re-run these. The C++ port in
`APMOSDComponentController.cc` should pass the same logical assertions —
each JS test maps to a Qt Test case.

---

## Tier 2 — Browser smoke tests (do this in any browser, ~10 minutes)

Open `prototype.html` in Chrome/Firefox/Safari.

### Visual

- [ ] Page loads, header shows "OSD · Layout Editor"
- [ ] Default screen 1 is shown with ~15 elements visible on the canvas
- [ ] Phosphor-green elements glow slightly against the black canvas
- [ ] Scanline overlay is subtly visible on the canvas

### Element palette

- [ ] All 25 elements listed on the left, mix of enabled/disabled (green vs grey dots)
- [ ] Counter at top right of palette shows enabled / total (e.g. `15 / 25`)
- [ ] Click on a disabled element → it gets selected (highlighted) but stays disabled
- [ ] Click on the same already-selected element again → it toggles enabled
- [ ] When enabled, the element appears on the canvas at its default position

### Canvas interaction

- [ ] Click an element on canvas → it gets selected (dashed outline, fills inspector)
- [ ] Drag an element → it snaps to grid cells, follows cursor
- [ ] Drop the element → its position is reflected in the inspector X/Y inputs
- [ ] Two elements on the same row overlapping each other → both render in red
- [ ] Click empty canvas area → deselects

### Keyboard

- [ ] With an element selected, arrow keys move it 1 cell
- [ ] Shift+arrow moves it 5 cells
- [ ] Space toggles its enabled state
- [ ] Delete/Backspace disables the selected element
- [ ] Keyboard shortcuts do NOT fire when typing in the inspector inputs

### Inspector

- [ ] Toggle screen "Enabled" switch → status pill changes between green/amber
- [ ] Edit X or Y input → element moves on canvas immediately
- [ ] Values outside [0, maxX-1] or [0, maxY-1] are clamped on input

### Resolution switching

- [ ] Change resolution from HD 60×22 to SD 30×16
- [ ] Canvas shrinks proportionally
- [ ] Elements positioned outside the new bounds get clamped to the edge
- [ ] Status pill at bottom shows the new resolution label

### Screen switching

- [ ] Switch to Screen 2 → palette shows everything disabled
- [ ] Status pill at bottom shows "Screen 2 (disabled)" in amber
- [ ] Enable screen 2 in the inspector → pill goes green
- [ ] Switch back to Screen 1 → original layout still there

### Import / export

- [ ] Click "Export .param" → file `osd-layout.param` downloads
- [ ] Open the file in a text editor — looks like:
      ```
      # ArduPilot OSD layout — exported by osd-editor
      # Generated: 2026-...
      # --- Screen 1 ---
      OSD1_ENABLE,1
      OSD1_TXT_RES,2
      ...
      ```
- [ ] Click "Import .param", choose the file you just exported → layout unchanged
- [ ] Modify the file (e.g. set `OSD1_ALTITUDE_X,5`), import → element moves to X=5
- [ ] Use the inspector's textarea: paste a partial file, click "Import pasted" → applies

### Persistence

- [ ] Reload the page → your layout is restored from localStorage
- [ ] Click "Reset" and confirm → defaults restored, reload again → defaults persist

---

## Tier 3 — QGC + ArduPilot SITL integration

This is where Tier 1's logic gets connected to real MAVLink. You need:

- QGroundControl built from source (`master` branch)
- ArduPilot SITL running (`./Tools/autotest/sim_vehicle.py -v ArduCopter`)
- The new `APMOSDComponent` files added per `qgc-integration-notes.md`

### Setup

```bash
# Terminal A — SITL
cd ardupilot
./Tools/autotest/sim_vehicle.py -v ArduCopter --console --map

# Terminal B — QGC with new component
cd qgroundcontrol
./qgroundcontrol-start.sh
# Connect to UDP 14550
```

### Test cases

1. **Component appears**
   - Open Vehicle Setup. The OSD component is in the left sidebar, between
     Power and Camera (or wherever you wired it).

2. **Reads existing params**
   - In SITL terminal: `param show OSD1_BAT_VOLT_X` → records the value
   - Open OSD page in QGC → BAT_VOLT element rendered at that X position

3. **Writes round-trip**
   - Drag BAT_VOLT element to a new X
   - In SITL: `param show OSD1_BAT_VOLT_X` → should show new value
   - Save params, restart SITL, re-fetch in QGC → value persists

4. **Multi-screen**
   - Switch QGC to Screen 2
   - In SITL: `param show OSD2_*` → matches what QGC displays

5. **Resolution change**
   - Change Screen 1 from HD to SD in QGC
   - In SITL: `param show OSD1_TXT_RES` → 0
   - Elements with X > 29 should be visually clamped in QGC

6. **Import .param file**
   - Export a layout from the web prototype
   - Import via QGC's OSD page
   - In SITL: `param show OSD1_ALTITUDE_X` → matches prototype's value

7. **Offline editing**
   - Disconnect QGC from SITL
   - Open OSD page → should still work against the offline param cache
   - Reconnect → changes sync up

8. **Conflict / missing params**
   - Build SITL without `HAL_PLUSCODE_ENABLE` (or pick any conditional element)
   - The corresponding palette entry should be hidden or disabled, not crash

### Qt Test suite

Port each test from `test-osd-params.js` into Qt Test cases under
`src/qgcunittest/APMOSDControllerTest.cc`. Pattern:

```cpp
void APMOSDControllerTest::testImportRoundTrip() {
    auto* ctrl = new APMOSDComponentController();
    QString original = ctrl->exportParamText();
    ctrl->importParamText(original);
    QString again = ctrl->exportParamText();
    QCOMPARE(original, again);
}
```

Run via:
```
./qgroundcontrol-start.sh --unittest:APMOSDControllerTest
```

---

## Tier 4 — Hardware loop

You need:
- An ArduPilot-supported flight controller (Pixhawk, MicoAir H743, etc.)
- DJI O3 Air Unit
- DJI Goggles 2 or Integra
- USB connection from FC to your QGC machine

### Setup

1. Flash ArduPilot to the FC (Copter or Plane).
2. Connect O3 air unit to a UART, set
   `SERIALn_PROTOCOL = 42` (MSP DisplayPort).
3. Set `OSD_TYPE = 5` (MSP DisplayPort).
4. Connect FC to QGC over USB.
5. Power up the goggles, enable Custom OSD in goggle settings.

### Test cases

1. **Round-trip on hardware**
   - Move BAT_VOLT in QGC → verify it moves on the goggle screen
   - Latency should be < 1 second after drag-end (params write
     completes within one MSP frame)

2. **Screen switching via RC**
   - Set `OSD_CHAN = 7` (or your spare channel)
   - Configure `OSD2_CHAN_MIN/MAX` to a different PWM band
   - Toggle the switch on your TX → goggle should switch to screen 2

3. **Resolution mismatch handling**
   - Set OSD1 to SD (30×16)
   - DJI O3 is HD — verify firmware doesn't crash and that elements
     just render at SD positions on an HD canvas

4. **Battery alarm**
   - Set `OSD_W_BLINK` and connect a low battery
   - Voltage element should blink

### Common gotchas

- If goggles show no OSD at all, check `OSD_TYPE` (must be 5 for DP) and
  `SERIALn_PROTOCOL` (must be 42) and the UART wiring (TX→RX, RX→TX).
- If only some elements show up, you may have build-flag-conditional
  elements that aren't compiled into your firmware. Check `param show OSD1_*`.
- DJI O3 Custom OSD must be **enabled** in goggle menu
  (Settings → Display → Custom OSD).

---

## What I cannot test for you

- The QML rendering of `APMOSDComponent.qml` — needs a built QGC
- The MAVLink round-trip behavior — needs a Vehicle instance
- The actual goggle output — needs hardware
- Performance under load (e.g. dragging an element with 50 params in flight)
- Qt Test integration with the existing QGC test harness

When you run Tier 3 / 4 tests, capture failures with:
- The exact param name + value that disagreed
- A screenshot of QGC's state
- The `param show` output from SITL/MAVProxy

If those gather around a pattern (e.g. "every element with `_` in its key
fails to bind"), that's a signal pointing at a specific function in the
controller. Most likely culprits, ranked: `paramName()` string assembly,
the `lookupFact()` component ID, or the `Q_INVOKABLE` Fact return type
(it must be `Fact*`, not `Fact`).
