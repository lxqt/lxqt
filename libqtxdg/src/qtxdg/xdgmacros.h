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

#ifndef QTXDG_MACROS_H
#define QTXDG_MACROS_H

#ifdef __cplusplus
#  include <QtCore/qglobal.h>
#  ifndef QTXDG_DEPRECATED
#    define QTXDG_DEPRECATED Q_DECL_DEPRECATED
#  endif
#endif

#ifdef QTXDG_COMPILATION
    #define QTXDG_API    Q_DECL_EXPORT
#else
    #define QTXDG_API    Q_DECL_IMPORT
#endif

#if defined(QTXDG_COMPILATION) && defined(QTXDG_TESTS)
#    define QTXDG_AUTOTEST Q_DECL_EXPORT /* Build library,tests enabled */
#elif defined(QTXDG_BUILDING_TESTS)      /* Build the tests */
#    define QTXDG_AUTOTEST Q_DECL_IMPORT
#else
#    define QTXDG_AUTOTEST               /* Building library, tests disabled */
#endif

#ifndef QL1S
#define QL1S(x) QLatin1String(x)
#endif

#ifndef QL1C
#define QL1C(x) QLatin1Char(x)
#endif

#ifndef QSL
#define QSL(x) QStringLiteral(x)
#endif

#ifndef QBAL
#define QBAL(x) QByteArrayLiteral(x)
#endif

#endif // QTXDG_MACROS_H
