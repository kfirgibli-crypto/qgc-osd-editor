# Task 02 — Get QGC Building Locally

**Estimated:** 2–4 evenings (mostly waiting for compiles)
**Type:** Environment setup
**Preconditions:** Task 01 complete.

## Goal

A working `qgroundcontrol` binary built from source on your machine. **Without modifications first**, just to prove the toolchain works.

## Steps

### 1. Clone QGC

```bash
cd ~/projects   # or wherever you keep code
git clone --recursive https://github.com/mavlink/qgroundcontrol.git
cd qgroundcontrol
git checkout master   # or the latest stable branch
git submodule update --init --recursive
```

### 2. Install Qt 6

QGC requires Qt 6.6+ as of late 2025. Use the [official online installer](https://www.qt.io/download-qt-installer):

- Select the latest 6.x LTS for your platform
- Required components: Qt itself, the platform compiler (MSVC/clang/gcc), Qt Charts, Qt Multimedia, Qt 3D, Qt Location, Qt Positioning, Qt Serial Port, Qt SerialBus, Qt Speech
- Add Qt's `bin` to your PATH

Verify:
```bash
qmake --version
# Expect: QMake version 3.x using Qt version 6.x.x
```

### 3. Build

QGC uses CMake on master:

```bash
cd qgroundcontrol
mkdir build && cd build
cmake -G Ninja ..
ninja
```

Expect 10–30 minutes the first time. Subsequent builds are incremental.

### 4. Run

```bash
./Release/qgroundcontrol   # path varies by build config
```

Smoke check:
- Window opens
- Click "Vehicle Setup" → see the existing components (Summary, Firmware, Airframe, Sensors, Radio, Flight Modes, Power, …)
- Connect to nothing — just confirm the UI loads.

## Acceptance

You can run QGC from your terminal. The Setup view shows the existing
APM and PX4 components.

## Common stumbles

- **Submodules out of date**: `git submodule update --init --recursive` (the `--recursive` matters).
- **Wrong Qt version selected by CMake**: Set `CMAKE_PREFIX_PATH` to your Qt install path.
- **Missing platform packages on Linux**: `sudo apt install libxcb-* libgl1-mesa-dev libudev-dev libsdl2-dev libfontconfig1-dev`.
- **Build crashes on macOS over Apple Silicon**: Make sure the Qt install is arm64, not x86_64.

If you get stuck for more than an evening, ask in QGC's Discord — there's a #dev channel that responds fast. Don't burn more than 2 evenings on toolchain alone before asking.

## What Claude Code can do here

Mostly nothing — this is your local environment work. Once QGC builds,
update `CLAUDE.md`'s "Current state" with the QGC commit SHA you built
against, so future sessions know which API version to target.
