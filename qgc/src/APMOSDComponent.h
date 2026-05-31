/****************************************************************************
 *
 * (c) 2026 QGroundControl. https://qgroundcontrol.com
 *
 ****************************************************************************/

#pragma once

#include "VehicleComponent.h"

#include <QStringList>
#include <QString>
#include <QUrl>

class Vehicle;
class AutoPilotPlugin;

/**
 * VehicleComponent subclass for the OSD layout editor.
 *
 * Wires the QML page and summary card into QGC's SetupView. Marked as
 * UnknownVehicleComponent since OSD is firmware-specific (no entry in
 * AutoPilotPlugin::KnownVehicleComponent enum).
 *
 * Wired into the autopilot plugin in APMAutoPilotPlugin::vehicleComponents().
 */
class APMOSDComponent : public VehicleComponent
{
    Q_OBJECT

public:
    APMOSDComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot,
                    QObject* parent = nullptr);

    // ----- VehicleComponent overrides -----
    QString name() const override               { return tr(kName); }
    QString description() const override        { return tr(kDescription); }
    QString iconResource() const override       { return QString::fromLatin1(kIconResource); }
    bool    requiresSetup() const override      { return false; }     // OSD is optional
    bool    setupComplete() const override;
    QStringList setupCompleteChangedTriggerList() const override;

    QUrl setupSource() const override;
    QUrl summaryQmlSource() const override;

    // OSD layout is read-only safe; let users tweak even while armed.
    bool allowSetupWhileArmed() const override  { return true; }

private:
    static constexpr const char* kName        = "OSD";
    static constexpr const char* kDescription =
        "Configure on-screen display elements rendered on FPV goggles.";
    // Temporary stand-in icon - swap to a custom OSDComponentIcon.png once
    // we have an asset. Battery.svg is a system-wide icon that's known to exist.
    static constexpr const char* kIconResource = "/qmlimages/Battery.svg";
};
