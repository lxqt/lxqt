#include "terminal.h"

#include <glib.h>
#include <gio/gdesktopappinfo.h>
#include <cstring>
#include <unistd.h>

namespace Fm {

static std::string defaultTerminalName{"xterm"};

#if !GLIB_CHECK_VERSION(2, 28, 0) && !HAVE_DECL_ENVIRON
extern char** environ;
#endif

static void child_setup(gpointer user_data) {
    /* Move child to grandparent group so it will not die with parent */
    setpgid(0, (pid_t)(gsize)user_data);
}

bool launchTerminal(const char* programName, const FilePath& workingDir, Fm::GErrorPtr& error) {
    /* read system terminals file */
    GKeyFile* kf = g_key_file_new();
    if(!g_key_file_load_from_file(kf, LIBFM_QT_DATA_DIR "/terminals.list", G_KEY_FILE_NONE, &error)) {
        g_key_file_free(kf);
        return false;
    }
    auto launch = g_key_file_get_string(kf, programName, "launch", nullptr);
    auto desktop_id = g_key_file_get_string(kf, programName, "desktop_id", nullptr);

    GDesktopAppInfo* appinfo = nullptr;
    if(desktop_id) {
        appinfo = g_desktop_app_info_new(desktop_id);
    }

    const gchar* cmd;
    gchar* _cmd = nullptr;
    if(appinfo) {
        cmd = g_app_info_get_commandline(G_APP_INFO(appinfo));
    }
    else if(launch) {
        cmd = _cmd = g_strdup_printf("%s %s", programName, launch);
    }
    else {
        cmd = programName;
    }

#if 0 // FIXME: what's this?
    if(custom_args) {
        cmd = g_strdup_printf("%s %s", cmd, custom_args);
        g_free(_cmd);
        _cmd = (char*)cmd;
    }
#endif

    char** argv;
    int argc;
    if(!g_shell_parse_argv(cmd, &argc, &argv, nullptr)) {
        argv = nullptr;
    }
    g_free(_cmd);

    if(appinfo) {
        g_object_unref(appinfo);
    }
    if(!argv) { /* parsing failed */
        return false;
    }
    char** envp;
#if GLIB_CHECK_VERSION(2, 28, 0)
    envp = g_get_environ();
#else
    envp = g_strdupv(environ);
#endif

    auto dir = workingDir ? workingDir.localPath() : nullptr;
    if(dir) {
#if GLIB_CHECK_VERSION(2, 32, 0)
        envp = g_environ_setenv(envp, "PWD", dir.get(), TRUE);
#else
        char** env = envp;

        if(env) while(*env != nullptr) {
                if(strncmp(*env, "PWD=", 4) == 0) {
                    break;
                }
                env++;
            }
        if(env == nullptr || *env == nullptr) {
            gint length;

            length = envp ? g_strv_length(envp) : 0;
            envp = g_renew(gchar*, envp, length + 2);
            env = &envp[length];
            env[1] = nullptr;
        }
        else {
            g_free(*env);
        }
        *env = g_strdup_printf("PWD=%s", dir);
#endif
    }

    bool ret = g_spawn_async(dir.get(), argv, envp, G_SPAWN_SEARCH_PATH,
                             child_setup, (gpointer)(gsize)getpgid(getppid()),
                             nullptr, &error);
    g_strfreev(argv);
    g_strfreev(envp);
    g_key_file_free(kf);
    return ret;
}

std::vector<CStrPtr> allKnownTerminals() {
    std::vector<CStrPtr> terminals;
    GKeyFile* kf = g_key_file_new();
    if(g_key_file_load_from_file(kf, LIBFM_QT_DATA_DIR "/terminals.list", G_KEY_FILE_NONE, nullptr)) {
        gsize n;
        auto programs = g_key_file_get_groups(kf, &n);
        terminals.reserve(n);
        for(auto name = programs; *name; ++name) {
            terminals.emplace_back(*name);
        }
        g_free(programs);
    }
    g_key_file_free(kf);
    return terminals;
}

// used by fm-app-info.c
extern "C" char* expand_terminal(char* cmd, gboolean keep_open, GError** error) {
    const char* program = nullptr;
    CStrPtr open_arg;
    CStrPtr noclose_arg;
    CStrPtr custom_args;

    /* read system terminals file */
    GKeyFile* kf = g_key_file_new();
    if(g_key_file_load_from_file(kf, LIBFM_QT_DATA_DIR "/terminals.list", G_KEY_FILE_NONE, error)) {
        if(g_key_file_has_group(kf, defaultTerminalName.c_str())) {
            program = defaultTerminalName.c_str();
            open_arg = CStrPtr{g_key_file_get_string(kf, program, "open_arg", nullptr)};
            noclose_arg = CStrPtr{g_key_file_get_string(kf, program, "noclose_arg", nullptr)};
            custom_args = CStrPtr{g_key_file_get_string(kf, program, "custom_args", nullptr)};
        }
    }
    g_key_file_free(kf);

    const char* opts;
    /* if %s is not found, fallback to -e */
    /* bug #3457335: Crash on application start with Terminal=true. */
    /* fallback to xterm if a terminal emulator is not found. */
    if(!program) {
        /* FIXME: we should not hard code xterm here. :-(
         * It's better to prompt the user and let he or she set
         * his preferred terminal emulator. */
        program = "xterm";
        open_arg = CStrPtr{g_strdup("-e")};
    }

    if(keep_open && noclose_arg)
        opts = noclose_arg.get();
    else
        opts = open_arg.get();

    char* ret = nullptr;
    if(custom_args)
        ret = g_strdup_printf("%s %s %s %s", program, custom_args.get(),
                              opts, cmd);
    else
        ret = g_strdup_printf("%s %s %s", program, opts, cmd);

    return ret;
}

const std::string defaultTerminal() {
    return defaultTerminalName;
}

void setDefaultTerminal(std::string program) {
    defaultTerminalName = program;
}

} // namespace Fm
