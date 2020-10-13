/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright (C) 2012  Alec Moskvin <alecm@gmx.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef AUTOSTARTITEM_H
#define AUTOSTARTITEM_H

#include <LXQt/AutostartEntry>

/*! \brief The AutostartItem class provides an interface for staging configuration of individual
autostart items. All changes are made in memory until commit() is called.

 - "system" file refers to a read-only file in /etc/xdg/autostart (or a directory in $XDG_CONFIG_DIRS)
 - "local" file refers to the file in user's ~/.config/autostart (or in $XDG_CONFIG_HOME/autostart)

When a "local" file has the same name as the "system" file, the local one overrides it. This class
tries to ensure that the "local" file is deleted if it's the same as the "system" file.
*/

class AutostartItem : public LXQt::AutostartEntry
{
public:
    //! \brief Default constructor
    AutostartItem();

    /*! Creates an autostart item with a "system" file
     * \param systemFile
     */
    AutostartItem(const XdgDesktopFile& systemFile);

    //! Returns the "system" file
    const XdgDesktopFile& systemfile() const { return mSystemFile; }

    /*! Sets the local version that exists on disk
     * \param file the desktop file, must already exist in user's autostart directory
     */
    void setLocalFromFile(const XdgDesktopFile &file);

    /*! Removes the "local" version of the file
     * \return true if there is no "system" version left (i.e. the item was entirely deleted)
     */
    bool removeLocal() { return LXQt::AutostartEntry::removeLocal(); }

    //! Returns true if both the "local" and "system" versions exist
    bool overrides() const { return mSystem && isLocal(); }

    //! Returns true if the local version exists
    bool isLocal() const { return LXQt::AutostartEntry::isLocal(); }

    //! Returns true if the local file does not exist on disk
    bool isTransient() const { return mLocalState == StateTransient; }

    //! Creates a mapping of file name (a unique ID) to AutostartItem for the running system
    static QMap<QString, AutostartItem> createItemMap();
};

#endif // AUTOSTARTITEM_H
