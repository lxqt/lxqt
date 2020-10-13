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


#include <QX11Info>
#include <QDebug>

#include "xrandrbrightness.h"

XRandrBrightness::XRandrBrightness()
{
    if (!QX11Info::isPlatformX11()) {
        return;
    }
    ScopedCPointer<xcb_randr_query_version_reply_t> versionReply(xcb_randr_query_version_reply(QX11Info::connection(),
        xcb_randr_query_version(QX11Info::connection(), 1, 2),
    nullptr));

    if (!versionReply) {
        qDebug() << "RandR Query version returned null";
        return;
    }

    if (versionReply->major_version < 1 || (versionReply->major_version == 1 && versionReply->minor_version < 2)) {
        qDebug() << "RandR version" << versionReply->major_version << "." << versionReply->minor_version << " too old";
        return;
    }
    ScopedCPointer<xcb_intern_atom_reply_t> backlightReply(xcb_intern_atom_reply(QX11Info::connection(),
        xcb_intern_atom (QX11Info::connection(), 1, strlen("Backlight"), "Backlight"),
    nullptr));

    if (!backlightReply) {
        qDebug() << "Intern Atom for Backlight returned null";
        return;
    }

    m_backlight = backlightReply->atom;

    if (m_backlight == XCB_NONE) {
        qDebug() << "No outputs have backlight property";
        //return;
    }
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(QX11Info::connection()));
    if (!iter.rem) {
        qDebug() << "XCB Screen Roots Iterator rem was null";
        return;
    }

    xcb_screen_t *screen = iter.data;
    xcb_window_t root = screen->root;

    m_resources.reset(xcb_randr_get_screen_resources_current_reply(QX11Info::connection(),
        xcb_randr_get_screen_resources_current(QX11Info::connection(), root)
    , nullptr));

    if (!m_resources) {
        qDebug() << "RANDR Get Screen Resources returned null";
        return;
    }
}



bool XRandrBrightness::backlight_get_with_range(xcb_randr_output_t output, long &value, long &min, long &max) const {
    long cur = backlight_get(output);
    if (cur == -1) {
       return false;
    }

    ScopedCPointer<xcb_randr_query_output_property_reply_t> propertyReply(xcb_randr_query_output_property_reply(QX11Info::connection(),
        xcb_randr_query_output_property(QX11Info::connection(), output, m_backlight)
    , nullptr));

    if (!propertyReply) {
        return false;
    }

    if (propertyReply->range && xcb_randr_query_output_property_valid_values_length(propertyReply.data()) == 2) {
        int32_t *values = xcb_randr_query_output_property_valid_values(propertyReply.data());
        value = cur;
        min = values[0];
        max = values[1];
        return true;
    }

    return false;
}

long XRandrBrightness::backlight_get(xcb_randr_output_t output) const
{
    ScopedCPointer<xcb_randr_get_output_property_reply_t> propertyReply;
    long value;

    if (m_backlight != XCB_ATOM_NONE) {
        propertyReply.reset(xcb_randr_get_output_property_reply(QX11Info::connection(),
            xcb_randr_get_output_property(QX11Info::connection(), output, m_backlight, XCB_ATOM_NONE, 0, 4, 0, 0)
        , nullptr));

        if (!propertyReply) {
            return -1;
        }
    }

    if (!propertyReply || propertyReply->type != XCB_ATOM_INTEGER || propertyReply->num_items != 1 || propertyReply->format != 32) {
        value = -1;
    } else {
        value = *(reinterpret_cast<long *>(xcb_randr_get_output_property_data(propertyReply.data())));
    }
    return value;
}

void XRandrBrightness::backlight_set(xcb_randr_output_t output, long value)
{
    xcb_randr_change_output_property(QX11Info::connection(), output, m_backlight, XCB_ATOM_INTEGER,
                                     32, XCB_PROP_MODE_REPLACE,
                                     1, reinterpret_cast<unsigned char *>(&value));
}


float XRandrBrightness::gamma_brightness_get(xcb_randr_output_t output)
{
    xcb_generic_error_t  *error;

    xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info (QX11Info::connection(), output, 0);
    ScopedCPointer<xcb_randr_get_output_info_reply_t> output_info(xcb_randr_get_output_info_reply (QX11Info::connection(), output_info_cookie, &error));
    if(error != NULL)
    {
        qDebug() << "Error getting output_info";
        return -1;
    }
    if(output_info == NULL)
    {
        qDebug() << "Error: output_info is null";
        return -1;
    }
    // xcb_randr_get_output_info_reply_t tiene como elemento crtc
    xcb_randr_get_crtc_gamma_cookie_t gamma_cookie = xcb_randr_get_crtc_gamma_unchecked (QX11Info::connection(), output_info->crtc);
    ScopedCPointer<xcb_randr_get_crtc_gamma_reply_t> gamma_reply(xcb_randr_get_crtc_gamma_reply (QX11Info::connection(), gamma_cookie, &error));
    if(error != NULL)
    {
        qDebug() << "Error getting gamma_reply";
        return -1;
    }
    if(gamma_reply == NULL)
    {
        qDebug() << "Error: gamma_reply is null";
        return -1;
    }
    uint16_t * red = xcb_randr_get_crtc_gamma_red (gamma_reply.data());
    if(red == NULL)
    {
        qDebug() << "Error: red is null";
        return -1;
    }
    int red_length = xcb_randr_get_crtc_gamma_red_length(gamma_reply.data());

    // uint16_t *green = xcb_randr_get_crtc_gamma_green (gamma_reply);
    // if(green == NULL)
    // {
    //     qDebug() << "Error: green is null";
    //     return -1;
    // }
    // uint16_t *blue = xcb_randr_get_crtc_gamma_blue (gamma_reply);
    // if(blue == NULL)
    // {
    //     qDebug() << "Error: blue is null";
    //     return -1;
    // }

    float brightness = (float)red[red_length-1]/65535.0;
    return brightness;
}

void XRandrBrightness::gamma_brightness_set(xcb_randr_output_t output, float percent)
{
    xcb_generic_error_t  *error;

    xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info (QX11Info::connection(), output, 0);
    ScopedCPointer<xcb_randr_get_output_info_reply_t> output_info(xcb_randr_get_output_info_reply (QX11Info::connection(), output_info_cookie, &error));
    if(error != NULL)
    {
        qDebug() << "Error getting output_info";
        return;
    }
    if(output_info == NULL)
    {
        qDebug() << "Error: output_info is null";
        return;
    }
    // xcb_randr_get_output_info_reply_t tiene como elemento crtc
    xcb_randr_get_crtc_gamma_cookie_t gamma_cookie = xcb_randr_get_crtc_gamma_unchecked (QX11Info::connection(), output_info->crtc);
    ScopedCPointer<xcb_randr_get_crtc_gamma_reply_t> gamma_reply(xcb_randr_get_crtc_gamma_reply (QX11Info::connection(), gamma_cookie, &error));
    if(error != NULL)
    {
        qDebug() << "Error getting gamma_reply";
        return;
    }
    if(gamma_reply == NULL)
    {
        qDebug() << "Error: gamma_reply is null";
        return;
    }
    uint16_t *red = xcb_randr_get_crtc_gamma_red (gamma_reply.data());
    if(red == NULL)
    {
        qDebug() << "Error: red is null";
        return;
    }
    int red_length = xcb_randr_get_crtc_gamma_red_length(gamma_reply.data());
    uint16_t *green = xcb_randr_get_crtc_gamma_green (gamma_reply.data());
    if(green == NULL)
    {
        qDebug() << "Error: green is null";
        return;
    }
    uint16_t *blue = xcb_randr_get_crtc_gamma_blue (gamma_reply.data());
    if(blue == NULL)
    {
        qDebug() << "Error: blue is null";
        return;
    }

    float max_gamma = 65535*percent;
    for(int i=0;i<red_length;i++)
    {
        int value = qMin((int)(((float)i/(float)(red_length-1))*max_gamma),65535);
        green[i] = blue[i] = red[i] = value;
    }
    xcb_randr_set_crtc_gamma (QX11Info::connection(), output_info->crtc, red_length, red, green, blue);
}


QList<MonitorInfo> XRandrBrightness::getMonitorsInfo()
{
    QList<MonitorInfo> monitors;

    if (!m_resources) {
        return monitors;
    }

    auto *outputs = xcb_randr_get_screen_resources_current_outputs(m_resources.data());
    for (int i = 0; i < m_resources->num_outputs; ++i) {
        xcb_randr_output_t output = outputs[i];

        xcb_generic_error_t  *error;

        xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info (QX11Info::connection(), output, 0);
        ScopedCPointer <xcb_randr_get_output_info_reply_t> output_info(xcb_randr_get_output_info_reply (QX11Info::connection(), output_info_cookie, &error));
        if(error != NULL)
        {
            qDebug() << "Error getting output_info";
            continue;
        }
        if(output_info == NULL)
        {
            qDebug() << "Error: output_info is null";
            continue;
        }

        QString name = QString::fromUtf8((const char *) xcb_randr_get_output_info_name(output_info.data()), output_info->name_len);


        qDebug() << "Found output:" << name;


        // Is connected?
        if ( (xcb_randr_connection_t)(output_info->connection) != XCB_RANDR_CONNECTION_CONNECTED )
        {
            qDebug() << "Output is not connected";
            continue; // This output is not connected. Check other
        }

        // Is enabled?
        if( output_info->crtc == 0)
        {
            qDebug() << "Crtc is not null. Output not enabled.";
            continue;
        }
        xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info_unchecked (QX11Info::connection(), output_info->crtc, 0);
        ScopedCPointer<xcb_randr_get_crtc_info_reply_t> crtc_info(xcb_randr_get_crtc_info_reply (QX11Info::connection(), crtc_info_cookie, &error));
        if(error != NULL)
        {
            qDebug() << "Error getting output_info";
            continue;
        }
        if(crtc_info == NULL)
        {
            qDebug() << "Error: output_info is null";
            continue;
        }
        if( crtc_info->mode == XCB_NONE )
        {
            qDebug() << "No modes. Output not enabled.";
            continue;
        }

        // Output is connected and enabled. Get data:
        bool backlightIsSuported = false;
        long cur, min, max, backlight_max = -1;
        if (backlight_get(output) != -1)
        {
            if (backlight_get_with_range(output, cur, min, max))
            {
                backlightIsSuported = true;
                backlight_max = max - min;
            }
        }

        MonitorInfo monitor((int)output, name, backlight_max);

        if(backlightIsSuported)
            monitor.setBacklight(cur-min);

        monitor.setBrightness(gamma_brightness_get(output));

        qDebug() << "Output:" << name << "added";
        monitors.append(monitor);

    }

    return monitors;
}

void XRandrBrightness::setMonitorsSettings(QList<MonitorInfo> monitors)
{
    if (!m_resources) {
        return;
    }

    auto *outputs = xcb_randr_get_screen_resources_current_outputs(m_resources.data());
    for (int i = 0; i < m_resources->num_outputs; ++i) {
        xcb_randr_output_t output = outputs[i];

        xcb_generic_error_t  *error;

        xcb_randr_get_output_info_cookie_t output_info_cookie = xcb_randr_get_output_info (QX11Info::connection(), output, 0);
        ScopedCPointer<xcb_randr_get_output_info_reply_t> output_info(xcb_randr_get_output_info_reply (QX11Info::connection(), output_info_cookie, &error));
        if(error != NULL)
        {
            qDebug() << "Error getting output_info";
            continue;
        }
        if(output_info == NULL)
        {
            qDebug() << "Error: output_info is null";
            continue;
        }

        // Is connected?
        if ( (xcb_randr_connection_t)(output_info->connection) != XCB_RANDR_CONNECTION_CONNECTED )
            continue; // This output is not connected. Check other

        // Is enabled?
        if( output_info->crtc == 0)
            continue;
        xcb_randr_get_crtc_info_cookie_t crtc_info_cookie = xcb_randr_get_crtc_info_unchecked (QX11Info::connection(), output_info->crtc, 0);
        ScopedCPointer<xcb_randr_get_crtc_info_reply_t> crtc_info(xcb_randr_get_crtc_info_reply (QX11Info::connection(), crtc_info_cookie, &error));
        if(error != NULL)
            continue;
        if(crtc_info == NULL && crtc_info->mode == XCB_NONE )
            continue;

        QString name = QString::fromUtf8((const char *) xcb_randr_get_output_info_name(output_info.data()), output_info->name_len);

        // Output is connected and enabled. Get data:
        bool backlightIsSuported = false;
        long cur, min, max, backlight_value = 0;
        if (backlight_get(output) != -1)
        {
            if (backlight_get_with_range(output, cur, min, max))
            {
                backlightIsSuported = true;
                backlight_value = cur - min;
            }
        }
        float brightness_value = gamma_brightness_get(output);

        // Compare output info with settings and set it.
        for(const MonitorInfo &monitor: monitors)
        {
            //qDebug() << "[XRandrBrightness::setMonitorsSettings]" << monitor.id() << (int)output << monitor.name() << name ;
            if(monitor.id() == (int)output && monitor.name() == name)
            {
                // Set settings
                if(backlightIsSuported && monitor.backlight() != backlight_value)
                    backlight_set(output, min+monitor.backlight());
                if(monitor.brightness() != brightness_value)
                    gamma_brightness_set(output, monitor.brightness());
                break;
            }
        }
    }
}

