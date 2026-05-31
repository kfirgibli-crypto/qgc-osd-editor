# Task 01 — Validate Prototype UX

**Estimated:** 30–60 min (mostly Liat in a browser, not Claude Code)
**Type:** Manual / decision-making
**Preconditions:** None — this is step one.

## Goal

Confirm the prototype editor's UX matches what you want the QGC page to feel like, **before** investing time in the QML port. Every UX bug found now is one not found after writing 800 lines of QML.

## Steps

1. From the repo root:
   ```bash
   npm run serve
   # or just open prototype/index.html directly in a browser
   ```
2. Walk the **Tier 2 checklist** in `docs/test-plan.md` end-to-end. Tick each box. Note anything that feels wrong.
3. Try these specifically with a critical eye:
   - Drag a long element (HORIZON, width 13) across the canvas. Does the snap feel right?
   - Switch from HD 60×22 to SD 30×16 with elements outside SD bounds. Are they clamped sensibly?
   - Import `fixtures/mission-planner-export.param`. Does the layout match what MP would show?
   - Export and re-import. Identical? (It should be — the JS tests prove this, but verify visually.)
4. Decide: which of the 25 MVP elements actually ship in v1 of the QGC page? Fewer is fine; the goal is to get the loop working end-to-end before broadening.

## Acceptance

Open issues (or just notes in this file) for anything that needs to change in the prototype. Resolve those before moving to Task 02.

## What Claude Code can do here

Not much directly — this is your validation pass. But if you find UX bugs, hand them to a follow-up Claude Code session like:

> "In prototype/index.html, the keyboard shortcut for `Delete` should disable
>  the element but currently it also deselects. Fix and re-run the parity test."

The prototype is intentionally small and self-contained so changes here are fast.

## Findings (fill in as you go)

- [ ] Drag feel:
- [x] Resolution switch: HD→SD per-element clamping piles elements at `(29,15)` and introduces overlaps (ALTITUDE×FLTMODE, RSSI×SATS, CURRENT×MESSAGE, etc.). **Decision: keep as-is for v1** — red overlap rendering surfaces the problem to the user. C++ port and QML must mirror this exactly.
- [ ] Import/export round trip: (data-layer round-trip verified programmatically against all 3 fixtures — passes; visual confirmation TBD)
- [ ] Final MVP element list:
- [ ] Other:
