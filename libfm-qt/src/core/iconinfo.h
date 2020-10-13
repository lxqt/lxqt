/*
 *      fm-icon.h
 *
 *      Copyright 2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __FM2_ICON_INFO_H__
#define __FM2_ICON_INFO_H__

#include "../libfmqtglobals.h"
#include <gio/gio.h>
#include "gioptrs.h"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <forward_list>
#include <QIcon>


namespace Fm {

class LIBFM_QT_API IconInfo: public std::enable_shared_from_this<IconInfo> {
public:
    friend class IconEngine;

    explicit IconInfo() {}

    explicit IconInfo(const char* name);

    explicit IconInfo(const GIconPtr gicon);

    ~IconInfo();

    static std::shared_ptr<const IconInfo> fromName(const char* name);

    static std::shared_ptr<const IconInfo> fromGIcon(GIconPtr gicon);

    static std::shared_ptr<const IconInfo> fromGIcon(GIcon* gicon) {
        return fromGIcon(GIconPtr{gicon, true});
    }

    static void updateQIcons();

    GIconPtr gicon() const {
        return gicon_;
    }

    QIcon qicon() const;

    bool hasEmblems() const {
        return G_IS_EMBLEMED_ICON(gicon_.get());
    }

    std::forward_list<std::shared_ptr<const IconInfo>> emblems() const;

    bool isValid() const {
        return gicon_ != nullptr;
    }

private:

    static QList<QIcon> qiconsFromNames(const char* const* names);

    // actual QIcon loaded by QIcon::fromTheme
    QIcon internalQicon() const;

    struct GIconHash {
        std::size_t operator()(GIcon* gicon) const {
            return g_icon_hash(gicon);
        }
    };

    struct GIconEqual {
        bool operator()(GIcon* gicon1, GIcon* gicon2) const {
            return g_icon_equal(gicon1, gicon2);
        }
    };

private:
    GIconPtr gicon_;
    mutable QIcon qicon_;
    mutable QList<QIcon> internalQicons_;

    static std::unordered_map<GIcon*, std::shared_ptr<IconInfo>, GIconHash, GIconEqual> cache_;
    static std::mutex mutex_;
    static QList<QIcon> fallbackQicons_;
};

} // namespace Fm

Q_DECLARE_METATYPE(std::shared_ptr<const Fm::IconInfo>)

#endif /* __FM2_ICON_INFO_H__ */
