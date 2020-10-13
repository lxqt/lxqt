#ifndef FILEACTIONCONDITION_H
#define FILEACTIONCONDITION_H

#include <glib.h>
#include "../core/gioptrs.h"
#include "../core/fileinfo.h"

namespace Fm {

// FIXME: we can use getgroups() to get groups of current process
// then, call stat() and stat.st_gid to handle capabilities
// in this way, we don't have to call euidaccess

enum class FileActionCapability {
    OWNER = 0,
    READABLE = 1 << 1,
    WRITABLE = 1 << 2,
    EXECUTABLE = 1 << 3,
    LOCAL = 1 << 4
};


class FileActionCondition {
public:
    explicit FileActionCondition(GKeyFile* kf, const char* group);

#if 0
    bool match_base_name_(const FileInfoList& files, const char* allowed_base_name) {
        // all files should match the base_name pattern.
        bool allowed = true;
        if(allowed_base_name.index_of_char('*') >= 0) {
            string allowed_base_name_ci;
            if(!match_case) {
                allowed_base_name_ci = allowed_base_name.casefold(); // FIXME: is this ok?
                allowed_base_name = allowed_base_name_ci;
            }
            var pattern = new PatternSpec(allowed_base_name);
            foreach(unowned FileInfo fi in files) {
                unowned string name = fi.get_name();
                if(match_case) {
                    if(!pattern.match_string(name)) {
                        allowed = false;
                        break;
                    }
                }
                else {
                    if(!pattern.match_string(name.casefold())) {
                        allowed = false;
                        break;
                    }
                }
            }
        }
        else {
            foreach(unowned FileInfo fi in files) {
                unowned string name = fi.get_name();
                if(match_case) {
                    if(allowed_base_name != name) {
                        allowed = false;
                        break;
                    }
                }
                else {
                    if(allowed_base_name.collate(name) != 0) {
                        allowed = false;
                        break;
                    }
                }
            }
        }
        return allowed;
    }
#endif

    bool match_try_exec(const FileInfoList& files);

    bool match_show_if_registered(const FileInfoList& files);

    bool match_show_if_true(const FileInfoList& files);

    bool match_show_if_running(const FileInfoList& files);

    static bool match_mime_type(const FileInfoList& files, const char* type, bool negated);

    bool match_mime_types(const FileInfoList& files);

    bool match_base_name(const FileInfoList& files, const char* base_name, bool negated) const;

    bool match_base_names(const FileInfoList& files);

    static bool match_scheme(const FileInfoList& files, const char* scheme, bool negated);

    bool match_schemes(const FileInfoList& files);

    static bool match_folder(const FileInfoList& files, const char* folder, bool negated);

    bool match_folders(const FileInfoList& files);

    bool match_selection_count(const FileInfoList &files) const;

    bool match_capabilities(const FileInfoList& files);

    bool match(const FileInfoList& files);

    CStrArrayPtr only_show_in;
    CStrArrayPtr not_show_in;
    CStrPtr try_exec;
    CStrPtr show_if_registered;
    CStrPtr show_if_true;
    CStrPtr show_if_running;
    CStrArrayPtr mime_types;
    CStrArrayPtr base_names;
    bool match_case;
    char selection_count_cmp;
    int selection_count;
    CStrArrayPtr schemes;
    CStrArrayPtr folders;
    FileActionCapability capabilities;
};

}

#endif // FILEACTIONCONDITION_H
