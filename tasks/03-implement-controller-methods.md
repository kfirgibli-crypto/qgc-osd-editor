# Task 03 — Implement importParamText / exportParamText / detectOverlaps

**Estimated:** 1 focused session (3–5 hours)
**Type:** C++ port from JS
**Preconditions:** Task 02 complete (you can build QGC).

## Goal

Three stub methods in `qgc/src/APMOSDComponentController.cc` become real
implementations. Their behavior is fully specified by the existing JS
implementations in `prototype/osd-params.js` and verified by the JS tests.

## Prompt to give Claude Code

> Read `CLAUDE.md`, then `tasks/03-implement-controller-methods.md`.
> 
> Implement three methods in `qgc/src/APMOSDComponentController.cc`:
> 
> 1. `importParamText` — see `parseParamFile` in `prototype/osd-params.js`
> 2. `exportParamText` — see `serializeParamFile` in `prototype/osd-params.js`
> 3. `detectOverlaps` — see `detectOverlaps` in `prototype/osd-params.js`
> 
> **Test-driven**: For each method, first replace the matching `QSKIP` in
> `qgc/tests/APMOSDControllerTest.cc` with the real assertion (port from the
> corresponding JS test), confirm the test fails for the right reason, then
> write the implementation until it passes.
> 
> Use Qt-idiomatic types: `QString::split` with `QRegularExpression`,
> `Fact::setRawValue` for writes, `QVariantList`/`QVariantMap` for return
> values that QML consumes.
> 
> Keep going until all three are done and their tests pass. If you discover
> behavior differences with the JS, update the JS as well — they must stay
> in lock-step.

## Acceptance

- Three previously-TODO methods are implemented
- Corresponding Qt Test cases now run (not QSKIP'd) and pass
- The JS tests still pass (verify with `npm test`)
- `CLAUDE.md` "Current state" table updated
- No new compile warnings

## Reference

- Algorithm specs as comments inline in the .cc file
- `prototype/osd-params.js` (source of truth)
- `docs/qgc-integration-notes.md` "Mapping the test suite to Qt Test" table

## What not to do

- Don't optimize prematurely. The naive O(n²) overlap loop is fine; we have
  25 elements, not 25k.
- Don't change the parsing regex without also updating the JS. The whole
  point of dual-implementation is identical semantics; divergence here
  defeats the purpose.
- Don't add new features. This task ports existing behavior. New behavior
  goes through the prototype first.
