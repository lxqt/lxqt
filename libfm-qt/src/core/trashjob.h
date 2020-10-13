#ifndef FM2_TRASHJOB_H
#define FM2_TRASHJOB_H

#include "../libfmqtglobals.h"
#include "fileoperationjob.h"
#include "filepath.h"

namespace Fm {

class LIBFM_QT_API TrashJob : public Fm::FileOperationJob {
    Q_OBJECT
public:
    explicit TrashJob(FilePathList paths);

    FilePathList unsupportedFiles() const {
        return unsupportedFiles_;
    }

protected:

    void exec() override;

private:
    FilePathList paths_;
    FilePathList unsupportedFiles_;
};

} // namespace Fm

#endif // FM2_TRASHJOB_H
