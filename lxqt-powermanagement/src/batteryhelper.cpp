/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
 *   Christian Surlykke <christian@surlykke.dk>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include <QList>
#include <QDebug>
#include <QMap>
#include <QDateTime>
#include <cmath>

#include "batteryhelper.h"
#include "../config/powermanagementsettings.h"

QString BatteryHelper::stateToString(Solid::Battery::ChargeState state)
{
    switch (state)
    {
    case Solid::Battery::NoCharge:
        return tr("Empty");
    case Solid::Battery::Discharging:
        return tr("Discharging");
    case Solid::Battery::FullyCharged:
        return tr("Fully charged");
    case Solid::Battery::Charging:
    default:
        return tr("Charging");
    }
}

QString BatteryHelper::technologyToString(Solid::Battery::Technology tech)
{
    switch (tech)
    {
    case Solid::Battery::LithiumIon:
        return tr("Lithium ion");
    case Solid::Battery::LithiumPolymer:
        return tr("Lithium polymer");
    case Solid::Battery::LithiumIronPhosphate:
        return tr("Lithium iron phosphate");
    case Solid::Battery::LeadAcid:
        return tr("Lead acid");
    case Solid::Battery::NickelCadmium:
        return tr("Nickel cadmium");
    case Solid::Battery::NickelMetalHydride:
        return tr("Nickel metal hydride");
    case Solid::Battery::UnknownTechnology:
    default:
        return tr("Unknown");
    }
}

QString BatteryHelper::typeToString(Solid::Battery::BatteryType type)
{
    switch (type)
    {
        case Solid::Battery::PdaBattery:
            return tr("Personal Digital Assistant's battery");
        case Solid::Battery::UpsBattery:
            return tr("Uninterruptible Power Supply's battery");
        case Solid::Battery::PrimaryBattery:
            return tr("Primary battery");
        case Solid::Battery::MouseBattery:
            return tr("Mouse battery");
        case Solid::Battery::KeyboardBattery:
            return tr("Keyboard battery");
        case Solid::Battery::KeyboardMouseBattery:
            return tr("Keyboard and mouse's battery");
        case Solid::Battery::CameraBattery:
            return tr("Camera battery");
        case Solid::Battery::PhoneBattery:
            return tr("Phone battery");
        case Solid::Battery::MonitorBattery:
            return tr("Monitor battery");
        case Solid::Battery::UnknownBattery:
        default:
            return tr("Unknown battery");
    }
}
