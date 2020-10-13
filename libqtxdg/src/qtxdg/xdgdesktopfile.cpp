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

// clazy:excludeall=non-pod-global-static

#include "desktopenvironment_p.cpp"
#include "xdgdesktopfile.h"
#include "xdgdesktopfile_p.h"
#include "xdgdirs.h"
#include "xdgicon.h"
#include "application_interface.h" // generated interface for DBus org.freedesktop.Application
#include "xdgmimeapps.h"

#include <cstdlib>
#include <unistd.h>

#include <QDebug>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDesktopServices>
#include <QDir>
#include <QSharedData>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QtAlgorithms>


/**
 *  See: http://standards.freedesktop.org/desktop-entry-spec
 */

// A list of executables that can't be run with QProcess::startDetached(). They
// will be run with QProcess::start()
static const QStringList nonDetachExecs = QStringList()
    << QLatin1String("pkexec");

static const QLatin1String onlyShowInKey("OnlyShowIn");
static const QLatin1String notShowInKey("NotShowIn");
static const QLatin1String categoriesKey("Categories");
static const QLatin1String actionsKey("Actions");
static const QLatin1String extendPrefixKey("X-");
static const QLatin1String mimeTypeKey("MimeType");
static const QLatin1String applicationsStr("applications");

static const QLatin1String nameKey("Name");
static const QLatin1String typeKey("Type");
static const QLatin1String ApplicationStr("Application");
static const QLatin1String LinkStr("Link");
static const QLatin1String DirectoryStr("Directory");
static const QLatin1String execKey("Exec");
static const QLatin1String urlKey("URL");
static const QLatin1String iconKey("Icon");

static const QLatin1String initialPreferenceKey("InitialPreference");

// Helper functions prototypes
bool checkTryExec(const QString& progName);
QString &doEscape(QString& str, const QHash<QChar,QChar> &repl);
QString &doUnEscape(QString& str, const QHash<QChar,QChar> &repl);
QString &escape(QString& str);
QString &escapeExec(QString& str);
QString expandDynamicUrl(QString url);
QString expandEnvVariables(const QString str);
QStringList expandEnvVariables(const QStringList strs);
QString findDesktopFile(const QString& dirName, const QString& desktopName);
QString findDesktopFile(const QString& desktopName);
static QStringList parseCombinedArgString(const QString &program);
bool read(const QString &prefix);
void replaceVar(QString &str, const QString &varName, const QString &after);
QString &unEscape(QString& str);
QString &unEscapeExec(QString& str);

namespace
{
    //! Simple helper for getting timeout for starting of DBus activatable applications
    class DBusActivateTimeout
    {
    private:
        int mTimeoutMs;
        DBusActivateTimeout()
        {
            bool ok;
            mTimeoutMs = qEnvironmentVariableIntValue("QTXDG_DBUSACTIVATE_TIMEOUT", &ok);
            if (!ok)
                mTimeoutMs = 1500;
        }
        static DBusActivateTimeout msInstance;
    public:
        static const DBusActivateTimeout & instance() { return msInstance; }
        operator int() const { return mTimeoutMs; }
    };
    DBusActivateTimeout DBusActivateTimeout::msInstance;
}

QString &doEscape(QString& str, const QHash<QChar,QChar> &repl)
{
    // First we replace slash.
    str.replace(QLatin1Char('\\'), QLatin1String("\\\\"));

    QHashIterator<QChar,QChar> i(repl);
    while (i.hasNext()) {
        i.next();
        if (i.key() != QLatin1Char('\\'))
            str.replace(i.key(), QString::fromLatin1("\\\\%1").arg(i.value()));
    }

    return str;
}

/************************************************
 The escape sequences \s, \n, \t, \r, and \\ are supported for values
 of type string and localestring, meaning ASCII space, newline, tab,
 carriage return, and backslash, respectively.
 ************************************************/
QString &escape(QString& str)
{
    QHash<QChar,QChar> repl;
    repl.insert(QLatin1Char('\n'),  QLatin1Char('n'));
    repl.insert(QLatin1Char('\t'),  QLatin1Char('t'));
    repl.insert(QLatin1Char('\r'),  QLatin1Char('r'));

    return doEscape(str, repl);
}


/************************************************
 Quoting must be done by enclosing the argument between double quotes and
 escaping the
    double quote character,
    backtick character ("`"),
    dollar sign ("$") and
    backslash character ("\")
by preceding it with an additional backslash character.
Implementations must undo quoting before expanding field codes and before
passing the argument to the executable program.

Note that the general escape rule for values of type string states that the
backslash character can be escaped as ("\\") as well and that this escape
rule is applied before the quoting rule. As such, to unambiguously represent a
literal backslash character in a quoted argument in a desktop entry file
requires the use of four successive backslash characters ("\\\\").
Likewise, a literal dollar sign in a quoted argument in a desktop entry file
is unambiguously represented with ("\\$").
 ************************************************/
QString &escapeExec(QString& str)
{
    QHash<QChar,QChar> repl;
    // The parseCombinedArgString() splits the string by the space symbols,
    // we temporarily replace them on the special characters.
    // Replacement will reverse after the splitting.
    repl.insert(QLatin1Char('"'), QLatin1Char('"'));    // double quote,
    repl.insert(QLatin1Char('\''), QLatin1Char('\''));  // single quote ("'"),
    repl.insert(QLatin1Char('\\'), QLatin1Char('\\'));  // backslash character ("\"),
    repl.insert(QLatin1Char('$'), QLatin1Char('$'));    // dollar sign ("$"),

    return doEscape(str, repl);
}


QString &doUnEscape(QString& str, const QHash<QChar,QChar> &repl)
{
    int n = 0;
    while (true)
    {
        n=str.indexOf(QLatin1String("\\"), n);
        if (n < 0 || n > str.length() - 2)
            break;

        if (repl.contains(str.at(n+1)))
        {
            str.replace(n, 2, repl.value(str.at(n+1)));
        }

        n++;
    }

    return str;
}


/************************************************
 The escape sequences \s, \n, \t, \r, and \\ are supported for values
 of type string and localestring, meaning ASCII space, newline, tab,
 carriage return, and backslash, respectively.
 ************************************************/
QString &unEscape(QString& str)
{
    QHash<QChar,QChar> repl;
    repl.insert(QLatin1Char('\\'), QLatin1Char('\\'));
    repl.insert(QLatin1Char('s'),  QLatin1Char(' '));
    repl.insert(QLatin1Char('n'),  QLatin1Char('\n'));
    repl.insert(QLatin1Char('t'),  QLatin1Char('\t'));
    repl.insert(QLatin1Char('r'),  QLatin1Char('\r'));

    return doUnEscape(str, repl);
}


/************************************************
 Quoting must be done by enclosing the argument between double quotes and
 escaping the
    double quote character,
    backtick character ("`"),
    dollar sign ("$") and
    backslash character ("\")
by preceding it with an additional backslash character.
Implementations must undo quoting before expanding field codes and before
passing the argument to the executable program.

Reserved characters are
    space (" "),
    tab,
    newline,
    double quote,
    single quote ("'"),
    backslash character ("\"),
    greater-than sign (">"),
    less-than sign ("<"),
    tilde ("~"),
    vertical bar ("|"),
    ampersand ("&"),
    semicolon (";"),
    dollar sign ("$"),
    asterisk ("*"),
    question mark ("?"),
    hash mark ("#"),
    parenthesis ("(") and (")")
    backtick character ("`").

Note that the general escape rule for values of type string states that the
backslash character can be escaped as ("\\") as well and that this escape
rule is applied before the quoting rule. As such, to unambiguously represent a
literal backslash character in a quoted argument in a desktop entry file
requires the use of four successive backslash characters ("\\\\").
Likewise, a literal dollar sign in a quoted argument in a desktop entry file
is unambiguously represented with ("\\$").
 ************************************************/
QString &unEscapeExec(QString& str)
{
    unEscape(str);
    QHash<QChar,QChar> repl;
    // The parseCombinedArgString() splits the string by the space symbols,
    // we temporarily replace them on the special characters.
    // Replacement will reverse after the splitting.
    repl.insert(QLatin1Char(' '),  01);    // space
    repl.insert(QLatin1Char('\t'), 02);    // tab
    repl.insert(QLatin1Char('\n'), 03);    // newline,

    repl.insert(QLatin1Char('"'), QLatin1Char('"'));    // double quote,
    repl.insert(QLatin1Char('\''), QLatin1Char('\''));  // single quote ("'"),
    repl.insert(QLatin1Char('\\'), QLatin1Char('\\'));  // backslash character ("\"),
    repl.insert(QLatin1Char('>'), QLatin1Char('>'));    // greater-than sign (">"),
    repl.insert(QLatin1Char('<'), QLatin1Char('<'));    // less-than sign ("<"),
    repl.insert(QLatin1Char('~'), QLatin1Char('~'));    // tilde ("~"),
    repl.insert(QLatin1Char('|'), QLatin1Char('|'));    // vertical bar ("|"),
    repl.insert(QLatin1Char('&'), QLatin1Char('&'));    // ampersand ("&"),
    repl.insert(QLatin1Char(';'), QLatin1Char(';'));    // semicolon (";"),
    repl.insert(QLatin1Char('$'), QLatin1Char('$'));    // dollar sign ("$"),
    repl.insert(QLatin1Char('*'), QLatin1Char('*'));    // asterisk ("*"),
    repl.insert(QLatin1Char('?'), QLatin1Char('?'));    // question mark ("?"),
    repl.insert(QLatin1Char('#'), QLatin1Char('#'));    // hash mark ("#"),
    repl.insert(QLatin1Char('('), QLatin1Char('('));    // parenthesis ("(")
    repl.insert(QLatin1Char(')'), QLatin1Char(')'));    // parenthesis (")")
    repl.insert(QLatin1Char('`'), QLatin1Char('`'));    // backtick character ("`").

    return doUnEscape(str, repl);
}

namespace
{
    /*!
     * Helper class for getting the keys for "Additional applications actions"
     * ([Desktop Action %s] sections)
     */
    class XdgDesktopAction : public XdgDesktopFile
    {
    public:
        XdgDesktopAction(const XdgDesktopFile & parent, const QString & action)
            : XdgDesktopFile(parent)
            , m_prefix(QString{QLatin1String("Desktop Action %1")}.arg(action))
        {}

    protected:
        QString prefix() const override { return m_prefix; }

    private:
        const QString m_prefix;
    };
}

class XdgDesktopFileData: public QSharedData {
public:
    XdgDesktopFileData();

    inline void clear() {
        mFileName.clear();
        mIsValid = false;
        mValidIsChecked = false;
        mIsShow.clear();
        mItems.clear();
        mType = XdgDesktopFile::UnknownType;
    }
    bool read(const QString &prefix);
    XdgDesktopFile::Type detectType(XdgDesktopFile *q) const;
    bool startApplicationDetached(const XdgDesktopFile *q, const QString & action, const QStringList& urls) const;
    bool startLinkDetached(const XdgDesktopFile *q) const;
    bool startByDBus(const QString & action, const QStringList& urls) const;
    QStringList getListValue(const XdgDesktopFile * q, const QString & key, bool tryExtendPrefix) const;

    QString mFileName;
    bool mIsValid;
    mutable bool mValidIsChecked;
    mutable QHash<QString, bool> mIsShow;
    QMap<QString, QVariant> mItems;

    XdgDesktopFile::Type mType;
};


XdgDesktopFileData::XdgDesktopFileData():
    mFileName(),
    mIsValid(false),
    mValidIsChecked(false),
    mIsShow(),
    mItems(),
    mType(XdgDesktopFile::UnknownType)
{
}


bool XdgDesktopFileData::read(const QString &prefix)
{
    QFile file(mFileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QString section;
    QTextStream stream(&file);
    bool prefixExists = false;
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        // Skip comments ......................
        if (line.startsWith(QLatin1Char('#')))
            continue;


        // Section ..............................
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']')))
        {
            section = line.mid(1, line.length()-2);
            if (section == prefix)
                prefixExists = true;

            continue;
        }

        QString key = line.section(QLatin1Char('='), 0, 0).trimmed();
        QString value = line.section(QLatin1Char('='), 1).trimmed();

        if (key.isEmpty())
            continue;

        mItems[section + QLatin1Char('/') + key] = QVariant(value);
    }


    // Not check for empty prefix
    mIsValid = (prefix.isEmpty()) || prefixExists;
    return mIsValid;
}

XdgDesktopFile::Type XdgDesktopFileData::detectType(XdgDesktopFile *q) const
{
    QString typeStr = q->value(typeKey).toString();
    if (typeStr == ApplicationStr)
        return XdgDesktopFile::ApplicationType;

    if (typeStr == LinkStr)
        return XdgDesktopFile::LinkType;

    if (typeStr == DirectoryStr)
        return XdgDesktopFile::DirectoryType;

    if (!q->value(execKey).toString().isEmpty())
        return XdgDesktopFile::ApplicationType;

    return XdgDesktopFile::UnknownType;
}

bool XdgDesktopFileData::startApplicationDetached(const XdgDesktopFile *q, const QString & action, const QStringList& urls) const
{
    //DBusActivatable handling
    if (q->value(QLatin1String("DBusActivatable"), false).toBool()) {
        /* WARNING: We fallback to use Exec when the DBusActivatable fails.
         *
         * This is a violation of the standard and we know it!
         *
         * From the Standard:
         * DBusActivatable	A boolean value specifying if D-Bus activation is
         * supported for this application. If this key is missing, the default
         * value is false. If the value is true then implementations should
         * ignore the Exec key and send a D-Bus message to launch the
         * application. See D-Bus Activation for more information on how this
         * works. Applications should still include Exec= lines in their desktop
         * files for compatibility with implementations that do not understand
         * the DBusActivatable key.
         *
         * So, why are we doing it ? In the benefit of user experience.
         * We first ignore the Exec line and in use the D-Bus to lauch the
         * application. But if it fails, we try the Exec method.
         *
         * We consider that this violation is more acceptable than an failure
         * in launching an application.
         */
        if (startByDBus(action, urls))
            return true;
    }
    QStringList args = action.isEmpty()
        ? q->expandExecString(urls)
        : XdgDesktopAction{*q, action}.expandExecString(urls);

    if (args.isEmpty())
        return false;

    if (q->value(QLatin1String("Terminal")).toBool())
    {
        QString term = QString::fromLocal8Bit(qgetenv("TERM"));
        if (term.isEmpty())
            term = QLatin1String("xterm");

        args.prepend(QLatin1String("-e"));
        args.prepend(term);
    }

    bool nonDetach = false;
    for (const QString &s : nonDetachExecs)
    {
        for (const QString &a : qAsConst(args))
        {
            if (a.contains(s))
            {
                nonDetach = true;
            }
        }
    }

    QString cmd = args.takeFirst();
    QString workingDir = q->value(QLatin1String("Path")).toString();
    if (!workingDir.isEmpty() && !QDir(workingDir).exists())
	    workingDir = QString();

    if (nonDetach)
    {
        QScopedPointer<QProcess> p(new QProcess);
        p->setStandardInputFile(QProcess::nullDevice());
        p->setProcessChannelMode(QProcess::ForwardedChannels);
        if (!workingDir.isEmpty())
            p->setWorkingDirectory(workingDir);
        p->start(cmd, args);
        bool started = p->waitForStarted();
        if (started)
        {
            QProcess* proc = p.take(); //release the pointer(will be selfdestroyed upon finish)
            QObject::connect(proc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                proc, &QProcess::deleteLater);
        }
        return started;
    }
    else
    {
        return QProcess::startDetached(cmd, args, workingDir);
    }
}


bool XdgDesktopFileData::startLinkDetached(const XdgDesktopFile *q) const
{
    QString url = q->url();

    if (url.isEmpty())
    {
        qWarning() << "XdgDesktopFileData::startLinkDetached: url is empty.";
        return false;
    }

    QString scheme = QUrl(url).scheme();

    if (scheme.isEmpty() || scheme == QLatin1String("file"))
    {
        // Local file
        QFileInfo fi(url);

        QMimeDatabase db;
        XdgMimeApps appsDb;
        QMimeType mimeInfo = db.mimeTypeForFile(fi);
        XdgDesktopFile* desktopFile = appsDb.defaultApp(mimeInfo.name());

        if (desktopFile)
            return desktopFile->startDetached(url);
    }
    else
    {
        // Internet URL
        return QDesktopServices::openUrl(QUrl::fromEncoded(url.toLocal8Bit()));
    }

    return false;
}

bool XdgDesktopFileData::startByDBus(const QString & action, const QStringList& urls) const
{
    QFileInfo f(mFileName);
    QString path(f.completeBaseName());
    path = path.replace(QLatin1Char('.'), QLatin1Char('/')).prepend(QLatin1Char('/'));

    QVariantMap platformData;
    platformData.insert(QLatin1String("desktop-startup-id"), QString::fromLocal8Bit(qgetenv("DESKTOP_STARTUP_ID")));

    QDBusObjectPath d_path(path);
    if (d_path.path().isEmpty())
    {
        qWarning() << "XdgDesktopFileData::startByDBus: invalid name" << f.fileName() << "of DBusActivatable .desktop file"
                ", assembled DBus object path" << path << "is invalid!";
        return false;
    }
    org::freedesktop::Application app{f.completeBaseName(), path, QDBusConnection::sessionBus()};
    //Note: after the QDBusInterface construction, it can *invalid* (has reasonable lastError())
    // but this can be due to some intermediate DBus call(s) which doesn't need to be fatal and
    // our next call() can succeed
    // see discussion https://github.com/lxqt/libqtxdg/pull/75
    if (app.lastError().isValid())
    {
        qWarning().noquote() << "XdgDesktopFileData::startByDBus: invalid interface:" << app.lastError().message()
            << ", but trying to continue...";
    }
    app.setTimeout(DBusActivateTimeout::instance());
    QDBusPendingReply<> reply;
    if (!action.isEmpty())
    {
        QList<QVariant> v_urls;
        for (const auto & url : urls)
             v_urls.append(url);
        reply = app.ActivateAction(action, v_urls, platformData);
    } else if (urls.isEmpty())
        reply = app.Activate(platformData);
    else
        reply = app.Open(urls, platformData);

    reply.waitForFinished();
    if (QDBusMessage::ErrorMessage == reply.reply().type())
    {
        qWarning().noquote().nospace() << "XdgDesktopFileData::startByDBus(timeout=" << DBusActivateTimeout::instance()
            << "): failed to start org.freedesktop.Application" << mFileName << ": " << reply.reply();
        return false;
    }

    return true;
}

QStringList XdgDesktopFileData::getListValue(const XdgDesktopFile * q, const QString & key, bool tryExtendPrefix) const
{
    QString used_key = key;
    if (!q->contains(used_key) && tryExtendPrefix)
    {
        used_key = extendPrefixKey + key;
        if (!q->contains(used_key))
            return QStringList();
    }

    return q->value(used_key).toString().split(QLatin1Char(';'), QString::SkipEmptyParts);
}


XdgDesktopFile::XdgDesktopFile():
    d(new XdgDesktopFileData)
{
}


XdgDesktopFile::XdgDesktopFile(const XdgDesktopFile& other):
    d(other.d)
{
}


XdgDesktopFile::XdgDesktopFile(Type type, const QString& name, const QString &value):
    d(new XdgDesktopFileData)
{
    d->mFileName = name + QLatin1String(".desktop");
    d->mType = type;
    setValue(QLatin1String("Version"), QLatin1String("1.0"));
    setValue(nameKey, name);
    if (type == XdgDesktopFile::ApplicationType)
    {
        setValue(typeKey, ApplicationStr);
        setValue(execKey, value);
    }
    else if (type == XdgDesktopFile::LinkType)
    {
        setValue(typeKey, LinkStr);
        setValue(urlKey, value);
    }
    else if (type == XdgDesktopFile::DirectoryType)
    {
        setValue(typeKey, DirectoryStr);
    }
    d->mIsValid = check();
}


XdgDesktopFile::~XdgDesktopFile()
{
}


XdgDesktopFile& XdgDesktopFile::operator=(const XdgDesktopFile& other)
{
    d = other.d;
    return *this;
}


bool XdgDesktopFile::operator==(const XdgDesktopFile &other) const
{
    return d->mItems == other.d->mItems;
}


bool XdgDesktopFile::load(const QString& fileName)
{
    d->clear();
    if (fileName.startsWith(QDir::separator())) { // absolute path
        QFileInfo f(fileName);
        if (f.exists())
            d->mFileName = f.canonicalFilePath();
        else
            return false;
    } else { // relative path
        const QString r = findDesktopFile(fileName);
        if (r.isEmpty())
            return false;
        else
            d->mFileName = r;
    }
    d->read(prefix());
    d->mIsValid = d->mIsValid && check();
    d->mType = d->detectType(this);
    return isValid();
}


bool XdgDesktopFile::save(QIODevice *device) const
{
    QTextStream stream(device);
    QMap<QString, QVariant>::const_iterator i = d->mItems.constBegin();

    QString section;
    while (i != d->mItems.constEnd())
    {
        QString path = i.key();
        QString sect =  path.section(QLatin1Char('/'),0,0);
        if (sect != section)
        {
            section = sect;
            stream << QLatin1Char('[') << section << QLatin1Char(']') << endl;

        }
        QString key = path.section(QLatin1Char('/'), 1);
        stream << key << QLatin1Char('=') << i.value().toString() << endl;
        ++i;
    }
    return true;
}


bool XdgDesktopFile::save(const QString &fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    return save(&file);
}


QVariant XdgDesktopFile::value(const QString& key, const QVariant& defaultValue) const
{
    QString path = (!prefix().isEmpty()) ? prefix() + QLatin1Char('/') + key : key;
    QVariant res = d->mItems.value(path, defaultValue);
    if (res.type() == QVariant::String)
    {
        QString s = res.toString();
        return unEscape(s);
    }

    return res;
}


void XdgDesktopFile::setValue(const QString &key, const QVariant &value)
{
    QString path = (!prefix().isEmpty()) ? prefix() + QLatin1Char('/') + key : key;
    if (value.type() == QVariant::String)
    {

        QString s=value.toString();
        if (key.toUpper() == QLatin1String("EXEC"))
            escapeExec(s);
        else
            escape(s);

        d->mItems[path] = QVariant(s);

        if (key.toUpper() == QLatin1String("TYPE"))
            d->mType = d->detectType(this);
    }
    else
    {
        d->mItems[path] = value;
    }
}


void XdgDesktopFile::setLocalizedValue(const QString &key, const QVariant &value)
{
    setValue(localizedKey(key), value);
}


/************************************************
 LC_MESSAGES value      Possible keys in order of matching
 lang_COUNTRY@MODIFIER  lang_COUNTRY@MODIFIER, lang_COUNTRY, lang@MODIFIER, lang,
                        default value
 lang_COUNTRY           lang_COUNTRY, lang, default value
 lang@MODIFIER          lang@MODIFIER, lang, default value
 lang                   lang, default value
 ************************************************/
QString XdgDesktopFile::localizedKey(const QString& key) const
{
    QString lang = QString::fromLocal8Bit(qgetenv("LC_MESSAGES"));

    if (lang.isEmpty())
        lang = QString::fromLocal8Bit(qgetenv("LC_ALL"));

    if (lang.isEmpty())
         lang = QString::fromLocal8Bit(qgetenv("LANG"));

    QString modifier = lang.section(QLatin1Char('@'), 1);
    if (!modifier.isEmpty())
        lang.truncate(lang.length() - modifier.length() - 1);

    QString encoding = lang.section(QLatin1Char('.'), 1);
    if (!encoding.isEmpty())
        lang.truncate(lang.length() - encoding.length() - 1);


    QString country = lang.section(QLatin1Char('_'), 1);
    if (!country.isEmpty())
        lang.truncate(lang.length() - country.length() - 1);

    //qDebug() << "LC_MESSAGES: " << qgetenv("LC_MESSAGES");
    //qDebug() << "Lang:" << lang;
    //qDebug() << "Country:" << country;
    //qDebug() << "Encoding:" << encoding;
    //qDebug() << "Modifier:" << modifier;


    if (!modifier.isEmpty() && !country.isEmpty())
    {
        QString k = QString::fromLatin1("%1[%2_%3@%4]").arg(key, lang, country, modifier);
        //qDebug() << "\t try " << k << contains(k);
        if (contains(k))
            return k;
    }

    if (!country.isEmpty())
    {
        QString k = QString::fromLatin1("%1[%2_%3]").arg(key, lang, country);
        //qDebug() << "\t try " << k  << contains(k);
        if (contains(k))
            return k;
    }

    if (!modifier.isEmpty())
    {
        QString k = QString::fromLatin1("%1[%2@%3]").arg(key, lang, modifier);
        //qDebug() << "\t try " << k  << contains(k);
        if (contains(k))
            return k;
    }

    QString k = QString::fromLatin1("%1[%2]").arg(key, lang);
    //qDebug() << "\t try " << k  << contains(k);
    if (contains(k))
        return k;


    //qDebug() << "\t try " << key  << contains(key);
    return key;
}


QVariant XdgDesktopFile::localizedValue(const QString& key, const QVariant& defaultValue) const
{
    return value(localizedKey(key), defaultValue);
}


QStringList XdgDesktopFile::categories() const
{
    return d->getListValue(this, categoriesKey, true);
}

QStringList XdgDesktopFile::actions() const
{
    return d->getListValue(this, actionsKey, false);
}

void XdgDesktopFile::removeEntry(const QString& key)
{
    QString path = (!prefix().isEmpty()) ? prefix() + QLatin1Char('/') + key : key;
    d->mItems.remove(path);
}


bool XdgDesktopFile::contains(const QString& key) const
{
    QString path = (!prefix().isEmpty()) ? prefix() + QLatin1Char('/') + key : key;
    return d->mItems.contains(path);
}


bool XdgDesktopFile::isValid() const
{
    return d->mIsValid;
}


QString XdgDesktopFile::fileName() const
{
    return d->mFileName;
}


QIcon const XdgDesktopFile::icon(const QIcon& fallback) const
{
    QIcon result = XdgIcon::fromTheme(value(iconKey).toString(), fallback);

    if (result.isNull() && type() == ApplicationType) {
        result = XdgIcon::fromTheme(QLatin1String("application-x-executable.png"));
        // TODO Maybe defaults for other desktopfile types as well..
    }

    return result;
}


QIcon const XdgDesktopFile::actionIcon(const QString & action, const QIcon& fallback) const
{
    return d->mType == ApplicationType
        ? XdgDesktopAction{*this, action}.icon(icon(fallback))
        : fallback;
}


QString const XdgDesktopFile::iconName() const
{
    return value(iconKey).toString();
}


QString const XdgDesktopFile::actionIconName(const QString & action) const
{
    return d->mType == ApplicationType
        ? XdgDesktopAction{*this, action}.iconName()
        : QString{};
}


QStringList XdgDesktopFile::mimeTypes() const
{
    return value(mimeTypeKey).toString().split(QLatin1Char(';'), QString::SkipEmptyParts);
}


QString XdgDesktopFile::actionName(const QString & action) const
{
    return d->mType == ApplicationType
        ? XdgDesktopAction{*this, action}.name()
        : QString{};
}

XdgDesktopFile::Type XdgDesktopFile::type() const
{
    return d->mType;
}


/************************************************
 Starts the program defined in this desktop file in a new process, and detaches
 from it. Returns true on success; otherwise returns false. If the calling process
 exits, the detached process will continue to live.

 Urls - the list of URLs or files to open, can be empty (app launched without
  argument)
 If the function is successful then *pid is set to the process identifier of the
 started process.
 ************************************************/
bool XdgDesktopFile::startDetached(const QStringList& urls) const
{
    switch(d->mType)
    {
    case ApplicationType:
        return d->startApplicationDetached(this, QString{}, urls);
        break;

    case LinkType:
        return d->startLinkDetached(this);
        break;

    default:
        return false;
    }
}

bool XdgDesktopFile::actionActivate(const QString & action, const QStringList& urls) const
{
    return d->mType == ApplicationType ? d->startApplicationDetached(this, action, urls) : false;
}


/************************************************
 This is an overloaded function.
 ************************************************/
bool XdgDesktopFile::startDetached(const QString& url) const
{
    if (url.isEmpty())
        return startDetached(QStringList());
    else
        return startDetached(QStringList(url));
}


static QStringList parseCombinedArgString(const QString &program)
{
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    // handle quoting. tokens can be surrounded by double quotes
    // "hello world". three consecutive double quotes represent
    // the quote character itself.
    for (int i = 0; i < program.size(); ++i) {
        if (program.at(i) == QLatin1Char('"')) {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += program.at(i);
            }
            continue;
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (!inQuote && program.at(i).isSpace()) {
            if (!tmp.isEmpty()) {
                args += tmp;
                tmp.clear();
            }
        } else {
            tmp += program.at(i);
        }
    }
    if (!tmp.isEmpty())
        args += tmp;

    return args;
}


void replaceVar(QString &str, const QString &varName, const QString &after)
{
    str.replace(QRegularExpression(QString::fromLatin1(R"regexp(\$%1(?!\w))regexp").arg(varName)), after);
    str.replace(QRegularExpression(QString::fromLatin1(R"(\$\{%1\})").arg(varName)), after);
}


QString expandEnvVariables(const QString str)
{
    QString scheme = QUrl(str).scheme();

    if (scheme == QLatin1String("http")   || scheme == QLatin1String("https") || scheme == QLatin1String("shttp") ||
        scheme == QLatin1String("ftp")    || scheme == QLatin1String("ftps")  ||
        scheme == QLatin1String("pop")    || scheme == QLatin1String("pops")  ||
        scheme == QLatin1String("imap")   || scheme == QLatin1String("imaps") ||
        scheme == QLatin1String("mailto") ||
        scheme == QLatin1String("nntp")   ||
        scheme == QLatin1String("irc")    ||
        scheme == QLatin1String("telnet") ||
        scheme == QLatin1String("xmpp")   ||
        scheme == QLatin1String("nfs")
      )
        return str;

    const QString homeDir = QFile::decodeName(qgetenv("HOME"));

    QString res = str;
    res.replace(QRegularExpression(QString::fromLatin1("~(?=$|/)")), homeDir);

    replaceVar(res, QLatin1String("HOME"), homeDir);
    replaceVar(res, QLatin1String("USER"), QString::fromLocal8Bit(qgetenv("USER")));

    replaceVar(res, QLatin1String("XDG_DESKTOP_DIR"),   XdgDirs::userDir(XdgDirs::Desktop));
    replaceVar(res, QLatin1String("XDG_TEMPLATES_DIR"), XdgDirs::userDir(XdgDirs::Templates));
    replaceVar(res, QLatin1String("XDG_DOCUMENTS_DIR"), XdgDirs::userDir(XdgDirs::Documents));
    replaceVar(res, QLatin1String("XDG_MUSIC_DIR"), XdgDirs::userDir(XdgDirs::Music));
    replaceVar(res, QLatin1String("XDG_PICTURES_DIR"), XdgDirs::userDir(XdgDirs::Pictures));
    replaceVar(res, QLatin1String("XDG_VIDEOS_DIR"), XdgDirs::userDir(XdgDirs::Videos));
    replaceVar(res, QLatin1String("XDG_PHOTOS_DIR"), XdgDirs::userDir(XdgDirs::Pictures));

    return res;
}


QStringList expandEnvVariables(const QStringList strs)
{
    QStringList res;
    for (const QString &s : strs)
        res << expandEnvVariables(s);

    return res;
}


QStringList XdgDesktopFile::expandExecString(const QStringList& urls) const
{
    if (d->mType != ApplicationType)
        return QStringList();

    QStringList result;

    QString execStr = value(execKey).toString();
    unEscapeExec(execStr);
    const QStringList tokens = parseCombinedArgString(execStr);

    for (QString token : tokens)
    {
        // The parseCombinedArgString() splits the string by the space symbols,
        // we temporarily replaced them on the special characters.
        // Now we reverse it.
        token.replace(01, QLatin1Char(' '));
        token.replace(02, QLatin1Char('\t'));
        token.replace(03, QLatin1Char('\n'));

        // ----------------------------------------------------------
        // A single file name, even if multiple files are selected.
        if (token == QLatin1String("%f"))
        {
            if (!urls.isEmpty())
                result << expandEnvVariables(urls.at(0));
            continue;
        }

        // ----------------------------------------------------------
        // A list of files. Use for apps that can open several local files at once.
        // Each file is passed as a separate argument to the executable program.
        if (token == QLatin1String("%F"))
        {
            result << expandEnvVariables(urls);
            continue;
        }

        // ----------------------------------------------------------
        // A single URL. Local files may either be passed as file: URLs or as file path.
        if (token == QLatin1String("%u"))
        {
            if (!urls.isEmpty())
            {
                QUrl url;
                url.setUrl(expandEnvVariables(urls.at(0)));
                result << ((!url.toLocalFile().isEmpty()) ? url.toLocalFile() : QString::fromUtf8(url.toEncoded()));
            }
            continue;
        }

        // ----------------------------------------------------------
        // A list of URLs. Each URL is passed as a separate argument to the executable
        // program. Local files may either be passed as file: URLs or as file path.
        if (token == QLatin1String("%U"))
        {
            for (const QString &s : urls)
            {
                QUrl url(expandEnvVariables(s));
                result << ((!url.toLocalFile().isEmpty()) ? url.toLocalFile() : QString::fromUtf8(url.toEncoded()));
            }
            continue;
        }

        // ----------------------------------------------------------
        // The Icon key of the desktop entry expanded as two arguments, first --icon
        // and then the value of the Icon key. Should not expand to any arguments if
        // the Icon key is empty or missing.
        if (token == QLatin1String("%i"))
        {
            QString icon = value(iconKey).toString();
            if (!icon.isEmpty())
                result << QLatin1String("-icon") << icon.replace(QLatin1Char('%'), QLatin1String("%%"));
            continue;
        }


        // ----------------------------------------------------------
        // The translated name of the application as listed in the appropriate Name key
        // in the desktop entry.
        if (token == QLatin1String("%c"))
        {
            result << localizedValue(nameKey).toString().replace(QLatin1Char('%'), QLatin1String("%%"));
            continue;
        }

        // ----------------------------------------------------------
        // The location of the desktop file as either a URI (if for example gotten from
        // the vfolder system) or a local filename or empty if no location is known.
        if (token == QLatin1String("%k"))
        {
            result << fileName().replace(QLatin1Char('%'), QLatin1String("%%"));
            break;
        }

        // ----------------------------------------------------------
        // Deprecated.
        // Deprecated field codes should be removed from the command line and ignored.
        if (token == QLatin1String("%d") || token == QLatin1String("%D") ||
            token == QLatin1String("%n") || token == QLatin1String("%N") ||
            token == QLatin1String("%v") || token == QLatin1String("%m")
            )
        {
            continue;
        }

        // ----------------------------------------------------------
        result << expandEnvVariables(token);
    }

    return result;
}


bool checkTryExec(const QString& progName)
{
    if (progName.startsWith(QDir::separator()))
        return QFileInfo(progName).isExecutable();

    const QStringList dirs = QFile::decodeName(qgetenv("PATH")).split(QLatin1Char(':'));

    for (const QString &dir : dirs)
    {
        if (QFileInfo(QDir(dir), progName).isExecutable())
            return true;
    }

    return false;
}


QString XdgDesktopFile::id(const QString &fileName, bool checkFileExists)
{
    const QFileInfo f(fileName);
    if (checkFileExists) {
        if (!f.exists()) {
            return QString();
        }
    }

    QString id = f.absoluteFilePath();
    const QStringList dataDirs = XdgDirs::dataDirs();

    for (const QString &d : dataDirs) {
        if (id.startsWith(d)) {
            // remove only the first occurence
            id.replace(id.indexOf(d), d.size(), QString());
        }
    }

    const QLatin1Char slash('/');
    const QString s = slash + applicationsStr + slash;
    if (!id.startsWith(s))
        return QString();

    id.replace(id.indexOf(s), s.size(), QString());
    id.replace(slash, QLatin1Char('-'));

    return id;
}


bool XdgDesktopFile::isShown(const QString &environment) const
{
    const QString env = environment.toUpper();

    if (d->mIsShow.contains(env))
        return d->mIsShow.value(env);

    d->mIsShow.insert(env, false);

    // Means "this application exists, but don't display it in the menus".
    if (value(QLatin1String("NoDisplay")).toBool())
        return false;

    // The file is not suitable to the current environment
    if (!isSuitable(true, env))
        return false;

    d->mIsShow.insert(env, true);
    return true;
}


bool XdgDesktopFile::isSuitable(bool excludeHidden, const QString &environment) const
{
    // Hidden should have been called Deleted. It means the user deleted
    // (at his level) something that was present
    if (excludeHidden && value(QLatin1String("Hidden")).toBool())
        return false;

    // A list of strings identifying the environments that should display/not
    // display a given desktop entry.
    // OnlyShowIn ........
    QString env;
    if (environment.isEmpty())
        env = QString::fromLocal8Bit(detectDesktopEnvironment());
    else {
        env = environment.toUpper();
    }

    QString key;
    bool keyFound = false;
    if (contains(onlyShowInKey))
    {
        key = onlyShowInKey;
        keyFound = true;
    }
    else
    {
        key = extendPrefixKey + onlyShowInKey;
        keyFound = contains(key) ? true : false;
    }

    if (keyFound)
    {
        QStringList s = value(key).toString().toUpper().split(QLatin1Char(';'));
        if (!s.contains(env))
            return false;
    }

    // NotShowIn .........
    if (contains(notShowInKey))
    {
        key = notShowInKey;
        keyFound = true;
    }
    else
    {
        key = extendPrefixKey + notShowInKey;
        keyFound = contains(key) ? true : false;
    }

    if (keyFound)
    {
        QStringList s = value(key).toString().toUpper().split(QLatin1Char(';'));
        if (s.contains(env))
            return false;
    }

    // actually installed. If not, entry may not show in menus, etc.
    QString s = value(QLatin1String("TryExec")).toString();
    if (!s.isEmpty() && ! checkTryExec(s))
        return false;

    return true;
}


QString expandDynamicUrl(QString url)
{
    const QStringList env = QProcess::systemEnvironment();
    for (const QString &line : env)
    {
        QString name = line.section(QLatin1Char('='), 0, 0);
        QString val =  line.section(QLatin1Char('='), 1);
        url.replace(QString::fromLatin1("$%1").arg(name), val);
        url.replace(QString::fromLatin1("${%1}").arg(name), val);
    }

    return url;
}


QString XdgDesktopFile::url() const
{
    if (type() != LinkType)
        return QString();

   QString url;

   url = value(urlKey).toString();
   if (!url.isEmpty())
   return url;

    // WTF? What standard describes it?
    url = expandDynamicUrl(value(QLatin1String("URL[$e]")).toString());
    if (!url.isEmpty())
        return url;

    return QString();
}


QString findDesktopFile(const QString& dirName, const QString& desktopName)
{
    QDir dir(dirName);
    QFileInfo fi(dir, desktopName);

    if (fi.exists())
        return fi.canonicalFilePath();

    // Working recursively ............
    const QFileInfoList dirs = dir.entryInfoList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &d : dirs)
    {
        QString cn = d.canonicalFilePath();
        if (dirName != cn)
        {
            QString f = findDesktopFile(cn, desktopName);
            if (!f.isEmpty())
                return f;
        }
    }

    return QString();
}


QString findDesktopFile(const QString& desktopName)
{
    QStringList dataDirs = XdgDirs::dataDirs();
    dataDirs.prepend(XdgDirs::dataHome(false));

    for (const QString &dirName : qAsConst(dataDirs))
    {
        QString f = findDesktopFile(dirName + QLatin1String("/applications"), desktopName);
        if (!f.isEmpty())
            return f;
    }

    return QString();
}


XdgDesktopFile* XdgDesktopFileCache::getFile(const QString& fileName)
{
    if (fileName.isEmpty())
        return nullptr;

    if (instance().m_fileCache.contains(fileName))
    {
        return instance().m_fileCache.value(fileName);
    }

    QString file;
    if (!fileName.startsWith(QDir::separator()))
    {
        // Relative path
        // Search desktop file ..................
        file = findDesktopFile(fileName);
        if (file.isEmpty())
            return nullptr;
    }
    else
    {
        file = fileName;
    }

    XdgDesktopFile* desktopFile;

    // The file was found
    if (!instance().m_fileCache.contains(file))
    {
        desktopFile = load(file);
        if (desktopFile)
        {
            instance().m_fileCache.insert(file, desktopFile);
            return desktopFile;
        }
        else
        {
            return nullptr;
        }
    }
    else
    {
        // already in the cache
        desktopFile = instance().m_fileCache.value(file);
        return desktopFile;
    }

}

QList<XdgDesktopFile*> XdgDesktopFileCache::getAllFiles()
{
    return instance().m_fileCache.values();
}



XdgDesktopFileCache & XdgDesktopFileCache::instance()
{
    static XdgDesktopFileCache cache;
    if (!cache.m_IsInitialized)
    {
       cache.initialize();
       cache.m_IsInitialized = true;
    }

    return cache;
}


/*!
 * Handles files with a syntax similar to desktopfiles as QSettings files.
 * The differences between ini-files and desktopfiles are:
 * desktopfiles uses '#' as comment marker, and ';' as list-separator.
 * Every key/value must be inside a section (i.e. there is no 'General' pseudo-section)
 */
bool readDesktopFile(QIODevice & device, QSettings::SettingsMap & map)
{
    QString section;
    QTextStream stream(&device);

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        // Skip comments and empty lines
        if (line.startsWith(QLatin1Char('#')) || line.isEmpty())
            continue;

        // Section ..............................
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']')))
        {
            section = line.mid(1, line.length()-2);
            continue;
        }

        QString key = line.section(QLatin1Char('='), 0, 0).trimmed();
        QString value = line.section(QLatin1Char('='), 1).trimmed();

        if (key.isEmpty())
            continue;

        if (section.isEmpty())
        {
            qWarning() << "key=value outside section";
            return false;
        }

        key.prepend(QLatin1Char('/'));
        key.prepend(section);

        if (value.contains(QLatin1Char(';')))
        {
            map.insert(key, value.split(QLatin1Char(';'), QString::SkipEmptyParts));
        }
        else
        {
            map.insert(key, value);
        }

    }

    return true;
}


/*! See readDesktopFile
 */
bool writeDesktopFile(QIODevice & device, const QSettings::SettingsMap & map)
{
    QTextStream stream(&device);
    QString section;

    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
    {
        bool isString     = it.value().canConvert<QString>();
        bool isStringList = (it.value().type() == QVariant::StringList);

        if ((! isString) && (! isStringList))
        {
            return false;
        }

        QString thisSection = it.key().section(QLatin1Char('/'), 0, 0);
        if (thisSection.isEmpty())
        {
            qWarning() << "No section defined";
            return false;
        }

        if (thisSection != section)
        {
            stream << QLatin1Char('[') << thisSection << QLatin1Char(']') << QLatin1Char('\n');
            section = thisSection;
        }

        QString remainingKey = it.key().section(QLatin1Char('/'), 1, -1);

        if (remainingKey.isEmpty())
        {
            qWarning() << "Only one level in key..." ;
            return false;
        }

        stream << remainingKey << QLatin1Char('=');

        if (isString)
        {
            stream << it.value().toString() << QLatin1Char(';');
        }
        else /* if (isStringList) */
        {
            const auto values = it.value().toStringList();
            for (const QString &value : values)
            {
                stream << value << QLatin1Char(';');
            }
        }

        stream << QLatin1Char('\n');

    }

    return true;
}


void XdgDesktopFileCache::initialize(const QString& dirName)
{
    QDir dir(dirName);
    // Directories have the type "application/x-directory", but in the desktop file
    // are shown as "inode/directory". To handle these cases, we use this hash.
    QHash<QString, QString> specials;
    specials.insert(QLatin1String("inode/directory"), QLatin1String("application/x-directory"));


    // Working recursively ............
    const QFileInfoList files = dir.entryInfoList(QStringList(), QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &f : files)
    {
        if (f.isDir())
        {
            initialize(f.absoluteFilePath());
            continue;
        }


        XdgDesktopFile* df = load(f.absoluteFilePath());
        if (!df)
            continue;

        if (! m_fileCache.contains(f.absoluteFilePath()))
        {
            m_fileCache.insert(f.absoluteFilePath(), df);
        }

        const QStringList mimes = df->value(mimeTypeKey).toString().split(QLatin1Char(';'), QString::SkipEmptyParts);

        for (const QString &mime : mimes)
        {
            int pref = df->value(initialPreferenceKey, 0).toInt();
            // We move the desktopFile forward in the list for this mime, so that
            // no desktopfile in front of it have a lower initialPreference.
            int position = m_defaultAppsCache[mime].length();
            while (position > 0 && m_defaultAppsCache[mime][position - 1]->value(initialPreferenceKey, 0).toInt() < pref)
            {
                position--;
            }
            m_defaultAppsCache[mime].insert(position, df);
        }
    }

}


XdgDesktopFile* XdgDesktopFileCache::load(const QString& fileName)
{
    XdgDesktopFile* desktopFile = new XdgDesktopFile();

    Q_CHECK_PTR(desktopFile);
    if (desktopFile && desktopFile->load(fileName))
        return desktopFile;

    delete desktopFile;
    return nullptr;
}


QSettings::Format XdgDesktopFileCache::desktopFileSettingsFormat()
{
    static QSettings::Format format = QSettings::InvalidFormat;

    if (format == QSettings::InvalidFormat)
        format = QSettings::registerFormat(QLatin1String("*.list"), readDesktopFile, writeDesktopFile);

    return format;
}


XdgDesktopFileCache::XdgDesktopFileCache() :
    m_IsInitialized(false),
        m_defaultAppsCache(),
        m_fileCache()
{
}


XdgDesktopFileCache::~XdgDesktopFileCache()
{
}


void XdgDesktopFileCache::initialize()
{
    QStringList dataDirs = XdgDirs::dataDirs();
    dataDirs.prepend(XdgDirs::dataHome(false));

    for (const QString &dirname : qAsConst(dataDirs))
    {
        initialize(dirname + QLatin1String("/applications"));
    }
}

QList<XdgDesktopFile*> XdgDesktopFileCache::getAppsOfCategory(const QString& category)
{
    QList<XdgDesktopFile*> list;
    const QString _category = category.toUpper();
    const QHash<QString, XdgDesktopFile*> fileCache = instance().m_fileCache;
    for (XdgDesktopFile *desktopFile : fileCache)
    {
        QStringList categories = desktopFile->value(categoriesKey).toString().toUpper().split(QLatin1Char(';'));
        if (!categories.isEmpty() && (categories.contains(_category) || categories.contains(QLatin1String("X-") + _category)))
            list.append(desktopFile);
    }
    return list;
}

QList<XdgDesktopFile*>  XdgDesktopFileCache::getApps(const QString& mimetype)
{
    return instance().m_defaultAppsCache.value(mimetype);
}


XdgDesktopFile* XdgDesktopFileCache::getDefaultApp(const QString& mimetype)
{
    // First, we look in following places for a default in specified order:
    // ~/.config/mimeapps.list
    // /etc/xdg/mimeapps.list
    // ~/.local/share/applications/mimeapps.list
    // /usr/local/share/applications/mimeapps.list
    // /usr/share/applications/mimeapps.list
    QStringList mimeDirsList;

    mimeDirsList.append(XdgDirs::configHome(false));
    mimeDirsList.append(XdgDirs::configDirs());
    mimeDirsList.append(XdgDirs::dataHome(false) + QLatin1String("/applications"));
    mimeDirsList.append(XdgDirs::dataDirs(QLatin1String("/applications")));

    for (const QString &mimeDir : qAsConst(mimeDirsList))
    {
        QString defaultsListPath = mimeDir + QLatin1String("/mimeapps.list");
        if (QFileInfo::exists(defaultsListPath))
        {
            QSettings defaults(defaultsListPath, desktopFileSettingsFormat());


            defaults.beginGroup(QLatin1String("Default Applications"));
            if (defaults.contains(mimetype))
            {
                QVariant value = defaults.value(mimetype);
                if (value.canConvert<QStringList>()) // A single string can also convert to a stringlist
                {
                    const QStringList values = value.toStringList();
                    for (const QString &desktopFileName : values)
                    {
                        XdgDesktopFile* desktopFile = XdgDesktopFileCache::getFile(desktopFileName);
                        if (desktopFile)
                        {
                            return desktopFile;
                        }
                        else
                        {
                            qWarning() << desktopFileName << "not a valid desktopfile";
                        }
                    }
                }
            }
            defaults.endGroup();
        }
    }

    // If we havent found anything up to here, we look for a desktopfile that declares
    // the ability to handle the given mimetype. See getApps.
    QList<XdgDesktopFile*> apps = getApps(mimetype);
    XdgDesktopFile* desktopFile = apps.isEmpty() ? 0 : apps[0];
    return desktopFile;
}
