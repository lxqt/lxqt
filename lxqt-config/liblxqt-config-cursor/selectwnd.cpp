/* coded by Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 * (c)DWTFYW
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */

// 2014-04-10 modified by Hong Jen Yee (PCMan) for integration with lxqt-config-input

#include <QDebug>

#include "selectwnd.h"
#include "ui_selectwnd.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QTimer>
#include <QWidget>
#include <QPushButton>
#include <QToolTip>

#include "cfgfile.h"
#include "crtheme.h"
#include "thememodel.h"
#include "itemdelegate.h"

#include "xcrimg.h"
#include "xcrxcur.h"
#include "xcrtheme.h"

#include <LXQt/Settings>
#include <XdgIcon>
#include <QTextStream>
#include <QProcess>

#include <QX11Info>
#include <X11/Xcursor/Xcursor.h>

const QString HOME_ICON_DIR(QDir::homePath() + QStringLiteral("/.icons"));

SelectWnd::SelectWnd(LXQt::Settings* settings, QWidget *parent)
    : QWidget(parent),
      mSettings(settings),
      ui(new Ui::SelectWnd)
{
    ui->setupUi(this);
    ui->warningLabel->hide();
    ui->preview->setCurrentCursorSize(XcursorGetDefaultSize(QX11Info::display()));
    ui->preview->setCursorSize(ui->preview->getCurrentCursorSize());

    mModel = new XCursorThemeModel(this);

    int size = style()->pixelMetric(QStyle::PM_LargeIconSize);
    ui->lbThemes->setModel(mModel);
    ui->lbThemes->setItemDelegate(new ItemDelegate(this));
    ui->lbThemes->setIconSize(QSize(size, size));
    ui->lbThemes->setSelectionMode(QAbstractItemView::SingleSelection);

    // Make sure we find out about selection changes
    connect(ui->lbThemes->selectionModel(), &QItemSelectionModel::currentChanged, this, &SelectWnd::currentChanged);
    // display/hide warning label
    connect(mModel, SIGNAL(modelReset()),
                    this, SLOT(handleWarning()));
    connect(mModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
                    this, SLOT(handleWarning()));
    connect(mModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
                    this, SLOT(handleWarning()));

    connect(ui->warningLabel, SIGNAL(showDirInfo()),
                    this, SLOT(showDirInfo()));
    
    // Set actual cursor size
    ui->cursorSizeSpinBox->setValue(XcursorGetDefaultSize(QX11Info::display()));
    
    connect(ui->cursorSizeSpinBox, SIGNAL(	valueChanged(int)),
                    this, SLOT(cursorSizeChaged(int)));

    // Disable the install button if we can't install new themes to ~/.icons,
    // or Xcursor isn't set up to look for cursor themes there
    ui->btInstall->setEnabled(mModel->searchPaths().contains(HOME_ICON_DIR) && iconsIsWritable());
    // TODO/FIXME: btInstall functionality
    ui->btInstall->hide();
    ui->btRemove->hide();

    //QTimer::singleShot(0, this, SLOT(setCurrent()));

    handleWarning();
}


SelectWnd::~SelectWnd()
{
    delete ui;
}

void SelectWnd::setCurrent()
{
    ui->lbThemes->selectionModel()->clear();

    QString ct = getCurrentTheme();
    mAppliedIndex = mModel->defaultIndex();

    if (!ct.isEmpty()) mAppliedIndex = mModel->findIndex(ct);
    else mAppliedIndex = mModel->defaultIndex();

    if (mAppliedIndex.isValid())
    {
        const XCursorThemeData *theme = mModel->theme(mAppliedIndex);
        // Select the current theme
        selectRow(mAppliedIndex);
        ui->lbThemes->scrollTo(mAppliedIndex, QListView::PositionAtCenter);
        // Update the preview widget as well
        if (theme) ui->preview->setTheme(theme);// else ui->preview->clearTheme();
    }
}

bool SelectWnd::iconsIsWritable() const
{
    const QFileInfo icons = QFileInfo(HOME_ICON_DIR);
    const QFileInfo home = QFileInfo(QDir::homePath());
    return ((icons.exists() && icons.isDir() && icons.isWritable()) || (!icons.exists() && home.isWritable()));
}

/*
void SelectWnd::keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Qt::Key_Escape) close();
}
*/

void SelectWnd::selectRow(int row) const
{
    // Create a selection that stretches across all columns
    QModelIndex from = mModel->index(row, 0);
    QModelIndex to = mModel->index(row, mModel->columnCount()-1);
    QItemSelection selection(from, to);
    ui->lbThemes->selectionModel()->select(selection, QItemSelectionModel::Select);
    ui->lbThemes->selectionModel()->setCurrentIndex(mAppliedIndex, QItemSelectionModel::NoUpdate);
}

void SelectWnd::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    if (current.isValid()) {
        const XCursorThemeData *theme = mModel->theme(current);
        if (theme) {
            ui->preview->setTheme(theme);
            ui->btRemove->setEnabled(theme->isWritable());
        } else {
            ui->preview->clearTheme();
        }

        // don't apply the current settings here
    } else {
        ui->preview->clearTheme();
    }
    emit settingsChanged();
}

void SelectWnd::on_btInstall_clicked()
{
    qDebug() << "'install' clicked";
}

void SelectWnd::applyCusorTheme()
{
    QModelIndex curIndex = ui->lbThemes->currentIndex();
    if(!curIndex.isValid()) return;
    const XCursorThemeData *theme = mModel->theme(curIndex);
    
    if(!theme ||
        (
            mSettings->value(QStringLiteral("Mouse/cursor_theme")) == theme->name()
            && mSettings->value(QStringLiteral("Mouse/cursor_size")) == ui->cursorSizeSpinBox->value()
        )
    ) {
        return;
    }
    
    applyTheme(*theme, ui->cursorSizeSpinBox->value());
    fixXDefaults(theme->name(), ui->cursorSizeSpinBox->value());

    // call xrdb to merge the new settings in ~/.Xdefaults
    // FIXME: need to check if we're running in X?
    QProcess xrdb;
    xrdb.start(QStringLiteral("xrdb -merge ") + QDir::home().path() + QStringLiteral("/.Xdefaults"));
    xrdb.waitForFinished();

    // old razor-qt and lxqt versions use $XCURSOR_THEME environment variable
    // for this, but it's less flexible and more problematic. Let's deprecate its use.
    mSettings->beginGroup(QStringLiteral("Environment"));
    mSettings->remove(QStringLiteral("XCURSOR_THEME")); // ensure that we're not using XCURSOR_THEME
    mSettings->endGroup();
    // save to Mouse/cursor_theme instead
    mSettings->beginGroup(QStringLiteral("Mouse"));
    mSettings->setValue(QStringLiteral("cursor_theme"), theme->name());
    mSettings->setValue(QStringLiteral("cursor_size"), ui->cursorSizeSpinBox->value());
    mSettings->endGroup();

    // The XCURSOR_THEME environment variable does not work sometimes.
    // Besides, XDefaults values are not used by Qt.
    // So, let's write the new theme name to ~/.icons/default/index.theme.
    // This is the most reliable way.
    // QSettings will encode the group name "Icon Theme" to "Icon%20Theme" and there is no way to turn it off.
    // So let's not use it here. :-(
    QString dirPath = HOME_ICON_DIR + QStringLiteral("/default");
    QDir().mkpath(dirPath); // ensure the existence of the ~/.icons/default dir
    QFile indexTheme(dirPath + QStringLiteral("/index.theme"));
    if(indexTheme.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
        QTextStream(&indexTheme) <<
        "# Written by lxqt-config-appearance\n" <<
        "[Icon Theme]\n" <<
        "Name=Default\n" <<
        "Comment=Default cursor theme\n" <<
        "Inherits=" << theme->name() << "\n" <<
        "Size=" << ui->cursorSizeSpinBox->value() << "\n";
        indexTheme.close();
    }
}

void SelectWnd::on_btRemove_clicked()
{
    qDebug() << "'remove' clicked";
    const XCursorThemeData *theme = mModel->theme(ui->lbThemes->currentIndex());
    if (!theme) return;
    QString ct = getCurrentTheme();
    if (ct == theme->name())
    {
        QMessageBox::warning(this, tr("XCurTheme error"),
                             tr("You can't remove active theme!"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    QDir d(theme->path());
    ui->preview->clearTheme();
    mModel->removeTheme(ui->lbThemes->currentIndex());
    removeXCursorTheme(d);
}

void SelectWnd::handleWarning()
{
    bool empty = mModel->rowCount();
    ui->warningLabel->setVisible(!empty);
    ui->preview->setVisible(empty);
    ui->infoLabel->setVisible(empty);
}

void SelectWnd::showDirInfo()
{
    QToolTip::showText(mapToGlobal(ui->warningLabel->buttonPos()), mModel->searchPaths().join(QStringLiteral("\n")));
}

void SelectWnd::cursorSizeChaged(int size)
{
    ui->preview->setCursorSize(size);
    emit settingsChanged();
}
