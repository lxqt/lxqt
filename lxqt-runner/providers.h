/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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


#ifndef PROVIDERS_H
#define PROVIDERS_H

#include <QList>
#include <QRegExp>
#include <QDomElement>
#include <QString>
#include <QIcon>

#ifdef HAVE_MENU_CACHE
#include <menu-cache.h>
#endif

#define MAX_RANK 0xFFFF

/*! The CommandProviderItem class provides an item for use with CommandProvider.
    Items usually contain title, comment, toolTip and icon.
 */
class CommandProviderItem: public QObject
{
    Q_OBJECT
public:
    CommandProviderItem(): QObject() {}
    virtual ~CommandProviderItem() {}

    virtual bool run() const = 0;
    virtual bool compare(const QRegExp &regExp) const = 0;

    /// Returns the item's icon.
    QIcon icon() const { return mIcon; }

    /// Returns the item's title.
    QString title() const { return mTitle; }

    /// Returns the item's comment.
    QString comment() const { return mComment; }

    /// Returns the item's tooltip.
    QString toolTip() const { return mToolTip; }

    /*! The result of this function is used when searching for a apropriate item.
        The item with the highest rank will be highlighted.
            0 - not suitable.
            MAX_RANK - an exact match.
        In the most cases you can yse something like:
          return stringRank(mTitle, pattern);
     */
    virtual unsigned int rank(const QString &pattern) const  = 0;

protected:
    /// Helper function for the CommandProviderItem::rank
    unsigned int stringRank(const QString str, const QString pattern) const;
    QIcon   mIcon;
    QString mTitle;
    QString mComment;
    QString mToolTip;
};


/*! The CommandProvider class provides task for the lxqt-runner.
 */
class CommandProvider: public QObject, public QList<CommandProviderItem*>
{
    Q_OBJECT
public:
    CommandProvider();
    virtual ~CommandProvider();

    virtual void rebuild() {}
    virtual bool isOutDated() const { return false; }

signals:
    void aboutToBeChanged();
    void changed();
};


/************************************************
 * Application desktop files
 ************************************************/

class AppLinkItem: public CommandProviderItem
{
    Q_OBJECT
public:
    AppLinkItem(const QDomElement &element);

#ifdef HAVE_MENU_CACHE
    AppLinkItem(MenuCacheApp* app);
#endif

    bool run() const;
    bool compare(const QRegExp &regExp) const;
    QString command() const { return mCommand; }
    QString exec() const { return mExec; }

    void operator=(const AppLinkItem &other);

    virtual unsigned int rank(const QString &pattern) const;
private slots:
    void updateIcon();
private:
    void initExec();
private:
    QString mDesktopFile;
    QString mIconName;
    QString mCommand;
    QString mExec; //!< the expanded executable (full path) from desktop file Exec key
    QString mProgram;
};


class XdgMenu;
class AppLinkProvider: public CommandProvider
{
    Q_OBJECT
public:
    AppLinkProvider();
    virtual ~AppLinkProvider();

private slots:
    void update();

private:
#ifdef HAVE_MENU_CACHE
    MenuCache* mMenuCache;
    MenuCacheNotifyId mMenuCacheNotify;
    static void menuCacheReloadNotify(MenuCache* cache, gpointer user_data);
#else
    XdgMenu *mXdgMenu;
#endif
};


/************************************************
 * History
 ************************************************/

class HistoryItem: public CommandProviderItem
{
public:
    HistoryItem(const QString &command);

    bool run() const;
    bool compare(const QRegExp &regExp) const;

    QString command() const { return mCommand; }
    virtual unsigned int rank(const QString &pattern) const;

private:
    QString mCommand;
};



class QSettings;
class HistoryProvider: public CommandProvider
{
public:
    HistoryProvider();
    virtual ~HistoryProvider();

    void AddCommand(const QString &command);
    void clearHistory();

private:
    QSettings *mHistoryFile;
};



/************************************************
 * Custom command
 ************************************************/
class CustomCommandProvider;

class CustomCommandItem: public CommandProviderItem
{
    Q_OBJECT

public:
    CustomCommandItem(CustomCommandProvider *provider);

    bool run() const;
    bool compare(const QRegExp &regExp) const;

    QString command() const { return mCommand; }
    void setCommand(const QString &command);
    QString exec() const { return mExec; }

    virtual unsigned int rank(const QString &pattern) const;
private:
    QString mCommand;
    QString mExec; //!< the expanded executable (full path)
    CustomCommandProvider *mProvider;
};



class QSettings;
class CustomCommandProvider: public CommandProvider
{
public:
    CustomCommandProvider();

    QString command() const { return mItem->command(); }
    void setCommand(const QString &command) { mItem->setCommand(command); }

    HistoryProvider* historyProvider() const { return mHistoryProvider; }
    void setHistoryProvider(HistoryProvider *historyProvider) { mHistoryProvider = historyProvider; }

private:
    CustomCommandItem *mItem;
    HistoryProvider *mHistoryProvider;
};



#ifdef MATH_ENABLED
/************************************************
 * Mathematics
 ************************************************/
class MathItem: public CommandProviderItem
{
public:
    class Parser;
public:
    MathItem();
    ~MathItem();

    bool run() const;
    bool compare(const QRegExp &regExp) const;
    virtual unsigned int rank(const QString &pattern) const;
private:
    QScopedPointer<Parser> mParser;
    mutable QString mCachedInput;
};



class MathProvider: public CommandProvider
{
public:
    MathProvider();
    //virtual ~MathProvider();
};
#endif

#ifdef VBOX_ENABLED
#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>
#include <QMap>

class VirtualBoxItem: public CommandProviderItem
{
public:
  VirtualBoxItem(const QString & MachineName , const QIcon & Icon);

  void setRDEPort (const QString & portNum);
  bool run() const;
  bool compare(const QRegExp &regExp) const;
  virtual unsigned int rank(const QString &pattern) const;
private:
  QString m_rdePortNum;
};

class VirtualBoxProvider: public CommandProvider
{
public:
  VirtualBoxProvider ();
  void rebuild();
  bool isOutDated() const;

private:
  QFile fp;
  QMap<QString,QString> osIcons;
  QString virtualBoxConfig;
  QDateTime timeStamp;
};
#endif

class ExternalProviderItem: public CommandProviderItem
{
    Q_OBJECT

public:
    ExternalProviderItem();

    bool setData(QMap<QString, QString> & data);

    bool run() const;
    bool compare(const QRegExp & /*regExp*/) const {return true;} // We leave the decision to the external process
    unsigned int rank(const QString &pattern) const;

    QString mCommand;
};

class QProcess;
class YamlParser;
class ExternalProvider: public CommandProvider
{
    Q_OBJECT

public:
    ExternalProvider(const QString name, const QString externalProgram);

    void setSearchTerm(const QString searchTerm);

signals:
    void dataChanged();

private slots:
    void readFromProcess();
    void newListOfMaps(QList<QMap<QString, QString> > maps);

private:
    QString mName;
    QProcess *mExternalProcess;
    QTextStream *mDataToProcess;
    YamlParser *mYamlParser;

    QByteArray mBuffer;
};


#endif // PROVIDERS_H
