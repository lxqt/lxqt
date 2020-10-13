#ifndef BASICFILELAUNCHER_H
#define BASICFILELAUNCHER_H

#include "../libfmqtglobals.h"

#include "fileinfo.h"
#include "filepath.h"
#include "mimetype.h"

#include <gio/gio.h>

namespace Fm {

class LIBFM_QT_API BasicFileLauncher {
public:

    enum class ExecAction {
        NONE,
        DIRECT_EXEC,
        EXEC_IN_TERMINAL,
        OPEN_WITH_DEFAULT_APP,
        CANCEL
    };

    explicit BasicFileLauncher();
    virtual ~BasicFileLauncher();

    bool launchFiles(const FileInfoList &fileInfos, GAppLaunchContext* ctx = nullptr);

    bool launchPaths(FilePathList paths, GAppLaunchContext* ctx = nullptr);

    bool launchDesktopEntry(const FileInfoPtr &fileInfo, const FilePathList& paths = FilePathList{}, GAppLaunchContext* ctx = nullptr);

    bool launchDesktopEntry(const char* desktopEntryName, const FilePathList& paths = FilePathList{}, GAppLaunchContext* ctx = nullptr);

    bool launchWithDefaultApp(const FileInfoPtr& fileInfo, GAppLaunchContext* ctx = nullptr);

    bool launchWithApp(GAppInfo* app, const FilePathList& paths, GAppLaunchContext* ctx = nullptr);

    bool launchExecutable(const FileInfoPtr &fileInfo, GAppLaunchContext* ctx = nullptr);

    bool quickExec() const {
        return quickExec_;
    }

    void setQuickExec(bool value) {
        quickExec_ = value;
    }

protected:

    virtual GAppInfoPtr chooseApp(const FileInfoList& fileInfos, const char* mimeType, GErrorPtr& err);

    virtual bool openFolder(GAppLaunchContext* ctx, const FileInfoList& folderInfos, GErrorPtr& err);

    virtual bool showError(GAppLaunchContext* ctx, const GErrorPtr& err, const FilePath& path = FilePath{}, const FileInfoPtr& info = FileInfoPtr{});

    virtual ExecAction askExecFile(const FileInfoPtr& file);

    virtual int ask(const char* msg, char* const* btn_labels, int default_btn);

private:

    FilePath handleShortcut(const FileInfoPtr &fileInfo, GAppLaunchContext* ctx = nullptr);

private:
    bool quickExec_; // Don't ask options on launch executable file
};

} // namespace Fm

#endif // BASICFILELAUNCHER_H
