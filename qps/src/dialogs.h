/*
 * dialogs.h
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


// misc. handy dialogs for use everywhere

#ifndef DIALOGS_H
#define DIALOGS_H

#include <QDialog>
#include <QSlider>
#include <QGroupBox>
#include <QLineEdit>
#include <QPixmap>

#include <QMessageBox>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>

#include "misc.h"

class IntervalDialog : public QDialog
{
    Q_OBJECT
  public:
    IntervalDialog(const char *ed_txt, bool toggle_state);

  protected slots:
    void done_dialog();
    void event_label_changed();

  public:
    QString ed_result;
    CrossBox *toggle;

  protected:
    QPushButton *ok, *cancel;
    QLabel *label;
    QLineEdit *lined;
};

class SliderDialog : public QDialog
{
    Q_OBJECT
  public:
    SliderDialog(int defaultval, int minval, int maxval);

    QString ed_result;

  protected slots:
    void slider_change(int val);
    void done_dialog();

  protected:
    QPushButton *ok, *cancel;
    QLabel *label;
    QLineEdit *lined;
    QSlider *slider;
};

class PermissionDialog : public QDialog
{
    Q_OBJECT
  public:
    PermissionDialog(QString msg, QString passwd);
    QLineEdit *lined;
    QLabel *label;

    // protected slots:
    //    void slider_change(int val);
    //  void done_dialog();
    /*
    protected:
        QPushButton *ok, *cancel;
        QLabel *label;
        QSlider *slider; */
};

class SchedDialog : public QDialog
{
    Q_OBJECT
  public:
    SchedDialog(int policy, int prio);

    int out_prio;
    int out_policy;

  protected slots:
    void done_dialog();
    void button_clicked(bool);

  private:
    QGroupBox *bgrp;
    QRadioButton *rb_other, *rb_fifo, *rb_rr;
    QLabel *lbl;
    QLineEdit *lined;
};
#endif // DIALOGS_H
