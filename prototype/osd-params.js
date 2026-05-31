// osd-params.js
// Pure, dependency-free logic for ArduPilot OSD parameter files.
// This module is intentionally framework-agnostic so it can be unit-tested
// in Node and reused by both the web prototype and the QGC C++ port
// (the C++ version is a mechanical translation of these functions).

// ----------------------------------------------------------------------------
// Element catalogue
// ----------------------------------------------------------------------------
// Each entry describes one OSD element as defined in ArduPilot's
// libraries/AP_OSD/AP_OSD_Screen.cpp. `key` is the parameter suffix used in
// OSD{n}_{key}_{EN|X|Y}. `width` is the approximate cell width the element
// occupies on the OSD canvas, used purely for visual layout in the editor;
// firmware ignores this and renders its own text length.

const ELEMENTS = [
  { key: 'ALTITUDE', label: 'Altitude',           width: 6,  sample: '142m'    },
  { key: 'BAT_VOLT', label: 'Battery voltage',    width: 6,  sample: '16.2V'   },
  { key: 'CURRENT',  label: 'Current',            width: 5,  sample: '8.2A'    },
  { key: 'BATUSED',  label: 'mAh consumed',       width: 7,  sample: '850mAh'  },
  { key: 'AVGCELLV', label: 'Avg cell V',         width: 7,  sample: '3.85V'   },
  { key: 'RSSI',     label: 'RSSI',               width: 5,  sample: '98%'     },
  { key: 'SATS',     label: 'GPS sats',           width: 5,  sample: 'S14'     },
  { key: 'HDOP',     label: 'HDOP',               width: 6,  sample: 'H0.8'    },
  { key: 'FLTMODE',  label: 'Flight mode',        width: 7,  sample: 'LOITER'  },
  { key: 'GSPEED',   label: 'Ground speed',       width: 6,  sample: '12m/s'   },
  { key: 'ASPEED',   label: 'Airspeed',           width: 6,  sample: '14m/s'   },
  { key: 'VSPEED',   label: 'Climb rate',         width: 6,  sample: '+1.2'    },
  { key: 'HORIZON',  label: 'Artificial horizon', width: 13, sample: '———————•———————' },
  { key: 'HOME',     label: 'Home dist+dir',      width: 8,  sample: '↑ 142m'  },
  { key: 'HEADING',  label: 'Heading',            width: 7,  sample: '147°'    },
  { key: 'THROTTLE', label: 'Throttle %',         width: 5,  sample: '64%'     },
  { key: 'COMPASS',  label: 'Compass rose',       width: 9,  sample: 'N · E ·' },
  { key: 'FLTIME',   label: 'Flight time',        width: 7,  sample: '04:23'   },
  { key: 'DIST',     label: 'Distance flown',     width: 6,  sample: '2.4km'   },
  { key: 'MESSAGE',  label: 'Status messages',    width: 20, sample: 'GPS: 3D Fix acquired' },
  { key: 'CRSSHAIR', label: 'Crosshair',          width: 3,  sample: '+'       },
  { key: 'CLK',      label: 'Clock',              width: 6,  sample: '14:32'   },
  { key: 'WIND',     label: 'Wind',               width: 6,  sample: '6m/s'    },
  { key: 'STATS',    label: 'Flight stats',       width: 10, sample: 'FT 04:23' },
  { key: 'ARMING',   label: 'Arming status',      width: 6,  sample: 'ARMED'   },
];

// ----------------------------------------------------------------------------
// Resolutions
// ----------------------------------------------------------------------------
const RESOLUTIONS = {
  0: { name: 'SD 30×16',     cols: 30, rows: 16, hd: false },
  1: { name: 'HD 50×18',     cols: 50, rows: 18, hd: true  },
  2: { name: 'HD 60×22 DP',  cols: 60, rows: 22, hd: true  },
};

// ----------------------------------------------------------------------------
// Default layout — sensible starting positions on a 60×22 canvas.
// Values are 0-indexed (column, row from top-left).
// ----------------------------------------------------------------------------
const DEFAULT_LAYOUT = {
  ALTITUDE: { en: 1, x: 53, y: 5  },
  BAT_VOLT: { en: 1, x: 1,  y: 20 },
  CURRENT:  { en: 1, x: 8,  y: 20 },
  BATUSED:  { en: 1, x: 14, y: 20 },
  AVGCELLV: { en: 0, x: 1,  y: 19 },
  RSSI:     { en: 1, x: 53, y: 0  },
  SATS:     { en: 1, x: 46, y: 0  },
  HDOP:     { en: 0, x: 40, y: 0  },
  FLTMODE:  { en: 1, x: 26, y: 20 },
  GSPEED:   { en: 1, x: 1,  y: 5  },
  ASPEED:   { en: 0, x: 1,  y: 6  },
  VSPEED:   { en: 1, x: 53, y: 6  },
  HORIZON:  { en: 1, x: 23, y: 9  },
  HOME:     { en: 1, x: 1,  y: 0  },
  HEADING:  { en: 1, x: 26, y: 1  },
  THROTTLE: { en: 1, x: 1,  y: 1  },
  COMPASS:  { en: 0, x: 25, y: 3  },
  FLTIME:   { en: 1, x: 52, y: 20 },
  DIST:     { en: 1, x: 1,  y: 4  },
  MESSAGE:  { en: 1, x: 1,  y: 16 },
  CRSSHAIR: { en: 0, x: 29, y: 10 },
  CLK:      { en: 0, x: 53, y: 21 },
  WIND:     { en: 0, x: 1,  y: 7  },
  STATS:    { en: 0, x: 20, y: 8  },
  ARMING:   { en: 0, x: 26, y: 11 },
};

// ----------------------------------------------------------------------------
// State factory
// ----------------------------------------------------------------------------
function makeDefaultState() {
  const state = {
    activeScreen: 1,
    screens: {},
  };
  for (let n = 1; n <= 4; n++) {
    state.screens[n] = {
      enabled:   n === 1 ? 1 : 0,
      txtRes:    2, // HD 60×22 DisplayPort
      chanMin:   900,
      chanMax:   2100,
      elements:  {},
    };
    for (const el of ELEMENTS) {
      const def = DEFAULT_LAYOUT[el.key] || { en: 0, x: 0, y: 0 };
      // Only screen 1 ships with anything enabled.
      state.screens[n].elements[el.key] = {
        en: n === 1 ? def.en : 0,
        x:  def.x,
        y:  def.y,
      };
    }
  }
  return state;
}

// ----------------------------------------------------------------------------
// .param file -> state
// ----------------------------------------------------------------------------
function parseParamFile(text) {
  // Returns a partial state object containing only the screens/elements
  // present in the file. Caller merges this onto a base state.
  const result = { screens: {} };
  const elementKeys = new Set(ELEMENTS.map((e) => e.key));

  const lines = text.split(/\r?\n/);
  for (const raw of lines) {
    const line = raw.split('#')[0].trim();
    if (!line) continue;

    // Mission Planner uses comma; some tools use whitespace. Handle both.
    const parts = line.split(/[,\s]+/).filter(Boolean);
    if (parts.length < 2) continue;
    const name = parts[0];
    const value = Number(parts[1]);
    if (!Number.isFinite(value)) continue;

    const match = name.match(/^OSD([1-4])_(.+)$/);
    if (!match) continue;
    const screen = Number(match[1]);
    const rest = match[2];

    if (!result.screens[screen]) {
      result.screens[screen] = { elements: {} };
    }
    const s = result.screens[screen];

    // Per-screen meta
    if (rest === 'ENABLE')   { s.enabled = value | 0; continue; }
    if (rest === 'TXT_RES')  { s.txtRes  = value | 0; continue; }
    if (rest === 'CHAN_MIN') { s.chanMin = value | 0; continue; }
    if (rest === 'CHAN_MAX') { s.chanMax = value | 0; continue; }

    // Per-element: <KEY>_<EN|X|Y>
    const elMatch = rest.match(/^(.+)_(EN|X|Y)$/);
    if (!elMatch) continue;
    const elKey = elMatch[1];
    const field = elMatch[2];

    if (!elementKeys.has(elKey)) continue; // unknown element; skip silently

    if (!s.elements[elKey]) s.elements[elKey] = {};
    if (field === 'EN') s.elements[elKey].en = value | 0;
    if (field === 'X')  s.elements[elKey].x  = value | 0;
    if (field === 'Y')  s.elements[elKey].y  = value | 0;
  }
  return result;
}

function mergeIntoState(baseState, partial) {
  // Deep-merge a partial state (typically from parseParamFile) onto a base.
  const next = JSON.parse(JSON.stringify(baseState));
  if (!partial.screens) return next;
  for (const [nStr, ps] of Object.entries(partial.screens)) {
    const n = Number(nStr);
    if (!next.screens[n]) continue;
    const target = next.screens[n];
    if (ps.enabled !== undefined) target.enabled = ps.enabled;
    if (ps.txtRes  !== undefined) target.txtRes  = ps.txtRes;
    if (ps.chanMin !== undefined) target.chanMin = ps.chanMin;
    if (ps.chanMax !== undefined) target.chanMax = ps.chanMax;
    if (ps.elements) {
      for (const [k, v] of Object.entries(ps.elements)) {
        if (!target.elements[k]) target.elements[k] = { en: 0, x: 0, y: 0 };
        if (v.en !== undefined) target.elements[k].en = v.en;
        if (v.x  !== undefined) target.elements[k].x  = v.x;
        if (v.y  !== undefined) target.elements[k].y  = v.y;
      }
    }
  }
  return next;
}

// ----------------------------------------------------------------------------
// state -> .param file
// ----------------------------------------------------------------------------
function serializeParamFile(state, opts = {}) {
  const { includeComments = true, screens = [1, 2, 3, 4] } = opts;
  const out = [];
  if (includeComments) {
    out.push('# ArduPilot OSD layout — exported by osd-editor');
    out.push(`# Generated: ${new Date().toISOString()}`);
    out.push('');
  }

  for (const n of screens) {
    const s = state.screens[n];
    if (!s) continue;
    if (includeComments) out.push(`# --- Screen ${n} ---`);
    out.push(`OSD${n}_ENABLE,${s.enabled}`);
    out.push(`OSD${n}_TXT_RES,${s.txtRes}`);
    out.push(`OSD${n}_CHAN_MIN,${s.chanMin}`);
    out.push(`OSD${n}_CHAN_MAX,${s.chanMax}`);
    for (const el of ELEMENTS) {
      const e = s.elements[el.key];
      if (!e) continue;
      out.push(`OSD${n}_${el.key}_EN,${e.en}`);
      out.push(`OSD${n}_${el.key}_X,${e.x}`);
      out.push(`OSD${n}_${el.key}_Y,${e.y}`);
    }
    if (includeComments) out.push('');
  }
  return out.join('\n');
}

// ----------------------------------------------------------------------------
// Validation helpers
// ----------------------------------------------------------------------------
function clampToResolution(state, screenIndex) {
  const s = state.screens[screenIndex];
  const res = RESOLUTIONS[s.txtRes] || RESOLUTIONS[2];
  for (const el of ELEMENTS) {
    const e = s.elements[el.key];
    if (!e) continue;
    if (e.x < 0) e.x = 0;
    if (e.y < 0) e.y = 0;
    if (e.x > res.cols - 1) e.x = res.cols - 1;
    if (e.y > res.rows - 1) e.y = res.rows - 1;
  }
  return state;
}

function detectOverlaps(state, screenIndex) {
  // Returns an array of { elementA, elementB } pairs that overlap on the canvas.
  const s = state.screens[screenIndex];
  const active = [];
  for (const el of ELEMENTS) {
    const e = s.elements[el.key];
    if (!e || !e.en) continue;
    active.push({
      key: el.key,
      x: e.x,
      y: e.y,
      w: el.width,
    });
  }
  const overlaps = [];
  for (let i = 0; i < active.length; i++) {
    for (let j = i + 1; j < active.length; j++) {
      const a = active[i];
      const b = active[j];
      if (a.y !== b.y) continue;
      const aEnd = a.x + a.w;
      const bEnd = b.x + b.w;
      if (a.x < bEnd && b.x < aEnd) {
        overlaps.push({ a: a.key, b: b.key, y: a.y });
      }
    }
  }
  return overlaps;
}

// ----------------------------------------------------------------------------
// Exports
// ----------------------------------------------------------------------------
const api = {
  ELEMENTS,
  RESOLUTIONS,
  DEFAULT_LAYOUT,
  makeDefaultState,
  parseParamFile,
  mergeIntoState,
  serializeParamFile,
  clampToResolution,
  detectOverlaps,
};

if (typeof module !== 'undefined' && module.exports) {
  module.exports = api;
}
if (typeof window !== 'undefined') {
  window.OSDParams = api;
}
