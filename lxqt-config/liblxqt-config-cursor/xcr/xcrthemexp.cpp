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

#include "xcrthemexp.h"

#include <unistd.h>
#include <zlib.h>

#include <QSet>
#include <QStringList>
#include <QTextStream>

#include "xcrimg.h"
#include "xcrxcur.h"
#include "xcrtheme.h"
#include "xcrthemefx.h"


static const char *curShapeName[] = {
  "Arrow",
  "Cross",
  "Hand",
  "IBeam",
  "UpArrow",
  "SizeNWSE",
  "SizeNESW",
  "SizeWE",
  "SizeNS",
  "Help",
  "Handwriting",
  "AppStarting",
  "SizeAll",
  "Wait",
  "NO",
  0
};


static const char *findCurShapeName (const QString &s) {
  QByteArray ba(s.toUtf8());
  const char *name = ba.constData();
  const char **nlst = curShapeName;
  while (*nlst) {
    if (!strcasecmp(name, *nlst)) return *nlst;
    nlst++;
  }
  return 0;
}


static QString findFile (const QDir &dir, const QString &name, bool fullName=false) {
  const QFileInfoList lst = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
  for (const QFileInfo &fi : lst) {
    if (!name.compare(fi.fileName(), Qt::CaseInsensitive)) {
      if (fullName) return fi.absoluteFilePath();
      return fi.fileName();
    }
  }
  return QString();
}


class CursorInfo {
public:
  CursorInfo () { clear(); curSection.clear(); }

  void clear () {
    frameCnt = 1; delay = 50; xhot = yhot = 0;
    script.clear();
    wasFrameCnt = wasXHot = wasYHot = wasDelay = wasScript = wasAStyle = wasStdCur = false;
    isStdCursor = false;
    isLooped = true; is2way = false;
  }

public:
  quint32 frameCnt, delay, xhot, yhot;
  QString script;
  bool wasFrameCnt, wasXHot, wasYHot, wasDelay, wasScript, wasAStyle, wasStdCur;
  bool isStdCursor;
  bool isLooped, is2way;
  QString curSection;
  QString nextSection;
};


/* true: EOF */
static bool readNextSection (QTextStream &stream, CursorInfo &info) {
  info.clear();
  if (info.nextSection.isEmpty()) {
    // find next section
   //qDebug() << "searching section...";
    for (;;) {
      QString s;
      info.curSection.clear();
      info.nextSection.clear();
      for (;;) {
        s = stream.readLine();
        if (s.isNull()) return true;
        s = s.trimmed();
       //qDebug() << "*" << s;
        if (s.isEmpty() || s[0] == QLatin1Char('#') || s[0] == QLatin1Char(';')) continue;
        if (s[0] == QLatin1Char('[')) break;
      }
      int len = s.length()-1;
      if (s[len] == QLatin1Char(']')) len--;
      s = s.mid(1, len);
      const char *csn = findCurShapeName(s);
      if (!csn) continue;
      // section found
      info.curSection = QString::fromUtf8(csn);
      break;
    }
  } else {
    info.curSection = info.nextSection;
    info.nextSection.clear();
  }
  // section found; read it
  for (;;) {
    QString s = stream.readLine();
    if (s.isNull()) return true;
    s = s.trimmed();
   //qDebug() << "+" << s;
    if (s.isEmpty() || s[0] == QLatin1Char('#') || s[0] == QLatin1Char(';')) continue;
    if (s[0] == QLatin1Char('[')) {
      int len = s.length()-1;
      if (s[len] == QLatin1Char(']')) len--;
      s = s.mid(1, len);
      const char *csn = findCurShapeName(s);
      if (csn) info.nextSection = QString::fromUtf8(csn); else info.nextSection.clear();
      break;
    }
    QStringList nv(s.split(QLatin1Char('=')));
    if (nv.size() != 2) continue; // invalid
    QString name = nv[0].simplified().toLower();
    quint32 num = 0;
    bool numOk = XCursorThemeFX::str2num(nv[1].trimmed(), num);
    if (!numOk) num = 0;
    if (name == QLatin1String("frames")) {
      info.frameCnt = qMax(num, (quint32)1);
      info.wasFrameCnt = true;
    } else if (name == QLatin1String("interval")) {
      info.delay = qMax(qMin(num, (quint32)0x7fffffffL), (quint32)10);
      info.wasDelay = true;
    } else if (name == QLatin1String("animation style")) {
      info.isLooped = true;
      info.is2way = (num != 0);
      info.wasAStyle = true;
    } else if (name == QLatin1String("hot spot x")) {
      info.xhot = num;
      info.wasXHot = true;
    } else if (name == QLatin1String("hot spot y")) {
      info.yhot = num;
      info.wasYHot = true;
    } else if (name == QLatin1String("framescript")) {
      // 1 or 0
    } else if (name == QLatin1String("stdcursor")) {
      info.isStdCursor = (num!=0);
      info.wasStdCur = true;
    } else if (name == QLatin1String("hot spot x2") || name == QLatin1String("hot spot y2")) {
    } else if (name == QLatin1String("stdcursor") || name == QLatin1String("hot spot x2") || name == QLatin1String("hot spot y2")) {
      // nothing
    } else {
     qDebug() << "unknown param:" << name << nv[1];
      qWarning() << "unknown param:" << name << nv[1];
    }
  }
  return false;
}


static void removeFilesAndDirs (QDir &dir) {
  //qDebug() << "dir:" << dir.path();
  // files
  QFileInfoList lst = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
  for (const QFileInfo &fi : qAsConst(lst)) {
    //qDebug() << "removing" << fi.fileName() << fi.absoluteFilePath();
    dir.remove(fi.fileName());
  }
  // dirs
  lst = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
  for (const QFileInfo &fi : qAsConst(lst)) {
    dir.cd(fi.fileName());
    removeFilesAndDirs(dir);
    dir.cd(QStringLiteral(".."));
    //qDebug() << "removing dir" << fi.fileName();
    dir.rmdir(fi.fileName());
  }
}


/*
 * returns temporary dir or empty string
 */
static QString unzipFile (const QString &zipFile) {
  QStringList args;

  char tmpDirName[18];
  strcpy(tmpDirName, "/tmp/unzXXXXXX");
  char *td = mkdtemp(tmpDirName);
  if (!td) return QString();

  QDir dir(QString::fromUtf8(td));

  args << QStringLiteral("-b"); // all files are binary
  args << QStringLiteral("-D"); // skip timestamps
  args << QStringLiteral("-n"); // never overwrite (just in case)
  args << QStringLiteral("-qq"); // be very quiet
  args << zipFile;
  args << QStringLiteral("-d") << dir.absolutePath(); // dest dir

  QProcess pr;
  pr.setStandardInputFile(QStringLiteral("/dev/null"));
  pr.setStandardOutputFile(QStringLiteral("/dev/null"));
  pr.setStandardErrorFile(QStringLiteral("/dev/null"));

  pr.start(QStringLiteral("unzip"), args);

  if (pr.waitForStarted()) {
    if (pr.waitForFinished()) return QLatin1String(td);
  }

  // cleanup
  removeFilesAndDirs(dir);
  dir.cd(QStringLiteral(".."));
  QString s = QLatin1String(strchr(td+1, '/')+1);
  //qDebug() << s;
  dir.rmdir(s);

  return QString();
}


///////////////////////////////////////////////////////////////////////////////
XCursorThemeXP::XCursorThemeXP (const QString &aFileName) : XCursorTheme() {
  QFileInfo fi(aFileName);
  if (fi.exists() && fi.isReadable()) {
    QString dst = unzipFile(aFileName);
    if (!dst.isEmpty()) {
      QDir d(dst);
      if (!parseCursorXPTheme(d)) {
        qDeleteAll(mList);
        mList.clear();
      }
      qDebug() << "doing cleanup...";
      dst.remove(0, dst.indexOf(QLatin1Char('/'), 1)+1);
      removeFilesAndDirs(d);
      d.cd(QStringLiteral(".."));
      qDebug() << dst;
      d.rmdir(dst);
    }
  }
}


bool XCursorThemeXP::parseCursorXPTheme (const QDir &thDir) {
 qDebug() << "loading" << thDir.path();
  QString ifn = findFile(thDir, QStringLiteral("scheme.ini"), true);
  if (ifn.isEmpty()) return false;
  qDebug() << "reading scheme:" << ifn;
  //
  QFile fl(ifn);
  if (!fl.open(QIODevice::ReadOnly)) return false; // no scheme --> no fun!
  QTextStream stream;
  stream.setDevice(&fl);
  stream.setCodec("UTF-8");
  CursorInfo info;
  QSet<QString> sectionsSeen;
  bool eof = false;
  do {
    eof = readNextSection(stream, info);
    if (info.curSection.isEmpty()) break;
/*
   qDebug() << "section:" << info.curSection <<
     "\n  stdcr was:" << info.wasStdCur << "value:" << info.isStdCursor <<
     "\n  frame was:" << info.wasFrameCnt << "value:" << info.frameCnt <<
     "\n  delay was:" << info.wasDelay << "value:" << info.delay <<
     "\n  xhot  was:" << info.wasXHot << "value:" << info.xhot <<
     "\n  yhot  was:" << info.wasYHot << "value:" << info.yhot <<
     "\n  style was:" << info.wasAStyle << "loop:" << info.isLooped << "2way:" << info.is2way <<
     "\n  scrpt was:" << info.wasScript << "value:" << info.script <<
     "\n  next section:" << info.nextSection
   ;
*/
    const char ** nlst = XCursorTheme::findCursorRecord(info.curSection, 0);
    QString imgFile = findFile(thDir, info.curSection+QStringLiteral(".png"), true);
    if (!sectionsSeen.contains(info.curSection) && nlst && !imgFile.isEmpty()) {
     qDebug() << "section" << info.curSection << "file:" << imgFile;
      sectionsSeen << info.curSection;
      //TODO: script
      QList<XCursorThemeFX::tAnimSeq> aseq;
      {
        // create 'standard' animseq
        XCursorThemeFX::tAnimSeq a;
        a.from = 0; a.to = info.frameCnt-1; a.delay = info.delay;
        aseq << a;
        // and back if 'alternate' flag set
        if (info.is2way) {
          a.from = info.frameCnt-1; a.to = 0;
          aseq << a;
        }
      }
      // load image
      QImage img(imgFile);
      if (!img.isNull()) {
        XCursorImages *cim = new XCursorImages(QString::fromUtf8(*nlst));
        quint32 frameWdt = img.width()/info.frameCnt;
       qDebug() << "frameWdt:" << frameWdt << "left:" << img.width()%(frameWdt*info.frameCnt);
        // now build animation sequence
        int fCnt = 0;
        for (const XCursorThemeFX::tAnimSeq &a : qAsConst(aseq)) {
          bool back = a.from > a.to; // going backwards
          quint32 fNo = a.from;
          for (;; fCnt++) {
           //k8:qDebug() << "  frame:" << fNo << "delay:" << a.delay;
            // copy frame
            QImage frame(img.copy(fNo*frameWdt, 0, frameWdt, img.height()));
            //frame.save(QString("_png/%1_%2.png").arg(cim->name()).arg(QString::number(f)));
            XCursorImage *i = new XCursorImage(QStringLiteral("%1%2").arg(cim->name(), QString::number(fCnt)),
              frame, info.xhot, info.yhot, a.delay, 1
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
          if (!info.isLooped) {
            // not looped
           qDebug() << "  anti-loop fix";
            XCursorImage *i = cim->item(cim->count()-1);
            i->setDelay(0x7fffffffL);
            //i->setCSize(2); // ???
          }
          mList << cim;
        }
      }
    }
  } while (!eof);
  return true;
}
