#include "fileactionprofile.h"
#include "fileaction.h"
#include <QDebug>

using namespace std;

namespace Fm {

FileActionProfile::FileActionProfile(GKeyFile *kf, const char* profile_name) {
    id = profile_name;
    std::string group_name = "X-Action-Profile " + id;
    name = CStrPtr{g_key_file_get_string(kf, group_name.c_str(), "Name", nullptr)};
    exec = CStrPtr{g_key_file_get_string(kf, group_name.c_str(), "Exec", nullptr)};
    // stdout.printf("id: %s, Exec: %s\n", id, exec);

    path = CStrPtr{g_key_file_get_string(kf, group_name.c_str(), "Path", nullptr)};
    auto s = CStrPtr{g_key_file_get_string(kf, group_name.c_str(), "ExecutionMode", nullptr)};
    if(s) {
        if(strcmp(s.get(), "Normal") == 0) {
            exec_mode = FileActionExecMode::NORMAL;
        }
        else if(strcmp(s.get(), "Terminal") == 0) {
            exec_mode = FileActionExecMode::TERMINAL;
        }
        else if(strcmp(s.get(), "Embedded") == 0) {
            exec_mode = FileActionExecMode::EMBEDDED;
        }
        else if(strcmp(s.get(), "DisplayOutput") == 0) {
            exec_mode = FileActionExecMode::DISPLAY_OUTPUT;
        }
        else {
            exec_mode = FileActionExecMode::NORMAL;
        }
    }
    else {
        exec_mode = FileActionExecMode::NORMAL;
    }

    startup_notify = g_key_file_get_boolean(kf, group_name.c_str(), "StartupNotify", nullptr);
    startup_wm_class = CStrPtr{g_key_file_get_string(kf, group_name.c_str(), "StartupWMClass", nullptr)};
    exec_as = CStrPtr{g_key_file_get_string(kf, group_name.c_str(), "ExecuteAs", nullptr)};

    condition = make_shared<FileActionCondition>(kf, group_name.c_str());
}


bool FileActionProfile::launch_once(GAppLaunchContext* /*ctx*/, std::shared_ptr<const FileInfo> first_file, const FileInfoList& files, CStrPtr& output) {
    if(exec == nullptr) {
        return false;
    }
    auto exec_cmd = FileActionObject::expand_str(exec.get(), files, false, first_file);
    bool ret = false;
    if(exec_mode == FileActionExecMode::DISPLAY_OUTPUT) {
        int exit_status;
        char* output_buf = nullptr;
        ret = g_spawn_command_line_sync(exec_cmd.c_str(), &output_buf, nullptr, &exit_status, nullptr);
        if(ret) {
            ret = (exit_status == 0);
        }
        output = CStrPtr{output_buf};
    }
    else {
        /*
        AppInfoCreateFlags flags = AppInfoCreateFlags.NONE;
        if(startup_notify)
            flags |= AppInfoCreateFlags.SUPPORTS_STARTUP_NOTIFICATION;
        if(exec_mode == FileActionExecMode::TERMINAL ||
           exec_mode == FileActionExecMode::EMBEDDED)
            flags |= AppInfoCreateFlags.NEEDS_TERMINAL;
        GLib.AppInfo app = Fm.AppInfo.create_from_commandline(exec, nullptr, flags);
        stdout.printf("Execute command line: %s\n\n", exec);
        ret = app.launch(nullptr, ctx);
        */

        // NOTE: we cannot use GAppInfo here since GAppInfo does
        // command line parsing which involving %u, %f, and other
        // code defined in desktop entry spec.
        // This may conflict with DES EMA parameters.
        // FIXME: so how to handle this cleaner?
        // Maybe we should leave all %% alone and don't translate
        // them to %. Then GAppInfo will translate them to %, not
        // codes specified in DES.
        ret = g_spawn_command_line_async(exec_cmd.c_str(), nullptr);
    }
    return ret;
}


bool FileActionProfile::launch(GAppLaunchContext* ctx, const FileInfoList& files, CStrPtr& output) {
    bool plural_form = FileActionObject::is_plural_exec(exec.get());
    bool ret;
    if(plural_form) { // plural form command, handle all files at a time
        ret = launch_once(ctx, files.front(), files, output);
    }
    else { // singular form command, run once for each file
        GString* all_output = g_string_sized_new(1024);
        bool show_output = false;
        for(auto& fi: files) {
            CStrPtr one_output;
            launch_once(ctx, fi, files, one_output);
            if(one_output) {
                show_output = true;
                // FIXME: how to handle multiple output std::strings properly?
                g_string_append(all_output, one_output.get());
                g_string_append(all_output, "\n");
            }
        }
        if(show_output) {
            output = CStrPtr{g_string_free(all_output, false)};
        }
        else {
            g_string_free(all_output, true);
        }
        ret = true;
    }
    return ret;
}

bool FileActionProfile::match(FileInfoList files) {
    // stdout.printf("  match profile: %s\n", id);
    return condition->match(files);
}

}
