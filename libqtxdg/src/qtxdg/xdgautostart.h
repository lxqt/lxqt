/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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

#ifndef QTXDG_XDGAUTOSTART_H
#define QTXDG_XDGAUTOSTART_H

#include "xdgmacros.h"
#include "xdgdesktopfile.h"

/*! @brief The XdgAutoStart class implements the "Desktop Application Autostart Specification"
 * from freedesktop.org.
 * This specification defines a method for automatically starting applications during the startup
 * of a desktop environment and after mounting a removable medium.
 * Now we impliment only startup.
 *
 * @sa http://standards.freedesktop.org/autostart-spec/autostart-spec-latest.html
 */
class QTXDG_API XdgAutoStart
{
public:
    /*! Returns a list of XdgDesktopFile objects for all the .desktop files in the Autostart directories
        When the .desktop file has the Hidden key set to true, the .desktop file must be ignored. But you
        can change this behavior by setting excludeHidden to false. */
    static XdgDesktopFileList desktopFileList(bool excludeHidden=true);

    /*! Returns a list of XdgDesktopFile objects for .desktop files in the specified Autostart directories
        When the .desktop file has the Hidden key set to true, the .desktop file must be ignored. But you
        can change this behavior by setting excludeHidden to false. */
    static XdgDesktopFileList desktopFileList(QStringList dirs, bool excludeHidden=true);

    /// For XdgDesktopFile returns the file path of the same name in users personal autostart directory.
    static QString localPath(const XdgDesktopFile& file);
};

#endif // XDGAUTOSTART_H
