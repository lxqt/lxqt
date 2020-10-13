#ifndef FM2_TOTALSIZEJOB_H
#define FM2_TOTALSIZEJOB_H

#include "../libfmqtglobals.h"
#include "fileoperationjob.h"
#include "filepath.h"
#include <cstdint>
#include "gioptrs.h"

namespace Fm {

class LIBFM_QT_API TotalSizeJob : public Fm::FileOperationJob {
    Q_OBJECT
public:
    enum Flags {
        DEFAULT = 0,
        FOLLOW_LINKS = 1 << 0,
        SAME_FS = 1 << 1,
        PREPARE_MOVE = 1 << 2,
        PREPARE_DELETE = 1 << 3
    };

    explicit TotalSizeJob(FilePathList paths = FilePathList{}, Flags flags = DEFAULT);

    std::uint64_t totalSize() const {
        return totalSize_;
    }

    std::uint64_t totalOnDiskSize() const {
        return totalOndiskSize_;
    }

    unsigned int fileCount() const {
        return fileCount_;
    }

protected:

    void exec() override;

private:
    void exec(FilePath path, GFileInfoPtr inf);

private:
    FilePathList paths_;

    int flags_;
    std::uint64_t totalSize_;
    std::uint64_t totalOndiskSize_;
    unsigned int fileCount_;
    const char* dest_fs_id;
};

} // namespace Fm

#endif // FM2_TOTALSIZEJOB_H
