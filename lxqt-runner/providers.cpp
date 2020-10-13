/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2013 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Aaron Lewis <the.warl0ck.1989@gmail.com>
 *   Petr Vanek <petr@scribus.info>
 *   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#include "providers.h"
#include "yamlparser.h"
#include <XdgDesktopFile>
#include <XdgMenu>
#include <XmlHelper>
#include <XdgDirs>

#include <QProcess>
#include <QtAlgorithms>
#include <QFileInfo>
#include <QSettings>
#include <QDir>
#include <QApplication>
#include <QAction>
#include <LXQt/Globals>
#include <LXQt/PowerManager>
#include <LXQt/ScreenSaver>
#include "providers.h"
#include <LXQtGlobalKeys/Action>
#include <wordexp.h>
#include <QStandardPaths>

#define MAX_HISTORY 100


/************************************************

 ************************************************/
static QString expandCommand(const QString &command, QStringList *arguments=0)
{
    QString program;
    wordexp_t words;

    if (wordexp(command.toLocal8Bit().data(), &words, 0) != 0)
        return QString();

    char **w;
    w = words.we_wordv;
    program = QString::fromLocal8Bit(w[0]);

    if (arguments)
    {
        for (size_t i = 1; i < words.we_wordc; i++)
            *arguments << QString::fromLocal8Bit(w[i]);
    }

    wordfree(&words);
    return program;
}


/************************************************

 ************************************************/
static QString which(const QString &progName)
{
    if (progName.isEmpty())
        return QString();

    if (progName.startsWith(QDir::separator()))
    {
        QFileInfo fileInfo(progName);
        if (fileInfo.isExecutable() && fileInfo.isFile())
            return fileInfo.absoluteFilePath();
    }

    const QStringList dirs = QFile::decodeName(qgetenv("PATH")).split(QL1C(':'));

    for (const QString &dir : dirs)
    {
        QFileInfo fileInfo(QDir(dir), progName);
        if (fileInfo.isExecutable() && fileInfo.isFile())
            return fileInfo.absoluteFilePath();
    }

    return QString();
}


/************************************************

 ************************************************/
static bool startProcess(QString command)
{
    QStringList args;
    QString program  = expandCommand(command, &args);
    if (program.isEmpty())
        return false;
    if (QProcess::startDetached(program, args))
    {
        return true;
    } else
    {
        //fallback for executable script with no #!
        //trying as in system(2)
        args.prepend(program);
        args.prepend(QSL("-c"));
        return QProcess::startDetached(QSL("/bin/sh"), args);
    }

}


/************************************************

 ************************************************/
unsigned int CommandProviderItem::stringRank(const QString str, const QString pattern) const
{
    int n = str.indexOf(pattern, 0, Qt::CaseInsensitive);
    if (n<0)
        return 0;

    return MAX_RANK - ((str.length() - pattern.length()) + n * 5);
}


/************************************************

 ************************************************/
CommandProvider::CommandProvider():
    QObject(),
    QList<CommandProviderItem*>()
{
}


/************************************************

 ************************************************/
CommandProvider::~CommandProvider()
{
//    qDebug() << "*****************************************";
//    qDebug() << hex << this;
//    qDebug() << "DESTROY";
    qDeleteAll(*this);
//    qDebug() << "*****************************************";
}


/************************************************

 ************************************************/
AppLinkItem::AppLinkItem(const QDomElement &element):
        CommandProviderItem()
{
    mIconName = element.attribute(QLatin1String("icon"));
    mTitle = element.attribute(QLatin1String("title"));
    mComment = element.attribute(QSL("genericName"));
    mToolTip = element.attribute(QLatin1String("comment"));
    mCommand = element.attribute(QL1S("exec"));
    mProgram = QFileInfo(element.attribute(QL1S("exec"))).baseName().section(QL1C(' '), 0, 0);
    mDesktopFile = element.attribute(QSL("desktopFile"));
    initExec();
    QMetaObject::invokeMethod(this, "updateIcon", Qt::QueuedConnection);
}

#ifdef HAVE_MENU_CACHE
AppLinkItem::AppLinkItem(MenuCacheApp* app):
        CommandProviderItem()
{
    MenuCacheItem* item = MENU_CACHE_ITEM(app);
    mIconName = QString::fromUtf8(menu_cache_item_get_icon(item));
    mTitle = QString::fromUtf8(menu_cache_item_get_name(item));
    mComment = QString::fromUtf8(menu_cache_app_get_generic_name(app));
    mToolTip = QString::fromUtf8(menu_cache_item_get_comment(item));
    mCommand = QString::fromUtf8(menu_cache_app_get_exec(app));
    mProgram = QFileInfo(mCommand).baseName().section(QL1C(' '), 0, 0);
    char* path = menu_cache_item_get_file_path(MENU_CACHE_ITEM(app));
    mDesktopFile = QString::fromLocal8Bit(path);
    g_free(path);
    initExec();
    QMetaObject::invokeMethod(this, "updateIcon", Qt::QueuedConnection);
    // qDebug() << "FOUND: " << mIconName << ", " << mCommand;
}
#endif

/************************************************

 ************************************************/
void AppLinkItem::updateIcon()
{
//    qDebug() << "*****************************************";
//    qDebug() << hex << this;
//    qDebug() << Q_FUNC_INFO;
    if (mIcon.isNull())
        mIcon = QIcon::fromTheme(mIconName);
//    qDebug() << "*****************************************";
}


/************************************************

 ************************************************/
void AppLinkItem::operator=(const AppLinkItem &other)
{
    mTitle = other.title();
    mComment = other.comment();
    mToolTip = other.toolTip();

    mCommand = other.mCommand;
    mProgram = other.mProgram;
    mDesktopFile = other.mDesktopFile;
    mExec = other.mExec;

    mIconName = other.mIconName;
    mIcon = other.icon();
}


/************************************************

 ************************************************/
unsigned int AppLinkItem::rank(const QString &pattern) const
{
    unsigned int result =  qMax(stringRank(mProgram, pattern),
                                stringRank(mTitle, pattern)
                                );
    return result;
}


/************************************************

 ************************************************/
bool AppLinkItem::run() const
{
    XdgDesktopFile desktop;
    if (desktop.load(mDesktopFile))
        return desktop.startDetached();
    else
        return false;
}


/************************************************

 ************************************************/
bool AppLinkItem::compare(const QRegExp &regExp) const
{
    if (regExp.isEmpty())
        return false;

    return mProgram.contains(regExp)
        || mTitle.contains(regExp)
        || mComment.contains(regExp)
        || mToolTip.contains(regExp);
}


/************************************************

 ************************************************/
void AppLinkItem::initExec()
{
    static const QRegExp split_re{QSL("\\s")};
    XdgDesktopFile desktop;
    if (desktop.load(mDesktopFile))
    {
        QStringList cmd = desktop.value(QSL("Exec")).toString().split(split_re);
        if (0 < cmd.size())
            mExec = which(expandCommand(cmd[0]));
    }
}


/************************************************

 ************************************************/
AppLinkProvider::AppLinkProvider():
        CommandProvider()
{
#ifdef HAVE_MENU_CACHE
    menu_cache_init(0);
    mMenuCache = menu_cache_lookup(XdgMenu::getMenuFileName().toLocal8Bit().constData());
    if(mMenuCache)
        mMenuCacheNotify = menu_cache_add_reload_notify(mMenuCache, (MenuCacheReloadNotify)menuCacheReloadNotify, this);
    else
        mMenuCacheNotify = 0;
#else
    mXdgMenu = new XdgMenu();
    mXdgMenu->setEnvironments(QStringList() << QSL("X-LXQT") << QSL("LXQt"));
    connect(mXdgMenu, SIGNAL(changed()), this, SLOT(update()));
    mXdgMenu->read(XdgMenu::getMenuFileName());
    update();
#endif
}


/************************************************

 ************************************************/
AppLinkProvider::~AppLinkProvider()
{
#ifdef HAVE_MENU_CACHE
    if(mMenuCache)
    {
        menu_cache_remove_reload_notify(mMenuCache, mMenuCacheNotify);
        menu_cache_unref(mMenuCache);
    }
#else
    delete mXdgMenu;
#endif
}


/************************************************

 ************************************************/
 #ifdef HAVE_MENU_CACHE

void AppLinkProvider::menuCacheReloadNotify(MenuCache* cache, gpointer user_data)
{
    // qDebug() << Q_FUNC_INFO;
    reinterpret_cast<AppLinkProvider*>(user_data)->update();
}

 #else // without menu-cache, use libqtxdg

 void doUpdate(const QDomElement &xml, QHash<QString, AppLinkItem*> &items)
{
    DomElementIterator it(xml, QString());
    while (it.hasNext())
    {
        QDomElement e = it.next();

        // Build submenu ........................
        if (e.tagName() == QL1S("Menu"))
            doUpdate(e, items);

        //Build application link ................
        else if (e.tagName() == QL1S("AppLink"))
        {
            AppLinkItem *item = new AppLinkItem(e);
            delete items[item->command()]; // delete previous item;
            items.insert(item->command(), item);
        }
    }
}
#endif

/************************************************

 ************************************************/
void AppLinkProvider::update()
{
    emit aboutToBeChanged();
    QHash<QString, AppLinkItem*> newItems;

#ifdef HAVE_MENU_CACHE
    // libmenu-cache is available, use it to get cached app list
    GSList* apps = menu_cache_list_all_apps(mMenuCache);
    for(GSList* l = apps; l; l = l->next)
    {
        MenuCacheApp* app = MENU_CACHE_APP(l->data);
        AppLinkItem *item = new AppLinkItem(app);
        AppLinkItem *prevItem = newItems[item->command()];
        if(prevItem)
            delete prevItem; // delete previous item;
        newItems.insert(item->command(), item);
        menu_cache_item_unref(MENU_CACHE_ITEM(app));
    }
    g_slist_free(apps);
#else
    // use libqtxdg XdgMenu to get installed apps
    doUpdate(mXdgMenu->xml().documentElement(), newItems);
#endif
    {
        QMutableListIterator<CommandProviderItem*> i(*this);
        while (i.hasNext()) {
            AppLinkItem *item = static_cast<AppLinkItem*>(i.next());
            AppLinkItem *newItem = newItems.take(item->command());
            if (newItem)
            {
                *(item) = *newItem;  // Copy by value, not pointer!
                // After the item is copied, the original "updateIcon" call queued
                // on the newItem object is never called since the object iss going to
                // be deleted. Hence we need to call it on the copied item manually.
                // Otherwise the copied item will have no icon.
                // FIXME: this is a dirty hack and it should be made cleaner later.
                if(item->icon().isNull())
                    QMetaObject::invokeMethod(item, "updateIcon", Qt::QueuedConnection);
                delete newItem;
            }
            else
            {
                i.remove();
                delete item;
            }
        }
    }

    {
        QHashIterator<QString, AppLinkItem*> i(newItems);
        while (i.hasNext())
        {
            append(i.next().value());
        }
    }

    emit changed();
}




/************************************************

 ************************************************/
HistoryItem::HistoryItem(const QString &command):
        CommandProviderItem()
{
    mIcon = QIcon::fromTheme(QLatin1String("application-x-executable"));
    mTitle = command;
    mComment = QObject::tr("History");
    mCommand = command;
}


/************************************************

 ************************************************/
bool HistoryItem::run() const
{
    return startProcess(mCommand);
}


/************************************************

 ************************************************/
bool HistoryItem::compare(const QRegExp &regExp) const
{
    return mCommand.contains(regExp);
}


/************************************************

 ************************************************/
unsigned int HistoryItem::rank(const QString &pattern) const
{
    return stringRank(mCommand, pattern);
}


/************************************************

 ************************************************/
HistoryProvider::HistoryProvider():
        CommandProvider()
{
    QString fileName = (XdgDirs::cacheHome() + QSL("/lxqt-runner.history"));
    mHistoryFile = new QSettings(fileName, QSettings::IniFormat);
    mHistoryFile->beginGroup(QSL("commands"));
    for (uint i=0; i<MAX_HISTORY; ++i)
    {
        QString key = QString::fromLatin1("%1").arg(i, 3, 10, QL1C('0'));
        if (mHistoryFile->contains(key))
        {
            HistoryItem *item = new HistoryItem(mHistoryFile->value(key).toString());
            append(item);
        }
    }
}


/************************************************

 ************************************************/
HistoryProvider::~HistoryProvider()
{
    delete mHistoryFile;
}


/************************************************

 ************************************************/
void HistoryProvider::AddCommand(const QString &command)
{
    bool commandExists = false;
    for (int i=0; !commandExists && i<length(); ++i)
    {
        if (command == static_cast<HistoryItem*>(at(i))->command())
        {
            move(i, 0);
            commandExists = true;
        }
    }

    if (!commandExists)
    {
        HistoryItem *item = new HistoryItem(command);
        insert(0, item);
    }

    mHistoryFile->clear();
    for (int i=0; i<qMin(length(), MAX_HISTORY); ++i)
    {
        QString key = QString::fromLatin1("%1").arg(i, 3, 10, QL1C('0'));
        mHistoryFile->setValue(key, static_cast<HistoryItem*>(at(i))->command());
    }
}

void HistoryProvider::clearHistory()
{
    clear();
    mHistoryFile->clear();
}

/************************************************

 ************************************************/
CustomCommandItem::CustomCommandItem(CustomCommandProvider *provider):
    CommandProviderItem(),
    mProvider(provider)
{
    mIcon = QIcon::fromTheme(QSL("utilities-terminal"));
}


/************************************************

 ************************************************/
void CustomCommandItem::setCommand(const QString &command)
{
    mCommand = command;
    mTitle = mCommand;

    mExec = which(expandCommand(command));

    if (!mExec.isEmpty())
        mComment = QString::fromLatin1("%1 %2").arg(mExec, command.section(QL1C(' '), 1));
    else
        mComment = QString();

}


/************************************************

 ************************************************/
bool CustomCommandItem::run() const
{
    bool ret = startProcess(mCommand);
    if (ret && mProvider->historyProvider())
        mProvider->historyProvider()->AddCommand(mCommand);

    return ret;
}


/************************************************

 ************************************************/
bool CustomCommandItem::compare(const QRegExp & /*regExp*/) const
{
    return !mComment.isEmpty();
}


/************************************************

 ************************************************/
unsigned int CustomCommandItem::rank(const QString & /*pattern*/) const
{
    return 0;
}


/************************************************

 ************************************************/
CustomCommandProvider::CustomCommandProvider():
    CommandProvider(),
    mHistoryProvider(0)
{
    mItem = new CustomCommandItem(this);
    append(mItem);
}

#ifdef VBOX_ENABLED
VirtualBoxItem::VirtualBoxItem(const QString & MachineName , const QIcon & Icon):
        CommandProviderItem()
{
    mTitle = MachineName;
    mIcon = Icon;
}

void VirtualBoxItem::setRDEPort(const QString & portNum)
{
    m_rdePortNum = portNum;
}

bool VirtualBoxItem::run() const
{
    QStringList arguments;
#ifdef VBOX_HEADLESS_ENABLED
    arguments << QSL("-startvm") << title();
    return QProcess::startDetached (QSL("VBoxHeadless") , arguments);
#else
    arguments << QSL("startvm") << title();
    return QProcess::startDetached (QSL("VBoxManage") , arguments);
#endif

}

bool VirtualBoxItem::compare(const QRegExp &regExp) const
{
    return (! regExp.isEmpty() && -1 != regExp.indexIn (title ()));
}

unsigned int VirtualBoxItem::rank(const QString &pattern) const
{
    return stringRank(mTitle, pattern);
}

inline QString homeDir() {
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

///////
VirtualBoxProvider::VirtualBoxProvider():
        virtualBoxConfig ( homeDir() + QSL("/.VirtualBox/VirtualBox.xml"))
{
    fp.setFileName (virtualBoxConfig);

    static const char *kOSTypeIcons [][2] =
    {
        {"Other",           ":/vbox-icons/os_other.png"},
        {"DOS",             ":/vbox-icons/os_dos.png"},
        {"Netware",         ":/vbox-icons/os_netware.png"},
        {"L4",              ":/vbox-icons/os_l4.png"},
        {"Windows31",       ":/vbox-icons/os_win31.png"},
        {"Windows95",       ":/vbox-icons/os_win95.png"},
        {"Windows98",       ":/vbox-icons/os_win98.png"},
        {"WindowsMe",       ":/vbox-icons/os_winme.png"},
        {"WindowsNT4",      ":/vbox-icons/os_winnt4.png"},
        {"Windows2000",     ":/vbox-icons/os_win2k.png"},
        {"WindowsXP",       ":/vbox-icons/os_winxp.png"},
        {"WindowsXP_64",    ":/vbox-icons/os_winxp_64.png"},
        {"Windows2003",     ":/vbox-icons/os_win2k3.png"},
        {"Windows2003_64",  ":/vbox-icons/os_win2k3_64.png"},
        {"WindowsVista",    ":/vbox-icons/os_winvista.png"},
        {"WindowsVista_64", ":/vbox-icons/os_winvista_64.png"},
        {"Windows2008",     ":/vbox-icons/os_win2k8.png"},
        {"Windows2008_64",  ":/vbox-icons/os_win2k8_64.png"},
        {"Windows7",        ":/vbox-icons/os_win7.png"},
        {"Windows7_64",     ":/vbox-icons/os_win7_64.png"},
        {"WindowsNT",       ":/vbox-icons/os_win_other.png"},
        {"OS2Warp3",        ":/vbox-icons/os_os2warp3.png"},
        {"OS2Warp4",        ":/vbox-icons/os_os2warp4.png"},
        {"OS2Warp45",       ":/vbox-icons/os_os2warp45.png"},
        {"OS2eCS",          ":/vbox-icons/os_os2ecs.png"},
        {"OS2",             ":/vbox-icons/os_os2_other.png"},
        {"Linux22",         ":/vbox-icons/os_linux22.png"},
        {"Linux24",         ":/vbox-icons/os_linux24.png"},
        {"Linux24_64",      ":/vbox-icons/os_linux24_64.png"},
        {"Linux26",         ":/vbox-icons/os_linux26.png"},
        {"Linux26_64",      ":/vbox-icons/os_linux26_64.png"},
        {"ArchLinux",       ":/vbox-icons/os_archlinux.png"},
        {"ArchLinux_64",    ":/vbox-icons/os_archlinux_64.png"},
        {"Debian",          ":/vbox-icons/os_debian.png"},
        {"Debian_64",       ":/vbox-icons/os_debian_64.png"},
        {"OpenSUSE",        ":/vbox-icons/os_opensuse.png"},
        {"OpenSUSE_64",     ":/vbox-icons/os_opensuse_64.png"},
        {"Fedora",          ":/vbox-icons/os_fedora.png"},
        {"Fedora_64",       ":/vbox-icons/os_fedora_64.png"},
        {"Gentoo",          ":/vbox-icons/os_gentoo.png"},
        {"Gentoo_64",       ":/vbox-icons/os_gentoo_64.png"},
        {"Mandriva",        ":/vbox-icons/os_mandriva.png"},
        {"Mandriva_64",     ":/vbox-icons/os_mandriva_64.png"},
        {"RedHat",          ":/vbox-icons/os_redhat.png"},
        {"RedHat_64",       ":/vbox-icons/os_redhat_64.png"},
        {"Ubuntu",          ":/vbox-icons/os_ubuntu.png"},
        {"Ubuntu_64",       ":/vbox-icons/os_ubuntu_64.png"},
        {"Xandros",         ":/vbox-icons/os_xandros.png"},
        {"Xandros_64",      ":/vbox-icons/os_xandros_64.png"},
        {"Linux",           ":/vbox-icons/os_linux_other.png"},
        {"FreeBSD",         ":/vbox-icons/os_freebsd.png"},
        {"FreeBSD_64",      ":/vbox-icons/os_freebsd_64.png"},
        {"OpenBSD",         ":/vbox-icons/os_openbsd.png"},
        {"OpenBSD_64",      ":/vbox-icons/os_openbsd_64.png"},
        {"NetBSD",          ":/vbox-icons/os_netbsd.png"},
        {"NetBSD_64",       ":/vbox-icons/os_netbsd_64.png"},
        {"Solaris",         ":/vbox-icons/os_solaris.png"},
        {"Solaris_64",      ":/vbox-icons/os_solaris_64.png"},
        {"OpenSolaris",     ":/vbox-icons/os_opensolaris.png"},
        {"OpenSolaris_64",  ":/vbox-icons/os_opensolaris_64.png"},
        {"QNX",             ":/vbox-icons/os_qnx.png"},
    };

    for (size_t n = 0; n < sizeof (kOSTypeIcons) / sizeof(kOSTypeIcons[0]); ++ n)
    {
        osIcons.insert (QL1S(kOSTypeIcons [n][0]), (QL1S(kOSTypeIcons [n][1])));
    }
}

void VirtualBoxProvider::rebuild()
{
    QDomDocument d;
    if ( !d.setContent( &fp ) )
    {
        qDebug() << "Unable to parse: " << fp.fileName();
        return;
    }

    QDomNodeList _dnlist = d.elementsByTagName( QSL("MachineEntry") );
    for ( int i = 0; i < _dnlist.count(); i++ )
    {
        const QDomNode & node = _dnlist.at( i );
        const QString & ref = node.toElement().attribute( QSL("src") );
        if ( ref.isEmpty() )
        {
            qDebug() << "MachineEntry with no src attribute";
            continue;
        }

        QFile m( ref );

        QDomDocument mspec;
        if ( !mspec.setContent( &m ) )
        {
            qDebug() << "Could not parse machine file " << m.fileName();
            continue;
        }

        QDomNodeList _mlist = mspec.elementsByTagName( QSL("Machine") );
        for ( int j = 0; j < _mlist.count(); j++ )
        {
         QDomNode mnode = _mlist.at( j );

         QString type = mnode.toElement().attribute( QSL("OSType") );
         VirtualBoxItem *virtualBoxItem = new VirtualBoxItem
            (
             mnode.toElement().attribute( QSL("name") ) ,
             QIcon ( osIcons.value (type , QSL(":/vbox-icons/os_other.png")) )
            );

         const QDomNodeList & rdeportConfig = mnode.toElement().elementsByTagName(QSL("VRDEProperties"));
         if ( ! rdeportConfig.isEmpty() )
         {
            QDomNode portNode = rdeportConfig.at(0).firstChild();
            virtualBoxItem->setRDEPort( portNode.toElement().attribute(QSL("value")) );
         }

         append ( virtualBoxItem );
        }
   }

    timeStamp = QDateTime::currentDateTime();

}

bool VirtualBoxProvider::isOutDated() const
{
    return fp.exists() && ( timeStamp < QFileInfo ( virtualBoxConfig ).lastModified () );
}
#endif


#ifdef MATH_ENABLED
#include <muParser.h>

class MathItem::Parser : private mu::Parser
{
public:
    Parser()
        : mLocale(QLocale::system())
    {
        try
        {
            // use the system's locale instead of the "C"
            const std::locale loc{""};
            auto && numpunct = std::use_facet<std::numpunct<mu::char_type>>(loc);
            SetDecSep(numpunct.decimal_point());
            SetThousandsSep(0); // means no grouping for muparser API

            const char arg_sep = GetArgSep();
            if (numpunct.decimal_point() == arg_sep)
            {
                if (arg_sep == ',')
                    SetArgSep(';');
                else
                    SetArgSep(',');
            }
        } catch (const std::runtime_error & e)
        {
            qWarning().noquote() << "Unable to set locale for Math, " << e.what();
        }

        // do not group in output
        mLocale.setNumberOptions(mLocale.numberOptions() | QLocale::OmitGroupSeparator);
    }


    QString evalString(const QString & s)
    {
        SetExpr(s.toStdString());
        return mLocale.toString(Eval()); // can throw
    }

private:
    QLocale mLocale;
};

/************************************************

 ************************************************/
MathItem::MathItem():
        CommandProviderItem(),
        mParser{new Parser}
{
    mToolTip =QObject::tr("Mathematics");
    mIcon = QIcon::fromTheme(QSL("accessories-calculator"));
}


/************************************************

 ************************************************/
MathItem::~MathItem()
{
}


/************************************************

 ************************************************/
bool MathItem::run() const
{
    return false;
}


/************************************************

 ************************************************/
bool MathItem::compare(const QRegExp &regExp) const
{
    QString s = regExp.pattern().trimmed();

    bool is_math = false;
    if (s.startsWith(QLatin1Char('=')))
    {
        is_math = true;
        s.remove(0, 1);
    }
    if (s.endsWith(QLatin1Char('=')))
    {
        is_math = true;
        s.chop(1);
    }

    if (is_math)
    {
        if (s != mCachedInput)
        {
            MathItem * self = const_cast<MathItem*>(this);
            mCachedInput = s;
            self->mTitle.clear();

            //try to compute anything suitable
            for (int attempts = 20; 0 < attempts && 0 < s.size(); s.chop(1), --attempts)
            {
                try
                {
                    self->mTitle = s + QL1C('=') + mParser->evalString(s);
                    break;
                } catch (const mu::Parser::exception_type & e)
                {
                    //don't do anything, return false -> no result will be showed
                }
            }
        }

        return !mTitle.isEmpty();
    }

    return false;
}

/************************************************

 ************************************************/
unsigned int MathItem::rank(const QString &pattern) const
{
    return stringRank(mTitle, pattern);
}


/************************************************

 ************************************************/
MathProvider::MathProvider()
{
    append(new MathItem());
}

#endif

ExternalProviderItem::ExternalProviderItem()
{
}

bool ExternalProviderItem::setData(QMap<QString,QString> & data)
{
    if (! (data.contains(QL1S("title")) && data.contains(QL1S("dialog/monitor"))))
    {
        return false;
    }

    mTitle = data[QLatin1String("title")];
    mComment = data[QLatin1String("comment")];
    mToolTip = data[QSL("tooltip")];
    mCommand = data[QL1S("dialog/monitor")];
    if (data.contains(QL1S("icon")))
        mIcon = QIcon::fromTheme(data[QL1S("icon")]);

    return true;
}



bool ExternalProviderItem::run() const
{
    return startProcess(mCommand);
}

unsigned int ExternalProviderItem::rank(const QString& pattern) const
{
    return stringRank(title(), pattern);
}

ExternalProvider::ExternalProvider(const QString name, const QString externalProgram) :
CommandProvider(), mName(name)
{
    mExternalProcess = new QProcess(this);
    mYamlParser = new YamlParser();
    connect(mYamlParser, SIGNAL(newListOfMaps(QList<QMap<QString, QString> >)),
            this,        SLOT(newListOfMaps(QList<QMap<QString, QString> >)));

    connect(mExternalProcess, SIGNAL(readyRead()), this, SLOT(readFromProcess()));
    mExternalProcess->start(externalProgram, QStringList());
}

void ExternalProvider::setSearchTerm(const QString searchTerm)
{
    mExternalProcess->write(searchTerm.toUtf8());
    mExternalProcess->write(QBAL("\n"));
//    mExternalProcess->write(QString("\n").toUtf8());
}

void ExternalProvider::newListOfMaps(QList<QMap<QString,QString> > maps)
{
    emit aboutToBeChanged();

    qDeleteAll(*this);
    clear();

    for(auto map : qAsConst(maps))
    {
        ExternalProviderItem *item  = new ExternalProviderItem();
        if (item->setData(map))
            this->append(item);
        else
            delete item;
    }

    emit changed();
}

void ExternalProvider::readFromProcess()
{
    char ch;
    while (mExternalProcess->getChar(&ch))
    {
        if (ch == '\n')
        {
            QString textLine = QString::fromLocal8Bit(mBuffer);
            mYamlParser->consumeLine(textLine);
            mBuffer.clear();
        }
        else
        {
            mBuffer.append(ch);
        }
    }
}

