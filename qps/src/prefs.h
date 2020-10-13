/*
 * prefs.h
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

#ifndef PREFS_H
#define PREFS_H

#include <qdialog.h>
#include <qradiobutton.h>
#include <qlabel.h>
//#include <q3buttongroup.h>
#include <QtGui>
#include "misc.h"

class Preferences : public QDialog
{
    Q_OBJECT
  public:
    Preferences(QWidget *parent = nullptr);
    QComboBox *psizecombo;
    //    QFontComboBox *font_cb;
    QRadioButton *rb_totalcpu;
    void init_font_size();

  public slots:
    void update_boxes();
    void update_reality();
    void update_config();
    void closed();
    void font_changed(int);
    void fontset_changed(int);
signals:
    void prefs_change();

  protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // PREFS_H
