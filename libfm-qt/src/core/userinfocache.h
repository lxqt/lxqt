#ifndef FM2_USERINFOCACHE_H
#define FM2_USERINFOCACHE_H

#include "../libfmqtglobals.h"
#include <QObject>
#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <memory>
#include <mutex>

namespace Fm {

class LIBFM_QT_API UserInfo {
public:
    explicit UserInfo(uid_t uid, const char* name, const char* realName):
        uid_{uid}, name_{QString::fromUtf8(name)}, realName_{QString::fromUtf8(realName)} {
    }

    uid_t uid() const {
        return uid_;
    }

    const QString& name() const {
        return name_;
    }

    const QString& realName() const {
        return realName_;
    }

private:
    uid_t uid_;
    QString name_;
    QString realName_;

};

class LIBFM_QT_API GroupInfo {
public:
    explicit GroupInfo(gid_t gid, const char* name): gid_{gid}, name_{QString::fromUtf8(name)} {
    }

    gid_t gid() const {
        return gid_;
    }

    const QString& name() const {
        return name_;
    }

private:
    gid_t gid_;
    QString name_;
};

// FIXME: handle file changes

class LIBFM_QT_API UserInfoCache : public QObject {
    Q_OBJECT
public:
    explicit UserInfoCache();

    const std::shared_ptr<const UserInfo>& userFromId(uid_t uid);

    const std::shared_ptr<const GroupInfo>& groupFromId(gid_t gid);

    static UserInfoCache* globalInstance();

Q_SIGNALS:
    void changed();

private:
    std::unordered_map<uid_t, std::shared_ptr<const UserInfo>> users_;
    std::unordered_map<gid_t, std::shared_ptr<const GroupInfo>> groups_;
    static UserInfoCache* globalInstance_;
    static std::mutex mutex_;
};

} // namespace Fm

#endif // FM2_USERINFOCACHE_H
