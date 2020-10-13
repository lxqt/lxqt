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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <LXQt/Notification>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->pushButton, SIGNAL(clicked()), SLOT(createNotification()));
    LXQt::Notification::notify(QStringLiteral("1"));
    LXQt::Notification::notify(QStringLiteral("2"));
    LXQt::Notification::notify(QStringLiteral("3"));
}

void MainWindow::createNotification()
{
    LXQt::Notification* n = new LXQt::Notification(QStringLiteral("Notification!"));
    nlist.append(n);
    connect(n, SIGNAL(actionActivated(int)), this, SLOT(clickEvent(int)));
    n->setActions(QStringList()
            << QStringLiteral("Hi")
            << QStringLiteral("Bye")
            << QStringLiteral("foo")
            << QStringLiteral("bar")
            << QStringLiteral("lorem")
            << QStringLiteral("ipsum"),
    1);
    n->setIcon(QStringLiteral("preferences-desktop-launch-feedback"));
    n->update();
}

void MainWindow::clickEvent(int button)
{
    LXQt::Notification* n = qobject_cast<LXQt::Notification*>(sender());
    if (n)
    {
        if (button == 0)
        {
            n->setBody(QStringLiteral("Hello!"));
        }
        else
        {
            n->setBody(QStringLiteral("Closing..."));
            n->setTimeout(1000);
        }
        n->update();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    qDeleteAll(nlist);
}
