#ifndef TERMINAL_H
#define TERMINAL_H

#include "../libfmqtglobals.h"
#include "gioptrs.h"
#include "filepath.h"
#include <vector>
#include <string>

namespace Fm {

LIBFM_QT_API bool launchTerminal(const char* programName, const FilePath& workingDir, GErrorPtr& error);

LIBFM_QT_API std::vector<CStrPtr> allKnownTerminals();

LIBFM_QT_API const std::string defaultTerminal();

LIBFM_QT_API void setDefaultTerminal(std::string program);

inline bool launchDefaultTerminal(const FilePath& workingDir, GErrorPtr& error) {
    return launchTerminal(defaultTerminal().c_str(), workingDir, error);
}

} // namespace Fm

#endif // TERMINAL_H
