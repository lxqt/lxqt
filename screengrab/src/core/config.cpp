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

#include "src/core/config.h"
#include "core.h"

#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QVector>
#include <QDebug>

#define CONFIG_FILE_DIR "screengrab"
#define CONFIG_FILE_NAME "screengrab.conf"

#define KEY_SAVEDIR             "defDir"
#define KEY_SAVENAME            "defFilename"
#define KEY_SAVEFORMAT          "defImgFormat"
#define KEY_DELAY               "delay"
#define KEY_SCREENSHOT_TYPE_DEF "defScreenshotType"
#define KEY_IMG_QUALITY         "imageQuality"
#define KEY_FILENAMEDATE        "insDateTimeInFilename"
#define KEY_DATETIME_TPL        "templateDateTime"
#define KEY_FILENAME_TO_CLB     "CopyFilenameToClipboard"
#define KEY_AUTOSAVE            "autoSave"
#define KEY_AUTOSAVE_FIRST      "autoSaveFirst"

#define KEY_SHOW_TRAY           "showTrayIcon"
#define KEY_CLOSE_INTRAY        "closeInTray"
#define KEY_TRAYMESSAGES        "trayMessages"

#define KEY_WND_WIDTH           "windowWidth"
#define KEY_WND_HEIGHT          "windowHeight"
#define KEY_ZOOMBOX             "zoomAroundMouse"
#define KEY_TIME_NOTIFY         "timeTrayMessages"
#define KEY_ALLOW_COPIES        "AllowCopies"
#define KEY_ENABLE_EXT_VIEWER   "enbaleExternalView"
#define KEY_NODECOR             "noDecorations"
#define KEY_INCLUDE_CURSOR      "includeCursor"
#define KEY_FIT_INSIDE          "fitInside"

#define KEY_LAST_SELECTION          "lastSelection"


static const QLatin1String FullScreen("FullScreen");
static const QLatin1String Window("Window");
static const QLatin1String Area("Area");
static const QLatin1String PreviousSelection("PreviousSelection");

static QString screenshotTypeToString(int v);
static int screenshotTypeFromString(const QString& str);

static QString screenshotTypeToString(int v)
{
    QString r;

    switch(v) {
    case Core::FullScreen:
        r = FullScreen;
        break;
    case Core::Window:
        r = Window;
        break;
    case Core::Area:
        r = Area;
        break;
    case Core::PreviousSelection:
        r = PreviousSelection;
        break;
    default:
        r = FullScreen;
    }
    return r;
}

static int screenshotTypeFromString(const QString& str)
{
    int r;

    if (str == FullScreen)
        r = Core::FullScreen;
    else if (str == Window)
        r = Core::Window;
    else if (str == Area)
        r = Core::Area;
    else if (str == PreviousSelection)
        r = Core::PreviousSelection;
    else
        r = Core::FullScreen; // Default

    return r;
}

const static QStringList _imageFormats = {QStringLiteral("png"), QStringLiteral("jpg")};

Config* Config::ptrInstance = nullptr;

// constructor
Config::Config()
{
    _settings = new Settings(QStringLiteral("screengrab"), QStringLiteral("screengrab"));

    _shortcuts = new ShortcutManager(_settings);

    _scrNum = 0;
}

Config::~Config()
{
    delete _shortcuts;
    delete _settings;
}

Config* Config::instance()
{
    if (!ptrInstance)
        ptrInstance = new Config;

    return ptrInstance;
}

void Config::setValue(const QString &key, const QVariant &val)
{
    _confData[key] = val;
}

QVariant Config::value(const QString &key)
{
    return _confData[key];
}

void Config::killInstance()
{
    if (ptrInstance)
    {
        delete ptrInstance;
        ptrInstance = nullptr;
    }
}

QString Config::getConfigDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    dir += QDir::separator();
    dir += QStringLiteral(CONFIG_FILE_DIR);

    QDir qdir(dir);
    if (!qdir.exists())
        qdir.mkpath(dir);

    return dir;
}

// public methods

QString Config::getScrNumStr()
{
    QString str = QString::number(_scrNum);

    if (_scrNum < 10)
        str.prepend(QLatin1Char('0'));

    return str;
}

int Config::getScrNum() const
{
    return _scrNum;
}

void Config::increaseScrNum()
{
    _scrNum++;
}

void Config::resetScrNum()
{
    _scrNum = 0;
}

void Config::updateLastSaveDate()
{
    _dateLastSaving = QDateTime::currentDateTime();
}

QDateTime Config::getLastSaveDate() const
{
    return _dateLastSaving;
}

bool Config::getEnableExtView()
{
    return value(QLatin1String(KEY_ENABLE_EXT_VIEWER)).toBool();
}

void Config::setEnableExtView(bool val)
{
    setValue(QLatin1String(KEY_ENABLE_EXT_VIEWER), val);
}

bool Config::getIncludeCursor()
{
    return value(QLatin1String(KEY_INCLUDE_CURSOR)).toBool();
}

void Config::setIncludeCursor(bool val)
{
    setValue(QLatin1String(KEY_INCLUDE_CURSOR), val);
}

QString Config::getSaveDir()
{
    return value(QLatin1String(KEY_SAVEDIR)).toString();
}

void Config::setSaveDir(const QString &path)
{
    setValue(QLatin1String(KEY_SAVEDIR), path);
}

QString Config::getSaveFileName()
{
    return value(QLatin1String(KEY_SAVENAME)).toString();
}

void Config::setSaveFileName(const QString &fileName)
{
    setValue(QLatin1String(KEY_SAVENAME), fileName);
}

QString Config::getSaveFormat()
{
    return value(QLatin1String(KEY_SAVEFORMAT)).toString().toLower();
}

void Config::setSaveFormat(const QString &format)
{
    setValue(QLatin1String(KEY_SAVEFORMAT), format);
}

quint8 Config::getDelay()
{
    return value(QLatin1String(KEY_DELAY)).toInt();
}

void Config::setDelay(quint8 sec)
{
    setValue(QLatin1String(KEY_DELAY), sec);
}

int Config::getDefScreenshotType()
{
    return (value(QLatin1String(KEY_SCREENSHOT_TYPE_DEF)).toInt());
}

void Config::setDefScreenshotType(const int type)
{
    setValue(QLatin1String(KEY_SCREENSHOT_TYPE_DEF), type);
}

quint8 Config::getAutoCopyFilenameOnSaving()
{
    return value(QLatin1String(KEY_FILENAME_TO_CLB)).toInt();
}

void Config::setAutoCopyFilenameOnSaving(quint8 val)
{
    setValue(QLatin1String(KEY_FILENAME_TO_CLB), val);
}


quint8 Config::getTrayMessages()
{
    return value(QLatin1String(KEY_TRAYMESSAGES)).toInt();
}

void Config::setTrayMessages(quint8 type)
{
    setValue(QLatin1String(KEY_TRAYMESSAGES), type);
}

bool Config::getAllowMultipleInstance()
{
    return value(QLatin1String(KEY_ALLOW_COPIES)).toBool();
}

void Config::setAllowMultipleInstance(bool val)
{
    setValue(QLatin1String(KEY_ALLOW_COPIES), val);
}

bool Config::getCloseInTray()
{
    return value(QLatin1String(KEY_CLOSE_INTRAY)).toBool();
}

void Config::setCloseInTray(bool val)
{
    setValue(QLatin1String(KEY_CLOSE_INTRAY), val);
}

quint8 Config::getTimeTrayMess()
{
    return value(QLatin1String(KEY_TIME_NOTIFY)).toInt();
}

void Config::setTimeTrayMess(int sec)
{
    setValue(QLatin1String(KEY_TIME_NOTIFY), sec);
}

QSize Config::getRestoredWndSize()
{
    QSize wndSize(value(QLatin1String(KEY_WND_WIDTH)).toInt(), value(QLatin1String(KEY_WND_HEIGHT)).toInt());
    return wndSize;
}

void Config::setRestoredWndSize(int w, int h)
{
    setValue(QLatin1String(KEY_WND_WIDTH), w);
    setValue(QLatin1String(KEY_WND_HEIGHT), h);
}

bool Config::getDateTimeInFilename()
{
    return value(QLatin1String(KEY_FILENAMEDATE)).toBool();
}

void Config::setDateTimeInFilename(bool val)
{
    setValue(QLatin1String(KEY_FILENAMEDATE), val);
}

bool Config::getAutoSave()
{
    return value(QLatin1String(KEY_AUTOSAVE)).toBool();
}

void Config::setAutoSave(bool val)
{
    setValue(QLatin1String(KEY_AUTOSAVE), val);
}

quint8 Config::getImageQuality()
{
    return value(QLatin1String(KEY_IMG_QUALITY)).toInt();
}

void Config::setImageQuality(quint8 qualuty)
{
    setValue(QLatin1String(KEY_IMG_QUALITY), qualuty);
}

bool Config::getAutoSaveFirst()
{
    return value(QLatin1String(KEY_AUTOSAVE_FIRST)).toBool();
}

void Config::setAutoSaveFirst(bool val)
{
    setValue(QLatin1String(KEY_AUTOSAVE_FIRST), val);
}

QString Config::getDateTimeTpl()
{
    return value(QLatin1String(KEY_DATETIME_TPL)).toString();
}

void Config::setDateTimeTpl(const QString &tpl)
{
    setValue(QLatin1String(KEY_DATETIME_TPL), tpl);
}

bool Config::getZoomAroundMouse()
{
    return value(QLatin1String(KEY_ZOOMBOX)).toBool();
}

void Config::setZoomAroundMouse(bool val)
{
    setValue(QLatin1String(KEY_ZOOMBOX), val);
}

bool Config::getShowTrayIcon()
{
    return value(QLatin1String(KEY_SHOW_TRAY)).toBool();
}

void Config::setShowTrayIcon(bool val)
{
    setValue(QLatin1String(KEY_SHOW_TRAY), val);
}

bool Config::getNoDecoration()
{
    return value(QLatin1String(KEY_NODECOR)).toBool();
}

void Config::setNoDecoration(bool val)
{
    setValue(QLatin1String(KEY_NODECOR), val);
}

bool Config::getFitInside()
{
    return value(QLatin1String(KEY_FIT_INSIDE)).toBool();
}

void Config::setFitInside(bool val)
{
    setValue(QLatin1String(KEY_FIT_INSIDE), val);
}

QRect Config::getLastSelection()
{
    return value(QLatin1String(KEY_LAST_SELECTION)).toRect();
}

void Config::setLastSelection(QRect rect)
{
    setValue(QLatin1String(KEY_LAST_SELECTION), rect);
}

void Config::saveWndSize()
{
    // saving size
    _settings->beginGroup(QStringLiteral("Display"));
    _settings->setValue(QLatin1String(KEY_WND_WIDTH), getRestoredWndSize().width());
    _settings->setValue(QLatin1String(KEY_WND_HEIGHT), getRestoredWndSize().height());
    _settings->endGroup();
}

// load all settings  from conf file
void Config::loadSettings()
{
    _settings->beginGroup(QStringLiteral("Base"));
    setSaveDir(_settings->value(QLatin1String(KEY_SAVEDIR), getDirNameDefault()).toString() );
    setSaveFileName(_settings->value(QLatin1String(KEY_SAVENAME),DEF_SAVE_NAME).toString());
    setSaveFormat(_settings->value(QLatin1String(KEY_SAVEFORMAT), DEF_SAVE_FORMAT).toString());
    setDelay(_settings->value(QLatin1String(KEY_DELAY), DEF_DELAY).toInt());
    setDefScreenshotType(screenshotTypeFromString(_settings->value(QLatin1String(KEY_SCREENSHOT_TYPE_DEF)).toString()));
    setAutoCopyFilenameOnSaving(_settings->value(QLatin1String(KEY_FILENAME_TO_CLB), DEF_FILENAME_TO_CLB).toInt());
    setDateTimeInFilename(_settings->value(QLatin1String(KEY_FILENAMEDATE), DEF_DATETIME_FILENAME).toBool());
    setDateTimeTpl(_settings->value(QLatin1String(KEY_DATETIME_TPL), DEF_DATETIME_TPL).toString());
    setAutoSave(_settings->value(QLatin1String(KEY_AUTOSAVE), DEF_AUTO_SAVE).toBool());
    setAutoSaveFirst(_settings->value(QLatin1String(KEY_AUTOSAVE_FIRST), DEF_AUTO_SAVE_FIRST).toBool());
    setNoDecoration(_settings->value(QLatin1String(KEY_NODECOR), DEF_X11_NODECOR).toBool());
    setImageQuality(_settings->value(QLatin1String(KEY_IMG_QUALITY), DEF_IMG_QUALITY).toInt());
    setIncludeCursor(_settings->value(QLatin1String(KEY_INCLUDE_CURSOR), DEF_INCLUDE_CURSOR).toBool());
    _settings->endGroup();

    _settings->beginGroup(QStringLiteral("Display"));
    setTrayMessages(_settings->value(QLatin1String(KEY_TRAYMESSAGES), DEF_TRAY_MESS_TYPE).toInt());
    setTimeTrayMess(_settings->value(QLatin1String(KEY_TIME_NOTIFY), DEF_TIME_TRAY_MESS).toInt( ));
    setZoomAroundMouse(_settings->value(QLatin1String(KEY_ZOOMBOX), DEF_ZOOM_AROUND_MOUSE).toBool());
    setLastSelection(_settings->value(QLatin1String(KEY_LAST_SELECTION)).toRect());
    // TODO - make set windows size without hardcode values
    setRestoredWndSize(_settings->value(QLatin1String(KEY_WND_WIDTH), DEF_WND_WIDTH).toInt(),
                       _settings->value(QLatin1String(KEY_WND_HEIGHT), DEF_WND_HEIGHT).toInt());
    setShowTrayIcon(_settings->value(QLatin1String(KEY_SHOW_TRAY), DEF_SHOW_TRAY).toBool());
    _settings->endGroup();

    _settings->beginGroup(QStringLiteral("System"));
    setCloseInTray(_settings->value(QLatin1String(KEY_CLOSE_INTRAY), DEF_CLOSE_IN_TRAY).toBool());
    setAllowMultipleInstance(_settings->value(QLatin1String(KEY_ALLOW_COPIES), DEF_ALLOW_COPIES).toBool());
    setEnableExtView(_settings->value(QLatin1String(KEY_ENABLE_EXT_VIEWER), DEF_ENABLE_EXT_VIEWER).toBool());
    setFitInside(_settings->value(QLatin1String(KEY_FIT_INSIDE), DEF_FIT_INSIDE).toBool());
    _settings->endGroup();

    _shortcuts->loadSettings();
}

void Config::saveSettings()
{ // save settings except for those on the main window
    _settings->beginGroup(QStringLiteral("Base"));
    _settings->setValue(QLatin1String(KEY_SAVEDIR), getSaveDir());
    _settings->setValue(QLatin1String(KEY_SAVENAME), getSaveFileName());
    _settings->setValue(QLatin1String(KEY_SAVEFORMAT), getSaveFormat());
    _settings->setValue(QLatin1String(KEY_FILENAME_TO_CLB), getAutoCopyFilenameOnSaving());
    _settings->setValue(QLatin1String(KEY_FILENAMEDATE), getDateTimeInFilename());
    _settings->setValue(QLatin1String(KEY_DATETIME_TPL), getDateTimeTpl());
    _settings->setValue(QLatin1String(KEY_AUTOSAVE), getAutoSave());
    _settings->setValue(QLatin1String(KEY_AUTOSAVE_FIRST), getAutoSaveFirst());
    _settings->setValue(QLatin1String(KEY_IMG_QUALITY), getImageQuality());
    _settings->endGroup();

    _settings->beginGroup(QStringLiteral("Display"));
    _settings->setValue(QLatin1String(KEY_TRAYMESSAGES), getTrayMessages());
    _settings->setValue(QLatin1String(KEY_TIME_NOTIFY), getTimeTrayMess());
    _settings->setValue(QLatin1String(KEY_SHOW_TRAY), getShowTrayIcon());
    _settings->endGroup();
    saveWndSize();

    _settings->beginGroup(QStringLiteral("System"));
    _settings->setValue(QLatin1String(KEY_CLOSE_INTRAY), getCloseInTray());
    _settings->setValue(QLatin1String(KEY_ALLOW_COPIES), getAllowMultipleInstance());
    _settings->setValue(QLatin1String(KEY_ENABLE_EXT_VIEWER), getEnableExtView());
    _settings->setValue(QLatin1String(KEY_FIT_INSIDE), getFitInside());
    _settings->endGroup();

    _shortcuts->saveSettings();

    resetScrNum();
}

void Config::saveScreenshotSettings()
{ // save the main window settings
    _settings->beginGroup(QStringLiteral("Base"));
    _settings->setValue(QLatin1String(KEY_SCREENSHOT_TYPE_DEF), screenshotTypeToString(getDefScreenshotType()));
    _settings->setValue(QLatin1String(KEY_NODECOR), getNoDecoration());
    _settings->setValue(QLatin1String(KEY_INCLUDE_CURSOR), getIncludeCursor());
    _settings->setValue(QLatin1String(KEY_DELAY), getDelay());
    _settings->endGroup();

    _settings->beginGroup(QStringLiteral("Display"));
    _settings->setValue(QLatin1String(KEY_ZOOMBOX), getZoomAroundMouse());
    _settings->setValue(QLatin1String(KEY_LAST_SELECTION), getLastSelection());
    _settings->endGroup();
}

// set default values
void Config::setDefaultSettings()
{
    setSaveDir(getDirNameDefault());
    setSaveFileName(DEF_SAVE_NAME);
    setSaveFormat(DEF_SAVE_FORMAT);
    setImageQuality(DEF_IMG_QUALITY);
    setDateTimeInFilename(DEF_DATETIME_FILENAME);
    setDateTimeTpl(DEF_DATETIME_TPL);
    setAutoCopyFilenameOnSaving(DEF_FILENAME_TO_CLB);
    setAutoSave(DEF_AUTO_SAVE);
    setAutoSaveFirst(DEF_AUTO_SAVE_FIRST);
    setTrayMessages(DEF_TRAY_MESS_TYPE);
    setIncludeCursor(DEF_INCLUDE_CURSOR);
    setCloseInTray(DEF_CLOSE_IN_TRAY);
    setTimeTrayMess(DEF_TIME_TRAY_MESS);
    setAllowMultipleInstance(DEF_ALLOW_COPIES);
    // TODO - make set windows size without hardcode values
    // setRestoredWndSize(DEF_WND_WIDTH, DEF_WND_HEIGHT);
    setShowTrayIcon(DEF_SHOW_TRAY);
    setEnableExtView(DEF_ENABLE_EXT_VIEWER);
    setFitInside(DEF_FIT_INSIDE);

    _shortcuts->setDefaultSettings();

    quint8 countModules = Core::instance()->modules()->count();
    for (int i = 0; i < countModules; ++i)
        Core::instance()->modules()->getModule(i)->defaultSettings();
}

// get defaukt directory path
QString Config::getDirNameDefault()
{
    return QDir::homePath()+QDir::separator();
}

QStringList Config::getFormatIDs() const
{
    return _imageFormats;
}
// get id of default save format
int Config::getDefaultFormatID()
{
    return _imageFormats.indexOf(getSaveFormat());
}

QString Config::getSysLang()
{
    QByteArray lang = qgetenv("LC_ALL");

    if (lang.isEmpty())
        lang = qgetenv("LC_MESSAGES");

    if (lang.isEmpty())
        lang = qgetenv("LANG");

    if (!lang.isEmpty())
        return QLocale (QString::fromLocal8Bit(lang)).name();
    else
        return  QLocale::system().name();
}

ShortcutManager* Config::shortcuts()
{
    return _shortcuts;
}
