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

#include "defwebbrowsermatcommand.h"

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

enum DefWebBrowserCommandMode {
    CommandModeGetDefWebBrowser,
    CommandModeSetDefWebBrowser,
    CommandModeListAvailableWebBrowsers
};

struct DefWebBrowserData {
    DefWebBrowserData() : mode(CommandModeGetDefWebBrowser) {}

    DefWebBrowserCommandMode mode;
    QString defWebBrowserName;
};

static CommandLineParseResult parseCommandLine(QCommandLineParser *parser, DefWebBrowserData *data, QString *errorMessage)
{
    parser->clearPositionalArguments();
    parser->setApplicationDescription(QL1S("Get/Set the default web browser"));

    parser->addPositionalArgument(QL1S("def-web-browser"), QL1S());

    const QCommandLineOption defWebBrowserNameOption(QStringList() << QSL("s") << QSL("set"),
                QSL("Web Browser to be set as default"), QSL("web bowser"));

    const QCommandLineOption listAvailableOption(QStringList() << QSL("l") << QSL("list-available"),
                QSL("List available web browsers"));

    parser->addOption(defWebBrowserNameOption);
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
    const bool isDefWebBrowserNameSet = parser->isSet(defWebBrowserNameOption);
    QString defWebBrowserName;

    if (isDefWebBrowserNameSet)
        defWebBrowserName = parser->value(defWebBrowserNameOption);

    QStringList posArgs = parser->positionalArguments();
    posArgs.removeAt(0);

    if (isDefWebBrowserNameSet && posArgs.size() > 0) {
        *errorMessage = QSL("Extra arguments given: ");
        errorMessage->append(posArgs.join(QLatin1Char(',')));
        return CommandLineError;
    }

    if (!isDefWebBrowserNameSet && posArgs.size() > 0) {
        *errorMessage = QSL("To set the default browser use the -s/--set option");
        return CommandLineError;
    }

    if (isListAvailableSet && (isDefWebBrowserNameSet || posArgs.size() > 0)) {
        *errorMessage = QSL("list-available can't be used with other options and doesn't take arguments");
        return CommandLineError;
    }

    if (isListAvailableSet) {
        data->mode = CommandModeListAvailableWebBrowsers;
    } else {
        data->mode = isDefWebBrowserNameSet ? CommandModeSetDefWebBrowser : CommandModeGetDefWebBrowser;
        data->defWebBrowserName = defWebBrowserName;
    }

    return CommandLineOk;
}

DefWebBrowserMatCommand::DefWebBrowserMatCommand(QCommandLineParser *parser)
    : MatCommandInterface(QL1S("def-web-browser"),
                          QSL("Get/Set the default web browser"),
                          parser)
{
   Q_CHECK_PTR(parser);
}

DefWebBrowserMatCommand::~DefWebBrowserMatCommand()
{
}

int DefWebBrowserMatCommand::run(const QStringList & /*arguments*/)
{
    bool success = true;
    DefWebBrowserData data;
    QString errorMessage;
    if (!MatCommandInterface::parser()) {
        qFatal("DefWebBrowserMatCommand::run: MatCommandInterface::parser() returned a null pointer");
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

    if (data.mode == CommandModeListAvailableWebBrowsers) {
        const auto webBrowsers = XdgDefaultApps::webBrowsers();
        for (const auto *app : webBrowsers)
            std::cout << qPrintable(XdgDesktopFile::id(app->fileName())) << "\n";

        qDeleteAll(webBrowsers);
        return EXIT_SUCCESS;
    }

    if (data.mode == CommandModeGetDefWebBrowser) { // Get default web browser
        XdgDesktopFile *defWebBrowser = XdgDefaultApps::webBrowser();
        if (defWebBrowser != nullptr && defWebBrowser->isValid()) {
            std::cout << qPrintable(XdgDesktopFile::id(defWebBrowser->fileName())) << "\n";
            delete defWebBrowser;
        }
    } else { // Set default web browser
        XdgDesktopFile toSetDefWebBrowser;
        if (toSetDefWebBrowser.load(data.defWebBrowserName)) {
            if (XdgDefaultApps::setWebBrowser(toSetDefWebBrowser)) {
                std::cout << qPrintable(QSL("Set '%1' as the default web browser\n").arg(toSetDefWebBrowser.fileName()));
            } else {
                std::cerr << qPrintable(QSL("Could not set '%1' as the default web browser\n").arg(toSetDefWebBrowser.fileName()));
                success = false;
            }
        } else { // could not load application file
            std::cerr << qPrintable(QSL("Could not find find '%1'\n").arg(data.defWebBrowserName));
            success = false;
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
