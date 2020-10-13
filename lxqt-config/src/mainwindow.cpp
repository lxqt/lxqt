/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#include <QDirIterator>
#include <QLineEdit>
#include <QTimer>

#include "mainwindow.h"
#include <QtDebug>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QShortcut>
#include <QKeySequence>
#include <QPushButton>
#include <XdgDesktopFile>
#include <XdgIcon>
#include <XdgMenu>
#include <XmlHelper>

#include "qcategorizedview.h"
#include "qcategorydrawer.h"
#include "qcategorizedsortfilterproxymodel.h"

namespace LXQtConfig {

struct ConfigPaneData: public QSharedData
{
    QString id;
    QString category;
    XdgDesktopFile xdg;
};

class ConfigPane
{
public:
    ConfigPane(): d(new ConfigPaneData) { }
    ConfigPane(const ConfigPane &other): d(other.d) { }

    inline QString &id() const { return d->id; }
    inline XdgDesktopFile xdg() const { return d->xdg; }
    inline void setXdg(XdgDesktopFile xdg) { d->xdg = xdg; }
    inline QString &category() const { return d->category; }

    bool operator==(const ConfigPane &other) const
    {
        return d->id == other.id();
    }

private:
    QExplicitlySharedDataPointer<ConfigPaneData> d;
};


class ConfigPaneModel: public QAbstractListModel
{
public:
    ConfigPaneModel(): QAbstractListModel()
    {
        QString menuFile = XdgMenu::getMenuFileName(QStringLiteral("config.menu"));
        XdgMenu xdgMenu;
        xdgMenu.setEnvironments(QStringList() << QStringLiteral("X-LXQT") << QStringLiteral("LXQt") << QStringLiteral("LXDE"));
        bool res = xdgMenu.read(menuFile);
        if (!res)
        {
            QMessageBox::warning(nullptr, QStringLiteral("Parse error"), xdgMenu.errorString());
            return;
        }

        DomElementIterator it(xdgMenu.xml().documentElement() , QStringLiteral("Menu"));
        while(it.hasNext())
        {
            this->buildGroup(it.next());
        }
    }

    void buildGroup(const QDomElement& xml)
    {
        QString category;
        if (! xml.attribute(QStringLiteral("title")).isEmpty())
            category = xml.attribute(QStringLiteral("title"));
        else
            category = xml.attribute(QStringLiteral("name"));

        DomElementIterator it(xml , QStringLiteral("AppLink"));
        while(it.hasNext())
        {
            QDomElement x = it.next();

            XdgDesktopFile xdg;
            xdg.load(x.attribute(QStringLiteral("desktopFile")));

            ConfigPane pane;
            pane.id() = xdg.value(QStringLiteral("Icon")).toString();
            pane.category() = category;
            pane.setXdg(xdg);
            m_list.append(pane);
        }
    }

    void activateItem(const QModelIndex &index)
    {
        if (!index.isValid())
            return;
        m_list[index.row()].xdg().startDetached();
    }

    ~ConfigPaneModel() override { }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return m_list.count();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        return false;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
            return m_list[index.row()].xdg().name();
        if (role == QCategorizedSortFilterProxyModel::CategoryDisplayRole)
            return m_list[index.row()].category();
        if (role == QCategorizedSortFilterProxyModel::CategorySortRole)
            return m_list[index.row()].category();
        if (role == Qt::UserRole)
            return m_list[index.row()].id();
        if (role == Qt::DecorationRole)
        {
            return m_list[index.row()].xdg().icon(XdgIcon::defaultApplicationIcon());
        }
        return QVariant();
    }

private:
    QList<ConfigPane> m_list;
};

}


class ConfigItemDelegate : public QStyledItemDelegate
{
public:
    ConfigItemDelegate(QCategorizedView* view) : mView(view) { }
    ~ConfigItemDelegate() override { }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        /* We let Qt calculate the real cell size but consider the 4-px margin
           around each cell and try to add a 2-px margin around the selection
           rectangle for styles that, unlike Fusion, highlight the whole item. */
        QStyleOptionViewItem opt = option;
        int delta = opt.rect.width() - (mView->gridSize().width() - 8);
        if (delta > 0)
          opt.rect.adjust(delta/2, 0 , -delta/2, 0);
        QSize defaultSize = QStyledItemDelegate::sizeHint(opt, index);
        return QSize(qMin(defaultSize.width() + 4, mView->gridSize().width() - 8),
                     qMin(defaultSize.height() + 4, mView->gridSize().height() - 8));
    }

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        if(!index.isValid())
            return;

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        const QSize & iconSize = option.decorationSize;
        int size = qMin(mView->gridSize().width() - 8, // 4-px margin around each cell
                        iconSize.height());
        opt.decorationSize = QSize(size, size);

        const QWidget* widget = opt.widget;
        QStyle* style = widget ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    }

private:
    QCategorizedView *mView;
};


LXQtConfig::MainWindow::MainWindow() : QMainWindow()
{
    setupUi(this);
   buttonBox->button(QDialogButtonBox::Close)->setText(tr("Close"));
    view->installEventFilter(this);
    /* To always have the intended layout on startup,
       the listview should be shown after it's fully formed. */
    view->hide();

    model = new ConfigPaneModel();

    view->setViewMode(QListView::IconMode);
    view->setWordWrap(true);
    view->setUniformItemSizes(true);
    view->setCategoryDrawer(new QCategoryDrawerV3(view));

    connect(view, &QAbstractItemView::activated, [this] (const QModelIndex & index) { pendingActivation = index; });
    view->setFocus();

    QTimer::singleShot(0, [this] { setSizing(); });
    QTimer::singleShot(1, this, SLOT(load()));
    new QShortcut{QKeySequence{Qt::CTRL + Qt::Key_Q}, this, SLOT(close())};
}

void LXQtConfig::MainWindow::load()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    proxyModel = new QCategorizedSortFilterProxyModel();
    proxyModel->setCategorizedModel(true);
    proxyModel->setSourceModel(model);

    view->setModel(proxyModel);
    view->setItemDelegate(new ConfigItemDelegate(view));

    view->show();

    QApplication::restoreOverrideCursor();
}

void LXQtConfig::MainWindow::activateItem()
{
    if (pendingActivation.isValid())
    {
        model->activateItem(pendingActivation);
        pendingActivation = QModelIndex{};
    }
}

/*Note: all this delayed activation is here to workaround the auto-repeated
 * Enter/Return key activation -> if the user keeps pressing the enter/return
 * we normaly will keep activating (spawning new processes) until the focus
 * isn't stolen from our window. New process is not spawned until the
 * (non-autorepeated) KeyRelease is delivered.
 *
 * ref https://github.com/lxqt/lxqt/issues/965
 */
bool LXQtConfig::MainWindow::eventFilter(QObject * watched, QEvent * event)
{
    if (view != watched)
        return false;
    switch (event->type())
    {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
            {
                QKeyEvent * ev = dynamic_cast<QKeyEvent *>(event);
                switch (ev->key())
                {
                    case Qt::Key_Enter:
                    case Qt::Key_Return:
                        if (QEvent::KeyRelease == ev->type() && !ev->isAutoRepeat())
                            activateItem();
                }
            }
            break;
        case QEvent::MouseButtonRelease:
            activateItem();
            break;
        default:
            //keep warnings quiet
            break;
    }
    return false;
}

void LXQtConfig::MainWindow::setSizing()
{
    // consult the style to know the icon size
    int iconSize = qBound(16, view->decorationSize().height(), 256);
    /* To have an appropriate grid size, we suppose that
     *
     * (1) The text has 3 lines and each line has 16 chars (for languages like German), at most;
     * (2) The selection rect has a margin of 2 px, at most;
     * (3) There is, at most, a 3-px spacing between text and icon; and
     * (4) There is a 4-px margin around each cell.
     */
    QFontMetrics fm = fontMetrics();
    int textWidth = fm.averageCharWidth() * 16;
    int textHeight = fm.lineSpacing() * 3;
    QSize grid;
    grid.setWidth(qMax(iconSize, textWidth) + 4);
    grid.setHeight(iconSize + textHeight + 4 + 3);
    view->setGridSize(grid + QSize(8, 8));
}

bool LXQtConfig::MainWindow::event(QEvent * event)
{
    if (QEvent::StyleChange == event->type())
        setSizing();
    return QMainWindow::event(event);
}
