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

/*
 * Note:
 * This is a workaround for the following Qt5 bugs.
 *
 * #49947: Drop events have broken mimeData()->urls() and text/uri-list.
 * #47981: Qt5.4 regression: Dropping text/urilist over browser windows stop working.
 *
 * Related LXQt bug: https://github.com/lxqt/lxqt/issues/688
 *
 * This workaround is not 100% reliable, but it should work most of the time.
 * In theory, when there are multiple drag and drops at nearly the same time and
 * you are using a remote X11 instance via a slow network connection, this workaround
 * might break. However, that should be a really rare corner case.
 *
 * How this fix works:
 * 1. Hook QApplication to filter raw X11 events
 * 2. Intercept SelectionRequest events sent from XDnd target window.
 * 3. Check if the data requested have the type "text/uri-list" or "x-moz-url"
 * 4. Bypass the broken Qt5 code and send the mime data to the target with our own code.
 *
 * The mime data is obtained during the most recent mouse button release event.
 * This can be incorrect in some corner cases, but it is still a simple and
 * good enough approximation that returns the correct data most of the time.
 * Anyway, a workarond is just a workaround. Ask Qt developers to fix their bugs.
 */

#ifndef XDNDWORKAROUND_H
#define XDNDWORKAROUND_H

#include <QtGlobal>

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <xcb/xcb.h>
#include <QByteArray>
#include <QPointer>

class QDrag;

class XdndWorkaround : public QAbstractNativeEventFilter {
public:
    explicit XdndWorkaround();
    ~XdndWorkaround() override;
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;
    static QByteArray atomName(xcb_atom_t atom);
    static xcb_atom_t internAtom(const char* name, int len = -1);
    static QByteArray windowProperty(xcb_window_t window, xcb_atom_t propAtom, xcb_atom_t typeAtom, int len);
    static void setWindowProperty(xcb_window_t window, xcb_atom_t propAtom, xcb_atom_t typeAtom, void* data, int len, int format = 8);

private:
    bool clientMessage(xcb_client_message_event_t* event);
    bool selectionNotify(xcb_selection_notify_event_t* event);

private:
    bool selectionRequest(xcb_selection_request_event_t* event);
    bool genericEvent(xcb_ge_generic_event_t* event);
    // _QBasicDrag* xcbDrag() const;
    void buttonRelease();

    QPointer<QDrag> lastDrag_;
    // xinput related
    bool xinput2Enabled_;
    int xinputOpCode_;
    int xinputEventBase_;
    int xinputErrorBase_;
};

#endif // XDNDWORKAROUND_H
