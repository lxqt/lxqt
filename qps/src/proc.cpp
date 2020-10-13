/*
 * proc.cpp for Linux
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 1997-1999 Mattias Engdegård
 * Copyright           Olivier Daudel
 *
 * Copyright 2015 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * Copyright 2012-2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

/*
        LWP (Light Weight Process): just thread, mainly used in Solaris
        Task : thread and process in Linux
        NPTL(Native POSIX Thread Library)
        TGID thread group leader's pid
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath> // log()
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctime>

#include <grp.h>
#include <pwd.h>
#include <sched.h>  // sched_rr_get_interval(pid, &ts);
#include <libgen.h> // basename()
#include <unistd.h> // sysconf()  POSIX.1-2001

#include <sys/time.h>
#include <sys/utsname.h> // uname()
//#include <sys/param.h>	//HZ defined, no more used.
//#include "misc.h"			// x_atoi() , userName() ,groupname()

#include "proc.h"
#include "uidstr.h"
#include "ttystr.h"
#include "wchan.h"

#include <QString>

#include "details.h"

// for Non-ASCII locale
//#include <global.h>

//#include <qtextcodec.h>
// extern QTextCodec * codec ;
//#define UniString(str)   codec->toUnicode(str)

#define PROCDIR "/proc" // hmmm

extern int flag_thread_ok;
extern bool flag_schedstat;
extern bool flag_show_thread;
extern bool flag_devel;

int flag_24_ok; // we presume a kernel 2.4.x

// COMMON
bool Procview::treeview = false; // true
bool Procview::flag_show_file_path = false;
bool Procview::flag_cumulative = false;  // times cumulative with children's
bool Procview::flag_pcpu_single = false; // %CPU= pcpu/num_cpus
int pagesize;
int Proc::update_msec = 1024;

// socket states, from <linux/net.h> and touched to avoid name collisions
enum
{
    SSFREE = 0,     /* not allocated		*/
    SSUNCONNECTED,  /* unconnected to any socket	*/
    SSCONNECTING,   /* in process of connecting	*/
    SSCONNECTED,    /* connected to socket		*/
    SSDISCONNECTING /* in process of disconnecting	*/
};

#define QPS_SCHED_AFFINITY ok

#ifdef QPS_SCHED_AFFINITY
#ifndef SYS_sched_setaffinity
#define SYS_sched_setaffinity 241
#endif
#ifndef SYS_sched_getaffinity
#define SYS_sched_getaffinity 242
#endif

// Needed for some glibc
int qps_sched_setaffinity(pid_t pid, unsigned int len, unsigned long *mask)
{
    return syscall(SYS_sched_setaffinity, pid, len, mask);
};
int qps_sched_getaffinity(pid_t pid, unsigned int len, unsigned long *mask)
{
    return syscall(SYS_sched_getaffinity, pid, len, mask);
};
#endif

/*
   Thread Problems.
   pthread_exit()
   */

struct proc_info_
{
    int proc_id;
    char flag;
    char type;
    int files;
} proc_info; // TESTING

struct list_files_
{
    int proc_id;
    int flag;
    char *filename; //  path + filename
} list_files;       // TESTING

// read() the number of bytes read is returned (zero indicates end  of  file)
// return the number of bytes read if ok, -1 if failed
inline int read_file(char *name, char *buf, int max)
{
    int fd = open(name, O_RDONLY);
    if (fd < 0)
        return -1;
    int r = read(fd, buf, max);
    close(fd);
    //	buf[r]=0;
    return r;
}

// Description : read proc files
// return 0 : if error occurs.
char buffer_proc[1024 * 4]; // enough..maybe
char *read_proc_file(const char *fname, int pid = -1, int tgid = -1,
                     int *size = nullptr)
{
    static int max_size = 0;
    char path[256];
    int r;

    if (pid < 0)
        sprintf(path, "/proc/%s", fname);
    else
    {
        if (tgid > 0)
            sprintf(path, "/proc/%d/task/%d/%s", tgid, pid, fname);
        else
            sprintf(path, "/proc/%d/%s", pid, fname);
    }

    if (strcmp(fname, "exe") == 0)
    {
        if ((r = readlink(path, buffer_proc, sizeof(buffer_proc) - 1)) >= 0)
        {
            buffer_proc[r] = 0; // safer
            return buffer_proc;
        }
        else
            return nullptr;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return nullptr;
    r = read(fd, buffer_proc, sizeof(buffer_proc) - 1); // return 0 , -1 ,
    if (r < 0)
        return nullptr;

    if (max_size < r)
        max_size = r;

    if (size != nullptr)
        *size = r;

    buffer_proc[r] = 0; // safer

    return buffer_proc;
    // note: not work  fgets(sbuf, sizeof(64), fp) why???
}

char *read_proc_file2(char *r_path, const char *fname, int *size = nullptr)
{
    static int max_size = 0;
    char path[256];
    int r;

    // strcpy(path,r_path);

    sprintf(path, "%s/%s", r_path, fname);

    if (strcmp(fname, "exe") == 0)
    {
        if ((r = readlink(path, buffer_proc, sizeof(buffer_proc) - 1)) >= 0)
        {
            buffer_proc[r] = 0; // safer
            return buffer_proc;
        }
        else
            return nullptr;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return nullptr;
    r = read(fd, buffer_proc,
             sizeof(buffer_proc) - 1); // return 0 , -1 , read_count
    if (r < 0)
        return nullptr;

    if (max_size < r)
        max_size = r;

    if (size != nullptr)
        *size = r;

    buffer_proc[r] = 0; // safer
    close(fd);

    return buffer_proc;
    // note: not work  fgets(sbuf, sizeof(64), fp) why???
}

// TEST CODE ,  Bottleneck
// Description: read /proc/PID/fd/*  check opened file, count opened files
//		this fuction will be called  when every update.
// Return Value :
bool proc_pid_fd(const int pid)
{
    char path[256];
    char fname[256];
    DIR *d;
//    int fdnum;
    int len, path_len;

    sprintf(path, "/proc/%d/fd", pid);

    path_len = strlen(path);
    d = opendir(path);

    if (!d)
    {
        // this happend when the process died already or Zombie process
        // printf("Qps : read fail !! /proc/%d/fd !!! kernel bug ?
        // \n",pid);
        return false;
    }

    struct dirent *e;
    while ((e = readdir(d)) != nullptr)
    {
        if (e->d_name[0] == '.')
            continue; // skip "." and ".."

        if (strlen(path) + 1 + strlen(e->d_name) >= 256) // overflow
        {
            closedir(d);
            return false;
        }
        path[path_len] = '/';
        path[path_len + 1] = 0;
        strcat(path, e->d_name);

        len = readlink(path, fname, sizeof(fname) - 1);
        if (len > 0)
        {
            fname[len] = 0;
            // printf("DEBUG: %s[%s]\n",path,fname);
            // if (strcmp(fname,"/dev/null")==0 ) continue;
        }

        /// num_opened_files++;
        // strcpy(p, e->d_name);
        // fdnum = atoi(p);
        // read_fd(fdnum, path);
    }
    closedir(d);
    return true;
}


// COMMON?
// return username from /etc/passwd
const QString userName(int uid, int euid)
{
    char buff[128];
    struct passwd *pw = getpwuid(uid);
    if (!pw)
    {
        // dont have name !
        sprintf(buff, "%d", uid);
    }
    else
        strcpy(buff, pw->pw_name);

    if (uid != euid)
        strcat(buff, euid == 0 ? "*" : "+");
    return QString(buff);
}

// return group name (possibly numeric)
const QString groupName(int gid, int /*egid*/)
{
    QString res;
    struct group *gr = getgrgid(gid);
    if (!gr)
        res = QString::number(gid);
    else
        res = QString::fromLocal8Bit(gr->gr_name);
    return res;
}

Category::~Category() {}

int Category::compare(Procinfo *a, Procinfo *b)
{
    return string(a).compare(string(b));
}

Cat_int::Cat_int(const QString &heading, const QString &explain, int w,
                 int Procinfo::*member)
    : Category(heading, explain), int_member(member), field_width(w)
{
}

QString Cat_int::string(Procinfo *p)
{
    QString s;
    s.setNum(p->*int_member);
    return s;
}

int Cat_int::compare(Procinfo *a, Procinfo *b)
{
    // qsort() only cares about the sign of the number returned by the
    // comparison function; only a subtraction is necessary
    return a->*int_member - b->*int_member;
}

// COMMON
Cat_percent::Cat_percent(const QString &heading, const QString &explain, int w,
                         float Procinfo::*member)
    : Category(heading, explain), float_member(member), field_width(w)
{
}

QString Cat_percent::string(Procinfo *p)
{
    return QString::asprintf("%01.2f", (double)(p->*float_member));
}

int Cat_percent::compare(Procinfo *a, Procinfo *b)
{
    float at = a->*float_member, bt = b->*float_member;
    return at < bt ? 1 : (at > bt ? -1 : 0);
}
// added 2006/05/07
Cat_memory::Cat_memory(const QString &heading, const QString &explain, int w,
                       unsigned long Procinfo::*member)
    : Category(heading, explain), uintl_member(member), field_width(w)
{
}

QString Cat_memory::string(Procinfo *p)
{
    QString s;
    long sizeK = p->*uintl_member;
    double sizeM = static_cast<double>(sizeK) / 1024;
    if (sizeM > 1.0)
        s = QString::number(sizeM, 'f', 1) + QStringLiteral("M");
    else
        s = QString::number(sizeK) + QStringLiteral("K");

    if (sizeK == 0)
        s = QStringLiteral("0");
    return s;
}

int Cat_memory::compare(Procinfo *a, Procinfo *b)
{
    int bu = b->*uintl_member, au = a->*uintl_member;
    return bu >= au ? (bu == au ? 0 : 1) : -1;
}

Cat_uintl::Cat_uintl(const QString &heading, const QString &explain, int w,
                     unsigned long Procinfo::*member)
    : Category(heading, explain), uintl_member(member), field_width(w)
{
}

QString Cat_uintl::string(Procinfo *p)
{
    QString s;
    s.setNum(p->*uintl_member);
    return s;
}

int Cat_uintl::compare(Procinfo *a, Procinfo *b)
{
    int bu = b->*uintl_member, au = a->*uintl_member;
    return bu >= au ? (bu == au ? 0 : 1) : -1;
}

Cat_hex::Cat_hex(const QString &heading, const QString &explain, int w,
                 unsigned long Procinfo::*member)
    : Cat_uintl(heading, explain, w, member)
{
}

// QString.sprintf 2x speed than glibc.sprintf (by fasthyun@magicn.com)
// but QString structuring & destructring eat more time
QString Cat_hex::string(Procinfo *p)
{
    return QString::asprintf("%8x", (unsigned)(p->*uintl_member));
}

// COMMON,
Cat_swap::Cat_swap(const QString &heading, const QString &explain)
    : Category(heading, explain)
{
}

QString Cat_swap::string(Procinfo *p)
{
    QString s;
    // It can actually happen that size < resident (Sun under Solaris 2.6)
    // Possible with Linux ?

    long sizeK, sizeM;
#ifdef LINUX
    sizeK = p->swap;
#else
    sizeK = p->size > p->resident ? p->size - p->resident : 0;
#endif
    sizeM = sizeK / 1024;

    if (sizeM > 0)
        s = QString::number(sizeM) + "M";
    else
        s = QString::number(sizeK) + "K";

    if (sizeK == 0)
        s = "0";
    return s;
}

int Cat_swap::compare(Procinfo *a, Procinfo *b)
{
#ifdef LINUX
    return b->swap - a->swap;
#else
    return (b->size - b->resident) - (a->size - a->resident);
#endif
}

Cat_string::Cat_string(const QString &heading, const QString &explain,
                       QString Procinfo::*member)
    : Category(heading, explain), str_member(member)
{
}

QString Cat_string::string(Procinfo *p) { return p->*str_member; }

Cat_user::Cat_user(const QString &heading, const QString &explain)
    : Cat_string(heading, explain)
{
}

QString Cat_user::string(Procinfo *p)
{
    if (p->uid == p->euid)
        return Uidstr::userName(p->uid);
    else
    {
        QString s = Uidstr::userName(p->uid);
        s.append(p->euid == 0 ? "*" : "+");
        return s;
    }
}

Cat_group::Cat_group(const QString &heading, const QString &explain)
    : Cat_string(heading, explain)
{
}

QString Cat_group::string(Procinfo *p)
{
    if (p->gid == p->egid)
        return Uidstr::groupName(p->gid);
    else
    {
        QString s = Uidstr::groupName(p->gid);
        s.append("*");
        return s;
    }
}

Cat_wchan::Cat_wchan(const QString &heading, const QString &explain)
    : Cat_string(heading, explain)
{
}

QString Cat_wchan::string(Procinfo *p)
{
#ifdef LINUX
    return p->wchan_str;
#else
    return Wchan::name(p->wchan);
#endif
}

Cat_cmdline::Cat_cmdline(const QString &heading, const QString &explain)
    : Cat_string(heading, explain)
{
}

QString Cat_cmdline::string(Procinfo *p)
{
    if (p->cmdline.isEmpty())
    {
        QString s("(");
        s.append(p->command);
        s.append(")");
        return s;
    }
    else
    {
        if (Procview::flag_show_file_path)
            return p->cmdline;
        else
        {
            QString s(p->cmdline);

            int i = s.indexOf(' ');
            if (i < 0)
                i = s.length();
            if (i > 0)
            {
                i = s.lastIndexOf('/', i - 1);
                if (i >= 0)
                    s.remove(0, i + 1);
            }
            return s;
        }
    }
}

// hmm COMMON? almost same...but Solaris's zombie process...
Cat_start::Cat_start(const QString &heading, const QString &explain)
    : Category(heading, explain)
{
}
// don't let your confidence be shaken, just learn what you can from this young
// master.
QString Cat_start::string(Procinfo *p)
{
#ifdef SOLARIS
    if (p->state == 'Z')
        return "-"; // Solaris zombies have no valid start time
#endif
    Proc *proc = p->proc;
    //	time_t start = boot_time + p->starttime / (unsigned)HZ;
    QString s;
    // time_t start = proc->boot_time + p->starttime / proc->clk_tick;
    time_t start = p->starttime;
    char *ct = ctime(&start);                    // ctime(sec)
    time_t sec = p->starttime - proc->boot_time; // secs
    // if(p->tv.tv_sec - start < 86400) {
    if (sec < 86400)
    { // 24hours
        ct[16] = '\0';
        s = ct + 11; // Hour:minu
    }
    else
    {
        ct[10] = '\0';
        s = ct + 4; // Date
    }
    return s;
}

int Cat_start::compare(Procinfo *a, Procinfo *b)
{
    unsigned long bs = b->starttime, as = a->starttime;
    return bs >= as ? (bs == as ? 0 : 1) : -1;
}

/* ========================  Procview ============================  */
float Procview::avg_factor = 1.0;

Procview::Procview()
{
    // Proc::Proc(); //call once more ?
    cats.clear();
    reversed = false;
    viewproc = ALL;
    treeview = false;
    idxF_CMD = 0; // **** important ****
    viewfields = USER;
    set_fields();

    sortcat = nullptr; // categories[F_PID];
    sort_column = -1;
    sortcat_linear = nullptr;
    enable = true;
    maxSizeHistory = 1200;
    refresh(); // before reading Watchdog list
}

// COMMON:
// Description :
// 		View mode : ALL , OWNER, NO-ROOOT , HIDDEN, NETWORK
bool Procview::accept_proc(Procinfo *p)
{
    QString pid;
    static int my_uid = getuid();
    bool result;

    result = true;

    // BAD
    if (false and viewproc == NETWORK)
    {
        /*
        p->read_fds();
        if(p->sock_inodes.size()==0)
                result=false;
        for(int i=0;i<p->sock_inodes.size();i++)
        {
                SockInode *sn=p->sock_inodes[i];
                Sockinfo *si=Procinfo::socks.value(sn->inode,NULL);
                if(si)
                {
                        si->pid=p->pid;
                        linear_socks.append(si);
                }
        }
        */
    }
    else if (viewproc == ALL)
        result = true;
    else if (viewproc == OWNED)
        result = (p->uid == my_uid);
    else if (viewproc == NROOT)
        result = (p->uid != 0);
    else if (viewproc == RUNNING)
        result = strchr("ORDW", p->state) != nullptr;

    /*
    if ( viewproc == HIDDEN)
    {
            result=false;
            for(int j=0;j<hidden_process.size();j++)
                    if(hidden_process[j]==p->command)
                            result=true ;
    }
    else
    {
            for(int j=0;j<hidden_process.size();j++)
                    if(hidden_process[j]==p->command)
                            result=false;
    } */

    if (result == false)
        return false; // dont go further !! for better speed

    /// if(search_box==NULL)		return result;
    if (filterstr == "")
        return result;

    if (filterstr == "*")
        return true;

    // Notice:
    //  1. some process name maybe different CMDLINE.  ex) Xorg
    if (p->cmdline.contains(filterstr,
                            Qt::CaseInsensitive)) // search_box->text()
        return true;
    if (p->command.contains(filterstr, Qt::CaseInsensitive))
        return true;
    if (p->username.contains(filterstr, Qt::CaseInsensitive))
        return true;

    pid = pid.setNum(p->pid); //=QString::number(p->pid);
    if (pid.contains(filterstr, Qt::CaseInsensitive))
        return true;

    /// printf("search_Box =%s , %s
    /// \n",search_box->text().toAscii().data(),p->command.toAscii().data());

    return false;
}

extern "C" {
typedef int (*compare_func)(const void *, const void *);
}

/*
template<class T>
void Svec<T>::sort(int (*compare)(const T *a, const T *b))
{
    qsort(vect, used, sizeof(T), (compare_func)compare);
}
*/

// table view - sort
void Procview::linearize_tree(QVector<Procinfo *> *ps, int level, int prow,
                              bool hide)
{
    static_sortcat = sortcat;
    // ps->sort(reversed ? compare_backwards : compare);
    if (reversed)
        qsort(ps->data(), ps->size(), sizeof(Procinfo *),
              (compare_func)compare_backwards);
    else
        qsort(ps->data(), ps->size(), sizeof(Procinfo *),
              (compare_func)compare);

    int size = ps->size();
    // printf("level=%d prow=%d size=%d\n",level,prow,size);
    for (int i = 0; i < size; i++)
    {
        Procinfo *p = (*ps)[i];
        //	if (p->pid<6 )	printf("pid=%d level=%d
        // parent_row=%d\n",p->pid,p->pid,p->parent_row);
        p->level = level;
        p->lastchild = false;
        p->table_child_seq = i; // ************* where using ? -> sequence
        p->parent_row = prow;
        if (!hide)
            linear_procs.append(p);   // need !!
        if (p->table_children.size()) // and !p->hidekids)
            linearize_tree(&p->table_children, level + 1,
                           linear_procs.size() - 1, hide | p->hidekids);
    }

    if (size > 0)
        (*ps)[size - 1]->lastchild = true;
}

/// basic,memory,job fields
void Procview::set_fields_list(int fields[])
{
    cats.clear();
    for (int i = 0; fields[i] != F_END; i++)
    {
        if (fields[i] == F_CPUNUM && Proc::num_cpus < 2)
            continue; // does not work correctly

        Category *c = categories.value(fields[i], nullptr);
        if (c)
            cats.append(c);
    }
}

// return the column number of a field, or -1 if not displayed
int Procview::findCol(int field_id)
{
    for (int i = 0; i < cats.size(); i++)
        if (cats[i]->id == field_id)
            return i;
    return -1;
}

// basis
// returns the place where the field is added
// (-1 if it already exists)
// called by
// 	void Procview::fieldArrange();
// 	qps();
//
int Procview::addField(int Fid, int where)
{
    if (where < 0 || where > cats.size())
    {
        // this is default
        where = cats.size();
    }

    if (Fid == F_CMDLINE)
        where = cats.size(); // always should be the last column
    else if (Fid == F_PID)
    {
        if (treeview == true)
            where = 0; // always should be the first column with the tree
        else if (!cats.isEmpty() && where == cats.size()
                 && cats[cats.size() - 1]->index == F_CMDLINE)
        {
            -- where;
        }
    }
    else if (!cats.isEmpty())
    {
        if (where == cats.size())
        {
            if (cats[cats.size() - 1]->index == F_CMDLINE)
              -- where;
        }
        else if (where == 0 && treeview == true && cats[0]->index == F_PID)
            where = 1;
    }

    Category *newcat = categories[Fid];
    if (cats.indexOf(newcat) < 0) // if not in the list
    {
        cats.insert(where, newcat);
        return where;
    }
    return -1; // nothings is added
}

// add a category to last by name
void Procview::addField(char *name)
{
    int id = field_id_by_name(name);
    if (id >= 0)
        addField(id); // add to last
}

void Procview::removeField(int field_id)
{
    for (int i = 0; i < cats.size();)
    {
        if (cats[i]->id == field_id)
        {
            cats.remove(i);
            break;
        }
        else
            i++;
    }
}

//	called by
//		1. write_settings()
//	DEL? -> not yet
void Procview::update_customfield()
{
    customfields.clear();

    for (int i = 0; i < cats.size(); i++)
    {
        customfields.append(cats[i]->name);
        customFieldIDs.append(cats[i]->id);
    }
}

// Description : FIELD movement by mouse drag
//				  From col To place
void Procview::moveColumn(int col, int place)
{
    int s = cats.size();
    if (col < 0 || place < 0 || s <= col || s <= place)
    {
        printf("QPS code bugs!! : moveColumn() col=%d place=%d "
               "cats.size=%d\n",
               col, place, s);
        return;
    }
    if (col == place)
        return;

    if (cats[col]->index == F_PID)
    { // PID should always be the first field in the tree model
        if (treeview == true)
            place = 0;
    }
    else if (cats[col]->index == F_CMDLINE)
        place = s; // COMMAND_LINE should always be the last field

    // first insert it into the new place; then remove it from the old one
    Category *cat = cats[col];
    if (place < col)
        ++col;
    else// if (place > col)
        place = qMin(place + 1, s);
    cats.insert(place, cat);
    cats.remove(col);
}

// always called when linear to tree
// DEL
void Procview::saveCOMMANDFIELD() {}

// call by
//		void Pstable::moveCol(int col, int place)
// 		void Pstable::setTreeMode(bool treemode)
//		void Procview::set_fields()
//	TODO: checkField();
void Procview::fieldArrange()
{
    int s = cats.size();

    // PID should always be the first field in the tree model
    if (treeview == true && cats[0]->index != F_PID)
    {
        bool found = false;
        for (int i = 1; i < s; i++)
        {
            if (cats[i]->index == F_PID)
            {
                found = true;
                moveColumn(i, 0);
                break;
            }
        }
        if (!found)
            addField(F_PID, 0);
    }

    // COMMAND_LINE should always be the last field
    if (cats[s - 1]->index != F_CMDLINE)
    {
        for (int i = 0; i < s - 1; i++)
        {
            if (cats[i]->index == F_CMDLINE)
            {
                moveColumn(i, 0); // will go to the end; see Procview::moveColumn
                break;
            }
        }
    }
}

void Procview::setTreeMode(bool /*b*/)
{
    if (treeview == false)
    {
        if (sortcat_linear == nullptr)
            sortcat_linear = sortcat;
        else
            sortcat = sortcat_linear;
    }
}

void Procview::setSortColumn(int col, bool keepSortOrder)
{
    if (col < 0 || col >= cats.size()) // restore defaults
    {
        if (col >= 0)
            qDebug("hmmm error? col>=cats.size() %d", col);
        sort_column = -1;
        sortcat = nullptr;
        reversed = false;
        return;
    }

    // if(!procview->treeview) ?
    if (!keepSortOrder)
    {
        if (col == sort_column)
            reversed = !reversed;
        else
            reversed = false;
    }
    sortcat = cats[col];
    sort_column = col;
}

Category *Procview::static_sortcat = nullptr;
int Procview::compare(Procinfo *const *a, Procinfo *const *b)
{
    if (static_sortcat == nullptr)
        return 0;
    int r = static_sortcat->compare(*a, *b);
    return (r == 0) ? ((*a)->pid > (*b)->pid ? 1 : -1) : r;
}

int Procview::compare_backwards(Procinfo *const *a, Procinfo *const *b)
{
    if (static_sortcat == nullptr)
        return 0;
    int r = static_sortcat->compare(*b, *a);
    return (r == 0) ? ((*b)->pid > (*a)->pid ? 1 : -1) : r;
}

// COMMON
Category *Proc::cat_by_name( const QString &s )
{

    if ( ! s.isNull() )
    {
        // java style
        QHashIterator<int, Category *> i(categories);
        while (i.hasNext())
        {
            i.next();
            const QString &p = i.value()->name;
            int index = p.indexOf( QRegExp( "\\S" ) );
            if ( p.indexOf( s, index ) == index )
                return i.value();
            //     cout << i.key() << ": " << i.value() << endl;
        }
    }
    return nullptr;
}

// COMMON
// call by
int Proc::field_id_by_name(const QString &s)
{
    if ( ! s.isNull() )
    {
        // STL style
        QHash<int, Category *>::iterator i = categories.begin();
        while (i != categories.end())
        {
            if ( i.value()->name == s )
                return i.key(); // cout << i.key() << ": " <<
                                // i.value() << endl;
            ++i;
        }
    }
    return -1;
}

// postInit
void Proc::commonPostInit()
{
    QHash<int, Category *>::iterator i = categories.begin();
    while (i != categories.end())
    {
        i.value()->index = i.key();
        i.value()->id = i.key();
        ++i;
    }

    num_opened_files = 0;    // test
    num_process = 0;         //	64bit
    num_network_process = 0; // 64bit

    dt_total = 0; // diff system tick
    dt_used = 0;  // for infobar

    loadavg[0] = loadavg[1] = loadavg[2] =
        0.0; // CPU load avg 1min,5min,15minute

    Proc::num_cpus = 0;
    Proc::old_num_cpus = 0;

    Proc::mem_total = 0;
    Proc::mem_free = 0;

    Proc::mem_buffers = 0;
    Proc::mem_cached = 0;

    Proc::swap_total = 0;
    Proc::swap_free = 0;

    Proc::qps_pid = -1;
    Proc::loadQps = 0.0;

    Proc::cpu_times_vec = nullptr; // array.
    Proc::old_cpu_times_vec = nullptr;

    Proc::boot_time = 0;
    Proc::clk_tick = 100; // for most system

    current_gen = 0; // !
    mprocs = nullptr;
    hprocs = nullptr;

    clk_tick =
        sysconf(_SC_CLK_TCK); //****** The  number  of  clock ticks per second.
                              //
                              // printf("Qps: hz=%d\n",clk_tick);

    ///	Procinfo::init_static();
}

// COMMON?
// 	called by Procview::rebuild()
// 	using procs, makes root_procs for TABLE_View
void Procview::build_tree(Proclist &procs)
{
    Procinfo *p;
    int proc_n = 0;

    // children clear
    root_procs.clear();

    QHash<int, Procinfo *>::iterator i;
    for (i = procs.begin(); i != procs.end();)
    {
        Procinfo *p = i.value();
        p->table_children.clear();
        if ((p->accepted = accept_proc(p)))
            proc_n++;
        ++i;
    }

    Proc::num_process = proc_n; // count process

    // find parent of a process
    for (i = procs.begin(); i != procs.end(); ++i)
    {
        p = i.value(); // always not NULL

        if (p->accepted)
        {
            Procinfo *parent = nullptr;
            int virtual_parent_pid;

            if (p->isThread())
                virtual_parent_pid = p->tgid; // thread's leader ID.
            else
                virtual_parent_pid = p->ppid;

            if (p->pid < p->ppid)
            {
                // this occurs !! a reporter mailed to me
            }

            parent = procs.value(virtual_parent_pid,
                                 NULL); // if pid not found, then return NULL.

            //	printf("thread_leader=0 (%d)
            //%s\n",p->tgid,p->command.ascii());
            if (treeview and parent and parent->accepted)
            {
                // p->table_child_seq=parent->table_children.size();
                parent->table_children.append(p);
            }
            else
            {
                // 1.init(pid=1) process
                // 2.some process which parent not accepted
                // 3.(null) thread has TGID=0,PPID=0
                // 4.when not tree mode
                root_procs.append(p);
            }
        }
    }
}

// COMMON
// BOTTLENECK No.3   0.5%
// re-sort the process list , ::rebuild() for Table
// called by Pstable::refresh()
void Procview::rebuild()
{
    linear_procs.clear(); // Svec<Procinfo *> procs in Proview
#ifdef LINUX
    linear_socks.clear();
#endif

    // Procinfo *pi = getProcinfoByPID(Procinfo::qps_pid);
    // if(pi) Procinfo::loadQps=pi->pcpu;
    //	printf("rebuild\n");

    /* if(mprocs)
    {
            build_tree(*mprocs);
    }
    else */
    build_tree(Proc::procs);

    linearize_tree(&root_procs, 0, -1);
}

// COMMON: CORE
// Description: update the process list      BottleNeck 1.5%
// 		read /proc/*
// 		called by Procview::refresh(), every UPDATE .
void Proc::refresh()
{
    current_gen++;

    // TEST for Process History
    SysHistory *s = new SysHistory;

    history.append(s);

    if(hprocs)
    {
        qDeleteAll(hprocs->begin(), hprocs->end());
        hprocs->clear();
    }
    hprocs = &(s->procs);

    // init
    /// num_opened_files=0;
    Proc::num_process = 0;
    Proc::num_network_process = 0;

    Proc::read_loadavg();
    Proc::read_system(); // **** should be every refresh !!
    // s->load_cpu=(float)Proc::dt_used/Proc::dt_total;  //after
    // read_system();

    s->load_cpu = load_cpu; // after read_system();
    s->time = time(nullptr);   // save current time in seconds since epoch

    // TODO: clean
    // Procinfo::read_sockets(); // for future, BottleNect 2%
    read_byte = 0;
    write_byte = 0;
    io_byte = 0; //!!!
                 ///	Proc::read_sockets(); // test

    read_proc_all(); // Linux, Solaris...

    // s->load_io=(float)(read_byte + write_byte ); // (50*1024*1024);
    // //dt_total;
    if (io_byte != 0)
    {
        s->load_io = log10(io_byte) * 2; // 9000 -> 3point
        //	printf("DEBUG: rw=[%d,%d] %d, %f\n",read_byte,
        // write_byte,io_byte,s->load_io);
    }
    else
        s->load_io = 0;

    // remove non-existing processes, remove Procinfos of nonexisting
    // processes
    QHash<int, Procinfo *>::iterator i;
    for (i = procs.begin(); i != procs.end();)
    {
        Procinfo *p = i.value();
        if (p->generation != current_gen)
        {
            //			printf("delete %d\n",p->pid);
            i = procs.erase(i);
            delete p;
        }
        else
        {
            //	p->table_children.clear();  // for rebuild()
            //	p->accepted=accept_proc(p);
            ++i;
        }
    }

    //	TESTING
    while (history.size() >= maxSizeHistory)
    {
        // if(history.isEmpty()==false)
        delete history.takeFirst();
    }
}

// COMMON
// read new process info
void Procview::refresh()
{
    /****************************************************************/
    /* Procview.procs has the procinfo
     */
    /****************************************************************/
    if (enable)
    {
        // printf("Procview::refresh()\n");
        Proc::refresh(); // read "/proc/*", then update the process list
    }
}

SysHistory::~SysHistory()
{
    for (const auto *p : qAsConst(procs))
    {
        delete p;
    }
    procs.clear(); // remove all (key,val)
}

/* check
Category *Proc::cat_by_name(const char *s)
{
    if( s )
    {
        for( int i = 0; i < categories.size(); i++ )
            if( strcmp(categories[i]->name, s) == 0 )
                return categories[i];
    }
    return 0;
}


int  Proc::field_id_by_name(const char *s)
{
    if( s )
    {
        for( int i = 0; i < categories.size(); i++ )
            if( strcmp(categories[i]->name, s) == 0 )
                return i;
    }
    return -1;
}
*/

// new process created
Procinfo::Procinfo(Proc *system_proc, int process_id, int thread_id) : refcnt(1)
{
    first_run = true;
    clone = false;

    proc = system_proc;

    if (thread_id < 0) //
    {
        pid = process_id;
        tgid = process_id; // thread group leader's id
    }
    else
    {
        pid = thread_id;
        tgid = process_id; // thread group leader's id
    }

    ppid = 0; // no parent
    selected = false;
    hidekids = false;
    envblock = nullptr; //!!

    table_child_seq = -1;
    child_seq_prev = -1;

    lastchild = false;
    generation = -1;
    detail = nullptr;

    /// per_cpu_times = 0; not yet

    size = 0;
    resident = 0;
    trs = 0;
    drs = 0;
    stack = 0;
    share = 0;
    mem = 0;
    swap = 0;

    io_read_prev = 0; // **
    io_write_prev = 0;
    io_read = 0;  // **
    io_write = 0; // **

    // tgid=0;
    pcpu = 0;
    pmem = 0;

    old_utime = 0; // this must be current utime !
    old_wcpu = 0;

    command = "noname";
    tty = 0;
    nice = 0;
    starttime = 0;
    state = 'Z';
    cutime = utime = 0;

    nthreads = 0; /* number of threads */

    hashstr[0] = 0;
    hashlen = 0;
}

Procinfo::~Procinfo()
{
    if (!clone)
    {
        void watchdog_check_if_finish(QString cmd, Procinfo * p);
        watchdog_check_if_finish(command, this);

        if (detail)
        {
            //	printf("~Procinfo() : pid=%d\n",pid);
            detail->process_gone();
            detail = nullptr;
        }

        qDeleteAll(sock_inodes.begin(), sock_inodes.end());
        sock_inodes.clear();

        qDeleteAll(fd_files.begin(), fd_files.end());
        fd_files.clear();

        qDeleteAll(maps.begin(), maps.end());
        maps.clear();

        //    if(environ)    delete environ;
        if (envblock)
            free(envblock); /// double free , SEGFAULT
    }

    //	fd_files.squeeze();
    //	maps.squeeze();
    /*
            if(maps) {
                    maps->purge();
                    delete maps;
            }
            if(fd_files) {
                    fd_files->purge();
                    delete fd_files;
            }
    */

    //	if(children)
    //	{	children->clear();	delete children; }
    ///	delete[] per_cpu_times;
}

// miscellaneous static initializations
void Proc::init_static()
{

    // socks.setAutoDelete(true);
    /// usocks.setAutoDelete(true);

    pagesize = sysconf(_SC_PAGESIZE); // same getpagesize()  in <unistd.h>
    //	printf("pagesize=%d, %d\n",getpagesize(),
    // sysconf(_SC_PAGESIZE)); //4027
}

//  tricky function...(by fasthyun@magicn.com)
//  Description :
//  	let's deal thread as normal process!
//		read /proc/PID/task/* and add to Proc::procs[]
int Proc::read_pid_tasks(int pid)
{
    char path[256];
    struct dirent *e;
    int thread_pid;
    int thread_n = 0;
    Procinfo *pi = nullptr;

    sprintf(path, "/proc/%d/task", pid);

    DIR *d = opendir(path);
    if (!d)
        return -1; // process dead  already!

    while ((e = readdir(d)) != nullptr)
    {
        if (e->d_name[0] == '.')
            continue; // skip "." , ".."

        thread_pid = atoi(e->d_name);
        if (pid == thread_pid)
            continue; // skip

        pi = procs.value(thread_pid, NULL);

        if (pi == nullptr)
        {
            pi = new Procinfo(this, pid, thread_pid);
            procs.insert(thread_pid, pi);
        }
        if (pi->readproc() >= 0)
        {
            pi->generation = current_gen;
            //		if(pid!=thread_pid)
            // pi->cmdline="(thread)";
        }
        thread_n++;
    }
    closedir(d);
    return thread_n;
}

// update wcpu,%cpu field
void Procinfo::calculate_cpu() //
{
}

// using cache for Speed up
int Procinfo::hashcmp(char *sbuf)
{
    size_t statlen;

    statlen = strlen(sbuf);
    if (statlen > sizeof(hashstr))
    {
        // some user reported 265byte.
        printf("Qps BUG:  hashstr shortage statlen(%zu) > hashstr(%zu), "
               "report this "
               "message to fasthyun@magicn.com \n",
               statlen, sizeof(hashstr));
        abort();
    }
    else if (statlen == hashlen)
    {
        if (memcmp(hashstr, sbuf, statlen) == 0)
        {
            pcpu = 0;
            // 1. I am a sleeping process
            // printf("[%d] sleep process \n",pid);
            return 1;
        }
    }
    memcpy(hashstr, sbuf, statlen); // to back
    hashlen = statlen;
    return 0;
}

int mini_sscanf(const char *s, const char *fmt, ...);

// Description :	read /proc/PID/*   or	read /proc/PID/task/*
//      be called every refresh() time.
//      return -1 means the process already dead !
int Procinfo::readproc()
{
    char cmdbuf[MAX_CMD_LEN];
    char path[64];
    char *sbuf; // should be enough to acommodate /proc/PID/stat
    char *buf;

//    int x_pid; // just pid
    int i_tty; //
    long stime, cstime;

    // Note :  /proc/PID/* is not same /proc/task/PID/*
    if (isThread()) // flag_thread_ok
    {
        sprintf(path, "/proc/%d/task/%d", tgid, pid);
    }
    else
        sprintf(path, "/proc/%d", pid);

    if (first_run)
    {
        //	Note: COMMAND(?) , TGID, UID , COMMAND_LINE  never
        // change !
        old_wcpu = wcpu = pcpu = 0.0;

        // read /proc/PID/status
        if ((buf = read_proc_file2(path, "status")) == nullptr)
            return -1;

        // Note: Process_name from
        // 		1.status (15 chars-name)
        // 		2.stat (15 chars-name, pass)
        // 		3.cmdline : full name  (sometimes have null, Thread
        // can't use
        // cmdline)
        // 		4.exe : full name (frequently this does not exist,
        // thread can't use
        // exe)
        // 		5.comm : 15 chars ?
        //
        // Note:
        //  1. thread's name_max is 15 chars

        if (mini_sscanf(buf, "Name: %S\n", cmdbuf) == 0)
            return -1;
        else
        {
            command = cmdbuf;
            if (command.contains("kthread"))
                hidekids = true; // kthread, kthreadd ,
                                 // ///Procinfo::qps_pid=pid;
        }

        if (mini_sscanf(buf, "Tgid: %d ", &tgid) == 0)
            return -1;
        if (mini_sscanf(buf, "Uid: %d %d %d %d", &uid, &euid, &suid, &fsuid) !=
            4)
            return -1;
        if (mini_sscanf(buf, "Gid: %d %d %d %d", &gid, &egid, &sgid, &fsgid) !=
            4)
            return -1;

        username = userName(uid, euid);
        groupname = groupName(gid, egid);

        int bug = 0;
        char cmdline_cmd[4096]; // some cmdline very large!  ex)chrome
        // read /proc/pid/cmdline
        int size;
        cmdline_cmd[0] = 0;

        // anyone can read [cmdline]
        if ((buf = read_proc_file2(path, "cmdline", &size)) == nullptr)
            return -1;
        else
        {
            // printf("DEBUG: size=%d \n",size);
            int cmdlen = strlen(buf);

            if (cmdlen == 0)
            {
                // 1. kthread
                // printf("Qps:debug no_cmdline pid=%d\n",pid );
                cmdline = "";
            }
            // for non-ascii locale language
            // cmdline = codec->toUnicode(cmdbuf,strlen(cmdbuf));
            else
            {

                // change 0x00,0xA to ' '
                for (int i = 0; i < size - 1; i++) // OVERFLOW
                    if (buf[i] == 0 or buf[i] == 0x0A)
                        buf[i] = ' ';
                cmdline = buf;
                strcpy(cmdline_cmd, buf);
            }
        }

        // VERY COMPLEX CODE
        // because Command's MAX_length is only 15, so sometimes
        // cmd_name truncated,
        // we should guess ...
        //
        // The solution is...
        // 1.check [exe] file ( only owner can read it)
        // 2.check [cmdline] ( anyone can read it )
        // 3.check [comm]
        //
        if (command.size() == 15)
        {
            // only root & owner can read [exe] link
            /*
                    if((buf= read_proc_file2(path,"exe")) !=0 )
                    {
                            // printf("Qps:debug %s\n",buf );
                            if(strlen(basename(buf))>15 and
                                    strncmp(qPrintable(command),basename(buf),15)==0
               )
                                    command=basename(buf); // no
               memory leak !
                            else  ;// just use command
                            //printf("Qps:debug %s\n",buf );
                    } */

            if (true) // guess the full name of the command
            {
                // Use /proc/PID/cmdline, comm, status
                // ex)
                //     /usr/lib/chromium/chromium --option1
                //     --option2
                //     python /usr/lib/system-service-d
                //	   pam: gdm-password
                //	   hald-addon-input: Listing On /dev~
                //
                char *p;
                p = strstr(cmdline_cmd, ": "); // cut the options !
                if (p != nullptr)
                    *p = 0;
                p = strchr(cmdline_cmd, ' '); // cut the options !
                if (p != nullptr)
                    *p = 0;

                // printf("Qps:debug %s\n",cmdline_cmd );
                char *pstart = strstr(basename(cmdline_cmd), cmdbuf);
                if (pstart != nullptr)
                {
                    command = pstart; // copy
                                      /// printf("Qps:debug2
                                      /// %s\n",basename(cmdline_cmd));
                }
            }
        }

        if (isThread())
            cmdline = command + " (thread)";

        if (flag_devel and bug)
        {
            // command.append("^");
            cmdline += " ^ Qps: may be a wrong commandline ";
        }

        void watchdog_check_if_start(QString cmd, Procinfo * ps);
        watchdog_check_if_start(command, this); // segfault possible.

        first_run = false;
    }

    if (flag_schedstat == true)
    {
        // if no change then return.    twice faster !
        // MAX_256 bytes check...?
        // 2.6.9 upper only and some system no has
        if ((sbuf = read_proc_file2(path, "schedstat")) == nullptr)
            return -1;

        if (hashcmp(sbuf))
            return 1; // no change
    }
    // read /proc/PID/stat
    if ((sbuf = read_proc_file2(path, "stat")) == nullptr)
        return -1;

    if (flag_schedstat == false) // if no change then return.    twice faster !
    {
        if (hashcmp(sbuf))
            return 1;
    }

    /// if (proc_pid_fd(pid)== true) ;  // bottleneck !!

    /*
            Not all values from /proc/#/stat are interesting; the ones left
       out
            have been retained in comments to see where they should go, in
       case
            they are needed again.

            Notes :
                    1. man -S 5 proc
                    2. man -S 2 times
                    3. ppid can be changed when parent dead !
                    4. initial utime maybe 0, so %CPU field NotAnumber !!
            utime: user time
            stime: kernel mode tick
            cutime : The  number  of jiffies that this process's waited-for
       children have been scheduled in user mode.

            #jiffies == tick
    */
    unsigned int guest_utime, cguest_utime;
#if 1
    char *p, *p1;
    // in odd cases the name can contain spaces and '(' or ')' and numbers,
    // so
    // this makes
    // parsing more difficult. We scan for the outermost '(' ')' to find the
    // name.
    p = strchr(sbuf, '(');
    p1 = strrchr(sbuf, ')');
    if (p == nullptr || p1 == nullptr)
        return -1;
    p1++;
    // we can safely use sscanf() on the rest of the string
    sscanf(p1,
           " %c %d %d %d %d %d"
           " %lu %lu %lu %lu %lu "
           "%ld %ld %ld %ld %d %d %d %*s %lu %*s %*s %*s %*s %*s %*s %*s %*s "
           "%*s %*s %*s %*s %lu %*s %*s %*s %d %*s %*s %*s %u %u",
#else
    // some errors will occur !
    mini_sscanf(
        sbuf,
        "%d (%S) %c %d %d %d %d %d"
        "%lu %lu %lu %lu %lu "
        "%ld %ld %ld %ld %d %d %d %*s %lu %*s %*s %*s %*s %*s %*s %*s %*s "
        "%*s %*s %*s %*s %lu %*s %*s %*s %u %*s %*s %*s %u %u",
        &x_pid, &cmdbuf[0],
#endif
           &state, &ppid, &pgrp, &session, &i_tty, &tpgid, &flags, &minflt,
           &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, &cstime,
           &priority, &nice, &nthreads /* number of threads v2.6 */,
           /* itrealvalue */
           &starttime, /* start time(static) */ // jiffes
                                                /* vsize */
                                                /* rss */
           /* rlim, startcode, endcode, startstack kstkesp kstkeip,
              signal, blocked, sigignore, sigcatch */
           &wchan,
           /* 0L, 0L, exit_signal */
           &which_cpu
           /* rt_priority, policy, delayacct_blkio_ticks  */
           ,
           &guest_utime, &cguest_utime);

    starttime = proc->boot_time /* secs */ + (starttime / proc->clk_tick);

    tty = (dev_t)i_tty; // hmmm
    // if(tty!=0)	printf("pid=%d tty =%d\n",pid,tty);

    //	if(guest_utime>0 or cguest_utime>0)
    //	printf("cmd [%s] guest_utime=%d cguest_utime
    //=%d\n",qPrintable(command),guest_utime,cguest_utime);

    utime += stime; // we make no user/system time distinction
    cutime += cstime;

    if (old_utime > 0) // check..
    {
        int dcpu;
        dcpu = utime - old_utime; // user_time from proc
        if (dcpu < 0)
        {
            // why.. this occurs ?
            // Qps exception:[3230,firefox] dcpu=-22 utime=39268
            // old_utime=39290 why
            // occur?
            if (flag_devel)
                printf("Qps :[%d,%s] dcpu=%d utime=%ld "
                       "old_utime=%ld why occurs?\n",
                       pid, qPrintable(command), dcpu, utime, old_utime);
            return 1;
        }

        // gettimeofday(&tv, 0); 	//sys/time
        if (proc->dt_total > 0) // move to Proc ??
        {
            pcpu = 100.0 * dcpu / proc->dt_total;
            if (Procview::flag_pcpu_single == true)
                pcpu *= proc->num_cpus; //
        }
        // else  too fast read again
        // printf("Qps exception: dt_total=%d  report to
        // fasthyun@magicn.com
        // \n",Proc::dt_total);

        if (flag_devel and pcpu > 100) // DEBUG CODE
        {
            printf("Qps pcpu error: %0.0f%% [%d,%s] dt_total=%ld "
                   "dcpu=%d utime=%ld "
                   "old_utime=%ld \n",
                   pcpu, pid, qPrintable(command), proc->dt_total, dcpu, utime,
                   old_utime);
            pcpu = 99.99;
        }

        const float a = Procview::avg_factor;
        wcpu = a * old_wcpu + (1 - a) * pcpu;
    }
    old_tv = tv;
    old_wcpu = wcpu;
    old_utime = utime; // ****

    // read /proc/%PID/statm  - memory usage
    if (true)
    {
        if ((buf = read_proc_file2(path, "statm")) == nullptr)
            return -1; // kernel 2.2 ?
        sscanf(buf, "%lu %lu %lu %lu %lu %lu %lu", &size, &resident, &share,
               &trs, &lrs, &drs, &dt);
        size *= pagesize / 1024; // total memory in kByte
        resident *= pagesize / 1024;
        share *= pagesize / 1024; // share
                                  //	trs		;	// text(code)
                                  //	lrs		;	// zero : lib, awlays zero in
        // Kernel 2.6
        //	drs		;	// data: wrong in kernel 2.6
        //	dt		;	// zero : in Kernel 2.6
        mem = resident - share;
        // pmem = 100.0 * resident / proc->mem_total;
        pmem = 100.0 * mem / proc->mem_total;
    }

    // read /proc/PID/status check !!
    if ((buf = read_proc_file2(path, "status")) == nullptr)
        return -1;
    else
    {
        // slpavg : not supported in kernel 2.4; default value of -1
        if (mini_sscanf(buf, "SleepAVG:%d", &slpavg) == 0)
            slpavg = -1;

        if (strstr(buf, "VmSize:"))
        {
            // mini_sscanf(p, "VmSize: %d",&size);	// XXX
            // mini_sscanf(p, "VmRSS: %d",&resident);
            // mini_sscanf(sbuf, "VmLib: %d",&share);
            mini_sscanf(buf, "VmData: %d", &drs);  // data	in kByte
            mini_sscanf(buf, "VmStk: %d", &stack); // stack	in kByte
            mini_sscanf(buf, "VmExe: %d", &trs);   // text
        }
        if (strstr(buf, "VmSwap:"))
            mini_sscanf(buf, "VmSwap: %d", &swap);   // swap in kByte
    }

    /*
      generally
      shared = RSS - ( CODE + DATA + STACK )
      share= resident - trs -drs -stack;

     // Defines from task_mmu.c of kernel source
     total_vm==size
     data = mm->total_vm - mm->shared_vm - mm->stack_vm;
     swap = p->size - p->resident ;
    */

    // read /proc/PID/file_io
    // NOTE:  2.6.11 dont have IO file
    // COMPLEX_CODE
    if ((buf = read_proc_file2(path, "io")) != nullptr)
    {
        // rchar = ... not file maybe sockread
        //
        mini_sscanf(buf, "read_bytes:%d", &io_read);
        mini_sscanf(buf, "write_bytes:%d", &io_write);
        io_read /= 1024; // io_read is in KB
        io_write /= 1024; // io_write is in KB

        // if(io_read_prev!=0)
        {
            if (io_read_prev == 0)
                io_read_prev = io_read;
            if (io_write_prev == 0)
                io_write_prev = io_write;

            io_read_KBps = 1000 * (io_read - io_read_prev) / proc->update_msec;
            io_write_KBps = 1000 * (io_write - io_write_prev) / proc->update_msec;

            proc->io_byte += io_read_KBps; // test
            proc->io_byte += io_write_KBps;
        }

        io_read_prev = io_read;
        io_write_prev = io_write;

        // io_read>>=10; //  divide by 1024
        // io_write>>=10; //  divide by 1024
    }
    // per_cpu_times = 0; // not yet

    if ((buf = read_proc_file2(path, "wchan")) != nullptr)
    {
        wchan_str = buf;
    }

    policy = -1; // will get it when needed
    rtprio = -1; // ditto
    tms = -1;    // ditto

    // useless ?  if(detail)	detail->set_procinfo(this);  // BAD !!!
    return 2; // return ok.
}

// just grab the load averages
// called by
void Proc::read_loadavg()
{
    char path[80];
    char buf[512];
    int n;
    strcpy(path, "/proc/loadavg");
    if ((n = read_file(path, buf, sizeof(buf) - 1)) <= 0)
    {
        fprintf(stderr, "qps: Cannot open /proc/loadavg  (make sure "
                        "/proc is mounted)\n");
        exit(1);
    }
    buf[n] = '\0';
    sscanf(buf, "%f %f %f", &loadavg[0], &loadavg[1], &loadavg[2]);
}

int Proc::countCpu()
{
    char path[80];
    char buf[1024 * 8]; // for SMP

    int num_cpus = 0, n;
    // read system status  /proc/stat
    strcpy(path, "/proc/stat");
    // if((buf= read_proc_file("stat:)) ==0 ) return -1;
    if ((n = read_file(path, buf, sizeof(buf) - 1)) <= 0)
    {
        printf("Qps Error: /proc/stat can't be read ! check it and "
               "report to "
               "fasthyun@magicn.com\n");
        abort(); //	return 0;
    }
    buf[n] = '\0';

    // count (current) cpu of system
    char *p;
    p = strstr(buf, "cpu");
    while (p < buf + sizeof(buf) - 4 && strncmp(p, "cpu", 3) == 0)
    {
        num_cpus++;
        if (strncmp(p, "cpu0", 4) == 0)
            Proc::num_cpus--;
        p = strchr(p, '\n');
        if (p)
            p++;
    }
    return num_cpus;
}

// LINUX
// Description:  read common information  for all processes
// return value
// 		-1 : too fast refresh !
int Proc::read_system() //
{
    static bool first_run = true;
    char path[80];
    char buf[1024 * 8]; // for SMP

    char *p;
    int n;

    if (first_run)
    {
        /* Version 2.4.x ? */
        strcpy(path, "/proc/vmstat");
        if (!stat(path, (struct stat *)buf))
            flag_24_ok = false;
        else
            flag_24_ok = true;

        /* NPTL(Native POSIX Thread Library) */
        strcpy(path, "/proc/1/task");
        if (!stat(path, (struct stat *)buf))
            flag_thread_ok = true;
        else
            flag_thread_ok = false;

        /* check schedstat  */
        strcpy(path, "/proc/1/schedstat"); // some system doesn't have
        if (!stat(path, (struct stat *)buf))
            flag_schedstat = true;
        else
            flag_schedstat = false;

        strcpy(path, "/proc/stat");
        if ((n = read_file(path, buf, sizeof(buf) - 1)) <= 0)
            return 0;
        buf[n] = '\0';
        p = strstr(buf, "btime");
        if (p == nullptr)
        {
            // used
            printf("Qps: A bug occurs ! [boot_time] \n");
            // boot_time= current time
        }
        else
        {
            p += 6;
            // sscanf(p, "%d", &Proc::boot_time); //???? why
            // segfault???
            sscanf(p, "%u", &boot_time);
        }

        // Max SMP 1024 cpus,  MOVETO: COMMON
        int max_cpus = 512;
        cpu_times_vec = new unsigned[CPUTIMES * max_cpus]; //??? +2
        old_cpu_times_vec = new unsigned[CPUTIMES * max_cpus];

        // init
        for (int cpu = 0; cpu < max_cpus; cpu++)
            for (int i = 0; i < CPUTIMES; i++)
            {
                cpu_times(cpu, i) = 0;
                old_cpu_times(cpu, i) = 0;
            }

        // first_run=false; // not yet , at the bottom of this function
    }

    // read system status  /proc/stat
    strcpy(path, "/proc/stat");
    // if((buf= read_proc_file("stat:)) ==0 ) return -1;
    if ((n = read_file(path, buf, sizeof(buf) - 1)) <= 0)
    {
        printf("Qps Error: /proc/stat can't be read ! check it and "
               "report to "
               "fasthyun@magicn.com\n");
        abort(); //	return 0;
    }
    buf[n] = '\0';

    if (true)
    {
        // count (current) cpu of system
        char *p;
        p = strstr(buf, "cpu");
        num_cpus = 0;
        while (p < buf + sizeof(buf) - 4 && strncmp(p, "cpu", 3) == 0)
        {
            num_cpus++;
            if (strncmp(p, "cpu0", 4) == 0)
                Proc::num_cpus--;
            p = strchr(p, '\n');
            if (p)
                p++;
        }

        // Hotplugging Detection : save total_cpu
        if (Proc::num_cpus != Proc::old_num_cpus)
        {
            //	for(int i = 0; i < CPUTIMES; i++)
            //		cpu_times(num_cpus, i) =
            // cpu_times(Proc::old_num_cpus,
            // i);

            Proc::old_num_cpus = Proc::num_cpus;
        }
    }

    // backup old values :  important*******
    for (int cpu = 0; cpu < Proc::num_cpus + 1; cpu++)
    {
        for (int i = 0; i < CPUTIMES; i++)
            old_cpu_times(cpu, i) = cpu_times(cpu, i);
    }

    /*
            /proc/stat
            cpu#	user	nice	system	idle		iowait(2.6)
       irq(2.6)	sft(2.6)	steal(2.6.11) 	guest(2.6.24)
            cpu0	3350 	9		535		160879
       1929		105			326			5
       1200

            Q1: kernel 2.4 cpu0 exist ?
    */

    // Total_cpu
    unsigned user, nice, system, idle, iowait, irq, sftirq, steal, guest, nflds;
    nflds = sscanf(buf, "cpu %u %u %u %u %u %u %u %u %u", &user, &nice, &system,
                   &idle, &iowait, &irq, &sftirq, &steal, &guest);
    if (nflds > 4)
    {
        // kernel 2.6.x
        system += (irq + sftirq);
        idle += iowait;
    }
    if (nflds == 9)
    {
        system += steal;
        system += guest;
    }
    cpu_times(Proc::num_cpus, CPUTIME_USER) = user;
    cpu_times(Proc::num_cpus, CPUTIME_NICE) = nice;
    cpu_times(Proc::num_cpus, CPUTIME_SYSTEM) = system;
    cpu_times(Proc::num_cpus, CPUTIME_IDLE) = idle;

    // DRAFT!
    // num_cpus == total_cpu
    //
    // dt_total= user + system + nice + idle
    // dt_used= user + system;
    Proc::dt_used =
        user - old_cpu_times(Proc::num_cpus,
                             CPUTIME_USER); // infobar uses this value
    Proc::dt_used += system - old_cpu_times(Proc::num_cpus, CPUTIME_SYSTEM);
    Proc::dt_total = dt_used + nice -
                     old_cpu_times(Proc::num_cpus, CPUTIME_NICE) + idle -
                     old_cpu_times(Proc::num_cpus, CPUTIME_IDLE);

    load_cpu = (float)Proc::dt_used / Proc::dt_total; // COMMON

    if (first_run)
    {
        //	printf("\n==================== tooo fast
        //=================================\n");
        //	printf("DEBUG:dt_total=%d
        // dt_used=%d\n",Proc::dt_total,Proc::dt_used);
        //	return -1; // too early refresh again  !!
    }
    if (Proc::dt_total == 0)
    {
        //?????
        printf("Error: dt_total=0 , dt_used=%ld(%u)  report to "
               "fasthyun@magicn.com\n",
               Proc::dt_used, old_cpu_times(Proc::num_cpus, CPUTIME_IDLE));
        dt_total = 500; // more tolerable?
                        // abort(); // stdlib.h
    }

    for (int cpu = 0; cpu < num_cpus; cpu++)
    {
        char cpu_buf[13];
        sprintf(cpu_buf, "cpu%d", cpu);
        if ((p = strstr(buf, cpu_buf)) != nullptr)
        {
            nflds = sscanf(p, "%*s %u %u %u %u %u %u %u %u %u",
                           &cpu_times(cpu, CPUTIME_USER),
                           &cpu_times(cpu, CPUTIME_NICE),
                           &cpu_times(cpu, CPUTIME_SYSTEM),
                           &cpu_times(cpu, CPUTIME_IDLE), &iowait, &irq,
                           &sftirq, &steal, &guest);

            if (nflds > 4)
            {
                // kernel 2.6.x
                cpu_times(cpu, CPUTIME_SYSTEM) += (irq + sftirq);
                cpu_times(cpu, CPUTIME_IDLE) += iowait;
            }
            if (nflds == 9)
            {
                cpu_times(cpu, CPUTIME_SYSTEM) += (steal + guest);
            }

            // 2.4.27-SMP bug ?
        }
        else
        {
            fprintf(stderr, "Qps: Error reading info for "
                            "cpu%d (/proc/stat)\n",
                    cpu);
            abort();
        }
    }

    // read memory info
    strcpy(path, PROCDIR);
    strcat(path, "/meminfo");
    if ((n = read_file(path, buf, sizeof(buf) - 1)) <= 0)
        return 0;
    buf[n] = '\0';

    // Skip the old /meminfo cruft, making this work in post-2.1.42 kernels
    // as well.  (values are now in kB)
    if ((p = strstr(buf, "MemTotal:")))
        sscanf(p, "MemTotal: %d kB\n", &mem_total);
    if ((p = strstr(buf, "MemFree:")) != nullptr)
        sscanf(p, "MemFree: %d kB\n", &mem_free);
    if ((p = strstr(buf, "Buffers:")) != nullptr)
        sscanf(p, "Buffers: %d kB\n", &mem_buffers);
    if ((p = strstr(buf, "Cached:")) != nullptr)
        sscanf(p, "Cached: %d kB\n", &mem_cached);

    p = strstr(buf, "SwapTotal:");
    sscanf(p, "SwapTotal: %d kB\nSwapFree: %d kB\n", &swap_total, &swap_free);

    first_run = false;
    return 0;
}

int Procinfo::get_policy()
{
    if (policy == -1)
        policy = sched_getscheduler(pid);
    return policy;
}

int Procinfo::get_rtprio()
{
    if (rtprio == -1)
    {
        struct sched_param p;
        if (sched_getparam(pid, &p) == 0)
            rtprio = p.sched_priority;
    }
    return rtprio;
}

double Procinfo::get_tms()
{
    struct timespec ts;
    if (sched_rr_get_interval(pid, &ts) == -1) // POSIX
        tms = -1;                              // should not be possible
    else
    {
        tms = ts.tv_nsec; // nano seconds
        tms /= 1000000;   // mili seconds
        tms += ts.tv_sec * 1000;
    }
    return tms;
}

unsigned long Procinfo::get_affcpu()
{
#ifdef QPS_SCHED_AFFINITY
    if (qps_sched_getaffinity(pid, sizeof(unsigned long), &affcpu) == -1)
        affcpu = (unsigned long)0;
#else
    if (sched_getaffinity(pid, sizeof(unsigned long), (cpu_set_t *)&affcpu) ==
        -1)
        affcpu = (unsigned long)0;
#endif
    return affcpu;
}

// Description : read  /proc/PID/fd/* (SYMBOLIC LINK NAME)
/* We need to implement support for IPV6 and sctp ? */
void Procinfo::read_fd(int fdnum, char *path)
{
    int len;
    char buf[128];
    struct stat sb;

    // The fd mode is contained in the link permission bits
    if (lstat(path, &sb) < 0)
        return;
    int mode = 0;
    if (sb.st_mode & 0400)
        mode |= OPEN_READ;
    if (sb.st_mode & 0200)
        mode |= OPEN_WRITE;

    if ((len = readlink(path, buf, sizeof(buf) - 1)) > 0)
    {
        buf[len] = '\0';
        unsigned long dev, ino;

        // check socket_fd
        if ((buf[0] == '[' and sscanf(buf, "[%lx]:%lu", &dev, &ino) == 2 and
             dev == 0) // Linux 2.0 style
            ||
            sscanf(buf, "socket:[%lu]", &ino) > 0) // Linux 2.1 upper
        {
            Sockinfo *si = nullptr;
            si = proc->socks.value(ino, NULL); // sock
            char buf[80];
            if (si)
            {
                printf("sock ino=%lu\n", ino);
                si->pid = pid;
                // a TCP or UDP socket
                sock_inodes.append(new SockInode(fdnum, ino));
                sprintf(buf, "%s socket %lu",
                        si->proto == Sockinfo::TCP ? "TCP" : "UDP", ino);
                fd_files.append(new Fileinfo(fdnum, buf, mode));
                return;
            }
            else
            {
                // maybe a unix domain socket?
                // read_usockets();
                UnixSocket *us = nullptr;

                us = proc->usocks.value(ino, NULL);
                if (us)
                {
                    const char *tp = "?", *st = "?";
                    switch (us->type)
                    {
                    case SOCK_STREAM:
                        tp = "stream";
                        break;
                    case SOCK_DGRAM:
                        tp = "dgram";
                        break;
                    }
                    switch (us->state)
                    {
                    case SSFREE:
                        st = "free";
                        break;
                    case SSUNCONNECTED:
                        st = "unconn";
                        break;
                    case SSCONNECTING:
                        st = "connecting";
                        break;
                    case SSCONNECTED:
                        st = "connected";
                        break;
                    case SSDISCONNECTING:
                        st = "disconn";
                        break;
                    }
                    sprintf(buf, "UNIX domain socket %lu (%s, %s) ", ino, tp,
                            st);
                    QString s = buf;
                    s.append(us->name);
                    fd_files.append(new Fileinfo(fdnum, s, mode));
                    return;
                }
            }
        }
        // normal filess
        // assume fds will be read in increasing order
        fd_files.append(new Fileinfo(fdnum, buf, mode));
    }
}

// Description :
// 		read /PID/fd opened files
// 		return true if /proc/PID/fd could be read, false otherwise
// 		store fileinfo, and also socket inodes separately
//
// called by Detail()
bool Procinfo::read_fds()
{
    char path[128];
    if (flag_thread_ok && flag_show_thread)
        sprintf(path, "/proc/%d/task/%d/fd", pid, pid);
    else
        sprintf(path, "/proc/%d/fd", pid);

    DIR *d = opendir(path);
    if (!d)
        return false;

    /*
    if(!sock_inodes) 	sock_inodes = new Svec<SockInode*>(4);
    else 	sock_inodes->purge(); */

    strcat(path, "/");

    fd_files.clear(); //
    struct dirent *e;
    while ((e = readdir(d)) != nullptr)
    {
        char str[128];
        if (e->d_name[0] == '.')
            continue; // skip . and ..
        int fdnum = atoi(e->d_name);
        strcpy(str, path);
        strncat(str, e->d_name, 128 - 1 - strlen(str));
        //	printf("str=%s\n",str);
        read_fd(fdnum, str);
    }
    //	printf("end str=\n");
    closedir(d);
    return true;
}

// tcp, udp
bool Proc::read_socket_list(Sockinfo::proto_t proto, const char *filename)
{
    char path[80];
    sprintf(path, "/proc/net/%s", filename);
    FILE *f = fopen(path, "r");
    if (!f)
        return false;

    char buf[128 * 3];
    // Header
    // sl  local_addr rem_addr  st tx_queue rx_queue tr tm->when retrnsmt
    // uid
    // timeout inode

    Sockinfo si;

    printf("read_socket_list()\n");
    fgets(buf, sizeof(buf), f); // skip header
    while (fgets(buf, sizeof(buf), f) != nullptr)
    {
        //	Sockinfo *si = new Sockinfo;
        si.pid = -1;
        si.proto = proto;
        unsigned local_port, rem_port, st, tr;
        sscanf(buf + 6, "%x:%x %x:%x %x %x:%x %x:%x %x %d %d %d",
               &si.local_addr, &local_port, &si.rem_addr, &rem_port, &st,
               &si.tx_queue, &si.rx_queue, &tr, &si.tm_when, &si.rexmits,
               &si.uid, &si.timeout, &si.inode);
        // fix fields that aren't sizeof(int)
        si.local_port = local_port;
        si.rem_port = rem_port;
        si.st = st;
        si.tr = tr;

        Sockinfo *psi;
        psi = socks.value(si.inode, NULL);
        if (psi == nullptr)
        {
            printf("inode =%d \n", si.inode);
            psi = new Sockinfo;
            *psi = si;
            socks.insert(si.inode, psi);
        }
        else
            *psi = si;

        // linear_socks.add(si);
    }
    fclose(f);
    return true;
}

bool Proc::read_usocket_list()
{
    char path[80];
    strcpy(path, PROCDIR);
    strcat(path, "/net/unix");
    FILE *f = fopen(path, "r");
    if (!f)
        return false;

    char buf[256];
    fgets(buf, sizeof(buf), f); // skip header
    while (fgets(buf, sizeof(buf), f))
    {
        if (buf[0])
            buf[strlen(buf) - 1] = '\0'; // chomp newline
        // UnixSocket *us = new UnixSocket;
        UnixSocket us;

        unsigned q;
        unsigned type, state;
        int n;
        sscanf(buf, "%x: %x %x %x %x %x %lud %n",
               // Num	refcount protocol flags type state indoe path
               &q, &q, &q /*protocol*/, &us.flags, &type, &state, &us.inode,
               &n);
        us.name = buf + n;
        us.type = type;
        us.state = state;

        UnixSocket *pus;
        pus = usocks.value(us.inode, NULL);
        if (pus == nullptr)
        {
            printf("inode =%lu \n", us.inode);

            pus = new UnixSocket;
            *pus = us;
            usocks.insert(us.inode, pus);
        }
        else
            *pus = us;
    }
    fclose(f);
    return true;
}

void Proc::read_sockets()
{
    // socks.clear();

    // memory leak !!
    if (!read_socket_list(Sockinfo::TCP, "tcp") ||
        !read_socket_list(Sockinfo::UDP, "udp"))
        return;
    // remove no more socket ino
    read_usocket_list();

    socks_current = true;
}

void Proc::read_usockets()
{
    if (usocks_current)
        return;

    usocks.clear();
    if (!read_usocket_list())
        return;

    usocks_current = true;
}

void Proc::invalidate_sockets() { socks_current = usocks_current = false; }

// return true if /proc/XX/maps could be read, false otherwise
bool Procinfo::read_maps()
{
    // prevent memory leak
    qDeleteAll(maps.begin(), maps.end());
    maps.clear();

    // idea: here we could readlink /proc/XX/exe to identify the executable
    // when running 2.0.x
    char name[80];

    if (flag_thread_ok && flag_show_thread)
        sprintf(name, "/proc/%d/task/%d/maps", pid, pid);
    else
        sprintf(name, "/proc/%d/maps", pid);

    FILE *f = fopen(name, "r");
    if (!f)
        return false;

    char line[1024]; // lines can be longer , SEGFAULT

    while (fgets(line, sizeof(line), f))
    {
        Mapsinfo *mi = new Mapsinfo;
        int n;
        sscanf(line, "%lx-%lx %4c %lx %x:%x %lu%n",
               // sscanf(line, "%lx-%lx %4c %lx %x:%x %n",
               &mi->from, &mi->to, mi->perm, &mi->offset, &mi->major,
               &mi->minor, &mi->inode, &n);
        if (line[n] != '\n')
        {
            int len = strlen(line);
            if (line[len - 1] == '\n')
                line[len - 1] = '\0';
            while (line[n] == ' ' && line[n])
                n++;
            mi->filename = line + n;
        }
        else if ((mi->major | mi->minor | mi->inode) == 0)
            mi->filename = "(anonymous)";
        maps.append(mi);
    }
    fclose(f);
    if (maps.size() == 0)
        return false;

    return true;
}

// DRAFT CODE:
// return true if /proc/777/environ could be read, false otherwise
int fsize(char *fname);
bool Procinfo::read_environ()
{
    int file_size = 0;
    int size;
    char path[256];

    environ.clear(); //
    if (flag_thread_ok && flag_show_thread)
        sprintf(path, "/proc/%d/task/%d/environ", pid, pid);
    else
        sprintf(path, "/proc/%d/environ", pid);

    file_size = fsize(path);
    if (file_size <= 0)
        return false;

    envblock = (char *)malloc(file_size + 1);
    size = read_file(path, envblock, file_size + 1);
    if (size <= 0)
        return false;

    int i = 0, n = 0, v = 0;
    char ch;

    for (i = 0; i < size; i++)
    {
        ch = envblock[i];
        if (ch == '=')
        {
            envblock[i] = 0;
            v = i + 1;
        }
        if (ch == 0)
        {
            NameValue nv(&envblock[n], &envblock[v]);
            environ.append(nv);
            // printf("%s %s\n",&envblock[n],&envblock[v]);
            n = i + 1;
        }
    }
    if (envblock)
    {
        free(envblock); // refresh() // INVALID VALGRIND
        envblock = nullptr;
    }
    return true;
}

// CWD,ROOT only so...
Cat_dir::Cat_dir(const QString &heading, const QString &explain, const char *dirname,
                 QString Procinfo::*member)
    : Cat_string(heading, explain), dir(dirname), cache(member)
{
}

QString Cat_dir::string(Procinfo *p)
{
    if ((p->*cache).isNull())
    {
        char path[128], buf[512];

        if (flag_thread_ok && flag_show_thread)
            sprintf(path, "/proc/%d/task/%d/%s", p->pid, p->pid, dir);
        else
            sprintf(path, "/proc/%d/%s", p->pid, dir);

        int n = readlink(path, buf, sizeof(buf) - 1);
        if (n < 0)
        {
            // Either a kernel process, or access denied.
            // A hyphen is visually least disturbing here.
            p->*cache = "-";
            return p->*cache;
        }
        else if (buf[0] != '[')
        {
            // linux >= 2.1.x: path name directly in link
            buf[n] = '\0';
            p->*cache = buf;
            return p->*cache;
        }

        // Either a Linux 2.0 link in [device]:inode form, or a Solaris
        // link.
        // To resolve it, we just chdir() to it and see where we end up.
        // Perhaps we should change back later?
        if (chdir(path) < 0)
        {
            p->*cache = "-"; // Most likely access denied
        }
        else
        {
            // getcwd() is fairly expensive, but this is cached
            // anyway
            if (!getcwd(buf, sizeof(buf)))
            {
                p->*cache = "(deleted)";
            }
            else
                p->*cache = buf;
        }
    }
    return p->*cache;
}

Cat_state::Cat_state(const QString &heading, const QString &explain)
    : Category(heading, explain)
{
}

QString Cat_state::string(Procinfo *p)
{
    QString s("   ");
    int ni = p->nice;

    s[0] = p->state;
    s[1] = (p->resident == 0 && p->state != 'Z') ? 'W' : ' ';
    s[2] = (ni > 0) ? 'N' : ((ni < 0) ? '<' : ' ');
    return s;
}

// LINUX
Cat_policy::Cat_policy(const QString &heading, const QString &explain)
    : Category(heading, explain)
{
}

QString Cat_policy::string(Procinfo *p)
{
    QString s;
    switch (p->get_policy())
    {
    case SCHED_FIFO:
        s = "FI";
        break; // first in, first out
    case SCHED_RR:
        s = "RR";
        break; // round-robin
    case SCHED_OTHER:
        s = "TS";
        break; // time-sharing
    default:
        s = "??";
        break;
    }
    return s;
}

int Cat_policy::compare(Procinfo *a, Procinfo *b)
{
    return b->get_policy() - a->get_policy();
}

Cat_rtprio::Cat_rtprio(const QString &heading, const QString &explain)
    : Category(heading, explain)
{
}

QString Cat_rtprio::string(Procinfo *p)
{
    QString s;
    s.setNum(p->get_rtprio());
    return s;
}

int Cat_rtprio::compare(Procinfo *a, Procinfo *b)
{
    return b->get_rtprio() - a->get_rtprio();
}

// maybe tms COMMON
Cat_tms::Cat_tms(const QString &heading, const QString &explain)
    : Category(heading, explain)
{
}

QString Cat_tms::string(Procinfo *p)
{
    p->tms = p->get_tms();
    return QString::asprintf("%.3f", p->tms);
    // s.sprintf("%d",p->tms);
}

int Cat_tms::compare(Procinfo *a, Procinfo *b)
{
    return (int)((b->get_tms() - a->get_tms()) * 1000);
}

Cat_affcpu::Cat_affcpu(const QString &heading, const QString &explain)
    : Category(heading, explain)
{
}

QString Cat_affcpu::string(Procinfo *p)
{
    p->affcpu = p->get_affcpu();
    return QString::asprintf("%lx", p->affcpu);
}
/*
   int Cat_affcpu::compare(Procinfo *a, Procinfo *b)
   {
   return (int)(b->affcpu - a->affcpu);
   }
   */

// LINUX or COMMON?
Cat_time::Cat_time(const QString &heading, const QString &explain)
    : Category(heading, explain)
{
}

QString Cat_time::string(Procinfo *p)
{
    QString s;
    char buff[64];
    int ticks = p->utime;
    int ms;

    if (Procview::flag_cumulative)
        ticks += p->cutime;

    int t = ticks / p->proc->clk_tick; // seconds
    // COMPLEX CODE
    if (t < 10)
    {                                                 // ex. 9.23s
        ms = ticks / (p->proc->clk_tick / 100) % 100; // Need FIX
        sprintf(buff, "%1d.%02ds", t, ms);
    }
    else if (t < 60)
    { // ex. 48s
        sprintf(buff, "%5ds", t);
    }
    else if (t < 60 * 10)
    { // ex. 8.9m, 9.0m
        sprintf(buff, "%2d.%1dm", t / 60, (t % 60) / 6);
    }
    else if (t < 60 * 60)
    { // 58m
        sprintf(buff, "%5dm", t / 60);
    }
    else if (t < 24 * 3600)
    {                     //
        int h = t / 3600; // 1hour = 3600 = 60m*60s
        t %= 3600;
        sprintf(buff, "%2d:%02d", h, t / 60);
    }
    else if (t < 10 * 24 * 3600)
    {                      //
        int d = t / 86400; // 1 day = 24* 3600s
        t %= 86400;
        sprintf(buff, "%2d.%1dd", d, (t * 10 / (3600 * 24)));
    }
    else
    {
        int d = t / 86400; // 1 day = 24* 3600s
        sprintf(buff, "%5dd", d);
    }
    s = buff;
    return s;
}

int Cat_time::compare(Procinfo *a, Procinfo *b)
{
    int at = a->utime, bt = b->utime;
    if (Procview::flag_cumulative)
    {
        at += a->cutime;
        bt += b->cutime;
    }
    return bt - at;
}

// LINUX ?
Cat_tty::Cat_tty(const QString &heading, const QString &explain)
    : Cat_string(heading, explain)
{
}

QString Cat_tty::string(Procinfo *p) { return Ttystr::name(p->tty); }

Proc::Proc()
{
    // Note:
    categories.insert( F_PID
                     , new Cat_int( tr( "PID" ), tr( "Process ID" ), 6, &Procinfo::pid) );
    categories.insert( F_TGID
                     , new Cat_int( tr( "TGID" ), tr( "Task group ID ( parent of threads )" ),6, &Procinfo::tgid));
    categories.insert( F_PPID
                     , new Cat_int( tr( "PPID" ), tr( "Parent process ID" ), 6, &Procinfo::ppid));
    categories.insert( F_PGID
                     , new Cat_int( tr( "PGID" ), tr( "Process group ID" ), 6, &Procinfo::pgrp));
    categories.insert( F_SID
                     , new Cat_int( tr( "SID" ), tr( "Session ID" ), 6, &Procinfo::session));
    categories.insert( F_TTY
                     , new Cat_tty( tr( "TTY" ), tr( "Terminal") ) );
    categories.insert( F_TPGID
                     , new Cat_int( tr( "TPGID" ), tr( "Process group ID of tty owner" ), 6, &Procinfo::tpgid));
    categories.insert( F_USER
                     , new Cat_string( tr( "USER" ), tr( "Owner (*=suid root, +=suid a user)" ),&Procinfo::username));
    categories.insert( F_GROUP
                     , new Cat_string( tr( "GROUP" ), tr( "Group name (*=sgid other)" ),&Procinfo::groupname));
    categories.insert( F_UID
                     , new Cat_int( tr( "UID" ), tr( "Real user ID" ), 6, &Procinfo::uid));
    categories.insert( F_EUID
                     , new Cat_int( tr( "EUID" ), tr( "Effective user ID" ), 6, &Procinfo::euid));
    categories.insert( F_SUID
                      , new Cat_int( tr( "SUID" ), tr( "Saved user ID (Posix)" ), 6,&Procinfo::suid));
    categories.insert( F_FSUID
                     , new Cat_int( tr( "FSUID" ), tr( "File system user ID" ), 6,&Procinfo::fsuid));
    categories.insert( F_GID
                     , new Cat_int( tr( "GID" ), tr( "Real group ID" ), 6, &Procinfo::gid));
    categories.insert( F_EGID
                      , new Cat_int( tr( "EGID" ), tr( "Effective group ID" ), 6, &Procinfo::egid));
    categories.insert( F_SGID
                     , new Cat_int( tr( "SGID" ), tr( "Saved group ID (Posix)" ), 6,&Procinfo::sgid));
    categories.insert( F_FSGID
                     , new Cat_int( tr( "FSGID" ), tr( "File system group ID" ), 6,&Procinfo::fsgid));
    categories.insert( F_PRI
                     , new Cat_int( tr( "PRI" ), tr( "Dynamic priority" ), 4, &Procinfo::priority));
    categories.insert( F_NICE
                     , new Cat_int( tr( "NICE" ), tr( "Scheduling favour (higher -> less cpu time)" ), 4, &Procinfo::nice));
    categories.insert( F_NLWP
                     , new Cat_int( tr( "NLWP" ), tr( "Number of tasks(threads) in task group" ), 5, &Procinfo::nthreads));
    categories.insert( F_PLCY
                     , new Cat_policy( tr( "PLCY" ), tr( "Scheduling policy" ) ) );
    categories.insert( F_RPRI
                     , new Cat_rtprio( tr( "RPRI" ), tr( "Realtime priority (0-99, more is better)" ) ) );
    categories.insert( F_TMS
                     , new Cat_tms( tr( "TMS" ), tr( "Time slice in milliseconds" ) ) );
    categories.insert( F_SLPAVG
                     , new Cat_int( tr( "%SAVG" ), tr( "Percentage average sleep time (-1 -> N/A)" ), 4, &Procinfo::slpavg));
    categories.insert( F_AFFCPU
                     , new Cat_affcpu( tr( "CPUSET" ), tr( "Affinity CPU mask (0 -> API not supported)" ) ) ); // ???
    categories.insert( F_MAJFLT
                     , new Cat_uintl( tr( "MAJFLT" ), tr( "Number of major faults (disk access)" ), 8, &Procinfo::majflt));
    categories.insert( F_MINFLT
                     , new Cat_uintl( tr( "MINFLT" ), tr( "Number of minor faults (no disk access)" ), 8, &Procinfo::minflt));
    // Memory
    categories.insert( F_SIZE
                     , new Cat_memory( tr( "VSIZE" ), tr( "Virtual image size of process" ), 8, &Procinfo::size));
    categories.insert( F_RSS
                     , new Cat_memory( tr( "RSS" ), tr( "Resident set size" ), 8, &Procinfo::resident));
    categories.insert( F_MEM
                     , new Cat_memory( tr( "MEM" ), tr( "memory usage (RSS-SHARE)" ), 8, &Procinfo::mem));
    categories.insert( F_TRS
                     , new Cat_memory( tr( "TRS" ), tr( "Text(code) resident set size" ), 8, &Procinfo::trs));
    categories.insert( F_DRS
                     , new Cat_memory( tr( "DRS" ), tr( "Data resident set size(malloc+global variable)" ), 8, &Procinfo::drs));
    categories.insert( F_STACK
                     , new Cat_memory( tr( "STACK" ), tr( "Stack size" ), 8, &Procinfo::stack));
    categories.insert( F_SHARE
                     , new Cat_memory( tr( "SHARE" ), tr( "Shared memory with other libs" ), 8, &Procinfo::share));
    categories.insert( F_SWAP
                     , new Cat_swap( tr( "SWAP" ), tr( "Kbytes on swap device" ) ) );
    categories.insert( F_IOR
                     , new Cat_memory( tr( "IO_R" ), tr( "io read (file)" ), 8, &Procinfo::io_read));
    categories.insert( F_IOW
                     , new Cat_memory( tr( "IO_W" ), tr( "io write (file)" ), 8, &Procinfo::io_write));
    categories.insert( F_DT
                     , new Cat_uintl( tr( "DT" ), tr( "Number of dirty (non-written) pages" ), 7, &Procinfo::dt));
    categories.insert( F_STAT
                     , new Cat_state( tr( "STAT" ), tr( "State of the process " ) ) );
    categories.insert( F_FLAGS
                     , new Cat_hex( tr( "FLAGS" ), tr( "Process flags (hex)" ), 9, &Procinfo::flags));
    categories.insert( F_WCHAN
                     , new Cat_wchan( tr( "WCHAN" ), tr( "Kernel function where process is sleeping" ) ) );
    categories.insert( F_WCPU
                     , new Cat_percent( tr( "%WCPU" ), tr( "Weighted percentage of CPU (30 s average)" ), 6, &Procinfo::wcpu));
    categories.insert( F_CPU
                     , new Cat_percent( tr( "%CPU" ), tr( "Percentage of CPU used since last update" ), 6, &Procinfo::pcpu));
    categories.insert( F_PMEM
                     , new Cat_percent( tr( "%MEM" ), tr( "Percentage of memory used (RSS/total mem)" ), 6, &Procinfo::pmem));
    categories.insert( F_START
                     , new Cat_start( tr( "START" ), tr( "Time process started" ) ) );
    categories.insert( F_TIME,
                      new Cat_time( tr( "TIME" ), tr( "Total CPU time used since start" ) ) );
    categories.insert( F_CPUNUM
                     , new Cat_int( tr( "CPU" ), tr( "CPU the process is executing on (SMP system)" ), 3, &Procinfo::which_cpu));
    categories.insert( F_CMD
                     , new Cat_string( tr( "Process Name" ), tr( "the process name" ), &Procinfo::command));
//    categories.insert( F_PROCESSNAME
//                     , new Cat_string( tr( "Process Name " ),tr( "the process name" ), &Procinfo::command));
    categories.insert( F_CWD
                     , new Cat_dir( tr( "CWD" ), tr( "Current working directory" ), "cwd", &Procinfo::cwd));
    categories.insert( F_ROOT, new Cat_dir( tr( "ROOT" ), tr( "Root directory of process" ), "root", &Procinfo::root));
    // command_line="COMMAND_LINE";	//reference to /proc/1234/cmdline
    categories.insert( F_CMDLINE,
                      new Cat_cmdline( tr( "COMMAND_LINE" ), tr( "Command line that started the process") ) );

    commonPostInit();

    socks_current = false;
    usocks_current = false;

    Proc::init_static();
}

Proc::~Proc()
{
    if(hprocs)
    {
        qDeleteAll(hprocs->begin(), hprocs->end());
        hprocs->clear();
    }
    while (history.size() > 0)
        delete history.takeFirst();

    qDeleteAll(socks.begin(), socks.end());
    socks.clear();

    qDeleteAll(usocks.begin(), usocks.end());
    usocks.clear();

    qDeleteAll(categories.begin(), categories.end());
    categories.clear();
}

// COMMON for LINUX,SOLARIS
// Polling /proc/PID/*
void Proc::read_proc_all()
{
    DIR *d = opendir("/proc");
    struct dirent *e;

    while ((e = readdir(d)) != nullptr)
    {
        if (e->d_name[0] >= '0' and e->d_name[0] <= '9')
        { // good idea !
            int pid;
            Procinfo *pi = nullptr;

            // inline int x_atoi(const char *sstr);
            // pid=x_atoi(e->d_name);	//if(pid<100) continue;
            pid = atoi(e->d_name);

            pi = procs.value(pid, NULL);

            if (pi == nullptr) // new process
            {
                pi = new Procinfo(this, pid);
                procs.insert(pid, pi);
                /*
                        Procinfo *parent;
                        parent =procs[pi->ppid];
                        if(parent)
                                parent->children->add(pi);
                        printf("Qps : parent null
                   pid[%d]\n",pi->pid);	}
                */
            }
            int ret = pi->readproc();
            if (ret > 0)
            {
                pi->generation = current_gen; // this process is alive
                // printf(" [%s] %d
                // %d\n",pi->command.toAscii().data(),pi->generation,current_gen);

                if (flag_show_thread and flag_thread_ok)
                    read_pid_tasks(pid); // for threads

                // add to History expect thread
                if (ret == 2)
                {
                    Procinfo *p = new Procinfo(*pi); // copy
                    p->clone = true;
                    hprocs->insert(pid, p);
                }
            }
            else
            {
                // already gone.  /proc/PID dead!
                // later remove this process ! not yet
            }
        }
    }
    closedir(d);
}

// COMMON , redesign
Proclist Proc::getHistory(int pos)
{
    Proclist l;
    if (pos <= 0)
    {
        return l;
    }
    int size = history.size();
    if (size > pos)
        l = history[size - pos]->procs;
    return l;
}

void Proc::setHistory(int tick)
{
    return;
    if (tick <= 0)
    {
        mprocs = nullptr;
        return;
    }
    int size = history.size();
    if (size > tick)
        mprocs = &history[size - tick]->procs;
    else
        mprocs = nullptr;
}

bool Procinfo::isThread()
{
    return pid != tgid; // how to check
}

// LINUX  size=64
int Procview::custom_fields[] = {F_PID,   F_TTY,  F_USER,    F_NICE,
                                 F_SIZE,  F_MEM,  F_STAT,    F_CPU,
                                 F_START, F_TIME, F_CMDLINE, F_END};

// COMMON: basic field
int Procview::basic_fields[] = {F_PID,  F_TTY,     F_USER, F_CPUNUM,
                                F_STAT, F_MEM,     F_CPU,  F_START,
                                F_TIME, F_CMDLINE, F_END};

int Procview::jobs_fields[] = {F_PID,  F_TGID,    F_PPID, F_PGID,
                               F_SID,  F_TPGID,   F_STAT, F_UID,
                               F_TIME, F_CMDLINE, F_END};

int Procview::mem_fields[] = {F_PID,     F_TTY, F_MAJFLT, F_MINFLT, F_SIZE,
                              F_RSS,     F_TRS, F_DRS,    F_STACK,  F_SHARE,
                              //	F_DT,
                              F_CMDLINE, F_END};

int Procview::sched_fields[] = {F_PID,     F_TGID,   F_NLWP, F_STAT,  F_FLAGS,
                                F_PLCY,    F_PRI,    F_NICE, F_TMS,   F_SLPAVG,
                                F_RPRI,    F_AFFCPU, F_CPU,  F_START, F_TIME,
                                F_CMDLINE, F_END};

void Procview::set_fields()
{
    switch (viewfields)
    {
    case USER: // BASIC FIELD
        set_fields_list(basic_fields);
        break;
    case JOBS:
        set_fields_list(jobs_fields);
        break;
    case MEM:
        set_fields_list(mem_fields);
        break;
    case SCHED:
        set_fields_list(sched_fields);
        break;
    case CUSTOM:
        set_fields_list(custom_fields);
        break;
    default:
        printf("Error ? set_fields_list \n");
    }
    fieldArrange();
}

// move to Proc.cpp
int get_kernel_version()
{
    int version = 0;
    char *p;
    struct utsname uname_info;
    if (uname(&uname_info) == 0) // man -S 2 uname
    {
        // printf("sysname =%s \n",uname_info.sysname);
        if (strcasecmp(uname_info.sysname, "linux") == 0)
        {
            Q_UNUSED(uname_info.release[0]);
        }
        p = uname_info.release;
        int major, minor, patch;
        int result;

        result = sscanf(p, "%d.%d.%d", &major, &minor, &patch);
        if (result == 2)
        {
            // only read two value
            patch = 0; // ex) 3.0-ARCH
        }
        else if (result < 3)
        {
            fprintf(stderr, "Qps: can't determine version, read %s \n", p);
            fprintf(stderr, "please report this bug.\n");
            exit(1);
        }
        version = major * 10000 + minor * 100 + patch;
        // ex) 2.6.17 == 20617 , 2.4.8  == 20408
        printf("DEBUG: version = %d\n", version);
    }
    else
    {
        fprintf(stderr, "Qps: uname() failed. (%d) \n", version);
        fprintf(stderr, "please report this bug.\n");
        exit(1);
    }
    return version;
}

void check_system_requirement()
{
    int kernel_version = 0;
    kernel_version = get_kernel_version();
    if (kernel_version < 20600) // less 2.6
    {
        printf("Qps: kernel 2.4.x not supported !!!\n\n"); // because of
                                                           // 2.4.x SMP
                                                           // bugs
        exit(0);
    }
}
