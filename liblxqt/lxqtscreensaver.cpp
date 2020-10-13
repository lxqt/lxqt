/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
 * Copyright (c) 2016 Luís Pereira <luis.artur.pereira@gmail.com>
 * Copyright (c) 2012 The Chromium Authors. All rights reserved.
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

#include <QProcess>
#include <QSettings>
#include "lxqtscreensaver.h"
#include "lxqttranslator.h"

#include <memory>

#include <XdgIcon>
#include <QMessageBox>
#include <QAction>
#include <QPointer>
#include <QProcess>
#include <QCoreApplication> // for Q_DECLARE_TR_FUNCTIONS
#include <QX11Info>
#include <QDebug>

#include <X11/extensions/scrnsaver.h>

// Avoid polluting everything with X11/Xlib.h:
typedef struct _XDisplay Display;

typedef unsigned long XAtom;
typedef unsigned long XID;

extern "C" {
int XFree(void*);
}

template <class T, class R, R (*F)(T*)>
struct XObjectDeleter {
  inline void operator()(void* ptr) const { F(static_cast<T*>(ptr)); }
};

template <class T, class D = XObjectDeleter<void, int, XFree>>
using XScopedPtr = std::unique_ptr<T, D>;


namespace LXQt {

static bool GetProperty(XID window, const std::string& property_name, long max_length,
                 Atom* type, int* format, unsigned long* num_items,
                 unsigned char** property);


static bool GetIntArrayProperty(XID window,
                         const std::string& property_name,
                         std::vector<int>* value);


static bool GetProperty(XID window, const std::string& property_name, long max_length,
                 Atom* type, int* format, unsigned long* num_items,
                 unsigned char** property)
{
    Atom property_atom =  XInternAtom(QX11Info::display(), property_name.c_str(), false);
    unsigned long remaining_bytes = 0;
    return XGetWindowProperty(QX11Info::display(),
                              window,
                              property_atom,
                              0,          // offset into property data to read
                              max_length, // max length to get
                              False,      // deleted
                              AnyPropertyType,
                              type,
                              format,
                              num_items,
                              &remaining_bytes,
                              property);
}

static bool GetIntArrayProperty(XID window,
                         const std::string& property_name,
                         std::vector<int>* value)
{
    Atom type = None;
    int format = 0;  // size in bits of each item in 'property'
    unsigned long num_items = 0;
    unsigned char* properties = nullptr;

    int result = GetProperty(window, property_name,
                           (~0L), // (all of them)
                           &type, &format, &num_items, &properties);
    XScopedPtr<unsigned char> scoped_properties(properties);
    if (result != Success)
        return false;

    if (format != 32)
        return false;

    long* int_properties = reinterpret_cast<long*>(properties);
    value->clear();
    for (unsigned long i = 0; i < num_items; ++i)
    {
        value->push_back(static_cast<int>(int_properties[i]));
    }
    return true;
}

class ScreenSaverPrivate
{
    Q_DECLARE_TR_FUNCTIONS(LXQt::ScreenSaver)
    Q_DECLARE_PUBLIC(ScreenSaver)
    ScreenSaver* const q_ptr;

public:
    ScreenSaverPrivate(ScreenSaver *q) : q_ptr(q) {
        QSettings settings(QSettings::UserScope, QLatin1String("lxqt"), QLatin1String("lxqt"));

        settings.beginGroup(QLatin1String("Screensaver"));
        lock_command = settings.value(QLatin1String("lock_command"), QLatin1String("xdg-screensaver lock")).toString();
        settings.endGroup();
    }

    void reportLockProcessError();
    void _l_lockProcess_finished(int, QProcess::ExitStatus);
    void _l_lockProcess_errorOccurred(QProcess::ProcessError);
    bool isScreenSaverLocked();

    QPointer<QProcess> m_lockProcess;

    QString lock_command;
};

void ScreenSaverPrivate::reportLockProcessError()
{
    QMessageBox box;
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Screen Saver Error"));
    QString message;
    // contains() instead of startsWith() as the command might be "env FOO=bar xdg-screensaver lock"
    // (e.g., overwrite $XDG_CURRENT_DESKTOP for some different behaviors)
    if (lock_command.contains(QLatin1String("xdg-screensaver"))) {
        message = tr("Failed to run  \"%1\". "
                     "Ensure you have a locker/screensaver compatible with xdg-screensaver installed and running."
                    );
    } else {
        message = tr("Failed to run  \"%1\". "
                     "Ensure the specified locker/screensaver is installed and running."
                    );
    }
    box.setText(message.arg(lock_command));
    box.exec();
}

void ScreenSaverPrivate::_l_lockProcess_finished(int err, QProcess::ExitStatus status)
{
    Q_UNUSED(status)
    Q_Q(ScreenSaver);
    if (err == 0)
        Q_EMIT q->activated();
    else
    {
        reportLockProcessError();
    }

    Q_EMIT q->done();
}

void ScreenSaverPrivate::_l_lockProcess_errorOccurred(QProcess::ProcessError)
{
    reportLockProcessError();
}

bool ScreenSaverPrivate::isScreenSaverLocked()
{
    XScreenSaverInfo *info = nullptr;
    Display *display = QX11Info::display();
    XID window = DefaultRootWindow(display);
    info = XScreenSaverAllocInfo();

    XScreenSaverQueryInfo(QX11Info::display(), window, info);
    const int state = info->state;
    XFree(info);
    if (state == ScreenSaverOn)
        return true;

    // Ironically, xscreensaver does not conform to the XScreenSaver protocol, so
    // info.state == ScreenSaverOff or info.state == ScreenSaverDisabled does not
    // necessarily mean that a screensaver is not active, so add a special check
    // for xscreensaver.
    XAtom lock_atom = XInternAtom(display, "LOCK", false);
    std::vector<int> atom_properties;
    if (GetIntArrayProperty(window, "_SCREENSAVER_STATUS", &atom_properties) &&
        atom_properties.size() > 0)
    {
        if (atom_properties[0] == static_cast<int>(lock_atom))
            return true;
    }

    return false;
}

ScreenSaver::ScreenSaver(QObject * parent)
    : QObject(parent),
      d_ptr(new ScreenSaverPrivate(this))
{
    Q_D(ScreenSaver);
    d->m_lockProcess = new QProcess(this);
    connect(d->m_lockProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus){ d->_l_lockProcess_finished(exitCode, exitStatus); });
    connect(d->m_lockProcess, &QProcess::errorOccurred,
        [=](QProcess::ProcessError error) { d->_l_lockProcess_errorOccurred(error); });
}

ScreenSaver::~ScreenSaver()
{
    delete d_ptr;
}

QList<QAction*> ScreenSaver::availableActions()
{
    QList<QAction*> ret;
    QAction * act;

    act = new QAction(XdgIcon::fromTheme(QL1S("system-lock-screen"), QL1S("lock")), tr("Lock Screen"), this);
    connect(act, &QAction::triggered, this, &ScreenSaver::lockScreen);
    ret.append(act);

    return ret;
}

void ScreenSaver::lockScreen()
{
    Q_D(ScreenSaver);
    if (!d->isScreenSaverLocked()) {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        d->m_lockProcess->start(d->lock_command);
#else
        QStringList args = QProcess::splitCommand(d->lock_command);
        if (args.isEmpty()) {
            qWarning() << Q_FUNC_INFO << "Empty screen lock_command";
            return;
        }
        const QString program = args.takeFirst();
        d->m_lockProcess->start(program, args, QIODevice::ReadWrite);
#endif
    }
}

} // namespace LXQt

#include "moc_lxqtscreensaver.cpp"
