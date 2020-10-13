#include "thumbnailjob.h"
#include <string>
#include <memory>
#include <algorithm>
#include <libexif/exif-loader.h>
#include <QImageReader>
#include <QDir>
#include "thumbnailer.h"

#include "core/legacy/fm-config.h"

namespace Fm {

QThreadPool* ThumbnailJob::threadPool_ = nullptr;

bool ThumbnailJob::localFilesOnly_ = true;
int ThumbnailJob::maxThumbnailFileSize_ = 0;

ThumbnailJob::ThumbnailJob(FileInfoList files, int size):
    files_{std::move(files)},
    size_{size},
    md5Calc_{g_checksum_new(G_CHECKSUM_MD5)} {
}

ThumbnailJob::~ThumbnailJob() {
    g_checksum_free(md5Calc_);
    // qDebug("delete  ThumbnailJob");
}

void ThumbnailJob::exec() {
    for(auto& file: files_) {
        if(isCancelled()) {
            break;
        }
        auto image = loadForFile(file);
        Q_EMIT thumbnailLoaded(file, size_, image);
        results_.emplace_back(std::move(image));
    }
}

QImage ThumbnailJob::readImageFromStream(GInputStream* stream, size_t len) {
    // FIXME: should we set a limit here? Otherwise if len is too large, we can run out of memory.
    std::unique_ptr<unsigned char[]> buffer{new unsigned char[len]}; // allocate enough buffer
    unsigned char* pbuffer = buffer.get();
    size_t totalReadSize = 0;
    while(!isCancelled() && totalReadSize < len) {
        size_t bytesToRead = totalReadSize + 4096 > len ? len - totalReadSize : 4096;
        gssize readSize = g_input_stream_read(stream, pbuffer, bytesToRead, cancellable_.get(), nullptr);
        if(readSize == 0) { // end of file
            break;
        }
        else if(readSize == -1) { // error
            return QImage();
        }
        totalReadSize += readSize;
        pbuffer += readSize;
    }
    QImage image;
    image.loadFromData(buffer.get(), totalReadSize);
    return image;
}

QImage ThumbnailJob::loadForFile(const std::shared_ptr<const FileInfo> &file) {
    if(!file->canThumbnail()) {
        return QImage();
    }

    // thumbnails are stored in $XDG_CACHE_HOME/thumbnails/large|normal|failed
    QString thumbnailDir{QString::fromUtf8(g_get_user_cache_dir())};
    thumbnailDir += QLatin1String("/thumbnails/");

    // don't make thumbnails for files inside the thumbnail directory
    if(FilePath::fromLocalPath(thumbnailDir.toLocal8Bit().constData()).isParentOf(file->dirPath())) {
        return QImage();
    }

    QLatin1String subdir = size_ > 128 ? QLatin1String("large") : QLatin1String("normal");
    thumbnailDir += subdir;

    // generate base name of the thumbnail  => {md5 of uri}.png
    auto origPath = file->path();
    CStrPtr uri;
    if(file->isSymlink()) {
        // use the symlink target in the name to update the thumbnail
        // if the file is changed to a symlink with the same time stamp
        auto target = file->target();
        if(!target.empty()) {
            uri = FilePath::fromLocalPath(target.c_str()).uri();
        }
    }
    if(!uri) {
        uri = origPath.uri();
    }

    char thumbnailName[32 + 5];
    // calculate md5 hash for the uri of the original file
    g_checksum_update(md5Calc_, reinterpret_cast<const unsigned char*>(uri.get()), -1);
    memcpy(thumbnailName, g_checksum_get_string(md5Calc_), 32);
    memcpy(thumbnailName + 32, ".png", 5);
    g_checksum_reset(md5Calc_); // reset the checksum calculator for next use

    QString thumbnailFilename = thumbnailDir;
    thumbnailFilename += QLatin1Char('/');
    thumbnailFilename += QString::fromUtf8(thumbnailName);
    // qDebug() << "thumbnail:" << file->getName().c_str() << thumbnailFilename;

    // try to load the thumbnail file if it exists
    QImage thumbnail{thumbnailFilename};
    if(thumbnail.isNull() || isThumbnailOutdated(file, thumbnail)) {
        // the existing thumbnail cannot be loaded, generate a new one

        // create the thumbnail dir as needd (FIXME: Qt file I/O is slow)
        QDir().mkpath(thumbnailDir);

        thumbnail = generateThumbnail(file, origPath, uri.get(), thumbnailFilename);
    }
    // resize to the size we need
    if(thumbnail.width() > size_ || thumbnail.height() > size_) {
        thumbnail = thumbnail.scaled(size_, size_, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return thumbnail;
}

bool ThumbnailJob::isSupportedImageType(const std::shared_ptr<const MimeType>& mimeType) const {
    if(mimeType->isImage()) {
        auto supportedTypes = QImageReader::supportedMimeTypes();
        auto found = std::find(supportedTypes.cbegin(), supportedTypes.cend(), mimeType->name());
        if(found != supportedTypes.cend())
            return true;
    }
    return false;
}

bool ThumbnailJob::isThumbnailOutdated(const std::shared_ptr<const FileInfo>& file, const QImage &thumbnail) const {
    QString thumb_mtime = thumbnail.text(QStringLiteral("Thumb::MTime"));
    return (thumb_mtime.isEmpty() || thumb_mtime.toULongLong() != file->mtime());
}

bool ThumbnailJob::readJpegExif(GInputStream *stream, QImage& thumbnail, QMatrix& matrix) {
    /* try to extract thumbnails embedded in jpeg files */
    ExifLoader* exif_loader = exif_loader_new();
    while(!isCancelled()) {
        unsigned char buf[4096];
        gssize read_size = g_input_stream_read(stream, buf, 4096, cancellable_.get(), nullptr);
        if(read_size <= 0) { // EOF or error
            break;
        }
        if(exif_loader_write(exif_loader, buf, read_size) == 0) {
            break;    // no more EXIF data
        }
    }
    ExifData* exif_data = exif_loader_get_data(exif_loader);
    exif_loader_unref(exif_loader);
    if(exif_data) {
        /* reference for EXIF orientation tag:
         * https://www.impulseadventure.com/photo/exif-orientation.html */
        ExifEntry* orient_ent = exif_data_get_entry(exif_data, EXIF_TAG_ORIENTATION);
        if(orient_ent) { /* orientation flag found in EXIF */
            gushort orient;
            ExifByteOrder bo = exif_data_get_byte_order(exif_data);
            /* bo == EXIF_BYTE_ORDER_INTEL ; */
            orient = exif_get_short(orient_ent->data, bo);
            switch(orient) {
            case 2: // mirror horizontally
                matrix.scale(-1, 1);
                break;
            case 3:
                matrix.rotate(180);
                break;
            case 4: // mirror vertically
                matrix.scale(1, -1);
                break;
            case 5: // transpose
                matrix.rotate(-90);
                matrix.scale(1, -1);
                break;
            case 6:
                matrix.rotate(90);
                break;
            case 7: // transverse
                matrix.rotate(90);
                matrix.scale(1, -1);
                break;
            case 8:
                matrix.rotate(270);
                break;
            case 1: // no rotation
            default:
                break;
            }
        }
        if(exif_data->data) { // if an embedded thumbnail is available, load it
            thumbnail.loadFromData(exif_data->data, exif_data->size);
        }
        exif_data_unref(exif_data);
    }
    return !thumbnail.isNull();
}

QImage ThumbnailJob::generateThumbnail(const std::shared_ptr<const FileInfo>& file, const FilePath& origPath, const char* uri, const QString& thumbnailFilename) {
    QImage result;
    auto mime_type = file->mimeType();
    if(isSupportedImageType(mime_type)) {
        GFileInputStreamPtr ins{g_file_read(origPath.gfile().get(), cancellable_.get(), nullptr), false};
        if(!ins)
            return QImage();
        bool fromExif = false;
        QMatrix matrix;
        if(strcmp(mime_type->name(), "image/jpeg") == 0) { // if this is a jpeg file
            // try to get the thumbnail embedded in EXIF data
            if(readJpegExif(G_INPUT_STREAM(ins.get()), result, matrix)) {
                fromExif = true;
            }
        }
        if(!fromExif) {  // not able to generate a thumbnail from the EXIF data
            // load the original file and do the scaling ourselves
            g_seekable_seek(G_SEEKABLE(ins.get()), 0, G_SEEK_SET, cancellable_.get(), nullptr);
            result = readImageFromStream(G_INPUT_STREAM(ins.get()), file->size());
        }
        g_input_stream_close(G_INPUT_STREAM(ins.get()), nullptr, nullptr);

        if(!result.isNull()) { // the image is successfully loaded
            // scale the image as needed
            int target_size = size_ > 128 ? 256 : 128;

            // only scale the original image if it's too large
            if(result.width() > target_size || result.height() > target_size) {
                result = result.scaled(target_size, target_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }

            if(!matrix.isIdentity()) { // transform the image if needed
                result = result.transformed(matrix);
            }

            // save the generated thumbnail to disk (don't save png thumbnails for JPEG EXIF thumbnails since loading them is cheap)
            if(!fromExif) {
                result.setText(QStringLiteral("Thumb::MTime"), QString::number(file->mtime()));
                result.setText(QStringLiteral("Thumb::URI"), QString::fromUtf8(uri));
                result.save(thumbnailFilename, "PNG");
            }
            // qDebug() << "save thumbnail:" << thumbnailFilename;
        }
    }
    else { // the image format is not supported, try to find an external thumbnailer
        // try all available external thumbnailers for it until sucess
        int target_size = size_ > 128 ? 256 : 128;
        file->mimeType()->forEachThumbnailer([&](const std::shared_ptr<const Thumbnailer>& thumbnailer) {
            if(thumbnailer->run(uri, thumbnailFilename.toLocal8Bit().constData(), target_size)) {
                result = QImage(thumbnailFilename);
            }
            return !result.isNull(); // return true on success, and forEachThumbnailer() will stop.
        });

        if(!result.isNull()) {
            // Some thumbnailers did not write the proper metadata required by the xdg spec to the output (such as evince-thumbnailer)
            // Here we waste some time to fix them so next time we don't need to re-generate these thumbnails. :-(
            bool changed = false;
            if(Q_UNLIKELY(result.text(QStringLiteral("Thumb::MTime")).isEmpty())) {
                result.setText(QStringLiteral("Thumb::MTime"), QString::number(file->mtime()));
                changed = true;
            }
            if(Q_UNLIKELY(result.text(QStringLiteral("Thumb::URI")).isEmpty())) {
                result.setText(QStringLiteral("Thumb::URI"), QString::fromUtf8(uri));
                changed = true;
            }
            if(Q_UNLIKELY(changed)) {
                // save the modified PNG file containing metadata to a file.
                result.save(thumbnailFilename, "PNG");
            }
        }
    }
    return result;
}

QThreadPool* ThumbnailJob::threadPool() {
    if(Q_UNLIKELY(threadPool_ == nullptr)) {
        threadPool_ = new QThreadPool();
        threadPool_->setMaxThreadCount(1);
    }
    return threadPool_;
}

void ThumbnailJob::setLocalFilesOnly(bool value) {
    localFilesOnly_ = value;
    if(fm_config) {
        fm_config->thumbnail_local = localFilesOnly_;
    }
}

void ThumbnailJob::setMaxThumbnailFileSize(int size) {
    maxThumbnailFileSize_ = size;
    if(fm_config) {
        fm_config->thumbnail_max = maxThumbnailFileSize_;
    }
}


} // namespace Fm
