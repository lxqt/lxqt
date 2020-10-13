/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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


#include "plugin.h"
#include "ilxqtpanelplugin.h"
#include "pluginsettings_p.h"
#include "lxqtpanel.h"

#include <KWindowSystem>

#include <QDebug>
#include <QProcessEnvironment>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QGridLayout>
#include <QDialog>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QApplication>
#include <QWindow>
#include <memory>

#include <LXQt/Settings>
#include <LXQt/Translator>
#include <XdgIcon>

// statically linked built-in plugins
#if defined(WITH_DESKTOPSWITCH_PLUGIN)
#include "../plugin-desktopswitch/desktopswitch.h" // desktopswitch
extern void * loadPluginTranslation_desktopswitch_helper;
#endif
#if defined(WITH_MAINMENU_PLUGIN)
#include "../plugin-mainmenu/lxqtmainmenu.h" // mainmenu
extern void * loadPluginTranslation_mainmenu_helper;
#endif
#if defined(WITH_QUICKLAUNCH_PLUGIN)
#include "../plugin-quicklaunch/lxqtquicklaunchplugin.h" // quicklaunch
extern void * loadPluginTranslation_quicklaunch_helper;
#endif
#if defined(WITH_SHOWDESKTOP_PLUGIN)
#include "../plugin-showdesktop/showdesktop.h" // showdesktop
extern void * loadPluginTranslation_showdesktop_helper;
#endif
#if defined(WITH_SPACER_PLUGIN)
#include "../plugin-spacer/spacer.h" // spacer
extern void * loadPluginTranslation_spacer_helper;
#endif
#if defined(WITH_STATUSNOTIFIER_PLUGIN)
#include "../plugin-statusnotifier/statusnotifier.h" // statusnotifier
extern void * loadPluginTranslation_statusnotifier_helper;
#endif
#if defined(WITH_TASKBAR_PLUGIN)
#include "../plugin-taskbar/lxqttaskbarplugin.h" // taskbar
extern void * loadPluginTranslation_taskbar_helper;
#endif
#if defined(WITH_TRAY_PLUGIN)
#include "../plugin-tray/lxqttrayplugin.h" // tray
extern void * loadPluginTranslation_tray_helper;
#endif
#if defined(WITH_WORLDCLOCK_PLUGIN)
#include "../plugin-worldclock/lxqtworldclock.h" // worldclock
extern void * loadPluginTranslation_worldclock_helper;
#endif

QColor Plugin::mMoveMarkerColor= QColor(255, 0, 0, 255);

/************************************************

 ************************************************/
Plugin::Plugin(const LXQt::PluginInfo &desktopFile, LXQt::Settings *settings, const QString &settingsGroup, LXQtPanel *panel) :
    QFrame(panel),
    mDesktopFile(desktopFile),
    mPluginLoader(0),
    mPlugin(0),
    mPluginWidget(0),
    mAlignment(AlignLeft),
    mPanel(panel)
{
    mSettings = PluginSettingsFactory::create(settings, settingsGroup);

    setWindowTitle(desktopFile.name());
    mName = desktopFile.name();

    QStringList dirs;
    dirs << QProcessEnvironment::systemEnvironment().value(QStringLiteral("LXQTPANEL_PLUGIN_PATH")).split(QStringLiteral(":"));
    dirs << QStringLiteral(PLUGIN_DIR);

    bool found = false;
    if(ILXQtPanelPluginLibrary const * pluginLib = findStaticPlugin(desktopFile.id()))
    {
        // this is a static plugin
        found = true;
        loadLib(pluginLib);
    }
    else {
        // this plugin is a dynamically loadable module
        QString baseName = QStringLiteral("lib%1.so").arg(desktopFile.id());
        for(const QString &dirName : qAsConst(dirs))
        {
            QFileInfo fi(QDir(dirName), baseName);
            if (fi.exists())
            {
                found = true;
                if (loadModule(fi.absoluteFilePath()))
                    break;
            }
        }
    }

    if (!isLoaded())
    {
        if (!found)
            qWarning() << QStringLiteral("Plugin %1 not found in the").arg(desktopFile.id()) << dirs;

        return;
    }

    setObjectName(mPlugin->themeId() + QStringLiteral("Plugin"));

    // plugin handle for easy context menu
    setProperty("NeedsHandle", mPlugin->flags().testFlag(ILXQtPanelPlugin::NeedsHandle));

    QString s = mSettings->value(QStringLiteral("alignment")).toString();

    // Retrun default value
    if (s.isEmpty())
    {
        mAlignment = (mPlugin->flags().testFlag(ILXQtPanelPlugin::PreferRightAlignment)) ?
                    Plugin::AlignRight :
                    Plugin::AlignLeft;
    }
    else
    {
        mAlignment = (s.toUpper() == QLatin1String("RIGHT")) ?
                    Plugin::AlignRight :
                    Plugin::AlignLeft;

    }

    if (mPluginWidget)
    {
        QGridLayout* layout = new QGridLayout(this);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);
        layout->addWidget(mPluginWidget, 0, 0);
    }

    saveSettings();

    // delay the connection to settingsChanged to avoid conflicts
    // while the plugin is still being initialized
    connect(mSettings, &PluginSettings::settingsChanged,
            this, &Plugin::settingsChanged);
}


/************************************************

 ************************************************/
Plugin::~Plugin()
{
    delete mPlugin;
    delete mPluginLoader;
    delete mSettings;
}

void Plugin::setAlignment(Plugin::Alignment alignment)
{
    mAlignment = alignment;
    saveSettings();
}


/************************************************

 ************************************************/
namespace
{
    //helper types for static plugins storage & binary search
    typedef std::unique_ptr<ILXQtPanelPluginLibrary> plugin_ptr_t;
    typedef std::tuple<QString, plugin_ptr_t, void *> plugin_tuple_t;

    //NOTE: Please keep the plugins sorted by name while adding new plugins.
    //NOTE2: we need to reference some (dummy) symbol from (autogenerated) LXQtPluginTranslationLoader.cpp
    // to be not stripped (as unused/unreferenced) in static linking time
    static plugin_tuple_t const static_plugins[] = {
#if defined(WITH_DESKTOPSWITCH_PLUGIN)
        std::make_tuple(QLatin1String("desktopswitch"), plugin_ptr_t{new DesktopSwitchPluginLibrary}, loadPluginTranslation_desktopswitch_helper),// desktopswitch
#endif
#if defined(WITH_MAINMENU_PLUGIN)
        std::make_tuple(QLatin1String("mainmenu"), plugin_ptr_t{new LXQtMainMenuPluginLibrary}, loadPluginTranslation_mainmenu_helper),// mainmenu
#endif
#if defined(WITH_QUICKLAUNCH_PLUGIN)
        std::make_tuple(QLatin1String("quicklaunch"), plugin_ptr_t{new LXQtQuickLaunchPluginLibrary}, loadPluginTranslation_quicklaunch_helper),// quicklaunch
#endif
#if defined(WITH_SHOWDESKTOP_PLUGIN)
        std::make_tuple(QLatin1String("showdesktop"), plugin_ptr_t{new ShowDesktopLibrary}, loadPluginTranslation_showdesktop_helper),// showdesktop
#endif
#if defined(WITH_SPACER_PLUGIN)
        std::make_tuple(QLatin1String("spacer"), plugin_ptr_t{new SpacerPluginLibrary}, loadPluginTranslation_spacer_helper),// spacer
#endif
#if defined(WITH_STATUSNOTIFIER_PLUGIN)
        std::make_tuple(QLatin1String("statusnotifier"), plugin_ptr_t{new StatusNotifierLibrary}, loadPluginTranslation_statusnotifier_helper),// statusnotifier
#endif
#if defined(WITH_TASKBAR_PLUGIN)
        std::make_tuple(QLatin1String("taskbar"), plugin_ptr_t{new LXQtTaskBarPluginLibrary}, loadPluginTranslation_taskbar_helper),// taskbar
#endif
#if defined(WITH_TRAY_PLUGIN)
        std::make_tuple(QLatin1String("tray"), plugin_ptr_t{new LXQtTrayPluginLibrary}, loadPluginTranslation_tray_helper),// tray
#endif
#if defined(WITH_WORLDCLOCK_PLUGIN)
        std::make_tuple(QLatin1String("worldclock"), plugin_ptr_t{new LXQtWorldClockLibrary}, loadPluginTranslation_worldclock_helper),// worldclock
#endif
    };
    static constexpr plugin_tuple_t const * const plugins_begin = static_plugins;
    static constexpr plugin_tuple_t const * const plugins_end = static_plugins + sizeof (static_plugins) / sizeof (static_plugins[0]);

    struct assert_helper
    {
        assert_helper()
        {
            Q_ASSERT(std::is_sorted(plugins_begin, plugins_end
                        , [] (plugin_tuple_t const & p1, plugin_tuple_t const & p2) -> bool { return std::get<0>(p1) < std::get<0>(p2); }));
        }
    };
    static assert_helper h;
}

ILXQtPanelPluginLibrary const * Plugin::findStaticPlugin(const QString &libraryName)
{
    // find a static plugin library by name -> binary search
    plugin_tuple_t const * plugin = std::lower_bound(plugins_begin, plugins_end, libraryName
            , [] (plugin_tuple_t const & plugin, QString const & name) -> bool { return std::get<0>(plugin) < name; });
    if (plugins_end != plugin && libraryName == std::get<0>(*plugin))
        return std::get<1>(*plugin).get();
    return nullptr;
}

// load a plugin from a library
bool Plugin::loadLib(ILXQtPanelPluginLibrary const * pluginLib)
{
    ILXQtPanelPluginStartupInfo startupInfo;
    startupInfo.settings = mSettings;
    startupInfo.desktopFile = &mDesktopFile;
    startupInfo.lxqtPanel = mPanel;

    mPlugin = pluginLib->instance(startupInfo);
    if (!mPlugin)
    {
        qWarning() << QStringLiteral("Can't load plugin \"%1\". Plugin can't build ILXQtPanelPlugin.").arg(mDesktopFile.id());
        return false;
    }

    mPluginWidget = mPlugin->widget();
    if (mPluginWidget)
    {
        mPluginWidget->setObjectName(mPlugin->themeId());
        watchWidgets(mPluginWidget);
    }
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return true;
}

// load dynamic plugin from a *.so module
bool Plugin::loadModule(const QString &libraryName)
{
    mPluginLoader = new QPluginLoader(libraryName);

    if (!mPluginLoader->load())
    {
        qWarning() << mPluginLoader->errorString();
        return false;
    }

    QObject *obj = mPluginLoader->instance();
    if (!obj)
    {
        qWarning() << mPluginLoader->errorString();
        return false;
    }

    ILXQtPanelPluginLibrary* pluginLib= qobject_cast<ILXQtPanelPluginLibrary*>(obj);
    if (!pluginLib)
    {
        qWarning() << QStringLiteral("Can't load plugin \"%1\". Plugin is not a ILXQtPanelPluginLibrary.").arg(mPluginLoader->fileName());
        delete obj;
        return false;
    }
    return loadLib(pluginLib);
}

/************************************************

 ************************************************/
void Plugin::watchWidgets(QObject * const widget)
{
    // the QWidget might not be fully constructed yet, but we can rely on the isWidgetType()
    if (!widget->isWidgetType())
        return;
    widget->installEventFilter(this);
    // watch also children (recursive)
    for (auto const & child : widget->children())
    {
        watchWidgets(child);
    }
}

/************************************************

 ************************************************/
void Plugin::unwatchWidgets(QObject * const widget)
{
    widget->removeEventFilter(this);
    // unwatch also children (recursive)
    for (auto const & child : widget->children())
    {
        unwatchWidgets(child);
    }
}

/************************************************

 ************************************************/
void Plugin::settingsChanged()
{
    mPlugin->settingsChanged();
}


/************************************************

 ************************************************/
void Plugin::saveSettings()
{
    mSettings->setValue(QStringLiteral("alignment"), (mAlignment == AlignLeft) ? QStringLiteral("Left") : QStringLiteral("Right"));
    mSettings->setValue(QStringLiteral("type"), mDesktopFile.id());
    mSettings->sync();

}


/************************************************

 ************************************************/
void Plugin::contextMenuEvent(QContextMenuEvent * /*event*/)
{
    mPanel->showPopupMenu(this);
}


/************************************************

 ************************************************/
void Plugin::mousePressEvent(QMouseEvent *event)
{
    switch (event->button())
    {
    case Qt::LeftButton:
        mPlugin->activated(ILXQtPanelPlugin::Trigger);
        break;

    case Qt::MidButton:
        mPlugin->activated(ILXQtPanelPlugin::MiddleClick);
        break;

    default:
        break;
    }
}


/************************************************

 ************************************************/
void Plugin::mouseDoubleClickEvent(QMouseEvent*)
{
    mPlugin->activated(ILXQtPanelPlugin::DoubleClick);
}


/************************************************

 ************************************************/
void Plugin::showEvent(QShowEvent *)
{
    // ensure that plugin widgets have correct sizes at startup
    if (mPluginWidget)
    {
        mPluginWidget->updateGeometry(); // needed for widgets with style sizes (like buttons)
        mPluginWidget->adjustSize();
    }
}


/************************************************

 ************************************************/
QMenu *Plugin::popupMenu() const
{
    QString name = this->name().replace(QLatin1String("&"), QLatin1String("&&"));
    QMenu* menu = new QMenu(windowTitle());

    if (mPlugin->flags().testFlag(ILXQtPanelPlugin::HaveConfigDialog))
    {
        QAction* configAction = new QAction(
            XdgIcon::fromTheme(QLatin1String("preferences-other")),
            tr("Configure \"%1\"").arg(name), menu);
        menu->addAction(configAction);
        connect(configAction, SIGNAL(triggered()), this, SLOT(showConfigureDialog()));
    }

    QAction* moveAction = new QAction(XdgIcon::fromTheme(QStringLiteral("transform-move")), tr("Move \"%1\"").arg(name), menu);
    menu->addAction(moveAction);
    connect(moveAction, SIGNAL(triggered()), this, SIGNAL(startMove()));

    menu->addSeparator();

    QAction* removeAction = new QAction(
        XdgIcon::fromTheme(QLatin1String("list-remove")),
        tr("Remove \"%1\"").arg(name), menu);
    menu->addAction(removeAction);
    connect(removeAction, SIGNAL(triggered()), this, SLOT(requestRemove()));

    return menu;
}


/************************************************

 ************************************************/
bool Plugin::isSeparate() const
{
   return mPlugin->isSeparate();
}


/************************************************

 ************************************************/
bool Plugin::isExpandable() const
{
    return mPlugin->isExpandable();
}


/************************************************

 ************************************************/
bool Plugin::eventFilter(QObject * /*watched*/, QEvent * event)
{
    switch (event->type())
    {
        case QEvent::DragLeave:
            emit dragLeft();
            break;
        case QEvent::ChildAdded:
            watchWidgets(dynamic_cast<QChildEvent *>(event)->child());
            break;
        case QEvent::ChildRemoved:
            unwatchWidgets(dynamic_cast<QChildEvent *>(event)->child());
            break;
        default:
            break;
    }
    return false;
}

/************************************************

 ************************************************/
void Plugin::realign()
{
    if (mPlugin)
        mPlugin->realign();
}


/************************************************

 ************************************************/
void Plugin::showConfigureDialog()
{
    if (!mConfigDialog)
        mConfigDialog = mPlugin->configureDialog();

    if (!mConfigDialog)
        return;

    connect(this, &Plugin::destroyed, mConfigDialog.data(), &QWidget::close);
    mPanel->willShowWindow(mConfigDialog);
    mConfigDialog->show();
    mConfigDialog->raise();
    mConfigDialog->activateWindow();

    WId wid = mConfigDialog->windowHandle()->winId();
    KWindowSystem::activateWindow(wid);
    KWindowSystem::setOnDesktop(wid, KWindowSystem::currentDesktop());
}


/************************************************

 ************************************************/
void Plugin::requestRemove()
{
    emit remove();
    deleteLater();
}
