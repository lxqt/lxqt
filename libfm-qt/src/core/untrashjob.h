#ifndef FM2_UNTRASHJOB_H
#define FM2_UNTRASHJOB_H

#include "../libfmqtglobals.h"
#include "fileoperationjob.h"

namespace Fm {

class LIBFM_QT_API UntrashJob : public FileOperationJob {
public:
    explicit UntrashJob(FilePathList srcPaths);

protected:
    void exec() override;

private:
    FilePathList srcPaths_;
};

} // namespace Fm

#endif // FM2_UNTRASHJOB_H
