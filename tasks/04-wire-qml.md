# Task 04 — Wire QML Drag/Drop & File Dialogs

**Estimated:** 1–2 sessions (5–8 hours total)
**Type:** QML implementation
**Preconditions:** Task 03 complete (controller methods exist).

## Goal

The QML editor page works end-to-end against an offline-editing vehicle.
You can drag elements, see Facts update, edit X/Y in the inspector, import
and export `.param` files.

## What needs to happen

In `qgc/qml/APMOSDComponent.qml`:

1. **Snap-to-cell on drag.** Currently uses raw `drag.target`. Port the
   prototype's `onReleased` snap logic — round to nearest cell on release
   AND continuously while dragging (the prototype updates X/Y as you drag,
   not just at drop, so the inspector readout stays live).

2. **Grid lines visible.** The canvas is currently solid black. Add subtle
   grid lines using `Canvas` or a `Repeater` of 1px `Rectangle`s. Match the
   prototype's `--grid-line` opacity (very subtle).

3. **File dialogs actually do something.** The `importDialog.onAccepted`
   and `exportDialog.onAccepted` handlers are stubs. Implement them:
   ```qml
   onAccepted: {
       var file = ... // read or write controller.importParamText / exportParamText
   }
   ```
   Use `QtCore.StandardPaths` for default locations.

4. **Overlap detection wired to UI.** `controller.detectOverlaps()` returns
   the pairs; the canvas should render overlapping elements in red. Match
   the prototype's `.overlap` class behavior.

5. **Keyboard navigation.** Arrow keys nudge the selected element by 1
   cell (Shift+arrow = 5). Hook to `Keys.onPressed` on the canvas. Match
   the prototype's behavior exactly.

## Prompt to give Claude Code

> Read `CLAUDE.md`, then `tasks/04-wire-qml.md`.
> 
> Wire up `qgc/qml/APMOSDComponent.qml` to feature-parity with
> `prototype/index.html`. The five sub-tasks are listed in the task file.
> 
> For each one: implement, then walk through the matching item in the
> Tier 2 checklist in `docs/test-plan.md`. If the QGC behavior doesn't
> match the prototype, the prototype is right — figure out why QGC
> differs.
> 
> Don't add styling beyond what QGC's palette/ScreenTools provides.
> The prototype's phosphor-green CRT aesthetic doesn't need to carry
> over — match QGC's existing visual language instead.

## Acceptance

- Drag element on canvas → Fact updates live → inspector shows new X/Y
- File import: pick a `.param` file → layout updates
- File export: writes a `.param` file matching what the prototype would
  emit for the same state (byte-for-byte, modulo timestamps)
- Overlapping elements visible in red
- Arrow keys nudge, Shift+arrow nudges by 5
- Resolution switch clamps positions correctly
- `CLAUDE.md` "Current state" updated

## Reference

- `prototype/index.html` (interaction model — copy the semantics)
- `qgroundcontrol/src/AutoPilotPlugins/APM/qml/APMPowerComponent.qml`
  (style + structural conventions)
- `qgroundcontrol/src/QmlControls/FactCheckBox.qml`,
  `FactComboBox.qml`, `FactTextField.qml` (binding patterns)
