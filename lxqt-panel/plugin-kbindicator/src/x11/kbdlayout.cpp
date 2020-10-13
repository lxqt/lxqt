/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *   Dmitriy Zhukov <zjesclean@gmail.com>
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

#include <QCoreApplication>
#include <QAbstractNativeEventFilter>
#include <QDebug>
#include <QFile>
#include <QDomDocument>
#include "kbdlayout.h"

#include <xkbcommon/xkbcommon-x11.h>
#include <xcb/xcb.h>

// Note: We need to override "explicit" as this is a C++ keyword. But it is
// used as variable name in xkb.h. This is causing a failure in C++ compile
// time.
// Similar bug here: https://bugs.freedesktop.org/show_bug.cgi?id=74080
#define explicit _explicit
#include <xcb/xkb.h>
#undef explicit

#include "../kbdinfo.h"
#include "../controls.h"

namespace pimpl {

struct LangInfo
{
    QString name;
    QString syn;
    QString variant;
};

class X11Kbd: public QAbstractNativeEventFilter
{
public:
    X11Kbd(::X11Kbd *pub):
        m_pub(pub)
    {}

    bool init()
    {
        m_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        m_connection = xcb_connect(0, 0);

        if (!m_connection || xcb_connection_has_error(m_connection)){
            qWarning() << "Couldn't connect to X server: error code"
                       << (m_connection ? xcb_connection_has_error(m_connection) : -1);
            return false;
        }

        xkb_x11_setup_xkb_extension(m_connection,
            XKB_X11_MIN_MAJOR_XKB_VERSION,
            XKB_X11_MIN_MINOR_XKB_VERSION,
            XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
            NULL, NULL, &m_eventType, NULL
        );

        m_deviceId = xkb_x11_get_core_keyboard_device_id(m_connection);
        qApp->installNativeEventFilter(this);

        readState();
        return true;
    }

    virtual ~X11Kbd()
    {
        xkb_state_unref(m_state);
        xkb_keymap_unref(m_keymap);
        xcb_disconnect(m_connection);
        xkb_context_unref(m_context);
    }

    bool isEnabled() const
    { return true;  }

    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *)
    {
        if (eventType != "xcb_generic_event_t")
            return false;

        xcb_generic_event_t *event = static_cast<xcb_generic_event_t*>(message);
        if ((event->response_type & ~0x80) == m_eventType){
            xcb_xkb_state_notify_event_t *sevent = reinterpret_cast<xcb_xkb_state_notify_event_t*>(event);
            switch(sevent->xkbType){
            case XCB_XKB_STATE_NOTIFY:
                xkb_state_update_mask(m_state,
                    sevent->baseMods,
                    sevent->latchedMods,
                    sevent->lockedMods,
                    sevent->baseGroup,
                    sevent->latchedGroup,
                    sevent->lockedGroup
                );

                if(sevent->changed & XCB_XKB_STATE_PART_GROUP_STATE){
                    emit m_pub->layoutChanged(sevent->group);
                    return true;
                }

                if(sevent->changed & XCB_XKB_STATE_PART_MODIFIER_LOCK){
                    for(auto i = m_modifiers.cbegin() ; i != m_modifiers.cend(); ++i) {
                        const auto& cnt = i.key();
                        bool oldState = m_modifiers[cnt];
                        bool newState = xkb_state_led_name_is_active(m_state, modName(cnt));
                        if(oldState != newState){
                            m_modifiers[cnt] = newState;
                            emit m_pub->modifierChanged(cnt, newState);
                        }
                    }
                }
                break;
            case XCB_XKB_NEW_KEYBOARD_NOTIFY:
                readState();
                break;
            }
        }

        emit m_pub->checkState();

        return false;
    }

    void readKbdInfo(KbdInfo & info) const
    {
        info.clear();
        xkb_layout_index_t count = xkb_keymap_num_layouts(m_keymap);
        for(xkb_layout_index_t i = 0; i < count; ++i){
            QString name = QString::fromUtf8(xkb_keymap_layout_get_name(m_keymap, i));
            const LangInfo & linfo = names(name);
            info.append({linfo.syn, linfo.name, linfo.variant});
            if (xkb_state_layout_index_is_active(m_state, i, XKB_STATE_LAYOUT_EFFECTIVE))
                info.setCurrentGroup(i);
        }
    }

    void lockGroup(uint group)
    {
        xcb_void_cookie_t cookie = xcb_xkb_latch_lock_state(m_connection, m_deviceId, 0, 0, 1, group, 0, 0, 0);
        xcb_generic_error_t *error = xcb_request_check(m_connection, cookie);
        if (error){
            qWarning() << "Lock group error: " << error->error_code;
        }
    }

    void lockModifier(Controls cnt, bool locked)
    {
        quint8 mask = fetchMask(cnt);
        quint8 curMask = locked ? mask : 0;
        xcb_void_cookie_t cookie = xcb_xkb_latch_lock_state(m_connection, m_deviceId, mask, curMask, 0, 0, 0, 0, 0);
        xcb_generic_error_t *error = xcb_request_check(m_connection, cookie);
        if (error){
            qWarning() << "Lock group error: " << error->error_code;
        }
    }

    bool isModifierLocked(Controls cnt) const
    { return m_modifiers[cnt]; }

private:
    quint8 fetchMask(Controls cnt) const
    {
        static QHash<Controls, quint8> masks;
        if (masks.contains(cnt))
            return masks[cnt];

        xkb_mod_index_t index =  xkb_keymap_led_get_index(m_keymap, modName(cnt));

        xcb_generic_error_t *error = 0;
        quint8 mask = 0;

        xcb_xkb_get_indicator_map_cookie_t cookie = xcb_xkb_get_indicator_map(m_connection, m_deviceId, 1 << index);
        xcb_xkb_get_indicator_map_reply_t *reply = xcb_xkb_get_indicator_map_reply(m_connection, cookie, &error);


        if (!reply || error){
            qWarning() << "Cannot fetch mask " << error->error_code;
            return mask;
        }

        xcb_xkb_indicator_map_t *map = xcb_xkb_get_indicator_map_maps(reply);

        mask = map->mods;
        masks[cnt] = mask;

        free(reply);
        return mask;
    }

    const char * modName(Controls cnt) const
    {
        switch(cnt){
        case Controls::Caps:
            return XKB_LED_NAME_CAPS;
        case Controls::Num:
            return XKB_LED_NAME_NUM;
        case Controls::Scroll:
            return XKB_LED_NAME_SCROLL;
        default:
            return 0;
        }
    }

    void readState()
    {
        if (m_keymap)
            xkb_keymap_unref(m_keymap);
        m_keymap = xkb_x11_keymap_new_from_device(m_context, m_connection, m_deviceId, (xkb_keymap_compile_flags)0);

        if (m_state)
            xkb_state_unref(m_state);
        m_state = xkb_x11_state_new_from_device(m_keymap, m_connection, m_deviceId);

        for(auto i = m_modifiers.cbegin(); i != m_modifiers.cend(); ++i) {
            m_modifiers[i.key()] = xkb_state_led_name_is_active(m_state, modName(i.key()));
        }
        emit m_pub->keyboardChanged();
    }

    const LangInfo & names(const QString & langName) const
    {
        static LangInfo def{QStringLiteral("Unknown"), QStringLiteral("??"), QStringLiteral("None")};
        static QHash<QString, LangInfo> names;
        if (names.empty()){
            if(QFile::exists(QStringLiteral("/usr/share/X11/xkb/rules/evdev.xml"))){
                QDomDocument doc;

                QFile file(QStringLiteral("/usr/share/X11/xkb/rules/evdev.xml"));
                if (file.open(QIODevice::ReadOnly)){
                    if (doc.setContent(&file)) {
                        QDomElement docElem = doc.documentElement();

                        auto layout= docElem.firstChildElement(QStringLiteral("layoutList"));
                        for(int i = 0; i < layout.childNodes().count(); ++i){
                            auto conf = layout.childNodes().at(i).firstChildElement(QStringLiteral("configItem"));
                            names.insert(
                                conf.firstChildElement(QStringLiteral("description")).firstChild().toText().data(),{
                                    conf.firstChildElement(QStringLiteral("description")).firstChild().toText().data(),
                                    conf.firstChildElement(QStringLiteral("name")).firstChild().toText().data(),
                                    QStringLiteral("None")
                                }
                            );
                            auto variants = layout.childNodes().at(i).firstChildElement(QStringLiteral("variantList"));
                            for(int j = 0; j < variants.childNodes().count(); ++j){
                                auto var = variants.childNodes().at(j).firstChildElement(QStringLiteral("configItem"));
                                names.insert(
                                    var.firstChildElement(QStringLiteral("description")).firstChild().toText().data(), {
                                        conf.firstChildElement(QStringLiteral("description")).firstChild().toText().data(),
                                        conf.firstChildElement(QStringLiteral("name")).firstChild().toText().data(),
                                        var.firstChildElement(QStringLiteral("name")).firstChild().toText().data()
                                    }
                                );
                            }
                        }
                    }
                    file.close();
                }
            }
        }
        if (names.contains(langName))
            return names[langName];
        return def;
    }

private:
    struct xkb_context    *m_context    = 0;
    xcb_connection_t      *m_connection = 0;
    int32_t                m_deviceId;
    uint8_t                m_eventType;
    xkb_state             *m_state      = 0;
    xkb_keymap            *m_keymap     = 0;
    ::X11Kbd              *m_pub;
    QHash<Controls, bool>  m_modifiers  = {
        {Controls::Caps,   false},
        {Controls::Num,    false},
        {Controls::Scroll, false},
    };
};

}

X11Kbd::X11Kbd():
    m_priv(new pimpl::X11Kbd(this))
{}

X11Kbd::~X11Kbd()
{}

bool X11Kbd::init()
{ return m_priv->init(); }

bool X11Kbd::isEnabled() const
{ return true; }

void X11Kbd::readKbdInfo(KbdInfo & info) const
{ m_priv->readKbdInfo(info); }

void X11Kbd::lockGroup(uint layId) const
{ m_priv->lockGroup(layId); }

void X11Kbd::lockModifier(Controls cnt, bool locked)
{ m_priv->lockModifier(cnt, locked); }

bool X11Kbd::isModifierLocked(Controls cnt) const
{ return m_priv->isModifierLocked(cnt); }
