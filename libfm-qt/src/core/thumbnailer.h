#ifndef FM2_THUMBNAILER_H
#define FM2_THUMBNAILER_H

#include "../libfmqtglobals.h"
#include "cstrptr.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace Fm {

class MimeType;

class LIBFM_QT_API Thumbnailer {
public:
    explicit Thumbnailer(const char *id, GKeyFile *kf);

    CStrPtr commandForUri(const char* uri, const char* output_file, guint size) const;

    bool run(const char* uri, const char* output_file, int size) const;

    static void loadAll();

private:
    CStrPtr id_;
    CStrPtr try_exec_; /* FIXME: is this useful? */
    CStrPtr exec_;
    //std::vector<std::shared_ptr<const MimeType>> mimeTypes_;

    static std::mutex mutex_;
    static std::vector<std::shared_ptr<Thumbnailer>> allThumbnailers_;
};

} // namespace Fm

#endif // FM2_THUMBNAILER_H
