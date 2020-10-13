/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2019  Luís Pereira <luis.artur.pereira@gmail.com>
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
#include "mimetypematcommand.h"
#include "matglobals.h"

#include "xdgmacros.h"
#include "xdgdesktopfile.h"
#include "xdgmimeapps.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStringList>
#include <QtGlobal>
#include <QUrl>

#include <iostream>

MimeTypeMatCommand::MimeTypeMatCommand(QCommandLineParser *parser)
    : MatCommandInterface(QL1S("mimetype"),
                          QL1S("Determines a file (mime)type"),
                          parser)
{
}

MimeTypeMatCommand::~MimeTypeMatCommand()
{
}

static CommandLineParseResult parseCommandLine(QCommandLineParser *parser, QString *file, QString *errorMessage)
{
    parser->clearPositionalArguments();
    parser->setApplicationDescription(QL1S("Determines a file (mime)type"));

    parser->addPositionalArgument(QL1S("mimetype"), QSL("file | URL"),
                                  QCoreApplication::tr("[file | URL]"));

    parser->addHelpOption();
    parser->addVersionOption();

    parser->process(QCoreApplication::arguments());
    QStringList fs = parser->positionalArguments();
    if (fs.size() < 2) {
        *errorMessage = QSL("No file given");
        return CommandLineError;
    }

    fs.removeAt(0);

    if (fs.size() > 1) {
        *errorMessage = QSL("Only one file, please");
        return CommandLineError;
    }
    *file = fs.at(0);

    return CommandLineOk;
}

int MimeTypeMatCommand::run(const QStringList &arguments)
{
    Q_UNUSED(arguments);

    QString errorMessage;
    QString file;

    switch(parseCommandLine(parser(), &file, &errorMessage)) {
    case CommandLineOk:
        break;
    case CommandLineError:
        std::cerr << qPrintable(errorMessage);
        std::cerr << "\n\n";
        std::cerr << qPrintable(parser()->helpText());
        return EXIT_FAILURE;
    case CommandLineVersionRequested:
        parser()->showVersion();
        Q_UNREACHABLE();
    case CommandLineHelpRequested:
        parser()->showHelp();
        Q_UNREACHABLE();
    }

    bool isLocalFile = false;
    QString localFilename;
    const QUrl url(file);
    const QString scheme = url.scheme();
    if (scheme.isEmpty()) {
        isLocalFile = true;
        localFilename = file;
    } else if (scheme == QL1S("file")) {
        isLocalFile = true;
        localFilename = QUrl(file).toLocalFile();
    }

    if (isLocalFile) {
        const QFileInfo info(file);
        if (!info.exists(localFilename)) {
            std::cerr << qPrintable(QSL("Cannot access '%1': No such file or directory\n").arg(file));
            return EXIT_FAILURE;
        } else {
            QMimeDatabase mimeDb;
            const QMimeType mimeType = mimeDb.mimeTypeForFile(info, QMimeDatabase::MatchExtension);
            std::cout << qPrintable(mimeType.name()) << "\n";
            return EXIT_SUCCESS;
        }
    } else { // not a local file
        std::cerr << qPrintable(QSL("Can't handle '%1': '%2' scheme not supported\n").arg(file, scheme));
        return EXIT_FAILURE;
    }
}
