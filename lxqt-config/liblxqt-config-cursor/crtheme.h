/* Copyright © 2006-2007 Fredrik Höglund <fredrik@kde.org>
 * (c)GPL2 (c)GPL3
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2 or at your option version 3 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/*
 * additional code: Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 */
#ifndef CRTHEME_H
#define CRTHEME_H

#include <QApplication>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QImage>
#include <QString>
#include <QStringList>

#include <xcb/xcb.h>
#include <xcb/xproto.h>

///////////////////////////////////////////////////////////////////////////////
// X11 is SHIT!
struct _XcursorImage;
struct _XcursorImages;

typedef _XcursorImage XcursorImage;
typedef _XcursorImages XcursorImages;


///////////////////////////////////////////////////////////////////////////////
class XCursorThemeData
{
public:
    enum ItemDataRole {
      // Note: use   printf "0x%08X\n" $(($RANDOM*$RANDOM))
      // to define additional roles.
      DisplayDetailRole = 0x24A3DAF8
    };

    XCursorThemeData(const QDir &aDir);

    const QString &name() const { return mName; }
    const QString &title() const { return mTitle; }
    const QString &description() const { return mDescription; }
    const QString &sample() const { return mSample; }
    const QString &path() const { return mPath; }
    bool isHidden() const { return mHidden; }
    QPixmap icon() const;
    const QStringList &inherits() const { return mInherits; }

    inline void setName(const QString &name)
    {
        mName = name;
        mHash = qHash(name);
    }

    bool isWritable() const;

    /// Hash value for the internal name
    uint hash() const { return mHash; }

    /// Loads the cursor @p name, with the nominal size @p size.
    /// If the theme doesn't have the cursor @p name, it should return
    /// the default cursor from the active theme instead.
    unsigned long loadCursorHandle(const QString &name, int size=-1) const;

    /// Loads the cursor image @p name, with the nominal size @p size.
    /// The image should be autocropped to the smallest possible size.
    /// If the theme doesn't have the cursor @p name, it should return a null image.
    QImage loadImage(const QString &name, int size=-1) const;

    XcursorImage *xcLoadImage(const QString &image, int size=-1) const;
    XcursorImages *xcLoadImages(const QString &image, int size=-1) const;

    QString findAlternative(const QString &name) const;

protected:
    void parseIndexFile();

    /// Creates the icon returned by @ref icon().
    QPixmap createIcon() const;

    /// Convenience function for cropping an image.
    QImage autoCropImage(const QImage &image) const;

protected:
    QString mName;
    QString mTitle;
    QString mDescription;
    QString mPath;
    QString mSample;
    mutable QPixmap mIcon;
    bool mHidden;
    uint mHash;
    QStringList mInherits;
};

bool haveXfixes();
bool applyTheme(const XCursorThemeData &theme, int cursorSize);

QString getCurrentTheme();

#endif
