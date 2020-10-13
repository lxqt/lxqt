/*  This file is part of the KDE project
 *    Copyright (C) 2010 Lukas Tinkl <ltinkl@redhat.com>
 *    Copyright (C) 2015 Kai Uwe Broulik <kde@privat.broulik.de>
 * 
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Library General Public
 *    License version 2 as published by the Free Software Foundation.
 * 
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Library General Public License for more details.
 * 
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to
 *    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *    Boston, MA 02110-1301, USA.
 * 
 */

#ifndef XRANDRBRIGHTNESS_H
#define XRANDRBRIGHTNESS_H

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include <QScopedPointer>
#include <QList>

#include "monitorinfo.h"

template <typename T> using ScopedCPointer = QScopedPointer<T, QScopedPointerPodDeleter>;

class XRandrBrightness
{
public:
    XRandrBrightness();
    ~XRandrBrightness() = default;

    QList<MonitorInfo> getMonitorsInfo();
    void setMonitorsSettings(QList<MonitorInfo> monitors);

private:
    bool backlight_get_with_range(xcb_randr_output_t output, long &value, long &min, long &max) const;
    long backlight_get(xcb_randr_output_t output) const;
    void backlight_set(xcb_randr_output_t output, long value);
    float gamma_brightness_get(xcb_randr_output_t output);
    void gamma_brightness_set(xcb_randr_output_t output, float percent);

    xcb_atom_t m_backlight = XCB_ATOM_NONE;
    ScopedCPointer<xcb_randr_get_screen_resources_current_reply_t> m_resources;

};

#endif // XRANDRBRIGHTNESS_H

