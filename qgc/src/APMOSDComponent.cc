/****************************************************************************
 *
 * (c) 2026 QGroundControl. https://qgroundcontrol.com
 *
 ****************************************************************************/

#include "APMOSDComponent.h"

#include "AutoPilotPlugin.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "Vehicle.h"

APMOSDComponent::APMOSDComponent(Vehicle* vehicle,
                                 AutoPilotPlugin* autopilot,
                                 QObject* parent)
    : VehicleComponent(vehicle, autopilot,
                       AutoPilotPlugin::UnknownVehicleComponent, parent)
{
}

bool APMOSDComponent::setupComplete() const
{
    // Consider setup complete iff at least one screen is enabled. OSD is
    // optional - the vehicle flies without it - so this just gates the
    // checkmark in the sidebar, not arming.
    if (!_vehicle || !_vehicle->parameterManager()) return true;
    if (!_vehicle->parameterManager()->parameterExists(
            ParameterManager::defaultComponentId, QStringLiteral("OSD1_ENABLE"))) {
        // No OSD params at all (firmware built without OSD support). Hide
        // the "needs setup" marker rather than nag the user.
        return true;
    }
    Fact* f = _vehicle->parameterManager()->getParameter(
        ParameterManager::defaultComponentId, QStringLiteral("OSD1_ENABLE"));
    return f && f->rawValue().toInt() == 1;
}

QStringList APMOSDComponent::setupCompleteChangedTriggerList() const
{
    // Params whose change should re-evaluate setupComplete().
    return {
        QStringLiteral("OSD1_ENABLE"),
        QStringLiteral("OSD2_ENABLE"),
        QStringLiteral("OSD3_ENABLE"),
        QStringLiteral("OSD4_ENABLE"),
    };
}

QUrl APMOSDComponent::setupSource() const
{
    return QUrl::fromUserInput(QStringLiteral(
        "qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMOSDComponent.qml"));
}

QUrl APMOSDComponent::summaryQmlSource() const
{
    return QUrl::fromUserInput(QStringLiteral(
        "qrc:/qml/QGroundControl/AutoPilotPlugins/APM/APMOSDComponentSummary.qml"));
}
