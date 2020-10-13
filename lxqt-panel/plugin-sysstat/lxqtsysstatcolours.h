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


#ifndef LXQTSYSSTATCOLOURS_HPP
#define LXQTSYSSTATCOLOURS_HPP

#include <QDialog>

#include <QMap>
#include <QString>
#include <QColor>


namespace Ui {
    class LXQtSysStatColours;
}

class QSignalMapper;
class QAbstractButton;
class QPushButton;

class LXQtSysStatColours : public QDialog
{
    Q_OBJECT

public:
    explicit LXQtSysStatColours(QWidget *parent = nullptr);
    ~LXQtSysStatColours();

    typedef QMap<QString, QColor> Colours;

    void setColours(const Colours&);

    Colours colours() const;

    Colours defaultColours() const;

signals:
    void coloursChanged();

public slots:
    void on_buttons_clicked(QAbstractButton*);

    void selectColour(const QString &);

    void restoreDefaults();
    void reset();
    void apply();

private:
    Ui::LXQtSysStatColours *ui;

    QSignalMapper *mSelectColourMapper;
    QMap<QString, QPushButton*> mShowColourMap;

    Colours mDefaultColours;
    Colours mInitialColours;
    Colours mColours;

    void applyColoursToButtons();
};

#endif // LXQTSYSSTATCOLOURS_HPP
