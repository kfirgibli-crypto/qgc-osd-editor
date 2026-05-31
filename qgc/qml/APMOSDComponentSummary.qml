/****************************************************************************
 *
 * (c) 2026 QGroundControl. https://qgroundcontrol.com
 *
 * APMOSDComponentSummary.qml - brief summary card shown in SetupView sidebar.
 *
 * Pattern mirrors APMPowerComponentSummary.qml in the current QGC tree:
 * Item root + FactPanelController + VehicleSummaryRow rows. The older
 * scaffold used FactPanel which no longer exists in upstream QGC.
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    FactPanelController { id: controller }

    // OSD element keys (kept in sync with osd-params.js + APMOSDComponentController.cc).
    // Used only to count "how many _EN=1" per screen.
    readonly property var _elementKeys: [
        "ALTITUDE", "BAT_VOLT", "CURRENT",  "BATUSED",  "AVGCELLV",
        "RSSI",     "SATS",     "HDOP",     "FLTMODE",  "GSPEED",
        "ASPEED",   "VSPEED",   "HORIZON",  "HOME",     "HEADING",
        "THROTTLE", "COMPASS",  "FLTIME",   "DIST",     "MESSAGE",
        "CRSSHAIR", "CLK",      "WIND",     "STATS",    "ARMING",
    ]

    function _enabledCount(screenIndex) {
        var n = 0
        for (var i = 0; i < _elementKeys.length; i++) {
            var name = "OSD" + screenIndex + "_" + _elementKeys[i] + "_EN"
            if (controller.parameterExists(-1, name)) {
                if (controller.getParameterFact(-1, name).rawValue === 1) n++
            }
        }
        return n
    }

    function _backendName() {
        if (!controller.parameterExists(-1, "OSD_TYPE")) return qsTr("n/a")
        var v = controller.getParameterFact(-1, "OSD_TYPE").rawValue
        switch (v) {
            case 0: return qsTr("None")
            case 1: return qsTr("MAX7456")
            case 2: return qsTr("SITL")
            case 3: return qsTr("MSP")
            case 4: return qsTr("TX only")
            case 5: return qsTr("MSP DisplayPort")
        }
        return qsTr("Unknown (%1)").arg(v)
    }

    function _screenStatus(screenIndex) {
        var enParam = "OSD" + screenIndex + "_ENABLE"
        if (!controller.parameterExists(-1, enParam)) return qsTr("n/a")
        var enabled = controller.getParameterFact(-1, enParam).rawValue === 1
        if (!enabled) return qsTr("Off")
        return qsTr("%1 element(s)").arg(_enabledCount(screenIndex))
    }

    ColumnLayout {
        id: mainLayout
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("Backend")
            valueText: _backendName()
        }
        Repeater {
            model: 4
            delegate: VehicleSummaryRow {
                required property int index
                labelText: qsTr("Screen %1").arg(index + 1)
                valueText: _screenStatus(index + 1)
            }
        }
    }
}
