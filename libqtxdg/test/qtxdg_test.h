/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013~2014 LXQt team
 * Authors:
 *   Christian Surlykke <christian@surlykke.dk>
 *   Luís Pereira <luis.artur.pereira@gmail.com>
 *   Jerome Leclanche <jerome@leclan.ch>
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


#ifndef QTXDG_TEST_H
#define QTXDG_TEST_H

#include <QObject>
#include <QString>
#include <QDebug>

class QtXdgTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCustomFormat();

private:
    // Test that XdgDesktopFile and xdg-mime script agree on
    // default application for each mime-type.
    void testDefaultApp();

    void testTextHtml();
    void testMeldComparison();
    void compare(QString mimetype);
    QString xdgDesktopFileDefaultApp(QString mimetype);
    QString xdgUtilDefaultApp(QString mimetype);

};

#endif /* QTXDG_TEST_H */
