/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 * Copyright (C) 2018  Nathan Osman <nathan@quickmediasolutions.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <QMutableListIterator>

#include "application.h"
#include "mrumenu.h"
#include "settings.h"

using namespace LxImage;

MruMenu::MruMenu(QWidget *parent)
    : QMenu(parent)
{
    auto settings = static_cast<Application*>(qApp)->settings();
    mMaxItems = qMin(qMax(settings.maxRecentFiles(), 0), 100);
    mFilenames = settings.recentlyOpenedFiles();
    while (mFilenames.count() > mMaxItems) { // the config file may have been incorrectly edited
        mFilenames.removeLast();
    }
    for (QStringList::const_iterator i = mFilenames.constBegin(); i != mFilenames.constEnd(); ++i) {
        addAction(createAction(*i));
    }
    setEnabled(mMaxItems != 0);

    // Add a separator and hide it if there are no items in the list
    mSeparator = addSeparator();
    mSeparator->setVisible(!mFilenames.empty());

    // Add the clear action and disable it if there are no items in the list
    mClearAction = new QAction(tr("&Clear"));
    mClearAction->setEnabled(!mFilenames.empty());
    connect(mClearAction, &QAction::triggered, this, &MruMenu::onClearTriggered);
    addAction(mClearAction);
}

void MruMenu::addItem(const QString &filename)
{
    if (mMaxItems == 0) {
        return;
    }

    if (mFilenames.isEmpty()) {
        mSeparator->setVisible(true);
        mClearAction->setEnabled(true);
    }

    // If the filename is already in the list, remove it
    int index = mFilenames.indexOf(filename);
    if (index != -1) {
        mFilenames.removeAt(index);
        destroyAction(index);
    }

    // Insert the action at the beginning of the list
    mFilenames.push_front(filename);
    insertAction(actions().first(), createAction(filename));

    // If the list contains more than mMaxItems, remove the last one
    while (mFilenames.count() > mMaxItems) {
        destroyAction(mFilenames.count() - 1);
        mFilenames.removeLast();
    }

    updateSettings();
}

void MruMenu::setMaxItems(int m) {
    m = qMin(qMax(m, 0), 100);
    if (m == mMaxItems) {
        return;
    }
    while (mFilenames.count() > m) {
        destroyAction(mFilenames.count() - 1);
        mFilenames.removeLast();
    }
    setEnabled(m != 0);
    mMaxItems = m;

    updateSettings();
}

void MruMenu::onItemTriggered()
{
    Q_EMIT itemClicked(qobject_cast<QAction*>(sender())->text());
}

void MruMenu::onClearTriggered()
{
    while (!mFilenames.empty()) {
        mFilenames.removeFirst();
        destroyAction(0);
    }

    // Hide the separator and disable the clear action
    mSeparator->setVisible(false);
    mClearAction->setEnabled(false);

    updateSettings();
}

QAction *MruMenu::createAction(const QString &filename)
{
    QAction *action = new QAction(filename, this);
    connect(action, &QAction::triggered, this, &MruMenu::onItemTriggered);
    return action;
}

void MruMenu::destroyAction(int index)
{
    QAction *action = actions().at(index);
    removeAction(action);
    delete action;
}

void MruMenu::updateSettings()
{
    static_cast<Application*>(qApp)->settings().setRecentlyOpenedFiles(mFilenames);
}
