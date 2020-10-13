/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
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

#ifndef LXQTAUTOSTARTENTRY_H
#define LXQTAUTOSTARTENTRY_H

#include "lxqtglobals.h"
#include <XdgDesktopFile>

namespace LXQt
{

/*! \brief The AutostartEntry class provides an interface for staging configuration of individual
autostart items. All changes are made in memory until commit() is called.

 - "system" file refers to a read-only file in /etc/xdg/autostart (or a directory in $XDG_CONFIG_DIRS)
 - "local" file refers to the file in user's ~/.config/autostart (or in $XDG_CONFIG_HOME/autostart)

When a "local" file has the same name as the "system" file, the local one overrides it. This class
tries to ensure that the "local" file is deleted if it's identical to the "system" file.
*/
class LXQT_API AutostartEntry
{
public:
    /*! Constructs an AutostartEntry object for a specific entry.
     * \param name The name of the autostart desktop file (e.g. "lxqt-panel.desktop")
     */
    AutostartEntry(const QString& name);

    //! \brief Default constructor
    AutostartEntry();

    //! Destructor
    virtual ~AutostartEntry() { }

    //! Returns the "active" desktop file
    const XdgDesktopFile& file() const;

    //! Returns the name of the autostart entry (e.g. "lxqt-panel.desktop")
    QString name() const;

    /*! Sets to the specified desktop file. Use this to make modifications.
     * \param file The desktop file
     */
    void setFile(const XdgDesktopFile& file);

    /*! Sets whether the item auto-starts
     * \param enable When false, sets the "Hidden" key which will prevent the item from starting
     */
    void setEnabled(bool enable);

    //! Returns true if the item will auto-start
    bool isEnabled() const;

    /*! Returns true if the entry does not exist, and the object carries no useful information
     *  and can be ignored/deleted.
     */
    bool isEmpty() const { return !mSystem && mLocalState == StateNone; }

    /*! Write any changes to disk
     * \return true on success
     */
    bool commit();

protected:
    //! Returns true if the user's "local" version exists
    bool isLocal() const { return mLocalState != StateNone && mLocalState != StateDeleted; }

    /*! Removes the user's "local" version of the file, reverting to system defaults
     * \return true if there is no "system" version left (i.e. the entry was entirely deleted)
     */
    bool removeLocal();

    //! \brief a read-only file in /etc/xdg/autostart (or a directory in $XDG_CONFIG_DIRS)
    XdgDesktopFile mSystemFile;

    //! \brief the file in user's ~/.config/autostart (or in $XDG_CONFIG_HOME/autostart)
    XdgDesktopFile mLocalFile;

    //! State of the "local" file
    enum ItemState
    {
        StateNone,      //! does not exist at all
        StateDeleted,   //! needs to be deleted from disk
        StateTransient, //! does not yet exist on disk
        StateModified,  //! exists on disk and is modified
        StateExists     //! exists on disk and unmodified
    } mLocalState;
    bool mSystem;       //! true if the "system" file exists
};

} // namespace LXQt
#endif // LXQTAUTOSTARTENTRY_H
