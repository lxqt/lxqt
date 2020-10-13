/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Christian Surlykke
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

#include <LXQt/SingleApplication>

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QIODevice>
#include <QSettings>
#include <QVariant>
#include <QCommandLineParser>

#include <LXQt/Settings>
#include <XdgDesktopFile>
#include "mimetypeviewer.h"
#include <XdgDirs>

int main (int argc, char **argv)
{
    LXQt::SingleApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("LXQt Config File Associations"));
    const QString VERINFO = QStringLiteral(LXQT_CONFIG_VERSION
                                           "\nliblxqt   " LXQT_VERSION
                                           "\nQt        " QT_VERSION_STR);
    app.setApplicationVersion(VERINFO);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    MimetypeViewer mimetypeViewer;
    app.setActivationWindow(&mimetypeViewer);
    mimetypeViewer.setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-filetype-association")));
    mimetypeViewer.show();

    return app.exec();
    qDebug() << "Efter exec";
}

