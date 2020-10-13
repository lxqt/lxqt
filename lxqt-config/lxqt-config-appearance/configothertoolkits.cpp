/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
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

#include "configothertoolkits.h"
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QMetaEnum>
#include <QToolBar>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QDateTime>
#include <QMessageBox>
#include <QProcess>

#include <sys/types.h>
#include <signal.h>

static const char *GTK2_CONFIG = R"GTK2_CONFIG(
# Created by lxqt-config-appearance (DO NOT EDIT!)
gtk-theme-name = "%1"
gtk-icon-theme-name = "%2"
gtk-font-name = "%3"
gtk-button-images = %4
gtk-menu-images = %4
gtk-toolbar-style = %5
gtk-cursor-theme-name = %6
)GTK2_CONFIG";

static const char *GTK3_CONFIG = R"GTK3_CONFIG(
# Created by lxqt-config-appearance (DO NOT EDIT!)
[Settings]
gtk-theme-name = %1
gtk-icon-theme-name = %2
# GTK3 ignores bold or italic attributes.
gtk-font-name = %3
gtk-menu-images = %4
gtk-button-images = %4
gtk-toolbar-style = %5
gtk-cursor-theme-name = %6
)GTK3_CONFIG";

static const char *XSETTINGS_CONFIG = R"XSETTINGS_CONFIG(
# Created by lxqt-config-appearance (DO NOT EDIT!)
Net/IconThemeName "%2"
Net/ThemeName "%1"
Gtk/FontName "%3"
Gtk/MenuImages %4
Gtk/ButtonImages %4
Gtk/ToolbarStyle "%5"
Gtk/CursorThemeName "%6"
)XSETTINGS_CONFIG";

ConfigOtherToolKits::ConfigOtherToolKits(LXQt::Settings *settings,  LXQt::Settings *configAppearanceSettings, QObject *parent) : QObject(parent)
{
    mSettings = settings;
    mConfigAppearanceSettings = configAppearanceSettings;
    if(tempFile.open()) {
        mXsettingsdProc.setProcessChannelMode(QProcess::ForwardedChannels);
        mXsettingsdProc.start(QStringLiteral("xsettingsd"), QStringList() << QStringLiteral("-c") << tempFile.fileName());
        if(!mXsettingsdProc.waitForStarted())
            return;
        tempFile.close();
    }
}

ConfigOtherToolKits::~ConfigOtherToolKits()
{
    mXsettingsdProc.close();
}

static QString get_environment_var(const char *envvar, const char *defaultValue)
{
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString mDirPath = QString::fromLocal8Bit(qgetenv(envvar));
    if(mDirPath.isEmpty())
        mDirPath = homeDir + QLatin1String(defaultValue);
    else {
        const auto paths = mDirPath.split(QLatin1Char(':'));
        for(const QString &path : paths) {
            mDirPath = path;
            break;
        }
    }
    return mDirPath;
}

static QString _get_config_path(QString path)
{
    QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    path.replace(QLatin1String("$XDG_CONFIG_HOME"), get_environment_var("XDG_CONFIG_HOME", "/.config"));
    path.replace(QLatin1String("$GTK2_RC_FILES"),   get_environment_var("GTK2_RC_FILES", "/.gtkrc-2.0")); // If $GTK2_RC_FILES is undefined, "~/.gtkrc-2.0" will be used.
    path.replace(QLatin1String("~"), homeDir);
    return path;
}

QString ConfigOtherToolKits::getGTKConfigPath(QString version)
{
    if(version == QLatin1String("2.0"))
        return _get_config_path(QStringLiteral("$GTK2_RC_FILES"));
    return _get_config_path(QStringLiteral("$XDG_CONFIG_HOME/gtk-%1/settings.ini").arg(version));
}

static bool grep(QFile &file, QByteArray text)
{
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    while (!file.atEnd()) {
        QByteArray line = file.readLine().trimmed();
        if(line.startsWith(text)) {
            return true;
        }
    }
    file.close();
    return false;
}

bool ConfigOtherToolKits::backupGTKSettings(QString version)
{
        QString gtkrcPath = getGTKConfigPath(version);
        QFile file(gtkrcPath);
        if(file.exists() && !grep(file, "# Created by lxqt-config-appearance (DO NOT EDIT!)")) {
            QString backupPath = gtkrcPath + QStringLiteral("-") + QString::number(QDateTime::currentSecsSinceEpoch()) + QStringLiteral("~");
            file.copy(backupPath);
            QMessageBox::warning(nullptr, tr("GTK themes"),
                tr("<p>'%1' has been overwritten.</p><p>You can find a copy of your old settings in '%2'</p>")
                .arg(getGTKConfigPath(version), backupPath), QMessageBox::Ok);
            return true;
        }
        return false;
}

void ConfigOtherToolKits::setConfig()
{
    if(!mConfigAppearanceSettings->contains(QStringLiteral("ControlGTKThemeEnabled")))
        mConfigAppearanceSettings->setValue(QStringLiteral("ControlGTKThemeEnabled"), false);
    bool controlGTKThemeEnabled = mConfigAppearanceSettings->value(QStringLiteral("ControlGTKThemeEnabled")).toBool();
    if(! controlGTKThemeEnabled)
        return;
    updateConfigFromSettings();
    mConfig.styleTheme = getGTKThemeFromRCFile(QStringLiteral("3.0"));
    setGTKConfig(QStringLiteral("3.0"));
    mConfig.styleTheme = getGTKThemeFromRCFile(QStringLiteral("2.0"));
    setGTKConfig(QStringLiteral("2.0"));
    setXSettingsConfig();
}

void ConfigOtherToolKits::setXSettingsConfig()
{
    // setGTKConfig is called before calling setXSettingsConfig,
    // then updateConfigFromSettings is not required.
    //updateConfigFromSettings();
    //mConfig.styleTheme = getGTKThemeFromRCFile(version);

    // Reload settings. xsettingsd must be installed.
    // xsettingsd settings are written to stdin.
    if(QProcess::Running == mXsettingsdProc.state()) {
        QFile file(tempFile.fileName());
        if(file.open(QIODevice::WriteOnly)) {
            file.write( getConfig(XSETTINGS_CONFIG).toLocal8Bit() );
            file.flush();
            file.close();
        }
        int pid = mXsettingsdProc.processId();
        kill(pid, SIGHUP);
    }
}

void ConfigOtherToolKits::setGTKConfig(QString version, QString theme)
{
    updateConfigFromSettings();
    if(!theme.isEmpty())
        mConfig.styleTheme = theme;
    backupGTKSettings(version);
    QString gtkrcPath = getGTKConfigPath(version);
    if(version == QLatin1String("2.0"))
        writeConfig(gtkrcPath, GTK2_CONFIG);
    else
        writeConfig(gtkrcPath, GTK3_CONFIG);
}

QString ConfigOtherToolKits::getConfig(const char *configString)
{
    LXQt::Settings* sessionSettings = new LXQt::Settings(QStringLiteral("session"));
    QString mouseStyle = sessionSettings->value(QStringLiteral("Mouse/cursor_theme")).toString();
    delete sessionSettings;
    return QString::fromUtf8(configString).arg(mConfig.styleTheme, mConfig.iconTheme,
        mConfig.fontName, mConfig.buttonStyle==0 ? QStringLiteral("0"):QStringLiteral("1"),
        mConfig.toolButtonStyle, mouseStyle
        );
}

void ConfigOtherToolKits::writeConfig(QString path, const char *configString)
{
    path = _get_config_path(path);

    QFile file(path);
    if(! file.exists()) {
        QFileInfo fileInfo(file);
        QDir::home().mkpath(fileInfo.path());
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    QTextStream out(&file);
    out << getConfig(configString);
    out.flush();
    file.close();
}

QStringList ConfigOtherToolKits::getGTKThemes(QString version)
{
    QStringList themeList;
    QString configFile = version==QLatin1String("2.0") ? QStringLiteral("gtkrc") : QStringLiteral("gtk.css");
    
    if(version != QLatin1String("2.0")) {
        // Insert default GTK3 themes:
        themeList << QStringLiteral("Adwaita") << QStringLiteral("HighContrast") << QStringLiteral("HighContrastInverse");
    }

    const QStringList dataPaths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for(const QString &dataPath : dataPaths) {
        QDir themesPath(dataPath + QStringLiteral("/themes"));
        const QStringList themes = themesPath.entryList(QDir::Dirs);
        for(const QString &theme : themes) {
            QDir dirsInTheme(QStringLiteral("%1/themes/%2").arg(dataPath, theme));
            const QStringList dirs = dirsInTheme.entryList(QDir::Dirs);
            for(const QString &dir : dirs) {
                if(dir.startsWith(QLatin1String("gtk-"))) {
                    if(!version.endsWith(QLatin1String("*")) && dir != QStringLiteral("gtk-%1").arg(version))
                         continue;
                    QFileInfo themePath(QStringLiteral("%1/themes/%2/%3/%4").arg(dataPath, theme, dir, configFile));
                    if(themePath.exists() && !themeList.contains(theme))
                         themeList.append(theme);
                }
            }
        }
    }
    return themeList;
}

QString ConfigOtherToolKits::getGTKThemeFromRCFile(QString version)
{
    if(version == QLatin1String("2.0")) {
        QString gtkrcPath = _get_config_path(QStringLiteral("$GTK2_RC_FILES"));
        QFile file(gtkrcPath);
        if(file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return getDefaultGTKTheme();
            while (!file.atEnd()) {
                QByteArray line = file.readLine().trimmed();
                if(line.startsWith("gtk-theme-name")) {
                    QList<QByteArray> parts = line.split('=');
                    if(parts.size()>=2) {
                        file.close();
                        return QString::fromUtf8(parts[1].replace('"', "").trimmed());
                    }
                }
            }
            file.close();
        }
    } else {
        QString gtkrcPath = _get_config_path(QStringLiteral("$XDG_CONFIG_HOME/gtk-%1/settings.ini").arg(version));
        QFile file(gtkrcPath);
        if(file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return getDefaultGTKTheme();
            bool settingsFound = false;
            while (!file.atEnd()) {
                QByteArray line = file.readLine().trimmed();
                if(line.startsWith("[Settings]"))
                    settingsFound = true;
                else if(line.startsWith("[") && line.endsWith("]"))
                    settingsFound = false;
                else if(settingsFound && line.startsWith("gtk-theme-name")) {
                    QList<QByteArray> parts = line.split('=');
                    if(parts.size()>=2) {
                        file.close();
                        return QString::fromUtf8(parts[1].trimmed());
                    }
                }
            }
            file.close();
        }
    }
    return getDefaultGTKTheme();
}

QString ConfigOtherToolKits::getDefaultGTKTheme()
{
    // Get the GTK default theme. Command line:
    // $ gsettings get org.gnome.desktop.interface gtk-theme
    QProcess gsettings;
    QStringList args;
    args << QStringLiteral("get") << QStringLiteral("org.gnome.desktop.interface") << QStringLiteral("gtk-theme");
    gsettings.start(QStringLiteral("gsettings"), args);
    if(! gsettings.waitForFinished())
        return QString();
    QByteArray defaultTheme = gsettings.readAll().trimmed();
    gsettings.close();
    if(defaultTheme.size() <= 1)
        return QString();
    // The theme has got quotation marks. Remove it:
    defaultTheme.replace("'","");
    return QString::fromUtf8(defaultTheme);
}

void ConfigOtherToolKits::updateConfigFromSettings()
{
    mSettings->beginGroup(QLatin1String("Qt"));
    QFont font;
    font.fromString(mSettings->value(QStringLiteral("font")).toString());
    // Font name from: https://developer.gnome.org/pango/stable/pango-Fonts.html#pango-font-description-from-string
    // FAMILY-LIST [SIZE]", where FAMILY-LIST is a comma separated list of families optionally terminated by a comma,
    // STYLE_OPTIONS is a whitespace separated list of words where each word describes one of style, variant, weight, stretch, or gravity, and
    // SIZE is a decimal number (size in points) or optionally followed by the unit modifier "px" for absolute size.
    mConfig.fontName = QStringLiteral("%1%2%3 %4")
        .arg(font.family(),                                 //%1
        font.style()==QFont::StyleNormal?QString():QStringLiteral(" Italic"), //%2
        font.weight()==QFont::Normal?QString():QStringLiteral(" Bold"),       //%3
        QString::number(font.pointSize()));                             //%4
    mSettings->endGroup();

    mConfig.iconTheme = mSettings->value(QStringLiteral("icon_theme")).toString();
    {
        // Tool button style
        QByteArray tb_style = mSettings->value(QStringLiteral("tool_button_style")).toByteArray();
        // convert toolbar style name to value
        QMetaEnum me = QToolBar::staticMetaObject.property(QToolBar::staticMetaObject.indexOfProperty("toolButtonStyle")).enumerator();
        int val = me.keyToValue(tb_style.constData());
        mConfig.buttonStyle = 1;
        switch(val) {
            case Qt::ToolButtonIconOnly:
                mConfig.toolButtonStyle = QLatin1String("GTK_TOOLBAR_ICONS");
                break;
            case Qt::ToolButtonTextOnly:
                mConfig.toolButtonStyle = QLatin1String("GTK_TOOLBAR_TEXT");
                mConfig.buttonStyle = 0;
                break;
            case Qt::ToolButtonTextUnderIcon:
                mConfig.toolButtonStyle = QLatin1String("GTK_TOOLBAR_BOTH");
                break;
            default:
                mConfig.toolButtonStyle = QLatin1String("GTK_TOOLBAR_BOTH_HORIZ");
        }
    }
}

