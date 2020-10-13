/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2018  Luís Pereira <luis.artur.pereira@gmail.com>
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

#include "matcommandmanager.h"
#include "mimetypematcommand.h"
#include "defappmatcommand.h"
#include "openmatcommand.h"
#include "defwebbrowsermatcommand.h"
#include "defemailclientmatcommand.h"
#include "deffilemanagermatcommand.h"

#include "xdgmacros.h"

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>

extern void Q_CORE_EXPORT qt_call_post_routines();

[[noreturn]] void showHelp(const QString &parserHelp, const QString &commandsDescription, int exitCode = 0);

[[noreturn]] void showHelp(const QString &parserHelp, const QString &commandsDescription, int exitCode)
{
    QString text;
    const QLatin1Char nl('\n');

    text.append(parserHelp);
    text.append(nl);
    text.append(QCoreApplication::tr("Available commands:\n"));
    text.append(commandsDescription);
    text.append(nl);
    fputs(qPrintable(text), stdout);

    qt_call_post_routines();
    ::exit(exitCode);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    int runResult = 0;
    app.setApplicationName(QSL("qtxdg-mat"));
    app.setApplicationVersion(QSL(QTXDG_VERSION));
    app.setOrganizationName(QSL("LXQt"));
    app.setOrganizationDomain(QSL("lxqt.org"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QSL("QtXdg MimeApps Tool"));

    parser.addPositionalArgument(QSL("command"),
                                 QSL("Command to execute."));

    QScopedPointer<MatCommandManager> manager(new MatCommandManager());

    MatCommandInterface *const mimeCmd = new DefAppMatCommand(&parser);
    manager->add(mimeCmd);

    MatCommandInterface *const openCmd = new OpenMatCommand(&parser);
    manager->add(openCmd);

    MatCommandInterface *const mimeTypeCmd = new MimeTypeMatCommand(&parser);
    manager->add(mimeTypeCmd);

    MatCommandInterface *const defWebBrowserCmd = new DefWebBrowserMatCommand(&parser);
    manager->add(defWebBrowserCmd);

    MatCommandInterface *const defEmailClientCmd = new DefEmailClientMatCommand(&parser);
    manager->add(defEmailClientCmd);

    MatCommandInterface *const defFileManagerCmd = new DefFileManagerMatCommand(&parser);
    manager->add(defFileManagerCmd);

    // Find out the positional arguments.
    parser.parse(QCoreApplication::arguments());
    const QStringList args = parser.positionalArguments();
    const QString command = args.isEmpty() ? QString() : args.first();
    bool cmdFound = false;

    const QList <MatCommandInterface *> commands = manager->commands();
    for (auto *const cmd : commands) {
        if (command == cmd->name()) {
            cmdFound = true;
            runResult = cmd->run(args);
        }
        if (cmdFound)
            break;
    }

    if (!cmdFound) {
        const QCommandLineOption helpOption = parser.addHelpOption();
        const QCommandLineOption versionOption = parser.addVersionOption();
        parser.parse(QCoreApplication::arguments());
        if (parser.isSet(helpOption)) {
            showHelp(parser.helpText(), manager->descriptionsHelpText(), EXIT_SUCCESS);
            Q_UNREACHABLE();
        }
        if (parser.isSet(versionOption)) {
            parser.showVersion();
            Q_UNREACHABLE();
        }
        showHelp(parser.helpText(), manager->descriptionsHelpText(), EXIT_FAILURE);
        Q_UNREACHABLE();
    } else {
        return runResult;
    }
}
