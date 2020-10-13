/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, deskop environment
 * https://lxqt.org/
 *
 * Copyright: 2010-2011 LXQt team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#ifndef LXQTMODMAN_H
#define LXQTMODMAN_H

#include <QAbstractNativeEventFilter>
#include <QProcess>
#include <QList>
#include <QMap>
#include <QTimer>
#include <XdgDesktopFile>
#include <QEventLoop>
#include <time.h>

class LXQtModule;
namespace LXQt {
class Settings;
}
class QFileSystemWatcher;

typedef QMap<QString,LXQtModule*> ModulesMap;
typedef QMapIterator<QString,LXQtModule*> ModulesMapIterator;
typedef QList<time_t> ModuleCrashReport;
typedef QMap<QProcess*, ModuleCrashReport> ModulesCrashReport;

/*! \brief LXQtModuleManager manages the processes of the session
and which modules of lxqt are about to load.

LXQtModuleManager handles the session management (logout/restart/shutdown)
as well.

Also it watches the current theme to react if it was removed or modified.

Processes in LXQtModuleManager are started as follows:
 - run lxqt-confupdate
 - start the window manager and wait until it's active
 - start all normal autostart items (including LXQt modules)
 - if there are any applications that need a system tray, wait until a system tray
   implementation becomes active, and then start those

Potential process recovering is done in \see restartModules()
*/

class LXQtModuleManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    //! \brief Construct LXQtModuleManager
    LXQtModuleManager(QObject* parent = nullptr);
    ~LXQtModuleManager() override;

    //! \brief Set the window manager (e.g. "/usr/bin/openbox")
    void setWindowManager(const QString & windowManager);

    //! \brief Start a module given its file name (e.g. "lxqt-panel.desktop")
    void startProcess(const QString& name);

    //! \brief Stop a running module
    void stopProcess(const QString& name);

    //! \brief List the running modules, identified by their file names
    QStringList listModules() const;

    //! \brief Read configuration and start processes
    void startup(LXQt::Settings& s);

    // Qt5 uses native event filter
    bool nativeEventFilter(const QByteArray & eventType, void * message, long * result) override;

public slots:
    /*! \brief Exit LXQt session.
    It tries to terminate processes from procMap and autostartList
    gracefully (to kill it if it is not possible). Then the session
    exits - it returns to the kdm/gdm in most cases.
    */
    void logout(bool doExit);

signals:
    void moduleStateChanged(QString moduleName, bool state);

private:
    //! \brief Start Window Manager
    void startWm(LXQt::Settings *settings);
    void wmStarted();

    void startAutostartApps();

    //! \brief Show Window Manager select dialog
    QString showWmSelectDialog();

    //! \brief Start a process described in a desktop file
    void startProcess(const XdgDesktopFile &file);

    //! \brief Start the lxqt-confupdate.
    void startConfUpdate();

    //! \brief Window manager command
    QString mWindowManager;

    //! \brief map file names to module processes
    ModulesMap mNameMap;

    //! \brief the window manager
    QProcess* mWmProcess;

    /*! \brief Keep creashes for given process to raise a message in the
        case of repeating crashes
     */
    ModulesCrashReport mCrashReport;

    //! \brief file system watcher to react on theme modifications
    QFileSystemWatcher *mThemeWatcher;
    QString mCurrentThemePath;

    bool mWmStarted;
    bool mTrayStarted;
    QEventLoop* mWaitLoop;

private slots:

    /*! \brief this slot is called by the QProcesses if they end.
    \warning The slot *has* to be called as a slot only due sender() cast.

    There are 2 types of handling in this slot.

    If the process has ended correctly (no crash or kill) nothing happens
    except when the process is *not* specified with doespower in the
    configuration. If there is no "doespower" (window manager mainly)
    entire session performs logout. (Handling of the window manager 3rd
    party "logout")

    If the process crashed and is set as "doespower" it's tried to
    be restarted automatically.
    */
    void restartModules(int exitCode, QProcess::ExitStatus exitStatus);

    /*! Clear m_crashReport after some amount of time
     */
    void resetCrashReport();

    void themeFolderChanged(const QString&);

    void themeChanged();
};


/*! \brief platform independent way how to set up an environment variable.
It sets env variable for all lxqt-session childs.
\param env a raw string variable name (PATH, TERM, ...)
\param value a QByteArray with the value. Variable will use this new value only
             - no appending/prepending is performed.
See lxqt_setenv_prepend.
*/
void lxqt_setenv(const char *env, const QByteArray &value);
/*! \brief Set up a environment variable with original value with new value prepending.
\param env a raw string with variable name
\param value a QByteArray value to be pre-pend to original content of the variable
\param separator an optional string with separator character. Eg. ":" for PATH.
See lxqt_setenv.
*/
void lxqt_setenv_prepend(const char *env, const QByteArray &value, const QByteArray &separator=":");

class LXQtModule : public QProcess
{
    Q_OBJECT
public:
    LXQtModule(const XdgDesktopFile& file, QObject *parent = nullptr);
    void start();
    void terminate();
    bool isTerminating();

    const XdgDesktopFile file;
    const QString fileName;

signals:
    void moduleStateChanged(QString name, bool state);

private slots:
    void updateState(QProcess::ProcessState newState);

private:
    bool mIsTerminating;
};

#endif
