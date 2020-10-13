/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2010-2012 LXQt team
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

#ifndef DEFAULTAPPS_H
#define DEFAULTAPPS_H

#include <QWidget>

namespace Ui {
class DefaultAppsPage;
}

class DefaultApps : public QWidget
{
    Q_OBJECT

public:
    explicit DefaultApps(QWidget *parent = nullptr);
    ~DefaultApps() override;

signals:
    void defaultAppChanged(const QString&, const QString&);

public slots:
    void updateEnvVar(const QString &var, const QString &val);

private:
    Ui::DefaultAppsPage *ui;

private slots:
    void browserButton_clicked();
    void terminalButton_clicked();
    void browserChanged();
    void terminalChanged();
};

#endif // DEFAULTAPPS_H
