/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "loadimagejob.h"
#include "mainwindow.h"
#include <QImageReader>
#include <QByteArray>
#include <qvarlengtharray.h>
#include <libexif/exif-loader.h>

using namespace LxImage;

LoadImageJob::LoadImageJob(const Fm::FilePath & filePath):
  path_{filePath} {
}

LoadImageJob::~LoadImageJob() {
}

void readExifEntry(ExifEntry* ee, void* user_data) {
    QMap<QString, QString>& exifData = *reinterpret_cast<QMap<QString, QString>*>(user_data);
    char c[100];
    auto ifd = exif_entry_get_ifd(ee);
    QString key = QString::fromUtf8(exif_tag_get_title_in_ifd(ee->tag, ifd));
    QString value = QString::fromUtf8(exif_entry_get_value(ee, c, 100));
    exifData.insert(key, value);
}

void readExifContent(ExifContent* ec, void* user_data) {
  exif_content_foreach_entry(ec, readExifEntry, user_data);
}

// This is called from the worker thread, not main thread
void LoadImageJob::exec() {
  GFileInputStream* fileStream = nullptr;
  Fm::GErrorPtr error;
  ErrorAction act = ErrorAction::RETRY;
  QByteArray imageBuffer;
  while (act == ErrorAction::RETRY && !isCancelled())
  {
    error.reset();
    if (nullptr == (fileStream = g_file_read(path_.gfile().get(), cancellable().get(), &error)))
    {
      act = emitError(error);
      continue;
    }

    // the file stream is successfully opened
    imageBuffer.truncate(0);
    GInputStream* inputStream = G_INPUT_STREAM(fileStream);
    while(!error && !isCancelled()) {
      char buffer[4096];
      error.reset();
      gssize readSize = g_input_stream_read(inputStream,
                                            buffer, 4096,
                                            cancellable().get(), &error);
      if(readSize == -1 || readSize == 0) // error or EOF
        break;
      // append the bytes read to the image buffer
      imageBuffer.append(buffer, readSize);
    }
    g_input_stream_close(inputStream, nullptr, nullptr);

    if (!error)
      break; // everything read or cancel requested

    act = emitError(error);
  }

  // FIXME: maybe it's a better idea to implement a GInputStream based QIODevice.
  if(!error && !isCancelled()) { // load the image from buffer if there are no errors
    image_ = QImage::fromData(imageBuffer);

    if(!image_.isNull()) { // if the image is loaded correctly
      // check if this file is a jpeg file
      // FIXME: can we use FmFileInfo instead if it's available?
      const Fm::CStrPtr basename = path_.baseName();
      const Fm::CStrPtr mime_type{g_content_type_guess(basename.get(), nullptr, 0, nullptr)};
      if(mime_type && strcmp(mime_type.get(), "image/jpeg") == 0) { // this is a jpeg file
        // use libexif to extract additional info embedded in jpeg files
        std::unique_ptr<ExifLoader, decltype (&exif_loader_unref)> exif_loader{exif_loader_new(), &exif_loader_unref};
        // write image data to exif loader
        exif_loader_write(exif_loader.get(), reinterpret_cast<unsigned char*>(const_cast<char *>(imageBuffer.constData())), static_cast<unsigned int>(imageBuffer.size()));
        std::unique_ptr<ExifData, decltype (&exif_data_unref)> exif_data{exif_loader_get_data(exif_loader.get()), &exif_data_unref};
        exif_loader.reset();
        if (exif_data) {
          /* reference for EXIF orientation tag:
          * https://www.impulseadventure.com/photo/exif-orientation.html */
          ExifEntry* orient_ent = exif_data_get_entry(exif_data.get(), EXIF_TAG_ORIENTATION);
          if(orient_ent) { /* orientation flag found in EXIF */
            gushort orient;
            ExifByteOrder bo = exif_data_get_byte_order(exif_data.get());
            /* bo == EXIF_BYTE_ORDER_INTEL ; */
            orient = exif_get_short (orient_ent->data, bo);
            QMatrix m;
            switch(orient) {
              case 1: /* no rotation */
                break;
              case 2:
                // mirror horizontally
                m.scale(-1, 1);
                break;
              case 3:
                m.rotate(180);
                break;
              case 4:
                // mirror vertically
                m.scale(1, -1);
                break;
              case 5:
                // transpose
                m.rotate(-90);
                m.scale(1, -1);
                break;
              case 6:
                m.rotate(90);
                break;
              case 7:
                // transverse
                m.rotate(90);
                m.scale(1, -1);
                break;
              case 8:
                m.rotate(270);
                break;
            }
            // rotate the image according to EXIF orientation tag
            if(!m.isIdentity()) {
              image_ = image_.transformed(m);
            }
          }

          // handle other EXIF tags as well
          exif_data_foreach_content(exif_data.get(), readExifContent, &exifData_);
        }
      }
    }
  }
}

