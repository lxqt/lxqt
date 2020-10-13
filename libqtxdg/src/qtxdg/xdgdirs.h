/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
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

#ifndef QTXDG_XDGDIRS_H
#define QTXDG_XDGDIRS_H

#include "xdgmacros.h"
#include <QString>
#include <QStringList>

/*! @brief The XdgMenu class implements the "XDG Base Directory Specification" from freedesktop.org.
 * This specification defines where these files should be looked for by defining one or more base
 * directories relative to which files should be located.
 *
 * All postfix parameters should start with an '/' slash.
 *
 * @sa http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
 */

class QTXDG_API XdgDirs
{
public:
    enum UserDirectory
    {
        Desktop,
        Download,
        Templates,
        PublicShare,
        Documents,
        Music,
        Pictures,
        Videos
    };

    /*! @brief Returns the path to the user folder passed as parameter dir defined in
     * $XDG_CONFIG_HOME/user-dirs.dirs. Returns /tmp if no $HOME defined, $HOME/Desktop if
     * dir equals XdgDirs::Desktop or $HOME othewise.
     */
    static QString userDir(UserDirectory dir);


    /*! @brief Returns the default path to the user specified directory.
     *  Returns /tmp if no $HOME defined, $HOME/Desktop if dir equals
     *  XdgDirs::Desktop or $HOME othewise. If dir value is invalid, an empty
     *  QString is returned.
     */
    static QString userDirDefault(UserDirectory dir);

    /*! @brief Returns true if writting into configuration file $XDG_CONFIG_HOME/user-dirs.dirs
     * the path in value for the directory in dir is succesfull. Returns false otherwise. If
     * createDir is true, dir will be created if it doesn't exist.
     */
    static bool setUserDir(UserDirectory dir, const QString &value, bool createDir);

    /*! @brief Returns the path to the directory that corresponds to the $XDG_DATA_HOME.
     * If @i createDir is true, the function will create the directory.
     *
     * $XDG_DATA_HOME defines the base directory relative to which user specific data files
     * should be stored. If $XDG_DATA_HOME is either not set or empty, a default equal to
     * $HOME/.local/share should be used.
     */
    static QString dataHome(bool createDir=true);


    /*! @brief Returns the path to the directory that corresponds to the $XDG_CONFIG_HOME.
     * If @i createDir is true, the function will create the directory.
     *
     * $XDG_CONFIG_HOME defines the base directory relative to which user specific configuration
     * files should be stored. If $XDG_CONFIG_HOME is either not set or empty, a default equal
     * to $HOME/.config should be used.
     */
    static QString configHome(bool createDir=true);


    /*! @brief Returns a list of all directories that corresponds to the $XDG_DATA_DIRS.
     * $XDG_DATA_DIRS defines the preference-ordered set of base directories to search for data
     * files in addition to the $XDG_DATA_HOME base directory. If $XDG_DATA_DIRS is either not set
     * or empty, a value equal to /usr/local/share:/usr/share is used.
     *
     * If the postfix is not empty it will append to end of each returned directory.
     */
    static QStringList dataDirs(const QString &postfix = QString());


    /*! @brief Returns a list of all directories that corresponds to the $XDG_CONFIG_DIRS.
     * $XDG_CONFIG_DIRS defines the preference-ordered set of base directories to search for
     * configuration files in addition to the $XDG_CONFIG_HOME base directory. If $XDG_CONFIG_DIRS
     * is either not set or empty, a value equal to /etc/xdg should be used.
     *
     * If the postfix is not empty it will append to end of each returned directory.
     */
    static QStringList configDirs(const QString &postfix = QString());


    /*! @brief Returns the path to the directory that corresponds to the $XDG_CACHE_HOME.
     * If @i createDir is true, the function will create the directory.
     *
     * $XDG_CACHE_HOME defines the base directory relative to which user specific non-essential
     * data files should be stored. If $XDG_CACHE_HOME is either not set or empty,
     * a default equal to $HOME/.cache should be used.
     */
    static QString cacheHome(bool createDir=true);


    /*! @brief Returns the path to the directory that corresponds to the $XDG_RUNTIME_DIR.
     * $XDG_RUNTIME_DIR defines the base directory relative to which user-specific non-essential
     * runtime files and other file objects (such as sockets, named pipes, ...) should be stored.
     * The directory MUST be owned by the user, and he MUST be the only one having read and write
     * access to it. Its Unix access mode MUST be 0700.
     */
     static QString runtimeDir();

     /*! @brief Returns the path to the directory that corresponds to the $XDG_CONFIG_HOME/autostart
      *
      * If $XDG_CONFIG_HOME is not set, the Autostart Directory in the user's home directory is
      * ~/.config/autostart/
      */
     static QString autostartHome(bool createDir=true);

     /*! @brief Returns a list of all directories that correspond to $XDG_CONFIG_DIRS/autostart
      * If $XDG_CONFIG_DIRS is not set, the system wide Autostart Directory is /etc/xdg/autostart
      *
      * If the postfix is not empty it will append to end of each returned directory.
      *
      * Note: this does not include the user's autostart directory
      * @sa autostartHome()
      */
     static QStringList autostartDirs(const QString &postfix = QString());
};

#endif // QTXDG_XDGDIRS_H
