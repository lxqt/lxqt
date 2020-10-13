/*
    Copyright (C) 2016  P.L. Lucas <selairi@gmail.com>
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __MONITOR_INFO_H__
#define __MONITOR_INFO_H__

#include <QString>


/** This class represents backlight and brightness values of screen.
 * If backlight is supported, backlight power of screen can be changed.
 * Brightness represents color saturation.
 */
class MonitorInfo
{
public:
    MonitorInfo(int id, QString name, long backlightMax);
    MonitorInfo(const MonitorInfo &monitor);

    bool isBacklightSupported() const;
    long backlight() const;
    long backlightMax() const;
    void setBacklight(const long value);

    float brightness() const;
    /**Brightness is a number between 0 and 2.*/
    void setBrightness(const float percent);

    int id() const;
    QString name() const;

private:
    long mBacklightMax;
    long mBacklight;

    float mBrightness;

    QString mName;
    int mId;
};

#endif

