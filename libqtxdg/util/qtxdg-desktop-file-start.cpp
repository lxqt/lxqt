/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2016  Luís Pereira <luis.artur.pereira@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFileInfo>

#include <XdgDesktopFile>

#include <iostream>

static void printErr(const QString & out)
{
    std::cerr << qPrintable(out);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QLatin1String("qtxdg-desktop-file-start"));
    QCoreApplication::setApplicationVersion(QLatin1String(QTXDG_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QLatin1String("QtXdg XdgDesktopFile start Tester"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QLatin1String("file [urls...]"), QLatin1String("desktop file to start and it's urls"),QLatin1String("file [urls...]"));
    parser.process(app);

    if (parser.positionalArguments().isEmpty()) {
        parser.showHelp(EXIT_FAILURE);
    }

    QStringList userArgs = parser.positionalArguments();
    const QString userFileName = userArgs.takeFirst();
    const QFileInfo fileInfo(userFileName);
    const QString canonicalFileName = fileInfo.canonicalFilePath();

    if (!fileInfo.exists()) {
        printErr(QString::fromLatin1("File %1 does not exist\n").arg(userFileName));
        return EXIT_FAILURE;
    }

    XdgDesktopFile f;
    const bool valid = f.load(canonicalFileName);
    if (valid) {
        f.startDetached(userArgs);
    } else {
        printErr(QString::fromLatin1("%1 is not a valid .desktop file\n").arg(canonicalFileName));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
