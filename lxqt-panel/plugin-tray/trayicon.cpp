/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
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


// Warning: order of those include is important.
#include <QDebug>
#include <QApplication>
#include <QResizeEvent>
#include <QPainter>
#include <QBitmap>
#include <QStyle>
#include <QScreen>

#include "../panel/lxqtpanel.h"
#include "trayicon.h"
#include "xfitman.h"

#include <QX11Info>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>

#define XEMBED_EMBEDDED_NOTIFY 0


static bool xError;

/************************************************

************************************************/
int windowErrorHandler(Display *d, XErrorEvent *e)
{
    xError = true;
    if (e->error_code != BadWindow) {
        char str[1024];
        XGetErrorText(d, e->error_code,  str, 1024);
        qWarning() << "Error handler" << e->error_code
                   << str;
    }
    return 0;
}


/************************************************

 ************************************************/
TrayIcon::TrayIcon(Window iconId, QSize const & iconSize, QWidget* parent):
    QFrame(parent),
    mIconId(iconId),
    mWindowId(0),
    mAppName(xfitMan().getApplicationName(mIconId)),
    mIconSize(iconSize),
    mDamage(0),
    mDisplay(QX11Info::display())
{
    // NOTE:
    // it's a good idea to save the return value of QX11Info::display().
    // In Qt 5, this API is slower and has some limitations which can trigger crashes.
    // The XDisplay value is actally stored in QScreen object of the primary screen rather than
    // in a global variable. So when the parimary QScreen is being deleted and becomes invalid,
    // QX11Info::display() will fail and cause crash. Storing this value improves the efficiency and
    // also prevent potential crashes caused by this bug.

    setObjectName(QStringLiteral("TrayIcon"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    init();
    update();
}



/************************************************

 ************************************************/
void TrayIcon::init()
{
    Display* dsp = mDisplay;

    XWindowAttributes attr;
    if (! XGetWindowAttributes(dsp, mIconId, &attr))
    {
        deleteLater();
        return;
    }

//    qDebug() << "New tray icon ***********************************";
//    qDebug() << "  * window id:  " << hex << mIconId;
//    qDebug() << "  * window name:" << xfitMan().getName(mIconId);
//    qDebug() << "  * size (WxH): " << attr.width << "x" << attr.height;
//    qDebug() << "  * color depth:" << attr.depth;

    unsigned long mask = 0;
    XSetWindowAttributes set_attr;

    Visual* visual = attr.visual;
    set_attr.colormap = attr.colormap;
    set_attr.background_pixel = 0;
    set_attr.border_pixel = 0;
    mask = CWColormap|CWBackPixel|CWBorderPixel;

    const QRect icon_geom = iconGeometry();
    mWindowId = XCreateWindow(dsp, this->winId(), icon_geom.x(), icon_geom.y(), icon_geom.width() * metric(PdmDevicePixelRatio), icon_geom.height() * metric(PdmDevicePixelRatio),
                              0, attr.depth, InputOutput, visual, mask, &set_attr);


    xError = false;
    XErrorHandler old;
    old = XSetErrorHandler(windowErrorHandler);
    XReparentWindow(dsp, mIconId, mWindowId, 0, 0);
    XSync(dsp, false);
    XSetErrorHandler(old);

    if (xError)
    {
        qWarning() << "****************************************";
        qWarning() << "* Not icon_swallow                     *";
        qWarning() << "****************************************";
        XDestroyWindow(dsp, mWindowId);
        mWindowId = 0;
        deleteLater();
        return;
    }


    {
        Atom acttype;
        int actfmt;
        unsigned long nbitem, bytes;
        unsigned char *data = 0;
        int ret;

        ret = XGetWindowProperty(dsp, mIconId, xfitMan().atom("_XEMBED_INFO"),
                                 0, 2, false, xfitMan().atom("_XEMBED_INFO"),
                                 &acttype, &actfmt, &nbitem, &bytes, &data);
        if (ret == Success)
        {
            if (data)
                XFree(data);
        }
        else
        {
            qWarning() << "TrayIcon: xembed error";
            XDestroyWindow(dsp, mWindowId);
            deleteLater();
            return;
        }
    }

    {
        XEvent e;
        e.xclient.type = ClientMessage;
        e.xclient.serial = 0;
        e.xclient.send_event = True;
        e.xclient.message_type = xfitMan().atom("_XEMBED");
        e.xclient.window = mIconId;
        e.xclient.format = 32;
        e.xclient.data.l[0] = CurrentTime;
        e.xclient.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
        e.xclient.data.l[2] = 0;
        e.xclient.data.l[3] = mWindowId;
        e.xclient.data.l[4] = 0;
        XSendEvent(dsp, mIconId, false, 0xFFFFFF, &e);
    }

    XSelectInput(dsp, mIconId, StructureNotifyMask);
    mDamage = XDamageCreate(dsp, mIconId, XDamageReportRawRectangles);
    XCompositeRedirectWindow(dsp, mWindowId, CompositeRedirectManual);

    XMapWindow(dsp, mIconId);
    XMapRaised(dsp, mWindowId);

    const QSize req_size{mIconSize * metric(PdmDevicePixelRatio)};
    XResizeWindow(dsp, mIconId, req_size.width(), req_size.height());
}


/************************************************

 ************************************************/
TrayIcon::~TrayIcon()
{
    Display* dsp = mDisplay;
    XSelectInput(dsp, mIconId, NoEventMask);

    if (mDamage)
        XDamageDestroy(dsp, mDamage);

    // reparent to root
    xError = false;
    XErrorHandler old = XSetErrorHandler(windowErrorHandler);

    XUnmapWindow(dsp, mIconId);
    XReparentWindow(dsp, mIconId, QX11Info::appRootWindow(), 0, 0);

    if (mWindowId)
        XDestroyWindow(dsp, mWindowId);
    XSync(dsp, False);
    XSetErrorHandler(old);
}


/************************************************

 ************************************************/
QSize TrayIcon::sizeHint() const
{
    QMargins margins = contentsMargins();
    return QSize(margins.left() + mIconSize.width() + margins.right(),
                 margins.top() + mIconSize.height() + margins.bottom()
                );
}


/************************************************

 ************************************************/
void TrayIcon::setIconSize(QSize iconSize)
{
    mIconSize = iconSize;

    const QSize req_size{mIconSize * metric(PdmDevicePixelRatio)};
    if (mWindowId)
        xfitMan().resizeWindow(mWindowId, req_size.width(), req_size.height());

    if (mIconId)
        xfitMan().resizeWindow(mIconId, req_size.width(), req_size.height());
}


/************************************************

 ************************************************/
bool TrayIcon::event(QEvent *event)
{
    if (mWindowId)
    {
        switch (event->type())
        {
        case QEvent::Paint:
            draw(static_cast<QPaintEvent*>(event));
            break;

        case QEvent::Move:
        case QEvent::Resize:
        {
            QRect rect = iconGeometry();
            xfitMan().moveWindow(mWindowId, rect.left(), rect.top());
        }
            break;

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            event->accept();
            break;

        default:
            break;
        }
    }

    return QFrame::event(event);
}


/************************************************

 ************************************************/
QRect TrayIcon::iconGeometry()
{
    QRect res = QRect(QPoint(0, 0), mIconSize);

    res.moveCenter(QRect(0, 0, width(), height()).center());
    return res;
}


/************************************************

 ************************************************/
void TrayIcon::draw(QPaintEvent* /*event*/)
{
    Display* dsp = mDisplay;

    XWindowAttributes attr;
    if (!XGetWindowAttributes(dsp, mIconId, &attr))
    {
        qWarning() << "Paint error";
        return;
    }

    QImage image;
    XImage* ximage = XGetImage(dsp, mIconId, 0, 0, attr.width, attr.height, AllPlanes, ZPixmap);
    if(ximage)
    {
        image = QImage((const uchar*) ximage->data, ximage->width, ximage->height, ximage->bytes_per_line,  QImage::Format_ARGB32_Premultiplied);
    }
    else
    {
        qWarning() << "    * Error image is NULL";

        XClearArea(mDisplay, (Window)winId(), 0, 0, attr.width, attr.height, False);
        // for some unknown reason, XGetImage failed. try another less efficient method.
        // QScreen::grabWindow uses XCopyArea() internally.
        image = qApp->primaryScreen()->grabWindow(mIconId).toImage();
    }

//    qDebug() << "Paint icon **************************************";
//    qDebug() << "  * XComposite: " << isXCompositeAvailable();
//    qDebug() << "  * Icon geometry:" << iconGeometry();
//    qDebug() << "  Icon";
//    qDebug() << "    * window id:  " << hex << mIconId;
//    qDebug() << "    * window name:" << xfitMan().getName(mIconId);
//    qDebug() << "    * size (WxH): " << attr.width << "x" << attr.height;
//    qDebug() << "    * pos (XxY):  " << attr.x << attr.y;
//    qDebug() << "    * color depth:" << attr.depth;
//    qDebug() << "  XImage";
//    qDebug() << "    * size (WxH):  " << ximage->width << "x" << ximage->height;
//    switch (ximage->format)
//    {
//        case XYBitmap: qDebug() << "    * format:   XYBitmap"; break;
//        case XYPixmap: qDebug() << "    * format:   XYPixmap"; break;
//        case ZPixmap:  qDebug() << "    * format:   ZPixmap"; break;
//    }
//    qDebug() << "    * color depth:  " << ximage->depth;
//    qDebug() << "    * bits per pixel:" << ximage->bits_per_pixel;

    // Draw QImage ...........................
    QPainter painter(this);
    QRect iconRect = iconGeometry();
    if (image.size() != iconRect.size())
    {
        image = image.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QRect r = image.rect();
        r.moveCenter(iconRect.center());
        iconRect = r;
    }
//    qDebug() << " Draw rect:" << iconRect;

    painter.drawImage(iconRect, image);

    if(ximage)
        XDestroyImage(ximage);
//    debug << "End paint icon **********************************";
}


/************************************************

 ************************************************/
void TrayIcon::windowDestroyed(Window w)
{
    //damage is destroyed if it's parent window was destroyed
    if (mIconId == w)
        mDamage = 0;
}


/************************************************

 ************************************************/
bool TrayIcon::isXCompositeAvailable()
{
    int eventBase, errorBase;
    return XCompositeQueryExtension(QX11Info::display(), &eventBase, &errorBase );
}
