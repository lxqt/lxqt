/***************************************************************************
 *   Copyright (C) 2009 - 2013 by Artem 'DOOMer' Galichkin                 *
 *   doomer3d@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include "shortcutmanager.h"

#include <QSettings>
#include <QString>
#include <QSize>
#include <QHash>
#include <QVariant>
#include <QDateTime>

//  default values const
const QString DEF_SAVE_NAME = QStringLiteral("screen");
const QString DEF_SAVE_FORMAT = QStringLiteral("png");
const quint8 DEF_DELAY = 0;
const bool DEF_X11_NODECOR = false;
const quint8 DEF_TRAY_MESS_TYPE = 1;
const quint8 DEF_FILENAME_TO_CLB = 0;
const quint8 DEF_IMG_QUALITY = 80;
const bool DEF_CLOSE_IN_TRAY = false;
const bool DEF_ALLOW_COPIES = true;
const bool DEF_ZOOM_AROUND_MOUSE = false;
// TODO - make set windows size without hardcode values
const int DEF_WND_WIDTH = 480;
const int DEF_WND_HEIGHT = 281;
const int DEF_TIME_TRAY_MESS = 5;
const bool DEF_DATETIME_FILENAME = false;
const bool DEF_AUTO_SAVE = false;
const bool DEF_AUTO_SAVE_FIRST = false;
const QString DEF_DATETIME_TPL = QStringLiteral("yyyy-MM-dd-hh-mm-ss");
const bool DEF_SHOW_TRAY = true;
const bool DEF_ENABLE_EXT_VIEWER = true;
const bool DEF_INCLUDE_CURSOR = false;
const bool DEF_FIT_INSIDE = true;

class Settings : public QSettings // prevents redundant writings
{
    Q_OBJECT
public:
    Settings(const QString &organization, const QString &application = QString(), QObject *parent = nullptr)
        : QSettings (organization, application, parent) {}

    void setValue(const QString &key, const QVariant &v) {
        if (value(key) == v)
            return;
        QSettings::setValue(key, v);
    }
};

// class worker with conf data
class Config
{
public:
    // type of shortcut
    enum Type {
        globalShortcut = 0,
        localShortcut = 1
    };

    Q_DECLARE_FLAGS(ShortcutType, Type)

    // definition of shortcut
    enum Actions {
        shortcutFullScreen = 0,
        shortcutActiveWnd = 1,
        shortcutAreaSelect = 2,
        shortcutNew = 3,
        shortcutSave = 4,
        shortcutCopy = 5,
        shortcutOptions = 6,
        shortcutHelp = 7,
        shortcutClose = 8
    };

    Q_DECLARE_FLAGS(ShortcutAction, Actions)

    enum AutoCopyFilename {
        nameToClipboardOff = 0,
        nameToClipboardFile = 1,
        nameToClipboardPath = 2
    };

    Q_DECLARE_FLAGS(FilenameToClipboard, AutoCopyFilename)

    /**
     * Get current instance of configuration object
     * @return Pointer on created object
     */
    static Config* instance();

    /**
     * Destroy current Config object
     */
    static void killInstance();
    ~Config();

    /**
     * @brief Gets the directory where to save the configuration files.
     * Does not end with '/'.
     */
    static QString getConfigDir();

    /**
     * Load configuration data from conf file
     */
    void loadSettings();

    /**
     * Save configuration data to conf file
     */
    void saveSettings();

    /**
     * Save screenshot settings to conf file
     */
    void saveScreenshotSettings();

    /**
     * Reset configuration data from default values
     */
    void setDefaultSettings();

    // save dir
    QString getSaveDir();
    void setSaveDir(const QString &path);

    // filename default
    QString getSaveFileName();
    void setSaveFileName(const QString &fileName);

    // save format str
    QString getSaveFormat();
    void setSaveFormat(const QString &format);

    quint8 getDelay();
    void setDelay(quint8 sec);

    // configured default screenshot type
    int getDefScreenshotType();
    void setDefScreenshotType(const int type);

    quint8 getAutoCopyFilenameOnSaving();
    void setAutoCopyFilenameOnSaving(quint8 val);

    // trayMessages
    quint8 getTrayMessages();
    void setTrayMessages(quint8 type);

    // allow multiple copies
    bool getAllowMultipleInstance();
    void setAllowMultipleInstance(bool val);

    // closing in tray
    bool getCloseInTray();
    void setCloseInTray(bool val);

    // tume of tray messages
    quint8 getTimeTrayMess();
    void setTimeTrayMess(int src);

    bool getDateTimeInFilename();
    void setDateTimeInFilename(bool val);

    // auto save screenshot
    bool getAutoSave();
    void setAutoSave(bool val);

    // umage qualuty
    quint8 getImageQuality();
    void setImageQuality(quint8 qualuty);

    // aoutosave first screenshot
    bool getAutoSaveFirst();
    void setAutoSaveFirst(bool val);

    // size wnd
    QSize getRestoredWndSize();
    void setRestoredWndSize(int w, int h);
    void saveWndSize();

    // get image save format(s)
    QStringList getFormatIDs() const;
    int getDefaultFormatID();
    QString getDirNameDefault();

    // datetime template
    QString getDateTimeTpl();
    void setDateTimeTpl(const QString &tpl);

    // zoom aroundd mouse
    bool getZoomAroundMouse();
    void setZoomAroundMouse(bool val);

    // show tray icon option
    bool getShowTrayIcon();
    void setShowTrayIcon(bool val);

    // no decoration of window
    bool getNoDecoration();
    void setNoDecoration(bool val);

    QString getScrNumStr();
    int getScrNum() const;
    void increaseScrNum();
    void resetScrNum();

    void updateLastSaveDate();
    QDateTime getLastSaveDate() const;

    bool getEnableExtView();
    void setEnableExtView(bool val);

    bool getIncludeCursor();
    void setIncludeCursor(bool val);

    bool getFitInside();
    void setFitInside(bool val);

    QRect getLastSelection();
    void setLastSelection(QRect rect);

    static QString getSysLang();

    ShortcutManager* shortcuts();

private:
    Config();
    Config(const Config &);
    Config& operator=(const Config& );

    static Config *ptrInstance;

    /**
     * Return value on configuration parameter
     *
     * @param String of name key
     * @return QVariant value of configuration parameter
     */
    QVariant value(const QString &key);

    /**
     * Set value on configuration parameter
     *
     * @param String of name key
     * @param String of saved value
     */
    void setValue(const QString& key, const QVariant &val);

    Settings *_settings;
    QHash<QString, QVariant> _confData;

    ShortcutManager *_shortcuts;

    int _scrNum; // screen num in session
    QDateTime _dateLastSaving;
};

#endif // CONFIG_H
