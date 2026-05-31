/****************************************************************************
 *
 * (c) 2026 QGroundControl. https://qgroundcontrol.com
 *
 * APMOSDControllerTest.cc — Qt Test cases for APMOSDComponentController.
 *
 * Test policy: every test in prototype/tests/test-osd-params.js has a
 * matching case here. The JS tests pass; these MUST also pass before the
 * controller is considered production-ready.
 *
 * Harness status: as of Task 03, init() is still a placeholder — the
 * Vehicle + Controller construction is wired in Task 05 (plugin
 * integration) using QGC's existing test harness pattern from
 * src/qgcunittest/FactSystemTestGeneric.cc. Until then, every test exits
 * early via QSKIP at the `if (!_controller)` guard at the top of each
 * body. The assertion bodies are ported from the JS source-of-truth and
 * are ready to execute the moment the harness lights up.
 ****************************************************************************/

#include "APMOSDControllerTest.h"

#include "APMOSDComponentController.h"
#include "Fact.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "Vehicle.h"

#include <QtTest/QtTest>
#include <QVariantList>
#include <QVariantMap>

UT_REGISTER_TEST(APMOSDControllerTest)

void APMOSDControllerTest::init() {
    UnitTest::init();
    // Spin up an offline-editing vehicle with ArduCopter param defaults
    // loaded, so OSD1_* parameters exist as Facts.
    // TODO(Task 05): wire to QGC's existing test harness pattern. See
    //   src/qgcunittest/FactSystemTestGeneric.cc for the reference.
    _vehicle = nullptr;     // placeholder
    _controller = nullptr;  // placeholder
}

void APMOSDControllerTest::cleanup() {
    delete _controller;
    _controller = nullptr;
    UnitTest::cleanup();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

// Single message used at the top of every test until the harness lands.
constexpr const char* kHarnessPending =
    "Awaiting Task 05: test harness wiring (init() needs a real Vehicle + "
    "Controller). See APMOSDControllerTest::init() TODO.";

// Search a detectOverlaps() result for a pair {a, b} regardless of order.
bool hasOverlapPair(const QVariantList& list, const QString& a, const QString& b) {
    for (const QVariant& v : list) {
        const QVariantMap m = v.toMap();
        const QString ka = m.value(QStringLiteral("a")).toString();
        const QString kb = m.value(QStringLiteral("b")).toString();
        if ((ka == a && kb == b) || (ka == b && kb == a)) return true;
    }
    return false;
}

} // namespace

// ---------------------------------------------------------------------------
// Mirrors JS test: "Parses standard Mission Planner comma format"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testImport_commaFormat()
{
    if (!_controller) QSKIP(kHarnessPending);

    const QString input = QStringLiteral(
        "# header\n"
        "OSD1_ENABLE,1\n"
        "OSD1_TXT_RES,2\n"
        "OSD1_ALTITUDE_EN,1\n"
        "OSD1_ALTITUDE_X,53\n"
        "OSD1_ALTITUDE_Y,5\n");
    _controller->importParamText(input);

    _controller->setActiveScreen(1);
    QCOMPARE(_controller->screenEnabledFact()->rawValue().toInt(), 1);
    QCOMPARE(_controller->screenTxtResFact()->rawValue().toInt(), 2);
    QCOMPARE(_controller->elEnabledFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 1);
    QCOMPARE(_controller->elXFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 53);
    QCOMPARE(_controller->elYFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 5);
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Parses whitespace-separated format"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testImport_whitespaceFormat()
{
    if (!_controller) QSKIP(kHarnessPending);

    const QString input = QStringLiteral(
        "OSD1_BAT_VOLT_EN 1\n"
        "OSD1_BAT_VOLT_X 1\n"
        "OSD1_BAT_VOLT_Y 20\n");
    _controller->importParamText(input);

    _controller->setActiveScreen(1);
    QCOMPARE(_controller->elEnabledFact(QStringLiteral("BAT_VOLT"))->rawValue().toInt(), 1);
    QCOMPARE(_controller->elXFact(QStringLiteral("BAT_VOLT"))->rawValue().toInt(), 1);
    QCOMPARE(_controller->elYFact(QStringLiteral("BAT_VOLT"))->rawValue().toInt(), 20);
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Strips inline comments"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testImport_stripsInlineComments()
{
    if (!_controller) QSKIP(kHarnessPending);

    const QString input = QStringLiteral(
        "OSD1_RSSI_X,42  # comment here\n"
        "OSD1_RSSI_Y,3\n");
    _controller->importParamText(input);

    _controller->setActiveScreen(1);
    QCOMPARE(_controller->elXFact(QStringLiteral("RSSI"))->rawValue().toInt(), 42);
    QCOMPARE(_controller->elYFact(QStringLiteral("RSSI"))->rawValue().toInt(), 3);
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Ignores unknown elements without error"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testImport_ignoresUnknownElements()
{
    if (!_controller) QSKIP(kHarnessPending);

    // Capture ALTITUDE's pre-state so we can confirm only the legit param applied.
    _controller->setActiveScreen(1);
    Fact* altEn = _controller->elEnabledFact(QStringLiteral("ALTITUDE"));
    QVERIFY(altEn);
    const int beforeAlt = altEn->rawValue().toInt();
    Q_UNUSED(beforeAlt);  // We only care that the import doesn't crash and ALTITUDE updates.

    const QString input = QStringLiteral(
        "OSD1_NOT_REAL_EN,1\n"
        "OSD1_ALTITUDE_EN,1\n");
    // Must not throw or crash.
    _controller->importParamText(input);

    QCOMPARE(altEn->rawValue().toInt(), 1);  // legit param was applied
    // Unknown element NOT_REAL has no corresponding Fact, so nothing to assert
    // about its absence — the test passes simply by not crashing.
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Distinguishes per-screen params"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testImport_distinguishesScreens()
{
    if (!_controller) QSKIP(kHarnessPending);

    const QString input = QStringLiteral(
        "OSD1_ALTITUDE_X,10\n"
        "OSD2_ALTITUDE_X,20\n"
        "OSD3_ALTITUDE_X,30\n");
    _controller->importParamText(input);

    _controller->setActiveScreen(1);
    QCOMPARE(_controller->elXFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 10);
    _controller->setActiveScreen(2);
    QCOMPARE(_controller->elXFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 20);
    _controller->setActiveScreen(3);
    QCOMPARE(_controller->elXFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 30);
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Handles CRLF line endings"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testImport_crlfLineEndings()
{
    if (!_controller) QSKIP(kHarnessPending);

    const QString input = QStringLiteral(
        "OSD1_ALTITUDE_X,7\r\n"
        "OSD1_ALTITUDE_Y,8\r\n");
    _controller->importParamText(input);

    _controller->setActiveScreen(1);
    QCOMPARE(_controller->elXFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 7);
    QCOMPARE(_controller->elYFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 8);
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Handles multi-underscore element keys like BAT_VOLT"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testImport_multiUnderscoreKeys()
{
    if (!_controller) QSKIP(kHarnessPending);

    // BAT_VOLT is a real catalogue entry; BAT2_VLT is not. Both share the
    // multi-underscore shape — the greedy regex must capture key=BAT_VOLT
    // and field=X for the first, and silently skip the second.
    const QString input = QStringLiteral(
        "OSD1_BAT_VOLT_X,5\n"
        "OSD1_BAT2_VLT_X,12\n");
    _controller->importParamText(input);

    _controller->setActiveScreen(1);
    QCOMPARE(_controller->elXFact(QStringLiteral("BAT_VOLT"))->rawValue().toInt(), 5);
    // BAT2_VLT has no Fact in our catalogue — nothing to assert; test passes by not crashing.
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Ignores non-OSD params"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testImport_ignoresNonOSDParams()
{
    if (!_controller) QSKIP(kHarnessPending);

    // Capture BATT_CAPACITY pre-import so we can verify we did NOT touch it.
    // This is critical: a sloppy importer would happily write 5000 to
    // BATT_CAPACITY, which is a real ArduPilot param. Our impl must only
    // consider names matching ^OSD([1-4])_...$.
    //
    // BATT_CAPACITY may not exist in every offline-editing profile — if
    // absent, we still confirm the OSD param applied (proves the loop
    // didn't abort early on the non-OSD line).
    Fact* battCap = nullptr;
    int battBefore = -1;
    if (_vehicle && _vehicle->parameterManager() &&
        _vehicle->parameterManager()->parameterExists(
            ParameterManager::defaultComponentId, QStringLiteral("BATT_CAPACITY"))) {
        battCap = _vehicle->parameterManager()->getParameter(
            ParameterManager::defaultComponentId, QStringLiteral("BATT_CAPACITY"));
        battBefore = battCap->rawValue().toInt();
    }

    const QString input = QStringLiteral(
        "BATT_CAPACITY,5000\n"
        "OSD1_ALTITUDE_X,3\n");
    _controller->importParamText(input);

    _controller->setActiveScreen(1);
    QCOMPARE(_controller->elXFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 3);

    if (battCap) {
        QCOMPARE(battCap->rawValue().toInt(), battBefore);  // untouched
    }
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Round trips: parse(serialize(state)) ≈ state"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testExport_roundTrip()
{
    if (!_controller) QSKIP(kHarnessPending);

    // Pin a known custom layout, export, mutate, re-import, verify it matches.
    _controller->setActiveScreen(1);
    Fact* altX = _controller->elXFact(QStringLiteral("ALTITUDE"));
    Fact* altY = _controller->elYFact(QStringLiteral("ALTITUDE"));
    Fact* batX = _controller->elXFact(QStringLiteral("BAT_VOLT"));
    QVERIFY(altX && altY && batX);

    altX->setRawValue(42);
    altY->setRawValue(7);
    batX->setRawValue(13);

    const QString exported = _controller->exportParamText();
    QVERIFY(!exported.isEmpty());
    QVERIFY(exported.contains(QStringLiteral("OSD1_ALTITUDE_X,42")));
    QVERIFY(exported.contains(QStringLiteral("OSD1_ALTITUDE_Y,7")));
    QVERIFY(exported.contains(QStringLiteral("OSD1_BAT_VOLT_X,13")));

    // Scramble values then re-import; everything should restore.
    altX->setRawValue(0);
    altY->setRawValue(0);
    batX->setRawValue(0);

    _controller->importParamText(exported);

    QCOMPARE(altX->rawValue().toInt(), 42);
    QCOMPARE(altY->rawValue().toInt(), 7);
    QCOMPARE(batX->rawValue().toInt(), 13);
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Reports overlap for same-row adjacent elements"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testOverlaps_sameRowAdjacent()
{
    if (!_controller) QSKIP(kHarnessPending);

    _controller->setActiveScreen(1);
    // BAT_VOLT (width 6) at (1, 10); ALTITUDE (width 6) at (3, 10).
    // Ranges [1,7) and [3,9) on row 10 → overlap.
    Fact* batEn = _controller->elEnabledFact(QStringLiteral("BAT_VOLT"));
    Fact* batX  = _controller->elXFact(QStringLiteral("BAT_VOLT"));
    Fact* batY  = _controller->elYFact(QStringLiteral("BAT_VOLT"));
    Fact* altEn = _controller->elEnabledFact(QStringLiteral("ALTITUDE"));
    Fact* altX  = _controller->elXFact(QStringLiteral("ALTITUDE"));
    Fact* altY  = _controller->elYFact(QStringLiteral("ALTITUDE"));
    QVERIFY(batEn && batX && batY && altEn && altX && altY);

    batEn->setRawValue(1); batX->setRawValue(1); batY->setRawValue(10);
    altEn->setRawValue(1); altX->setRawValue(3); altY->setRawValue(10);

    const QVariantList overlaps = _controller->detectOverlaps();
    QVERIFY2(hasOverlapPair(overlaps, QStringLiteral("BAT_VOLT"), QStringLiteral("ALTITUDE")),
             "BAT_VOLT/ALTITUDE overlap not detected on same row");
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Disabled elements never overlap"
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testOverlaps_disabledExcluded()
{
    if (!_controller) QSKIP(kHarnessPending);

    _controller->setActiveScreen(1);
    Fact* batEn = _controller->elEnabledFact(QStringLiteral("BAT_VOLT"));
    Fact* batX  = _controller->elXFact(QStringLiteral("BAT_VOLT"));
    Fact* batY  = _controller->elYFact(QStringLiteral("BAT_VOLT"));
    Fact* altEn = _controller->elEnabledFact(QStringLiteral("ALTITUDE"));
    Fact* altX  = _controller->elXFact(QStringLiteral("ALTITUDE"));
    Fact* altY  = _controller->elYFact(QStringLiteral("ALTITUDE"));
    QVERIFY(batEn && batX && batY && altEn && altX && altY);

    // Same coords as the previous test, but BAT_VOLT is disabled → no overlap.
    batEn->setRawValue(0); batX->setRawValue(1); batY->setRawValue(10);
    altEn->setRawValue(1); altX->setRawValue(1); altY->setRawValue(10);

    const QVariantList overlaps = _controller->detectOverlaps();
    QVERIFY2(!hasOverlapPair(overlaps, QStringLiteral("BAT_VOLT"), QStringLiteral("ALTITUDE")),
             "disabled BAT_VOLT should not contribute to overlap list");
}

// ---------------------------------------------------------------------------
// Mirrors JS test: "Clamps values exceeding SD bounds"
//
// Project memory: per-element clamping is the v1 design — see
// memory/project_sd_clamp_behavior.md. Overlaps after HD→SD are intentional;
// don't ever change clampActiveScreen to "fix" them.
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testClamp_sdBounds()
{
    if (!_controller) QSKIP(kHarnessPending);

    _controller->setActiveScreen(1);
    Fact* txtRes = _controller->screenTxtResFact();
    Fact* altX   = _controller->elXFact(QStringLiteral("ALTITUDE"));
    Fact* altY   = _controller->elYFact(QStringLiteral("ALTITUDE"));
    QVERIFY(txtRes && altX && altY);

    // Switch to SD (mode 0 → 30x16 → maxX=29, maxY=15), park ALTITUDE OOB.
    txtRes->setRawValue(0);
    altX->setRawValue(55);
    altY->setRawValue(20);

    _controller->clampActiveScreen();

    QCOMPARE(altX->rawValue().toInt(), 29);
    QCOMPARE(altY->rawValue().toInt(), 15);
}

// ---------------------------------------------------------------------------
// Real-world fixture test — load fixtures/mission-planner-export.param
// and verify selected element positions match expected values.
// ---------------------------------------------------------------------------
void APMOSDControllerTest::testFixture_missionPlannerExport()
{
    if (!_controller) QSKIP(kHarnessPending);

    // Same fixture shape as in test-osd-params.js. Includes a non-OSD param
    // (BATT_CAPACITY) that must be silently ignored.
    const QString fixture = QStringLiteral(
        "# Vehicle parameters\n"
        "OSD_TYPE,5\n"
        "OSD_TYPE2,0\n"
        "OSD_CHAN,0\n"
        "OSD_UNITS,0\n"
        "OSD1_ENABLE,1\n"
        "OSD1_TXT_RES,2\n"
        "OSD1_CHAN_MIN,900\n"
        "OSD1_CHAN_MAX,2100\n"
        "OSD1_ALTITUDE_EN,1\n"
        "OSD1_ALTITUDE_X,53\n"
        "OSD1_ALTITUDE_Y,5\n"
        "OSD1_BAT_VOLT_EN,1\n"
        "OSD1_BAT_VOLT_X,1\n"
        "OSD1_BAT_VOLT_Y,20\n"
        "OSD1_FLTMODE_EN,1\n"
        "OSD1_FLTMODE_X,26\n"
        "OSD1_FLTMODE_Y,20\n"
        "OSD1_HORIZON_EN,1\n"
        "OSD1_HORIZON_X,23\n"
        "OSD1_HORIZON_Y,9\n"
        "BATT_CAPACITY,5000\n");
    _controller->importParamText(fixture);

    _controller->setActiveScreen(1);
    QCOMPARE(_controller->elXFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 53);
    QCOMPARE(_controller->elYFact(QStringLiteral("BAT_VOLT"))->rawValue().toInt(), 20);
    QCOMPARE(_controller->elXFact(QStringLiteral("HORIZON"))->rawValue().toInt(), 23);
    QCOMPARE(_controller->elXFact(QStringLiteral("FLTMODE"))->rawValue().toInt(), 26);

    // Round-trip: export → re-import → all four anchor positions survive.
    const QString exported = _controller->exportParamText();
    QVERIFY(!exported.isEmpty());

    // Scramble, then restore from exported text.
    _controller->elXFact(QStringLiteral("ALTITUDE"))->setRawValue(0);
    _controller->elXFact(QStringLiteral("HORIZON"))->setRawValue(0);
    _controller->importParamText(exported);

    QCOMPARE(_controller->elXFact(QStringLiteral("ALTITUDE"))->rawValue().toInt(), 53);
    QCOMPARE(_controller->elXFact(QStringLiteral("HORIZON"))->rawValue().toInt(), 23);
}
