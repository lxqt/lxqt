/*
    Copyright (C) 2014  P.L. Lucas <selairi@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <LXQt/SingleApplication>
#include <LXQt/ConfigDialog>
#include <LXQt/Settings>
#include <QDebug>
#include <QProcess>
#include <QStandardPaths>
#include <QCommandLineParser>
#include "monitorsettingsdialog.h"
#include <QCoreApplication>
#include "loadsettings.h"

static bool loadSettingsOk(int argc, char** argv)
{
    for(int i=0; i<argc; i++) {
        if(QString::fromUtf8(argv[i]) == QLatin1String("-l"))
            return true;
    }
    return false;
}

int main(int argc, char** argv)
{
    if( loadSettingsOk(argc, argv) ) {
        // If -l option is provided, settings are loaded and app is closed.
        QGuiApplication app(argc, argv);
        LoadSettings load;
        load.applyBestSettings();
        qDebug() << "[load.applyBestSettings()] Finished";
        //QCoreApplication::instance()->exit(0);
        return app.exec();
    }

    LXQt::SingleApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    // Command line options
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("LXQt Config Monitor"));
    const QString VERINFO = QStringLiteral(LXQT_CONFIG_VERSION
                                           "\nliblxqt   " LXQT_VERSION
                                           "\nQt        " QT_VERSION_STR);
    app.setApplicationVersion(VERINFO);
    QCommandLineOption loadOption(QStringList() << QStringLiteral("l") << QStringLiteral("loadlast"),
                                  app.tr("Load last settings."));
    parser.addOption(loadOption);
    parser.addVersionOption();
    parser.addHelpOption();

    parser.process(app);
    //bool loadLastSettings = parser.isSet(loadOption);

    MonitorSettingsDialog dlg;
    app.setActivationWindow(&dlg);
    dlg.setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-display")));
    dlg.show();

    int ok = app.exec();

    // FIXME: kscreen backend doesn't close properly.
    //system("killall kscreen_backend_launcher");

    return ok;

}
