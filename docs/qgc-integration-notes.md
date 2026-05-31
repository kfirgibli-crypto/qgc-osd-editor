# QGC Integration Notes

This is the porting guide from the web prototype to a QGroundControl SetupView page.
Everything below is **not tested in my environment** — I don't have a QGC build chain.
These are derived from QGC's repository conventions (`src/AutoPilotPlugins/`,
`SetupView.qml`, the Fact/Parameter system, and existing components like
`PowerComponent` and `SensorsComponent`).

## Architecture at a glance

```
┌─────────────────────────────────────────────────────────┐
│ OSDComponent (QML)                                       │
│   - palette list                                         │
│   - canvas grid (60x22 / 50x18 / 30x16)                 │
│   - inspector panel                                      │
│   - toolbar                                              │
└──────────────────┬──────────────────────────────────────┘
                   │ binds to
                   ▼
┌─────────────────────────────────────────────────────────┐
│ OSDComponentController (C++)                             │
│   - listens to Vehicle.parameterManager                  │
│   - exposes Q_PROPERTY for each Fact wrapper             │
│   - implements importFromText / exportToText             │
└──────────────────┬──────────────────────────────────────┘
                   │ reads/writes
                   ▼
┌─────────────────────────────────────────────────────────┐
│ Vehicle.parameterManager (existing QGC infrastructure)   │
│   ↓ MAVLink PARAM_SET / PARAM_VALUE                      │
│ ArduPilot autopilot                                      │
└─────────────────────────────────────────────────────────┘
```

The web prototype's `osd-params.js` maps **1:1** to a `OSDLayoutSerializer` C++
helper class. The state object becomes a thin wrapper around `Fact*` pointers
fetched by name.

## File placement in the QGC source tree

```
src/AutoPilotPlugins/APM/
├── APMAutoPilotPlugin.cc              ← register new component
├── APMAutoPilotPlugin.h
├── APMOSDComponent.cc                 ← NEW: VehicleComponent subclass
├── APMOSDComponent.h
├── APMOSDComponentController.cc       ← NEW: business logic, exposed to QML
├── APMOSDComponentController.h
└── APMOSDComponentSummary.qml         ← NEW: brief summary card

src/AutoPilotPlugins/APM/qml/
├── APMOSDComponent.qml                ← NEW: the editor page
└── APMOSDComponentSummary.qml         ← NEW: summary card body

src/AutoPilotPlugins/APM/Resources/
└── APMResources.qrc                   ← register new qml files
```

PX4 mirror (for later, when PX4 firmware catches up):

```
src/AutoPilotPlugins/PX4/PX4OSDComponent.* (PX4-specific param names)
```

## C++ skeleton: the controller

This is the file that mediates between QML and the Fact system. It mirrors the
data layer the prototype tests cover.

```cpp
// APMOSDComponentController.h
#pragma once

#include "FactPanelController.h"
#include <QObject>
#include <QStringList>

class APMOSDComponentController : public FactPanelController
{
    Q_OBJECT

public:
    APMOSDComponentController(void);

    // Active screen (1..4) drives which OSDn_ parameters QML binds to.
    Q_PROPERTY(int activeScreen READ activeScreen
               WRITE setActiveScreen NOTIFY activeScreenChanged)

    Q_PROPERTY(QStringList elementKeys READ elementKeys CONSTANT)
    Q_PROPERTY(int maxX READ maxX NOTIFY resolutionChanged)
    Q_PROPERTY(int maxY READ maxY NOTIFY resolutionChanged)

    // Fact lookups for any element on the active screen.
    Q_INVOKABLE Fact* elEnabledFact(const QString& key);
    Q_INVOKABLE Fact* elXFact(const QString& key);
    Q_INVOKABLE Fact* elYFact(const QString& key);

    // Meta facts for the active screen.
    Q_INVOKABLE Fact* screenEnabledFact();
    Q_INVOKABLE Fact* screenTxtResFact();
    Q_INVOKABLE Fact* screenChanMinFact();
    Q_INVOKABLE Fact* screenChanMaxFact();

    // Import a .param-format text blob: sets each known Fact accordingly.
    Q_INVOKABLE QString importParamText(const QString& text);

    // Export the current OSD layout as .param-format text.
    Q_INVOKABLE QString exportParamText();

    // Overlap detection for status bar UI.
    Q_INVOKABLE QVariantList detectOverlaps();

    int activeScreen() const { return _activeScreen; }
    void setActiveScreen(int n);
    QStringList elementKeys() const;
    int maxX() const;
    int maxY() const;

signals:
    void activeScreenChanged();
    void resolutionChanged();

private:
    int _activeScreen = 1;

    static const QList<struct ElementDef>& elements();
    QString paramName(const QString& suffix) const;
    Fact* lookupFact(const QString& paramName);
};
```

```cpp
// APMOSDComponentController.cc — abbreviated, showing the key methods.

struct ElementDef {
    QString key;     // matches AP_OSD source, e.g. "BAT_VOLT"
    QString label;
    int     width;   // visual width hint for QML canvas
    QString sample;  // mock value for visual rendering
};

const QList<ElementDef>& APMOSDComponentController::elements() {
    static const QList<ElementDef> defs = {
        { "ALTITUDE", "Altitude",         6, "142m"   },
        { "BAT_VOLT", "Battery voltage",  6, "16.2V"  },
        { "CURRENT",  "Current",          5, "8.2A"   },
        // ... rest of catalogue mirrors osd-params.js ELEMENTS array
    };
    return defs;
}

QString APMOSDComponentController::paramName(const QString& suffix) const {
    return QString("OSD%1_%2").arg(_activeScreen).arg(suffix);
}

Fact* APMOSDComponentController::lookupFact(const QString& name) {
    if (!_vehicle) return nullptr;
    if (!_vehicle->parameterManager()->parameterExists(
            FactSystem::defaultComponentId, name)) {
        qWarning() << "OSD param missing:" << name;
        return nullptr;
    }
    return _vehicle->parameterManager()->getParameter(
        FactSystem::defaultComponentId, name);
}

Fact* APMOSDComponentController::elEnabledFact(const QString& key) {
    return lookupFact(paramName(key + "_EN"));
}
Fact* APMOSDComponentController::elXFact(const QString& key) {
    return lookupFact(paramName(key + "_X"));
}
Fact* APMOSDComponentController::elYFact(const QString& key) {
    return lookupFact(paramName(key + "_Y"));
}

QString APMOSDComponentController::importParamText(const QString& text) {
    // This function is the C++ analogue of parseParamFile() + mergeIntoState()
    // in osd-params.js. The tests in test-osd-params.js define exactly what
    // it should do — port them as Qt Test cases.
    int applied = 0;
    for (const QString& rawLine : text.split(QRegularExpression("\\r?\\n"))) {
        QString line = rawLine.section('#', 0, 0).trimmed();
        if (line.isEmpty()) continue;
        QStringList parts = line.split(QRegularExpression("[,\\s]+"),
                                        Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;
        QString name = parts[0];
        bool ok = false;
        int value = parts[1].toInt(&ok);
        if (!ok) continue;
        Fact* fact = lookupFact(name);
        if (!fact) continue;
        fact->setRawValue(value);
        applied++;
    }
    return tr("Imported %1 parameters").arg(applied);
}

QString APMOSDComponentController::exportParamText() {
    // Mirrors serializeParamFile() in osd-params.js.
    QStringList out;
    out << "# ArduPilot OSD layout — exported by QGC";
    out << QString("# Generated: %1").arg(
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    out << "";
    for (int n = 1; n <= 4; ++n) {
        out << QString("# --- Screen %1 ---").arg(n);
        for (const QString& suffix : { "ENABLE", "TXT_RES", "CHAN_MIN", "CHAN_MAX" }) {
            auto* f = lookupFact(QString("OSD%1_%2").arg(n).arg(suffix));
            if (f) out << QString("OSD%1_%2,%3").arg(n).arg(suffix).arg(f->rawValue().toInt());
        }
        for (const auto& el : elements()) {
            for (const QString& field : { "EN", "X", "Y" }) {
                auto* f = lookupFact(QString("OSD%1_%2_%3").arg(n).arg(el.key).arg(field));
                if (f) out << QString("OSD%1_%2_%3,%4")
                              .arg(n).arg(el.key).arg(field).arg(f->rawValue().toInt());
            }
        }
        out << "";
    }
    return out.join('\n');
}
```

## QML skeleton: the editor page

```qml
// APMOSDComponent.qml — abbreviated, key bindings shown.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.ScreenTools

SetupPage {
    id:                 osdPage
    pageComponent:      pageComponent
    pageName:           qsTr("OSD Layout")
    pageDescription:    qsTr("Configure on-screen display elements shown on FPV goggles")

    Component {
        id: pageComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            // Toolbar: screen + resolution selector
            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel { text: qsTr("Screen") }
                FactComboBox {
                    fact: controller.screenSelectorFact // a synthetic fact mapping to activeScreen
                    indexModel: false
                }

                QGCLabel { text: qsTr("Resolution") }
                FactComboBox {
                    fact: controller.screenTxtResFact()
                    indexModel: false
                }

                Item { Layout.fillWidth: true }

                QGCButton {
                    text: qsTr("Import .param…")
                    onClicked: importDialog.open()
                }
                QGCButton {
                    text: qsTr("Export .param…")
                    onClicked: exportDialog.open()
                }
            }

            // Main 3-column layout
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // === Element palette ===
                Rectangle {
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 30
                    Layout.fillHeight: true
                    color: qgcPal.windowShade

                    ListView {
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        model: controller.elementKeys
                        delegate: PaletteRow {
                            elementKey: modelData
                            // Fact bindings using controller helpers:
                            enabledFact: controller.elEnabledFact(modelData)
                        }
                    }
                }

                // === Canvas ===
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#000000"
                    border.color: qgcPal.text
                    border.width: 1

                    Repeater {
                        model: controller.elementKeys
                        delegate: CanvasElement {
                            elementKey:   modelData
                            enabledFact:  controller.elEnabledFact(modelData)
                            xFact:        controller.elXFact(modelData)
                            yFact:        controller.elYFact(modelData)
                            cellWidth:    canvas.cellWidth
                            cellHeight:   canvas.cellHeight
                        }
                    }
                }

                // === Inspector ===
                Rectangle {
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 32
                    Layout.fillHeight: true
                    color: qgcPal.windowShade
                    // ... selected element X/Y inputs, screen meta facts ...
                }
            }
        }
    }
}
```

`PaletteRow` and `CanvasElement` are small reusable QML components — equivalent
to the `.palette-item` and `.canvas-element` divs in the prototype.

## Wiring it into QGC's plugin system

In `APMAutoPilotPlugin.cc`, add the new component to the list:

```cpp
QVariantList& APMAutoPilotPlugin::vehicleComponents(void) {
    if (_components.isEmpty() && !_vehicle->isOfflineEditingVehicle()) {
        // ... existing components ...
        _osdComponent = new APMOSDComponent(_vehicle, this);
        _osdComponent->setupTriggerSignals();
        _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_osdComponent)));
    }
    return _components;
}
```

The `VehicleComponent` subclass declares the QML URL, icon, name, and an
"setup complete" predicate that QGC uses to mark the component with a
checkmark/X in the SetupView sidebar.

## Mapping the test suite to Qt Test

Every JS test in `test-osd-params.js` should have a direct C++ analogue once
the controller is in place. Below maps the major ones:

| JS test                                              | C++ test it should become                       |
| ---------------------------------------------------- | ----------------------------------------------- |
| `parseParamFile` standard format                     | `importParamText` with comma-separated input    |
| `parseParamFile` strips comments                     | `importParamText` with `# inline` comments      |
| `serializeParamFile` round-trip                      | `exportParamText` → `importParamText` identity  |
| `clampToResolution`                                  | Setting txtRes=0 with X=55 clamps Fact to 29    |
| `detectOverlaps`                                     | `detectOverlaps()` returns expected QVariantList |

Use QGC's existing Qt Test harness (`src/qgcunittest/`) and run via
`./qgroundcontrol-start.sh --unittest`.

## Things to be careful about

1. **Parameter existence**. Not every ArduPilot build exposes every element
   (some require `HAL_MSP_ENABLED`, `HAL_PLUSCODE_ENABLE`, etc.). `lookupFact`
   returning `nullptr` is normal and the UI should hide rather than fail.
2. **`txtRes` boundary**. The X range param metadata says `0..59` even on
   SD; firmware silently clamps. Your QML should clamp visually too, using
   `controller.maxX/maxY`, not the param metadata.
3. **MAVLink param write rate-limiting**. Each Fact write is a `PARAM_SET`
   round-trip. Dragging an element across 30 cells = 30 writes. Throttle
   in the controller: batch X+Y writes per drag-end, not per pixel.
4. **PX4 vs ArduPilot**. Build this as APM-only first. PX4 doesn't have the
   same param schema yet (see the earlier conversation about `msp_osd`),
   so don't show the page on PX4 vehicles.
5. **Offline editing**. The component must work against the offline param
   list (`_vehicle->isOfflineEditingVehicle()`), since users commonly
   configure layouts without a connected vehicle.

## Reference reading order for the QGC port

1. `src/AutoPilotPlugins/APM/APMPowerComponent.*` — closest existing
   component in shape (single setup page, parameter-driven).
2. `src/AutoPilotPlugins/APM/qml/APMPowerComponent.qml` — QML conventions.
3. `src/FactSystem/FactPanelController.h` — base class your controller
   inherits.
4. `src/qgcunittest/FactSystemTestGeneric.cc` — example Qt Test pattern.
5. Mission Planner's `MissionPlanner/Controls/OSDSetup.cs` (C#, in the
   `ArduPilot/MissionPlanner` repo) — the reference UI for this exact
   feature. Useful for "what does Mission Planner do here?" questions.
