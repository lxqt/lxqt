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

#include "xcrtheme.h"

#include <unistd.h>

#include <QStringList>
#include <QTextStream>

#include "xcrimg.h"
#include "xcrxcur.h"

/*
 0   standard arrow
 1   help arrow (the one with '?')
 2   working arrow
 3   busy cursor
 4   precision select
 5   text select
 6   handwriting
 7   unavailable
8   north (vert) resize
9   south resize
 10  west (vert-means horiz???) resize
11  east resize
 12  north-west resize
13  south-east resize
 14  north-east resize
15  south-west resize
 16  move
 17  alternate select
 18  hand
19  button
*/


static const char *nameTransTbl[] = {
  // curFX idx, curXP name, [...altnames], finalname, 0
  // end with 0
  "", // standard arrow (it's \x00)
  "Arrow",
  "left_ptr", "X_cursor", "right_ptr", "top_left_arrow", "move",
   "4498f0e0c1937ffe01fd06f973665830",
  0,
  //
  "\x04", // precision select
  "Cross",
  "tcross", "cross", "crosshair", "cross_reverse",
   "draped_box",
  0,
  //
  "\x12", // hand
  "Hand",
  "hand", "hand1", "hand2", "9d800788f1b08800ae810202380a0822",
   "e29285e634086352946a0e7090d73106",
  0,
  //
  "\x05", // text select
  "IBeam",
   "xterm",
  0,
  //
  "\x11", // alternate select
  "UpArrow",
   "center_ptr",
  0,
  //
  "\x0c",
  "SizeNWSE", // north-west resize
  "bottom_right_corner", "top_left_corner", "bd_double_arrow", "lr_angle",
   "c7088f0f3e6c8088236ef8e1e3e70000",
  0,
  //
  "\x0e", // north-east resize
  "SizeNESW",
  "bottom_left_corner", "top_right_corner", "fd_double_arrow", "ll_angle",
   "fcf1c3c7cd4491d801f1e1c78f100000",
  0,
  //
  "\x0a", // west resize
  "SizeWE",
  "sb_h_double_arrow", "left_side", "right_side", "h_double_arrow", "028006030e0e7ebffc7f7070c0600140",
   "14fef782d02440884392942c11205230",
  0,
  //
  "\x08", // north resize
  "SizeNS",
  "double_arrow", "bottom_side", "top_side", "v_double_arrow", "sb_v_double_arrow", "00008160000006810000408080010102",
   "2870a09082c103050810ffdffffe0204",
  0,
  //
  "\x01", // help arrow
  "Help",
  "question_arrow",
   "d9ce0ab605698f320427677b458ad60b",
  0,
  //
  "\x06", // handwriting
  "Handwriting",
   "pencil",
  0,
  //
  "\x02", // working arrow
  "AppStarting",
  "left_ptr_watch", "08e8e1c95fe2fc01f976f1e063a24ccd",
   "3ecb610c1bf2410f44200f48c40d3599",
  0,
  //
  "\x10", // move (???)
  "SizeAll",
   "fleur",
  0,
  //
  "\x03", // busy cursor
  "Wait",
   "watch",
  0,
  //
  "\x07", // unavailable
  "NO",
   "crossed_circle",
   "03b6e0fcb3499374a867c041f52298f0",
  0,
  0
};


/*
static QString convertFXIndexToXPName (qint32 idx) {
  int f = 0;
  do {
    if (nameTransTbl[f][0] == idx) return QString(nameTransTbl[f+1]);
    f += 2;
    while (nameTransTbl[f]) ++f;
    ++f;
  } while (nameTransTbl[f]);
  return QString();
}
*/


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


static void removeFiles (QDir &dir) {
  //
  const QFileInfoList lst = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
  for (const QFileInfo &fi : lst) {
    qDebug() << "removing" << fi.fileName() << fi.absoluteFilePath();
    QFile fl(fi.absoluteFilePath());
    fl.remove();
  }
  //
}


static void removeCursorFiles (QDir &dir) {
  QString pt = dir.path();
  if (!pt.isEmpty() && pt != QLatin1String("/")) pt += QLatin1String("/");
  const char **nlst = nameTransTbl;
  while (*nlst) {
    nlst += 2; // skip non-files
    while (*nlst) {
      QString n = QString::fromUtf8(*nlst);
      QFile fl(pt+n);
      qDebug() << "removing" << fl.fileName();
      fl.remove();
      nlst++;
    }
    nlst++;
  }
}


///////////////////////////////////////////////////////////////////////////////
const char **XCursorTheme::findCursorByFXId (int id) {
  if (id < 0 || id > 19) return 0; // invalid id
  const char **nlst = nameTransTbl;
  while (*nlst) {
    int lid = (**nlst)&0xff;
    nlst += 2; // skip unnecessary
    if (lid == id) return nlst;
    while (nlst[-1]) nlst++ ; // skip
  }
  return 0; // none found
}


/*
 * type:
 *    0: by CursorXP name
 *    1: by Xcursor name
 * return:
 *  pointer to Xcursor name list
 */
const char **XCursorTheme::findCursorRecord (const QString &cname, int type) {
  QByteArray ba(cname.toUtf8());
  const char *name = ba.constData();
  const char **nlst = nameTransTbl;
  while (*nlst) {
    nlst += 2; // skip unnecessary
    if (!type) {
      // by CursorXP name
      if (!strcmp(name, nlst[-1])) return nlst;
    } else {
      // by Xcursor name
      // find name
      const char **nx = nlst;
      while (*nx && strcmp(*nx, name)) nx++;
      if (*nx) return nlst; // we got it!
    }
    while (nlst[-1]) nlst++ ; // skip
  }
  return 0; // none found
}


QString XCursorTheme::findCursorFile (const QDir &dir, const char *name) {
  QString d(dir.path());
  if (d != QLatin1String("/")) d += QLatin1String("/");
  d += QLatin1String("cursors/");
  const char **nlst = nameTransTbl;
  while (*nlst) {
    nlst += 2; // skip unnecessary
    // find name
    const char **nx = nlst;
    while (*nx && strcmp(*nx, name)) nx++;
    if (*nx) {
      // we got it!
      //qDebug() << "found" << *nx << "(" << *nlst << ")";
      nx = nlst;
      while (*nx) {
        QString s = QString::fromUtf8(*nx);
        //qDebug() << "checking" << s;
        QFileInfo fl(d+s);
        if (fl.exists() && fl.isReadable()) {
          //qDebug() << " ok" << s;
          return s;
        }
        nx++;
      }
    }
    while (nlst[-1]) nlst++ ; // skip
  }
  return QString(); // none found
}


///////////////////////////////////////////////////////////////////////////////
XCursorTheme::XCursorTheme () :
  mName(QLatin1String("")), mPath(QLatin1String("")), mTitle(QLatin1String("")), mAuthor(QLatin1String("")), mLicense(QLatin1String("")),
  mEMail(QLatin1String("")), mSite(QLatin1String("")), mDescr(QLatin1String("")), mIM(QLatin1String("")), mSample(QStringLiteral("left_ptr"))
{
}


XCursorTheme::XCursorTheme (const QDir &aDir, const QString &aName) :
  mName(aName), mPath(aDir.path()), mTitle(QLatin1String("")), mAuthor(QLatin1String("")), mLicense(QLatin1String("")),
  mEMail(QLatin1String("")), mSite(QLatin1String("")), mDescr(QLatin1String("")), mIM(QLatin1String("")), mSample(QStringLiteral("left_ptr"))
{
  parseXCursorTheme(aDir);
}


XCursorTheme::~XCursorTheme () {
  qDeleteAll(mList);
  mList.clear();
}


bool XCursorTheme::writeToDir (const QDir &destDir) {
  bool res = true;
  QDir dir (destDir);
  dir.mkdir(QStringLiteral("cursors"));
  if (!dir.exists(QStringLiteral("cursors"))) return false;
  dir.cd(QStringLiteral("cursors"));
  removeCursorFiles(dir);
  //
  for (const XCursorImages *ci : qAsConst(mList)) {
    const char **nlst = findCursorRecord(ci->name());
    if (!nlst) continue; // unknown cursor, skip it
    qDebug() << "writing" << *nlst;
    {
      QByteArray ba(ci->genXCursor());
      QFile fo(dir.path()+QStringLiteral("/")+ci->name());
      if (fo.open(QIODevice::WriteOnly)) {
        fo.write(ba);
        fo.close();
      } else {
        res = false;
        break;
      }
    }
    // make symlinks
    const char *orig = *nlst;
    nlst++;
    while (*nlst) {
      qDebug() << "symlinking to" << orig << "as" << *nlst;
      QByteArray newName(QFile::encodeName(dir.path()+QStringLiteral("/")+QString::fromUtf8(*nlst)));
      qDebug() << "old" << orig << "new" << newName.constData();
      if (symlink(orig, newName.constData())) {
        res = false;
        break;
      }
      nlst++;
    }
    if (!res) break;
    nlst++;
  }
  if (res) res = writeIndexTheme(destDir);
  if (!res) removeCursorFiles(dir);
  return res;
}


void XCursorTheme::parseThemeIndex (const QDir &dir) {
  QString ifn = dir.path();
  if (!ifn.isEmpty() && ifn != QLatin1String("/")) ifn += QLatin1String("/");
  ifn += QLatin1String("index.theme");
  qDebug() << "reading theme index:" << ifn;
  QFile fl(ifn);
  //
  QString cmt;
  mInherits.clear();
  // read config file, drop out 'icon theme' section (IT'S BAD!)
  if (fl.open(QIODevice::ReadOnly)) {
    QTextStream stream;
    stream.setDevice(&fl);
    stream.setCodec("UTF-8");
    bool inIconTheme = false;
    QString curPath;
    while (1) {
      QString s = stream.readLine();
      if (s.isNull()) break;
      s = s.trimmed();
      if (s.isEmpty() || s[0] == QLatin1Char('#') || s[0] == QLatin1Char(';')) continue;
      if (s[0] == QLatin1Char('[')) {
        // new path
        int len = s.length()-1;
        if (s[len] == QLatin1Char(']')) len--;
        s = s.mid(1, len).simplified();
        curPath = s.toLower();
        inIconTheme = (curPath == QLatin1String("icon theme"));
        continue;
      }
      if (!inIconTheme) continue;
      int eqp = s.indexOf(QLatin1Char('='));
      if (eqp < 0) continue; // invalid entry
      QString name = s.left(eqp).simplified().toLower();
      QString value = s.mid(eqp+1).simplified();
      qDebug() << name << value;
      if (name == QLatin1String("name") && !value.isEmpty()) mTitle = value;
      else if (name == QLatin1String("comment") && !value.isEmpty()) cmt = value;
      else if (name == QLatin1String("author") && !value.isEmpty()) mAuthor = value;
      else if (name == QLatin1String("url") && !value.isEmpty()) mSite = value;
      else if (name == QLatin1String("description") && !value.isEmpty()) mDescr = value;
      else if (name == QLatin1String("example") && !value.isEmpty()) mSample = value;
      else if (name == QLatin1String("inherits") && !value.isEmpty()) mInherits << value;
    }
    fl.close();
  }
  if (mDescr.isEmpty() && !cmt.isEmpty()) mDescr = cmt;
  if (mSample.isEmpty()) mSample = QLatin1String("left_ptr");
  mInherits.removeDuplicates();
}


void XCursorTheme::dumpInfo () {
/*
  qDebug() <<
    "INFO:" <<
    "\n  name:" << mName <<
    "\n  path:" << mPath <<
    "\n  title:" << mTitle <<
    "\n  author:" << mAuthor <<
    "\n  license:" << mLicense <<
    "\n  mail:" << mEMail <<
    "\n  site:" << mSite <<
    "\n  dscr:" << mDescr <<
    "\n  im:" << mIM <<
    "\n  sample:" << mSample <<
    "\n  inherits:" << mInherits
  ;
*/
}


void XCursorTheme::parseXCursorTheme (const QDir &dir) {
  parseThemeIndex(dir);
 dumpInfo();
  const char **nlst = nameTransTbl;
  QDir dr(dir); dr.cd(QStringLiteral("cursors"));
  while (*nlst) {
    //qDebug() << "CurFX: (" << nlst[1] << ")";
    nlst += 2; // skip unnecessary
    //qDebug() << "searching" << *nlst;
    QString fn = findCursorFile(dir, *nlst);
    if (fn.isEmpty()) continue; // no such file
    //qDebug() << " Xcrusor: (" << nlst[0] << ")";
    while (nlst[-1]) nlst++ ; // skip
    //qDebug() << "  skiped: (" << nlst[1] << ")";
    qDebug() << "loading" << fn;
    XCursorImages *ci = new XCursorImagesXCur(dr, fn);
    if (ci->count()) {
      qDebug() << " OK:" << fn << "name:" << ci->name();
      if (mTitle.isEmpty() && !ci->title().isEmpty()) mTitle = ci->title();
      if (mAuthor.isEmpty() && !ci->author().isEmpty()) mAuthor = ci->author();
      if (mLicense.isEmpty() && !ci->license().isEmpty()) mLicense = ci->license();
      if (mEMail.isEmpty() && !ci->mail().isEmpty()) mEMail = ci->mail();
      if (mSite.isEmpty() && !ci->site().isEmpty()) mSite = ci->site();
      if (mDescr.isEmpty() && !ci->descr().isEmpty()) mDescr = ci->descr();
      if (mIM.isEmpty() && !ci->im().isEmpty()) mIM = ci->im();
      mList << ci;
     dumpInfo();
    } else {
      qWarning() << "can't load" << fn << nlst[-2];
      delete ci;
    }
  }
 dumpInfo();
  fixInfoFields();
 dumpInfo();
}


void XCursorTheme::fixInfoFields () {
  for (XCursorImages *ci : qAsConst(mList)) {
    if (!mTitle.isEmpty() && ci->title().isEmpty()) ci->setTitle(title());
    if (!mAuthor.isEmpty() && ci->author().isEmpty()) ci->setAuthor(author());
    if (!mLicense.isEmpty() && ci->license().isEmpty()) ci->setLicense(license());
    if (!mEMail.isEmpty() && ci->mail().isEmpty()) ci->setMail(mail());
    if (!mSite.isEmpty() && ci->site().isEmpty()) ci->setSite(site());
    if (!mDescr.isEmpty() && ci->descr().isEmpty()) ci->setDescr(descr());
    if (!mIM.isEmpty() && ci->im().isEmpty()) ci->setIM(im());
  }
}


bool XCursorTheme::writeIndexTheme (const QDir &destDir) {
  QString ifn = destDir.path();
  if (!ifn.isEmpty() && ifn != QLatin1String("/")) ifn += QLatin1String("/");
  ifn += QLatin1String("index.theme");
  qDebug() << "writing theme index:" << ifn;
  QFile fl(ifn);
  //
  QStringList cfg, inhs, iconOther;
  QString name, cmt, author, url, dscr, sample;
  inhs.append(mInherits);
  // read config file, drop out 'icon theme' section (IT'S BAD!)
  if (fl.open(QIODevice::ReadOnly)) {
    QTextStream stream;
    stream.setDevice(&fl);
    stream.setCodec("UTF-8");
    QString curPath;
    while (1) {
      QString s = stream.readLine();
      if (s.isNull()) break;
      QString orig(s);
      s = s.trimmed();
      if (s.isEmpty() || s[0] == QLatin1Char('#') || s[0] == QLatin1Char(';')) {
        if (curPath != QLatin1String("icon theme")) cfg << orig;
        continue;
      }
      if (s[0] == QLatin1Char('[')) {
        // new path
        int len = s.length()-1;
        if (s[len] == QLatin1Char(']')) len--;
        s = s.mid(1, len).simplified();
        curPath = s.toLower();
        if (curPath != QLatin1String("icon theme")) cfg << orig;
        continue;
      }
      if (curPath != QLatin1String("icon theme")) cfg << orig;
      else {
        int eqp = s.indexOf(QLatin1Char('='));
        if (eqp < 0) {
          // invalid entry
          iconOther << orig;
          continue;
        }
        QString name = s.left(eqp).simplified();
        QString value = s.mid(eqp+1).simplified();
        if (name.isEmpty()) {
          // invalid entry
          iconOther << orig;
          continue;
        }
        QString nn(name.toLower());
        if (nn == QLatin1String("name")) name = value;
        else if (nn == QLatin1String("comment")) cmt = value;
        else if (nn == QLatin1String("author")) author = value;
        else if (nn == QLatin1String("url")) url = value;
        else if (nn == QLatin1String("description")) dscr = value;
        else if (nn == QLatin1String("example")) sample = value;
        else if (nn == QLatin1String("inherits")) { if (!value.isEmpty()) inhs << value; }
        else iconOther << orig;
/*
Name=Fire Dragon
Comment=Fire Dragon Cursor Theme -- part of the 'Four Dragons' suite; (c) Sleeping Dragon, http://sleeping-dragon.deviantart.com/art/Fire-Dragon-30419542
Author=Sleeping Dragon
Url=http://sleeping-dragon.deviantart.com/art/Fire-Dragon-30419542
Description=Fire Dragon Cursor Theme -- part of the 'Four Dragons' suite
Example=left_ptr
Inherits=core
*/
      }
    }
    fl.close();
  }
  // theme file parsed; rewrite it!
  if (cfg.size() > 0 && !cfg.at(cfg.size()-1).isEmpty()) cfg << QLatin1String("");
  if (!fl.open(QIODevice::WriteOnly)) return false;
  if (name.isEmpty()) name = mTitle;
  if (author.isEmpty()) author = mAuthor;
  if (url.isEmpty()) url = mSite;
  if (dscr.isEmpty()) dscr = mDescr;
  if (cmt.isEmpty()) cmt = dscr;
  /*if (sample.isEmpty())*/ sample = QLatin1String("left_ptr");
  if (inhs.size() == 0) inhs << QStringLiteral("core");
  inhs.removeDuplicates();
 dumpInfo();
/*
  qDebug() <<
    "***INFO:" <<
    "\n  name:" << name <<
    "\n  cmt:" << cmt <<
    "\n  author:" << author <<
    "\n  site:" << url <<
    "\n  dscr:" << dscr <<
    "\n  sample:" << mSample <<
    "\n  inherits:" << inhs
  ;
*/
  {
    QTextStream stream;
    stream.setDevice(&fl);
    stream.setCodec("UTF-8");
    for (const QString &s : qAsConst(cfg)) stream << s << "\n";
    stream << "[Icon Theme]\n";
    stream << "Name=" << name << "\n";
    stream << "Comment=" << cmt << "\n";
    stream << "Author=" << author << "\n";
    stream << "Url=" << url << "\n";
    stream << "Description=" << dscr << "\n";
    stream << "Example=" << mSample << "\n";
    for (const QString &s : qAsConst(inhs)) stream << "Inherits=" << s << "\n";
  }
  fl.close();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
static bool removeXCTheme (const QDir &thDir) {
  if (thDir.exists(QStringLiteral("cursors"))) {
    QDir d(thDir);
    d.cd(QStringLiteral("cursors"));
    //removeCursorFiles(d);
    removeFiles(d);
  }
  thDir.rmdir(QStringLiteral("cursors"));
  // check if there are some other files
  QFileInfoList lst = thDir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
  bool cantKill = false;
  for (const QFileInfo &fi : qAsConst(lst)) {
    QString s(fi.fileName());
    if (s != QLatin1String("icon-theme.cache") && s != QLatin1String("index.theme")) {
      cantKill = true;
      break;
    }
  }
  // can kill this?
  if (!cantKill) {
    QDir d(thDir);
    d.remove(QStringLiteral("icon-theme.cache"));
    d.remove(QStringLiteral("index.theme"));
  }
  return true;
}


bool removeXCursorTheme (const QDir &thDir, const QString &name) {
  qDebug() << "to kill:" << thDir.path() << name;
  QDir d(thDir);
  if (!d.exists(name)) return false;
  qDebug() << "removing" << d.path() << name;
  d.cd(name);
  removeXCTheme(d);
  d.cd(QStringLiteral(".."));
  d.rmdir(name);
  return true;
}


bool removeXCursorTheme (const QString &name) {
  QDir d(QDir::homePath());
  return removeXCursorTheme(d, name);
}


bool removeXCursorTheme (const QDir &thDir) {
  QString name(thDir.path());
  while (!name.isEmpty() && name.endsWith(QLatin1Char('/'))) name.chop(1);
  int i = name.lastIndexOf(QLatin1Char('/'));
  if (i < 1) return false;
  name = name.mid(i+1);
  QDir d(thDir);
  d.cd(QStringLiteral(".."));
  return removeXCursorTheme(d, name);
}


///////////////////////////////////////////////////////////////////////////////
/*
 * returns temporary dir or empty string
 */
static bool tarDir (const QString &destFName, const QDir &pth, const QString &dir) {
  QStringList args;

  QFile fl(destFName);
  fl.remove();

  args << QStringLiteral("-c"); // create archive...
  args << QStringLiteral("-z"); // and gzip it
  QString ps(pth.path());
  if (!ps.isEmpty() && ps != QLatin1String(".")) {
    args << QStringLiteral("-C"); // dir to go
    args << ps;
  }
  args << QStringLiteral("-f"); // to file
  args << destFName;
  QString s(dir);
  if (!s.endsWith(QLatin1Char('/'))) s += QLatin1Char('/');
  args << s;

  QProcess pr;
  pr.setStandardInputFile(QStringLiteral("/dev/null"));
  pr.setStandardOutputFile(QStringLiteral("/dev/null"));
  pr.setStandardErrorFile(QStringLiteral("/dev/null"));

  pr.start(QStringLiteral("tar"), args);

  if (pr.waitForStarted()) {
    if (pr.waitForFinished()) return true;
  }

  // cleanup
  fl.remove();
  return false;
}
//tar -c -C /home/ketmar/k8prj/xcurtheme/src -f z.tar xcr/


bool packXCursorTheme (const QString &destFName, const QDir &thDir, const QString &thName, bool removeTheme) {
  if (destFName.isEmpty() || thName.isEmpty()) return false;
  QDir d(thDir);
  if (!d.cd(thName)) return false;

  bool res = tarDir(destFName, thDir, thName);
  if (res && removeTheme) {
    removeFilesAndDirs(d);
    d.cd(QStringLiteral(".."));
    d.rmdir(thName);
  }
  return res;
}


bool XCursorTheme::writeXPTheme (const QDir &destDir) {
  QString ifn = destDir.path();
  if (!ifn.isEmpty() && ifn != QLatin1String("/")) ifn += QLatin1Char('/');

  QFile fl(ifn+QStringLiteral("Scheme.ini"));
  if (fl.open(QIODevice::WriteOnly)) {
    QTextStream stream;
    stream.setDevice(&fl);
    stream.setCodec("UTF-8");
    stream << "[General]\r\n";
    stream << "Version=130\r\n";
    qDebug() << "writing images...";
    for (XCursorImages *ci : qAsConst(mList)) {
      const char **nlst = findCursorRecord(ci->name());
      if (!nlst) continue; // unknown cursor, skip it
      qDebug() << "image:" << *(nlst-1);
      QImage img(ci->buildImage());
      if (!img.save(ifn+QString::fromUtf8(*(nlst-1))+QStringLiteral(".png"))) return false;
      stream << QStringLiteral("[")+QString::fromUtf8(*(nlst-1))+QStringLiteral("]\r\n");
      stream << "StdCursor=0\r\n";
      stream << "Frames=" << ci->count() << "\r\n";
      stream << "Hot spot x=" << ci->at(0)->xhot() << "\r\n";
      stream << "Hot spot y=" << ci->at(0)->yhot() << "\r\n";
      stream << "Interval=" << (ci->at(0)->delay() == 2147483647 ? 100 : ci->at(0)->delay()) << "\r\n";
      // x2, y2? wtf?
      if (ci->count() > 1) {
        stream << "Frames=" << ci->count() << "\r\n";
        stream << "Animation style=0\r\n";
      } else {
        stream << "Frames=1\r\n";
        stream << "Animation style=0\r\n";
      }
    }
    stream << "[[Description]\r\n";
    if (!mName.isEmpty()) stream << mName << "\r\n";
    if (!mTitle.isEmpty()) stream << mTitle << "\r\n";
    if (!mAuthor.isEmpty()) stream << mAuthor << "\r\n";
    if (!mLicense.isEmpty()) stream << mLicense << "\r\n";
    if (!mEMail.isEmpty()) stream << mEMail << "\r\n";
    if (!mSite.isEmpty()) stream << mSite << "\r\n";
    if (!mDescr.isEmpty()) stream << mDescr << "\r\n";
    if (!mIM.isEmpty()) stream << mIM << "\r\n";
  }
  fl.close();
  return true;
}
