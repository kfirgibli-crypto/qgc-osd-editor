// Smoke test: extract the inlined logic from prototype.html, execute it,
// and compare its outputs against the tested module's outputs.
const fs = require('fs');
const path = require('path');
const { parseParamFile: ref_parse,
        serializeParamFile: ref_serialize,
        makeDefaultState: ref_makeDefault,
        mergeIntoState: ref_merge,
        detectOverlaps: ref_overlaps,
      } = require('../osd-params.js');

const html = fs.readFileSync(path.join(__dirname, '..', 'index.html'), 'utf8');
const scripts = [...html.matchAll(/<script>([\s\S]*?)<\/script>/g)];
const logicBlock = scripts[0][1].replace(/\/\/<!\[CDATA\[/g, '').replace(/\/\/\]\]>/g, '');

// Execute in an isolated context that exposes a sandbox we can read back from.
const sandbox = {};
const wrapped = logicBlock + '; sandbox.parseParamFile=parseParamFile; sandbox.serializeParamFile=serializeParamFile; sandbox.makeDefaultState=makeDefaultState; sandbox.mergeIntoState=mergeIntoState; sandbox.detectOverlaps=detectOverlaps; sandbox.ELEMENTS=ELEMENTS;';
new Function('sandbox', wrapped)(sandbox);

let pass = 0, fail = 0;
function check(name, got, expected) {
  const a = JSON.stringify(got), b = JSON.stringify(expected);
  if (a === b) { console.log('  ✓', name); pass++; }
  else { console.log('  ✗', name, '\n     got:', a.slice(0,120), '\n     exp:', b.slice(0,120)); fail++; }
}

console.log('\nInlined-vs-module parity:');

// 1. Element catalogue identical
const ref_keys = require('../osd-params.js').ELEMENTS.map(e => e.key).join(',');
const inl_keys = sandbox.ELEMENTS.map(e => e.key).join(',');
check('Element catalogue identical', inl_keys, ref_keys);

// 2. Default state identical
check('makeDefaultState parity',
  sandbox.makeDefaultState(),
  ref_makeDefault());

// 3. Round-trip parity
const fixture = `
OSD1_ENABLE,1
OSD1_TXT_RES,2
OSD1_ALTITUDE_EN,1
OSD1_ALTITUDE_X,42
OSD1_ALTITUDE_Y,7
OSD2_BAT_VOLT_EN,1
OSD2_BAT_VOLT_X,10
OSD2_BAT_VOLT_Y,15
`;
check('parseParamFile parity',
  sandbox.parseParamFile(fixture),
  ref_parse(fixture));

const refState = ref_merge(ref_makeDefault(), ref_parse(fixture));
const inlState = sandbox.mergeIntoState(sandbox.makeDefaultState(), sandbox.parseParamFile(fixture));
check('mergeIntoState parity', inlState, refState);

check('serializeParamFile parity',
  sandbox.serializeParamFile(inlState, {includeComments:false}),
  ref_serialize(refState, {includeComments:false}));

// 4. Overlap detection
const s = sandbox.makeDefaultState();
s.screens[1].elements.BAT_VOLT = { en: 1, x: 1, y: 10 };
s.screens[1].elements.ALTITUDE = { en: 1, x: 3, y: 10 };
const r = ref_makeDefault();
r.screens[1].elements.BAT_VOLT = { en: 1, x: 1, y: 10 };
r.screens[1].elements.ALTITUDE = { en: 1, x: 3, y: 10 };
check('detectOverlaps parity',
  sandbox.detectOverlaps(s, 1),
  ref_overlaps(r, 1));

console.log(`\n${pass + fail} parity checks, ${pass} passed, ${fail} failed`);
process.exit(fail > 0 ? 1 : 0);
