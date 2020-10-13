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

#ifndef MATCOMMANDINTERFACE_H
#define MATCOMMANDINTERFACE_H

#include <QStringList>

class QCommandLineParser;
/*!
 * \brief The MatCommandInterface class
 */
class MatCommandInterface {

public:
    /*!
     * \brief MatCommandInterface
     * \param name
     * \param description
     * \param parser
     */
    explicit MatCommandInterface(const QString &name, const QString &description, QCommandLineParser *parser);

    /*!
     * \brief ~MatCommandInterface
     */
    virtual ~MatCommandInterface();

    /*!
     * \brief name
     * \return
     */
    inline QString name() const { return mName; }

    /*!
     * \brief description
     * \return
     */
    inline QString description() const { return mDescription; }

    /*!
     * \brief parser
     * \return
     */
    inline QCommandLineParser *parser() const { return mParser; }

    /*!
     * \brief run
     * \param arguments
     * \return
     */
    virtual int run(const QStringList &arguments) = 0;

    /*!
     * \brief showHelp
     * \param exitCode
     */
    virtual void showHelp(int exitCode = 0) const;

    /*!
     * \brief showVersion
     */
    virtual void showVersion() const;

private:
    QString mName;
    QString mDescription;
    QCommandLineParser *mParser;
};

#endif // MATCOMMANDINTERFACE_H
