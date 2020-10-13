/* coded by Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 * (c)DWTFYW
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */
#include <QDebug>
//#include <QtCore>

#include "xcrxcur.h"

#include <QApplication>
#include <QStringList>
#include <QStyle>
#include <QTextCodec>
#include <QTextStream>

#include <QX11Info>


#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xfixes.h>


/*
static QImage autoCropImage (const QImage &image) {
  // Compute an autocrop rectangle for the image
  QRect r(image.rect().bottomRight(), image.rect().topLeft());
  const quint32 *pixels = reinterpret_cast<const quint32*>(image.bits());
  for (int y = 0; y < image.height(); y++) {
    for (int x = 0; x < image.width(); x++) {
      if (*(pixels++)) {
        if (x < r.left()) r.setLeft(x);
        if (x > r.right()) r.setRight(x);
        if (y < r.top()) r.setTop(y);
        if (y > r.bottom()) r.setBottom(y);
      }
    }
  }
  // Normalize the rectangle
  return image.copy(r.normalized());
}
*/


inline static quint32 getDW (const void *data) {
  const quint8 *d = (const quint8 *)data;
  d += 3;
  quint32 res = 0;
  for (int f = 4; f > 0; f--, d--) {
    res <<= 8;
    res |= *d;
  }
  return res;
}


static quint32 baGetDW (QByteArray &ba, int &pos) {
  const uchar *d = (const uchar *)ba.constData();
  d += pos+3;
  pos += 4;
  quint32 res = 0;
  for (int f = 4; f > 0; f--, d--) {
    res <<= 8;
    res |= *d;
  }
  return res;
}


///////////////////////////////////////////////////////////////////////////////
XCursorImageXCur::XCursorImageXCur (const QString &aName, const void *aImgData) : XCursorImage(aName) {
  parseImgData(aImgData);
}


XCursorImageXCur::~XCursorImageXCur () {
}


void XCursorImageXCur::parseImgData (const void *aImgData) {
  mIsValid = false;
  delete mImage; // It's fine to delete a null pointer
  mImage = 0;
  const quint32 *data = (const quint32 *)aImgData;
  if (getDW(data) != 36) return; // header size
  if (getDW(data+1) != 0xfffd0002L) return; // magic
  //if (getDW(data+2) != 1) return; // image subtype
  if (getDW(data+3) != 1) return; // version
  mCSize = getDW(data+2);
  data += 4;
  quint32 wdt = getDW(data++); // width
  quint32 hgt = getDW(data++); // height
  if (wdt > 0x7fff) return;
  if (hgt > 0x7fff) return;
/*
  quint32 xhot = getDW(data++);
  quint32 yhot = getDW(data++);
  assert(xhot <= wdt);
  assert(yhot <= hgt);
*/
  mXHot = *((const qint32 *)data); data++;
  mYHot = *((const qint32 *)data); data++;
  mDelay = getDW(data++); // milliseconds
  // got to pixels (ARGB)
  QImage img((const uchar *)data, wdt, hgt, QImage::Format_ARGB32_Premultiplied);
  mImage = new QImage(img.copy());
  mIsValid = true;
}


///////////////////////////////////////////////////////////////////////////////
XCursorImagesXCur::XCursorImagesXCur (const QDir &aDir, const QString &aName) : XCursorImages(aName, aDir.path()) {
  parseCursorFile(aDir.path()+QStringLiteral("/")+aName);
}


XCursorImagesXCur::XCursorImagesXCur (const QString &aFileName) : XCursorImages(QLatin1String(""), QLatin1String("")) {
  QString name(aFileName);
  if (name.isEmpty() || name.endsWith(QLatin1Char('/'))) return;
  int i = name.lastIndexOf(QLatin1Char('/'));
  QString dir;
  if (i < 0) dir = QLatin1String("./"); else dir = name.left(i);
  name = name.mid(i+1);
  setName(name); setPath(dir);
  parseCursorFile(aFileName);
}


bool XCursorImagesXCur::parseCursorFile (const QString &fname) {
  //qDebug() << fname;
  qDeleteAll(mList);
  mList.clear();
  QFile fl(fname);
  if (!fl.open(QIODevice::ReadOnly)) return false; // shit!
  QByteArray ba(fl.readAll());
  fl.close();
  if (ba.size() < 4*4) return false; // shit!
  if (ba[0] != 'X' || ba[1] != 'c' || ba[2] != 'u' || ba[3] != 'r') return false; // shit!
  //FIXME: add more checks!
  int pos = 4;
  quint32 hdrSize = baGetDW(ba, pos);
  if (hdrSize < 16) return false; // invalid header size
  quint32 version = baGetDW(ba, pos);
  if (version != 65536) return false; // invalid version
  quint32 ntoc = baGetDW(ba, pos);
  if (!ntoc) return true; // nothing to parse
  if (ntoc >= 65536) return false; // idiot or what?
  quint32 tocEndOfs = hdrSize+(ntoc*(3*4));
  if (tocEndOfs > (quint32)ba.size()) return false; // out of data
  pos = hdrSize;
  // parse TOC
  int cnt = -1;
  bool wasTitle = false, wasAuthor = false, wasLic = false;
  bool wasMail = false, wasSite = false, wasDescr = false, wasIM = false;
  while (ntoc-- > 0) {
    cnt++;
    quint32 type = baGetDW(ba, pos);
    pos += 4; // skip the shit
    quint32 dataOfs = baGetDW(ba, pos);
    if (type == 0xfffd0001) {
      // text
      if (dataOfs < tocEndOfs || dataOfs > (quint32)ba.size()-20) continue; // invalid text
      // parse text
      int ipos = dataOfs;
      if (baGetDW(ba, ipos) != 20) continue; // invalid header size
      if (baGetDW(ba, ipos) != 0xfffd0001) continue; // invalid type
      quint32 subT = baGetDW(ba, ipos);
      if (baGetDW(ba, ipos) != 1) continue; // invalid version
      quint32 len = baGetDW(ba, ipos);
      // check for data presence
      if (ipos+len > (quint32)ba.size()) continue; // out of data
      QByteArray stBA(ba.mid(ipos, len));
      QString s(QString::fromUtf8(stBA));
      switch (subT) {
        case 1: // copyright (author)
          if (!wasAuthor) {
            wasAuthor = true;
            mAuthor = s;
          }
          break;
        case 2: // license
          if (!wasLic) {
            wasLic = true;
            mLicense = s;
          }
          break;
        case 3: // other (description)
          if (!wasDescr) {
            wasDescr = true;
            mDescr = s;
          }
          break;
        // my extensions
        case 4: // title
          if (!wasTitle) {
            wasTitle = true;
            mTitle = s;
          }
          break;
        case 5: // e-mail
          if (!wasMail) {
            wasMail = true;
            mEMail = s;
          }
          break;
        case 6: // site
          if (!wasSite) {
            wasSite = true;
            mSite = s;
          }
          break;
        case 7: // IM
          if (!wasIM) {
            wasIM = true;
            mIM = s;
          }
          break;
      }
      continue;
    }
    if (type != 0xfffd0002) continue; // not an image, skip this one
    // image
    // check
    if (dataOfs < tocEndOfs || dataOfs > (quint32)ba.size()-36) continue; // invalid image
    // parse image
    int ipos = dataOfs;
    if (baGetDW(ba, ipos) != 36) continue; // invalid image (header size)
    if (baGetDW(ba, ipos) != 0xfffd0002) continue; // invalid type
    ipos += 4; // skip the shit
    if (baGetDW(ba, ipos) != 1) continue; // invalid image (version)
    quint32 wdt = baGetDW(ba, ipos);
    quint32 hgt = baGetDW(ba, ipos);
    if (wdt > 0x7fff || hgt > 0x7fff) continue; // invalid sizes
    // check for data presence
    if ((ipos+3*4)+(wdt*hgt*4) > (quint32)ba.size()) continue; // out of data
    // load image
    const uchar *dta = (const uchar *)ba.constData();
    dta += dataOfs;
    XCursorImage *img = new XCursorImageXCur(mName+QStringLiteral("_")+QString::number(cnt), dta);
    if (img->isValid()) mList << img; else delete img;
  }
  return true;
}
