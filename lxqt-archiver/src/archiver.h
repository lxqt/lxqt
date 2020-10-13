#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "core/fr-archive.h"
#include "archivererror.h"

#include <libfm-qt/core/filepath.h>

#include <QObject>
#include <QUrl>

#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>


class ArchiverItem;

class Archiver : public QObject {
    Q_OBJECT
public:

    explicit Archiver(QObject* parent = nullptr);

    ~Archiver();

    Fm::FilePath currentArchivePath() const;

    const char* currentArchiveContentType() const;

    bool createNewArchive(const char* uri);

    bool createNewArchive(const QUrl& uri);

    bool openArchive(const char* uri, const char* password);

    bool openArchive(const QUrl& uri, const char* password);

    void reloadArchive(const char* password);

    bool isLoaded() const;

    bool isBusy() const;

    void stopCurrentAction();

    QUrl archiveUrl() const;

    QString archiveDisplayName() const;

    FrAction currentAction() const;

    ArchiverError lastError() const;

    std::vector<const ArchiverItem *> flatFileList() const;

    const ArchiverItem* dirTreeRoot() const;

    const ArchiverItem* dirByPath(const char* path) const;

    const ArchiverItem* parentDir(const ArchiverItem* file) const;

    const ArchiverItem* itemByPath(const char* fullPath) const;

    bool isDir(const FileData* file) const;

    const FileData* fileDataByOriginalPath(const char* fullPath);

    void addFiles(GList* relativeFileNames, const char* srcDirUri, const char* destDirPath,
                  bool onlyIfNewer, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size);

    void addFiles(const Fm::FilePathList& srcPaths, const char* destDirPath,
                  bool onlyIfNewer, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size);

    void addDirectory(const char* directoryUri, const char* baseDirUri, const char* destDirPath,
                      bool onlyIfNewer, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size);

    void addDirectory(const Fm::FilePath& directory, const char* destDirPath,
                      bool onlyIfNewer, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size);

    /*
    void addWithWildcard(const char* include_files, const char* exclude_files, const char* exclude_folders, const char* base_dir, const char* dest_dir, bool update, bool follow_links, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size);

    void addWithWildcard(const char* include_files, const char* exclude_files, const char* exclude_folders, const QUrl& base_dir, const QUrl& dest_dir, bool update, bool follow_links, const char* password, bool encrypt_header, FrCompression compression, unsigned int volume_size);

    */

    void addDroppedItems(GList *item_list, const char *base_dir, const char *dest_dir, bool update, const char *password, bool encrypt_header, FrCompression compression, unsigned int volume_size);

    void addDroppedItems(const Fm::FilePathList& srcPaths, const char *base_dir, const char *dest_dir, bool update, const char *password, bool encrypt_header, FrCompression compression, unsigned int volume_size);

    void removeFiles(GList* fileNames, FrCompression compression);

    void removeFiles(const std::vector<const FileData *> &files, FrCompression compression);

    void extractFiles(GList* fileNames, const char* destDirUri, const char* baseDirPath, bool skip_older, bool overwrite, bool junk_path, const char* password);

    void extractFiles(const std::vector<const FileData*>& files, const Fm::FilePath& destDir, const char* baseDirPath, bool skip_older, bool overwrite, bool junk_path, const char* password);

    void extractAll(const char* destDirUri, bool skip_older, bool overwrite, bool junk_path, const char* password);

    bool extractHere(bool skip_older, bool overwrite, bool junk_path, const char* password);

    const char* lastExtractionDestination() const;

    void testArchiveIntegrity(const char* password);

    static QStringList supportedCreateMimeTypes();

    static QStringList supportedCreateNameFilters();

    static QStringList supportedOpenMimeTypes();

    static QStringList supportedOpenNameFilters();

    static QStringList supportedSaveMimeTypes();

    static QStringList supportedSaveNameFilters();

    bool isEncrypted() const;

    std::uint64_t uncompressedSize() const;

Q_SIGNALS:

    void invalidateContent();  // after receiving this signal, all old FileData* pointers are invalidated

    void start(FrAction action);

    void finish(FrAction action, ArchiverError error);

    void progress(double fraction);

    void message(QString msg);

    void stoppableChanged(bool value);

    void workingArchive(QString filename);

public Q_SLOTS:

private:

    static std::string stripTrailingSlash(std::string dirPath);

    static void freeStrsGList(GList* strs);

    void rebuildDirTree();

    static QStringList mimeDescToNameFilters(int *mimeDescIndexes);

private:
    // GObject signal callbacks
    static void onStart(FrArchive*, FrAction action, Archiver* _this);

    static void onDone(FrArchive*, FrAction action, FrProcError* error, Archiver* _this);

    static void onProgress(FrArchive*, double fraction, Archiver* _this);

    static void onMessage(FrArchive*, const char* msg, Archiver* _this);

    static void onStoppable(FrArchive*, gboolean value, Archiver* _this);

    static void onWorkingArchive(FrCommand* comm, const char* filename, Archiver* _this);

private:
    FrArchive* frArchive_;
    std::unordered_map<std::string, ArchiverItem*> dirMap_;
    std::vector<std::unique_ptr<ArchiverItem>> items_;
    ArchiverItem* rootItem_;
    bool busy_;
    bool isEncrypted_;
    std::uint64_t uncompressedSize_;
};

Q_DECLARE_METATYPE(FrAction)

#endif // ARCHIVE_H
