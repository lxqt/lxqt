/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2018  Luís Pereira <luis.artur.pereira@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef MATCOMMANDMANAGER_H
#define MATCOMMANDMANAGER_H

#include <QList>

class MatCommandInterface;

/*!
 * \brief The MatCommandManager class
 */
class MatCommandManager {

public:
    /*!
     * \brief MatCommandManager
     */
    MatCommandManager();

    /*!
     * \brief ~MatCommandManager
     */
    virtual ~MatCommandManager();

    /*!
     * \brief add
     * \param cmd
     */
    void add(MatCommandInterface *cmd);

    /*!
     * \brief commands
     * \return
     */
    QList<MatCommandInterface *> commands() const;

    /*!
     * \brief descriptionsHelpText
     * \return
     */
    QString descriptionsHelpText() const;

private:
    QList <MatCommandInterface *> mCommands;
};

#endif // MATCOMMANDMANAGER_H
