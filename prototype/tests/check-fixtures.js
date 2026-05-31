// One-off pre-flight: round-trip every file in fixtures/ through the data layer
// and report any drift. Not part of `npm test` — invoked manually during Task 01.

const fs = require('fs');
const path = require('path');
const {
  makeDefaultState,
  parseParamFile,
  mergeIntoState,
  serializeParamFile,
  clampToResolution,
  detectOverlaps,
  ELEMENTS,
} = require('../osd-params.js');

const FIXTURES_DIR = path.join(__dirname, '..', '..', 'fixtures');
const files = fs.readdirSync(FIXTURES_DIR).filter((f) => f.endsWith('.param'));

let problems = 0;

function note(file, msg) {
  console.log(`  ${file}: ${msg}`);
}

for (const file of files) {
  console.log(`\n\x1b[1m${file}\x1b[0m`);
  const text = fs.readFileSync(path.join(FIXTURES_DIR, file), 'utf8');

  const parsed = parseParamFile(text);
  const state = mergeIntoState(makeDefaultState(), parsed);

  const screensSeen = Object.keys(parsed.screens || {}).map(Number).sort();
  note(file, `screens referenced: ${screensSeen.join(', ') || '(none)'}`);

  for (const n of screensSeen) {
    const sc = state.screens[n];
    const enabledEls = ELEMENTS.filter((e) => sc.elements[e.key].en === 1);
    note(file, `  screen ${n}: enabled=${sc.enabled}, txtRes=${sc.txtRes}, elements=${enabledEls.length}`);
  }

  const out = serializeParamFile(state, { screens: screensSeen, includeComments: false });
  const reparsed = parseParamFile(out);
  const restored = mergeIntoState(makeDefaultState(), reparsed);

  for (const n of screensSeen) {
    for (const el of ELEMENTS) {
      const a = state.screens[n].elements[el.key];
      const b = restored.screens[n].elements[el.key];
      if (a.en !== b.en || a.x !== b.x || a.y !== b.y) {
        note(file, `  DRIFT screen ${n}/${el.key}: ${JSON.stringify(a)} -> ${JSON.stringify(b)}`);
        problems++;
      }
    }
    if (state.screens[n].enabled !== restored.screens[n].enabled) {
      note(file, `  DRIFT screen ${n}.enabled`);
      problems++;
    }
    if (state.screens[n].txtRes !== restored.screens[n].txtRes) {
      note(file, `  DRIFT screen ${n}.txtRes`);
      problems++;
    }
  }

  // Special: sd-resolution.param is meant to exercise clamping. Show what clamp does.
  if (file === 'sd-resolution.param') {
    const before = JSON.parse(JSON.stringify(state.screens[1].elements));
    clampToResolution(state, 1);
    for (const el of ELEMENTS) {
      const a = before[el.key];
      const b = state.screens[1].elements[el.key];
      if (a.x !== b.x || a.y !== b.y) {
        note(file, `  clamp(SD) ${el.key}: (${a.x},${a.y}) -> (${b.x},${b.y})`);
      }
    }
  }

  // Overlap report for screen 1.
  if (screensSeen.includes(1)) {
    const ov = detectOverlaps(state, 1);
    if (ov.length) {
      note(file, `  overlaps on screen 1:`);
      for (const o of ov) note(file, `    ${o.a} × ${o.b}`);
    } else {
      note(file, `  no overlaps on screen 1`);
    }
  }
}

console.log('');
if (problems === 0) {
  console.log('\x1b[32mAll fixtures round-trip cleanly.\x1b[0m');
} else {
  console.log(`\x1b[31m${problems} drift(s) detected.\x1b[0m`);
  process.exit(1);
}
