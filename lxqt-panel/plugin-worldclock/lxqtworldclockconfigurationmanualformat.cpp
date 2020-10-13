/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 *            2014 LXQt team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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


#include <QTimeZone>
#include <QPushButton>
#include "lxqtworldclockconfigurationmanualformat.h"

#include "ui_lxqtworldclockconfigurationmanualformat.h"


LXQtWorldClockConfigurationManualFormat::LXQtWorldClockConfigurationManualFormat(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LXQtWorldClockConfigurationManualFormat)
{
    setObjectName(QStringLiteral("WorldClockConfigurationManualFormatWindow"));
    setWindowModality(Qt::WindowModal);
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui->buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(ui->manualFormatPTE, SIGNAL(textChanged()), this, SIGNAL(manualFormatChanged()));
}
LXQtWorldClockConfigurationManualFormat::~LXQtWorldClockConfigurationManualFormat()
{
    delete ui;
}

void LXQtWorldClockConfigurationManualFormat::setManualFormat(const QString& text)
{
    ui->manualFormatPTE->setPlainText(text);
}

QString LXQtWorldClockConfigurationManualFormat::manualFormat() const
{
    return ui->manualFormatPTE->toPlainText();
}
