#ifndef FM2_COPYJOB_H
#define FM2_COPYJOB_H

#include "../libfmqtglobals.h"
#include "fileoperationjob.h"
#include "gioptrs.h"

namespace Fm {

class LIBFM_QT_API FileTransferJob : public Fm::FileOperationJob {
    Q_OBJECT
public:

    enum class Mode {
        COPY,
        MOVE,
        LINK
    };

    explicit FileTransferJob(FilePathList srcPaths, Mode mode = Mode::COPY);
    explicit FileTransferJob(FilePathList srcPaths, FilePathList destPaths, Mode mode = Mode::COPY);
    explicit FileTransferJob(FilePathList srcPaths, const FilePath &destDirPath, Mode mode = Mode::COPY);

    void setSrcPaths(FilePathList srcPaths);
    void setDestPaths(FilePathList destPaths);
    void setDestDirPath(const FilePath &destDirPath);

protected:
    void exec() override;

private:
    bool processPath(const FilePath& srcPath, const FilePath& destPath, const char *destFileName);
    bool moveFile(const FilePath &srcPath, const GFileInfoPtr &srcInfo, const FilePath &destDirPath, const char *destFileName);
    bool copyFile(const FilePath &srcPath, const GFileInfoPtr &srcInfo, const FilePath &destDirPath, const char *destFileName, bool skip = false);
    bool linkFile(const FilePath &srcPath, const GFileInfoPtr &srcInfo, const FilePath &destDirPath, const char *destFileName);

    bool moveFileSameFs(const FilePath &srcPath, const GFileInfoPtr& srcInfo, FilePath &destPath);
    bool copyRegularFile(const FilePath &srcPath, const GFileInfoPtr& srcInfo, FilePath &destPath);
    bool copySpecialFile(const FilePath &srcPath, const GFileInfoPtr& srcInfo, FilePath& destPath);
    bool copyDirContent(const FilePath &srcPath, GFileInfoPtr srcInfo, FilePath &destPath, bool skip = false);
    bool makeDir(const FilePath &srcPath, GFileInfoPtr srcInfo, FilePath &destPath);
    bool createSymlink(const FilePath &srcPath, const GFileInfoPtr& srcInfo, FilePath& destPath);
    bool createShortcut(const FilePath &srcPath, const GFileInfoPtr& srcInfo, FilePath& destPath);

    bool handleError(GErrorPtr& err, const FilePath &srcPath, const GFileInfoPtr &srcInfo, FilePath &destPath, int& flags);

    static void gfileCopyProgressCallback(goffset current_num_bytes, goffset total_num_bytes, FileTransferJob* _this);

private:
    FilePathList srcPaths_;
    FilePathList destPaths_;
    Mode mode_;
};


} // namespace Fm

#endif // FM2_COPYJOB_H
