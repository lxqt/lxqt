/*
 * qps.h
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

#ifndef QPS_H
#define QPS_H

#include <QWidget>
#include <QMenuBar>
#include <QColor>
#include <QMenu>
#include <QCloseEvent>
#include <QTimerEvent>
#include <QPixmap>
#include <QUrl>
#include <QSystemTrayIcon>

#define HTABLE1
#include "misc.h"
#include "pstable.h"
#include "proc.h"
#include "infobar.h"
#include "fieldsel.h"
#include "details.h"
#include "prefs.h"
#include "command.h"

#define FUNC_START
#define FUNC_END

class CommandDialog;

class Qps : public QWidget
{
    Q_OBJECT
  public:
    Qps();
    ~Qps() override;

    void set_update_period(int milliseconds);
    void setIconSize(int w, int h) { icon_width = w, icon_height = h; }
    void write_settings();
    void save_settings();

    QPixmap *get_load_icon();

    // settings
    static bool show_file_path;    // shows command/file map path
    static bool show_cmd_path;     // DEL CMDLINE shows command path
    static bool show_infobar;      // information bar is shown
    static bool show_ctrlbar;      // control bar is shown
    static bool show_statusbar;    //
    static bool show_mem_bar;      // memory is shown as a bar
    static bool show_swap_bar;     // swap is shown as a bar
    static bool show_cpu_bar;      // cpu states are shown as a bar
    static bool show_load_graph;   // load is shown as a graph
    static bool load_in_icon;      // load graph in icon
    static bool auto_save_options; // settings saved on exit
#ifdef LINUX
    static bool hostname_lookup; // do a name lookup on socket addresses
    static bool service_lookup;  // map port numbers to service names
#endif
    static bool pids_to_selection; // selected processes' PIDS to X11 sel
    static bool vertical_cpu_bar;  // save space with SMP

    static int swaplimit;        // limit of swap redline, in K or %
    static bool swaplim_percent; // true if swaplimit is in %
#ifdef SOLARIS
    static bool
        normalize_nice;   // whether nice should be normalized (normal nice
                          // is 0 etc)
    static bool use_pmap; // use /proc/bin/pmap for map names
#endif
    static bool tree_gadgets; // use open/close gadgets (triangles)
    static bool tree_lines;   // draw tree branch lines

    //		static bool comm_is_magic;	//DEL, auto-remove COMM
    // when
    // switching
    static bool flag_systray; // trayicon , sytemtray
    static bool flag_exit;
    static bool save_pos;
    static bool flag_show; // last qps-main_window state
    static bool flag_useTabView;
    static bool flag_qps_hide;

    // colors which may be set by the user
    enum
    {
        COLOR_CPU_USER,
#ifdef LINUX
        COLOR_CPU_NICE,
#endif
        COLOR_CPU_SYS,
#ifdef SOLARIS
        COLOR_CPU_WAIT,
#endif
        COLOR_CPU_IDLE,
        COLOR_MEM_USED,
        COLOR_MEM_BUFF,
        COLOR_MEM_CACHE,
        COLOR_MEM_FREE,
        COLOR_SWAP_USED,
        COLOR_SWAP_FREE,
        COLOR_SWAP_WARN,
        COLOR_LOAD_BG,
        COLOR_LOAD_FG,
        COLOR_LOAD_LINES,
        COLOR_SELECTION_BG,
        COLOR_SELECTION_FG,
        NUM_COLORS
    };

    enum menuid
    {
        MENU_SIGQUIT,
        MENU_SIGILL,
        MENU_SIGABRT,
        MENU_SIGFPE,
        MENU_SIGSEGV,
        MENU_SIGPIPE,
        MENU_SIGALRM,
        MENU_SIGUSR1,
        MENU_SIGUSR2,
        MENU_SIGCHLD,
        MENU_SIGCONT,
        MENU_SIGSTOP,
        MENU_SIGTSTP,
        MENU_SIGTTIN,
        MENU_SIGTTOU,
        MENU_RENICE,
        MENU_SCHED,
        MENU_DETAILS,
        MENU_PARENT,
        MENU_CHILD,
        MENU_DYNASTY,
        MENU_SIGTERM,
        MENU_SIGHUP,
        MENU_SIGINT,
        MENU_SIGKILL,
        MENU_OTHERS,
        MENU_PROCS,
        MENU_FIELDS,
        MENU_CUSTOM,
        MENU_PATH,
        MENU_INFOBAR,
        MENU_CTRLBAR,
        MENU_CUMUL,
        MENU_PREFS,
        MENU_ADD_HIDDEN,
        MENU_REMOVE_HIDDEN,
        MENU_STATUS,
#ifdef MOSIX
        POPUP_MIGRATE,
#endif
        MENU_FIRST_COMMAND // must be last
    };

  public slots:
    void clicked_trayicon(QSystemTrayIcon::ActivationReason);
    void clicked_trayicon();
    void sig_term();
    void sig_hup();
    void sig_stop();
    void sig_cont();
    void sig_kill();
    void signal_menu(QAction *);
    void run_command(QAction *);
    void about();
    void license();
    void menu_update();
    void menu_toggle_path();
    void menu_toggle_infobar();
    void menu_toggle_ctrlbar();
    void menu_toggle_statusbar();
    void menu_toggle_cumul();
    void menu_prefs();
    void menu_renice();
    void menu_sched();
    void Action_Detail();
    void menu_parent();
    void menu_children();
    void menu_dynasty();
    void menu_custom();
    void menu_remove_field();
    void menu_edit_cmd();
    void mig_menu(int id); // MOSIX only
    void make_command_menu();
    void view_menu(QAction *);
    void save_quit();
    void add_fields_menu(QAction *act);

    void show_popup_menu(QPoint p);
    void context_heading_menu(QPoint p, int col);

    void field_added(int index, bool fromContextMenu = false);
    void field_removed(int index);
    void set_table_mode(bool treemode); // hmmm

    void open_details(int row);

    void config_change();

    void update_timer();
    void refresh();
    void test_popup(const QUrl &link);
    void update_menu_status();

  protected:
    // reimplementation of QWidget methods
    void timerEvent(QTimerEvent *) override;
    void closeEvent(QCloseEvent *) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    // bool event(QEvent *e);
    void transfer_selection();
    void adjust_popup_menu();
    bool all_selected_stopped();

    void locate_relatives(int Procinfo::*a, int Procinfo::*b);
    QMenu *make_signal_popup_menu();
#ifdef MOSIX
    QMenu *make_migrate_menu();
    void migrate_selected(int sig);
#endif
    void send_to_selected(int sig);
    void sendsig(Procinfo *p, int sig);
    bool read_settings();

    bool load_settings();

    void bar_visibility();
    void set_default_icon();
    void set_load_icon();

  public:
    Pstable *pstable;

    Procview *procview;
    StatusBar *statusBar;
    // Netable *netable;
    QTextEdit *logbox;
    void update_icon();

  private:
    QMenuBar *menubar;
    QMenu *m_field; // Field Menu
    QMenu *m_view;  // Field Menu
    QAction *am_view;
    QMenu *m_command;
    QMenu *m_options;
    QMenu *m_popup;
    QMenu *m_headpopup;
    QMenu *m_fields;
    QMenu *popupx; // testing

    bool selection_items_enabled; // state of certain menu items (???)

    Infobar2 *infobar; // tmp
    ControlBar *ctrlbar;
    FieldSelect *field_win;
    Preferences *prefs_win;
    CommandDialog *command_win;

    QList<Details> details;

    int update_period;                      // in mili second
    static const int eternity = 0x7fffffff; // turns off automatic update
    // the Linux kernel updates its load average counters every 5 seconds
    // (kernel 2.0.29), so there is no point in reading it too often
    static const int load_update_period = 4800; // in ms
    int update_load_time;                       // time left to update load

    // size of our icon
    int icon_width;
    int icon_height;
    int timer_id;

    QPixmap *default_icon;
    bool default_icon_set; // true if default icon is current icon
    int context_col;       // where heading context menu was clicked

    QPoint winPos;
};

#endif // QPS_H
