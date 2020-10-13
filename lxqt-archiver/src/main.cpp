// LXQt Archiver
// Copyright (C) 2018 The LXQt team
// Some of the following code is derived from Engrampa and File Roller

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 */

#include "mainwindow.h"
#include "progressdialog.h"
#include "archiver.h"
#include "passworddialog.h"
#include "createfiledialog.h"
#include "extractfiledialog.h"
#include "core/fr-init.h"
#include "core/config.h"
#include "core/tr-wrapper.h"

#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>

#include <libfm-qt/libfmqt.h>

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QMimeDatabase>

#include <string>
#include <unordered_map>

static char** remaining_args;
static char*  add_to = nullptr;
static int    add;
static char*  extract_to = nullptr;
static int    extract;
static int    extract_here;
static char*  default_url = nullptr;

/* argv[0] from main(); used as the command to restart the program */
static const char* program_argv0 = nullptr;
static std::unordered_map<std::string, QByteArray> gettextCache;

static const GOptionEntry options[] = {
    {
        "add-to", 'a', 0, G_OPTION_ARG_STRING, &add_to,
        N_("Add files to the specified archive and quit the program"),
        N_("ARCHIVE")
    },

    {
        "add", 'd', 0, G_OPTION_ARG_NONE, &add,
        N_("Add files asking the name of the archive and quit the program"),
        nullptr
    },

    {
        "extract-to", 'e', 0, G_OPTION_ARG_STRING, &extract_to,
        N_("Extract archives to the specified folder and quit the program"),
        N_("FOLDER")
    },

    {
        "extract", 'f', 0, G_OPTION_ARG_NONE, &extract,
        N_("Extract archives asking the destination folder and quit the program"),
        nullptr
    },

    {
        "extract-here", 'h', 0, G_OPTION_ARG_NONE, &extract_here,
        N_("Extract the contents of the archives in the archive folder and quit the program"),
        nullptr
    },

    {
        "default-dir", '\0', 0, G_OPTION_ARG_STRING, &default_url,
        N_("Default folder to use for the '--add' and '--extract' commands"),
        N_("FOLDER")
    },

    {
        "force", '\0', 0, G_OPTION_ARG_NONE, &ForceDirectoryCreation,
        N_("Create destination folder without asking confirmation"),
        nullptr
    },

    {
        G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args,
        nullptr,
        nullptr
    },

    { nullptr }
};


static char* get_uri_from_command_line(const char* path) {
    GFile* file;
    char*  uri;

    file = g_file_new_for_commandline_arg(path);
    uri = g_file_get_uri(file);
    g_object_unref(file);

    return uri;
}

static int runApp(QApplication& app) {
    char*        extract_to_uri = nullptr;
    char*        add_to_uri = nullptr;

    // handle command line options

    if(remaining_args == nullptr) {  /* No archive specified. */
        auto mainWin = new MainWindow();
        mainWin->show();
        return app.exec();
    }

    bool extractSkipOlder = false;
    bool extractOverwrite = false;
    bool extractReCreateFolders = false;

    if(extract_to != nullptr) {
        extract_to_uri = get_uri_from_command_line(extract_to);
    }
    else {
        // if we want to extract files but dest dir is not specified, choose one
        if(extract && !extract_here) {
            if(!extract_to_uri) {
                ExtractFileDialog dlg{nullptr, default_url ? Fm::FilePath::fromUri(default_url) : Fm::FilePath::homeDir()};
                dlg.setExtractSelectedEnabled(false);
                dlg.setExtractAll(true);
                if(dlg.exec() != QDialog::Accepted) {
                    return 1;
                }

                const QUrl dirUrl = dlg.selectedFiles().at(0);
                extract_to_uri = g_strdup(dirUrl.toEncoded().constData());

                extractSkipOlder = dlg.skipOlder();
                extractOverwrite = dlg.overwrite();
                extractReCreateFolders = dlg.reCreateFolders();
            }
        }
    }

    std::string addPassword;
    bool addEncryptFileList = false;
    bool addSplitVolumes = false;
    unsigned int addVolumeSize = 0;

    if(add_to != nullptr) {
        add_to_uri = get_uri_from_command_line(add_to);
    }
    else {
        // if we want to add files but archive path is not specified, choose one
        if(add) {
            CreateFileDialog dlg;
            if(default_url) {
                dlg.setDirectory(QUrl::fromEncoded(default_url));
            }
            else {
                // set the directory to the parent of the first file
                // and make the archive name out of its name
                const char* firstFile = remaining_args[0];
                if(firstFile != nullptr) {
                    auto path = Fm::FilePath::fromPathStr(firstFile);
                    if(path.hasParent()) {
                        auto target = path.parent();
                        auto name = QString::fromUtf8(path.baseName().get());
                        const auto mimeTypes = Archiver::supportedCreateMimeTypes();
                        if(!mimeTypes.isEmpty()) {
                            QMimeDatabase mimeDb;
                            auto mimeType = mimeDb.mimeTypeForName(mimeTypes.at(0));
                            if(mimeType.isValid()) {
                                auto suffixes = mimeType.suffixes();
                                if(!suffixes.isEmpty()) {
                                    name += QStringLiteral(".") + suffixes.at(0);
                                    target = target.child(name.toLocal8Bit().constData());
                                    dlg.selectFile(QUrl::fromEncoded(QByteArray(target.uri().get())));
                                }
                            }
                        }
                    }
                }
            }
            if(dlg.exec() == QDialog::Accepted) {
                const auto url = dlg.selectedFiles().at(0);
                if(url.isEmpty()) {
                    return 1;
                }
                add_to_uri = g_strdup(url.toEncoded().constData());
                addPassword = dlg.password().toStdString();
                addEncryptFileList = dlg.encryptFileList();
                addSplitVolumes = dlg.splitVolumes();
                addVolumeSize = dlg.volumeSize();
            }
            else {
                return 1;
            }
        }
    }

    if((add_to != nullptr) || (add == 1)) {
        /* Add files to an archive */
        const char* filename = nullptr;
        int i = 0;

        Fm::FilePathList filePaths;
        while((filename = remaining_args[i++]) != nullptr) {
            filePaths.emplace_back(Fm::FilePath::fromPathStr(filename));
        }

        Archiver archiver;
        ProgressDialog dlg;
        dlg.setOperation(QObject::tr("Adding file: "));

        dlg.setArchiver(&archiver);
        archiver.createNewArchive(add_to_uri);

        // we can only add files after the archive is fully created
        QObject::connect(&archiver, &Archiver::finish, &dlg, [&](FrAction action, ArchiverError err) {
            if(err.hasError()) {
                QMessageBox::critical(&dlg, QObject::tr("Error"), err.message());
                dlg.reject();
                return;
            }
            switch(action) {
            case FR_ACTION_CREATING_NEW_ARCHIVE:
                archiver.addDroppedItems(filePaths, // src files
                                         nullptr, // base dir
                                         default_url, // dest dir
                                         false,  // update: what's this?
                                         addPassword.empty() ? nullptr : addPassword.c_str(),
                                         addEncryptFileList,
                                         FR_COMPRESSION_NORMAL,
                                         addSplitVolumes ? addVolumeSize : 0);
                break;
            case FR_ACTION_ADDING_FILES:
                dlg.accept();
                break;
            };
        });
        dlg.exec();
        return 0;
    }
    else if((extract_to != nullptr) || (extract == 1) || (extract_here == 1)) {
        const char* filename = nullptr;
        int i = 0;
        /* Extract all archives. */
        while((filename = remaining_args[i++]) != nullptr) {
            auto archive_uri = Fm::CStrPtr{get_uri_from_command_line(filename)};

            Archiver archiver;
            ProgressDialog dlg;
            dlg.setOperation(QObject::tr("Extracting file: "));

            dlg.setArchiver(&archiver);
            archiver.openArchive(archive_uri.get(), nullptr);

            // we can only start archive extraction after its content is fully loaded
            QObject::connect(&archiver, &Archiver::finish, &dlg, [&](FrAction action, ArchiverError err) {
                if(err.hasError()) {
                    QMessageBox::critical(&dlg, QObject::tr("Error"), err.message());
                    dlg.reject();
                    return;
                }
                switch(action) {
                case FR_ACTION_LISTING_CONTENT: {            /* loading the archive from a remote location */
                    std::string password_;
                    if(archiver.isEncrypted()) {
                        password_ = PasswordDialog::askPassword().toStdString();
                    }

                    if(extract_here) {
                        archiver.extractHere(extractSkipOlder,
                                             extractOverwrite,
                                             extractReCreateFolders,
                                             password_.empty() ? nullptr : password_.c_str());
                    }
                    else {
                        // a target dir is specified
                        archiver.extractAll(extract_to_uri,
                                            extractSkipOlder,
                                            extractOverwrite,
                                            extractReCreateFolders,
                                            password_.empty() ? nullptr : password_.c_str());
                    }
                    break;
                }
                case FR_ACTION_EXTRACTING_FILES:
                    dlg.accept();
                    break;
                default:
                    break;
                };
            });
            dlg.exec();
            return 0;
        }
    }
    else { /* Open each archive in a window */
        const char* filename = nullptr;
        int i = 0;
        while((filename = remaining_args[i++]) != nullptr) {
            auto mainWindow = new MainWindow();
            auto file = Fm::FilePath::fromPathStr(filename);
            mainWindow->loadFile(file);
            mainWindow->show();
        }
    }
    g_free(add_to_uri);
    g_free(extract_to_uri);
    return app.exec();
}


const char* qtGettext(const char* msg) {
    auto it = gettextCache.find(msg);
    if(it == gettextCache.end()) {
        auto result = QObject::tr(msg).toUtf8();
        it = gettextCache.emplace(msg, std::move(result)).first;
    }
    return it->second.constData();
}

int main(int argc, char** argv) {
    GOptionContext* context = nullptr;
    GError*         error = nullptr;
    int             status;

    program_argv0 = argv[0];

    QApplication app(argc, argv);
    app.setApplicationVersion(QStringLiteral(LXQT_ARCHIVER_VERSION));
    app.setQuitOnLastWindowClosed(true);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // load translations
    // install the translations built-into Qt itself
    QTranslator qtTranslator;
    qtTranslator.load(QStringLiteral("qt_") + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    // install our own tranlations
    QTranslator translator;
    translator.load(QStringLiteral("lxqt-archiver_") + QLocale::system().name(), QStringLiteral(LXQT_ARCHIVER_DATA_DIR) + QStringLiteral("/translations"));
    app.installTranslator(&translator);

    // initialize libfm-qt
    Fm::LibFmQt libfmQt;
    app.installTranslator(libfmQt.translator());

    // set a fake gettext replacement to translate the C code in src/core/*.c taken from engrampa
    // this function pointer is defined in src/core/tr-wrapper.c
    qt_gettext = qtGettext;

    // TODO: replace this with QCommandLineParser
    context = g_option_context_new(N_("- Create and modify an archive"));
    g_option_context_add_main_entries(context, options, nullptr);

    if(! g_option_context_parse(context, &argc, &argv, &error)) {
        g_critical("Failed to parse arguments: %s", error->message);
        g_error_free(error);
        g_option_context_free(context);
        return EXIT_FAILURE;
    }

    g_option_context_free(context);
    g_set_application_name(_("LXQt Archiver"));

    // FIXME: port command line parsing to Qt
    initialize_data(); // initialize the file-roller core
    status = runApp(app);
    release_data();

    return status;
}
