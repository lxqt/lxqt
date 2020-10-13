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

#include "defemailclientmatcommand.h"

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

enum DefEmailClientCommandMode {
    CommandModeGetDefEmailClient,
    CommandModeSetDefEmailClient,
    CommandModeListAvailableEmailClients
};

struct DefEmailClientData {
    DefEmailClientData() : mode(CommandModeGetDefEmailClient) {}

    DefEmailClientCommandMode mode;
    QString defEmailClientName;
};

static CommandLineParseResult parseCommandLine(QCommandLineParser *parser, DefEmailClientData *data, QString *errorMessage)
{
    parser->clearPositionalArguments();
    parser->setApplicationDescription(QL1S("Get/Set the default email client"));

    parser->addPositionalArgument(QL1S("def-email-client"), QL1S());

    const QCommandLineOption defEmailClientNameOption(QStringList() << QSL("s") << QSL("set"),
                QSL("Email Client to be set as default"), QSL("email client"));

    const QCommandLineOption listAvailableOption(QStringList() << QSL("l") << QSL("list-available"),
                QSL("List available email clients"));

    parser->addOption(defEmailClientNameOption);
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
    const bool isDefEmailClientNameSet = parser->isSet(defEmailClientNameOption);
    QString defEmailClientName;

    if (isDefEmailClientNameSet)
        defEmailClientName = parser->value(defEmailClientNameOption);

    QStringList posArgs = parser->positionalArguments();
    posArgs.removeAt(0);

    if (isDefEmailClientNameSet && posArgs.size() > 0) {
        *errorMessage = QSL("Extra arguments given: ");
        errorMessage->append(posArgs.join(QLatin1Char(',')));
        return CommandLineError;
    }

    if (isListAvailableSet && (isDefEmailClientNameSet || posArgs.size() > 0)) {
        *errorMessage = QSL("list-available can't be used with other options and doesn't take arguments");
        return CommandLineError;
    }

    if (isListAvailableSet) {
        data->mode = CommandModeListAvailableEmailClients;
    } else {
        data->mode = isDefEmailClientNameSet ? CommandModeSetDefEmailClient: CommandModeGetDefEmailClient;
        data->defEmailClientName = defEmailClientName;
    }

    return CommandLineOk;
}

DefEmailClientMatCommand::DefEmailClientMatCommand(QCommandLineParser *parser)
    : MatCommandInterface(QL1S("def-email-client"),
                          QSL("Get/Set the default email client"),
                          parser)
{
   Q_CHECK_PTR(parser);
}

DefEmailClientMatCommand::~DefEmailClientMatCommand()
{
}

int DefEmailClientMatCommand::run(const QStringList & /*arguments*/)
{
    bool success = true;
    DefEmailClientData data;
    QString errorMessage;
    if (!MatCommandInterface::parser()) {
        qFatal("DefEmailClientMatCommand::run: MatCommandInterface::parser() returned a null pointer");
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

    if (data.mode == CommandModeListAvailableEmailClients) {
        const auto emailClients = XdgDefaultApps::emailClients();
        for (const auto *app : emailClients)
            std::cout << qPrintable(XdgDesktopFile::id(app->fileName())) << "\n";

        qDeleteAll(emailClients);
        return EXIT_SUCCESS;
    }

    if (data.mode == CommandModeGetDefEmailClient) { // Get default email client
        XdgDesktopFile *defEmailClient = XdgDefaultApps::emailClient();
        if (defEmailClient != nullptr && defEmailClient->isValid()) {
            std::cout << qPrintable(XdgDesktopFile::id(defEmailClient->fileName())) << "\n";
            delete defEmailClient;
        }
    } else { // Set default email client
        XdgDesktopFile toSetDefEmailClient;
        if (toSetDefEmailClient.load(data.defEmailClientName)) {
            if (XdgDefaultApps::setEmailClient(toSetDefEmailClient)) {
                std::cout << qPrintable(QSL("Set '%1' as the default email client\n").arg(toSetDefEmailClient.fileName()));
            } else {
                std::cerr << qPrintable(QSL("Could not set '%1' as the default email client\n").arg(toSetDefEmailClient.fileName()));
                success = false;
            }
        } else { // could not load application file
            std::cerr << qPrintable(QSL("Could not find find '%1'\n").arg(data.defEmailClientName));
            success = false;
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
