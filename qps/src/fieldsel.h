/*
 * fieldsel.h
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 1997-1999 Mattias Engdegård
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef FIELDSEL_H
#define FIELDSEL_H

#include <QBitArray>
#include <QDialog>
#include <QCheckBox>

#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QCloseEvent>

#include "proc.h"

class FieldSelect : public QDialog
{
    Q_OBJECT
  public:
    FieldSelect(Procview *pv);

    void update_boxes();

  public slots:
    void field_toggled(bool);
    void closed();

signals:
    void added_field(int);
    void removed_field(int);

  protected:
    QCheckBox **buts;
    int nbuttons;
    QBitArray disp_fields;
    bool updating;
    Procview *procview;

    void set_disp_fields();
    void closeEvent(QCloseEvent *) override;
    void showEvent(QShowEvent *) override;
};

#endif // FIELDSEL_H
