#include "archiver.h"
#include "archiveritem.h"

extern "C" {
#include "core/fr-command.h"
#include "core/file-utils.h"
#include "core/fr-init.h"
}

#include <glib.h>
#include <gobject/gobject.h>

#include <QMimeDatabase>
#include <QMimeType>
#include <QFile>
#include <QDir>

#include <unordered_map>


Archiver::Archiver(QObject* parent):
    QObject(parent),
    frArchive_{fr_archive_new()},
    rootItem_{nullptr},
    busy_{false},
    isEncrypted_{false},
    uncompressedSize_{0} {

    g_signal_connect(frArchive_, "start", G_CALLBACK(&onStart), this);
    g_signal_connect(frArchive_, "done", G_CALLBACK(&onDone), this);
    g_signal_connect(frArchive_, "progress", G_CALLBACK(&onProgress), this);
    g_signal_connect(frArchive_, "message", G_CALLBACK(&onMessage), this);
    g_signal_connect(frArchive_, "stoppable", G_CALLBACK(&onStoppable), this);
    g_signal_connect(frArchive_, "working-archive", G_CALLBACK(&onWorkingArchive), this);
}

Archiver::~Archiver() {
    if(frArchive_) {
        g_signal_handlers_disconnect_by_data(frArchive_, this);
        g_object_unref(frArchive_);
    }
}

Fm::FilePath Archiver::currentArchivePath() const {
    Fm::FilePath path;
    if(frArchive_) {
        path = Fm::FilePath{frArchive_->file, true};
    }
    return path;
}

const char *Archiver::currentArchiveContentType() const {
    return frArchive_ ? frArchive_->content_type : nullptr;
}

bool Archiver::createNewArchive(const char* uri) {
    Q_EMIT invalidateContent();
    isEncrypted_ = false;
    uncompressedSize_ = 0;

    if(!fr_archive_create(frArchive_, uri)) {
        // this will trigger a queued finished() signal
        fr_archive_action_completed(frArchive_,
                                    FR_ACTION_CREATING_NEW_ARCHIVE,
                                    FR_PROC_ERROR_GENERIC,
                                    tr("Archive type not supported.").toUtf8().constData());
        return false;
    }

    // this will trigger a queued finished() signal
    fr_archive_action_completed(frArchive_,
                                FR_ACTION_CREATING_NEW_ARCHIVE,
                                FR_PROC_ERROR_NONE,
                                NULL);
    return true;
}

bool Archiver::createNewArchive(const QUrl& uri) {
    return createNewArchive(uri.toEncoded().constData());
}

bool Archiver::openArchive(const char* uri, const char* password) {
    Q_EMIT invalidateContent();
    isEncrypted_ = false;
    uncompressedSize_ = 0;

    return fr_archive_load(frArchive_, uri, password);
}

bool Archiver::openArchive(const QUrl& uri, const char* password) {
    return openArchive(uri.toEncoded().constData(), password);
}

void Archiver::reloadArchive(const char* password) {
    Q_EMIT invalidateContent();
    isEncrypted_ = false;
    uncompressedSize_ = 0;

    fr_archive_reload(frArchive_, password);
}

bool Archiver::isLoaded() const {
    return frArchive_->file != nullptr;
}

void Archiver::addFiles(GList* relativefileNames, const char* srcDirUri, const char* destDirPath, bool onlyIfNewer, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size) {
    fr_archive_add_files(frArchive_, relativefileNames, srcDirUri, destDirPath, onlyIfNewer, password, encrypt_header, compression, volume_size);
}

void Archiver::addFiles(const Fm::FilePathList& srcPaths, const char* destDirPath, bool onlyIfNewer, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size) {
    if(srcPaths.empty()) {
        return;
    }

    bool hasError = false;
    GList* relNames = nullptr;
    auto srcParent = srcPaths.front().parent();
    for(int i = srcPaths.size() - 1; i >= 0; --i) {
        const auto& path = srcPaths[i];
        // ensure that all source files are children of the same base dir
        if(!srcParent.isPrefixOf(path)) {
            hasError = true;
            break;
        }
        // get relative paths of the src files
        relNames = g_list_prepend(relNames,
                                  g_strdup(srcParent.relativePathStr(path).get()));
    }

    if(hasError) {
        // TODO: report errors properly
    }
    else {
        addFiles(relNames, srcParent.uri().get(), destDirPath, onlyIfNewer, password, encrypt_header, compression, volume_size);
    }

    freeStrsGList(relNames);
}

void Archiver::addDirectory(const char* directoryUri, const char* baseDirUri, const char* destDirPath, bool onlyIfNewer, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size) {
    fr_archive_add_directory(frArchive_, directoryUri, baseDirUri, destDirPath, onlyIfNewer, password, encrypt_header, compression, volume_size);
}

void Archiver::addDirectory(const Fm::FilePath& directory, const char* destDirPath, bool onlyIfNewer, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size) {
    auto parent = directory.parent();
    addDirectory(directory.uri().get(), parent.uri().get(), destDirPath,
                 onlyIfNewer, password, encrypt_header, compression, volume_size);
}

void Archiver::addDroppedItems(GList *item_list, const char *base_dir, const char *dest_dir, bool update, const char *password, bool encrypt_header, FrCompression compression, unsigned int volume_size) {
    fr_archive_add_dropped_items(frArchive_, item_list, base_dir, dest_dir, update, password, encrypt_header, compression, volume_size);
}

void Archiver::addDroppedItems(const Fm::FilePathList &srcPaths, const char *base_dir, const char *dest_dir, bool update, const char *password, bool encrypt_header, FrCompression compression, unsigned int volume_size) {
    GList* itemGList = nullptr;
    for(int i = srcPaths.size() - 1; i >= 0; --i) {
        const auto& path = srcPaths[i];
        itemGList = g_list_prepend(itemGList,
                                  g_strdup(path.uri().get()));
    }
    addDroppedItems(itemGList, base_dir, dest_dir, update, password, encrypt_header, compression, volume_size);
    freeStrsGList(itemGList);
}

/*
void Archive::addWithWildcard(const char* include_files, const char* exclude_files, const char* exclude_folders, const char* base_dir, const char* dest_dir, bool update, bool follow_links, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size) {
    return fr_archive_add_with_wildcard(impl_, include_files, exclude_files, exclude_folders, base_dir, dest_dir, update, follow_links, password, encrypt_header, compression, volume_size);
}
*/

void Archiver::removeFiles(GList* fileNames, FrCompression compression) {
    fr_process_clear(frArchive_->process);
    fr_archive_remove(frArchive_, fileNames, compression);
    fr_process_start(frArchive_->process);
}

void Archiver::removeFiles(const std::vector<const FileData*>& files, FrCompression compression) {
    GList* glist = nullptr;
    for(int i = files.size() - 1; i >= 0; --i) {
        glist = g_list_prepend(glist, g_strdup(files[i]->original_path));
    }
    removeFiles(glist, compression);
    freeStrsGList(glist);
}

void Archiver::extractFiles(GList* fileNames, const char* destDirUri, const char* baseDirPath, bool skip_older, bool overwrite, bool junk_path, const char* password) {
    fr_process_clear(frArchive_->process);
    fr_archive_extract(frArchive_, fileNames, destDirUri, baseDirPath, skip_older, overwrite, junk_path, password);
    fr_process_start(frArchive_->process);
}

void Archiver::extractFiles(const std::vector<const FileData*>& files, const Fm::FilePath& destDir, const char* baseDirPath, bool skip_older, bool overwrite, bool junk_path, const char* password) {
    GList* glist = nullptr;
    for(int i = files.size() - 1; i >= 0; --i) {
        glist = g_list_prepend(glist, g_strdup(files[i]->original_path));
    }
    extractFiles(glist, destDir.uri().get(), baseDirPath, skip_older, overwrite, junk_path, password);
    freeStrsGList(glist);
}

void Archiver::extractAll(const char* destDirUri, bool skip_older, bool overwrite, bool junk_path, const char* password) {
    if(!overwrite && rootItem_) {
        // if the archive has multiple files but no parent folder, make a directory
        // and extract them in it, instead of putting them among existing files
        if(rootItem_->children().size() > 1
           || (rootItem_->children().size() == 1
               && rootItem_->children()[0]->isDir()
               && (rootItem_->children()[0]->name() == nullptr
                   || strcmp(rootItem_->children()[0]->name(), ".") == 0))) { // may happen with rpm
            auto dirUrl = QUrl::fromEncoded(destDirUri);
            if(dirUrl.isLocalFile()) {
                QString dirName = archiveDisplayName().section(QStringLiteral("/"), -1);
                if(dirName.contains(QStringLiteral("."))) {
                    dirName = dirName.section(QStringLiteral("."), 0, -2);
                }
                if(dirName.isEmpty()) { // this is possible (as in ".zip")
                    dirName = QStringLiteral("lxqt-archiver-extracted");
                }

                QString extDir = dirUrl.toLocalFile();
                if(!extDir.endsWith(QStringLiteral("/"))) {
                    extDir += QStringLiteral("/");
                }
                extDir += dirName;

                // don't overwrite but make a directory
                QString suffix;
                int i = 0;
                while(QFile::exists(extDir + suffix)) {
                    suffix = QStringLiteral("-") + QString::number(i);
                    i++;
                }
                extDir += suffix;
                QDir dir(extDir);
                dir.mkpath(extDir);

                dirUrl = QUrl::fromLocalFile(extDir);
                extractFiles(nullptr, dirUrl.toEncoded().constData(), nullptr, skip_older, false, junk_path, password);
                return;
            }
        }
    }
    extractFiles(nullptr, destDirUri, nullptr, skip_older, overwrite, junk_path, password);
}

bool Archiver::extractHere(bool skip_older, bool overwrite, bool junk_path, const char* password) {
    fr_process_clear(frArchive_->process);
    if(fr_archive_extract_here(frArchive_, skip_older, overwrite, junk_path, password)) {
        fr_process_start(frArchive_->process);
        return true;
    }
    return false;
}

const char* Archiver::lastExtractionDestination() const {
    return fr_archive_get_last_extraction_destination(frArchive_);
}

void Archiver::testArchiveIntegrity(const char* password) {
    fr_archive_test(frArchive_, password);
}

QStringList Archiver::supportedCreateMimeTypes() {
    QStringList types;
    for(auto p = create_type; *p != -1; ++p) {
        types << QString::fromUtf8(mime_type_desc[*p].mime_type);
    }
    return types;
}

static QString suffixesToNameFilter(QString name, const QStringList& suffixes) {
    QString filter = std::move(name);
    filter += QLatin1String(" (");
    for(const auto& suffix: suffixes) {
        if(filter[filter.length() - 1] != QLatin1Char('(')) {
            filter += QLatin1Char(' ');
        }
        filter += QLatin1String("*.");
        filter += suffix;
    }
    filter += QLatin1String(")");
    return filter;
}

bool Archiver::isEncrypted() const {
    return isEncrypted_;
}


std::uint64_t Archiver::uncompressedSize() const {
    return uncompressedSize_;
}

QStringList Archiver::mimeDescToNameFilters(int* mimeDescIndexes) {
    QStringList filters;
    QStringList allSuffixes;
    QMimeDatabase mimeDb;
    for(auto p = mimeDescIndexes; *p != -1; ++p) {
        auto mimeType = mimeDb.mimeTypeForName(QString::fromUtf8(mime_type_desc[*p].mime_type));
        QString filter;
        if(mimeType.isValid()) {
            auto suffixes = mimeType.suffixes();
            if(suffixes.empty()) {
                suffixes.append(QString::fromUtf8(mime_type_desc[*p].default_ext));
            }
            filter = suffixesToNameFilter(mimeType.comment(), suffixes);
            allSuffixes += suffixes;
        }
        else {
            filter = tr("*%1 files (*%1)").arg(QString::fromUtf8(mime_type_desc[*p].default_ext));
        }
        filters << filter;
    }
    filters.insert(0, suffixesToNameFilter(tr("All supported formats"), allSuffixes));
    return filters;
}

QStringList Archiver::supportedCreateNameFilters() {
    return mimeDescToNameFilters(create_type);
}

QStringList Archiver::supportedOpenMimeTypes() {
    QStringList types;
    for(auto p = open_type; *p != -1; ++p) {
        types << QString::fromUtf8(mime_type_desc[*p].mime_type);
    }
    return types;

}

QStringList Archiver::supportedOpenNameFilters() {
    return mimeDescToNameFilters(open_type);
}

QStringList Archiver::supportedSaveMimeTypes() {
    QStringList types;
    for(auto p = save_type; *p != -1; ++p) {
        types << QString::fromUtf8(mime_type_desc[*p].mime_type);
    }
    return types;
}

QStringList Archiver::supportedSaveNameFilters() {
    return mimeDescToNameFilters(save_type);
}

std::string Archiver::stripTrailingSlash(std::string dirPath) {
    if(dirPath != "/" && dirPath.back() == '/') {
        dirPath.pop_back();
    }
    return dirPath;
}

void Archiver::freeStrsGList(GList* strs) {
    g_list_foreach(strs, (GFunc)g_free, nullptr);
}

void Archiver::rebuildDirTree() {
    // The archive content is listed by Archiver in a flat list
    // Let's rebuild the tree structure by mapping dir_path => [file1, file2, ...]
    rootItem_ = nullptr;
    items_.clear();
    dirMap_.clear();

    isEncrypted_ = false;
    uncompressedSize_ = 0;

    if(!frArchive_->command || !frArchive_->command->files) {
        return;
    }

    auto n_files = frArchive_->command->files->len;

    // create one ArchiverItem per file and build dir_path => ArchiverItem mappings
    items_.reserve(n_files);
    for(unsigned int i = 0; i < n_files; ++i) {
        auto fileData = reinterpret_cast<FileData*>(g_ptr_array_index(frArchive_->command->files, i));
        items_.emplace_back(new ArchiverItem{fileData, false}); // do not take ownership of the existing FileData object
        auto item = items_.back().get();
        if(item->isDir()) {
            std::string dirName = stripTrailingSlash(item->fullPath());
            dirMap_[dirName] = item;
        }

        if(fileData->encrypted) {
            isEncrypted_ = true;
        }

        uncompressedSize_ += fileData->size;
    }

    // By default, file-roller FrArchive does not creates FileData for some parent dirs.
    // For example, for the following content:
    //   /exampe/sub/dir/file1.txt
    //   /exampe/sub/dir/file2.txt
    // FrArchive only generates /example/sub/dir/, /example/sub/dir/file1.txt, /example/sub/dir/file2.txt.
    // However we still need create FileData for /, /example/, /example/sub/ to form a full dir tree.
    // So we create the missing items by ourselves :-(

    // for each item, ensure all its parent dirs exist and setup the parent-child links
    for(unsigned int i = 0; i < n_files; ++i) {
        auto item = items_[i].get();
        while(strcmp(item->fullPath(), "/")) {
            ArchiverItem* parent = nullptr;
            std::string dirName = stripTrailingSlash(item->fullPath());
            if(dirName.empty() || dirName == "/") {
                break;
            }
            // get parent dir of the current file
            dirName = Fm::CStrPtr{g_path_get_dirname(dirName.c_str())}.get();
            auto it = dirMap_.find(dirName);
            if(it == dirMap_.end()) { // parent dir is not found, create an item for it
                // Create a new FileData item for this parent dir
                auto fileData = file_data_new();

                // ensure that dir paths end with '/'
                fileData->full_path = dirName.back() == '/' ? g_strdup(dirName.c_str()) : g_strconcat(dirName.c_str(), "/", nullptr); // ensure slash
                auto full_path_len = strlen(fileData->full_path);
                if(fileData->full_path[full_path_len - 1] == '/') {
                    fileData->full_path[full_path_len] = '\0';
                }

                fileData->original_path = g_path_get_dirname(stripTrailingSlash(item->originalPath()).c_str());
                auto original_path_len = strlen(fileData->original_path);
                if(fileData->original_path[original_path_len - 1] != '/') {
                    auto tmp = fileData->original_path;
                    fileData->original_path = g_strconcat(tmp, "/", nullptr);
                    g_free(tmp);
                }

                //qDebug("op: %s, %s", fileData->original_path, item->originalPath());
                //qDebug("fp: %s, %s", fileData->full_path, item->fullPath());
                fileData->name = g_path_get_basename(dirName.c_str());
                fileData->dir = 1;
                file_data_update_content_type(fileData);
                items_.emplace_back(new ArchiverItem{fileData, true}); // take ownership of the new FileData object
                parent = items_.back().get();
                it = dirMap_.emplace(dirName, parent).first;

                // add current file to its children
                parent->addChild(item);
                item = parent; // go up one level and continue
            }
            else { // parent item found, add current file to its children
                parent = it->second;
                parent->addChild(item);
                break;
            }
        }
    }

    // if the archive is completey empty, at least generate a root node "/"
    if(dirMap_.empty()) {
        auto fileData = file_data_new();
        fileData->full_path = g_strdup("/");
        fileData->original_path = g_strdup("/");
        fileData->name = g_strdup("/");
        fileData->dir = 1;
        file_data_update_content_type(fileData);
        items_.emplace_back(new ArchiverItem{fileData, true}); // take ownership of the new FileData object
        dirMap_.emplace("/", items_.back().get());
    }
    rootItem_ = dirMap_["/"];

    /*for(auto& kv: dirMap_) {
        qDebug("dir: %s: %d", kv.first.c_str(), kv.second->children().size());
    }*/
}

void Archiver::stopCurrentAction() {
    fr_archive_stop(frArchive_);
}

QUrl Archiver::archiveUrl() const {
    QUrl ret;
    if(frArchive_->file) {
        char* uriStr = g_file_get_uri(frArchive_->file);
        ret = QUrl::fromEncoded(uriStr);
        g_free(uriStr);
    }
    return ret;
}

QString Archiver::archiveDisplayName() const {
    QString ret;
    if(frArchive_->file) {
        char* name = g_file_get_parse_name(frArchive_->file);
        ret = QString::fromUtf8(name);
        g_free(name);
    }
    return ret;
}

bool Archiver::isBusy() const {
    return busy_;
}

FrAction Archiver::currentAction() const {
    if(frArchive_->command) {
        return frArchive_->command->action;
    }
    return FR_ACTION_NONE;
}

ArchiverError Archiver::lastError() const {
    return ArchiverError{&frArchive_->error};
}

std::vector<const ArchiverItem*> Archiver::flatFileList() const {
    std::vector<const ArchiverItem*> files;
    for(const auto& item: items_) {
        if(!item->isDir()) {
            files.emplace_back(item.get());
        }
    }
    return files;
}

const ArchiverItem *Archiver::itemByPath(const char *fullPath) const {
    return nullptr;
}

const ArchiverItem *Archiver::dirTreeRoot() const {
    return rootItem_;
}

const ArchiverItem *Archiver::dirByPath(const char *path) const {
    // strip trailing /
    std::string dirPath{path};
    if(dirPath.length() > 1 && dirPath.back() == '/') {
        dirPath.pop_back();
    }
    auto it = dirMap_.find(path);
    return it != dirMap_.end() ? it->second : nullptr;
}

const ArchiverItem *Archiver::parentDir(const ArchiverItem *file) const {
    if(file && strcmp(file->fullPath(), "/")) {
        auto parentPath = Fm::CStrPtr{g_path_get_dirname(stripTrailingSlash(file->fullPath()).c_str())};
        return dirByPath(parentPath.get());
    }
    return nullptr;
}

bool Archiver::isDir(const FileData *file) const {
    return file_data_is_dir(const_cast<FileData*>(file));
}

const FileData* Archiver::fileDataByOriginalPath(const char* originalPath) {
    // FIXME: this searches using file->original_path but sometimes we want file->full_path instead :-(
    auto idx = find_path_in_file_data_array(frArchive_->command->files, originalPath);
    if(idx >= 0) {
        return reinterpret_cast<FileData*>(g_ptr_array_index(frArchive_->command->files, idx));
    }
    return nullptr;
}


// GObject signal callbacks

// NOTE: emitting Qt signals within glib signal callbacks does not work in some cases.
// We use the workaround provided here: https://bugreports.qt.io/browse/QTBUG-18434

void Archiver::onStart(FrArchive*, FrAction action, Archiver* _this) {
    //qDebug("start");

    _this->busy_ = true;
    QMetaObject::invokeMethod(_this, "start", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(FrAction, action));
}

void Archiver::onDone(FrArchive*, FrAction action, FrProcError* error, Archiver* _this) {
    //qDebug("done: %s", error && error->gerror ? error->gerror->message : "");
    // FIXME: error might become dangling pointer for queued connections. :-(

    switch(action) {
    case FR_ACTION_CREATING_NEW_ARCHIVE:  // same as listing empty content
    case FR_ACTION_CREATING_ARCHIVE:           /* creating a local archive */
    case FR_ACTION_LISTING_CONTENT:            /* listing the content of the archive */
        _this->rebuildDirTree();
        break;
    default:
        break;
    }

    _this->busy_ = false;

    QMetaObject::invokeMethod(_this, "finish", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(FrAction, action), Q_ARG(ArchiverError, error));
}

void Archiver::onProgress(FrArchive*, double fraction, Archiver* _this) {
    QMetaObject::invokeMethod(_this, "progress", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(double, fraction));
    //qDebug("progress: %lf", fraction);
}

void Archiver::onMessage(FrArchive*, const char* msg, Archiver* _this) {
    //qDebug("message: %s", msg);
    QMetaObject::invokeMethod(_this, "message", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(QString, QString::fromUtf8(msg)));
}

void Archiver::onStoppable(FrArchive*, gboolean value, Archiver* _this) {
    QMetaObject::invokeMethod(_this, "stoppableChanged", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(bool, bool(value)));
}

void Archiver::onWorkingArchive(FrCommand* comm, const char* filename, Archiver* _this) {
    // FIXME: why the first param is comm?
    //qDebug("working: %s", filename);
    Q_EMIT _this->workingArchive(QString::fromUtf8(filename));
}
