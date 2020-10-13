/*
    Copyright (C) 2019  P.L. Lucas <selairi@gmail.com>

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

#ifndef _KSCREEN_UTILS_H_
#define _KSCREEN_UTILS_H_

#include <KScreen/Config>

class KScreenUtils {
public:
    /** Virtual screen size is updated.
     * If all monitors are allocated over a big rectangle, the size of this rectangle is the virtual screen size.
     */
    static void updateScreenSize(KScreen::ConfigPtr &config);
    
    /** This method applies config and it shows a TimeoutDialog to undo the changes.
     */
    static bool applyConfig(KScreen::ConfigPtr &config, KScreen::ConfigPtr &oldConfig);
    
    /** Sets the monitors aligned side by side.
     */
    static void extended(KScreen::ConfigPtr &config);
};

#endif // _KSCREEN_UTILS_H_
