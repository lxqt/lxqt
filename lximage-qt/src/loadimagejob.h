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

#ifndef LXIMAGE_LOADIMAGEJOB_H
#define LXIMAGE_LOADIMAGEJOB_H

#include <libfm-qt/core/filepath.h>
#include <QImage>
#include <libfm-qt/core/job.h>
#include <QMap>

namespace LxImage {

class LoadImageJob : public Fm::Job {

public:
  LoadImageJob(const Fm::FilePath & filePath);

  QImage image() const {
    return image_;
  }

  const Fm::FilePath & filePath() const {
    return path_;
  }

  QMap<QString, QString> getExifData() {
    return exifData_;
  }

private:
  ~LoadImageJob(); // prevent direct deletion

  virtual void exec() override;
  QMap<QString, QString> exifData_;

  const Fm::FilePath path_;
  QImage image_;
};

}

#endif // LXIMAGE_LOADIMAGEJOB_H
