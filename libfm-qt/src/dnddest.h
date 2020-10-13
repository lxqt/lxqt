/*
 * Copyright (C) 2012 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef FM_DNDDEST_H
#define FM_DNDDEST_H

#include <QMimeData>
#include "core/filepath.h"

namespace Fm {

class DndDest {
public:
    explicit DndDest();
    ~DndDest();

    void setDestPath(Fm::FilePath dest) {
        destPath_ = std::move(dest);
    }

    const Fm::FilePath& destPath() const {
        return destPath_;
    }

    static bool isSupported(const QMimeData* data);
    static bool isSupported(QString mimeType);

    bool dropMimeData(const QMimeData* data, Qt::DropAction action);

private:
    Fm::FilePath destPath_;
};

}

#endif // FM_DNDDEST_H
