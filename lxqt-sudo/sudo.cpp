/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015-2018 LXQt team
 * Authors:
 *   Palo Kisa <palo.kisa@gmail.com>
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

#include "sudo.h"
#include "passworddialog.h"

#include <LXQt/Application>

#include <QTextStream>
#include <QMessageBox>
#include <QFileInfo>
#include <QSocketNotifier>
#include <QDebug>
#include <QThread>
#include <QProcessEnvironment>
#include <QTimer>
#include <QRegularExpression>
#if defined(Q_OS_LINUX)
#include <pty.h>
#else
#include <errno.h>
#include <termios.h>
#include <util.h>
#endif
#include <unistd.h>
#include <memory>
#include <csignal>
#include <sys/wait.h>
#include <fcntl.h>
#include <iostream>
#include <thread>
#include <sstream>

namespace
{
    const QString app_master{QStringLiteral(LXQTSUDO)};
    const QString app_version{QStringLiteral(LXQT_VERSION)};
    const QString app_lxsu{QStringLiteral(LXQTSUDO_LXSU)};
    const QString app_lxsudo{QStringLiteral(LXQTSUDO_LXSUDO)};

    const QString su_prog{QStringLiteral(LXQTSUDO_SU)};
    const QString sudo_prog{QStringLiteral(LXQTSUDO_SUDO)};
    const QString pwd_prompt_end{QStringLiteral(": ")};
    const QChar nl{QLatin1Char('\n')};

    void usage(QString const & err = QString())
    {
        if (!err.isEmpty())
            QTextStream(stderr) << err << '\n';
        QTextStream(stdout)
            << QObject::tr("Usage: %1 option [command [arguments...]]\n\n"
                    "GUI frontend for %2/%3\n\n"
                    "Arguments:\n"
                    "  option:\n"
                    "    -h|--help      Print this help.\n"
                    "    -v|--version   Print version information.\n"
                    "    -s|--su        Use %3(1) as backend.\n"
                    "    -d|--sudo      Use %2(8) as backend.\n"
                    "  command          Command to run.\n"
                    "  arguments        Optional arguments for command.\n\n").arg(app_master).arg(sudo_prog).arg(su_prog);
        if (!err.isEmpty())
            QMessageBox(QMessageBox::Critical, app_master, err, QMessageBox::Ok).exec();
    }

    void version()
    {
        QTextStream(stdout)
            << QObject::tr("%1 version %2\n").arg(app_master).arg(app_version);
    }

    //Note: array must be sorted to allow usage of binary search
    static constexpr char const * const ALLOWED_VARS[] = {
        "DISPLAY"
            , "LANG", "LANGUAGE", "LC_ADDRESS", "LC_ALL", "LC_COLLATE", "LC_CTYPE", "LC_IDENTIFICATION", "LC_MEASUREMENT"
            , "LC_MESSAGES", "LC_MONETARY", "LC_NAME", "LC_NUMERIC", "LC_PAPER", "LC_TELEPHONE", "LC_TIME"
            , "PATH", "QT_PLATFORM_PLUGIN", "QT_QPA_PLATFORMTHEME", "TERM", "WAYLAND_DISPLAY", "XAUTHLOCALHOSTNAME", "XAUTHORITY"
    };
    static constexpr char const * const * const ALLOWED_END = ALLOWED_VARS + sizeof (ALLOWED_VARS) / sizeof (ALLOWED_VARS[0]);
    struct assert_helper
    {
        assert_helper()
        {
            Q_ASSERT(std::is_sorted(ALLOWED_VARS, ALLOWED_END
                        , [] (char const * const a, char const * const b) { return strcmp(a, b) < 0; }));
        }
    };
    assert_helper h;

    inline std::string env_workarounds()
    {
        std::cerr << LXQTSUDO << ": Stripping child environment except for: ";
        std::ostringstream left_env_params;
        std::copy(ALLOWED_VARS, ALLOWED_END - 1, std::ostream_iterator<const char *>{left_env_params, ","});
        left_env_params << *(ALLOWED_END - 1); // printing the last separately to avoid trailing comma
        std::cerr << left_env_params.str() << '\n';
        // cleanup environment, because e.g.:
        // - pcmanfm-qt will not start if the DBUS_SESSION_BUS_ADDRESS is preserved
        // - Qt apps may change user's config files permissions if the XDG_* are preserved
        for (auto const & key : QProcessEnvironment::systemEnvironment().keys())
        {
            auto const & i = std::lower_bound(ALLOWED_VARS, ALLOWED_END, key, [] (char const * const a, QString const & b) {
                    return b > QLatin1String(a);
                    });
            if (i == ALLOWED_END || key != QLatin1String(*i))
            {
                unsetenv(key.toLatin1().data());
            }
        }
        return left_env_params.str();
    }

    inline QString quoteShellArg(const QString& arg, bool userFriendly)
    {
        QString rv = arg;

        //^ check if thre are any bash special file characters
        if (!userFriendly || arg.contains(QRegularExpression(QStringLiteral("(\\s|[][!\"#$&'()*,;<=>?\\^`{}|~])")))) {
            rv.replace(QStringLiteral("'"), QStringLiteral("'\\''"));
            rv.prepend (QLatin1Char('\'')).append(QLatin1Char('\''));
        }

        return rv;
    }
}

Sudo::Sudo()
    : mArgs{lxqtApp->arguments()}
    , mBackend{BACK_NONE}
{
    QString cmd = QFileInfo(mArgs[0]).fileName();
    mArgs.removeAt(0);
    if (app_lxsu == cmd)
        mBackend = BACK_SU;
    else if (app_lxsudo == cmd || app_master == cmd)
        mBackend = BACK_SUDO;
    mRet = mPwdFd = mChildPid = 0;
}

Sudo::~Sudo()
{
}

int Sudo::main()
{
    if (0 < mArgs.size())
    {
        //simple option check
        QString const & arg1 = mArgs[0];
        if (QStringLiteral("-h") == arg1 || QStringLiteral("--help") == arg1)
        {
            usage();
            return 0;
        } else if (QStringLiteral("-v") == arg1 || QStringLiteral("--version") == arg1)
        {
            version();
            return 0;
        } else if (QStringLiteral("-s") == arg1 || QStringLiteral("--su") == arg1)
        {
            mBackend = BACK_SU;
            mArgs.removeAt(0);
        } else if (QStringLiteral("-d") == arg1 || QStringLiteral("--sudo") == arg1)
        {
            mBackend = BACK_SUDO;
            mArgs.removeAt(0);
        }
    }
    //any other arguments we simply forward to su/sudo

    if (1 > mArgs.size())
    {
        usage(tr("%1: no command to run provided!").arg(app_master));
        return 1;
    }

    if (BACK_NONE == mBackend)
    {
        //we were invoked through unknown link (or renamed binary)
        usage(tr("%1: no backend chosen!").arg(app_master));
        return 1;
    }

    mChildPid = forkpty(&mPwdFd, nullptr, nullptr, nullptr);
    if (0 == mChildPid)
    {
        child(); // never returns
        return 1; // but for sure
    }

    mDlg.reset(new PasswordDialog{squashedArgs(/*userFriendly = */ true), backendName()});
    mDlg->setModal(true);
    lxqtApp->setActiveWindow(mDlg.data());

    if (-1 == mChildPid)
        QMessageBox(QMessageBox::Critical, mDlg->windowTitle()
                , tr("Syscall error, failed to fork: %1").arg(QString::fromUtf8(strerror(errno))), QMessageBox::Ok).exec();
    else
        return parent();

    return 1;
}

QString Sudo::squashedArgs(bool userFriendly) const
{
    QString rv;

    rv = quoteShellArg (mArgs[0], userFriendly);
    for (auto argP = ++mArgs.begin(); argP != mArgs.end(); ++argP) {
        rv.append (QLatin1Char(' ')).append(quoteShellArg (*argP, userFriendly));
    }

    return rv;
}

QString Sudo::backendName (backend_t backEnd)
{
    QString rv;
    // Remove leading paths in case variables are set with full path
    switch (backEnd) {
        case BACK_SU   : rv = su_prog;   break;
        case BACK_SUDO : rv = sudo_prog; break;
        //: shouldn't be actually used but keep as short as possible in translations just in case.
        case BACK_NONE : rv = tr("unset");
    }

    return rv;
}

void Sudo::child()
{
    int params_cnt = 3 //1. su/sudo & "shell command" & last nullptr
        + (BACK_SU == mBackend ? 1 : 3); //-c for su | -E /bin/sh -c for sudo
    std::unique_ptr<char const *[]> params{new char const *[params_cnt]};
    const char ** param_arg = params.get() + 1;

    std::string program = backendName().toLocal8Bit().data();

    std::string preserve_env_param;
    switch (mBackend)
    {
        case BACK_SUDO:
            preserve_env_param = "--preserve-env=";

            preserve_env_param += env_workarounds();

            *(param_arg++) = preserve_env_param.c_str(); //preserve environment
            *(param_arg++) = "/bin/sh";
            break;
        case BACK_SU:
        case BACK_NONE:
            env_workarounds();
            break;

    }
    *(param_arg++) = "-c"; //run command

    params[0] = program.c_str();

    // Note: we force the su/sudo to communicate with us in the simplest
    // locale and then set the locale back for the command
    char const * const env_lc_all = getenv("LC_ALL");
    std::string command;
    if (env_lc_all == nullptr)
    {
        command = "unset LC_ALL; ";
    } else
    {
        // Note: we need to check if someone is not trying to inject commands
        // for privileged execution via the LC_ALL
        if (nullptr != strchr(env_lc_all, '\''))
        {
            QTextStream{stderr, QIODevice::WriteOnly} << tr("%1: Detected attempt to inject privileged command via LC_ALL env(%2). Exiting!\n").arg(app_master).arg(QString::fromUtf8(env_lc_all));
            exit(1);
        }
        command = "LC_ALL='";
        command += env_lc_all;
        command += "' ";
    }
    command += "exec ";
    command += squashedArgs().toLocal8Bit().data();
    *(param_arg++) = command.c_str();

    *param_arg = nullptr;

    setenv("LC_ALL", "C", 1);

    setsid(); //session leader
    execvp(params[0], const_cast<char **>(params.get()));

    //exec never returns in case of success
    QTextStream{stderr, QIODevice::WriteOnly} << tr("%1: Failed to exec '%2': %3\n").arg(app_master).arg(QString::fromUtf8(params[0])).arg(QString::fromUtf8(strerror(errno)));
    exit(1);
}

void Sudo::stopChild()
{
    kill(mChildPid, SIGINT);
    int res, status;
    for (int cnt = 10; 0 == (res = waitpid(mChildPid, &status, WNOHANG)) && 0 < cnt; --cnt)
        QThread::msleep(100);

    if (0 == res)
    {
        kill(mChildPid, SIGKILL);
    }
}

int Sudo::parent()
{
    //set the FD as non-blocking
    if (0 != fcntl(mPwdFd, F_SETFL, O_NONBLOCK))
    {
        QMessageBox(QMessageBox::Critical, mDlg->windowTitle()
                , tr("Syscall error, failed to bring pty to non-block mode: %1").arg(QString::fromUtf8(strerror(errno))), QMessageBox::Ok).exec();
        return 1;
    }

    FILE * pwd_f = fdopen(mPwdFd, "r+");
    if (nullptr == pwd_f)
    {
        QMessageBox(QMessageBox::Critical, mDlg->windowTitle()
                , tr("Syscall error, failed to fdopen pty: %1").arg(QString::fromUtf8(strerror(errno))), QMessageBox::Ok).exec();
        return 1;
    }

    QTextStream child_str{pwd_f};

    QObject::connect(mDlg.data(), &QDialog::finished, [&] (int result)
        {
            if (QDialog::Accepted == result)
            {
                child_str << mDlg->password().append(nl);
                child_str.flush();
            } else
            {
                stopChild();
            }
        });

    QString last_line;
    QScopedPointer<QSocketNotifier> pwd_watcher{new QSocketNotifier{mPwdFd, QSocketNotifier::Read}};
    auto reader = [&]
        {
            QString line = child_str.readAll();
            if (line.isEmpty())
            {
                pwd_watcher.reset(nullptr); //stop the notifications events

                QString const & prog = backendName();
                if (last_line.startsWith(QStringLiteral("%1:").arg(prog)))
                {
                    QMessageBox(QMessageBox::Critical, mDlg->windowTitle()
                            , tr("Child '%1' process failed!\n%2").arg(prog).arg(last_line), QMessageBox::Ok).exec();
                }
            } else
            {
                if (line.endsWith(pwd_prompt_end))
                {
                    //if now echo is turned off, su/sudo requests password
                    struct termios tios;
                    //loop to be sure we don't miss the flag (we can afford such small delay in "normal" output processing)
                    for (size_t cnt = 10; 0 < cnt && 0 == tcgetattr(mPwdFd, &tios) && (ECHO & tios.c_lflag); --cnt)
                        QThread::msleep(10);
                    if (!(ECHO & tios.c_lflag))
                    {
                        mDlg->show();
                        return;
                    }
                }
                QTextStream{stderr, QIODevice::WriteOnly} << line;
                //assuming text oriented output
                QStringList lines = line.split(nl, QString::SkipEmptyParts);
                last_line = lines.isEmpty() ? QString() : lines.back();
            }

        };

    QObject::connect(pwd_watcher.data(), &QSocketNotifier::activated, reader);

    std::unique_ptr<std::thread> child_waiter;
    QTimer::singleShot(0, [&child_waiter, this] {
            child_waiter.reset(new std::thread{[this] {
                    int res, status;
                    res = waitpid(mChildPid, &status, 0);
                    mRet = (mChildPid == res && WIFEXITED(status)) ? WEXITSTATUS(status) : 1;
                    lxqtApp->quit();
                }
            });
        });

    lxqtApp->exec();

    child_waiter->join();

    // try to read the last line(s)
    reader();

    fclose(pwd_f);
    close(mPwdFd);

    return mRet;
}

