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

#include "monitorinfo.h"

MonitorInfo::MonitorInfo(int id, QString name, long backlightMax)
    : mBacklightMax(backlightMax),
      mBacklight(-1),
      mBrightness(-1.0f),
      mName(name),
      mId(id)
{
}

MonitorInfo::MonitorInfo(const MonitorInfo &monitor)
{
    mId = monitor.mId;
    mName = monitor.mName;
    mBacklightMax = monitor.mBacklightMax;
    mBacklight = monitor.mBacklight;
    mBrightness = monitor.mBrightness;
}

bool MonitorInfo::isBacklightSupported() const
{
    return mBacklightMax > 0;
}

long MonitorInfo::backlightMax() const
{
    return mBacklightMax;
}

long MonitorInfo::backlight() const
{
    return mBacklight;
}

void MonitorInfo::setBacklight(const long value)
{
    mBacklight = value;
}

float MonitorInfo::brightness() const
{
    return mBrightness;
}

void MonitorInfo::setBrightness(const float percent)
{
    mBrightness = qMax(qMin((float)2.0, percent), (float)0.0);
}

int MonitorInfo::id() const
{
    return mId;
}

QString MonitorInfo::name() const
{
    return mName;
}

