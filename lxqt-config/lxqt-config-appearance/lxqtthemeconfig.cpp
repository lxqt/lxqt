/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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

#include "lxqtthemeconfig.h"
#include "ui_lxqtthemeconfig.h"
#include <QTreeWidget>
#include <QStandardPaths>
#include <QProcess>
#include <QItemDelegate>
#include <QPainter>

/*!
 * \brief Simple delegate to draw system background color below decoration/icon
 * (needed by System theme, which uses widget background and therefore provides semi-transparent preview)
 */
class ThemeDecorator : public QItemDelegate
{
public:
    using QItemDelegate::QItemDelegate;
protected:
    virtual void drawDecoration(QPainter * painter, const QStyleOptionViewItem & option, const QRect & rect, const QPixmap & pixmap) const override
    {
        //Note: can't use QItemDelegate::drawDecoration, because it is ignoring pixmap,
        //if the icon is valid (and that is set in paint())
        if (pixmap.isNull() || !rect.isValid())
            return;

        QPoint p = QStyle::alignedRect(option.direction, option.decorationAlignment, pixmap.size(), rect).topLeft();
        painter->fillRect(QRect{p, pixmap.size()}, QApplication::palette().color(QPalette::Window));
        painter->drawPixmap(p, pixmap);
    }
};

/*!
 * \brief Check if currently configured wallpaper (read from pcmanfm-qt's
 * settings) is the same as \param themeWallpaper
 */
static bool isWallpaperChanged(const QString & themeWallpaper)
{
    static const QString config_path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/pcmanfm-qt/lxqt/settings.conf");
    static const QString wallpaper_key = QStringLiteral("Desktop/Wallpaper");
    const QString current_wallpaper = QSettings{config_path, QSettings::IniFormat}.value(wallpaper_key).toString();
    return themeWallpaper != current_wallpaper;
}

LXQtThemeConfig::LXQtThemeConfig(LXQt::Settings *settings, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LXQtThemeConfig),
    mSettings(settings)
{
    ui->setupUi(this);
    {
        QScopedPointer<QAbstractItemDelegate> p{ui->lxqtThemeList->itemDelegate()};
        ui->lxqtThemeList->setItemDelegate(new ThemeDecorator{this});
    }

    const QList<LXQt::LXQtTheme> themes = LXQt::LXQtTheme::allThemes();
    for(const LXQt::LXQtTheme &theme : themes)
    {
        QString themeName = theme.name();
        themeName[0] = themeName[0].toTitleCase();
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(themeName));
        if (!theme.previewImage().isEmpty())
        {
            item->setIcon(0, QIcon(theme.previewImage()));
        }
        item->setSizeHint(0, QSize(42,42)); // make icons non-cropped
        item->setData(0, Qt::UserRole, theme.name());
        ui->lxqtThemeList->addTopLevelItem(item);
    }

    initControls();

    connect(ui->lxqtThemeList, &QTreeWidget::currentItemChanged, this, &LXQtThemeConfig::settingsChanged);
    connect(ui->wallpaperOverride, &QAbstractButton::clicked, this, &LXQtThemeConfig::settingsChanged);
}


LXQtThemeConfig::~LXQtThemeConfig()
{
    delete ui;
}


void LXQtThemeConfig::initControls()
{
    QString currentTheme = mSettings->value(QStringLiteral("theme")).toString();

    QTreeWidgetItemIterator it(ui->lxqtThemeList);
    while (*it) {
        if ((*it)->data(0, Qt::UserRole).toString() == currentTheme)
        {
            ui->lxqtThemeList->setCurrentItem((*it));
            break;
        }
        ++it;
    }

    update();
}

void LXQtThemeConfig::applyLxqtTheme()
{
    QTreeWidgetItem* item = ui->lxqtThemeList->currentItem();
    if (!item)
        return;

    LXQt::LXQtTheme currentTheme{mSettings->value(QStringLiteral("theme")).toString()};
    QVariant themeName = item->data(0, Qt::UserRole);
    if(mSettings->value(QStringLiteral("theme")) != themeName)
        mSettings->setValue(QStringLiteral("theme"), themeName);
    LXQt::LXQtTheme theme(themeName.toString());
    if(theme.isValid()) {
        QString wallpaper = theme.desktopBackground();
        if(!wallpaper.isEmpty() && (ui->wallpaperOverride->isChecked() || !isWallpaperChanged(currentTheme.desktopBackground()))) {
            // call pcmanfm-qt to update wallpaper
            QStringList args;
            args << QStringLiteral("--set-wallpaper") << wallpaper;
            QProcess::startDetached(QStringLiteral("pcmanfm-qt"), args);
        }
    }
}
