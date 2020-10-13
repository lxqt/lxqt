/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
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

#ifndef LXIMAGE_MRUMENU_H
#define LXIMAGE_MRUMENU_H

#include <QAction>
#include <QMenu>
#include <QStringList>

namespace LxImage {

/**
 * @brief Menu for quick access to most recently used (MRU) files
 */
class MruMenu : public QMenu
{
    Q_OBJECT

public:

    explicit MruMenu(QWidget *parent = Q_NULLPTR);

    /**
     * @brief Add an item to the MRU list
     * @param filename absolute path to the file
     *
     * If the item already exists in the menu, it will be moved to the top.
     */
    void addItem(const QString &filename);

    /**
     * @brief Set maximum number of items
     *
     * The last item will be removed if the number is exceeded.
     */
    void setMaxItems(int m);

Q_SIGNALS:

    /**
     * @brief Indicate that an item in the menu has been clicked
     * @param filename absolute path to the file
     */
    void itemClicked(const QString &filename);

private Q_SLOTS:

    void onItemTriggered();
    void onClearTriggered();

private:

    QAction *createAction(const QString &filename);
    void destroyAction(int index);
    void updateSettings();

    QStringList mFilenames;

    QAction *mSeparator;
    QAction *mClearAction;

    int mMaxItems;
};

}

#endif // LXIMAGE_MRUMENU_H
