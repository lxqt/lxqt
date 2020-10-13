/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2014  Luís Pereira <luis.artur.pereira@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef QTXDG_MIMETYPE_H
#define QTXDG_MIMETYPE_H

#include "xdgmacros.h"
#include <QMimeType>
#include <QIcon>
#include <QString>
#include <QSharedData>

#include <QDebug>

class XdgMimeTypePrivate;

//! Describes types of file or data, represented by a MIME type string.
/*! This class is an QMimeType descendent. The differences are the icon() and
 *  iconName() methods. @see icon() @see iconName()
 *
 * Some parts of the documentation are based on QMimeType documentation.
 * @see http://qt-project.org/doc/qt-5/qmimetype.html
 *
 * @author Luís Pereira (luis.artur.pereira@gmail.com)
 */

class QTXDG_API XdgMimeType : public QMimeType {
public:

    /*! Constructs an XdgMimeType object initialized with default property
        values that indicate an invalid MIME type.
        @see QMimeType::QMimeType()
    */
    XdgMimeType();

    /*! Constructs an XdgMimeType object from an QMimeType object
     *  @see QMimeType
     */
    XdgMimeType(const QMimeType& mime);

    //! Constructs an XdgMimeType object from another XdgMimeType object
    XdgMimeType(const XdgMimeType& mime);

    /*! Assigns the data of other to this XdgMimeType object.
     * @return a reference to this object.
     */
    XdgMimeType &operator=(const XdgMimeType &other);


    /*! Compares the other XdgMimeType object to this XdgMimeType object.
     * @return true if other equals this XdgMimeType object, otherwise returns
     * false. The name is the unique identifier for a mimetype, so two mimetypes
     * with the same name, are equal.
     * @see QMimeType::operator==()
     */
    bool operator==(const XdgMimeType &other) const
    {
        return QMimeType::operator==(other);
    }

    inline bool operator!=(const XdgMimeType &other) const
    {
        return !QMimeType::operator==(other);
    }

    void swap(XdgMimeType &other) noexcept;

    //! Destructs the mimetype
    ~XdgMimeType();

    //! Returns the name of the MIME type.
    /*! The same as QMimeType::name(). Provided for compatibilty with deprecated
     *  XdgMimeInfo::mimeType().
     *  @see QMimeType::name()
     */
    inline QString mimeType() const { return QMimeType::name(); }

    //! Returns an icon associated with the mimetype.
    /*! @return an icon from the current icon theme associated with the
     *  mimetype. If the icon theme doesn't provide one it returns QIcon().
     *  It gets the icon name from iconName() and then gives it to
     *  XdgIcon::fromTheme().
     *  @see iconName() @see XdgIcon::fromTheme()
     */
    QIcon icon() const;

    //! Returns an icon name associated with the mimetype.
    /*! @return an icon name from the current icon theme associated with the
     *  mimetype. If the current icon theme doesn't provide one, it returns an
     *  empty QString.
     *  The returned icon name is suitable to be given to XdgIcon::fromTheme()
     *  to load the icon.
     *  @see XdgIcon::fromTheme()
     */
    QString iconName() const;

protected:
    friend class XdgMimeTypePrivate;
    QExplicitlySharedDataPointer<XdgMimeTypePrivate> dx;


};
Q_DECLARE_SHARED(XdgMimeType)

#endif // QTXDG_MIMETYPE_H
