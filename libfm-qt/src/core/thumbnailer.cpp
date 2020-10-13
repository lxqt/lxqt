#include "thumbnailer.h"
#include "mimetype.h"
#include <string>
#include <QDebug>
#include <sys/stat.h>
#include <sys/types.h>

namespace Fm {

std::mutex Thumbnailer::mutex_;
std::vector<std::shared_ptr<Thumbnailer>> Thumbnailer::allThumbnailers_;

Thumbnailer::Thumbnailer(const char* id, GKeyFile* kf):
    id_{g_strdup(id)},
    try_exec_{g_key_file_get_string(kf, "Thumbnailer Entry", "TryExec", nullptr)},
    exec_{g_key_file_get_string(kf, "Thumbnailer Entry", "Exec", nullptr)} {
}

CStrPtr Thumbnailer::commandForUri(const char* uri, const char* output_file, guint size) const {
    if(exec_) {
        /* FIXME: how to handle TryExec? */

        /* parse the command line and do required substitutions according to:
         * https://developer.gnome.org/integration-guide/stable/thumbnailer.html.en
         */
        GString* cmd_line = g_string_sized_new(1024);
        const char* p;
        for(p = exec_.get(); *p; ++p) {
            if(G_LIKELY(*p != '%')) {
                g_string_append_c(cmd_line, *p);
            }
            else {
                char* quoted;
                ++p;
                switch(*p) {
                case '\0':
                    break;
                case 's':
                    g_string_append_printf(cmd_line, "%d", size);
                    break;
                case 'i': {
                    char* src_path = g_filename_from_uri(uri, nullptr, nullptr);
                    if(src_path) {
                        quoted = g_shell_quote(src_path);
                        g_string_append(cmd_line, quoted);
                        g_free(quoted);
                        g_free(src_path);
                    }
                    break;
                }
                case 'u':
                    quoted = g_shell_quote(uri);
                    g_string_append(cmd_line, quoted);
                    g_free(quoted);
                    break;
                case 'o':
                    g_string_append(cmd_line, output_file);
                    break;
                default:
                    g_string_append_c(cmd_line, '%');
                    if(*p != '%') {
                        g_string_append_c(cmd_line, *p);
                    }
                }
            }
        }
        return CStrPtr{g_string_free(cmd_line, FALSE)};
    }
    return nullptr;
}

bool Thumbnailer::run(const char* uri, const char* output_file, int size) const {
    auto cmd = commandForUri(uri, output_file, size);
    qDebug() << cmd.get();
    int status;
    bool ret = g_spawn_command_line_sync(cmd.get(), nullptr, nullptr, &status, nullptr);
    return ret && status == 0;
}

static void find_thumbnailers_in_data_dir(std::unordered_map<std::string, const char*>& hash, const char* data_dir) {
    CStrPtr dir_path{g_build_filename(data_dir, "thumbnailers", nullptr)};
    GDir* dir = g_dir_open(dir_path.get(), 0, nullptr);
    if(dir) {
        const char* basename;
        while((basename = g_dir_read_name(dir)) != nullptr) {
            /* we only want filenames with .thumbnailer extension */
            if(G_LIKELY(g_str_has_suffix(basename, ".thumbnailer"))) {
                hash.insert(std::make_pair(basename, data_dir));
            }
        }
        g_dir_close(dir);
    }
}

void Thumbnailer::loadAll() {
    const gchar* const* data_dirs = g_get_system_data_dirs();
    const gchar* const* data_dir;

    /* use a temporary hash table to collect thumbnailer basenames
     * key: basename of thumbnailer entry file
     * value: data dir the thumbnailer entry file is in */
    std::unordered_map<std::string, const char*> hash;

    /* load user-specific thumbnailers */
    find_thumbnailers_in_data_dir(hash, g_get_user_data_dir());

    /* load system-wide thumbnailers */
    for(data_dir = data_dirs; *data_dir; ++data_dir) {
        find_thumbnailers_in_data_dir(hash, *data_dir);
    }

    /* load all found thumbnailers */
    if(!hash.empty()) {
        std::lock_guard<std::mutex> lock{mutex_};
        GKeyFile* kf = g_key_file_new();
        for(auto& item: hash) {
            auto& base_name = item.first;
            auto& dir_path = item.second;
            CStrPtr file_path{g_build_filename(dir_path, "thumbnailers", base_name.c_str(), nullptr)};
            if(g_key_file_load_from_file(kf, file_path.get(), G_KEY_FILE_NONE, nullptr)) {
                auto thumbnailer = std::make_shared<Thumbnailer>(base_name.c_str(), kf);
                if(thumbnailer->exec_) {
                    char** mime_types = g_key_file_get_string_list(kf, "Thumbnailer Entry", "MimeType", nullptr, nullptr);
                    if(mime_types) {
                        for(char** name = mime_types; *name; ++name) {
                            auto mime_type = MimeType::fromName(*name);
                            if(mime_type) {
                                std::const_pointer_cast<MimeType>(mime_type)->addThumbnailer(thumbnailer);
                            }
                        }
                        g_strfreev(mime_types);
                    }
                }
                allThumbnailers_.push_back(std::move(thumbnailer));
            }
        }
        g_key_file_free(kf);
    }
}

} // namespace Fm
