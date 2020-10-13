/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *   Dmitriy Zhukov <zjesclean@gmail.com>
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

#ifndef _KBDSTATECONFIG_H_
#define _KBDSTATECONFIG_H_

#include <QDialog>

namespace Ui {
class KbdStateConfig;
}

class KbdStateConfig : public QDialog
{
    Q_OBJECT

public:
    explicit KbdStateConfig(QWidget *parent = nullptr);
    ~KbdStateConfig();
private:
    void save();
    void load();

    void configureLayouts();
private:
    Ui::KbdStateConfig *m_ui;
};

#endif
