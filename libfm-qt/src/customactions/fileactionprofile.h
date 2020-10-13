#ifndef FILEACTIONPROFILE_H
#define FILEACTIONPROFILE_H


#include <string>
#include <glib.h>
#include <gio/gio.h>

#include "../core/fileinfo.h"
#include "fileactioncondition.h"

namespace Fm {

enum class FileActionExecMode {
    NORMAL,
    TERMINAL,
    EMBEDDED,
    DISPLAY_OUTPUT
};

class FileActionProfile {
public:
    explicit FileActionProfile(GKeyFile* kf, const char* profile_name);

    bool launch_once(GAppLaunchContext* ctx, std::shared_ptr<const FileInfo> first_file, const FileInfoList& files, CStrPtr& output);

    bool launch(GAppLaunchContext* ctx, const FileInfoList& files, CStrPtr& output);

    bool match(FileInfoList files);

    std::string id;
    CStrPtr name;
    CStrPtr exec;
    CStrPtr path;
    FileActionExecMode exec_mode;
    bool startup_notify;
    CStrPtr startup_wm_class;
    CStrPtr exec_as;

    std::shared_ptr<FileActionCondition> condition;
};

} // namespace Fm

#endif // FILEACTIONPROFILE_H
