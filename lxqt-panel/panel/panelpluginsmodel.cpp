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

#include "panelpluginsmodel.h"
#include "plugin.h"
#include "ilxqtpanelplugin.h"
#include "lxqtpanel.h"
#include "lxqtpanelapplication.h"
#include <QPointer>
#include <XdgIcon>
#include <LXQt/Settings>

#include <QDebug>

PanelPluginsModel::PanelPluginsModel(LXQtPanel * panel,
                                     QString const & namesKey,
                                     QStringList const & desktopDirs,
                                     QObject * parent/* = nullptr*/)
    : QAbstractListModel{parent},
    mNamesKey(namesKey),
    mPanel(panel)
{
    loadPlugins(desktopDirs);
}

PanelPluginsModel::~PanelPluginsModel()
{
    qDeleteAll(plugins());
}

int PanelPluginsModel::rowCount(const QModelIndex & parent/* = QModelIndex()*/) const
{
    return QModelIndex() == parent ? mPlugins.size() : 0;
}


QVariant PanelPluginsModel::data(const QModelIndex & index, int role/* = Qt::DisplayRole*/) const
{
    Q_ASSERT(QModelIndex() == index.parent()
            && 0 == index.column()
            && mPlugins.size() > index.row()
            );

    pluginslist_t::const_reference plugin = mPlugins[index.row()];
    QVariant ret;
    switch (role)
    {
        case Qt::DisplayRole:
            if (plugin.second.isNull())
                ret = QStringLiteral("<b>Unknown</b> (%1)").arg(plugin.first);
            else
                ret = QStringLiteral("<b>%1</b> (%2)").arg(plugin.second->name(), plugin.first);
            break;
        case Qt::DecorationRole:
            if (plugin.second.isNull())
                ret = XdgIcon::fromTheme(QStringLiteral("preferences-plugin"));
            else
                ret = plugin.second->desktopFile().icon(XdgIcon::fromTheme(QStringLiteral("preferences-plugin")));
            break;
        case Qt::UserRole:
            ret = QVariant::fromValue(const_cast<Plugin const *>(plugin.second.data()));
            break;
    }
    return ret;
}

Qt::ItemFlags PanelPluginsModel::flags(const QModelIndex & /*index*/) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
}

QStringList PanelPluginsModel::pluginNames() const
{
    QStringList names;
    for (auto const & p : mPlugins)
        names.append(p.first);
    return names;
}

QList<Plugin *> PanelPluginsModel::plugins() const
{
    QList<Plugin *> plugins;
    for (auto const & p : mPlugins)
        if (!p.second.isNull())
            plugins.append(p.second.data());
    return plugins;
}

Plugin* PanelPluginsModel::pluginByName(QString name) const
{
    for (auto const & p : mPlugins)
        if (p.first == name)
            return p.second.data();
    return nullptr;
}

Plugin const * PanelPluginsModel::pluginByID(QString id) const
{
    for (auto const & p : mPlugins)
    {
        Plugin *plugin = p.second.data();
        if (plugin && plugin->desktopFile().id() == id)
            return plugin;
    }
    return nullptr;
}

void PanelPluginsModel::addPlugin(const LXQt::PluginInfo &desktopFile)
{
    if (dynamic_cast<LXQtPanelApplication const *>(qApp)->isPluginSingletonAndRunnig(desktopFile.id()))
        return;

    QString name = findNewPluginSettingsGroup(desktopFile.id());

    QPointer<Plugin> plugin = loadPlugin(desktopFile, name);
    if (plugin.isNull())
        return;

    beginInsertRows(QModelIndex(), mPlugins.size(), mPlugins.size());
    mPlugins.append({name, plugin});
    endInsertRows();
    mPanel->settings()->setValue(mNamesKey, pluginNames());
    emit pluginAdded(plugin.data());
}

void PanelPluginsModel::removePlugin(pluginslist_t::iterator plugin)
{
    if (mPlugins.end() != plugin)
    {
        mPanel->settings()->remove(plugin->first);
        Plugin * p = plugin->second.data();
        const int row = plugin - mPlugins.begin();
        beginRemoveRows(QModelIndex(), row, row);
        mPlugins.erase(plugin);
        endRemoveRows();
        emit pluginRemoved(p); // p can be nullptr
        mPanel->settings()->setValue(mNamesKey, pluginNames());
        if (nullptr != p)
            p->deleteLater();
    }
}

void PanelPluginsModel::removePlugin()
{
    Plugin * p = qobject_cast<Plugin*>(sender());
    auto plugin = std::find_if(mPlugins.begin(), mPlugins.end(),
                               [p] (pluginslist_t::const_reference obj) { return p == obj.second; });
    removePlugin(std::move(plugin));
}

void PanelPluginsModel::movePlugin(Plugin * plugin, QString const & nameAfter)
{
    //merge list of plugins (try to preserve original position)
    //subtract mPlugin.begin() from the found Plugins to get the model index
    const int from =
        std::find_if(mPlugins.begin(), mPlugins.end(), [plugin] (pluginslist_t::const_reference obj) { return plugin == obj.second.data(); })
        - mPlugins.begin();
    const int to =
        std::find_if(mPlugins.begin(), mPlugins.end(), [nameAfter] (pluginslist_t::const_reference obj) { return nameAfter == obj.first; })
        - mPlugins.begin();
    /* 'from' is the current position of the Plugin to be moved ("moved Plugin"),
     * 'to' is the position of the Plugin behind the one that is being moved
     * ("behind Plugin"). There are several cases to distinguish:
     * 1. from > to: The moved Plugin had been behind the behind Plugin before
     * and is moved to the front of the behind Plugin. The moved Plugin will
     * be inserted at position 'to', the behind Plugin and all the following
     * Plugins (until the former position of the moved Plugin) will increment
     * their indexes.
     * 2. from < to: The moved Plugin had already been located before the
     * behind Plugin. In this case, the move operation only reorders the
     * Plugins before the behind Plugin. All the Plugins between the moved
     * Plugin and the behind Plugin will decrement their index. Therefore, the
     * movedPlugin will not be at position 'to' but rather on position 'to-1'.
     * 3. from == to: This does not make sense, we catch this case to prevent
     * errors.
     * 4. from == to-1: The moved Plugin has not moved because it had already
     * been located in front of the behind Plugin.
     */
    const int to_plugins = from < to ? to - 1 : to;

    if (from != to && from != to_plugins)
    {
        /* Although the new position of the moved Plugin will be 'to-1' if
         * from < to, we insert 'to' here. This is exactly how it is done
         * in the Qt documentation.
         */
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        // For the QList::move method, use the right position
        mPlugins.move(from, to_plugins);
        endMoveRows();
        emit pluginMoved(plugin);
        mPanel->settings()->setValue(mNamesKey, pluginNames());
    }
}

void PanelPluginsModel::loadPlugins(QStringList const & desktopDirs)
{
    QStringList plugin_names = mPanel->settings()->value(mNamesKey).toStringList();

#ifdef DEBUG_PLUGIN_LOADTIME
    QElapsedTimer timer;
    timer.start();
    qint64 lastTime = 0;
#endif
    for (auto const & name : qAsConst(plugin_names))
    {
        pluginslist_t::iterator i = mPlugins.insert(mPlugins.end(), {name, nullptr});
        QString type = mPanel->settings()->value(name + QStringLiteral("/type")).toString();
        if (type.isEmpty())
        {
            qWarning() << QStringLiteral("Section \"%1\" not found in %2.").arg(name, mPanel->settings()->fileName());
            continue;
        }
#ifdef WITH_SCREENSAVER_FALLBACK
        if (QStringLiteral("screensaver") == type)
        {
            //plugin-screensaver was dropped
            //convert settings to plugin-quicklaunch
            const QString & lock_desktop = QStringLiteral(LXQT_LOCK_DESKTOP);
            qWarning().noquote() << "Found deprecated plugin of type 'screensaver', migrating to 'quicklaunch' with '" << lock_desktop << '\'';
            type = QStringLiteral("quicklaunch");
            LXQt::Settings * settings = mPanel->settings();
            settings->beginGroup(name);
            settings->remove(QString{});//remove all existing keys
            settings->setValue(QStringLiteral("type"), type);
            settings->beginWriteArray(QStringLiteral("apps"), 1);
            settings->setArrayIndex(0);
            settings->setValue(QStringLiteral("desktop"), lock_desktop);
            settings->endArray();
            settings->endGroup();
        }
#endif

        LXQt::PluginInfoList list = LXQt::PluginInfo::search(desktopDirs, QStringLiteral("LXQtPanel/Plugin"), QStringLiteral("%1.desktop").arg(type));
        if( !list.count())
        {
            qWarning() << QStringLiteral("Plugin \"%1\" not found.").arg(type);
            continue;
        }

        i->second = loadPlugin(list.first(), name);
#ifdef DEBUG_PLUGIN_LOADTIME
        qDebug() << "load plugin" << type << "takes" << (timer.elapsed() - lastTime) << "ms";
        lastTime = timer.elapsed();
#endif
    }
}

QPointer<Plugin> PanelPluginsModel::loadPlugin(LXQt::PluginInfo const & desktopFile, QString const & settingsGroup)
{
    std::unique_ptr<Plugin> plugin(new Plugin(desktopFile, mPanel->settings(), settingsGroup, mPanel));
    if (plugin->isLoaded())
    {
        connect(mPanel, &LXQtPanel::realigned, plugin.get(), &Plugin::realign);
        connect(plugin.get(), &Plugin::remove,
                this, static_cast<void (PanelPluginsModel::*)()>(&PanelPluginsModel::removePlugin));
        return plugin.release();
    }

    return nullptr;
}

QString PanelPluginsModel::findNewPluginSettingsGroup(const QString &pluginType) const
{
    QStringList groups = mPanel->settings()->childGroups();
    groups.sort();

    // Generate new section name
    QString pluginName = QStringLiteral("%1").arg(pluginType);

    if (!groups.contains(pluginName))
        return pluginName;
    else
    {
        for (int i = 2; true; ++i)
        {
            pluginName = QStringLiteral("%1%2").arg(pluginType).arg(i);
            if (!groups.contains(pluginName))
                return pluginName;
        }
    }
}

bool PanelPluginsModel::isIndexValid(QModelIndex const & index) const
{
    return index.isValid() && QModelIndex() == index.parent()
        && 0 == index.column() && mPlugins.size() > index.row();
}

void PanelPluginsModel::onMovePluginUp(QModelIndex const & index)
{
    if (!isIndexValid(index))
        return;

    const int row = index.row();
    if (0 >= row)
        return; //can't move up

    beginMoveRows(QModelIndex(), row, row, QModelIndex(), row - 1);
    mPlugins.swap(row - 1, row);
    endMoveRows();
    pluginslist_t::const_reference moved_plugin = mPlugins[row - 1];
    pluginslist_t::const_reference prev_plugin = mPlugins[row];

    emit pluginMoved(moved_plugin.second.data());
    //emit signal for layout only in case both plugins are loaded/displayed
    if (!moved_plugin.second.isNull() && !prev_plugin.second.isNull())
        emit pluginMovedUp(moved_plugin.second.data());

    mPanel->settings()->setValue(mNamesKey, pluginNames());
}

void PanelPluginsModel::onMovePluginDown(QModelIndex const & index)
{
    if (!isIndexValid(index))
        return;

    const int row = index.row();
    if (mPlugins.size() <= row + 1)
        return; //can't move down

    beginMoveRows(QModelIndex(), row, row, QModelIndex(), row + 2);
    mPlugins.swap(row, row + 1);
    endMoveRows();
    pluginslist_t::const_reference moved_plugin = mPlugins[row + 1];
    pluginslist_t::const_reference next_plugin = mPlugins[row];

    emit pluginMoved(moved_plugin.second.data());
    //emit signal for layout only in case both plugins are loaded/displayed
    if (!moved_plugin.second.isNull() && !next_plugin.second.isNull())
        emit pluginMovedUp(next_plugin.second.data());

    mPanel->settings()->setValue(mNamesKey, pluginNames());
}

void PanelPluginsModel::onConfigurePlugin(QModelIndex const & index)
{
    if (!isIndexValid(index))
        return;

    Plugin * const plugin = mPlugins[index.row()].second.data();
    if (nullptr != plugin && (ILXQtPanelPlugin::HaveConfigDialog & plugin->iPlugin()->flags()))
        plugin->showConfigureDialog();
}

void PanelPluginsModel::onRemovePlugin(QModelIndex const & index)
{
    if (!isIndexValid(index))
        return;

    auto plugin = mPlugins.begin() + index.row();
    if (plugin->second.isNull())
        removePlugin(std::move(plugin));
    else
        plugin->second->requestRemove();
}
