#include "libfmqtglobals.h"
#include "archiver.h"

#include <cstring>
#include <glib.h>
#include <gio/gdesktopappinfo.h>

#include <string>

namespace Fm {

Archiver* Archiver::defaultArchiver_ = nullptr;  // static
std::vector<std::unique_ptr<Archiver>> Archiver::allArchivers_;  // static

Archiver::Archiver() {
}

bool Archiver::isMimeTypeSupported(const char* type) {
    char** p;
    if(G_UNLIKELY(!type)) {
        return false;
    }
    for(p = mimeTypes_.get(); *p; ++p) {
        if(strcmp(*p, type) == 0) {
            return true;
        }
    }
    return false;
}

bool Archiver::launchProgram(GAppLaunchContext* ctx, const char* cmd, const FilePathList& files, const FilePath& dir) {
    char* _cmd = nullptr;
    const char* dir_place_holder;
    GKeyFile* dummy;

    if(dir.isValid() && (dir_place_holder = strstr(cmd, "%d"))) {
        CStrPtr dir_str;
        int len;
        if(strstr(cmd, "%U") || strstr(cmd, "%u")) { /* supports URI */
            dir_str = dir.uri();
        }
        else {
            dir_str = dir.localPath();
        }

        // FIXME: remove libfm dependency here
        /* replace all % with %% so encoded URI can be handled correctly when parsing Exec key. */
        std::string percentEscapedDir;
        for(auto p = dir_str.get(); *p; ++p) {
            percentEscapedDir += *p;
            if(*p == '%') {
                percentEscapedDir += '%';
            }
        }

        /* quote the path or URI */
        dir_str = CStrPtr{g_shell_quote(percentEscapedDir.c_str())};

        len = strlen(cmd) - 2 + strlen(dir_str.get()) + 1;
        _cmd = (char*)g_malloc(len);
        len = (dir_place_holder - cmd);
        strncpy(_cmd, cmd, len);
        strcpy(_cmd + len, dir_str.get());
        strcat(_cmd, dir_place_holder + 2);
        cmd = _cmd;
    }

    /* create a fake key file to cheat GDesktopAppInfo */
    dummy = g_key_file_new();
    g_key_file_set_string(dummy, G_KEY_FILE_DESKTOP_GROUP, "Type", "Application");
    g_key_file_set_string(dummy, G_KEY_FILE_DESKTOP_GROUP, "Name", program_.get());

    /* replace all % with %% so encoded URI can be handled correctly when parsing Exec key. */
    g_key_file_set_string(dummy, G_KEY_FILE_DESKTOP_GROUP, "Exec", cmd);
    GAppInfoPtr app{reinterpret_cast<GAppInfo*>(g_desktop_app_info_new_from_keyfile(dummy)), false};

    g_key_file_free(dummy);
    g_debug("cmd = %s", cmd);
    if(app) {
        GList* uris = nullptr;
        for(auto& file: files) {
            uris = g_list_prepend(uris, g_strdup(file.uri().get()));
        }
        g_app_info_launch_uris(app.get(), uris, ctx, nullptr);
        g_list_free_full(uris, g_free);
    }
    g_free(_cmd);
    return true;
}

bool Archiver::createArchive(GAppLaunchContext* ctx, const FilePathList& files) {
    if(createCmd_ && !files.empty()) {
        launchProgram(ctx, createCmd_.get(), files, FilePath{});
    }
    return false;
}

bool Archiver::extractArchives(GAppLaunchContext* ctx, const FilePathList& files) {
    if(extractCmd_ && !files.empty()) {
        launchProgram(ctx, extractCmd_.get(), files, FilePath{});
    }
    return false;
}

bool Archiver::extractArchivesTo(GAppLaunchContext* ctx, const FilePathList& files, const FilePath& dest_dir) {
    if(extractToCmd_ && !files.empty()) {
        launchProgram(ctx, extractToCmd_.get(), files, dest_dir);
    }
    return false;
}

// static
Archiver* Archiver::defaultArchiver() {
    allArchivers(); // to have a preliminary default archiver
    return defaultArchiver_;
}

void Archiver::setDefaultArchiverByName(const char *name) {
    if(name) {
        auto& all = allArchivers();
        for(auto& archiver: all) {
            if(archiver->program_ && strcmp(archiver->program_.get(), name) == 0) {
                defaultArchiver_ = archiver.get();
                break;
            }
        }
    }
}

// static
void Archiver::setDefaultArchiver(Archiver* archiver) {
    if(archiver) {
        defaultArchiver_ = archiver;
    }
}

// static
const std::vector<std::unique_ptr<Archiver> >& Archiver::allArchivers() {
    // load all archivers on demand
    if(allArchivers_.empty()) {
        GKeyFile* kf = g_key_file_new();
        if(g_key_file_load_from_file(kf, LIBFM_QT_DATA_DIR "/archivers.list", G_KEY_FILE_NONE, nullptr)) {
            gsize n_archivers;
            CStrArrayPtr programs{g_key_file_get_groups(kf, &n_archivers)};
            if(programs) {
                gsize i;
                for(i = 0; i < n_archivers; ++i) {
                    auto program = programs[i];
                    std::unique_ptr<Archiver> archiver{new Archiver{}};
                    archiver->createCmd_ = CStrPtr{g_key_file_get_string(kf, program, "create", nullptr)};
                    archiver->extractCmd_ = CStrPtr{g_key_file_get_string(kf, program, "extract", nullptr)};
                    archiver->extractToCmd_ = CStrPtr{g_key_file_get_string(kf, program, "extract_to", nullptr)};
                    archiver->mimeTypes_ = CStrArrayPtr{g_key_file_get_string_list(kf, program, "mime_types", nullptr, nullptr)};
                    archiver->program_ = CStrPtr{g_strdup(program)};

                    // if default archiver is not set, find the first program existing in the current system.
                    if(!defaultArchiver_) {
                        CStrPtr fullPath{g_find_program_in_path(program)};
                        if(fullPath) {
                            defaultArchiver_ = archiver.get();
                        }
                    }

                    allArchivers_.emplace_back(std::move(archiver));
                }
            }
        }
        g_key_file_free(kf);
    }
    return allArchivers_;
}

} // namespace Fm
