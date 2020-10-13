#include "templates.h"
#include "gioptrs.h"
#include "core/legacy/fm-config.h"

#include <algorithm>
#include <QDebug>

using namespace std;

namespace Fm {

std::weak_ptr<Templates> Templates::globalInstance_;

TemplateItem::TemplateItem(std::shared_ptr<const FileInfo> file): fileInfo_{file} {
}

FilePath TemplateItem::filePath() const {
    auto& target = fileInfo_->target();
    if(fileInfo_->isDesktopEntry() && !target.empty()) {
        if(target[0] == '/') { // target is an absolute path
            return FilePath::fromLocalPath(target.c_str());
        }
        else { // resolve relative path
            return fileInfo_->dirPath().relativePath(target.c_str());
        }
    }
    return fileInfo_->path();
}

Templates::Templates() : QObject() {
    if(!fm_config || !fm_config->only_user_templates) {
        auto* data_dirs = g_get_system_data_dirs();
        // system-wide template dirs
        for(auto data_dir = data_dirs; *data_dir; ++data_dir) {
            CStrPtr dir_name{g_build_filename(*data_dir, "templates", nullptr)};
            addTemplateDir(dir_name.get());
        }
    }

    // user-specific template dir
    CStrPtr dir_name{g_build_filename(g_get_user_data_dir(), "templates", nullptr)};
    addTemplateDir(dir_name.get());

    // $XDG_TEMPLATES_DIR (FIXME: this might change at runtime)
    const gchar *special_dir = g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES);
    if (special_dir) {
        addTemplateDir(special_dir);
    }
}

shared_ptr<Templates> Templates::globalInstance() {
    auto templates = globalInstance_.lock();
    if(!templates) {
        templates = make_shared<Templates>();
        globalInstance_ = templates;
    }
    return templates;
}

void Templates::addTemplateDir(const char* dirPathName) {
    auto dir_path = FilePath::fromLocalPath(dirPathName);
    if(dir_path.isValid()) {
        auto folder = Folder::fromPath(dir_path);
        if(folder->isLoaded()) {
            bool typeOnce(fm_config && fm_config->template_type_once);
            const auto files = folder->files();
            for(auto& file : files) {
                if(!typeOnce || std::find(types_.cbegin(), types_.cend(), file->mimeType()) == types_.cend()) {
                    items_.emplace_back(std::make_shared<TemplateItem>(file));
                    if(typeOnce) {
                        types_.emplace_back(file->mimeType());
                    }
                }
            }
        }
        connect(folder.get(), &Folder::filesAdded, this, &Templates::onFilesAdded);
        connect(folder.get(), &Folder::filesChanged, this, &Templates::onFilesChanged);
        connect(folder.get(), &Folder::filesRemoved, this, &Templates::onFilesRemoved);
        connect(folder.get(), &Folder::removed, this, &Templates::onTemplateDirRemoved);
        templateFolders_.emplace_back(std::move(folder));
    }
}

void Templates::onFilesAdded(FileInfoList& addedFiles) {
    for(auto& file : addedFiles) {
        // FIXME: we do not support subdirs right now (only XFCE supports this)
        if(file->isHidden() || file->isDir()) {
            continue;
        }
        bool typeOnce(fm_config && fm_config->template_type_once);
        if(!typeOnce || std::find(types_.cbegin(), types_.cend(), file->mimeType()) == types_.cend()) {
            items_.emplace_back(std::make_shared<TemplateItem>(file));
            if(typeOnce) {
                types_.emplace_back(file->mimeType());
            }
            // emit a signal for the addition
            Q_EMIT itemAdded(items_.back());
        }
    }
}

void Templates::onFilesChanged(std::vector<FileInfoPair>& changePairs) {
    for(auto& change: changePairs) {
        auto& old_file = change.first;
        auto& new_file = change.second;
        auto it = std::find_if(items_.begin(), items_.end(), [&](const std::shared_ptr<TemplateItem>& item) {
            return item->fileInfo() == old_file;
        });
        if(it != items_.end()) {
            // emit a signal for the change
            auto old = *it;
            *it = std::make_shared<TemplateItem>(new_file);
            Q_EMIT itemChanged(old, *it);
        }
    }
}

void Templates::onFilesRemoved(FileInfoList& removedFiles) {
    for(auto& file : removedFiles) {
        auto filePath = file->path();
        auto it = std::remove_if(items_.begin(), items_.end(), [&](const std::shared_ptr<TemplateItem>& item) {
            return item->fileInfo() == file;
        });
        for(auto removed_it = it; it != items_.end(); ++it) {
            // emit a signal for the removal
            Q_EMIT itemRemoved(*removed_it);
        }
        items_.erase(it, items_.end());
    }
}

void Templates::onTemplateDirRemoved() {
    // the whole template dir is removed
    auto folder = static_cast<Folder*>(sender());
    if(!folder) {
        return;
    }
    auto dirPath = folder->path();

    // remote all files under this dir
    auto it = std::remove_if(items_.begin(), items_.end(), [&](const std::shared_ptr<TemplateItem>& item) {
        return dirPath.isPrefixOf(item->filePath());
    });
    for(auto removed_it = it; it != items_.end(); ++it) {
        // emit a signal for the removal
        Q_EMIT itemRemoved(*removed_it);
    }
    items_.erase(it, items_.end());
}

} // namespace Fm
