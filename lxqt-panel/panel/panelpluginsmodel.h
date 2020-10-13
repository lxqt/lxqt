/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef PANELPLUGINSMODEL_H
#define PANELPLUGINSMODEL_H

#include <QAbstractListModel>
#include <memory>

namespace LXQt
{
    class PluginInfo;
    struct PluginData;
}

class LXQtPanel;
class Plugin;

/*!
 * \brief The PanelPluginsModel class implements the Model part of the
 * Qt Model/View architecture for the Plugins, i.e. it is the interface
 * to access the Plugin data associated with this Panel. The
 * PanelPluginsModel takes care for read-access as well as changes
 * like adding, removing or moving Plugins.
 */
class PanelPluginsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    PanelPluginsModel(LXQtPanel * panel,
                      QString const & namesKey,
                      QStringList const & desktopDirs,
                      QObject * parent = nullptr);
    ~PanelPluginsModel();

    /*!
     * \brief rowCount returns the number of Plugins. It overrides/implements
     * QAbstractListModel::rowCount().
     * \param parent The parameter parent should be omitted to get the number of
     * Plugins. If it is given and a valid model index, the method returns 0
     * because PanelPluginsModel is not a hierarchical model.
     */
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    /*!
     * \brief data returns the Plugin data as defined by the Model/View
     * architecture. The Plugins itself can be accessed with the role
     * Qt::UserRole but they can also be accessed by the methods plugins(),
     * pluginByName() and pluginByID(). This method overrides/implements
     * QAbstractListModel::data().
     * \param index should be a valid model index to determine the Plugin
     * that should be read.
     * \param role The Qt::ItemDataRole to determine what kind of data should
     * be read, can be one of the following:
     * 1. Qt::DisplayRole to return a string that describes the Plugin.
     * 2. Qt::DecorationRole to return an icon for the Plugin.
     * 3. Qt::UserRole to return a Plugin*.
     * \return The data as determined by index and role.
     */
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    /*!
     * \brief flags returns the item flags for the given model index. For
     * all Plugins, this is the same:
     * Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren.
     */
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;

    /*!
     * \brief pluginNames returns a list of names for all the Plugins in
     * this panel. The names are not the human-readable names but the names
     * that are used to identify the Plugins, e.g. in the config files. These
     * names can be used in the method pluginByName() to get a corresponding
     * Plugin.
     *
     * The plugin names are normally chosen to be equal to the
     * filename of the corresponding *.desktop-file. If multiple instances
     * of a single plugin-type are created, their names are created by
     * appending increasing numbers, e.g. 'mainmenu' and 'mainmenu2'.
     *
     * \sa findNewPluginSettingsGroup
     */
    QStringList pluginNames() const;
    /*!
     * \brief plugins returns a list of Plugins in this panel.
     */
    QList<Plugin *> plugins() const;
    /*!
     * \brief pluginByName gets a Plugin by its name.
     * \param name is the name of the plugin as it is used in the
     * config files. A list of names can be retrieved with the
     * method pluginNames().
     * \return the Plugin with the given name.
     *
     * \sa pluginNames
     */
    Plugin *pluginByName(QString name) const;
    /*!
     * \brief pluginByID gets a Plugin by its ID.
     * \param id is the *.desktop-file-ID of the plugin which in turn is the
     * QFileInfo::completeBaseName() of the desktop-file, e.g. "mainmenu".
     *
     * As these IDs are chosen according to the corresponding
     * desktop-file, these IDs are not unique. If multiple
     * instances of a single plugin-type are created, they share
     * the same ID in this sense. Then, this method will return
     * the first plugin of the given type.
     * \return the first Plugin found with the given ID.
     */
    Plugin const *pluginByID(QString id) const;

    /*!
     * \brief movePlugin moves a Plugin in the underlying data.
     *
     * This method is useful whenever a Plugin should be moved several
     * positions at once. If a Plugin should only be moved one position
     * up or down, consider using onMovePluginUp or onMovePluginDown.
     *
     * \param plugin Plugin that has been moved
     * \param nameAfter name of the Plugin that should be located after
     * the moved Plugin after the move operation, so this parameter
     * determines the new position of plugin. If an empty string is
     * given, plugin will be moved to the end of the list.
     *
     * \note This method is especially useful for drag and drop reordering.
     * Therefore, it will be called whenever the user moves a Plugin in
     * the panel ("Move Plugin" in the context menu of the panel).
     *
     * \sa onMovePluginUp, onMovePluginDown
     */
    void movePlugin(Plugin * plugin, QString const & nameAfter);

signals:
    /*!
     * \brief pluginAdded gets emitted whenever a new Plugin is added
     * to the panel.
     */
    void pluginAdded(Plugin * plugin);
    /*!
     * \brief pluginRemoved gets emitted whenever a Plugin is removed.
     * \param plugin The Plugin that was removed. This could be a nullptr.
     */
    void pluginRemoved(Plugin * plugin);
    /*!
     * \brief pluginMoved gets emitted whenever a Plugin is moved.
     *
     * This signal gets emitted in movePlugin, onMovePluginUp and
     * onMovePluginDown.
     *
     * \param plugin The Plugin that was moved. This could be a nullptr.
     *
     * \sa pluginMovedUp
     */
    void pluginMoved(Plugin * plugin); //plugin can be nullptr in case of move of not loaded plugin
    /*!
     * \brief pluginMovedUp gets emitted whenever a Plugin is moved a single
     * slot upwards.
     *
     * When a Plugin is moved a single slot upwards, this signal will be
     * emitted additionally to the pluginMoved signal so that two signals
     * get emitted.
     *
     * If a Plugin is moved downwards, that Plugin will swap places with
     * the following Plugin so that the result equals moving the following
     * Plugin a single slot upwards. So, whenever two adjacent Plugins
     * swap their places, this signal gets emitted with the Plugin that
     * moves upwards as parameter.
     *
     * For simplified use, only this signal is implemented. There is no
     * similar pluginMovedDown-signal.
     *
     * This signal gets emitted from onMovePluginUp and onMovePluginDown.
     *
     * \param plugin The Plugin that moved a slot upwards.
     *
     * \sa pluginMoved
     */
    void pluginMovedUp(Plugin * plugin);

public slots:
    /*!
     * \brief addPlugin Adds a new Plugin to the model.
     *
     * \param desktopFile The PluginInfo (which inherits XdgDesktopFile)
     * for the Plugin that should be added.
     *
     * \note AddPluginDialog::pluginSelected is connected to this slot.
     */
    void addPlugin(const LXQt::PluginInfo &desktopFile);
    /*!
     * \brief removePlugin Removes a Plugin from the model.
     *
     * The Plugin to remove is identified by the QObject::sender() method
     * when the slot is called. Therefore, this method should only be called
     * by connecting a signal that a Plugin will emit to this slot.
     * Otherwise, nothing will happen.
     *
     * \note Plugin::remove is connected to this slot as soon as the
     * Plugin is loaded in the PanelPluginsModel.
     */
    void removePlugin();

    // slots for configuration dialog
    /*!
     * \brief onMovePluginUp Moves the Plugin corresponding to the given
     * model index a slot upwards.
     *
     * \note The 'Up' button in the configuration widget is connected to this
     * slot.
     */
    void onMovePluginUp(QModelIndex const & index);
    /*!
     * \brief onMovePluginDown Moves the Plugin corresponding to the given
     * model index a slot downwards.
     *
     * \note The 'Down' button in the configuration widget is connected to this
     * slot.
     */
    void onMovePluginDown(QModelIndex const & index);
    /*!
     * \brief onConfigurePlugin If the Plugin corresponding to the given
     * model index has a config dialog (checked via the flag
     * ILXQtPanelPlugin::HaveConfigDialog), this method shows
     * it by calling plugin->showConfigureDialog().
     *
     * \note The 'Configure' button in the configuration widget is connected to
     * this slot.
     */
    void onConfigurePlugin(QModelIndex const & index);
    /*!
     * \brief onRemovePlugin Removes the Plugin corresponding to the given
     * model index from the Model.
     *
     * \note The 'Remove' button in the configuration widget is connected to
     * this slot.
     */
    void onRemovePlugin(QModelIndex const & index);

private:
    /*!
     * \brief pluginslist_t is the data type used for mPlugins which stores
     * all the Plugins.
     *
     * \sa mPlugins
     */
    typedef QList<QPair <QString/*name*/, QPointer<Plugin> > > pluginslist_t;

private:
    /*!
     * \brief loadPlugins Loads all the Plugins.
     * \param desktopDirs These directories are scanned for corresponding
     * .desktop-files which are necessary to load the plugins.
     */
    void loadPlugins(QStringList const & desktopDirs);
    /*!
     * \brief loadPlugin Loads a Plugin and connects signals and slots.
     * \param desktopFile The desktop file that specifies how to load the
     * Plugin.
     * \param settingsGroup QString which specifies the settings group. This
     * will only be redirected to the Plugin so that it knows how to read
     * its settings.
     * \return A QPointer to the Plugin that was loaded.
     */
    QPointer<Plugin> loadPlugin(LXQt::PluginInfo const & desktopFile, QString const & settingsGroup);
    /*!
     * \brief findNewPluginSettingsGroup Creates a name for a new Plugin
     * that is not yet present in the settings file. Whenever multiple
     * instances of a single Plugin type are created, they have to be
     * distinguished by this name.
     *
     * The first Plugin of a given type will be named like the type, e.g.
     * "mainmenu". If a name is already present, this method tries to
     * find a free name by appending increasing integers (starting with 2),
     * e.g. "mainmenu2". If, for example, only "mainmenu2" exists because
     * "mainmenu" was deleted, "mainmenu" would be returned. So, the method
     * always finds the first suitable name that is not yet present in the
     * settings file.
     * \param pluginType Type of the Plugin.
     * \return The created name for the Plugin.
     */
    QString findNewPluginSettingsGroup(const QString &pluginType) const;
    /*!
     * \brief isIndexValid Checks if a given model index is valid for the
     * underlying data (column 0, row lower than number of Plugins and
     * so on).
     */
    bool isIndexValid(QModelIndex const & index) const;
    /*!
     * \brief removePlugin Removes a given Plugin from the model.
     */
    void removePlugin(pluginslist_t::iterator plugin);

    /*!
     * \brief mNamesKey The key to the settings-entry that stores the
     * names of the Plugins in a panel. Set upon creation, passed as
     * a parameter by the panel.
     */
    const QString mNamesKey;
    /*!
     * \brief mPlugins Stores all the Plugins.
     *
     * mPlugins is a QList of elements while each element corresponds to a
     * single Plugin. Each element is a QPair of a QString and a QPointer
     * while the QPointer points to a Plugin.
     *
     * To access the elements, you can use indexing or an iterator on the
     * list. For each element p, p.first is the name of the Plugin as it
     * is used in the configuration files, p.second.data() is the Plugin.
     *
     * \sa pluginslist_t
     */
    pluginslist_t mPlugins;
    /*!
     * \brief mPanel Stores a reference to the LXQtPanel.
     */
    LXQtPanel * mPanel;
};

Q_DECLARE_METATYPE(Plugin const *)

#endif // PANELPLUGINSMODEL_H
