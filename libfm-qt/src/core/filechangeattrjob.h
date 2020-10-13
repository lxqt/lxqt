#ifndef FM2_FILECHANGEATTRJOB_H
#define FM2_FILECHANGEATTRJOB_H

#include "../libfmqtglobals.h"
#include "fileoperationjob.h"
#include "gioptrs.h"

#include <string>

#include <sys/types.h>

namespace Fm {

class LIBFM_QT_API FileChangeAttrJob : public Fm::FileOperationJob {
    Q_OBJECT
public:
    explicit FileChangeAttrJob(FilePathList paths);

    void setFileModeEnabled(bool enabled) {
        fileModeEnabled_ = enabled;
    }
    void setFileMode(mode_t newMode, mode_t newModeMask) {
        newMode_ = newMode;
        newModeMask_ = newModeMask;
    }

    bool ownerEnabled() const {
        return ownerEnabled_;
    }
    void setOwnerEnabled(bool enabled) {
        ownerEnabled_ = enabled;
    }
    void setOwner(uid_t uid) {
        uid_ = uid;
    }

    bool groupEnabled() const {
        return groupEnabled_;
    }
    void setGroupEnabled(bool groupEnabled) {
        groupEnabled_ = groupEnabled;
    }
    void setGroup(gid_t gid) {
        gid_ = gid;
    }

    // This only work for change attr jobs.
    void setRecursive(bool recursive) {
        recursive_ = recursive;
    }

    void setHiddenEnabled(bool enabled) {
        hiddenEnabled_ = enabled;
    }
    void setHidden(bool hidden) {
        hidden_ = hidden;
    }

    bool iconEnabled() const {
        return iconEnabled_;
    }
    void setIconEnabled(bool iconEnabled) {
        iconEnabled_ = iconEnabled;
    }

    bool displayNameEnabled() const {
        return displayNameEnabled_;
    }
    void setDisplayNameEnabled(bool displayNameEnabled) {
        displayNameEnabled_ = displayNameEnabled;
    }

    const std::string& displayName() const {
        return displayName_;
    }
    void setDisplayName(const std::string& displayName) {
        displayName_ = displayName;
    }

    bool targetUriEnabled() const {
        return targetUriEnabled_;
    }
    void setTargetUriEnabled(bool targetUriEnabled) {
        targetUriEnabled_ = targetUriEnabled;
    }

    const std::string& targetUri() const {
        return targetUri_;
    }
    void setTargetUri(const std::string& value) {
        targetUri_ = value;
    }


protected:
    void exec() override;

private:
    bool processFile(const FilePath& path, const GFileInfoPtr& info);
    bool handleError(GErrorPtr& err, const FilePath& path, const GFileInfoPtr& info, ErrorSeverity severity = ErrorSeverity::MODERATE);

    bool changeFileOwner(const FilePath& path, const GFileInfoPtr& info, uid_t uid);
    bool changeFileGroup(const FilePath& path, const GFileInfoPtr& info, gid_t gid);
    bool changeFileMode(const FilePath& path, const GFileInfoPtr& info, mode_t newMode, mode_t newModeMask);
    bool changeFileDisplayName(const FilePath& path, const GFileInfoPtr& info, const char* displayName);
    bool changeFileIcon(const FilePath& path, const GFileInfoPtr& info, GIconPtr& icon);
    bool changeFileHidden(const FilePath& path, const GFileInfoPtr& info, bool hidden);
    bool changeFileTargetUri(const FilePath& path, const GFileInfoPtr& info, const char* targetUri_);

private:
    FilePathList paths_;
    bool recursive_;

    // chmod
    bool fileModeEnabled_;
    mode_t newMode_;
    mode_t newModeMask_;

    // chown
    bool ownerEnabled_;
    uid_t uid_;

    bool groupEnabled_;
    gid_t gid_;

    // Display name
    bool displayNameEnabled_;
    std::string displayName_;

    // icon
    bool iconEnabled_;
    GIconPtr icon_;

    // hidden
    bool hiddenEnabled_;
    bool hidden_;

    // target uri
    bool targetUriEnabled_;
    std::string targetUri_;
};

} // namespace Fm

#endif // FM2_FILECHANGEATTRJOB_H
