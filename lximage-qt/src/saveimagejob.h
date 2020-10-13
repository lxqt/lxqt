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

#ifndef LXIMAGE_SAVEIMAGEJOB_H
#define LXIMAGE_SAVEIMAGEJOB_H

#include <libfm-qt/core/job.h>
#include <libfm-qt/core/filepath.h>
#include <QImage>

namespace LxImage {

class SaveImageJob : public Fm::Job {

public:
  SaveImageJob(const QImage & image, const Fm::FilePath & filePath);

  QImage image() const {
    return image_;
  }

  const Fm::FilePath & filePath() const {
    return path_;
  }

  bool failed() const
  {
      return failed_;
  }

protected:
  virtual void exec() override;

private:
  ~SaveImageJob(); // prevent direct deletion

  const Fm::FilePath path_;
  const QImage image_;
  bool failed_;
};

}

#endif // LXIMAGE_SAVEIMAGEJOB_H
