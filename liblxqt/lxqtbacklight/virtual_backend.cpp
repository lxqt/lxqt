/*
 * Copyright (C) 2018  P.L. Lucas
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "virtual_backend.h"

namespace LXQt {

VirtualBackEnd::VirtualBackEnd(QObject *parent):QObject(parent)
{
}

bool VirtualBackEnd::isBacklightAvailable()
{
    return false;
}

bool VirtualBackEnd::isBacklightOff()
{
    return false;
}

void VirtualBackEnd::setBacklight(int /*value*/)
{
}

int VirtualBackEnd::getBacklight()
{
    return -1;
}

int VirtualBackEnd::getMaxBacklight()
{
    return -1;
}

} // namespace LXQt