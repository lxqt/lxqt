/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2016 LXQt team
 * Authors:
 *   Palo Kisa <palo.kisa@gmail.com>
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

#include "actionview.h"
#ifdef HAVE_MENU_CACHE
    #include "xdgcachedmenu.h"
#else
    #include <XdgAction>
#endif

#include <QAction>
#include <QWidgetAction>
#include <QMenu>
#include <QStandardItemModel>
#include <QScrollBar>
#include <QProxyStyle>
#include <QStyledItemDelegate>
//==============================
#ifdef HAVE_MENU_CACHE
#include <QSortFilterProxyModel>
#else
FilterProxyModel::FilterProxyModel(QObject* parent) :
    QSortFilterProxyModel(parent) {
}

FilterProxyModel::~FilterProxyModel() {
}

bool FilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    if (filterStr_.isEmpty())
        return true;
    if (QStandardItemModel* srcModel = static_cast<QStandardItemModel*>(sourceModel())) {
        QModelIndex index = srcModel->index(source_row, 0, source_parent);
        if (QStandardItem * item = srcModel->itemFromIndex(index)) {
            XdgAction * action = qobject_cast<XdgAction *>(qvariant_cast<QAction *>(item->data(ActionView::ActionRole)));
            if (action) {
                const XdgDesktopFile& df = action->desktopFile();
                if (df.name().contains(filterStr_, filterCaseSensitivity()))
                    return true;
                QStringList list = df.expandExecString();
                if (!list.isEmpty()) {
                    if (list.at(0).contains(filterStr_, filterCaseSensitivity()))
                        return true;
                }
            }
        }
    }
    return false;
}
#endif
//==============================
namespace
{
    class SingleActivateStyle : public QProxyStyle
    {
    public:
        using QProxyStyle::QProxyStyle;
        virtual int styleHint(StyleHint hint, const QStyleOption * option = 0, const QWidget * widget = 0, QStyleHintReturn * returnData = 0) const override
        {
            if(hint == QStyle::SH_ItemView_ActivateItemOnSingleClick)
                return 1;
            return QProxyStyle::styleHint(hint, option, widget, returnData);

        }
    };

    class DelayedIconDelegate : public QStyledItemDelegate
    {
    public:
        DelayedIconDelegate(QObject * parent = nullptr)
            : QStyledItemDelegate(parent)
            , mMaxItemWidth(300)
        {
        }

        void setMaxItemWidth(int max)
        {
            mMaxItemWidth = max;
        }

        virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
            //the XdgCachedMenuAction/XdgAction does load the icon upon showing its menu
#ifdef HAVE_MENU_CACHE
            if (icon.isNull())
            {
                XdgCachedMenuAction * cached_action = qobject_cast<XdgCachedMenuAction *>(qvariant_cast<QAction *>(index.data(ActionView::ActionRole)));
                Q_ASSERT(nullptr != cached_action);
                cached_action->updateIcon();
                const_cast<QAbstractItemModel *>(index.model())->setData(index, cached_action->icon(), Qt::DecorationRole);
            }
#else
            if (icon.isNull())
            {
                XdgAction * action = qobject_cast<XdgAction *>(qvariant_cast<QAction *>(index.data(ActionView::ActionRole)));
                if (action != nullptr)
                {
                  action->updateIcon();
                  const_cast<QAbstractItemModel *>(index.model())->setData(index, action->icon(), Qt::DecorationRole);
                }
            }
#endif
            QSize s = QStyledItemDelegate::sizeHint(option, index);
            s.setWidth(qMin(mMaxItemWidth, s.width()));
            return s;
        }
    private:
        int mMaxItemWidth;
    };

}
//==============================
ActionView::ActionView(QWidget * parent /*= nullptr*/)
    : QListView(parent)
    , mModel{new QStandardItemModel{this}}
#ifdef HAVE_MENU_CACHE
    , mProxy{new QSortFilterProxyModel{this}}
#else
    , mProxy{new FilterProxyModel{this}}
#endif
    , mMaxItemsToShow(10)
{
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSizeAdjustPolicy(AdjustToContents);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionBehavior(SelectRows);
    setSelectionMode(SingleSelection);

    SingleActivateStyle * s = new SingleActivateStyle;
    s->setParent(this);
    setStyle(s);
    mProxy->setSourceModel(mModel);
    mProxy->setDynamicSortFilter(true);
    mProxy->setFilterRole(FilterRole);
    mProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mProxy->sort(0);
    {
        QScopedPointer<QItemSelectionModel> guard{selectionModel()};
        setModel(mProxy);
    }
    {
        QScopedPointer<QAbstractItemDelegate> guard{itemDelegate()};
        setItemDelegate(new DelayedIconDelegate{this});
    }
    connect(this, &QAbstractItemView::activated, this, &ActionView::onActivated);
}

void ActionView::ActionView::clear()
{
    for (int i = mModel->rowCount() - 1; i >= 0; --i)
    {
        mModel->removeRow(i);
    }
}

void ActionView::addAction(QAction * action)
{
    QStandardItem * item = new QStandardItem;
    item->setData(QVariant::fromValue<QAction *>(action), ActionRole);
    item->setFont(action->font());
    //Note: XdgCachedMenuAction has delayed icon loading... we are loading the icon
    //in QStyledItemDelegate:sizeHint if necessary
    item->setIcon(action->icon());
    item->setText(action->text());
    item->setToolTip(action->toolTip());
    QString all = action->text();
    all += QLatin1Char('\n');
    all += action->toolTip();
    item->setData(all, FilterRole);

    mModel->appendRow(item);
    connect(action, &QObject::destroyed, this, &ActionView::onActionDestroyed);
}

bool ActionView::existsAction(QAction const * action) const
{
    bool exists = false;
    for (int row = mModel->rowCount() - 1; 0 <= row; --row)
    {
        const QModelIndex index = mModel->index(row, 0);
        if (action->text() == mModel->data(index, Qt::DisplayRole)
                && action->toolTip() == mModel->data(index, Qt::ToolTipRole)
                )
        {
            exists = true;
            break;
        }

    }
    return exists;
}

void ActionView::fillActions(QMenu * menu)
{
    clear();
    fillActionsRecursive(menu);
}

void ActionView::setFilter(QString const & filter)
{
#ifdef HAVE_MENU_CACHE
    mProxy->setFilterFixedString(filter);
#else
    mProxy->setfilerString(filter);
#endif
    const int count = mProxy->rowCount();
    if (0 < count)
    {
        if (count > mMaxItemsToShow)
        {
            setCurrentIndex(mProxy->index(mMaxItemsToShow - 1, 0));
            verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
        } else
        {
            setCurrentIndex(mProxy->index(count - 1, 0));
        }
    }
}

void ActionView::setMaxItemsToShow(int max)
{
    mMaxItemsToShow = max;
}

void ActionView::setMaxItemWidth(int max)
{
    dynamic_cast<DelayedIconDelegate *>(itemDelegate())->setMaxItemWidth(max);
}

void ActionView::activateCurrent()
{
    QModelIndex const index = currentIndex();
    if (index.isValid())
        emit activated(index);
}

QSize ActionView::viewportSizeHint() const
{
    const int count = mProxy->rowCount();
    QSize s{0, 0};
    if (0 < count)
    {
        const bool scrollable = mMaxItemsToShow < count;
        s.setWidth(sizeHintForColumn(0) + (scrollable ? verticalScrollBar()->sizeHint().width() : 0));
        s.setHeight(sizeHintForRow(0) * (scrollable ? mMaxItemsToShow  : count));
    }
    return s;
}

QSize ActionView::minimumSizeHint() const
{
    return QSize{0, 0};
}

void ActionView::onActivated(QModelIndex const & index)
{
    QAction * action = qvariant_cast<QAction *>(model()->data(index, ActionRole));
    Q_ASSERT(nullptr != action);
    action->trigger();
}

void ActionView::onActionDestroyed()
{
    QObject * const action = sender();
    Q_ASSERT(nullptr != action);
    for (int i = mModel->rowCount() - 1; 0 <= i; --i)
    {
        QStandardItem * item = mModel->item(i);
        if (action == item->data(ActionRole).value<QObject *>())
        {
            mModel->removeRow(i);
            break;
        }
    }
}

void ActionView::fillActionsRecursive(QMenu * menu)
{
    const auto actions = menu->actions();
    for (auto const & action : actions)
    {
        if (QMenu * sub_menu = action->menu())
        {
            fillActionsRecursive(sub_menu); //recursion
        } else if (nullptr == qobject_cast<QWidgetAction* >(action)
                && !action->isSeparator())
        {
            //real menu action -> app
            if (!existsAction(action))
                addAction(action);
        }
    }
}

