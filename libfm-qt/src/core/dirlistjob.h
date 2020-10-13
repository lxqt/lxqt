#ifndef FM2_DIRLISTJOB_H
#define FM2_DIRLISTJOB_H

#include "../libfmqtglobals.h"
#include <mutex>
#include "job.h"
#include "filepath.h"
#include "gobjectptr.h"
#include "fileinfo.h"

namespace Fm {

class LIBFM_QT_API DirListJob : public Job {
    Q_OBJECT
public:
    enum Flags {
        FAST = 0,
        DIR_ONLY = 1 << 0,
        DETAILED = 1 << 1
    };

    explicit DirListJob(const FilePath& path, Flags flags);

    FileInfoList& files() {
        return files_;
    }

    void setIncremental(bool set);

    bool incremental() const {
        return emit_files_found;
    }

    FilePath dirPath() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return dir_path;
    }

    std::shared_ptr<const FileInfo> dirInfo() const {
        std::lock_guard<std::mutex> lock{mutex_};
        return dir_fi;
    }

Q_SIGNALS:
    void filesFound(FileInfoList& foundFiles);

protected:

    void exec() override;

private:
    mutable std::mutex mutex_;
    FilePath dir_path;
    Flags flags;
    std::shared_ptr<const FileInfo> dir_fi;
    FileInfoList files_;
    bool emit_files_found;
    // guint delay_add_files_handler;
    // GSList* files_to_add;
};

} // namespace Fm

#endif // FM2_DIRLISTJOB_H
