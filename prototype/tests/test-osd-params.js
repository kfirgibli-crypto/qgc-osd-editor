// test-osd-params.js
// Run with: node test-osd-params.js

const {
  ELEMENTS,
  RESOLUTIONS,
  makeDefaultState,
  parseParamFile,
  mergeIntoState,
  serializeParamFile,
  clampToResolution,
  detectOverlaps,
} = require('../osd-params.js');

let passed = 0;
let failed = 0;
const failures = [];

function test(name, fn) {
  try {
    fn();
    passed++;
    console.log(`  \x1b[32m✓\x1b[0m ${name}`);
  } catch (err) {
    failed++;
    failures.push({ name, err });
    console.log(`  \x1b[31m✗\x1b[0m ${name}`);
    console.log(`      ${err.message}`);
  }
}

function assert(cond, msg) {
  if (!cond) throw new Error(msg || 'assertion failed');
}

function assertEq(a, b, msg) {
  if (a !== b) {
    throw new Error(`${msg || 'assertEq'}: expected ${JSON.stringify(b)}, got ${JSON.stringify(a)}`);
  }
}

function assertDeepEq(a, b, msg) {
  if (JSON.stringify(a) !== JSON.stringify(b)) {
    throw new Error(`${msg || 'assertDeepEq'}:\n  expected ${JSON.stringify(b)}\n       got ${JSON.stringify(a)}`);
  }
}

function group(name, fn) {
  console.log(`\n\x1b[1m${name}\x1b[0m`);
  fn();
}

// ----------------------------------------------------------------------------

group('Catalogue integrity', () => {
  test('ELEMENTS is non-empty and has unique keys', () => {
    assert(ELEMENTS.length > 0);
    const seen = new Set();
    for (const e of ELEMENTS) {
      assert(typeof e.key === 'string' && e.key.length > 0, `bad key for ${JSON.stringify(e)}`);
      assert(!seen.has(e.key), `duplicate key ${e.key}`);
      seen.add(e.key);
      assert(typeof e.label === 'string', 'label missing');
      assert(typeof e.width === 'number' && e.width > 0, 'width missing/invalid');
      assert(typeof e.sample === 'string', 'sample missing');
    }
  });

  test('Resolutions cover SD and both HD modes', () => {
    assert(RESOLUTIONS[0].cols === 30 && RESOLUTIONS[0].rows === 16, 'SD wrong');
    assert(RESOLUTIONS[1].cols === 50 && RESOLUTIONS[1].rows === 18, 'HD 50x18 wrong');
    assert(RESOLUTIONS[2].cols === 60 && RESOLUTIONS[2].rows === 22, 'HD 60x22 wrong');
  });
});

group('Default state', () => {
  test('makeDefaultState produces 4 screens with all elements', () => {
    const s = makeDefaultState();
    assertEq(Object.keys(s.screens).length, 4);
    for (let n = 1; n <= 4; n++) {
      const screen = s.screens[n];
      assert(screen, `screen ${n} missing`);
      for (const el of ELEMENTS) {
        assert(screen.elements[el.key], `screen ${n} missing element ${el.key}`);
        const e = screen.elements[el.key];
        assert(typeof e.en === 'number');
        assert(typeof e.x === 'number');
        assert(typeof e.y === 'number');
      }
    }
  });

  test('Only screen 1 is enabled by default', () => {
    const s = makeDefaultState();
    assertEq(s.screens[1].enabled, 1);
    assertEq(s.screens[2].enabled, 0);
    assertEq(s.screens[3].enabled, 0);
    assertEq(s.screens[4].enabled, 0);
  });

  test('Default txtRes is HD 60x22 (mode 2)', () => {
    const s = makeDefaultState();
    assertEq(s.screens[1].txtRes, 2);
  });
});

group('parseParamFile', () => {
  test('Parses standard Mission Planner comma format', () => {
    const text = [
      '# header',
      'OSD1_ENABLE,1',
      'OSD1_TXT_RES,2',
      'OSD1_ALTITUDE_EN,1',
      'OSD1_ALTITUDE_X,53',
      'OSD1_ALTITUDE_Y,5',
    ].join('\n');
    const r = parseParamFile(text);
    assertEq(r.screens[1].enabled, 1);
    assertEq(r.screens[1].txtRes, 2);
    assertEq(r.screens[1].elements.ALTITUDE.en, 1);
    assertEq(r.screens[1].elements.ALTITUDE.x, 53);
    assertEq(r.screens[1].elements.ALTITUDE.y, 5);
  });

  test('Parses whitespace-separated format', () => {
    const text = 'OSD1_BAT_VOLT_EN 1\nOSD1_BAT_VOLT_X 1\nOSD1_BAT_VOLT_Y 20';
    const r = parseParamFile(text);
    assertEq(r.screens[1].elements.BAT_VOLT.en, 1);
    assertEq(r.screens[1].elements.BAT_VOLT.x, 1);
    assertEq(r.screens[1].elements.BAT_VOLT.y, 20);
  });

  test('Strips inline comments', () => {
    const text = 'OSD1_RSSI_X,42  # comment here\nOSD1_RSSI_Y,3';
    const r = parseParamFile(text);
    assertEq(r.screens[1].elements.RSSI.x, 42);
    assertEq(r.screens[1].elements.RSSI.y, 3);
  });

  test('Ignores unknown elements without error', () => {
    const text = 'OSD1_NOT_REAL_EN,1\nOSD1_ALTITUDE_EN,1';
    const r = parseParamFile(text);
    assert(!r.screens[1] || !r.screens[1].elements.NOT_REAL, 'should ignore unknown');
    assertEq(r.screens[1].elements.ALTITUDE.en, 1);
  });

  test('Distinguishes per-screen params', () => {
    const text = 'OSD1_ALTITUDE_X,10\nOSD2_ALTITUDE_X,20\nOSD3_ALTITUDE_X,30';
    const r = parseParamFile(text);
    assertEq(r.screens[1].elements.ALTITUDE.x, 10);
    assertEq(r.screens[2].elements.ALTITUDE.x, 20);
    assertEq(r.screens[3].elements.ALTITUDE.x, 30);
  });

  test('Handles empty input', () => {
    const r = parseParamFile('');
    assertDeepEq(r, { screens: {} });
  });

  test('Handles CRLF line endings', () => {
    const text = 'OSD1_ALTITUDE_X,7\r\nOSD1_ALTITUDE_Y,8\r\n';
    const r = parseParamFile(text);
    assertEq(r.screens[1].elements.ALTITUDE.x, 7);
    assertEq(r.screens[1].elements.ALTITUDE.y, 8);
  });

  test('Handles multi-underscore element keys like BAT_VOLT', () => {
    const text = 'OSD1_BAT_VOLT_X,5\nOSD1_BAT2_VLT_X,12';
    const r = parseParamFile(text);
    assertEq(r.screens[1].elements.BAT_VOLT.x, 5);
    // BAT2_VLT not in our MVP catalogue — should be silently ignored, not crash
    assert(!r.screens[1].elements.BAT2_VLT);
  });

  test('Ignores non-OSD params', () => {
    const text = 'BATT_CAPACITY,5000\nOSD1_ALTITUDE_X,3';
    const r = parseParamFile(text);
    assertEq(r.screens[1].elements.ALTITUDE.x, 3);
  });
});

group('mergeIntoState', () => {
  test('Preserves base state for unmodified screens', () => {
    const base = makeDefaultState();
    const partial = parseParamFile('OSD1_ALTITUDE_X,42');
    const merged = mergeIntoState(base, partial);
    assertEq(merged.screens[1].elements.ALTITUDE.x, 42);
    // Other screens untouched
    assertEq(merged.screens[2].enabled, base.screens[2].enabled);
    // Other elements on screen 1 untouched
    assertEq(merged.screens[1].elements.BAT_VOLT.x, base.screens[1].elements.BAT_VOLT.x);
  });

  test('Mutating result does not affect base', () => {
    const base = makeDefaultState();
    const merged = mergeIntoState(base, parseParamFile('OSD1_ALTITUDE_X,99'));
    merged.screens[1].elements.ALTITUDE.x = 0;
    assert(base.screens[1].elements.ALTITUDE.x !== 0, 'base should be unchanged');
  });
});

group('serializeParamFile', () => {
  test('Round trips: parse(serialize(state)) ≈ state', () => {
    const original = makeDefaultState();
    const text = serializeParamFile(original);
    const parsed = parseParamFile(text);
    const restored = mergeIntoState(makeDefaultState(), parsed);
    assertDeepEq(restored, original);
  });

  test('Custom layout round-trips correctly', () => {
    let state = makeDefaultState();
    state.screens[1].elements.ALTITUDE = { en: 1, x: 42, y: 7 };
    state.screens[2].enabled = 1;
    state.screens[2].elements.BAT_VOLT = { en: 1, x: 10, y: 15 };
    state.screens[2].elements.RSSI = { en: 0, x: 0, y: 0 };

    const text = serializeParamFile(state);
    const parsed = parseParamFile(text);
    const restored = mergeIntoState(makeDefaultState(), parsed);

    assertEq(restored.screens[1].elements.ALTITUDE.x, 42);
    assertEq(restored.screens[1].elements.ALTITUDE.y, 7);
    assertEq(restored.screens[2].enabled, 1);
    assertEq(restored.screens[2].elements.BAT_VOLT.x, 10);
    assertEq(restored.screens[2].elements.RSSI.en, 0);
  });

  test('includeComments=false produces no comment lines', () => {
    const state = makeDefaultState();
    const text = serializeParamFile(state, { includeComments: false });
    const commentLines = text.split('\n').filter((l) => l.startsWith('#'));
    assertEq(commentLines.length, 0);
  });

  test('screens option limits output', () => {
    const state = makeDefaultState();
    const text = serializeParamFile(state, { screens: [1] });
    assert(text.includes('OSD1_ENABLE'), 'screen 1 should be present');
    assert(!text.includes('OSD2_ENABLE'), 'screen 2 should be absent');
  });
});

group('clampToResolution', () => {
  test('Clamps values exceeding SD bounds', () => {
    let s = makeDefaultState();
    s.screens[1].txtRes = 0; // SD 30x16
    s.screens[1].elements.ALTITUDE = { en: 1, x: 55, y: 20 };
    clampToResolution(s, 1);
    assertEq(s.screens[1].elements.ALTITUDE.x, 29);
    assertEq(s.screens[1].elements.ALTITUDE.y, 15);
  });

  test('Leaves valid HD positions alone', () => {
    let s = makeDefaultState();
    s.screens[1].txtRes = 2;
    s.screens[1].elements.ALTITUDE = { en: 1, x: 30, y: 10 };
    clampToResolution(s, 1);
    assertEq(s.screens[1].elements.ALTITUDE.x, 30);
    assertEq(s.screens[1].elements.ALTITUDE.y, 10);
  });

  test('Clamps negatives', () => {
    let s = makeDefaultState();
    s.screens[1].elements.ALTITUDE = { en: 1, x: -5, y: -3 };
    clampToResolution(s, 1);
    assertEq(s.screens[1].elements.ALTITUDE.x, 0);
    assertEq(s.screens[1].elements.ALTITUDE.y, 0);
  });
});

group('detectOverlaps', () => {
  test('Reports overlap for same-row adjacent elements', () => {
    const s = makeDefaultState();
    // BAT_VOLT width 6, place ALTITUDE inside it on same row
    s.screens[1].elements.BAT_VOLT = { en: 1, x: 1, y: 10 };
    s.screens[1].elements.ALTITUDE = { en: 1, x: 3, y: 10 };
    const overlaps = detectOverlaps(s, 1);
    const pair = overlaps.find(
      (o) =>
        (o.a === 'BAT_VOLT' && o.b === 'ALTITUDE') ||
        (o.a === 'ALTITUDE' && o.b === 'BAT_VOLT'),
    );
    assert(pair, 'BAT_VOLT and ALTITUDE overlap not detected');
  });

  test('No overlap when on different rows', () => {
    const s = makeDefaultState();
    s.screens[1].elements.BAT_VOLT = { en: 1, x: 1, y: 10 };
    s.screens[1].elements.ALTITUDE = { en: 1, x: 1, y: 11 };
    const overlaps = detectOverlaps(s, 1);
    const pair = overlaps.find(
      (o) =>
        (o.a === 'BAT_VOLT' && o.b === 'ALTITUDE') ||
        (o.a === 'ALTITUDE' && o.b === 'BAT_VOLT'),
    );
    assert(!pair, 'should not detect cross-row overlap');
  });

  test('Disabled elements never overlap', () => {
    const s = makeDefaultState();
    s.screens[1].elements.BAT_VOLT = { en: 0, x: 1, y: 10 };
    s.screens[1].elements.ALTITUDE = { en: 1, x: 1, y: 10 };
    const overlaps = detectOverlaps(s, 1);
    const pair = overlaps.find(
      (o) =>
        (o.a === 'BAT_VOLT' && o.b === 'ALTITUDE') ||
        (o.a === 'ALTITUDE' && o.b === 'BAT_VOLT'),
    );
    assert(!pair, 'disabled element should not contribute to overlap');
  });
});

group('Real-world fixture: load + round-trip Mission Planner export', () => {
  // Realistic export sample fabricated to match the format MP produces.
  const fixture = `
# Vehicle parameters
OSD_TYPE,5
OSD_TYPE2,0
OSD_CHAN,0
OSD_UNITS,0
OSD1_ENABLE,1
OSD1_TXT_RES,2
OSD1_CHAN_MIN,900
OSD1_CHAN_MAX,2100
OSD1_ALTITUDE_EN,1
OSD1_ALTITUDE_X,53
OSD1_ALTITUDE_Y,5
OSD1_BAT_VOLT_EN,1
OSD1_BAT_VOLT_X,1
OSD1_BAT_VOLT_Y,20
OSD1_RSSI_EN,1
OSD1_RSSI_X,53
OSD1_RSSI_Y,0
OSD1_FLTMODE_EN,1
OSD1_FLTMODE_X,26
OSD1_FLTMODE_Y,20
OSD1_HORIZON_EN,1
OSD1_HORIZON_X,23
OSD1_HORIZON_Y,9
BATT_CAPACITY,5000
`;
  test('Parses MP-style export and round-trips losslessly', () => {
    const partial = parseParamFile(fixture);
    const state = mergeIntoState(makeDefaultState(), partial);
    assertEq(state.screens[1].elements.ALTITUDE.x, 53);
    assertEq(state.screens[1].elements.BAT_VOLT.y, 20);
    assertEq(state.screens[1].elements.HORIZON.x, 23);

    const out = serializeParamFile(state, { includeComments: false, screens: [1] });
    const reparsed = parseParamFile(out);
    const restored = mergeIntoState(makeDefaultState(), reparsed);

    assertEq(restored.screens[1].elements.ALTITUDE.x, 53);
    assertEq(restored.screens[1].elements.BAT_VOLT.y, 20);
    assertEq(restored.screens[1].elements.FLTMODE.x, 26);
    assertEq(restored.screens[1].elements.HORIZON.x, 23);
  });
});

// ----------------------------------------------------------------------------

console.log('');
console.log(`\x1b[1m${passed + failed} tests, ${passed} passed, ${failed} failed\x1b[0m`);
if (failed > 0) {
  console.log('');
  console.log('Failures:');
  for (const f of failures) {
    console.log(`  • ${f.name}: ${f.err.message}`);
  }
  process.exit(1);
}
