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

#ifndef ENVIRONMENTPAGE_H
#define ENVIRONMENTPAGE_H

#include <QTreeWidgetItem>
#include <LXQt/Settings>

namespace Ui {
class EnvironmentPage;
}

class EnvironmentPage : public QWidget
{
    Q_OBJECT

public:
    explicit EnvironmentPage(LXQt::Settings *settings, QWidget *parent = nullptr);
    ~EnvironmentPage() override;

signals:
    void needRestart();
    void envVarChanged(const QString&, const QString&);

public slots:
    void restoreSettings();
    void save();
    void updateItem(const QString &var, const QString &val);

private:
    LXQt::Settings *m_settings;
    Ui::EnvironmentPage *ui;

private slots:
    void addButton_clicked();
    void deleteButton_clicked();
    void itemChanged(QTreeWidgetItem *item, int column);
};

#endif // ENVIRONMENTPAGE_H
