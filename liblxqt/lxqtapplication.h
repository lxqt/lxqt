/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012-2013 Razor team
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

#ifndef LXQTAPPLICATION_H
#define LXQTAPPLICATION_H

#include <QApplication>
#include <QProxyStyle>
#include "lxqtglobals.h"

namespace LXQt
{

/*! \brief LXQt wrapper around QApplication.
 * It loads various LXQt related stuff by default (window icon, icon theme...)
 *
 * \note This wrapper is intended to be used only inside LXQt project. Using it
 *       in external application will automatically require linking to various
 *       LXQt libraries.
 *
 */
class LXQT_API Application : public QApplication
{
    Q_OBJECT

public:
    /*! Construct a LXQt application object.
     * \param argc standard argc as in QApplication
     * \param argv standard argv as in QApplication
     */
    Application(int &argc, char **argv);
    /*! Construct a LXQt application object.
     * \param argc standard argc as in QApplication
     * \param argv standard argv as in QApplication
     * \param handleQuitSignals flag if signals SIGINT, SIGTERM, SIGHUP should be handled internaly (\sa quit() application)
     */
    Application(int &argc, char **argv, bool handleQuitSignals);
    ~Application() override {}
    /*! Install UNIX signal handler for signals defined in \param signalList
     * Upon receiving of any of this signals the \sa unixSignal signal is emitted
     */
    void listenToUnixSignals(QList<int> const & signolList);

private Q_SLOTS:
    void updateTheme();

Q_SIGNALS:
    void themeChanged();
    /*! Signal is emitted upon receival of registered unix signal
     * \param signo the received unix signal number
     */
    void unixSignal(int signo);
};

#if defined(lxqtApp)
#undef lxqtApp
#endif
#define lxqtApp (static_cast<LXQt::Application *>(qApp))

} // namespace LXQt
#endif // LXQTAPPLICATION_H
