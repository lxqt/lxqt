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


#ifndef LXQTSETTINGS_H
#define LXQTSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QSharedDataPointer>
#include "lxqtglobals.h"

class QEvent;

namespace LXQt
{

class SettingsPrivate;
class GlobalSettings;

/*! \brief User settings handling */
class LXQT_API Settings : public QSettings
{
    Q_OBJECT
public:

    /*! \brief Constructs a Settings object for accessing settings of the module called module, and with parent parent.
    Settings can be accessed using the standard interface provided by QSettings, but it also provides some convenience functions and signals.

    \param module a base name of the config file. It's a name without
            the extension. For example: if you want to open settings for
            panel create it as Settings("panel").
            The function will create all parent directories necessary to create
            the file.
    \param parent It's no need to delete this class manually if it's set.
    */
    explicit Settings(const QString& module, QObject* parent = nullptr);
    //explicit Settings(QObject* parent=0);
    explicit Settings(const QSettings* parentSettings, const QString& subGroup, QObject* parent=nullptr);
    explicit Settings(const QSettings& parentSettings, const QString& subGroup, QObject* parent=nullptr);
    Settings(const QString &fileName, QSettings::Format format, QObject *parent = nullptr);
    ~Settings() override;

    static const GlobalSettings *globalSettings();


    /*! Returns the localized value for key. In the desktop file keys may be postfixed by [LOCALE]. If the
        localized value doesn't exist, returns non lokalized value. If non localized value doesn't exist, returns defaultValue.
        LOCALE must be of the form lang_COUNTRY.ENCODING@MODIFIER, where _COUNTRY, .ENCODING, and @MODIFIER may be omitted.

        If no default value is specified, a default QVariant is returned. */
    QVariant localizedValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /*! Sets the value of setting key to value. If a localized version of the key already exists, the previous value is
        overwritten. Otherwise, it overwrites the the un-localized version. */
    void setLocalizedValue(const QString &key, const QVariant &value);

Q_SIGNALS:
    /*! /brief signal for backward compatibility (emitted whenever settingsChangedFromExternal() or settingsChangedByApp() is emitted)
     */
    void settingsChanged();
    /*! /brief signal emitted when the settings file is changed by external application
     */
    void settingsChangedFromExternal();
    /*! /brief signal emitted when any setting is changed by this object
     */
    void settingsChangedByApp();

protected:
    bool event(QEvent *event) override;

protected Q_SLOTS:
    /*! Called when the config file is changed */
    virtual void fileChanged();

private Q_SLOTS:
    void _fileChanged(QString path);

private:
    void addWatchedFile(QString const & path);

private:
    Q_DISABLE_COPY(Settings)

    SettingsPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(Settings)
};


class LXQtThemeData;

/*! \brief QSS theme handling */
class LXQT_API LXQtTheme
{
public:
    /// Constructs a null theme.
    LXQtTheme();

    /*! Constructs an theme from the dir with the given path. If path not absolute
        (i.e. the theme name only) the relevant dir must be found relative to the
        $XDG_DATA_HOME + $XDG_DATA_DIRS directories. */
    LXQtTheme(const QString &path);

    /// Constructs a copy of other. This is very fast.
    LXQtTheme(const LXQtTheme &other);

    LXQtTheme& operator=(const LXQtTheme &other);
    ~LXQtTheme();

    /// Returns the name of the theme.
    QString name() const;

    QString path() const;

    QString previewImage() const;

    /// Returns true if this theme is valid; otherwise returns false.
    bool isValid() const;

    /*! \brief Returns StyleSheet text (not file name, but real text) of the module called module.
        Paths in url() C/QSS functions are parsed to be real values for the theme,
        relative to full path
    */
    QString qss(const QString& module) const;

    /*! \brief A full path to image used as a wallpaper
     \param screen is an ID of the screen like in Qt. -1 means default (any) screen.
                   Any other value greater than -1 is the exact screen (in dualhead).
                  In themes the index starts from 1 (ix 1 means 1st screen).
     \retval QString a file name (including path).
    */
    QString desktopBackground(int screen=-1) const;

    /// Returns the current lxqt theme.
    static const LXQtTheme &currentTheme();

    /// Returns the all themes found in the system.
    static QList<LXQtTheme> allThemes();

private:
    static LXQtTheme* mInstance;
    QSharedDataPointer<LXQtThemeData> d;
};

/*!
A global pointer referring to the unique LXQtTheme object.
It is equivalent to the pointer returned by the LXQtTheme::instance() function.
Only one theme object can be created. !*/

#define lxqtTheme LXQtTheme::currentTheme()


class LXQT_API SettingsCache
{
public:
    explicit SettingsCache(QSettings &settings);
    explicit SettingsCache(QSettings *settings);
    virtual ~SettingsCache() {}

    void loadFromSettings();
    void loadToSettings();

private:
    QSettings &mSettings;
    QHash<QString, QVariant> mCache;
};

class GlobalSettingsPrivate;

class GlobalSettings: public Settings
{
    Q_OBJECT
public:
    GlobalSettings();
    ~GlobalSettings() override;

Q_SIGNALS:
    /// Signal emitted when the icon theme has changed.
    void iconThemeChanged();

    /// Signal emitted when the lxqt theme has changed.
    void lxqtThemeChanged();

protected Q_SLOTS:
    void fileChanged() override;

private:
    GlobalSettingsPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(GlobalSettings)
};

} // namespace LXQt
#endif // LXQTSETTINGS_H
