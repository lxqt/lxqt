/* coded by Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 * (c)DWTFYW
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */
#ifndef XCRTHEME_H
#define XCRTHEME_H

#include <QtCore>
#include <QCursor>
#include <QPixmap>

#include "xcrimg.h"


class XCursorTheme {
public:
  XCursorTheme (const QDir &aDir, const QString &aName);
  XCursorTheme ();
  virtual ~XCursorTheme ();

  inline const QString &name () const { return mName; }
  inline const QString &path () const { return mPath; }
  inline const QString &title () const { return mTitle; }
  inline const QString &author () const { return mAuthor; }
  inline const QString &license () const { return mLicense; }
  inline const QString &mail () const { return mEMail; }
  inline const QString &site () const { return mSite; }
  inline const QString &descr () const { return mDescr; }
  inline const QString &im () const { return mIM; }
  inline const QString &sample () const { return mSample; }
  inline const QStringList &inherits () const { return mInherits; }

  inline void setName (const QString &v) { mName = v; }
  inline void setPath (const QString &v) { mPath = v; }
  inline void setTitle (const QString &v) { mTitle = v; }
  inline void setAuthor (const QString &v) { mAuthor = v; }
  inline void setLicense (const QString &v) { mLicense = v; }
  inline void setMail (const QString &v) { mEMail = v; }
  inline void setSite (const QString &v) { mSite = v; }
  inline void setDescr (const QString &v) { mDescr = v; }
  inline void setIM (const QString &v) { mIM = v; }
  inline void setSample (const QString &v) { mSample = v; }
  inline void setInherits (const QStringList &v) { mInherits.clear(); mInherits.append(v); }

  inline const QList<XCursorImages *> &list () const { return mList; }
  inline const XCursorImages *at (int idx) const { return mList.at(idx); }
  inline int count () const { return mList.size(); }
  inline int size () const { return mList.size(); }

  inline void append (XCursorImages *img) { mList << img; }

  bool writeToDir (const QDir &destDir);

  void fixInfoFields ();

  bool writeXPTheme (const QDir &destDir);

protected:
  static const char **findCursorByFXId (int id);
  static QString findCursorFile (const QDir &dir, const char *name);
  static const char **findCursorRecord (const QString &cname, int type=1);

  bool writeIndexTheme (const QDir &destDir);
  void parseXCursorTheme (const QDir &dir);
  void parseThemeIndex (const QDir &dir);

  void dumpInfo ();

protected:
  QString mName;
  QString mPath;
  QString mTitle;
  QString mAuthor;
  QString mLicense;
  QString mEMail;
  QString mSite;
  QString mDescr;
  QString mIM;
  QString mSample;
  QStringList mInherits;
  QList<XCursorImages *> mList;
};


bool removeXCursorTheme (const QDir &thDir, const QString &name);
bool removeXCursorTheme (const QString &name);
bool removeXCursorTheme (const QDir &thDir);

bool packXCursorTheme (const QString &destFName, const QDir &thDir, const QString &thName, bool removeTheme=false);


#endif
