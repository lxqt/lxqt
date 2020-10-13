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

#include "saveimagejob.h"
#include "mainwindow.h"
#include <QImageReader>
#include <QBuffer>
#include <qvarlengtharray.h>

using namespace LxImage;

SaveImageJob::SaveImageJob(const QImage & image, const Fm::FilePath & filePath):
  path_{filePath},
  image_{image},
  failed_{true}
{
}

SaveImageJob::~SaveImageJob() {
}

// This is called from the worker thread, not main thread
void SaveImageJob::exec() {
  const Fm::CStrPtr f = path_.baseName();
  char const * format = f.get();
  format = strrchr(format, '.');
  if(format) // use filename extension as the image format
    ++format;

  QBuffer imageBuffer;
  // save the image to buffer
  if(!image_.save(&imageBuffer, format)) {
    // do not create an empty file when the format is not supported
    Fm::GErrorPtr err = Fm::GErrorPtr {
                            G_IO_ERROR,
                            G_IO_ERROR_NOT_SUPPORTED,
                            tr("Cannot save with this image format!")
    };
    emitError(err, Fm::Job::ErrorSeverity::SEVERE);
    return;
  }

  GFileOutputStream* fileStream = nullptr;
  Fm::GErrorPtr error;
  ErrorAction act = ErrorAction::RETRY;
  while (act == ErrorAction::RETRY && !isCancelled())
  {
    error.reset();
    if (nullptr == (fileStream = g_file_replace(path_.gfile().get(), nullptr, false, G_FILE_CREATE_NONE, cancellable().get(), &error)))
    {
      act = emitError(error);
      continue;
    }

    // the file stream is successfually opened
    if (!isCancelled())
    {
      GOutputStream* outputStream = G_OUTPUT_STREAM(fileStream);
      g_output_stream_write_all(outputStream,
                                imageBuffer.data().constData(),
                                imageBuffer.size(),
                                nullptr,
                                cancellable().get(),
                                &error);
      g_output_stream_close(outputStream, nullptr, nullptr);
      if (!error)
      {
        // successfully written
        failed_ = false;
        break; // successfully written
      }

      act = emitError(error);
    }
  }
}
