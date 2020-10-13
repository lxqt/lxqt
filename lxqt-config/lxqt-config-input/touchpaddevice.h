/*
    Copyright (C) 2016-2018 Chih-Hsuan Yen <yan12125@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef TOUCHPADDEVICE_H
#define TOUCHPADDEVICE_H

#include <QList>
#include <QVariant>

namespace LXQt {
    class Settings;
}

enum ScrollingMethod
{
    NONE = 0,
    TWO_FINGER = 1,
    EDGE = 2,
    BUTTON = 4
};

const char TAPPING_ENABLED[] = "tappingEnabled";
const char NATURAL_SCROLLING_ENABLED[] = "naturalScrollingEnabled";
const char TAP_TO_DRAG_ENABLED[] = "tapToDragEnabled";
const char SCROLLING_METHOD_ENABLED[] = "scrollingMethodEnabled";
const char ACCELERATION_SPEED[] = "accelSpeed";

class TouchpadDevice
{
public:
    TouchpadDevice(): deviceid(0) {}

    static QList<TouchpadDevice> enumerate_from_udev();

    const QString& name() const { return m_name; }
    QString escapedName() const;

    int tappingEnabled() const;
    int naturalScrollingEnabled() const;
    int tapToDragEnabled() const;
    float accelSpeed() const;
    bool setTappingEnabled(bool enabled) const;
    bool setNaturalScrollingEnabled(bool enabled) const;
    bool setTapToDragEnabled(bool enabled) const;
    bool setAccelSpeed(float speed) const;
    bool oldTappingEnabled() const { return m_oldTappingEnabled; }
    bool oldNaturalScrollingEnabled() const { return m_oldNaturalScrollingEnabled; }
    bool oldTapToDragEnabled() const { return m_oldTapToDragEnabled; }
    ScrollingMethod oldScrollingMethodEnabled() const { return m_oldScrollingMethodEnabled; }
    float oldAccelSpeed() const { return m_oldAccelSpeed; }

    int scrollMethodsAvailable() const;
    ScrollingMethod scrollingMethodEnabled() const;
    bool setScrollingMethodEnabled(ScrollingMethod method) const;

    static void loadSettings(LXQt::Settings* settings);
    void saveSettings(LXQt::Settings* settings) const;
private:
    QString m_name;
    QString devnode;
    int deviceid;

    bool m_oldTappingEnabled;
    bool m_oldNaturalScrollingEnabled;
    bool m_oldTapToDragEnabled;
    float m_oldAccelSpeed;
    ScrollingMethod m_oldScrollingMethodEnabled;

    QList<QVariant> get_xi2_property(const char* prop) const;
    bool set_xi2_property(const char* prop, QList<QVariant> values) const;
    bool find_xi2_device();
    int featureEnabled(const char* prop) const;
};

#endif
