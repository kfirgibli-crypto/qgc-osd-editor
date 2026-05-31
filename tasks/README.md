# Tasks

Ordered, sized-for-one-session tasks. Each file has preconditions, work,
acceptance criteria, and a ready-to-paste prompt for Claude Code.

## Order

1. **[01 — Validate Prototype UX](./01-validate-prototype-ux.md)**
   Open the prototype, walk the checklist, fix what's wrong before any QGC work.
   *30–60 min. Mostly you in a browser.*

2. **[02 — Get QGC Building Locally](./02-setup-qgc-build.md)**
   Clone QGC, install Qt 6, build, run.
   *2–4 evenings. Environment setup.*

3. **[03 — Implement Controller Methods](./03-implement-controller-methods.md)**
   Port `importParamText`, `exportParamText`, `detectOverlaps` from JS to C++.
   *1 focused session. Test-driven.*

4. **[04 — Wire QML](./04-wire-qml.md)**
   Drag/drop, file dialogs, grid lines, keyboard nav, overlap rendering.
   *1–2 sessions.*

5. **[05 — Wire Into Plugin](./05-wire-into-plugin.md)**
   Register the component with `APMAutoPilotPlugin` so it appears in SetupView.
   *1 session.*

6. **[06 — SITL Validation](./06-sitl-validation.md)**
   Tier 3 tests against ArduCopter SITL.
   *1–2 sessions.*

7. **[07 — Hardware & PR](./07-hardware-and-pr.md)**
   Real FC + DJI O3, then PR to upstream QGC.
   *Bench/field test + multi-week PR review.*

## Don't skip ahead

The order matters. Each task assumes the previous ones are done. Skipping
ahead — e.g. starting on QML before the controller methods exist — wastes
time on iteration cycles that the predecessor task would have caught.

## How to start a session

```
Read CLAUDE.md and tasks/0N-<name>.md, then start work.
```

That's it. The task file tells Claude Code what to do, why, and when it's
done. Don't pre-load context the file already provides.
