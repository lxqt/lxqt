/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2018 LXQt team
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

#ifndef CONFIGOTHERTOOLKITS_H
#define CONFIGOTHERTOOLKITS_H

#include <QWidget>
#include <QProcess>
#include <QTemporaryFile>
#include <LXQt/Settings>

class ConfigOtherToolKits : public QObject
{
    Q_OBJECT

public:
    ConfigOtherToolKits(LXQt::Settings *settings, LXQt::Settings *configAppearanceSettings, QObject *parent = 0);
    ~ConfigOtherToolKits();
    QStringList getGTKThemes(QString version);
    QString getGTKThemeFromRCFile(QString version);
    QString getGTKConfigPath(QString version);
    bool backupGTKSettings(QString version);
    QString getDefaultGTKTheme();

public slots:
    void setConfig();
    void setXSettingsConfig();
    void setGTKConfig(QString version, QString theme = QString());

private:
    struct Config {
        QString iconTheme;
        QString styleTheme;
        QString fontName;
        QString toolButtonStyle;
        int buttonStyle;
    } mConfig;
    void writeConfig(QString path, const char *configString);
    QString getConfig(const char *configString);
    void updateConfigFromSettings();

    LXQt::Settings *mSettings;
    LXQt::Settings *mConfigAppearanceSettings;

    QProcess mXsettingsdProc;
    QTemporaryFile tempFile;
};

#endif // CONFIGOTHERTOOLKITS_H
