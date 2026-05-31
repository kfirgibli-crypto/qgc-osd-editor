/****************************************************************************
 *
 * (c) 2026 QGroundControl. https://qgroundcontrol.com
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"

class Vehicle;
class APMOSDComponentController;

class APMOSDControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    // Mirror of prototype/tests/test-osd-params.js test list:
    void testImport_commaFormat();
    void testImport_whitespaceFormat();
    void testImport_stripsInlineComments();
    void testImport_ignoresUnknownElements();
    void testImport_distinguishesScreens();
    void testImport_crlfLineEndings();
    void testImport_multiUnderscoreKeys();
    void testImport_ignoresNonOSDParams();

    void testExport_roundTrip();

    void testOverlaps_sameRowAdjacent();
    void testOverlaps_disabledExcluded();

    void testClamp_sdBounds();

    void testFixture_missionPlannerExport();

private:
    Vehicle*                    _vehicle    = nullptr;
    APMOSDComponentController*  _controller = nullptr;
};
