/****************************************************************************
 *
 * (c) 2026 QGroundControl. https://qgroundcontrol.com
 *
 * APMOSDComponent.qml - OSD layout editor page.
 *
 * Layout mirrors prototype/index.html: three-column workspace
 * (palette | canvas | inspector) with a toolbar on top and status bar below.
 *
 * All parameter binding goes through APMOSDComponentController. Don't read
 * Vehicle.parameterManager directly from here - the controller exists so
 * QML stays declarative.
 *
 * Interaction parity with the prototype (osd-editor/prototype/index.html):
 *   - Click element on palette: selects (highlights). Second click toggles enable.
 *   - Drag element on canvas: continuous snap to grid cells, writes Fact on every move.
 *   - Arrow keys: nudge selected element by 1 cell. Shift+arrow: 5 cells.
 *   - Space toggles selected element enable; Delete/Backspace disables.
 *   - Overlapping elements render in red.
 *   - Import / Export use the controller's QFile-backed helpers.
 ****************************************************************************/

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls
// NOTE: Several "QGroundControl.*" sub-modules do NOT exist as registered QML
// modules in current QGC, and importing a non-existent module makes Qt
// silently fail to load the ENTIRE file (blank page, no error). Confirmed bad:
//   - QGroundControl.ScreenTools  (ScreenTools is a singleton from .Controls)
//   - QGroundControl.FactSystem   (Fact type comes from .FactControls)
//   - QGroundControl.Palette      (QGCPalette comes from .Controls)
// Everything this file needs is covered by the three imports above. Don't add
// those back. Verify any new "import QGroundControl.X" against the URIs in the
// qt_add_qml_module() calls before adding it.

SetupPage {
    id:                 osdPage
    pageComponent:      pageComponent
    pageName:           qsTr("OSD Layout")
    pageDescription:    qsTr("Configure on-screen display elements rendered on FPV goggles.")

    APMOSDComponentController { id: controller }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    // ------- Page-level reactive state -------

    // The element key currently selected for inspection. "" = none.
    property string selectedKey: ""

    // Demo preview: when true, the canvas shows ALL positioned elements with
    // their sample values + full phosphor styling, regardless of enable state.
    // Purely cosmetic (for screenshots / showcasing the layout) - it never
    // writes any Fact. Normal editing/visibility is unchanged when false.
    property bool demoMode: false

    // Cached set of keys participating in any overlap pair, recomputed
    // whenever the controller signals a change or the user finishes a drag.
    // Stored as an Array of strings; QML rebinds on reassignment.
    property var overlapKeys: []

    // Statusbar transient message (set by import/export, auto-clears).
    property string statusMessage: ""

    // ------- Reactive recomputation -------

    function recomputeOverlaps() {
        var pairs = controller.detectOverlaps()
        var s = []
        for (var i = 0; i < pairs.length; i++) {
            var p = pairs[i]
            if (s.indexOf(p.a) === -1) s.push(p.a)
            if (s.indexOf(p.b) === -1) s.push(p.b)
        }
        // Reassign so QML binding subscribers re-evaluate.
        overlapKeys = s
    }

    function isOverlapping(key) {
        return overlapKeys.indexOf(key) !== -1
    }

    function setStatus(msg) {
        statusMessage = msg
        statusClearTimer.restart()
    }

    Timer {
        id: statusClearTimer
        interval: 4000
        onTriggered: osdPage.statusMessage = ""
    }

    // Controller-side changes that may affect overlaps (import, clamp).
    Connections {
        target: controller
        function onOverlapsChanged()     { osdPage.recomputeOverlaps() }
        function onResolutionChanged()   { osdPage.recomputeOverlaps() }
        function onActiveScreenChanged() { osdPage.recomputeOverlaps() }
        function onImportCompleted(n)    { osdPage.setStatus(qsTr("Imported %1 parameter(s)").arg(n)) }
    }

    Component.onCompleted: recomputeOverlaps()

    // ------- Keyboard handling helper -------

    function nudgeSelected(dx, dy) {
        if (!selectedKey) return
        var xFact = controller.elXFact(selectedKey)
        var yFact = controller.elYFact(selectedKey)
        if (xFact && dx !== 0) {
            var nx = Math.max(0, Math.min(xFact.rawValue + dx, controller.maxX))
            if (nx !== xFact.rawValue) xFact.rawValue = nx
        }
        if (yFact && dy !== 0) {
            var ny = Math.max(0, Math.min(yFact.rawValue + dy, controller.maxY))
            if (ny !== yFact.rawValue) yFact.rawValue = ny
        }
        recomputeOverlaps()
    }

    function toggleSelectedEnabled() {
        if (!selectedKey) return
        var enF = controller.elEnabledFact(selectedKey)
        if (enF) {
            enF.rawValue = enF.rawValue ? 0 : 1
            recomputeOverlaps()
        }
    }

    function disableSelected() {
        if (!selectedKey) return
        var enF = controller.elEnabledFact(selectedKey)
        if (enF && enF.rawValue) {
            enF.rawValue = 0
            recomputeOverlaps()
        }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            id: root
            // SetupPage exposes availableWidth/availableHeight; the Loader
            // doesn't size the loaded item so we must claim them ourselves
            // or the page collapses to zero and renders blank.
            width:  availableWidth
            height: availableHeight
            spacing: ScreenTools.defaultFontPixelHeight / 2

            // ================================================================
            // TOOLBAR
            // ================================================================
            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                QGCLabel { text: qsTr("Screen") }
                ComboBox {
                    id: screenSelect
                    model: [1, 2, 3, 4]
                    currentIndex: controller.activeScreen - 1
                    onActivated: controller.activeScreen = currentValue
                }

                QGCLabel {
                    text: qsTr("Resolution")
                    Layout.leftMargin: ScreenTools.defaultFontPixelWidth
                }
                FactComboBox {
                    fact: controller.screenTxtResFact()
                    indexModel: false
                    visible: controller.screenTxtResFact() !== null
                }

                QGCButton {
                    text: qsTr("Clamp to bounds")
                    onClicked: controller.clampActiveScreen()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Clamp every element's X/Y to fit the current resolution")
                }

                QGCButton {
                    text: osdPage.demoMode ? qsTr("Demo: ON") : qsTr("Demo preview")
                    checkable: true
                    checked: osdPage.demoMode
                    onClicked: osdPage.demoMode = checked
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Preview all elements with sample values and FPV styling (cosmetic only - writes nothing)")
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

            // ================================================================
            // WORKSPACE (palette | canvas | inspector)
            // ================================================================
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: ScreenTools.defaultFontPixelWidth

                // ---------- PALETTE ----------
                Rectangle {
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 30
                    Layout.fillHeight: true
                    color: qgcPal.windowShade
                    border.color: qgcPal.text
                    border.width: 1

                    ListView {
                        id: paletteList
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth / 2
                        clip: true
                        model: controller.elementKeys
                        spacing: 2

                        delegate: Rectangle {
                            id: paletteRow
                            width: paletteList.width
                            height: ScreenTools.defaultFontPixelHeight * 1.8
                            color: osdPage.selectedKey === modelData
                                   ? qgcPal.buttonHighlight
                                   : "transparent"

                            property var  enabledFact: controller.elEnabledFact(modelData)
                            property bool isEnabled:   enabledFact && enabledFact.rawValue === 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2

                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: paletteRow.isEnabled ? qgcPal.colorGreen : qgcPal.colorGrey
                                }
                                QGCLabel {
                                    Layout.fillWidth: true
                                    text: controller.elementInfo(modelData)["label"] || modelData
                                    color: paletteRow.isEnabled ? qgcPal.text : qgcPal.textFieldDisabled
                                }
                                QGCLabel {
                                    text: modelData
                                    font.family: ScreenTools.fixedFontFamily
                                    font.pointSize: ScreenTools.smallFontPointSize
                                    color: qgcPal.textFieldDisabled
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (osdPage.selectedKey === modelData && paletteRow.enabledFact) {
                                        // Second click toggles enable.
                                        paletteRow.enabledFact.rawValue = paletteRow.isEnabled ? 0 : 1
                                        osdPage.recomputeOverlaps()
                                    } else {
                                        osdPage.selectedKey = modelData
                                    }
                                }
                            }
                        }
                    }
                }

                // ---------- CANVAS ----------
                Rectangle {
                    id: canvas
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "black"
                    border.color: qgcPal.text
                    border.width: 1
                    focus: true   // accept keyboard input when nothing else has focus

                    // Defensive: clamp to >= 1 so a transient zero width during
                    // layout doesn't divide by zero in element bindings.
                    property int cellW: Math.max(1, Math.floor(width  / (controller.maxX + 1)))
                    property int cellH: Math.max(1, Math.floor(height / (controller.maxY + 1)))

                    // Repaint grid when sizing changes.
                    onCellWChanged: gridCanvas.requestPaint()
                    onCellHChanged: gridCanvas.requestPaint()

                    // ---- Grid lines ----
                    Canvas {
                        id: gridCanvas
                        anchors.fill: parent
                        antialiasing: false
                        opacity: 1.0

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            // Phosphor-green grid, slightly brighter for a crisper
                            // "FPV screen" feel while staying subtle.
                            ctx.strokeStyle = "rgba(126, 231, 199, 0.09)"
                            ctx.lineWidth = 1
                            ctx.beginPath()
                            var cw = canvas.cellW
                            var ch = canvas.cellH
                            var cols = controller.maxX + 1
                            var rows = controller.maxY + 1
                            for (var c = 0; c <= cols; c++) {
                                var x = Math.floor(c * cw) + 0.5  // crisp 1px lines
                                ctx.moveTo(x, 0); ctx.lineTo(x, height)
                            }
                            for (var r = 0; r <= rows; r++) {
                                var y = Math.floor(r * ch) + 0.5
                                ctx.moveTo(0, y); ctx.lineTo(width, y)
                            }
                            ctx.stroke()
                        }
                    }

                    // ---- Click-through to deselect ----
                    MouseArea {
                        anchors.fill: parent
                        // Lower than element MouseAreas; clicks on bare canvas land here.
                        onClicked: {
                            osdPage.selectedKey = ""
                            canvas.forceActiveFocus()
                        }
                    }

                    // ---- Elements ----
                    Repeater {
                        model: controller.elementKeys
                        delegate: Loader {
                            // Normally: render elements whose Fact exists AND is
                            // enabled. In demo mode: render every element that has
                            // X/Y Facts, so the layout shows fully styled for a
                            // screenshot (still writes nothing).
                            active: {
                                var f = controller.elEnabledFact(modelData)
                                if (f !== null && f.rawValue === 1) return true
                                if (osdPage.demoMode) {
                                    return controller.elXFact(modelData) !== null
                                           && controller.elYFact(modelData) !== null
                                }
                                return false
                            }
                            sourceComponent: canvasElementComponent
                            property string elKey: modelData
                        }
                    }

                    Component {
                        id: canvasElementComponent

                        Rectangle {
                            id: elementRect
                            property string elKey: parent ? parent.elKey : ""
                            property var xFact:  controller.elXFact(elKey)
                            property var yFact:  controller.elYFact(elKey)
                            property var info:   controller.elementInfo(elKey)
                            property bool selected:    osdPage.selectedKey === elKey
                            property bool overlapping: osdPage.isOverlapping(elKey)

                            x: xFact ? xFact.rawValue * canvas.cellW : 0
                            y: yFact ? yFact.rawValue * canvas.cellH : 0
                            width:  Math.max(1, (info.width || 6) * canvas.cellW)
                            height: canvas.cellH

                            color: overlapping ? Qt.rgba(0.91, 0.36, 0.36, 0.12)
                                               : (selected ? Qt.rgba(0.49, 0.91, 0.78, 0.08)
                                                           : "transparent")
                            border.color: overlapping ? "#e85d5d"
                                                      : (selected ? "#7ee7c7" : "transparent")
                            border.width: 1

                            // Phosphor-glow OSD text. Uses built-in Text.Outline
                            // styling (no extra imports - keeps the file load-safe)
                            // to give a soft halo like a real FPV character OSD.
                            Text {
                                anchors.fill: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment:   Text.AlignVCenter
                                text: parent.info.sample || parent.elKey
                                font.family: ScreenTools.fixedFontFamily
                                font.bold: true
                                font.pixelSize: Math.max(10, canvas.cellH * 0.9)
                                color: parent.overlapping ? "#ff6b6b" : "#8effd6"
                                style: Text.Outline
                                styleColor: parent.overlapping
                                            ? Qt.rgba(1.0, 0.20, 0.20, 0.55)
                                            : Qt.rgba(0.20, 1.0, 0.70, 0.45)
                                elide: Text.ElideRight
                            }

                            // ---- Delta-based drag, matches prototype ----
                            // We don't use drag.target because the element's
                            // position is bound to xFact/yFact - moving it via
                            // drag.target would fight the binding. Instead we
                            // capture the mouse origin in canvas-local coords
                            // (mapToItem) and write Fact values directly on
                            // every move. Inspector and overlap recompute see
                            // updates live.
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                                preventStealing: true

                                property point startCanvas
                                property int   origX
                                property int   origY

                                onPressed: {
                                    osdPage.selectedKey = elementRect.elKey
                                    canvas.forceActiveFocus()
                                    var p = mapToItem(canvas, mouseX, mouseY)
                                    startCanvas = p
                                    origX = elementRect.xFact ? elementRect.xFact.rawValue : 0
                                    origY = elementRect.yFact ? elementRect.yFact.rawValue : 0
                                }

                                onPositionChanged: {
                                    if (!pressed) return
                                    if (!elementRect.xFact || !elementRect.yFact) return
                                    var p  = mapToItem(canvas, mouseX, mouseY)
                                    var dx = Math.round((p.x - startCanvas.x) / canvas.cellW)
                                    var dy = Math.round((p.y - startCanvas.y) / canvas.cellH)
                                    var nx = Math.max(0, Math.min(origX + dx, controller.maxX))
                                    var ny = Math.max(0, Math.min(origY + dy, controller.maxY))
                                    if (nx !== elementRect.xFact.rawValue) elementRect.xFact.rawValue = nx
                                    if (ny !== elementRect.yFact.rawValue) elementRect.yFact.rawValue = ny
                                }

                                onReleased: osdPage.recomputeOverlaps()
                            }
                        }
                    }

                    // ---- Keyboard handling ----
                    // Lives on the canvas Rectangle (focus: true above). Fires
                    // only when no input field has focus, since TextFields
                    // capture arrows for caret movement.
                    Keys.onPressed: (event) => {
                        if (!osdPage.selectedKey) return
                        var step = (event.modifiers & Qt.ShiftModifier) ? 5 : 1
                        var handled = true
                        switch (event.key) {
                            case Qt.Key_Left:  osdPage.nudgeSelected(-step,  0); break
                            case Qt.Key_Right: osdPage.nudgeSelected( step,  0); break
                            case Qt.Key_Up:    osdPage.nudgeSelected( 0, -step); break
                            case Qt.Key_Down:  osdPage.nudgeSelected( 0,  step); break
                            case Qt.Key_Space: osdPage.toggleSelectedEnabled(); break
                            case Qt.Key_Delete:
                            case Qt.Key_Backspace: osdPage.disableSelected(); break
                            default: handled = false
                        }
                        if (handled) event.accepted = true
                    }
                }

                // ---------- INSPECTOR ----------
                Rectangle {
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 32
                    Layout.fillHeight: true
                    color: qgcPal.windowShade
                    border.color: qgcPal.text
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        spacing: ScreenTools.defaultFontPixelHeight / 2

                        // --- Screen meta ---
                        QGCLabel {
                            text: qsTr("Screen %1").arg(controller.activeScreen)
                            font.bold: true
                        }
                        RowLayout {
                            QGCLabel { text: qsTr("Enabled"); Layout.fillWidth: true }
                            FactCheckBox {
                                fact: controller.screenEnabledFact()
                                visible: controller.screenEnabledFact() !== null
                            }
                        }
                        RowLayout {
                            QGCLabel { text: qsTr("RC PWM min"); Layout.fillWidth: true }
                            FactTextField {
                                fact: controller.screenChanMinFact()
                                visible: controller.screenChanMinFact() !== null
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 8
                            }
                        }
                        RowLayout {
                            QGCLabel { text: qsTr("RC PWM max"); Layout.fillWidth: true }
                            FactTextField {
                                fact: controller.screenChanMaxFact()
                                visible: controller.screenChanMaxFact() !== null
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 8
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true; height: 1
                            color: qgcPal.text; opacity: 0.2
                        }

                        // --- Selected element ---
                        QGCLabel {
                            text: osdPage.selectedKey
                                  ? controller.elementInfo(osdPage.selectedKey)["label"]
                                  : qsTr("No element selected")
                            font.bold: true
                            opacity: osdPage.selectedKey ? 1.0 : 0.5
                        }
                        QGCLabel {
                            text: osdPage.selectedKey
                                  ? qsTr("OSD%1_%2_*").arg(controller.activeScreen).arg(osdPage.selectedKey)
                                  : ""
                            font.family: ScreenTools.fixedFontFamily
                            font.pointSize: ScreenTools.smallFontPointSize
                            color: qgcPal.textFieldDisabled
                            visible: osdPage.selectedKey !== ""
                        }
                        RowLayout {
                            visible: osdPage.selectedKey !== ""
                            QGCLabel { text: qsTr("Enabled"); Layout.fillWidth: true }
                            FactCheckBox { fact: controller.elEnabledFact(osdPage.selectedKey) }
                        }
                        RowLayout {
                            visible: osdPage.selectedKey !== ""
                            QGCLabel { text: qsTr("X · col"); Layout.fillWidth: true }
                            FactTextField {
                                fact: controller.elXFact(osdPage.selectedKey)
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 8
                                onEditingFinished: osdPage.recomputeOverlaps()
                            }
                        }
                        RowLayout {
                            visible: osdPage.selectedKey !== ""
                            QGCLabel { text: qsTr("Y · row"); Layout.fillWidth: true }
                            FactTextField {
                                fact: controller.elYFact(osdPage.selectedKey)
                                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 8
                                onEditingFinished: osdPage.recomputeOverlaps()
                            }
                        }

                        QGCLabel {
                            visible: osdPage.selectedKey !== ""
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            font.pointSize: ScreenTools.smallFontPointSize
                            color: qgcPal.textFieldDisabled
                            text: qsTr("Arrow keys nudge by 1, Shift+arrow by 5. Space toggles enable.")
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            // ================================================================
            // STATUS BAR
            // ================================================================
            RowLayout {
                Layout.fillWidth: true
                QGCLabel {
                    text: qsTr("Screen %1 · %2").arg(controller.activeScreen)
                                                .arg(controller.resolutionName)
                }
                Item { width: ScreenTools.defaultFontPixelWidth * 2 }
                QGCLabel {
                    text: qsTr("%1 overlap(s)").arg(osdPage.overlapKeys.length / 2)
                    color: osdPage.overlapKeys.length > 0 ? qgcPal.colorOrange : qgcPal.colorGreen
                }
                Item { Layout.fillWidth: true }
                QGCLabel {
                    text: osdPage.statusMessage
                    color: qgcPal.text
                    visible: osdPage.statusMessage !== ""
                }
            }
        }
    }

    // ================================================================
    // FILE DIALOGS
    // ================================================================
    FileDialog {
        id: importDialog
        title: qsTr("Import .param file")
        nameFilters: [ "Param files (*.param *.txt)", "All files (*)" ]
        fileMode: FileDialog.OpenFile
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            var msg = controller.importParamTextFromUrl(selectedFile)
            osdPage.setStatus(msg)
            // overlap recompute handled by controller's overlapsChanged signal
        }
    }
    FileDialog {
        id: exportDialog
        title: qsTr("Export .param file")
        nameFilters: [ "Param files (*.param)", "All files (*)" ]
        fileMode: FileDialog.SaveFile
        defaultSuffix: "param"
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            var ok = controller.exportParamTextToUrl(selectedFile)
            osdPage.setStatus(ok ? qsTr("Exported %1").arg(selectedFile)
                                 : qsTr("Export failed - see logs"))
        }
    }
}
