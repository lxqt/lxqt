/*
 * proc.h
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

#ifndef PROC_H
#define PROC_H

#include "config.h"
#include <QCoreApplication>

#ifdef SOLARIS
#include <kstat.h> // kstat_ctl_t
#endif

#include <QHash>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QDir>

#include <sys/types.h>

class Procinfo;
int read_file(char *name, char *buf, int max);
int read_file_addNULL(char *name, void *buf, int max);
void check_system_requirement();
int get_kernel_version();

//#define F_PID 0x00000000
enum fields
{
    F_PID = 0,
#ifdef LINUX
    F_TGID,
#endif
    F_PPID,
    F_PGID,
    F_SID,
    F_TTY,
#ifdef LINUX
    F_TPGID,
#endif
#ifdef MOSIX
    F_MIGR,
    F_LOCKED,
    F_NMIGS,
    F_NOMOVE,
    F_RPID,
#endif
    F_USER,
    F_GROUP,
    F_UID,
    F_EUID,
#ifdef LINUX
    F_SUID,
    F_FSUID,
#endif
    F_GID,
    F_EGID,
#ifdef LINUX
    F_SGID,
    F_FSGID,
#endif
    F_PRI,
    F_NICE,
    F_PLCY,
    F_RPRI,
#ifdef LINUX
    F_TMS,
    F_AFFCPU,
    F_SLPAVG,
#endif
    F_NLWP,
#ifdef SOLARIS
    F_ARCH,
#endif
    F_MAJFLT,
    F_MINFLT,
#ifdef LINUX
    F_TRS,
    F_DRS,
    F_STACK,
#endif
    F_SIZE, // VSIZE
    F_SWAP,
    F_MEM,
    F_RSS,
#ifdef LINUX
    F_SHARE,
    F_DT,
    F_IOW,
    F_IOR,
#endif
    F_STAT,
    F_FLAGS,
    F_WCHAN,
    F_WCPU,
    F_CPU,  /* %CPU */
    F_PMEM, // F_PMEM: %MEM
    F_START,
    F_TIME,
    F_CPUNUM,
    F_CMD,
    F_PROCESSNAME, // NEW
    F_CWD,
    F_ROOT, //?
    F_CMDLINE,
    F_END = -1
};

class Details;

#ifdef LINUX

class Sockinfo
{
  public:
    enum proto_t
    {
        TCP,
        UDP
    };
    proto_t proto;
    unsigned char st;
    unsigned char tr;
    unsigned local_addr;
    unsigned rem_addr;
    unsigned short local_port;
    unsigned short rem_port;
    unsigned tx_queue;
    unsigned rx_queue;
    unsigned tm_when;
    unsigned rexmits;
    int uid; //??
    int pid;
    int timeout;
    int inode;
};

class SockInode
{
  public:
    SockInode(int descr, unsigned long ino) : fd(descr), inode(ino){};
    int fd;
    unsigned long inode;
};

class UnixSocket
{
  public:
    unsigned long inode;
    QString name;
    unsigned flags;
    unsigned char type;  // SOCK_STREAM or SOCK_DGRAM
    unsigned char state; // SS_FREE, SS_UNCONNECTED, SS_CONNECTING,
                         // SS_CONNECTED, SS_DISCONNECTING
};

#endif // LINUX

// COMMON
class Mapsinfo
{
  public:
    unsigned long from, to;
    unsigned long offset;
    unsigned long inode;
    QString filename; // null if name unknown
    char perm[4];     // "rwx[ps]"; last is private/shared flag
    unsigned minor, major;
};

#define OPEN_READ 1
#define OPEN_WRITE 2

class Fileinfo
{
  public:
    Fileinfo(int descr, QString name, int open_mode = 0)
        : fd(descr), filename(name), mode(open_mode){};
    int fd;
    QString filename; // "major:minor inode" in Linux 2.0,
    // texual description in Solaris 2.6
    unsigned mode; // bits from OPEN_* above (Linux only)
};

class NameValue
{
  public:
    NameValue(){};
    // NameValue(const NameValue &nv){ name=nv.name; value=nv.value; };
    NameValue(const char *n, const char *val) : name(n), value(val){};
    QString name;
    QString value;
};

class Category
{
  public:
    Category( const QString &heading, const QString &explain ) : name(heading), help(explain), reversed(false),flag_int_value(false){}
    virtual ~Category();

    virtual int alignment() = 0;
    virtual QString string(Procinfo *p) = 0;
    virtual int width() = 0;
    virtual int compare(Procinfo *a, Procinfo *b);

    QString name;
    QString help;
    int index;
    int id;
    bool reversed;       // testing
    bool flag_int_value; // testing: for total sum , cat_memory , cat_int
};

// COMMON
class Cat_int : public Category
{
  public:
    Cat_int(const QString &heading, const QString &explain, int w,
            int Procinfo::*member);
    int alignment() override { return Qt::AlignRight; };
    QString string(Procinfo *p) override;
    int width() override { return field_width; };
    int compare(Procinfo *a, Procinfo *b) override;

  protected:
    int Procinfo::*int_member;
    int field_width;
};

// COMMON  for memory usage
class Cat_memory : public Category
{
  public:
    Cat_memory(const QString &heading, const QString &explain, int w,
               unsigned long Procinfo::*member);
    int alignment() override { return Qt::AlignRight; };
    QString string(Procinfo *p) override;
    int width() override { return field_width; };
    int compare(Procinfo *a, Procinfo *b) override;

  protected:
    unsigned long Procinfo::*uintl_member;
    int field_width;
};

class Cat_uintl : public Category
{
  public:
    Cat_uintl(const QString &heading, const QString &explain, int w,
              unsigned long Procinfo::*member);
    int alignment() override { return Qt::AlignRight; };
    QString string(Procinfo *p) override;
    int width() override { return field_width; };
    int compare(Procinfo *a, Procinfo *b) override;

  protected:
    unsigned long Procinfo::*uintl_member;
    int field_width;
};

class Cat_hex : public Cat_uintl
{
  public:
    Cat_hex(const QString &heading, const QString &explain, int w,
            unsigned long Procinfo::*member);
    QString string(Procinfo *p) override;
};

class Cat_swap : public Category
{
  public:
    Cat_swap(const QString &heading, const QString &explain);
    int alignment() override { return Qt::AlignRight; };
    QString string(Procinfo *p) override;
    int width() override { return 8; };
    int compare(Procinfo *a, Procinfo *b) override;
};

class Cat_string : public Category
{
  public:
    Cat_string(const QString &heading, const QString &explain,
               QString Procinfo::*member = nullptr);
    int alignment() override { return Qt::AlignLeft; };
    QString string(Procinfo *p) override;
    int width() override { return -9; };
    virtual int gap() { return 8; };

  protected:
    QString Procinfo::*str_member;
};

class Cat_user : public Cat_string
{
  public:
    Cat_user(const QString &heading, const QString &explain);
    QString string(Procinfo *p) override;
};

class Cat_group : public Cat_string
{
  public:
    Cat_group(const QString &heading, const QString &explain);
    QString string(Procinfo *p) override;
};

class Cat_wchan : public Cat_string
{
  public:
    Cat_wchan(const QString &heading, const QString &explain);
    QString string(Procinfo *p) override;
};

class Cat_dir : public Cat_string
{
  public:
    Cat_dir(const QString &heading, const QString &explain, const char *dirname,
            QString Procinfo::*member);
    QString string(Procinfo *p) override;

  protected:
    const char *dir;
    QString Procinfo::*cache;
};

class Cat_cmdline : public Cat_string
{
  public:
    Cat_cmdline(const QString &heading, const QString &explain);
    QString string(Procinfo *p) override;
};

class Cat_state : public Category
{
  public:
    Cat_state(const QString &heading, const QString &explain);
    int alignment() override { return Qt::AlignLeft; };
    QString string(Procinfo *p) override;
    int width() override { return 6; };
    virtual int gap() { return 8; };
};

class Cat_policy : public Category
{
  public:
    Cat_policy(const QString &heading, const QString &explain);
    int alignment() override { return Qt::AlignLeft; };
    QString string(Procinfo *p) override;
    int width() override { return 3; };
    virtual int gap() { return 8; };
    int compare(Procinfo *a, Procinfo *b) override;
};

class Cat_rtprio : public Category
{
  public:
    Cat_rtprio(const QString &heading, const QString &explain);
    int alignment() override { return Qt::AlignRight; };
    QString string(Procinfo *p) override;
    int width() override { return 5; };
    int compare(Procinfo *a, Procinfo *b) override;
};

#ifdef LINUX
class Cat_tms : public Category
{
  public:
    Cat_tms(const QString &heading, const QString &explain);
    int alignment() override { return Qt::AlignRight; };
    QString string(Procinfo *p) override;
    int width() override { return 5; };
    int compare(Procinfo *a, Procinfo *b) override;
};

class Cat_affcpu : public Category
{
  public:
    Cat_affcpu(const QString &heading, const QString &explain);
    int alignment() override { return Qt::AlignRight; };
    QString string(Procinfo *p) override;
    int width() override { return 8; };
    // virtual int compare(Procinfo *a, Procinfo *b);
};
#endif

class Cat_time : public Category
{
  public:
    Cat_time(const QString &heading, const QString &explain);
    int alignment() override { return Qt::AlignRight; };
    QString string(Procinfo *p) override;
    int width() override { return 7; };
    int compare(Procinfo *a, Procinfo *b) override;
};

class Cat_start : public Category
{
  public:
    Cat_start(const QString &heading, const QString &explain);
    int alignment() override { return Qt::AlignRight; };
    QString string(Procinfo *p) override;
    int width() override { return 8; };
    int compare(Procinfo *a, Procinfo *b) override;
};

class Cat_percent : public Category
{
  public:
    Cat_percent(const QString &heading, const QString &explain, int w,
                float Procinfo::*member);
    int alignment() override { return Qt::AlignRight; };
    QString string(Procinfo *p) override;
    int width() override { return field_width; };
    int compare(Procinfo *a, Procinfo *b) override;

  protected:
    float Procinfo::*float_member;
    int field_width;
};

class Cat_tty : public Cat_string
{
  public:
    Cat_tty(const QString &heading, const QString &explain);
    QString string(Procinfo *p) override;
};

#define CPU_TIMES(cpu, kind) cpu_times_vec[cpu * CPUTIMES + kind]
class Proc;

class Procinfo // Process Infomation
{
  public:
    Procinfo(Proc *system_proc, int pid, int thread_id = -1);
#ifdef SOLARIS
    //	Procinfo(int pid);  //solaris !
    //	Procinfo(int pid, int thread);  //solaris !
    int readproc(int pid, int lwp);
#endif
    ~Procinfo();
    Procinfo *ref()
    {
        refcnt++;
        return this;
    };
    void deref()
    {
        if (!--refcnt)
            delete this;
    };
    Proc *proc;

    inline void calculate_cpu();

    int readproc();

    bool isThread();
    void read_fd(int fdnum, char *path);
    bool read_fds();
    bool read_maps();
    bool read_environ();
#ifdef SOLARIS
    void read_pmap_maps();
#endif
#ifdef MOSIX
    static void check_for_mosix();
    static Svec<int> mosix_nodes();
#endif

    int get_policy();
    int get_rtprio();

#ifdef LINUX
    double get_tms();
    unsigned long get_affcpu();

    QVector<SockInode *> sock_inodes; // socket inodes or NULL if not read
#endif
    int pid;
    bool clone;

    bool first_run;        // for optimization
    char hashstr[128 * 8]; // cache
    size_t hashlen;
    int hashcmp(char *str);

    QString command;   // COMMAND
    QString cmdline;   // COMMAND_LINE
    QString username;  //
    QString groupname; //
    QString cwd;       // null if not read
    QString root;      // null if not read

    bool accepted;
    int test_stop; // for test
    int session;   //	???

    int uid, euid;
    int gid, egid;

    char state;
    int ppid; // Parent's PID
    int pgrp;
    dev_t tty; // tty major:minor device
    int type;  // TESTING X,NETWORK,FILE_OPEN,TERMINAL(tty),THREAD,

    int nthreads; // number of threads : LWP(Solaris), task(Linux)
    int tgid;     // thread leader's id

#ifdef LINUX
    double tms; // slice time
    int slpavg;
    unsigned long affcpu;

    int suid, fsuid;
    int sgid, fsgid;
    int tpgid;

    unsigned long cminflt;
    unsigned long cmajflt;
#endif

    unsigned long io_read;       // KB
    unsigned long io_write;      // KB
    unsigned long io_read_KBps;  // K byte/sec
    unsigned long io_write_KBps; // K byte/sec
    unsigned long io_read_prev, io_write_prev; // KB

    unsigned long flags; //?
    unsigned long minflt;
    unsigned long majflt;

    long utime;
    long old_utime; // initial value = -1 ;
    long cutime;
    int priority;
    int nice;
    unsigned long starttime; // start time since run in epoch? Linux : jiffies
                             // since boot , solaris
    unsigned long wchan;
    QString wchan_str;

    // Memory
    unsigned long mem;      // user Memory define
    unsigned long size;     // SIZE: total memory (K)
    unsigned long resident; // RSS: pages in resident set (non-swapped) (K)
#ifdef LINUX
    unsigned long share; // shared memory pages (mmaped) (K)
    unsigned long trs;   // text resident set size (K)
    unsigned long lrs;   // shared-lib resident set size (K)
    unsigned long drs;   // data resident set size (K)
    unsigned long
        dt; // dirty pages (number of pages, not K), obsolute in Kernel 2.6
    unsigned long stack; // stack size (K)
    unsigned long swap;  // swap size (K)
#endif

#ifdef SOLARIS
    int addr_bits; // address bits (32 or 64)

    char policy_name[2]; // two first letters of scheduling class
#endif
    struct timeval tv;     // time when the snapshot was taken
    struct timeval old_tv; //

    // Posix.1b scheduling
    int policy; // -1 = uninitialized
    int rtprio; // 0-99, higher can pre-empt lower (-1 = uninitialized)

    // Linux: the cpu used most of the time of the process
    // Solaris: the cpu on which the process last ran
    int which_cpu;

    // computed %cpu and %mem since last update
    float wcpu, old_wcpu; // %WCPUwheight cpu
    float pcpu;           // %CPU: percent cpu after last update
    float pmem;           // %MEM

    QVector<Fileinfo *> fd_files; // file names list
    QVector<Mapsinfo *> maps;     // maps list
    QVector<NameValue> environ;   // environment
    char *envblock;               // malloc()ed environment data block

#ifdef SOLARIS
    unsigned long env_ofs;
#endif

    Details *detail; // details window or NULL (backlink)

    unsigned int generation; // timestamp

    bool selected : 1;  // true if selected in current view
    bool hidekids : 1;  // true if children are hidden in tree view
    bool lastchild : 1; // true if last (visible) child in tree view

    short level;                  // distance from process root
    QVector<Procinfo *> children; // real child processes
    static const int MAX_CMD_LEN = 512;

    char refcnt;

    // virtual child for Table_Tree
    QVector<Procinfo *> table_children;
    int table_child_seq;
    int clear_gen;
    int child_seq_prev;
    int parent_row; // virtual parent for tree table

#ifdef MOSIX
    bool isremote;
    int from;
    int where;
    int remotepid;
    QString migr; // String explaining migration "node>" or ">node"
    int nmigs;
    int locked;
    QString cantmove;
    static bool mosix_running; // true if MOSIX is running
#endif
};

typedef QHash<int, Procinfo *> Proclist;

class SysHistory
{
  public:
    int idx;
    time_t time; // saved time, epoch...
    int current_gen;
    float load_cpu; // %CPU total ; green
    float load_mem; // %mem 	; yellow?
    float load_io;  // %SYS_IO 	; BLUE
    float load_net; //	;blue?	orange?
    Proclist procs;
    ~SysHistory();
};


// for A System
// cf. Procinfo is for a Process
//
class Proc
{
Q_DECLARE_TR_FUNCTIONS(Proc)
  public:
    Proc();
    ~Proc();
    void commonPostInit(); // COMMON
    void read_proc_all();  // test
    void refresh();

    static void init_static();
    int read_system();
    int countCpu();
    void read_loadavg();

    int read_pid_tasks(int pid);

    Category *cat_by_name( const QString &s );
    int field_id_by_name( const QString &s );

#ifdef LINUX
    /* 	from /proc/net/{tcp,udp,unix}  */
    QHash<int, Sockinfo *> socks;    // tcp/udp sockets
    QHash<int, UnixSocket *> usocks; // unix domain sockets
    bool socks_current;              // true if the socks list is current
    bool usocks_current;             // true if the usocks list is current

    bool read_socket_list(Sockinfo::proto_t proto, const char *pseudofile);
    void read_sockets();
    bool read_usocket_list();
    void read_usockets();
    void invalidate_sockets();

#endif

#ifdef SOLARIS
    static kstat_ctl_t *kc; // NULL if kstat not opened
#endif
    QHash<int, Category *> categories;

    Proclist procs; // processes indexed by pid

    // TESTING
    QString supasswd; // test
    int syshistoryMAX;
    Proclist *hprocs; // temp_hprocs list
    Proclist *mprocs; //
    QList<SysHistory *> history;

    void setHistory(int tick);
    Proclist getHistory(int pos);

    int qps_pid;   // test
    float loadQps; // TEST
    static int update_msec;

    // class
    int num_cpus;     // current number of CPUs
    int old_num_cpus; // previous number of CPUs

    long num_network_process; //  number of network(socket) process
    long num_opened_files;    //  number of opened normal(not socket) files
    int num_process;          //  number of process

    long dt_total; //
    long dt_used;  //  cpu used time in clktick

    long read_byte;  //  test
    long write_byte; // test
    long io_byte;    // test file_io

    float load_cpu;   // %CPU total
    float loadavg[3]; // 1,5,15 minutes load avgs

    unsigned int clk_tick;  // the  number  of  clock ticks per second.
    unsigned int boot_time; // boot time in seconds since the Epoch

    int mem_total, mem_free;   // (Kb)
    int swap_total, swap_free; // in kB

    int mem_shared, mem_buffers, mem_cached; // Linux

    // the following are pointers to matrices indexed by kind (above) and
    // cpu
    unsigned *cpu_times_vec;
    unsigned *old_cpu_times_vec;

    // accessors for (old_)cpu_times_vec
    unsigned &cpu_times(int cpu, int kind)
    {
        return cpu_times_vec[cpu * CPUTIMES + kind];
    }
    unsigned &old_cpu_times(int cpu, int kind)
    {
        return old_cpu_times_vec[cpu * CPUTIMES + kind];
    }

#ifdef LINUX
    // from /proc/stat
    unsigned long *per_cpu_times;     // vector of num_cpus times
    unsigned long *old_per_cpu_times; // vector of num_cpus times
#endif

    // Solaris <sys/sysinfo.h> #defines CPU_xxx so we must avoid them
    enum
    {
        CPUTIME_USER,
#ifdef LINUX
        CPUTIME_NICE,
#endif
        CPUTIME_SYSTEM,
#ifdef SOLARIS
        CPUTIME_WAIT,
#endif
        CPUTIME_IDLE,
        CPUTIMES
    };

    //#define CPU_TIMES(cpu, kind) cpu_times_vec[cpu * CPUTIMES + kind]

    unsigned int current_gen;
    int maxSizeHistory;
};

class Procview : public Proc
{
  public:
    Procview();
    QString filterstr;

    static bool flag_show_file_path;
    static bool flag_pcpu_single;
    static bool flag_cumulative;

    static int compare(Procinfo *const *a, Procinfo *const *b);
    static int compare_backwards(Procinfo *const *a, Procinfo *const *b);
    void refresh();
    bool accept_proc(Procinfo *p); // COMMON
    void linearize_tree(QVector<Procinfo *> *ps, int level, int prow,
                        bool flag_hide = false);
    void build_tree(Proclist &);
    void rebuild();

    void set_fields();
    void set_fields_list(int fields[]);
    void addField(char *name);                   // interface
    int addField(int FIELD_ID, int where = -1); // base interface
    void removeField(int FIELD_ID);
    int findCol(int FIELD_ID);
    void moveColumn(int col, int place);
    void fieldArrange();
    void update_customfield();
    //
    void setSortColumn(int col, bool keepSortOrder = false);
    void setTreeMode(bool b);
    void saveCOMMANDFIELD();

    Procinfo *getProcinfoByPID(int pid) { return procs.value(pid, NULL); };

    QVector<Procinfo *> linear_procs; // this is linear_proc_list for viewer

#ifdef LINUX
    QVector<Sockinfo *> linear_socks; // Linux Testing
#endif

    // QList<> tags_kernel;

    // root_procs contains processes without parent; normally only init, but
    // we cannot rely on this (Solaris has several parentless processes).
    // Also, if the view is restricted, all processes whose parent isn't in
    // the table.
    QVector<Procinfo *> root_procs; // table_root_procs; for viewer
    QVector<Category *> cats;       // for table

    Category *sortcat;
    Category *sortcat_linear; // testing
    int sort_column;          // index of
    bool reversed;            // true if sorted backwards
    static bool treeview;     // true if viewed in tree form
    bool enable;              // tmp

    enum procstates
    {
        ALL,
        OWNED,
        NROOT,
        RUNNING,
        HIDDEN,
        NETWORK
    };
    enum fieldstates
    {
        USER = HIDDEN + 1,
        JOBS,
        MEM,
        SCHED,
        CUSTOM
    };
    int viewproc;
    int viewfields;

    int idxF_CMD; ////Test
    QStringList customfields;
    QList<QVariant> customFieldIDs;
    static int custom_fields[64];
    // lists of fields to be used for different views, terminated by -1:
    static int basic_fields[];
    static int jobs_fields[];
    static int mem_fields[];
#ifdef LINUX
    static int sched_fields[];
#endif

#ifdef MOSIX
    static int user_fields_mosix[];
    static int jobs_fields_mosix[];
    static int mem_fields_mosix[];
#endif
    static float avg_factor; // exponential factor for averaging
    static const int cpu_avg_time = 30 * 1000; // averaging time for WCPU (ms)

  private:
    static Category *static_sortcat; // kludge: to be used by compare
};

#endif // PROC_H
