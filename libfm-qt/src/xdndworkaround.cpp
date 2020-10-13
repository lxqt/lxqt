/*
 * Copyright (C) 2016  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <QtGlobal>
#include "xdndworkaround.h"
#include <QApplication>
#include <QDebug>
#include <QX11Info>
#include <QMimeData>
#include <QCursor>
#include <QWidget>

#include <QDrag>
#include <QUrl>
#include <cstring>

// these are private Qt headers which are not part of Qt APIs
#include <private/qdnd_p.h>  // Too bad that we need to use private headers of Qt :-(

// For some unknown reasons, the event type constants defined in
// xcb/input.h are different from that in X11/extension/XI2.h
// To be safe, we define it ourselves.
#undef XI_ButtonRelease
#define XI_ButtonRelease                 5


XdndWorkaround::XdndWorkaround() {
    if(!QX11Info::isPlatformX11()) {
        return;
    }

    // we need to filter all X11 events
    qApp->installNativeEventFilter(this);

    lastDrag_ = nullptr;

    // initialize xinput2 since newer versions of Qt5 uses it.
    static char xi_name[] = "XInputExtension";
    xcb_connection_t* conn = QX11Info::connection();
    xcb_query_extension_cookie_t cookie = xcb_query_extension(conn, strlen(xi_name), xi_name);
    xcb_generic_error_t* err = nullptr;
    xcb_query_extension_reply_t* reply = xcb_query_extension_reply(conn, cookie, &err);
    if(err == nullptr) {
        xinput2Enabled_ = true;
        xinputOpCode_ = reply->major_opcode;
        xinputEventBase_ = reply->first_event;
        xinputErrorBase_ = reply->first_error;
        // qDebug() << "xinput: " << m_xi2Enabled << m_xiOpCode << m_xiEventBase;
    }
    else {
        xinput2Enabled_ = false;
        free(err);
    }
    free(reply);
}

XdndWorkaround::~XdndWorkaround() {
    if(!QX11Info::isPlatformX11()) {
        return;
    }
    qApp->removeNativeEventFilter(this);
}

bool XdndWorkaround::nativeEventFilter(const QByteArray& eventType, void* message, long* /*result*/) {
    if(Q_LIKELY(eventType == "xcb_generic_event_t")) {
        xcb_generic_event_t* event = static_cast<xcb_generic_event_t*>(message);
        switch(event->response_type & ~0x80) {
        case XCB_CLIENT_MESSAGE:
            return clientMessage(reinterpret_cast<xcb_client_message_event_t*>(event));
        case XCB_SELECTION_NOTIFY:
            return selectionNotify(reinterpret_cast<xcb_selection_notify_event_t*>(event));
        case XCB_SELECTION_REQUEST:
            return selectionRequest(reinterpret_cast<xcb_selection_request_event_t*>(event));
        case XCB_GE_GENERIC:
            // newer versions of Qt5 supports xinput2, which sends its mouse events via XGE.
            return genericEvent(reinterpret_cast<xcb_ge_generic_event_t*>(event));
        case XCB_BUTTON_RELEASE:
            // older versions of Qt5 receive mouse events via old XCB events.
            buttonRelease();
            break;
        default:
            break;
        }
    }
    return false;
}

// static
QByteArray XdndWorkaround::atomName(xcb_atom_t atom) {
    QByteArray name;
    xcb_connection_t* conn = QX11Info::connection();
    xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, atom);
    xcb_get_atom_name_reply_t* reply = xcb_get_atom_name_reply(conn, cookie, nullptr);
    int len = xcb_get_atom_name_name_length(reply);
    if(len > 0) {
        name.append(xcb_get_atom_name_name(reply), len);
    }
    free(reply);
    return name;
}

// static
xcb_atom_t XdndWorkaround::internAtom(const char* name, int len) {
    xcb_atom_t atom = 0;
    if(len == -1) {
        len = strlen(name);
    }
    xcb_connection_t* conn = QX11Info::connection();
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, false, len, name);
    xcb_generic_error_t* err = nullptr;
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(conn, cookie, &err);
    if(reply != nullptr) {
        atom = reply->atom;
        free(reply);
    }
    if(err != nullptr) {
        free(err);
    }
    return atom;
}

// static
QByteArray XdndWorkaround::windowProperty(xcb_window_t window, xcb_atom_t propAtom, xcb_atom_t typeAtom, int len) {
    QByteArray data;
    xcb_connection_t* conn = QX11Info::connection();
    xcb_get_property_cookie_t cookie = xcb_get_property(conn, false, window, propAtom, typeAtom, 0, len);
    xcb_generic_error_t* err = nullptr;
    xcb_get_property_reply_t* reply = xcb_get_property_reply(conn, cookie, &err);
    if(reply != nullptr) {
        len = xcb_get_property_value_length(reply);
        const char* buf = (const char*)xcb_get_property_value(reply);
        data.append(buf, len);
        free(reply);
    }
    if(err != nullptr) {
        free(err);
    }
    return data;
}

// static
void XdndWorkaround::setWindowProperty(xcb_window_t window, xcb_atom_t propAtom, xcb_atom_t typeAtom, void* data, int len, int format) {
    xcb_connection_t* conn = QX11Info::connection();
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, propAtom, typeAtom, format, len, data);
}


bool XdndWorkaround::clientMessage(xcb_client_message_event_t* event) {
    QByteArray event_type = atomName(event->type);
    // qDebug() << "client message:" << event_type;

    // NOTE: Because of the limitation of Qt, this hack is required to provide
    // Xdnd direct save (XDS) protocol support.
    // https://www.freedesktop.org/wiki/Specifications/XDS/#index4h2
    //
    // XDS requires that the drop target should get and set the window property of the
    // drag source to pass the file path, but in Qt there is NO way to know the
    // window ID of the drag source so it's not possible to implement XDS with Qt alone.
    // Here is a simple hack. We get the drag source window ID with raw XCB code.
    // Then, save it on the drop target widget using QObject dynamic property.
    // So in the drop event handler of the target widget, it can obtain the
    // window ID of the drag source with QObject::property().
    // This hack works 99.99% of the time, but it's not bullet-proof.
    // In theory, there is one corner case for which this will not work.
    // That is, when you drag multiple XDS sources at the same time and drop
    // all of them on the same widget. (Does XDND support doing this?)
    // I do not think that any app at the moment support this.
    // Even if somebody is using it, X11 will die and we should solve this in Wayland instead.
    //
    if(event_type == "XdndDrop") {
        // data.l[0] contains the XID of the source window.
        // data.l[1] is reserved for future use (flags).
        // data.l[2] contains the time stamp for retrieving the data. (new in version 1)
        QWidget* target = QWidget::find(event->window);
        if(target != nullptr) { // drop on our widget
            target = qApp->widgetAt(QCursor::pos()); // get the exact child widget that receives the drop
            if(target != nullptr) {
                target->setProperty("xdnd::lastDragSource", event->data.data32[0]);
                target->setProperty("xdnd::lastDropTime", event->data.data32[2]);
            }
        }
    }
    else if(event_type == "XdndFinished") {
        lastDrag_ = nullptr;
    }
    return false;
}

bool XdndWorkaround::selectionNotify(xcb_selection_notify_event_t* event) {
    qDebug() << "selection notify" << atomName(event->selection);
    return false;
}


bool XdndWorkaround::selectionRequest(xcb_selection_request_event_t* event) {
    xcb_connection_t* conn = QX11Info::connection();
    if(event->property == XCB_ATOM_PRIMARY || event->property == XCB_ATOM_SECONDARY) {
        return false;    // we only touch selection requests related to XDnd
    }
    QByteArray prop_name = atomName(event->property);
    if(prop_name == "CLIPBOARD") {
        return false;    // we do not touch clipboard, either
    }

    xcb_atom_t atomFormat = event->target;
    QByteArray type_name = atomName(atomFormat);
    // qDebug() << "selection request" << prop_name << type_name;
    // We only want to handle text/x-moz-url and text/uri-list
    if(type_name == "text/x-moz-url" || type_name.startsWith("text/uri-list")) {
        QDragManager* mgr = QDragManager::self();
        QDrag* drag = mgr->object();
        if(drag == nullptr) {
            drag = lastDrag_;
        }
        QMimeData* mime = drag ? drag->mimeData() : nullptr;
        if(mime != nullptr && mime->hasUrls()) {
            QByteArray data;
            QList<QUrl> uris = mime->urls();
            if(type_name == "text/x-moz-url") {
                QString mozurl = uris.at(0).toString(QUrl::FullyEncoded);
                data.append((const char*)mozurl.utf16(), mozurl.length() * 2);
            }
            else { // text/uri-list
                for(const QUrl& uri : uris) {
                    data.append(uri.toString(QUrl::FullyEncoded).toUtf8());
                    data.append("\r\n");
                }
            }
            xcb_change_property(conn, XCB_PROP_MODE_REPLACE, event->requestor, event->property,
                                atomFormat, 8, data.size(), (const void*)data.constData());
            xcb_selection_notify_event_t notify;
            notify.response_type = XCB_SELECTION_NOTIFY;
            notify.requestor = event->requestor;
            notify.selection = event->selection;
            notify.time = event->time;
            notify.property = event->property;
            notify.target = atomFormat;
            xcb_window_t proxy_target = event->requestor;
            xcb_send_event(conn, false, proxy_target, XCB_EVENT_MASK_NO_EVENT, (const char*)&notify);
            return true; // stop Qt 5 from touching the event
        }
    }
    return false; // let Qt handle this
}

bool XdndWorkaround::genericEvent(xcb_ge_generic_event_t* event) {
    // check this is an xinput event
    if(xinput2Enabled_ && event->extension == xinputOpCode_) {
        if(event->event_type == XI_ButtonRelease) {
            buttonRelease();
        }
    }
    return false;
}

void XdndWorkaround::buttonRelease() {
    QDragManager* mgr = QDragManager::self();
    lastDrag_ = mgr->object();
    // qDebug() << "BUTTON RELEASE!!!!" << xcbDrag()->canDrop() << lastDrag_;
}

