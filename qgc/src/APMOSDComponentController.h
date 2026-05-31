/****************************************************************************
 *
 * (c) 2026 QGroundControl. https://qgroundcontrol.com
 *
 * Author: Liat (initial implementation)
 *
 * QGroundControl is licensed according to the terms in the LICENSE file in
 * the project root. SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-or-later
 *
 ****************************************************************************/

#pragma once

#include "FactPanelController.h"

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

class Fact;

/**
 * Controller for the ArduPilot OSD layout editor.
 *
 * Exposes the OSDn_* parameter family (n = 1..4) as a structured API
 * consumable from QML. Mirrors the data layer in
 * `prototype/osd-params.js` — each pure function there has a corresponding
 * Q_INVOKABLE here, and each JS test in
 * `prototype/tests/test-osd-params.js` has a matching Qt Test case in
 * `tests/APMOSDControllerTest.cc`.
 *
 * Reference: docs/qgc-integration-notes.md (full porting guide)
 */
class APMOSDComponentController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT     // auto-registers under QGroundControl module (no qmlRegisterType needed in Qt 6)

public:
    APMOSDComponentController(void);

    // ----- Active screen selection -----
    // Screens 1..4 map to ArduPilot OSDn_ parameter families.
    Q_PROPERTY(int activeScreen READ activeScreen
               WRITE setActiveScreen NOTIFY activeScreenChanged)

    // ----- Element catalogue (constant) -----
    // Element keys in the order they should appear in the palette.
    // Mirrors ELEMENTS in osd-params.js (key only — labels/widths live in
    // a parallel data structure available via elementInfo()).
    Q_PROPERTY(QStringList elementKeys READ elementKeys CONSTANT)

    // ----- Active resolution bounds -----
    // Resolved from OSD{activeScreen}_TXT_RES. Used by QML for canvas sizing
    // and clamp validation on X/Y inputs.
    Q_PROPERTY(int maxX READ maxX NOTIFY resolutionChanged)
    Q_PROPERTY(int maxY READ maxY NOTIFY resolutionChanged)
    Q_PROPERTY(QString resolutionName READ resolutionName NOTIFY resolutionChanged)

    // ----- Fact lookups for the active screen -----
    // Return nullptr if the underlying parameter doesn't exist on this
    // firmware (build-flag-conditional elements like PLUSCODE may be absent).
    // QML must handle nullptr gracefully — hide the row, don't crash.
    Q_INVOKABLE Fact* elEnabledFact(const QString& key);
    Q_INVOKABLE Fact* elXFact(const QString& key);
    Q_INVOKABLE Fact* elYFact(const QString& key);

    // ----- Per-screen meta -----
    Q_INVOKABLE Fact* screenEnabledFact();
    Q_INVOKABLE Fact* screenTxtResFact();
    Q_INVOKABLE Fact* screenChanMinFact();
    Q_INVOKABLE Fact* screenChanMaxFact();

    // ----- Display metadata (constant per element) -----
    // Returns { "label": QString, "width": int, "sample": QString }
    Q_INVOKABLE QVariantMap elementInfo(const QString& key) const;

    // ----- Parameter file IO -----
    // importParamText: parses a Mission Planner-style .param blob and writes
    //   each recognized OSDn_* parameter to its Fact. Non-OSD params and
    //   unknown elements are silently skipped. Returns a human-readable
    //   status string for the UI.
    Q_INVOKABLE QString importParamText(const QString& text);

    // exportParamText: serializes all four screens to .param format.
    //   Output is identical in structure to what Mission Planner produces.
    Q_INVOKABLE QString exportParamText();

    // File I/O conveniences for the QML FileDialog. QML can't write files
    // natively; these are thin wrappers around importParamText/exportParamText
    // that handle the QFile read/write. Both accept a QUrl (file:/// scheme)
    // as produced by QtQuick.Dialogs.FileDialog.selectedFile.
    //   importParamTextFromUrl: returns the same human-readable status as
    //     importParamText, or a descriptive error if the file can't be read.
    //   exportParamTextToUrl: returns true on success, false on any I/O failure.
    Q_INVOKABLE QString importParamTextFromUrl(const QUrl& fileUrl);
    Q_INVOKABLE bool    exportParamTextToUrl(const QUrl& fileUrl);

    // ----- Validation -----
    // detectOverlaps: returns a QVariantList of overlap pairs on the active
    //   screen. Each entry is a QVariantMap { "a": "BAT_VOLT", "b": "ALTITUDE",
    //   "y": 10 }. QML uses this to mark overlapping elements in red.
    Q_INVOKABLE QVariantList detectOverlaps();

    // clampActiveScreen: clamps all element positions to the active
    //   resolution bounds. Call after resolution changes.
    Q_INVOKABLE void clampActiveScreen();

    // ----- Plain getters -----
    int activeScreen() const { return _activeScreen; }
    void setActiveScreen(int n);
    QStringList elementKeys() const;
    int maxX() const;
    int maxY() const;
    QString resolutionName() const;

signals:
    void activeScreenChanged();
    void resolutionChanged();
    void importCompleted(int parametersApplied);
    // Fired after any controller-side change that could alter the overlap
    // topology of the active screen (import, clamp, file-import). QML wires
    // this to re-evaluate detectOverlaps() and update red highlighting.
    // Per-drag/per-edit changes are still the QML's responsibility to
    // recompute — the signal covers only changes made through the controller.
    void overlapsChanged();

private slots:
    void _onTxtResChanged();

private:
    int _activeScreen = 1;

    // Builds "OSD{n}_{suffix}" for the active screen.
    QString _paramName(const QString& suffix) const;

    // Looks up a Fact by full param name, returns nullptr if missing.
    // Logs a qDebug message on miss so devs see which params are absent.
    Fact* _lookupFact(const QString& paramName);

    // Connect/disconnect TXT_RES change signal when screen switches, so
    // we re-emit resolutionChanged on the right fact.
    void _rebindResolutionFact();
    Fact* _boundTxtResFact = nullptr;
};
