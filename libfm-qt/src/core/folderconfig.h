#ifndef FOLDERCONFIG_H
#define FOLDERCONFIG_H

#include "../libfmqtglobals.h"

#include "filepath.h"
#include "gioptrs.h"

#include <cstdint>

namespace Fm {

class LIBFM_QT_API FolderConfig {
public:
    FolderConfig();

    explicit FolderConfig(const Fm::FilePath& path);

    ~FolderConfig();

    bool open(const Fm::FilePath& path);

    bool close(GErrorPtr& err);

    bool isOpened() const;

    void purge(void);

    void removeKey(const char* key);

    void setStringList(const char* key, const gchar* const list[], gsize length);

    void setString(const char* key, const char* string);

    void setBoolean(const char* key, bool val);

    void setDouble(const char* key, double val);

    void setUint64(const char* key, std::uint64_t val);

    void setInteger(const char* key, int val);

    char** getStringList(const char* key, gsize* length);

    char* getString(const char* key);

    bool getBoolean(const char* key, bool* val);

    bool getDouble(const char* key, double* val);

    bool getUint64(const char* key, std::uint64_t* val);

    bool getInteger(const char* key, int* val);

    bool isEmpty(void);

    // this needs to be called before using Fm::FolderConfig
    static void init(const char* globalConfigFile);

    static void finalize();

    static void saveCache(void);

// the object cannot be copied.
private:
    FolderConfig(const FolderConfig& other) = delete;
    FolderConfig& operator=(const FolderConfig& other) = delete;

private:
    GKeyFile *keyFile_;
    CStrPtr group_; /* allocated if not in cache */
    CStrPtr configFilePath_; /* NULL if in cache */
    bool changed_;

    static CStrPtr globalConfigFile_;
};

} // namespace Fm

#endif // FOLDERCONFIG_H
