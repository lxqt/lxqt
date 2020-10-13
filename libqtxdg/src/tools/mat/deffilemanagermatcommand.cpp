/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2020  Luís Pereira <luis.artur.pereira@gmail.com>
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

#include "deffilemanagermatcommand.h"

#include "matglobals.h"
#include "xdgmacros.h"
#include "xdgdefaultapps.h"
#include "xdgdesktopfile.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QStringList>

#include <iostream>

enum DefFileManagerCommandMode {
    CommandModeGetDefFileManager,
    CommandModeSetDefFileManager,
    CommandModeListAvailableFileManagers,
};

struct DefFileManagerData {
    DefFileManagerData() : mode(CommandModeGetDefFileManager) {}

    DefFileManagerCommandMode mode;
    QString defFileManagerName;
};

static CommandLineParseResult parseCommandLine(QCommandLineParser *parser, DefFileManagerData *data, QString *errorMessage)
{
    parser->clearPositionalArguments();
    parser->setApplicationDescription(QL1S("Get/Set the default file manager"));

    parser->addPositionalArgument(QL1S("def-file-manager"), QL1S());

    const QCommandLineOption defFileManagerNameOption(QStringList() << QSL("s") << QSL("set"),
                QSL("File Manager to be set as default"), QSL("file manager"));

    const QCommandLineOption listAvailableOption(QStringList() << QSL("l") << QSL("list-available"),
                QSL("List available file managers"));

    parser->addOption(defFileManagerNameOption);
    parser->addOption(listAvailableOption);
    const QCommandLineOption helpOption = parser->addHelpOption();
    const QCommandLineOption versionOption = parser->addVersionOption();

    if (!parser->parse(QCoreApplication::arguments())) {
        *errorMessage = parser->errorText();
        return CommandLineError;
    }

    if (parser->isSet(versionOption)) {
        return CommandLineVersionRequested;
    }

    if (parser->isSet(helpOption)) {
        return CommandLineHelpRequested;
    }

    const bool isListAvailableSet = parser->isSet(listAvailableOption);
    const bool isDefFileManagerNameSet = parser->isSet(defFileManagerNameOption);
    QString defFileManagerName;

    if (isDefFileManagerNameSet)
        defFileManagerName = parser->value(defFileManagerNameOption);

    QStringList posArgs = parser->positionalArguments();
    posArgs.removeAt(0);

    if (isDefFileManagerNameSet && posArgs.size() > 0) {
        *errorMessage = QSL("Extra arguments given: ");
        errorMessage->append(posArgs.join(QLatin1Char(',')));
        return CommandLineError;
    }

    if (isListAvailableSet && (isDefFileManagerNameSet || posArgs.size() > 0)) {
        *errorMessage = QSL("list-available can't be used with other options and doesn't take arguments");
        return CommandLineError;
    }

    if (isListAvailableSet) {
        data->mode = CommandModeListAvailableFileManagers;
    } else {
        data->mode = isDefFileManagerNameSet ? CommandModeSetDefFileManager: CommandModeGetDefFileManager;
        data->defFileManagerName = defFileManagerName;
    }

    return CommandLineOk;
}

DefFileManagerMatCommand::DefFileManagerMatCommand(QCommandLineParser *parser)
    : MatCommandInterface(QL1S("def-file-manager"),
                          QSL("Get/Set the default file manager"),
                          parser)
{
   Q_CHECK_PTR(parser);
}

DefFileManagerMatCommand::~DefFileManagerMatCommand()
{
}

int DefFileManagerMatCommand::run(const QStringList & /*arguments*/)
{
    bool success = true;
    DefFileManagerData data;
    QString errorMessage;
    if (!MatCommandInterface::parser()) {
        qFatal("DefFileManagerMatCommand::run: MatCommandInterface::parser() returned a null pointer");
    }
    switch(parseCommandLine(parser(), &data, &errorMessage)) {
    case CommandLineOk:
        break;
    case CommandLineError:
        std::cerr << qPrintable(errorMessage);
        std::cerr << "\n\n";
        std::cerr << qPrintable(parser()->helpText());
        return EXIT_FAILURE;
    case CommandLineVersionRequested:
        showVersion();
        Q_UNREACHABLE();
    case CommandLineHelpRequested:
        showHelp();
        Q_UNREACHABLE();
    }

    if (data.mode == CommandModeListAvailableFileManagers) {
        const auto fileManagers = XdgDefaultApps::fileManagers();
        for (const auto *app : fileManagers)
            std::cout << qPrintable(XdgDesktopFile::id(app->fileName())) << "\n";

        qDeleteAll(fileManagers);
        return EXIT_SUCCESS;
    }

    if (data.mode == CommandModeGetDefFileManager) { // Get default file manager
        XdgDesktopFile *defFileManager = XdgDefaultApps::fileManager();
        if (defFileManager != nullptr && defFileManager->isValid()) {
            std::cout << qPrintable(XdgDesktopFile::id(defFileManager->fileName())) << "\n";
            delete defFileManager;
        }
    } else { // Set default file manager
        XdgDesktopFile toSetDefFileManager;
        if (toSetDefFileManager.load(data.defFileManagerName)) {
            if (XdgDefaultApps::setFileManager(toSetDefFileManager)) {
                std::cout << qPrintable(QSL("Set '%1' as the default file manager\n").arg(toSetDefFileManager.fileName()));
            } else {
                std::cerr << qPrintable(QSL("Could not set '%1' as the default file manager\n").arg(toSetDefFileManager.fileName()));
                success = false;
            }
        } else { // could not load application file
            std::cerr << qPrintable(QSL("Could not find find '%1'\n").arg(data.defFileManagerName));
            success = false;
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
