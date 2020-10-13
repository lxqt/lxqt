/* coded by Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 * (c)DWTFYW
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */
#ifndef XCRXCUR_H
#define XCRXCUR_H

#include <QtCore>
#include <QCursor>
#include <QPixmap>

#include "xcrimg.h"


class XCursorImageXCur : public XCursorImage {
public:
  XCursorImageXCur (const QString &aName, const void *aImgData); // create from Xcursor file contents (ptr to image)
  virtual ~XCursorImageXCur ();

protected:
  void parseImgData (const void *aImgData);
};


class XCursorImagesXCur : public XCursorImages {
public:
  XCursorImagesXCur (const QDir &aDir, const QString &aName);
  XCursorImagesXCur (const QString &aFileName);

protected:
  bool parseCursorFile (const QString &fname);
};


#endif
