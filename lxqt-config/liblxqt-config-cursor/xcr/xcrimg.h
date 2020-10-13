/* coded by Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 * (c)DWTFYW
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */
#ifndef XCRIMG_H
#define XCRIMG_H

#include <QtCore>
#include <QCursor>
#include <QPixmap>


class XCursorImage {
public:
  XCursorImage (const QString &aName, const QImage &aImg, int aXHot=0, int aYHot=0, quint32 aDelay=50, quint32 aCSize=1);
  XCursorImage (const QString &aName);
  virtual ~XCursorImage ();

  inline bool isValid () const { return mIsValid; }
  inline const QString &name () const { return mName; }
  inline quint32 delay () const { return mDelay; }
  inline int xhot () const { return mXHot; }
  inline int yhot () const { return mYHot; }
  inline quint32 csize () const { return mCSize; }

  inline void setXHot (int v) { mXHot = v; }
  inline void setYHot (int v) { mYHot = v; }
  inline void setDelay (quint32 v) { mDelay = v; }
  inline void setName (const QString &n) { mName = n; }
  inline void setCSize (quint32 v) { mCSize = v; }

  virtual QPixmap icon () const;
  virtual QCursor cursor () const;
  virtual QImage image (int size=-1) const;

  quint32 xcurSize () const; // size in bytes

  virtual void genXCursorImg (QByteArray &res) const; // generate Xcursor image

  static void convertARGB2PreMul (QImage &img);

protected:
  virtual QPixmap createIcon () const;

protected:
  bool mIsValid;
  QString mName;
  QImage *mImage;
  quint32 mDelay; // milliseconds
  int mXHot;
  int mYHot;
  quint32 mCSize;
  mutable QPixmap mIcon;
};


class XCursorImages {
public:
  XCursorImages (const QString &aName, const QString &aPath=QLatin1String(""));
  virtual ~XCursorImages ();

  inline const QString &name () const { return mName; }
  inline const QString &path () const { return mPath; }
  inline const QString &title () const { return mTitle; }
  inline const QString &author () const { return mAuthor; }
  inline const QString &license () const { return mLicense; }
  inline const QString &mail () const { return mEMail; }
  inline const QString &site () const { return mSite; }
  inline const QString &descr () const { return mDescr; }
  inline const QString &im () const { return mIM; }
  inline const QString &script () const { return mScript; }

  inline void setName (const QString &v) { mName = v; }
  inline void setPath (const QString &v) { mPath = v; }
  inline void setTitle (const QString &v) { mTitle = v; }
  inline void setAuthor (const QString &v) { mAuthor = v; }
  inline void setLicense (const QString &v) { mLicense = v; }
  inline void setMail (const QString &v) { mEMail = v; }
  inline void setSite (const QString &v) { mSite = v; }
  inline void setDescr (const QString &v) { mDescr = v; }
  inline void setIM (const QString &v) { mIM = v; }
  inline void setScript (const QString &v) { mScript = v; }

  inline const QList<XCursorImage *> &list () const { return mList; }
  inline XCursorImage *item (int idx) { return mList.at(idx); }
  inline const XCursorImage *at (int idx) const { return mList.at(idx); }
  inline int count () const { return mList.size(); }
  inline int size () const { return mList.size(); }

  inline void append (XCursorImage *img) { mList << img; }

  QByteArray genXCursor () const; // generate Xcursor file

  QImage buildImage () const;

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
  QString mScript; // just for writing it back
  QList<XCursorImage *> mList;
};


#endif
