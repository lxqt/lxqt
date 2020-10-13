/* coded by Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 * (c)DWTFYW
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */
#ifndef XCRTHEMEFX_H
#define XCRTHEMEFX_H

#include <QtCore>
#include <QCursor>
#include <QPixmap>

#include "xcrimg.h"
#include "xcrtheme.h"


class XCursorThemeFX : public XCursorTheme {
public:
  XCursorThemeFX (const QString &aFileName);

public:
  class tAnimSeq {
  public:
    quint32 from, to;
    quint32 delay;
  };

  static QList<tAnimSeq> parseScript (const QString &script, quint32 maxFrame);
  static bool str2num (const QString &s, quint32 &res);

protected:
  bool parseCursorFXTheme (const QString &aFileName);
};


#endif
