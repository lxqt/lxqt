/**
 * SECTION:fm-folder-config
 * @short_description: Folder specific settings cache.
 * @title: FmFolderConfig
 *
 * @include: libfm/fm.h
 *
 * This API represents access to folder-specific configuration settings.
 * Each setting is a key/value pair. To use it the descriptor should be
 * opened first, then required operations performed, then closed. Each
 * opened descriptor holds a lock on the cache so it is not adviced to
 * keep it somewhere.
 */

#include "folderconfig.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <cerrno>

namespace Fm {

CStrPtr FolderConfig::globalConfigFile_;

// FIXME: sharing the same keyfile object everywhere is problematic
// FIXME: this is MT-unsafe
static GKeyFile* fc_cache = nullptr;
static bool fc_cache_changed = FALSE;

FolderConfig::FolderConfig():
    keyFile_{nullptr},
    changed_{false} {
}

FolderConfig::FolderConfig(const Fm::FilePath& path): FolderConfig{} {
    (void)open(path);
}

FolderConfig::~FolderConfig() {
    if(isOpened()) {
        GErrorPtr err;
        close(err);
    }
}

bool FolderConfig::open(const Fm::FilePath& path) {
    if(isOpened()) {  // the config is already opened
        return false;
    }

    changed_ = FALSE;
    if(path.isNative()) {
        /* clear .directory file first */
        auto sub_path = path.child(".directory");
        configFilePath_ = sub_path.toString();

        // FIXME: this only works for local filesystem and it's a blocking call. :-(
        if(g_file_test(configFilePath_.get(), G_FILE_TEST_EXISTS)) {
            keyFile_ = g_key_file_new();
            if(g_key_file_load_from_file(keyFile_, configFilePath_.get(),
                                         GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS),
                                         nullptr) &&
                    g_key_file_has_group(keyFile_, "File Manager")) {
                group_ = CStrPtr{g_strdup("File Manager")};
                return true;
            }
            g_key_file_free(keyFile_);
        }
    }

    // No per-folder config file.
    // use the global key file instead and use the folder path as group key
    configFilePath_.reset();
    group_ = path.toString();

    // FIXME: we should use ref counting here. glib 2.36+ supports g_key_file_ref()
    keyFile_ = fc_cache;
    return true;
}


bool FolderConfig::close(GErrorPtr& err) {
    bool ret = TRUE;
    if(!isOpened()) {
        return false;
    }

    if(configFilePath_) {
        if(changed_) {
            char* out;
            gsize len;

            out = g_key_file_to_data(keyFile_, &len, &err);
            if(!out || !g_file_set_contents(configFilePath_.get(), out, len, &err)) {
                ret = FALSE;
            }
            g_free(out);
        }
        configFilePath_.reset();
        g_key_file_free(keyFile_);
    }
    else {
        group_.reset();
        if(changed_) {
            fc_cache_changed = TRUE;
        }
    }
    keyFile_ = nullptr;
    return ret;
}

bool FolderConfig::isOpened() const {
    return keyFile_ != nullptr;
}

bool FolderConfig::isEmpty() {
    return !g_key_file_has_group(keyFile_, group_.get());
}

bool FolderConfig::getInteger(const char* key, int* val) {
    GErrorPtr err;
    auto ret = g_key_file_get_integer(keyFile_, group_.get(), key, &err);
    if(err) {
        return false;
    }
    *val = ret;
    return true;
}

bool FolderConfig::getUint64(const char* key, uint64_t *val) {
    GError* error = nullptr;
#if GLIB_CHECK_VERSION(2, 26, 0)
    guint64 ret = g_key_file_get_uint64(keyFile_, group_.get(), key, &error);
#else
    gchar* s, *end;
    guint64 ret;

    s = g_key_file_get_value(keyFile_, group_.get(), key, &error);
#endif
    if(error) {
        g_error_free(error);
        return FALSE;
    }
#if !GLIB_CHECK_VERSION(2, 26, 0)
    ret = g_ascii_strtoull(s, &end, 10);
    if(*s == '\0' || *end != '\0') {
        g_free(s);
        return FALSE;
    }
    g_free(s);
#endif
    *val = ret;
    return TRUE;
}

bool FolderConfig::getDouble(const char* key,
                                     double* val) {
    GError* error = nullptr;
    double ret = g_key_file_get_double(keyFile_, group_.get(), key, &error);
    if(error) {
        g_error_free(error);
        return FALSE;
    }
    *val = ret;
    return TRUE;
}

bool FolderConfig::getBoolean(const char* key, bool* val) {
    GErrorPtr err;
    auto ret = g_key_file_get_boolean(keyFile_, group_.get(), key, &err);
    if(err) {
        return false;
    }
    *val = ret;
    return true;
}

char* FolderConfig::getString(const char* key) {
    return g_key_file_get_string(keyFile_, group_.get(), key, nullptr);
}

char** FolderConfig::getStringList(const char* key, gsize* length) {
    return g_key_file_get_string_list(keyFile_, group_.get(), key, length, nullptr);
}


void FolderConfig::setInteger(const char* key, int val) {
    changed_ = TRUE;
    g_key_file_set_integer(keyFile_, group_.get(), key, val);
}

void FolderConfig::setUint64(const char* key, uint64_t val) {
    changed_ = TRUE;
#if GLIB_CHECK_VERSION(2, 26, 0)
    g_key_file_set_uint64(keyFile_, group_.get(), key, val);
#else
    gchar* result = g_strdup_printf("%" G_GUINT64_FORMAT, val);
    g_key_file_set_value(keyFile_, group_.get(), key, result);
    g_free(result);
#endif
}

void FolderConfig::setDouble(const char* key, double val) {
    changed_ = TRUE;
    g_key_file_set_double(keyFile_, group_.get(), key, val);
}

void FolderConfig::setBoolean(const char* key, bool val) {
    changed_ = TRUE;
    g_key_file_set_boolean(keyFile_, group_.get(), key, val);
}

void FolderConfig::setString(const char* key, const char* string) {
    changed_ = TRUE;
    g_key_file_set_string(keyFile_, group_.get(), key, string);
}

void FolderConfig::setStringList(const char* key,
                                      const gchar* const list[], gsize length) {
    changed_ = TRUE;
    g_key_file_set_string_list(keyFile_, group_.get(), key, list, length);
}

void FolderConfig::removeKey(const char* key) {
    changed_ = TRUE;
    g_key_file_remove_key(keyFile_, group_.get(), key, nullptr);
}

void FolderConfig::purge() {
    changed_ = TRUE;
    g_key_file_remove_group(keyFile_, group_.get(), nullptr);
}

// static
void FolderConfig::saveCache(void) {
    char* out;
    gsize len;

    /* if per-directory cache was changed since last invocation then save it */
    if(fc_cache_changed && (out = g_key_file_to_data(fc_cache, &len, nullptr))) {
        /* FIXME: create dir */
        /* create temp file with settings */
        GFilePtr gfile{g_file_new_for_path(globalConfigFile_.get()), false};
        GErrorPtr err;
        /* do safe replace now, the file is important enough to be lost */
        if(g_file_replace_contents(gfile.get(), out, len, nullptr, true, G_FILE_CREATE_PRIVATE, nullptr, nullptr, &err)) {
            fc_cache_changed = FALSE;
        }
        else {
            g_warning("cannot save %s: %s", globalConfigFile_.get(), err->message);
        }
        g_free(out);
    }
}

// static
void FolderConfig::finalize(void) {
    saveCache();
    g_key_file_free(fc_cache);
    fc_cache = nullptr;
}

// static
void FolderConfig::init(const char* globalConfigFile) {
    globalConfigFile_ = CStrPtr{g_strdup(globalConfigFile)};
    fc_cache = g_key_file_new();
    if(!g_key_file_load_from_file(fc_cache, globalConfigFile_.get(), G_KEY_FILE_NONE, nullptr)) {
        // fail to load the config file.
        // fallback to the legacy libfm config file for backward compatibility
        CStrPtr legacyConfigFlie{g_build_filename(g_get_user_config_dir(), "libfm/dir-settings.conf", nullptr)};
        g_key_file_load_from_file(fc_cache, legacyConfigFlie.get(), G_KEY_FILE_NONE, nullptr);
    }
}

} // namespace Fm
