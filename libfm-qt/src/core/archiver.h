#ifndef ARCHIVER_H
#define ARCHIVER_H

#include "../libfmqtglobals.h"
#include "filepath.h"
#include "gioptrs.h"

#include <vector>
#include <memory>

namespace Fm {

class LIBFM_QT_API Archiver {
public:
    Archiver();

    bool isMimeTypeSupported(const char* type);

    bool canCreateArchive() const {
        return createCmd_ != nullptr;
    }

    bool createArchive(GAppLaunchContext* ctx, const FilePathList& files);

    bool canExtractArchives() const {
        return extractCmd_ != nullptr;
    }

    bool extractArchives(GAppLaunchContext* ctx, const FilePathList& files);

    bool canExtractArchivesTo() const {
        return extractToCmd_ != nullptr;
    }

    bool extractArchivesTo(GAppLaunchContext* ctx, const FilePathList& files, const FilePath& dest_dir);

    /* get default GUI archivers used by libfm */
    static Archiver* defaultArchiver();

    /* set default GUI archivers used by libfm */
    static void setDefaultArchiverByName(const char* name);

    /* set default GUI archivers used by libfm */
    static void setDefaultArchiver(Archiver* archiver);

    /* get a list of FmArchiver* of all GUI archivers known to libfm */
    static const std::vector<std::unique_ptr<Archiver>>& allArchivers();

    const char* program() const {
        return program_.get();
    }

private:
    bool launchProgram(GAppLaunchContext* ctx, const char* cmd, const FilePathList& files, const FilePath &dir);

private:
    CStrPtr program_;
    CStrPtr createCmd_;
    CStrPtr extractCmd_;
    CStrPtr extractToCmd_;
    CStrArrayPtr mimeTypes_;

    static Archiver* defaultArchiver_;
    static std::vector<std::unique_ptr<Archiver>> allArchivers_;
};

} // namespace Fm

#endif // ARCHIVER_H
