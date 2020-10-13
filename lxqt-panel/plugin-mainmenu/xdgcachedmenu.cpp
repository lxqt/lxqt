/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  <copyright holder> <email>
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

#include "xdgcachedmenu.h"
#include <QAction>
#include <QIcon>
#include <QCursor>
#include <QToolTip>
#include <QList>
#include <QUrl>
#include <QDrag>
#include <QMouseEvent>
#include <QApplication>
#include <XdgDesktopFile>
#include <QHelpEvent>
#include <QMimeData>
#include <QDebug>
#include <memory>

XdgCachedMenuAction::XdgCachedMenuAction(MenuCacheItem* item, QObject* parent):
    QAction{parent}
    , iconName_{QString::fromUtf8(menu_cache_item_get_icon(item))}
{
    QString title = QString::fromUtf8(menu_cache_item_get_name(item));
    title = title.replace(QLatin1Char('&'), QLatin1String("&&")); // & is reserved for mnemonics
    setText(title);
    // Only set tooltips for app items
    if(menu_cache_item_get_type(item) == MENU_CACHE_TYPE_APP)
    {
        QString comment = QString::fromUtf8(menu_cache_item_get_comment(item));
        setToolTip(comment);
    }
    if (char * file_path = menu_cache_item_get_file_path(item))
    {
        filePath_ = QString::fromUtf8(file_path);
        g_free(file_path);
    }
}

void XdgCachedMenuAction::updateIcon()
{
    if(icon().isNull())
    {
        // Note: We don't use the QIcon::fromTheme(const QString &name
        // , const QIcon &fallback) overload because of "availableSizes()"
        // check in it, see https://bugreports.qt.io/browse/QTBUG-63187
        QIcon icon = QIcon::fromTheme(iconName_);

        if (icon.isNull())
            icon = QIcon::fromTheme(QStringLiteral("unknown"));
        // Some themes may lack the "unknown" icon; checking null prevents 
        // infinite recursion (setIcon->dataChanged->updateIcon->setIcon)
        if (icon.isNull())
            return;
        setIcon(icon);
    }
}

XdgCachedMenu::XdgCachedMenu(QWidget* parent): QMenu(parent)
{
    connect(this, &QMenu::aboutToShow, this, &XdgCachedMenu::onAboutToShow);
}

XdgCachedMenu::XdgCachedMenu(MenuCache* menuCache, QWidget* parent): QMenu(parent)
{
    // qDebug() << "CREATE MENU FROM CACHE" << menuCache;
    MenuCacheDir* dir = menu_cache_dup_root_dir(menuCache);

    // get current desktop name or fallback to LXQt
    const QByteArray xdgDesktop = qgetenv("XDG_CURRENT_DESKTOP");
    const QByteArray desktop = xdgDesktop.isEmpty() ? "LXQt:X-LXQt" : xdgDesktop;
    menu_cache_desktop_ = menu_cache_get_desktop_env_flag(menuCache, desktop.constData());

    addMenuItems(this, dir);
    menu_cache_item_unref(MENU_CACHE_ITEM(dir));
    connect(this, &QMenu::aboutToShow, this, &XdgCachedMenu::onAboutToShow);
}

XdgCachedMenu::~XdgCachedMenu()
{
}

void XdgCachedMenu::addMenuItems(QMenu* menu, MenuCacheDir* dir)
{
  GSList* list = menu_cache_dir_list_children(dir);
  for(GSList * l = list; l; l = l->next)
  {
    // Note: C++14 is needed for usage of the std::make_unique
    //auto guard = std::make_unique(static_cast<MenuCacheItem *>(l->data), menu_cache_item_unref);
    std::unique_ptr<MenuCacheItem, gboolean (*)(MenuCacheItem* item)> guard{static_cast<MenuCacheItem *>(l->data), menu_cache_item_unref};
    MenuCacheItem* item = guard.get();
    MenuCacheType type = menu_cache_item_get_type(item);

    if(type == MENU_CACHE_TYPE_SEP)
    {
      menu->addSeparator();
      continue;
    }
    else
    {
      bool appVisible = type == MENU_CACHE_TYPE_APP
          && menu_cache_app_get_is_visible(MENU_CACHE_APP(item),
                                           menu_cache_desktop_);
      bool dirVisible = type == MENU_CACHE_TYPE_DIR
          && menu_cache_dir_is_visible(MENU_CACHE_DIR(item));

      if(!appVisible && !dirVisible)
        continue;

      XdgCachedMenuAction* action = new XdgCachedMenuAction(item, menu);
      menu->addAction(action);

      if(type == MENU_CACHE_TYPE_APP)
        connect(action, &QAction::triggered, this, &XdgCachedMenu::onItemTrigerred);
      else if(type == MENU_CACHE_TYPE_DIR)
      {
        XdgCachedMenu* submenu = new XdgCachedMenu(menu);
        action->setMenu(submenu);
        addMenuItems(submenu, MENU_CACHE_DIR(item));
      }
    }
  }
  if (list)
      g_slist_free(list);
}

void XdgCachedMenu::onItemTrigerred()
{
    XdgCachedMenuAction* action = static_cast<XdgCachedMenuAction*>(sender());
    XdgDesktopFile df;
    df.load(action->filePath());
    df.startDetached();
}

// taken from libqtxdg: XdgMenuWidget
bool XdgCachedMenu::event(QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *e = static_cast<QMouseEvent*>(event);
        if (e->button() == Qt::LeftButton)
            mDragStartPosition = e->pos();
    }

    else if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent *e = static_cast<QMouseEvent*>(event);
        handleMouseMoveEvent(e);
    }

    else if(event->type() == QEvent::ToolTip)
    {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        QAction* action = actionAt(helpEvent->pos());
        if(action && action->menu() == nullptr)
            QToolTip::showText(helpEvent->globalPos(), action->toolTip(), this);
    }

    return QMenu::event(event);
}

// taken from libqtxdg: XdgMenuWidget
void XdgCachedMenu::handleMouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if ((event->pos() - mDragStartPosition).manhattanLength() < QApplication::startDragDistance())
        return;

    XdgCachedMenuAction *a = qobject_cast<XdgCachedMenuAction*>(actionAt(mDragStartPosition));
    if (!a)
        return;

    QList<QUrl> urls;
    urls << QUrl(QString::fromLatin1("file://%1").arg(a->filePath()));

    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(urls);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction | Qt::LinkAction);
}

void XdgCachedMenu::onAboutToShow()
{
    const auto actionList = actions();
    for(QAction* action : actionList)
    {
        if(action->inherits("XdgCachedMenuAction"))
        {
            static_cast<XdgCachedMenuAction*>(action)->updateIcon();

            // this seems to cause some incorrect menu behaviour.
            // qApp->processEvents();
        }
    }
}
