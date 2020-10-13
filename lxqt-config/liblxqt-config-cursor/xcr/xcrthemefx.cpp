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

#include "xcrthemefx.h"

#include <unistd.h>
#include <zlib.h>

#include <QSet>
#include <QStringList>
#include <QTextStream>

#include "xcrimg.h"
#include "xcrxcur.h"
#include "xcrtheme.h"


static const char *curShapeName[] = {
  "standard arrow",
  "help arrow (the one with '?')",
  "working arrow",
  "busy cursor",
  "precision select",
  "text select",
  "handwriting",
  "unavailable",
  "north (vert) resize",
  "south resize",
  "west (vert-means horiz?) resize",
  "east resize",
  "north-west resize",
  "south-east resize",
  "north-east resize",
  "south-west resize",
  "move",
  "alternate select",
  "hand",
  "button"
};


bool XCursorThemeFX::str2num (const QString &s, quint32 &res) {
  quint64 n = 0;
  if (s.isEmpty()) return false;
  for (int f = 0; f < s.length(); f++) {
    QChar ch = s.at(f);
    if (!ch.isDigit()) return false;
    n = n*10+ch.unicode()-'0';
  }
  //if (n >= (quint64)0x100000000LL) n = 0xffffffffLL;
  if (n >= (quint64)0x80000000LL) n = 0x7fffffffLL;
  res = n;
  return true;
}


QList<XCursorThemeFX::tAnimSeq> XCursorThemeFX::parseScript (const QString &script, quint32 maxFrame) {
  QList<tAnimSeq> res;
  QString scp = script; scp.replace(QLatin1String("\r"), QLatin1String("\n"));
  const QStringList scpL = scp.split(QLatin1Char('\n'), QString::SkipEmptyParts);
  for (const QString &ss : scpL) {
    const QString s = ss.simplified();
    //qDebug() << s;
    QStringList fld = s.split(QLatin1Char(','), QString::SkipEmptyParts); //BUG!BUG!BUG!
    if (fld.size() != 2) {
     qDebug() << "script error:" << s;
      qWarning() << "script error:" << s;
      continue;
    }
    // frame[s]
    int hyph = fld[0].indexOf(QLatin1Char('-'));
    tAnimSeq a;
    if (hyph == -1) {
      // just a number
      if (!str2num(fld[0], a.from)) {
       qDebug() << "script error (frame):" << s;
        qWarning() << "script error (frame):" << s;
        continue;
      }
      a.from = qMax(qMin(maxFrame, a.from), (quint32)1)-1;
      a.to = a.from;
    } else {
      // a..b
      if (!str2num(fld[0].left(hyph), a.from)) {
       qDebug() << "script error (frame from):" << s;
        qWarning() << "script error (frame from):" << s;
        continue;
      }
      a.from = qMax(qMin(maxFrame, a.from), (quint32)1)-1;
      if (!str2num(fld[0].mid(hyph+1), a.to)) {
       qDebug() << "script error (frame to):" << s;
        qWarning() << "script error (frame to):" << s;
        continue;
      }
      a.to = qMax(qMin(maxFrame, a.to), (quint32)1)-1;
    }
    // delay
    if (!str2num(fld[1], a.delay)) {
     qDebug() << "script error (delay):" << s;
      qWarning() << "script error (delay):" << s;
      continue;
    }
    if (a.delay < 10) a.delay = 10;
    qDebug() << "from" << a.from << "to" << a.to << "delay" << a.delay;
    res << a;
  }
  return res;
}


///////////////////////////////////////////////////////////////////////////////
static QByteArray zlibInflate (const void *buf, int bufSz, int destSz) {
  QByteArray res;
  z_stream stream;
  int err;

  res.resize(destSz+1);
  stream.next_in = (Bytef *)buf;
  stream.avail_in = bufSz;
  stream.zalloc = (alloc_func)0;
  stream.zfree = (free_func)0;
  stream.next_out = (Bytef *)res.data();
  stream.avail_out = destSz;

  err = inflateInit(&stream);
  if (err != Z_OK) return QByteArray();
  err = inflate(&stream, Z_SYNC_FLUSH);
  fprintf(stderr, "inflate result: %i\n", err);
  switch (err) {
    case Z_STREAM_END:
      err = inflateEnd(&stream);
      fprintf(stderr, "Z_STREAM_END: inflate result: %i\n", err);
      if (err != Z_OK) return QByteArray();
      break;
    case Z_OK:
      err = inflateEnd(&stream);
      fprintf(stderr, "Z_OK: inflate result: %i\n", err);
      if (err != Z_OK) return QByteArray();
      break;
    default: return QByteArray();
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
XCursorThemeFX::XCursorThemeFX (const QString &aFileName) : XCursorTheme() {
  if (!parseCursorFXTheme(aFileName)) {
    qDeleteAll(mList);
    mList.clear();
  }
}


bool XCursorThemeFX::parseCursorFXTheme (const QString &aFileName) {
 qDebug() << "loading" << aFileName;
  QFile fl(aFileName);
  if (!fl.open(QIODevice::ReadOnly)) return false; // shit!
  QByteArray ba(fl.readAll());
  fl.close();
  if (ba.size() < 0xb8) return false; // shit!
  int pos = 0;
  if (baGetDW(ba, pos) != 1) return false; // invalid version
  quint32 mainHdrSize = baGetDW(ba, pos);
  if (mainHdrSize < 0xb8) return false; // invalid header size
  quint32 unDataSize = baGetDW(ba, pos);
  if (unDataSize < 0x4c) return false; // no cursors anyway
  struct {
    quint32 ofs;
    quint32 len;
  } infoFields[6];
  pos = 0x84;
  for (int f = 0; f < 6; f++) {
    infoFields[f].ofs = baGetDW(ba, pos);
    infoFields[f].len = baGetDW(ba, pos);
  }
  pos = 0xb4;
  quint32 ihdrSize = baGetDW(ba, pos);
  // now read data
  pos = mainHdrSize;
 qDebug() << "reading data from" << pos;
  QByteArray unp(zlibInflate(ba.constData()+pos, ba.size()-pos, unDataSize));
  if (unp.isEmpty()) {
   qDebug() << "CursorFX: can't depack data";
    qWarning() << "CursorFX: can't depack data";
    return false;
  }
  // process info section
  for (int f = 0; f < 6; f++) {
    int len = infoFields[f].len;
    if (!len) continue;
    pos = infoFields[f].ofs;
    if ((quint32)pos >= ihdrSize || (quint32)pos+len >= ihdrSize) continue; // skip invalid one
    QByteArray sBA(unp.mid(pos, len));
    sBA.append('\0'); sBA.append('\0');
    QString s = QString::fromUtf16((const ushort *)sBA.constData()).simplified();
    switch (f) {
      case 0: setTitle(s); break;
      case 1: setAuthor(s); break;
      //case 2: setVersion(s); break;
      case 3: setSite(s); break;
      case 4: setMail(s); break;
      case 5: setDescr(s); break;
      default: ;
    }
  }
  // process resources
  QSet<quint8> shapesSeen;
  pos = ihdrSize;
 qDebug() << "resources started at hex" << QString::number(pos, 16);
  while (pos <= unp.size()-12) {
    quint32 ipos = pos; // will be fixed later
    quint32 rtype = baGetDW(unp, pos);
    quint32 basicHdrSize = baGetDW(unp, pos);
    quint32 itemSize = baGetDW(unp, pos);
   qDebug() << "pos hex:" << QString::number(pos, 16) << "rtype:" << rtype << "bhdrsz hex:" << QString::number(basicHdrSize, 16) << "itemsz hex:" << QString::number(itemSize, 16);
    if (itemSize < 12) {
      // invalid data
     qDebug() << "CursorFX: invalid data chunk size";
      qWarning() << "CursorFX: invalid data chunk size";
      return false;
    }
    pos = ipos+itemSize; // skip it
    if (rtype != 2) continue; // not cursor resource
    if (itemSize < 0x4c) {
     qDebug() << "CursorFX: invalid cursor chunk size:" << itemSize;
      qWarning() << "CursorFX: invalid cursor chunk size:" << itemSize;
      return false;
    }
    // cursor
    int cps = ipos+3*4;
    rtype = baGetDW(unp, cps);
    if (rtype != 2) {
     qDebug() << "CursorFX: invalid cursor chunk type:" << rtype;
      qWarning() << "CursorFX: invalid cursor chunk type:" << rtype;
      return false;
    }
    quint32 curShape = baGetDW(unp, cps);
    quint32 curType = baGetDW(unp, cps);
    if (curShape > 19) {
     qDebug() << "CursorFX: unknown cursor shape:" << curShape;
      qWarning() << "CursorFX: unknown cursor shape:" << curShape;
      return false;
    }
    if (curType != 1) {
     qDebug() << "skipping 'press' cursor; shape no" << curShape << "named" << curShapeName[curShape];
      continue;
      // we need only 'normal' cursors
    }
   qDebug() << "cursor shape:" << curShape;
    const char **nlst = findCursorByFXId((int)curShape);
    if (!nlst) {
      // unknown cursor type, skip it
     qDebug() << "CursorFX: skipping cursor shape:" << curShapeName[curShape];
      qWarning() << "CursorFX: skipping cursor shape:" << curShapeName[curShape];
      continue;
    }
    if (shapesSeen.contains(curShape&0xff)) {
      // unknown cursor type, skip it
     qDebug() << "CursorFX: skipping duplicate cursor shape:" << curShapeName[curShape];
      qWarning() << "CursorFX: skipping duplicate cursor shape:" << curShapeName[curShape];
      continue;
    }
    shapesSeen << (curShape&0xff);
   qDebug() << "importing cursor" << *nlst;
    quint32 unk0 = baGetDW(unp, cps); // unknown field
    quint32 frameCnt = baGetDW(unp, cps);
    if (frameCnt < 1) frameCnt = 1; // just in case
    quint32 imgWdt = baGetDW(unp, cps);
    quint32 imgHgt = baGetDW(unp, cps);
    quint32 imgDelay = baGetDW(unp, cps);
    quint32 aniFlags = baGetDW(unp, cps);
    quint32 unk1 = baGetDW(unp, cps); // unknown field
    quint32 imgXHot = baGetDW(unp, cps);
    quint32 imgYHot = baGetDW(unp, cps);
    quint32 realHdrSize = baGetDW(unp, cps);
    quint32 imgDataSize = baGetDW(unp, cps);
    quint32 addonOfs = baGetDW(unp, cps);
    quint32 addonLen = baGetDW(unp, cps);
    if (imgDelay < 10) imgDelay = 10;
    if (imgDelay >= (quint32)0x80000000LL) imgDelay = 0x7fffffffLL;
   qDebug() <<
     "cursor data:" <<
     "\n  frames:" << frameCnt <<
     "\n  width:" << imgWdt <<
     "\n  height:" << imgHgt <<
     "\n  delay:" << imgDelay <<
     "\n  flags:" << QString::number(aniFlags, 2) <<
     "\n  xhot:" << imgXHot <<
     "\n  yhot:" << imgYHot <<
     "\n  unk0:" << unk0 <<
     "\n  unk1:" << unk1 <<
     "\n  rhdata:" << QString::number(realHdrSize, 16) <<
     "\n  dataOfs:" << QString::number(ipos+realHdrSize, 16) <<
     "\n  cdataOfs:" << QString::number(ipos+basicHdrSize+addonLen, 16)
   ;
    // now check if we have enought data
    if (ipos+realHdrSize+imgDataSize > (quint32)pos) {
     qDebug() << "CursorFX: cursor data too big";
      qWarning() << "CursorFX: cursor data too big";
      return false;
    }
    // addon is the script; parse it later
    QList<tAnimSeq> aseq;
    QString script;
    if (addonLen) {
      // script
      if (addonOfs < 0x4c || addonOfs > realHdrSize) {
       qDebug() << "CursorFX: invalid addon data offset";
        qWarning() << "CursorFX: invalid addon data offset";
        return false;
      }
      QByteArray bs(unp.mid(ipos+addonOfs, addonLen));
      bs.append('\0'); bs.append('\0');
      script = QString::fromUtf16((const ushort *)bs.constData());
      qDebug() << "script:\n" << script;
      // create animseq
      aseq = parseScript(script, frameCnt);
    } else {
      // create 'standard' animseq
      tAnimSeq a;
      a.from = 0; a.to = frameCnt-1; a.delay = imgDelay;
      aseq << a;
      // and back if 'alternate' flag set
      if (aniFlags&0x01) {
        a.from = frameCnt-1; a.to = 0;
        aseq << a;
      }
    }
    if (imgWdt*imgHgt*4 != imgDataSize) {
     qDebug() << "data size:" << imgDataSize << "but should be" << imgWdt*imgHgt*4;
      continue;
    }
    // decode image
    QImage img((const uchar *)unp.constData()+ipos+realHdrSize, imgWdt, imgHgt, QImage::Format_ARGB32);
    img = img.mirrored(false, true);
    //
    XCursorImages *cim = new XCursorImages(QString::fromUtf8(*nlst));
    cim->setScript(script);
    //!!!
    //!!!img.save(QString("_png/%1.png").arg(cim->name()));
    //!!!
    quint32 frameWdt = img.width()/frameCnt;
   qDebug() << "frameWdt:" << frameWdt << "left:" << img.width()%(frameWdt*frameCnt);
    // now build animation sequence
    int fCnt = 0;
    for (const tAnimSeq &a : qAsConst(aseq)) {
      bool back = a.from > a.to; // going backwards
      quint32 fNo = a.from;
      for (;; fCnt++) {
       //k8:qDebug() << "  frame:" << fNo << "delay:" << a.delay;
        // copy frame
        QImage frame(img.copy(fNo*frameWdt, 0, frameWdt, img.height()));
        //frame.save(QString("_png/%1_%2.png").arg(cim->name()).arg(QString::number(f)));
        XCursorImage *i = new XCursorImage(QStringLiteral("%1%2").arg(cim->name(), QString::number(fCnt)),
          frame, imgXHot, imgYHot, a.delay, 1
        );
        cim->append(i);
        //
        if (fNo == a.to) break;
        if (back) fNo--; else fNo++;
      }
    }
    // append if not empty
    if (cim->count()) {
      // now check if it is looped and cancel looping if necessary
      if ((aniFlags & 0x02) == 0) {
        // not looped
       qDebug() << "  anti-loop fix";
        XCursorImage *i = cim->item(cim->count()-1);
        i->setDelay(0x7fffffffL);
        //i->setCSize(2); // ???
      }
      mList << cim;
    }
  }
  return true;
}
