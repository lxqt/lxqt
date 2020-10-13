/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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
#include "lxqtsysstatcolours.h"
#include "ui_lxqtsysstatcolours.h"
#include <QPushButton>
#include <QSignalMapper>
#include <QColorDialog>
LXQtSysStatColours::LXQtSysStatColours(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LXQtSysStatColours),
    mSelectColourMapper(new QSignalMapper(this))
{
    setWindowModality(Qt::WindowModal);
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::RestoreDefaults)->setText(tr("RestoreDefaults"));
    ui->buttonBox_3->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui->buttonBox_4->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox_5->button(QDialogButtonBox::Apply)->setText(tr("Apply"));
    mDefaultColours[QStringLiteral("grid")]  = QColor("#808080");
    mDefaultColours[QStringLiteral("title")] = QColor("#000000");

    mDefaultColours[QStringLiteral("cpuSystem")]    = QColor("#800000");
    mDefaultColours[QStringLiteral("cpuUser")]      = QColor("#000080");
    mDefaultColours[QStringLiteral("cpuNice")]      = QColor("#008000");
    mDefaultColours[QStringLiteral("cpuOther")]     = QColor("#808000");
    mDefaultColours[QStringLiteral("cpuFrequency")] = QColor("#808080");

    mDefaultColours[QStringLiteral("memApps")]    = QColor("#000080");
    mDefaultColours[QStringLiteral("memBuffers")] = QColor("#008000");
    mDefaultColours[QStringLiteral("memCached")]  = QColor("#808000");
    mDefaultColours[QStringLiteral("memSwap")]    = QColor("#800000");

    mDefaultColours[QStringLiteral("netReceived")]    = QColor("#000080");
    mDefaultColours[QStringLiteral("netTransmitted")] = QColor("#808000");


#undef CONNECT_SELECT_COLOUR
#define CONNECT_SELECT_COLOUR(VAR) \
    connect(ui-> VAR ## B, SIGNAL(clicked()), mSelectColourMapper, SLOT(map())); \
    mSelectColourMapper->setMapping(ui-> VAR ## B, QString::fromLatin1( #VAR )); \
    mShowColourMap[QString::fromLatin1( #VAR )] = ui-> VAR ## B;

    CONNECT_SELECT_COLOUR(grid)
    CONNECT_SELECT_COLOUR(title)
    CONNECT_SELECT_COLOUR(cpuSystem)
    CONNECT_SELECT_COLOUR(cpuUser)
    CONNECT_SELECT_COLOUR(cpuNice)
    CONNECT_SELECT_COLOUR(cpuOther)
    CONNECT_SELECT_COLOUR(cpuFrequency)
    CONNECT_SELECT_COLOUR(memApps)
    CONNECT_SELECT_COLOUR(memBuffers)
    CONNECT_SELECT_COLOUR(memCached)
    CONNECT_SELECT_COLOUR(memSwap)
    CONNECT_SELECT_COLOUR(netReceived)
    CONNECT_SELECT_COLOUR(netTransmitted)

#undef CONNECT_SELECT_COLOUR

    connect(mSelectColourMapper, SIGNAL(mapped(const QString &)), SLOT(selectColour(const QString &)));
}

LXQtSysStatColours::~LXQtSysStatColours()
{
    delete ui;
}

void LXQtSysStatColours::selectColour(const QString &name)
{
    QColor color = QColorDialog::getColor(mColours[name], this);
    if (color.isValid())
    {
        mColours[name] = color;
        mShowColourMap[name]->setStyleSheet(QStringLiteral("background-color: %1;\ncolor: %2;").arg(color.name()).arg((color.toHsl().lightnessF() > 0.5) ? QStringLiteral("black") : QStringLiteral("white")));

        ui->buttonBox_5->button(QDialogButtonBox::Apply)->setEnabled(true);
    }
}

void LXQtSysStatColours::setColours(const Colours &colours)
{
    mInitialColours = colours;
    mColours = colours;
    applyColoursToButtons();

    ui->buttonBox_5->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void LXQtSysStatColours::applyColoursToButtons()
{
    Colours::ConstIterator M = mColours.constEnd();
    for (Colours::ConstIterator I = mColours.constBegin(); I != M; ++I)
    {
        const QColor &color = I.value();
        mShowColourMap[I.key()]->setStyleSheet(QStringLiteral("background-color: %1;\ncolor: %2;").arg(color.name()).arg((color.toHsl().lightnessF() > 0.5) ? QStringLiteral("black") : QStringLiteral("white")));
    }
}

void LXQtSysStatColours::on_buttons_clicked(QAbstractButton *button)
{
    switch (ui->buttonBox_2->standardButton(button))
    {
    case QDialogButtonBox::RestoreDefaults:
        restoreDefaults();
        break;

    case QDialogButtonBox::Reset:
        reset();
        break;

    case QDialogButtonBox::Ok:
        apply();
        accept();
        break;

    case QDialogButtonBox::Apply:
        apply();
        break;

    case QDialogButtonBox::Cancel:
        reset();
        reject();
        break;

    default:;
    }
}

void LXQtSysStatColours::restoreDefaults()
{
    bool wereTheSame = mColours == mDefaultColours;

    mColours = mDefaultColours;
    applyColoursToButtons();

    ui->buttonBox_5->button(QDialogButtonBox::Apply)->setEnabled(!wereTheSame);
}

void LXQtSysStatColours::reset()
{
    bool wereTheSame = mColours == mInitialColours;

    mColours = mInitialColours;
    applyColoursToButtons();

    ui->buttonBox_5->button(QDialogButtonBox::Apply)->setEnabled(!wereTheSame);
}

void LXQtSysStatColours::apply()
{
    emit coloursChanged();

    ui->buttonBox_5->button(QDialogButtonBox::Apply)->setEnabled(false);
}

LXQtSysStatColours::Colours LXQtSysStatColours::colours() const
{
    return mColours;
}

LXQtSysStatColours::Colours LXQtSysStatColours::defaultColours() const
{
    return mDefaultColours;
}
