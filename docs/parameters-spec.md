# ArduPilot OSD Parameter Reference

Extracted from `libraries/AP_OSD/AP_OSD_Screen.cpp` and `libraries/AP_OSD/AP_OSD.cpp` (master branch).

## Schema

Every OSD element follows the pattern:

```
OSD{n}_{ELEMENT}_EN    -> int (0 = disabled, 1 = enabled)
OSD{n}_{ELEMENT}_X     -> int (column, 0..maxX)
OSD{n}_{ELEMENT}_Y     -> int (row,    0..maxY)
```

Where `n ∈ {1, 2, 3, 4}` for the four screens.

## Resolution & grid size

`OSD{n}_TXT_RES` selects per-screen resolution:

| Value | Layout | maxX | maxY |
| ----- | ------ | ---- | ---- |
| 0     | SD 30×16          | 29 | 15 |
| 1     | HD 50×18 (MSP DP) | 49 | 17 |
| 2     | HD 60×22 (MSP DP) | 59 | 21 |

The parameter ranges in source (`@Range: 0 59` and `@Range: 0 21`) are written for the maximum HD resolution; values outside the active resolution silently clamp.

## Top-level OSD params (not per-screen)

| Param         | Purpose |
| ------------- | ------- |
| `OSD_TYPE`    | 0=None, 1=MAX7456, 2=SITL, 3=MSP, 4=TXONLY, 5=MSP_DISPLAYPORT |
| `OSD_TYPE2`   | Second backend (e.g. analog + DJI simultaneously) |
| `OSD_CHAN`    | RC channel for screen switching (0 = disabled) |
| `OSD_UNITS`   | 0=Metric, 1=Imperial, 2=AP Native, 3=Aviation |
| `OSD_W_RSSI`  | RSSI warning threshold |
| `OSD_W_BLINK` | Blink rate for warnings |
| `OSD_FONT`    | Font index (0–N depending on board) |

## Per-screen meta params

| Param                 | Purpose |
| --------------------- | ------- |
| `OSD{n}_ENABLE`       | 0/1 — enable this screen |
| `OSD{n}_CHAN_MIN`     | Lower PWM bound to activate this screen |
| `OSD{n}_CHAN_MAX`     | Upper PWM bound |
| `OSD{n}_TXT_RES`      | 0=SD, 1=HD 50×18, 2=HD 60×22 |
| `OSD{n}_STATS_EN`     | If 1, this screen displays flight stats |

## Elements (canonical list)

All elements present in `AP_OSD_Screen.cpp` parameter table, in source order. Some require build flags (`HAL_MSP_ENABLED`, `HAL_PLUSCODE_ENABLE`, etc.) and may not exist on every firmware build.

| Element key | Display description |
| ----------- | ------------------- |
| `ALTITUDE`  | Altitude AGL |
| `BAT_VOLT`  | Main battery voltage |
| `RSSI`      | RC signal strength |
| `CURRENT`   | Main battery current |
| `BATUSED`   | Primary battery mAh consumed |
| `SATS`      | GPS satellite count |
| `FLTMODE`   | Flight mode |
| `MESSAGE`   | MAVLink status messages |
| `GSPEED`    | GPS ground speed |
| `HORIZON`   | Artificial horizon |
| `HOME`      | Distance + direction to home |
| `HEADING`   | Heading |
| `THROTTLE`  | Throttle % |
| `COMPASS`   | Compass rose |
| `WIND`      | Wind speed/direction (or apparent on Rover) |
| `ASPEED`    | TECS-fused airspeed |
| `VSPEED`    | Climb rate |
| `ESCTEMP`   | ESC temperature (HAL_WITH_ESC_TELEM) |
| `ESCRPM`    | ESC RPM (HAL_WITH_ESC_TELEM) |
| `ESCAMPS`   | ESC current (HAL_WITH_ESC_TELEM) |
| `GPSLAT`    | GPS latitude |
| `GPSLONG`   | GPS longitude |
| `ROLL`      | Roll angle |
| `PITCH`     | Pitch angle |
| `TEMP`      | Primary baro temperature |
| `HDOP`      | GPS HDOP |
| `WAYPOINT`  | Bearing + distance to next WP |
| `XTRACK`    | Crosstrack error |
| `DIST`      | Total distance flown |
| `STATS`     | Flight stats |
| `FLTIME`    | Total flight time |
| `CLIMBEFF`  | Climb efficiency |
| `EFF`       | Flight efficiency mAh/km |
| `BTEMP`     | Secondary baro temperature (BARO_MAX_INSTANCES > 1) |
| `ATEMP`     | Airspeed sensor temperature |
| `BAT2_VLT`  | Battery 2 voltage |
| `BAT2USED`  | Battery 2 mAh consumed |
| `ASPD2`     | Direct airspeed sensor 2 |
| `ASPD1`     | Direct airspeed sensor 1 |
| `CLK`       | Clock from AP_RTC |
| `SIDEBARS`  | AH side bars (HAL_OSD_SIDEBAR_ENABLE or HAL_MSP_ENABLED) |
| `CRSSHAIR`  | Crosshair (MSP only) |
| `HOMEDIST`  | Distance from home (MSP only) |
| `HOMEDIR`   | Direction to home (MSP only) |
| `POWER`     | Power draw (MSP only) |
| `CELLVOLT`  | Average cell voltage (MSP only) |
| `BATTBAR`   | Battery usage bar (MSP only) |
| `ARMING`    | Arming status (MSP only) |
| `PLUSCODE`  | OLC plus code (HAL_PLUSCODE_ENABLE) |
| `CALLSIGN`  | Callsign from SD card |
| `CURRENT2`  | Battery 2 current |
| `VTX_PWR`   | VTX power (AP_VIDEOTX_ENABLED) |
| `TER_HGT`   | Height above terrain |
| `AVGCELLV`  | Average cell voltage |
| `RESTVOLT`  | Battery resting voltage |
| `FENCE`     | Geofence status |
| `RNGF`      | Rangefinder distance |
| _…remainder of file truncated; ~10 more elements present in source._ |

## .param file format

Mission Planner / QGC parameter files are plain text, one assignment per line:

```
OSD1_ENABLE,1
OSD1_TXT_RES,2
OSD1_ALTITUDE_EN,1
OSD1_ALTITUDE_X,1
OSD1_ALTITUDE_Y,1
OSD1_BAT_VOLT_EN,1
OSD1_BAT_VOLT_X,1
OSD1_BAT_VOLT_Y,20
…
```

Comments start with `#`. Values are integers for OSD params. Whitespace around the comma is tolerated by most parsers but not produced.

## MVP element set for prototype

The web prototype implements a representative subset that covers ~95% of typical FPV/quad use cases. Adding the remaining elements is mechanical — same schema, same UI, just more rows.

```
ALTITUDE BAT_VOLT CURRENT BATUSED AVGCELLV
RSSI SATS HDOP FLTMODE GSPEED
ASPEED VSPEED HORIZON HOME HEADING
THROTTLE COMPASS FLTIME DIST MESSAGE
CRSSHAIR CLK WIND STATS ARMING
```
