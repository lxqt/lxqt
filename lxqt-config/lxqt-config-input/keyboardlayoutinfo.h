/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
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

#ifndef _KEYBOARD_LAYOUT_INFO_H_
#define _KEYBOARD_LAYOUT_INFO_H_

#include <QString>
#include <QList>

struct LayoutVariantInfo {
  QString name;
  QString description;
  LayoutVariantInfo(QString _name, QString desc): name(_name), description(desc) {
  }
};

struct KeyboardLayoutInfo {
  QString description;
  QList<LayoutVariantInfo> variants;

  KeyboardLayoutInfo(QString desc = QString()): description(desc) {
  }

  const LayoutVariantInfo* findVariant(QString name) const {
    if(!name.isEmpty()) {
      for(const LayoutVariantInfo& vinfo : qAsConst(variants)) {
        if(vinfo.name == name)
          return &vinfo;
      }
    }
    return NULL;
  }
};

#endif
