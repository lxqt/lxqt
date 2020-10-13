/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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


#ifndef LXQTPANELAPPLICATION_H
#define LXQTPANELAPPLICATION_H

#include <LXQt/Application>
#include "ilxqtpanelplugin.h"

class QScreen;

class LXQtPanel;
class LXQtPanelApplicationPrivate;

/*!
 * \brief The LXQtPanelApplication class inherits from LXQt::Application and
 * is therefore the QApplication that we will create and execute in our
 * main()-function.
 *
 * LXQtPanelApplication itself is not a visible panel, rather it is only
 * the container which holds the visible panels. These visible panels are
 * LXQtPanel objects which are stored in mPanels. This approach enables us
 * to have more than one panel (for example one panel at the top and one
 * panel at the bottom of the screen) without additional effort.
 */
class LXQtPanelApplication : public LXQt::Application
{
    Q_OBJECT
public:
    /*!
     * \brief Creates a new LXQtPanelApplication with the given command line
     * arguments. Performs the following steps:
     * 1. Initializes the LXQt::Application, sets application name and version.
     * 2. Handles command line arguments. Currently, the only cmdline argument
     * is -c = -config = -configfile which chooses a different config file
     * for the LXQt::Settings.
     * 3. Creates the LXQt::Settings.
     * 4. Connects QCoreApplication::aboutToQuit to cleanup().
     * 5. Calls addPanel() for each panel found in the config file. If there is
     * none, adds a new panel.
     * \param argc
     * \param argv
     */
    explicit LXQtPanelApplication(int& argc, char** argv);
    ~LXQtPanelApplication();

    void setIconTheme(const QString &iconTheme);

    /*!
     * \brief Determines the number of LXQtPanel objects
     * \return the current number of LXQtPanel objects
     */
    int count() const { return mPanels.count(); }

    /*!
     * \brief Checks if a given Plugin is running and has the
     * ILXQtPanelPlugin::SingleInstance flag set. As Plugins are added to
     * LXQtPanel instances, this method only iterates over these LXQtPanel
     * instances and lets them check the conditions.
     * \param pluginId Plugin Identifier which is the basename of the .desktop
     * file that specifies the plugin.
     * \return true if the Plugin is running and has the
     * ILXQtPanelPlugin::SingleInstance flag set, false otherwise.
     */
    bool isPluginSingletonAndRunnig(QString const & pluginId) const;

public slots:
    /*!
     * \brief Adds a new LXQtPanel which consists of the following steps:
     * 1. Create id/name.
     * 2. Create the LXQtPanel: call addPanel(name).
     * 3. Update the config file (add the new panel id to the list of panels).
     * 4. Show the panel configuration dialog so that the user can add plugins.
     *
     * This method will create a new LXQtPanel with a new name and add this
     * to the config file. So this should only be used while the application
     * is running and the user decides to add a new panel. At application
     * startup, addPanel() should be used instead.
     *
     * \note This slot will be used from the LXQtPanel right-click menu. As we
     * can only add new panels from a visible panel, we should never run
     * lxqt-panel without an LXQtPanel. Without a panel, we have just an
     * invisible application.
     */
    void addNewPanel();

signals:
    /*!
     * \brief Signal that re-emits the signal pluginAdded() from LXQtPanel.
     */
    void pluginAdded();
    /*!
     * \brief Signal that re-emits the signal pluginRemoved() from LXQtPanel.
     */
    void pluginRemoved();

private:
    /*!
     * \brief Holds all the instances of LXQtPanel.
     */
    QList<LXQtPanel*> mPanels;
    /*!
     * \brief The global icon theme used by all apps (except for panels perhaps).
     */
    QString mGlobalIconTheme;
    /*!
     * \brief Creates a new LXQtPanel with the given name and connects the
     * appropriate signals and slots.
     * This method can be used at application startup.
     * \param name Name of the LXQtPanel as it is used in the config file.
     * \return The newly created LXQtPanel.
     */
    LXQtPanel* addPanel(const QString &name);

private slots:
    /*!
     * \brief Removes the given LXQtPanel which consists of the following
     * steps:
     * 1. Remove the panel from mPanels.
     * 2. Remove the panel from the config file.
     * 3. Schedule the QObject for deletion: QObject::deleteLater().
     * \param panel LXQtPanel instance that should be removed.
     */
    void removePanel(LXQtPanel* panel);

    /*!
     * \brief Connects the QScreen::destroyed signal of a new screen to
     * the screenDestroyed() slot so that we can handle this screens'
     * destruction as soon as it happens.
     * \param newScreen The QScreen that was created and added.
     */
    void handleScreenAdded(QScreen* newScreen);
    /*!
     * \brief Handles screen destruction. This is a workaround for a Qt bug.
     * For further information, see the implementation notes.
     * \param screenObj The QScreen that was destroyed.
     */
    void screenDestroyed(QObject* screenObj);
    /*!
     * \brief Reloads the panels. This is the second part of the workaround
     * mentioned above.
     */
    void reloadPanelsAsNeeded();
    /*!
     * \brief Deletes all LXQtPanel instances that are stored in mPanels.
     */
    void cleanup();

private:
    /*!
     * \brief mSettings is the LXQt::Settings object that is used for the
     * current instance of lxqt-panel. Normally, this refers to the config file
     * $HOME/.config/lxqt/panel.conf (on Unix systems). This behaviour can be
     * changed with the -c command line option.
     */

    LXQtPanelApplicationPrivate *const d_ptr;

    Q_DECLARE_PRIVATE(LXQtPanelApplication)
    Q_DISABLE_COPY(LXQtPanelApplication)
};


#endif // LXQTPANELAPPLICATION_H
