#ifndef FM2_FILESYSTEMINFOJOB_H
#define FM2_FILESYSTEMINFOJOB_H

#include "../libfmqtglobals.h"
#include "job.h"
#include "filepath.h"

namespace Fm {

class LIBFM_QT_API FileSystemInfoJob : public Job {
    Q_OBJECT
public:
    explicit FileSystemInfoJob(const FilePath& path):
        path_{path},
        isAvailable_{false},
        size_{0},
        freeSize_{0} {
    }

    bool isAvailable() const {
        return isAvailable_;
    }

    uint64_t size() const {
        return size_;
    }

    uint64_t freeSize() const {
        return freeSize_;
    }

protected:

    void exec() override;

private:
    FilePath path_;
    bool isAvailable_;
    uint64_t size_;
    uint64_t freeSize_;
};

} // namespace Fm

#endif // FM2_FILESYSTEMINFOJOB_H
