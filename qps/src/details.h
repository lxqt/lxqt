/*
 * details.h
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

#ifndef DETAILS_H
#define DETAILS_H

#include <QTabWidget>
#include <QFrame>
#include <QHash>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QLayout>

#include "proc.h"
#include "lookup.h"
#include "tablefield.h"
#include "htable.h"

class Details : public QWidget
{
    Q_OBJECT
  public:
    Details(Procinfo *p, Proc *proc);
    ~Details() override;

    void refresh();
    void config_change();
    void process_gone();
    Procinfo *get_procinfo() { return pi; }
    Proc *proc() { return pr; }
    void set_procinfo(Procinfo *p);

  protected:
    void resizeEvent(QResizeEvent *) override;

  private:
    QTabWidget *tbar;
    Procinfo *pi;
    Proc *pr;
};

// SimpleTable: a HeadedTable with fixed number of columns
class SimpleTable : public HeadedTable
{
    Q_OBJECT
  public:
    SimpleTable(QWidget *parent, int nfields, TableField *f, int options = 0);
    QSize sizeHint() const override;
    virtual void refresh(){};

  protected:
    QString title(int col) override;
    QString dragTitle(int col) override;
    QString text(int row, int col) override = 0;
    int colWidth(int col) override;
    int alignment(int col) override;
    virtual int leftGap(int col);
    QString tipText(int col) override;
    //   Procinfo *procinfo() { return ((Details
    //   *)parentWidget())->procinfo(); }
    Procinfo *procinfo() { return detail->get_procinfo(); }
    Proc *proc() { return detail->proc(); }

  private:
    const TableField *fields;
    Details *detail;
};

class Sockets : public SimpleTable
{
    Q_OBJECT
  public:
    Sockets(QWidget *parent);
    ~Sockets() override;

    void refresh() override;
    void refresh_window();
    bool refresh_sockets();
    const char *servname(unsigned port);
    QString ipAddr(unsigned addr);
    QString hostname(unsigned addr);
    void config_change();

  public slots:
    void update_hostname(unsigned addr);

  protected:
    QString text(int row, int col) override;

  private:
    enum
    {
        FD,
        PROTO,
        RECVQ,
        SENDQ,
        LOCALADDR,
        LOCALPORT,
        REMOTEADDR,
        REMOTEPORT,
        STATE,
        SOCKFIELDS
    };
    static TableField *fields();

    bool doing_lookup; // if table painted with host lookup

    static Lookup *lookup;
    static bool have_services; // true if we have tried reading services
    static QHash<int, char *> servdict;
};

class Maps : public SimpleTable
{
  public:
    Maps(QWidget *parent);
    ~Maps() override;

    void refresh() override;
    void refresh_window();
    bool refresh_maps();

  protected:
    QString text(int row, int col) override;

  private:
    enum
    {
        ADDRESS,
        SIZE,
        PERM,
        OFFSET,
        DEVICE,
        INODE,
        FILENAME,
        MAPSFIELDS
    };
    static TableField *fields();
};

class Files : public SimpleTable
{
  public:
    Files(QWidget *parent);
    ~Files() override;

    void refresh() override;
    void refresh_window();
    bool refresh_fds();

  protected:
    QString text(int row, int col) override;

  private:
    enum
    {
        FILEDESC,
#ifdef LINUX
        FILEMODE,
#endif
        FILENAME,
        FILEFIELDS
    };
    static TableField *fields();
};

class Environ : public SimpleTable
{
    Q_OBJECT
  public:
    Environ(QWidget *parent);
    ~Environ() override;

    void refresh() override;
    void refresh_window();

  protected:
    QString text(int row, int col) override;

  private:
    enum
    {
        ENVNAME,
        ENVVALUE,
        ENVFIELDS
    };
    static TableField *fields();
    QVector<NameValue> sorted_environs;
};

class AllFields : public SimpleTable
{
  public:
    AllFields(QWidget *parent);
    ~AllFields() override;

    void refresh() override;
    void refresh_window();

  protected:
    QString text(int row, int col) override;

  private:
    enum
    {
        FIELDNAME,
        FIELDDESC,
        FIELDVALUE,
        FIELDSFIELDS
    };
    static TableField *fields();
    QList<Category*> sorted_cats;
};

#endif // DETAILS_H
