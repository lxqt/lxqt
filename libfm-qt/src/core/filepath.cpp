#include "filepath.h"
#include <cstdlib>
#include <utility>
#include <glib.h>

namespace Fm {

FilePath FilePath::homeDir_;

const FilePath &FilePath::homeDir() {
    if(!homeDir_) {
        const char* home = getenv("HOME");
        if(!home) {
            home = g_get_home_dir();
        }
        homeDir_ = FilePath::fromLocalPath(home);
    }
    return homeDir_;
}

} // namespace Fm
