/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2010-2011 LXQt team
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

#ifndef SESSIONCONFIGWINDOW_H
#define SESSIONCONFIGWINDOW_H

#include <QComboBox>
#include <LXQt/ConfigDialog>


class SessionConfigWindow : public LXQt::ConfigDialog
{
    Q_OBJECT

public:
    SessionConfigWindow();
    ~SessionConfigWindow() override;

    static void handleCfgComboBox(QComboBox * cb,
                           const QStringList &availableValues,
                           const QString &value
                          );

    static void updateCfgComboBox(QComboBox * cb, const QString &prompt);

    void closeEvent(QCloseEvent * event) override;

public slots:
    void setRestart();

private:
    // display restart warning
    bool m_restart;

private slots:
    void clearRestart();
};

#endif
