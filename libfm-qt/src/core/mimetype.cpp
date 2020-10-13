#include "mimetype.h"
#include <cstring>

#include <glib.h>
#include <gio/gio.h>

using namespace std;

namespace Fm {

std::unordered_map<const char*, std::shared_ptr<const MimeType>, CStrHash, CStrEqual> MimeType::cache_;
std::mutex MimeType::mutex_;

std::shared_ptr<const MimeType> MimeType::inodeDirectory_;  // inode/directory
std::shared_ptr<const MimeType> MimeType::inodeShortcut_;  // inode/x-shortcut
std::shared_ptr<const MimeType> MimeType::inodeMountPoint_;  // inode/mount-point
std::shared_ptr<const MimeType> MimeType::desktopEntry_; // application/x-desktop


MimeType::MimeType(const char* typeName):
    name_{g_strdup(typeName)},
    desc_{nullptr} {

    GObjectPtr<GIcon> gicon{g_content_type_get_icon(typeName), false};
    if(strcmp(typeName, "inode/directory") == 0)
        g_themed_icon_prepend_name(G_THEMED_ICON(gicon.get()), "folder");
    else if(g_content_type_can_be_executable(typeName))
        g_themed_icon_append_name(G_THEMED_ICON(gicon.get()), "application-x-executable");

    icon_ = IconInfo::fromGIcon(gicon);
}

MimeType::~MimeType () {
}

//static
std::shared_ptr<const MimeType> MimeType::fromName(const char* typeName) {
    std::shared_ptr<const MimeType> ret;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(typeName);
    if(it == cache_.end()) {
        ret = std::make_shared<MimeType>(typeName);
        cache_.insert(std::make_pair(ret->name_.get(), ret));
    }
    else {
        ret = it->second;
    }
    return ret;
}

// static
std::shared_ptr<const MimeType> MimeType::guessFromFileName(const char* fileName) {
    gboolean uncertain;
    /* let skip scheme and host from non-native names */
    auto uri_scheme = g_strstr_len(fileName, -1, "://");
    if(uri_scheme)
        fileName = strchr(uri_scheme + 3, '/');
    if(fileName == nullptr)
        fileName = "unknown";
    auto type = CStrPtr{g_content_type_guess(fileName, nullptr, 0, &uncertain)};
    return fromName(type.get());
}

} // namespace Fm
