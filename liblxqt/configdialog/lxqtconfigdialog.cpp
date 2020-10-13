/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright (C) 2012  Alec Moskvin <alecm@gmx.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "lxqtconfigdialog.h"
#include "lxqtconfigdialog_p.h"
#include "ui_lxqtconfigdialog.h"

#include <XdgIcon>
#include <QPushButton>

#include "lxqtsettings.h"

using namespace LXQt;

ConfigDialogPrivate::ConfigDialogPrivate(ConfigDialog *q, Settings *settings)
    : q_ptr(q),
      mCache(new SettingsCache(settings)),
      ui(new Ui::ConfigDialog)
{
    init();
}

ConfigDialogPrivate::~ConfigDialogPrivate()
{
    delete ui;
    delete mCache;
}

void ConfigDialogPrivate::init()
{
    Q_Q(ConfigDialog);
    ui->setupUi(q);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(QCoreApplication::translate("ConfigDialogPrivate", "Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(QCoreApplication::translate("ConfigDialogPrivate", "Close"));
    QObject::connect(ui->buttonBox_1, &QDialogButtonBox::clicked,
                     [=](QAbstractButton* button) { dialogButtonsAction(button); }
    );
    ui->moduleList->setVisible(false);
    const QList<QPushButton*> buttons = ui->buttonBox_1->findChildren<QPushButton*>();
    for(QPushButton* button : buttons)
        button->setAutoDefault(false);
}

void ConfigDialogPrivate::dialogButtonsAction(QAbstractButton* button)
{
    Q_Q(ConfigDialog);
    QDialogButtonBox::StandardButton standardButton = ui->buttonBox_1->standardButton(button);
    Q_EMIT q->clicked(standardButton);
    if (standardButton == QDialogButtonBox::Reset)
    {
        mCache->loadToSettings();
        Q_EMIT q->reset();
    }
    else if(standardButton == QDialogButtonBox::Close)
    {
        q->close();
    }
}

void ConfigDialogPrivate::updateIcons()
{
    Q_Q(ConfigDialog);
    for (int ix = 0; ix < mIcons.size(); ix++)
        ui->moduleList->item(ix)->setIcon(XdgIcon::fromTheme(mIcons.at(ix)));
    q->update();
}

ConfigDialog::ConfigDialog(const QString& title, Settings* settings, QWidget* parent) :
    QDialog(parent),
    mSettings(settings),
    d_ptr(new ConfigDialogPrivate(this, settings))
{
    setWindowTitle(title);
}

void ConfigDialog::setButtons(QDialogButtonBox::StandardButtons buttons)
{
    Q_D(ConfigDialog);
    d->ui->buttonBox_1->setStandardButtons(buttons);
    const QList<QPushButton*> b = d->ui->buttonBox_1->findChildren<QPushButton*>();
    for(QPushButton* button : b)
        button->setAutoDefault(false);
}

void ConfigDialog::enableButton(QDialogButtonBox::StandardButton which, bool enable)
{
    Q_D(ConfigDialog);
    if (QPushButton* pb = d->ui->buttonBox_1->button(which))
        pb->setEnabled(enable);
}

void ConfigDialog::addPage(QWidget* page, const QString& name, const QString& iconName)
{
    addPage(page, name, QStringList() << iconName);
}

void ConfigDialog::addPage(QWidget* page, const QString& name, const QStringList& iconNames)
{
    Q_D(ConfigDialog);
    Q_ASSERT(page);
    if (!page)
    {
        return;
    }

    /* We set the layout margin to 0. In the default configuration, one page
     *  only, it aligns buttons with the page. In multi-page it saves a little
     *  bit of space, without clutter.
     */
    if (page->layout())
    {
        page->layout()->setMargin(0);
    }

    QStringList icons = QStringList(iconNames) << QL1S("application-x-executable");
    new QListWidgetItem(XdgIcon::fromTheme(icons), name, d->ui->moduleList);
    d->mIcons.append(icons);
    d->ui->stackedWidget->addWidget(page);
    d->mPages[name] = page;
    if(d->ui->stackedWidget->count() > 1)
    {
        d->ui->moduleList->setVisible(true);
        d->ui->moduleList->setCurrentRow(0);
        d->mMaxSize = QSize(qMax(page->geometry().width() + d->ui->moduleList->geometry().width(),
                              d->mMaxSize.width()),
                         qMax(page->geometry().height() + d->ui->buttonBox_1->geometry().height(),
                              d->mMaxSize.height()));
    }
    else
    {
        d->mMaxSize = page->geometry().size();
    }
    resize(d->mMaxSize);
}

void ConfigDialog::showPage(QWidget* page)
{
    Q_D(ConfigDialog);
    int index = d->ui->stackedWidget->indexOf(page);
    if (index < 0)
        return;

    d->ui->stackedWidget->setCurrentIndex(index);
    d->ui->moduleList->setCurrentRow(index);
}

void ConfigDialog::showPage(const QString &name)
{
    Q_D(ConfigDialog);
    if (d->mPages.contains(name))
        showPage(d->mPages.value(name));
    else
        qWarning("ConfigDialog::showPage: Invalid page name (%s)", name.toLocal8Bit().constData());
}

bool ConfigDialog::event(QEvent * event)
{
    Q_D(ConfigDialog);
    if (QEvent::ThemeChange == event->type())
        d->updateIcons();
    return QDialog::event(event);
}

void ConfigDialog::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event)
    Q_EMIT save();
    mSettings->sync();
}

ConfigDialog::~ConfigDialog()
{
}
