#ifndef FM2_FILELINKJOB_H
#define FM2_FILELINKJOB_H

#include "../libfmqtglobals.h"
#include "fileoperationjob.h"

namespace Fm {

class LIBFM_QT_API FileLinkJob : public Fm::FileOperationJob {
public:
    explicit FileLinkJob();
};

} // namespace Fm

#endif // FM2_FILELINKJOB_H
