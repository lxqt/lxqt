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

#include "xcrimg.h"

#include <QApplication>
#include <QPaintEngine>
#include <QStringList>
#include <QStyle>
#include <QTextCodec>
#include <QTextStream>

#include <QX11Info>


#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xfixes.h>


inline static quint8 alphaPreMul (quint8 clr, quint8 alpha) {
  quint32 c32 = clr, a32 = alpha;
  c32 = c32*a32/255;
  if (c32 > a32) c32 = a32;
  return c32&0xff;
}


static int nominalCursorSize (int iconSize) {
  for (int i = 512; i > 8; i /= 2) {
    if (i < iconSize) return i;
    if (int(i*0.75) < iconSize) return int(i*0.75);
  }
  return 8;
}


static void baPutDW (QByteArray &ba, quint32 v) {
  ba.append('\0'); ba.append('\0');
  ba.append('\0'); ba.append('\0');
  uchar *d = (uchar *)ba.data();
  d += ba.size()-4;
  for (int f = 4; f > 0; f--, d++) {
    *d = (v&0xff);
    v >>= 8;
  }
}


void XCursorImage::convertARGB2PreMul (QImage &img) {
  switch (img.format()) {
    case QImage::Format_ARGB32_Premultiplied: return;
    case QImage::Format_ARGB32: break;
    default: (void) img.convertToFormat(QImage::Format_ARGB32/*_Premultiplied*/);
  }
  (void) img.convertToFormat(QImage::Format_ARGB32_Premultiplied); // this shouldn't convert anything
  // so convert it!
  for (int y = img.height()-1; y >= 0; y--) {
    quint8 *line = (quint8 *)img.scanLine(y);
    for (int x = 0; x < img.width(); x++, line += 4) {
      // convert to 'premultiplied'
      line[0] = alphaPreMul(line[0], line[3]);
      line[1] = alphaPreMul(line[1], line[3]);
      line[2] = alphaPreMul(line[2], line[3]);
    }
  }
}


///////////////////////////////////////////////////////////////////////////////
XCursorImage::XCursorImage (const QString &aName, const QImage &aImg, int aXHot, int aYHot, quint32 aDelay, quint32 aCSize) :
  mIsValid(true), mName(aName), mImage(0), mDelay(aDelay), mXHot(aXHot), mYHot(aYHot), mCSize(aCSize)
{
  mImage = new QImage(aImg.copy());
  convertARGB2PreMul(*mImage);
}


XCursorImage::XCursorImage (const QString &aName) :
  mIsValid(false), mName(aName), mImage(0), mDelay(50), mXHot(0), mYHot(0)
{
}


XCursorImage::~XCursorImage () {
  delete mImage; // or memory will leak!
}


QPixmap XCursorImage::icon () const {
  if (mIcon.isNull()) mIcon = createIcon();
  return mIcon;
}


QCursor XCursorImage::cursor () const {
  return QCursor(icon(), mXHot, mYHot);
}


QImage XCursorImage::image (int size) const {
  if (size == -1) size = XcursorGetDefaultSize(QX11Info::display());
  if (!mImage) return QImage();
  return mImage->copy();
}


QPixmap XCursorImage::createIcon () const {
  QPixmap pixmap;
  int iconSize = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize);
  int cursorSize = nominalCursorSize(iconSize);
  QSize size = QSize(iconSize, iconSize);
  QImage img = image(cursorSize);
  if (!img.isNull()) {
    // Scale the image if it's larger than the preferred icon size
    if (img.width() > size.width() || img.height() > size.height())
      img = img.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    pixmap = QPixmap::fromImage(img);
  }
  return pixmap;
}


quint32 XCursorImage::xcurSize () const {
  if (!mImage || !mIsValid) return 0;
  quint32 res = 9*4; // header
  res += mImage->width()*mImage->height()*4;
  return res;
}


void XCursorImage::genXCursorImg (QByteArray &res) const {
  if (!mImage || !mIsValid) return;
  baPutDW(res, 36); // header size
  baPutDW(res, 0xfffd0002); // chunk type
  baPutDW(res, mCSize); // subtype
  baPutDW(res, 1); // version
  baPutDW(res, (quint32)mImage->width());
  baPutDW(res, (quint32)mImage->height());
  baPutDW(res, (quint32)mXHot);
  baPutDW(res, (quint32)mYHot);
  baPutDW(res, (quint32)mDelay);
  // now put the pixels
  QImage i = mImage->copy();
  (void) i.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  //i.convertToFormat(QImage::Format_ARGB32);
  for (int y = 0; y < i.height(); y++) {
    const uchar *sl = i.scanLine(y);
    const quint32 *d = (const quint32 *)sl;
    for (int x = 0; x < i.width(); x++, d++) baPutDW(res, *d);
  }
}


///////////////////////////////////////////////////////////////////////////////
XCursorImages::XCursorImages (const QString &aName, const QString &aPath) :
  mName(aName), mPath(aPath), mTitle(QLatin1String("")), mAuthor(QLatin1String("")), mLicense(QLatin1String("")),
  mEMail(QLatin1String("")), mSite(QLatin1String("")), mDescr(QLatin1String("")), mIM(QLatin1String(""))
{
}


XCursorImages::~XCursorImages () {
  qDeleteAll(mList);
  mList.clear();
}


QByteArray XCursorImages::genXCursor () const {
  QByteArray res;
  // build 'credits' block
  QByteArray crdBA[7];
  {
    QStringList crb;
    crb << mAuthor << mLicense << mDescr << mTitle << mEMail << mSite << mIM;
    for (int f = 0; f < crb.size(); f++) {
      QString s(crb[f]);
      if (s.isEmpty()) crdBA[f].clear(); else crdBA[f] = s.toUtf8();
    }
  }
  //
  res.append("Xcur"); // signature
  baPutDW(res, 16); // header size
  baPutDW(res, 65536); // version number
  // TOC
  quint32 cnt = 0;
  for (const XCursorImage *i : qAsConst(mList)) if (i->xcurSize() > 0) cnt++;
  // 'credits'
  for (int f = 0; f < 7; f++) if (!crdBA[f].isEmpty()) cnt++;
  //
  baPutDW(res, cnt); // # of chunks
  if (!cnt) return res;
  quint32 dataOfs = cnt*(3*4)+16;
  // add 'credits' chunks
  for (int f = 0; f < 7; f++) {
    if (crdBA[f].isEmpty()) continue;
    baPutDW(res, 0xfffd0001); // entry type
    baPutDW(res, f+1); // subtype
    baPutDW(res, dataOfs); // offset
    dataOfs += crdBA[f].size()+20;
  }
  // add image chunks
  for (const XCursorImage *i : qAsConst(mList)) {
    quint32 isz = i->xcurSize();
    if (!isz) continue;
    baPutDW(res, 0xfffd0002); // entry type
    baPutDW(res, i->csize()); // subtype
    baPutDW(res, dataOfs); // offset
    dataOfs += isz;
  }
  // credits
  for (int f = 0; f < 7; f++) {
    if (crdBA[f].isEmpty()) continue;
    baPutDW(res, 20); // header size
    baPutDW(res, 0xfffd0001); // entry type
    baPutDW(res, f+1); // subtype
    baPutDW(res, 1); // version
    baPutDW(res, crdBA[f].size()); // length
    res.append(crdBA[f]);
  }
  // images
  for (const XCursorImage *i : qAsConst(mList)) {
    quint32 isz = i->xcurSize();
    if (!isz) continue;
    i->genXCursorImg(res);
  }
  return res;
}


QImage XCursorImages::buildImage () const {
  int width = 0, height = 0, cnt = 0;
  for (const XCursorImage *i : qAsConst(mList)) {
    if (i->xcurSize() > 0) {
      QImage img = i->image();
      width = qMax(width, img.width());
      height = qMax(height, img.height());
      ++cnt;
    }
  }
  //qDebug() << cnt << "images; size" << width << "by" << height;

  QImage res(width*cnt, height, QImage::Format_ARGB32);
  QPainter p(&res);
  int x = 0;
  for (const XCursorImage *i : qAsConst(mList)) {
    if (i->xcurSize() > 0) {
      QImage img = i->image();
      p.drawImage(QPoint(x, 0), img);
      x += img.width();
    }
  }
  return res;
}
