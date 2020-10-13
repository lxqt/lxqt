#ifndef FM2_THUMBNAILJOB_H
#define FM2_THUMBNAILJOB_H

#include "../libfmqtglobals.h"
#include "fileinfo.h"
#include "gioptrs.h"
#include "job.h"
#include <QThreadPool>

namespace Fm {

class LIBFM_QT_API ThumbnailJob: public Job {
    Q_OBJECT
public:

    explicit ThumbnailJob(FileInfoList files, int size);

    ~ThumbnailJob() override;

    int size() const {
        return size_;
    }

    static QThreadPool* threadPool();

    static void setLocalFilesOnly(bool value);

    static bool localFilesOnly() {
        return localFilesOnly_;
    }

    static int maxThumbnailFileSize() {
        return maxThumbnailFileSize_;
    }

    static void setMaxThumbnailFileSize(int size);

    const std::vector<QImage>& results() const {
        return results_;
    }

Q_SIGNALS:
    void thumbnailLoaded(const std::shared_ptr<const FileInfo>& file, int size, QImage thumbnail);

protected:

    void exec() override;

private:

    bool isSupportedImageType(const std::shared_ptr<const MimeType>& mimeType) const;

    bool isThumbnailOutdated(const std::shared_ptr<const FileInfo>& file, const QImage& thumbnail) const;

    QImage generateThumbnail(const std::shared_ptr<const FileInfo>& file, const FilePath& origPath, const char* uri, const QString& thumbnailFilename);

    QImage readImageFromStream(GInputStream* stream, size_t len);

    QImage loadForFile(const std::shared_ptr<const FileInfo>& file);

    bool readJpegExif(GInputStream* stream, QImage& thumbnail, QMatrix& matrix);

private:
    FileInfoList files_;
    int size_;
    std::vector<QImage> results_;
    GCancellablePtr cancellable_;
    GChecksum* md5Calc_;

    static QThreadPool* threadPool_;

    static bool localFilesOnly_;
    static int maxThumbnailFileSize_;
};

} // namespace Fm

#endif // FM2_THUMBNAILJOB_H
