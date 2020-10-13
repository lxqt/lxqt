#include "fileactioncondition.h"
#include "fileaction.h"
#include <string>


using namespace std;

namespace Fm {

FileActionCondition::FileActionCondition(GKeyFile *kf, const char* group) {
    only_show_in = CStrArrayPtr{g_key_file_get_string_list(kf, group, "OnlyShowIn", nullptr, nullptr)};
    not_show_in = CStrArrayPtr{g_key_file_get_string_list(kf, group, "NotShowIn", nullptr, nullptr)};
    try_exec = CStrPtr{g_key_file_get_string(kf, group, "TryExec", nullptr)};
    show_if_registered = CStrPtr{g_key_file_get_string(kf, group, "ShowIfRegistered", nullptr)};
    show_if_true = CStrPtr{g_key_file_get_string(kf, group, "ShowIfTrue", nullptr)};
    show_if_running = CStrPtr{g_key_file_get_string(kf, group, "ShowIfRunning", nullptr)};
    mime_types = CStrArrayPtr{g_key_file_get_string_list(kf, group, "MimeTypes", nullptr, nullptr)};
    base_names = CStrArrayPtr{g_key_file_get_string_list(kf, group, "Basenames", nullptr, nullptr)};
    match_case = g_key_file_get_boolean(kf, group, "Matchcase", nullptr);

    CStrPtr selection_count_str{g_key_file_get_string(kf, group, "SelectionCount", nullptr)};
    if(selection_count_str != nullptr) {
        switch(selection_count_str[0]) {
        case '<':
        case '>':
        case '=':
            selection_count_cmp = selection_count_str[0];
            selection_count = atoi(selection_count_str.get() + 1);
            break;
        default:
            selection_count_cmp = '>';
            selection_count = 0;
            break;
        }
    }
    else {
        selection_count_cmp = '>';
        selection_count = 0;
    }

    schemes = CStrArrayPtr{g_key_file_get_string_list(kf, group, "Schemes", nullptr, nullptr)};
    folders = CStrArrayPtr{g_key_file_get_string_list(kf, group, "Folders", nullptr, nullptr)};
    auto caps = CStrArrayPtr{g_key_file_get_string_list(kf, group, "Capabilities", nullptr, nullptr)};

    // FIXME: implement Capabilities support

}

bool FileActionCondition::match_try_exec(const FileInfoList& files) {
    if(try_exec != nullptr) {
        // stdout.printf("    TryExec: %s\n", try_exec);
        CStrPtr exec_path{g_find_program_in_path(FileActionObject::expand_str(try_exec.get(), files).c_str())};
        if(!g_file_test(exec_path.get(), G_FILE_TEST_IS_EXECUTABLE)) {
            return false;
        }
    }
    return true;
}

bool FileActionCondition::match_show_if_registered(const FileInfoList& files) {
    if(show_if_registered != nullptr) {
        // stdout.printf("    ShowIfRegistered: %s\n", show_if_registered);
        auto service = FileActionObject::expand_str(show_if_registered.get(), files);
        // References:
        // http://people.freedesktop.org/~david/eggdbus-20091014/eggdbus-interface-org.freedesktop.DBus.html#eggdbus-method-org.freedesktop.DBus.NameHasOwner
        // glib source code: gio/tests/gdbus-names.c
        auto con = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
        auto result = g_dbus_connection_call_sync(con,
                                                  "org.freedesktop.DBus",
                                                   "/org/freedesktop/DBus",
                                                   "org.freedesktop.DBus",
                                                   "NameHasOwner",
                                                   g_variant_new("(s)", service.c_str()),
                                                   g_variant_type_new("(b)"),
                                                   G_DBUS_CALL_FLAGS_NONE,
                                                  -1, nullptr, nullptr);
        bool name_has_owner;
        g_variant_get(result, "(b)", &name_has_owner);
        g_variant_unref(result);
        // stdout.printf("check if service: %s is in use: %d\n", service, (int)name_has_owner);
        if(!name_has_owner) {
            return false;
        }
    }
    return true;
}

bool FileActionCondition::match_show_if_true(const FileInfoList& files) {
    if(show_if_true != nullptr) {
        auto cmd = FileActionObject::expand_str(show_if_true.get(), files);
        int exit_status;
        // FIXME: Process.spawn cannot handle shell commands. Use Posix.system() instead.
        //if(!Process.spawn_command_line_sync(cmd, nullptr, nullptr, out exit_status)
        //	|| exit_status != 0)
        //	return false;
        exit_status = system(cmd.c_str());
        if(exit_status != 0) {
            return false;
        }
    }
    return true;
}

bool FileActionCondition::match_show_if_running(const FileInfoList& files) {
    if(show_if_running != nullptr) {
        auto process_name = FileActionObject::expand_str(show_if_running.get(), files);
        CStrPtr pgrep{g_find_program_in_path("pgrep")};
        bool running = false;
        // pgrep is not fully portable, but we don't have better options here
        if(pgrep != nullptr) {
            int exit_status;
            // cmd = "$pgrep -x '$process_name'"
            string cmd = pgrep.get();
            cmd += " -x \'";
            cmd += process_name;
            cmd += "\'";
            if(g_spawn_command_line_sync(cmd.c_str(), nullptr, nullptr, &exit_status, nullptr)) {
                if(exit_status == 0) {
                    running = true;
                }
            }
        }
        if(!running) {
            return false;
        }
    }
    return true;
}

bool FileActionCondition::match_mime_type(const FileInfoList& files, const char* type, bool negated) {
    // stdout.printf("match_mime_type: %s, neg: %d\n", type, (int)negated);

    if(strcmp(type, "all/all") == 0 || strcmp(type, "*") == 0) {
        return negated ? false : true;
    }
    else if(strcmp(type, "all/allfiles") == 0) {
        // see if all fileinfos are files
        if(negated) { // all fileinfos should not be files
            for(auto& fi: files) {
                if(!fi->isDir()) { // at least 1 of the fileinfos is a file.
                    return false;
                }
            }
        }
        else { // all fileinfos should be files
            for(auto& fi: files) {
                if(fi->isDir()) { // at least 1 of the fileinfos is a file.
                    return false;
                }
            }
        }
    }
    else if(g_str_has_suffix(type, "/*")) {
        // check if all are subtypes of allowed_type
        string prefix{type};
        prefix.erase(prefix.length() - 1); // remove the last char
        if(negated) { // all files should not have the prefix
            for(auto& fi: files) {
                if(g_str_has_prefix(fi->mimeType()->name(), prefix.c_str())) {
                    return false;
                }
            }
        }
        else { // all files should have the prefix
            for(auto& fi: files) {
                if(!g_str_has_prefix(fi->mimeType()->name(), prefix.c_str())) {
                    return false;
                }
            }
        }
    }
    else {
        if(negated) { // all files should not be of the type
            for(auto& fi: files) {
                if(strcmp(fi->mimeType()->name(),type) == 0) {
                    // if(ContentType.is_a(type, fi.get_mime_type().get_type())) {
                    return false;
                }
            }
        }
        else { // all files should be of the type
            for(auto& fi: files) {
                // stdout.printf("get_type: %s, type: %s\n", fi.get_mime_type().get_type(), type);
                if(strcmp(fi->mimeType()->name(),type) != 0) {
                    // if(!ContentType.is_a(type, fi.get_mime_type().get_type())) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool FileActionCondition::match_mime_types(const FileInfoList& files) {
    if(mime_types != nullptr) {
        bool allowed = false;
        // FIXME: this is inefficient, but easier to implement
        // check if all of the mime_types are allowed
        for(auto mime_type = mime_types.get(); *mime_type; ++mime_type) {
            const char* allowed_type = *mime_type;
            const char* type;
            bool negated;
            if(allowed_type[0] == '!') {
                type = allowed_type + 1;
                negated = true;
            }
            else {
                type = allowed_type;
                negated = false;
            }

            if(negated) { // negated mime_type rules are ANDed
                bool type_is_allowed = match_mime_type(files, type, negated);
                if(!type_is_allowed) { // so any mismatch is not allowed
                    return false;
                }
            }
            else { // other mime_type rules are ORed
                // matching any one of the mime_type is enough
                if(!allowed) { // if no rule is matched yet
                    allowed = match_mime_type(files, type, false);
                }
            }
        }
        return allowed;
    }
    return true;
}

bool FileActionCondition::match_base_name(const FileInfoList& files, const char* base_name, bool negated) const {
    // see if all files has the base_name
    // FIXME: this is inefficient, some optimization is needed later
    GPatternSpec* pattern;
    if(match_case) {
        pattern = g_pattern_spec_new(base_name);
    }
    else {
        CStrPtr case_fold{g_utf8_casefold(base_name, -1)};
        pattern = g_pattern_spec_new(case_fold.get());    // FIXME: is this correct?
    }
    for(auto& fi: files) {
        const char* name = fi->name().c_str();
        if(match_case) {
            if(g_pattern_match_string(pattern, name)) {
                // at least 1 file has the base_name
                if(negated) {
                    return false;
                }
            }
            else {
                // at least 1 file does not has the scheme
                if(!negated) {
                    return false;
                }
            }
        }
        else {
            CStrPtr case_fold{g_utf8_casefold(name, -1)};
            if(g_pattern_match_string(pattern, case_fold.get())) {
                // at least 1 file has the base_name
                if(negated) {
                    return false;
                }
            }
            else {
                // at least 1 file does not has the scheme
                if(!negated) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool FileActionCondition::match_base_names(const FileInfoList& files) {
    if(base_names != nullptr) {
        bool allowed = false;
        // FIXME: this is inefficient, but easier to implement
        // check if all of the base_names are allowed
        for(auto it = base_names.get(); *it; ++it) {
            auto allowed_name = *it;
            const char* name;
            bool negated;
            if(allowed_name[0] == '!') {
                name = allowed_name + 1;
                negated = true;
            }
            else {
                name = allowed_name;
                negated = false;
            }

            if(negated) { // negated base_name rules are ANDed
                bool name_is_allowed = match_base_name(files, name, negated);
                if(!name_is_allowed) { // so any mismatch is not allowed
                    return false;
                }
            }
            else { // other base_name rules are ORed
                // matching any one of the base_name is enough
                if(!allowed) { // if no rule is matched yet
                    allowed = match_base_name(files, name, false);
                }
            }
        }
        return allowed;
    }
    return true;
}

bool FileActionCondition::match_scheme(const FileInfoList& files, const char* scheme, bool negated) {
    // FIXME: this is inefficient, some optimization is needed later
    // see if all files has the scheme
    for(auto& fi: files) {
        if(fi->path().hasUriScheme(scheme)) {
            // at least 1 file has the scheme
            if(negated) {
                return false;
            }
        }
        else {
            // at least 1 file does not has the scheme
            if(!negated) {
                return false;
            }
        }
    }
    return true;
}

bool FileActionCondition::match_schemes(const FileInfoList& files) {
    if(schemes != nullptr) {
        bool allowed = false;
        // FIXME: this is inefficient, but easier to implement
        // check if all of the schemes are allowed
        for(auto it = schemes.get(); *it; ++it) {
            auto allowed_scheme = *it;
            const char* scheme;
            bool negated;
            if(allowed_scheme[0] == '!') {
                scheme = allowed_scheme + 1;
                negated = true;
            }
            else {
                scheme = allowed_scheme;
                negated = false;
            }

            if(negated) { // negated scheme rules are ANDed
                bool scheme_is_allowed = match_scheme(files, scheme, negated);
                if(!scheme_is_allowed) { // so any mismatch is not allowed
                    return false;
                }
            }
            else { // other scheme rules are ORed
                // matching any one of the scheme is enough
                if(!allowed) { // if no rule is matched yet
                    allowed = match_scheme(files, scheme, false);
                }
            }
        }
        return allowed;
    }
    return true;
}

bool FileActionCondition::match_folder(const FileInfoList& files, const char* folder, bool negated) {
    // trailing /* should always be implied.
    // FIXME: this is inefficient, some optimization is needed later
    GPatternSpec* pattern;
    if(g_str_has_suffix(folder, "/*")) {
        pattern = g_pattern_spec_new(folder);
    }
    else {
        auto pat_str = g_str_has_suffix(folder, "/") ? string(folder) + "*" // be tolerant
                                                     : string(folder) + "/*";
        pattern = g_pattern_spec_new(pat_str.c_str());
    }
    for(auto& fi: files) {
        auto dirname = fi->isDir() ? fi->path().toString() // also match "folder" itself
                                   : fi->dirPath().toString();
        // Since the pattern ends with "/*", if the directory path is equal to "folder",
        // it should end with "/" to be found as a match. Adding "/" is always harmless.
        auto path_str = string(dirname.get()) + "/";
        if(g_pattern_match_string(pattern, path_str.c_str())) { // at least 1 file is in the folder
            if(negated) {
                return false;
            }
        }
        else {
            if(!negated) {
                return false;
            }
        }
    }
    return true;
}

bool FileActionCondition::match_folders(const FileInfoList& files) {
    if(folders != nullptr) {
        bool allowed = false;
        // FIXME: this is inefficient, but easier to implement
        // check if all of the schemes are allowed
        for(auto it = folders.get(); *it; ++it) {
            auto allowed_folder = *it;
            const char* folder;
            bool negated;
            if(allowed_folder[0] == '!') {
                folder = allowed_folder + 1;
                negated = true;
            }
            else {
                folder = allowed_folder;
                negated = false;
            }

            if(negated) { // negated folder rules are ANDed
                bool folder_is_allowed = match_folder(files, folder, negated);
                if(!folder_is_allowed) { // so any mismatch is not allowed
                    return false;
                }
            }
            else { // other folder rules are ORed
                // matching any one of the folder is enough
                if(!allowed) { // if no rule is matched yet
                    allowed = match_folder(files, folder, false);
                }
            }
        }
        return allowed;
    }
    return true;
}

bool FileActionCondition::match_selection_count(const FileInfoList& files) const {
    const int n_files = files.size();
    switch(selection_count_cmp) {
    case '<':
        if(n_files >= selection_count) {
            return false;
        }
        break;
    case '=':
        if(n_files != selection_count) {
            return false;
        }
        break;
    case '>':
        if(n_files <= selection_count) {
            return false;
        }
        break;
    }
    return true;
}

bool FileActionCondition::match_capabilities(const FileInfoList& /*files*/) {
    // TODO
    return true;
}

bool FileActionCondition::match(const FileInfoList& files) {
    // all of the condition are combined with AND
    // So, if any one of the conditions is not matched, we quit.

    // TODO: OnlyShowIn, NotShowIn
    if(!match_try_exec(files)) {
        return false;
    }

    if(!match_mime_types(files)) {
        return false;
    }
    if(!match_base_names(files)) {
        return false;
    }
    if(!match_selection_count(files)) {
        return false;
    }
    if(!match_schemes(files)) {
        return false;
    }
    if(!match_folders(files)) {
        return false;
    }
    // TODO: Capabilities
    // currently, due to limitations of Fm.FileInfo, this cannot
    // be implemanted correctly.
    if(!match_capabilities(files)) {
        return false;
    }

    if(!match_show_if_registered(files)) {
        return false;
    }
    if(!match_show_if_true(files)) {
        return false;
    }
    if(!match_show_if_running(files)) {
        return false;
    }

    return true;
}


} // namespace Fm
