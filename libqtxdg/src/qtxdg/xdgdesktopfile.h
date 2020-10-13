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

#ifndef QTXDG_XDGDESKTOPFILE_H
#define QTXDG_XDGDESKTOPFILE_H

#include "xdgmacros.h"

#include <QSharedDataPointer>
#include <QString>
#include <QVariant>
#include <QStringList>
#include <QtGui/QIcon>
#include <QSettings>

class XdgDesktopFileData;

/**
 \brief Desktop files handling.
 XdgDesktopFile class gives the interface for reading the values from the XDG .desktop file.
 The interface of this class is similar on QSettings. XdgDesktopFile objects can be passed
 around by value since the XdgDesktopFile class uses implicit data sharing.

 The Desktop Entry Specification defines 3 types of desktop entries: Application, Link and
 Directory. The format of .desktop file is described on
 http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html

 Note that not all methods in this class make sense for all types of desktop files.
 \author Alexander Sokoloff <sokoloff.a@gmail.com>
 */

class QTXDG_API XdgDesktopFile
{
public:
    /*! The Desktop Entry Specification defines 3 types of desktop entries: Application, Link and
        Directory. File type is determined by the "Type" tag. */
    enum Type
    {
        UnknownType,     //! Unknown desktop file type. Maybe is invalid.
        ApplicationType, //! The file describes application.
        LinkType,        //! The file describes URL.
        DirectoryType    //! The file describes directory settings.
    };

    //! Constructs an empty XdgDesktopFile
    XdgDesktopFile();

    /*! Constructs a copy of other.
        This operation takes constant time, because XdgDesktopFile is implicitly shared. This makes
        returning a XdgDesktopFile from a function very fast. If a shared instance is modified,
        it will be copied (copy-on-write), and that takes linear time. */
    XdgDesktopFile(const XdgDesktopFile& other);

    /*! Constructs a new basic DesktopFile. If type is:
        - ApplicationType, "value" should be the Exec value;
        - LinkType, "value" should be the URL;
        - DirectoryType, "value" should be omitted */
    XdgDesktopFile(XdgDesktopFile::Type type, const QString& name, const QString& value = QString());

    //! Destroys the object.
    virtual ~XdgDesktopFile();

    //! Assigns other to this DesktopFile and returns a reference to this DesktopFile.
    XdgDesktopFile& operator=(const XdgDesktopFile& other);

    //! Returns true if both files contain the identical key-value pairs
    bool operator==(const XdgDesktopFile &other) const;

    //! Returns false if both files contain the identical key-value pairs
    inline bool operator!=(const XdgDesktopFile &other) const
    {
        return !operator==(other);
    }

    //! Loads an DesktopFile from the file with the given fileName.
    virtual bool load(const QString& fileName);

    //! Saves the DesktopFile to the file with the given fileName. Returns true if successful; otherwise returns false.
    virtual bool save(const QString &fileName) const;

    /*! This is an overloaded function.
        This function writes a DesktopFile to the given device. */
    virtual bool save(QIODevice *device) const;

    /*! Returns the value for key. If the key doesn't exist, returns defaultValue.
        If no default value is specified, a default QVariant is returned. */
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /*! Returns the localized value for key. In the desktop file keys may be postfixed by [LOCALE]. If the
        localized value doesn't exist, returns non lokalized value. If non localized value doesn't exist, returns defaultValue.
        LOCALE must be of the form lang_COUNTRY.ENCODING@MODIFIER, where _COUNTRY, .ENCODING, and @MODIFIER may be omitted.

        If no default value is specified, a default QVariant is returned. */
    QVariant localizedValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

    //! Sets the value of setting key to value. If the key already exists, the previous value is overwritten.
    void setValue(const QString &key, const QVariant &value);

    /*! Sets the value of setting key to value. If a localized version of the key already exists, the previous value is
        overwritten. Otherwise, it overwrites the the un-localized version. */
    void setLocalizedValue(const QString &key, const QVariant &value);

    //! Removes the entry with the specified key, if it exists.
    void removeEntry(const QString& key);

    //! Returns the entry Categories. It supports X-Categories extensions.
    QStringList categories() const;

    //! Returns list of values in entry Actions.
    QStringList actions() const;

    //! Returns true if there exists a setting called key; returns false otherwise.
    bool contains(const QString& key) const;

    //! Returns true if the XdgDesktopFile is valid; otherwise returns false.
    bool isValid() const;

    /*! Returns the file name of the desktop file.
     *  Returns QString() if the file wasn't found when load was called. */
    QString fileName() const;

    //! Returns an icon specified in this file.
    QIcon const icon(const QIcon& fallback = QIcon()) const;
    //! Returns an icon for application action \param action.
    QIcon const actionIcon(const QString & action, const QIcon& fallback = QIcon()) const;

    //! Returns an icon name specified in this file.
    QString const iconName() const;
    //! Returns an icon name for application action \param action.
    QString const actionIconName(const QString & action) const;

    //! Returns an list of mimetypes specified in this file.
    /*! @return  Returns a list of the "MimeType=" entries.
     *  If the file doens't contain the MimeType entry, an empty QStringList is
     *  returned. Empty values are removed from the returned list.
     */
    QStringList mimeTypes() const;

    //! This function is provided for convenience. It's equivalent to calling localizedValue("Name").toString().
    QString name() const { return localizedValue(QLatin1String("Name")).toString(); }
    //! Returns an (localized) name for application action \param action.
    QString actionName(const QString & action) const;

    //! This function is provided for convenience. It's equivalent to calling localizedValue("Comment").toString().
    QString comment() const { return localizedValue(QLatin1String("Comment")).toString(); }

    /*! Returns the desktop file type.
        @see XdgDesktopFile::Type */
    Type type() const;

    /*! For file with Application type. Starts the program with the optional urls in a new process, and detaches from it.
        Returns true on success; otherwise returns false.
          @par urls - A list of files or URLS. Each file is passed as a separate argument to the executable program.

        For file with Link type. Opens URL in the associated application. Parametr urls is not used.

        For file with Directory type, do nothing.  */
    bool startDetached(const QStringList& urls) const;

    //! This function is provided for convenience. It's equivalent to calling startDetached(QStringList(url)).
    bool startDetached(const QString& url = QString()) const;

    /*! For file with Application type. Activates action defined by the \param action. Action is activated
     * either with the [Desktop Action %s]/Exec or by the D-Bus if the [Desktop Entry]/DBusActivatable is set.
     * \note Starting is done the same way as \sa startDetached()
     *
     * \return true on success; otherwise returns false.
     * \param urls - A list of files or URLS. Each file is passed as a separate argument to the executable program.
     *
     * For file with Link type, do nothing.
     *
     * For file with Directory type, do nothing.
    */
    bool actionActivate(const QString & action, const QStringList & urls) const;

    /*! A Exec value consists of an executable program optionally followed by one or more arguments.
        This function expands this arguments and returns command line string parts.
        Note this method make sense only for Application type.
        @par urls - A list of files or URLS. Each file is passed as a separate argument to the result string program.*/
    QStringList expandExecString(const QStringList& urls = QStringList()) const;

    /*! Returns the URL for the Link desktop file; otherwise an empty string is returned.  */
    QString url() const;

    /*! Computes the desktop file ID. It is the identifier of an installed
     *  desktop entry file.
     * @par fileName - The desktop file complete name.
     * @par checkFileExists If true and the file doesn't exist the computed ID
     * will be an empty QString(). Defaults to true.
     * @return The computed ID. Returns an empty QString() if it's impossible to
     * compute the ID. Reference:
     * https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#desktop-file-id
     */
    static QString id(const QString &fileName, bool checkFileExists = true);

    /*! The desktop entry specification defines a number of fields to control
        the visibility of the application menu. Thisfunction checks whether
        to display a this application or not.
        @par environment - User supplied desktop environment name. If not
            supplied the desktop will be detected reading the
            XDG_CURRENT_DESKTOP environment variable. If not set, "UNKNOWN"
            will be used as the desktop name. All operations envolving the
            desktop environment name are case insensitive.
    */
    bool isShown(const QString &environment = QString()) const;

    /*! This fuction returns true if the desktop file is applicable to the
        current environment.
        @par excludeHidden - if set to true (default), files with
            "Hidden=true" will be considered "not applicable". Setting this
            to false is be useful when the user wants to enable/disable items
            and wants to see those that are Hidden
        @par environment - User supplied desktop environment name. If not
            supplied the desktop will be detected reading the
            XDG_CURRENT_DESKTOP environment variable. If not set, "UNKNOWN"
            will be used as the desktop name. All operations envolving the
            desktop environment name are case insensitive.
    */
    bool isSuitable(bool excludeHidden = true, const QString &environment = QString()) const;

protected:
    virtual QString prefix() const { return QLatin1String("Desktop Entry"); }
    virtual bool check() const { return true; }
private:
    /*! Returns the localized version of the key if the Desktop File already contains a localized version of it.
        If not, returns the same key back */
    QString localizedKey(const QString& key) const;

    QSharedDataPointer<XdgDesktopFileData> d;
};


/// Synonym for QList<XdgDesktopFile>
typedef QList<XdgDesktopFile> XdgDesktopFileList;


class QTXDG_API QTXDG_DEPRECATED XdgDesktopFileCache
{
public:
    static XdgDesktopFile* getFile(const QString& fileName);
    static QList<XdgDesktopFile*> getAllFiles();
    static QList<XdgDesktopFile*> getApps(const QString & mimeType);
    static XdgDesktopFile* getDefaultApp(const QString& mimeType);
    static QSettings::Format desktopFileSettingsFormat();

    /*! Return all desktop apps that have category for their Categories key
     * Note that, according to xdg's spec, for non-standard categories "X-"
     * is added to the beginning of the category's name. This method takes care
     * of both cases.
     * See http://standards.freedesktop.org/menu-spec/menu-spec-latest.html#desktop-entry-extensions
     */
    static QList<XdgDesktopFile*> getAppsOfCategory(const QString &category);

private:
    static XdgDesktopFileCache & instance();
    static XdgDesktopFile* load(const QString & fileName);

    XdgDesktopFileCache();
    ~XdgDesktopFileCache();

    void initialize();
    void initialize(const QString & dirName);
    bool m_IsInitialized;
    QHash<QString, QList<XdgDesktopFile*> > m_defaultAppsCache;
    QHash<QString, XdgDesktopFile*> m_fileCache;
 };


#endif // QTXDG_XDGDESKTOPFILE_H
