/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Christopher "VdoP" Regali
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include <QDebug>

// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <assert.h>

#include <QX11Info>
#include <QIcon>

#include "xfitman.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/**
 * @file xfitman.cpp
 * @brief implements class Xfitman
 * @author Christopher "VdoP" Regali
 */

/*
 S ome requests from Cli*ents include type of the Client, for example the _NET_ACTIVE_WINDOW
 message. Currently the types can be 1 for normal applications, and 2 for pagers.
 See http://standards.freedesktop.org/wm-spec/latest/ar01s09.html#sourceindication
 */
#define SOURCE_NORMAL 1
#define SOURCE_PAGER 2

const XfitMan& xfitMan()
{
    static XfitMan instance;
    return instance;
}

/**
 * @brief constructor: gets Display vars and registers us
 */
XfitMan::XfitMan()
{
    root = (Window)QX11Info::appRootWindow();
}

Atom XfitMan::atom(const char* atomName)
{
    static QHash<QString, Atom> hash;
    if (hash.contains(QString::fromUtf8(atomName)))
        return hash.value(QString::fromUtf8(atomName));
    Atom atom = XInternAtom(QX11Info::display(), atomName, false);
    hash[QString::fromUtf8(atomName)] = atom;
    return atom;
}

/**
 * @brief moves a window to a new position
 */

void XfitMan::moveWindow(Window _win, int _x, int _y) const
{
    XMoveWindow(QX11Info::display(), _win, _x, _y);
}

/************************************************

 ************************************************/
bool XfitMan::getWindowProperty(Window window,
                       Atom atom,               // property
                       Atom reqType,            // req_type
                       unsigned long* resultLen,// nitems_return
                       unsigned char** result   // prop_return
                      ) const
{
    int  format;
    unsigned long type, rest;
    return XGetWindowProperty(QX11Info::display(), window, atom, 0, 4096, false,
                              reqType, &type, &format, resultLen, &rest,
                              result)  == Success;
}


/************************************************

 ************************************************/
bool XfitMan::getRootWindowProperty(Atom atom,    // property
                           Atom reqType,            // req_type
                           unsigned long* resultLen,// nitems_return
                           unsigned char** result   // prop_return
                          ) const
{
    return getWindowProperty(root, atom, reqType, resultLen, result);
}

/**
 * @brief resizes a window to the given dimensions
 */
void XfitMan::resizeWindow(Window _wid, int _width, int _height) const
{
    XResizeWindow(QX11Info::display(), _wid, _width, _height);
}

/**
 * @brief gets a windowpixmap from a window
 */
bool XfitMan::getClientIcon(Window _wid, QPixmap& _pixreturn) const
{
    int format;
    ulong type, nitems, extra;
    ulong* data = 0;

    XGetWindowProperty(QX11Info::display(), _wid, atom("_NET_WM_ICON"),
                       0, LONG_MAX, False, AnyPropertyType,
                       &type, &format, &nitems, &extra,
                       (uchar**)&data);
    if (!data)
    {
        return false;
    }

    QImage img (data[0], data[1], QImage::Format_ARGB32);
    for (int i=0; i<img.sizeInBytes()/4; ++i)
        ((uint*)img.bits())[i] = data[i+2];

    _pixreturn = QPixmap::fromImage(img);
    XFree(data);

    return true;
}

bool XfitMan::getClientIcon(Window _wid, QIcon *icon) const
{
    int format;
    ulong type, nitems, extra;
    ulong* data = 0;

    XGetWindowProperty(QX11Info::display(), _wid, atom("_NET_WM_ICON"),
                       0, LONG_MAX, False, AnyPropertyType,
                       &type, &format, &nitems, &extra,
                       (uchar**)&data);
    if (!data)
    {
        return false;
    }

    ulong* d = data;
    while (d < data + nitems)
    {
        QImage img (d[0], d[1], QImage::Format_ARGB32);
        d+=2;
        for (int i=0; i<img.sizeInBytes()/4; ++i, ++d)
            ((uint*)img.bits())[i] = *d;

        icon->addPixmap(QPixmap::fromImage(img));
    }

    XFree(data);
    return true;
}

/**
 * @brief destructor
 */
XfitMan::~XfitMan()
{
}

/**
 * @brief returns a windowname and sets _nameSource to the finally used Atom
 */
// i got the idea for this from taskbar-plugin of LXPanel - so credits fly out :)
QString XfitMan::getWindowTitle(Window _wid) const
{
    QString name = QLatin1String("");
    //first try the modern net-wm ones
    unsigned long length;
    unsigned char *data = nullptr;
    
    Atom utf8Atom = atom("UTF8_STRING");

    if (getWindowProperty(_wid, atom("_NET_WM_VISIBLE_NAME"), utf8Atom, &length, &data))
    {
        name = QString::fromUtf8((char*) data);
        XFree(data);

    }

    if (name.isEmpty())
    {
        if (getWindowProperty(_wid, atom("_NET_WM_NAME"), utf8Atom, &length, &data))
        {
            name = QString::fromUtf8((char*) data);
            XFree(data);
        }
    }

    if (name.isEmpty())
    {
        if (getWindowProperty(_wid, atom("XA_WM_NAME"), XA_STRING, &length, &data))
        {
            name =  QString::fromUtf8(reinterpret_cast<char *>(data));
            XFree(data);
        }
    }

    if (name.isEmpty())
    {
        Status ok = XFetchName(QX11Info::display(), _wid, (char**) &data);
        name = QString::fromUtf8(reinterpret_cast<char *> (data));
        if (0 != ok) XFree(data);
    }

    if (name.isEmpty())
    {
        XTextProperty prop;
        if (XGetWMName(QX11Info::display(), _wid, &prop))
        {
            name = QString::fromUtf8((char*) prop.value);
            XFree(prop.value);
        }
    }

    return name;
}

QString XfitMan::getApplicationName(Window _wid) const
{
    XClassHint hint;
    QString ret;

    if (XGetClassHint(QX11Info::display(), _wid, &hint))
    {
        if (hint.res_name)
        {
            ret = QString::fromUtf8(hint.res_name);
            XFree(hint.res_name);
        }
        if (hint.res_class)
        {
            XFree(hint.res_class);
        }
    }

    return ret;
}

/**
 * @brief sends a clientmessage to a window
 */
int XfitMan::clientMessage(Window _wid, Atom _msg,
                            unsigned long data0,
                            unsigned long data1,
                            unsigned long data2,
                            unsigned long data3,
                            unsigned long data4) const
{
    XClientMessageEvent msg;
    msg.window = _wid;
    msg.type = ClientMessage;
    msg.message_type = _msg;
    msg.send_event = true;
    msg.display = QX11Info::display();
    msg.format = 32;
    msg.data.l[0] = data0;
    msg.data.l[1] = data1;
    msg.data.l[2] = data2;
    msg.data.l[3] = data3;
    msg.data.l[4] = data4;
    if (XSendEvent(QX11Info::display(), root, false, (SubstructureRedirectMask | SubstructureNotifyMask) , (XEvent *) &msg) == Success)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

/**
 * @brief raises windows _wid
 */
void XfitMan::raiseWindow(Window _wid) const
{
    clientMessage(_wid, atom("_NET_ACTIVE_WINDOW"),
                  SOURCE_PAGER);
}

/************************************************

 ************************************************/
void XfitMan::closeWindow(Window _wid) const
{
    clientMessage(_wid, atom("_NET_CLOSE_WINDOW"),
                  0, // Timestamp
                  SOURCE_PAGER);
}

void XfitMan::setIconGeometry(Window _wid, QRect* rect) const
{
    Atom net_wm_icon_geometry = atom("_NET_WM_ICON_GEOMETRY");
    if(!rect)
        XDeleteProperty(QX11Info::display(), _wid, net_wm_icon_geometry);
    else
    {
        long data[4];
        data[0] = rect->x();
        data[1] = rect->y();
        data[2] = rect->width();
        data[3] = rect->height();
        XChangeProperty(QX11Info::display(), _wid, net_wm_icon_geometry,
                        XA_CARDINAL, 32, PropModeReplace, (unsigned char*)data, 4);
    }
}
