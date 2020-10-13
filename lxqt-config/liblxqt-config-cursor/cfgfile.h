/* coded by Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 * (c)DWTFYW
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */
#ifndef CFGFILE_H
#define CFGFILE_H

#include <QMultiMap>
#include <QString>

QMultiMap<QString, QString> loadCfgFile(const QString &fname, bool forceLoCase=false);
void fixXDefaults(const QString &themeName, int cursorSize);
const QString findDefaultTheme();

#endif
