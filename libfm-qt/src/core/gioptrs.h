#ifndef GIOPTRS_H
#define GIOPTRS_H
// define smart pointers for GIO data types

#include <glib.h>
#include <gio/gio.h>
#include "gobjectptr.h"
#include "cstrptr.h"

namespace Fm {

typedef GObjectPtr<GFile> GFilePtr;
typedef GObjectPtr<GFileInfo> GFileInfoPtr;
typedef GObjectPtr<GFileMonitor> GFileMonitorPtr;
typedef GObjectPtr<GCancellable> GCancellablePtr;
typedef GObjectPtr<GFileEnumerator> GFileEnumeratorPtr;

typedef GObjectPtr<GInputStream> GInputStreamPtr;
typedef GObjectPtr<GFileInputStream> GFileInputStreamPtr;
typedef GObjectPtr<GOutputStream> GOutputStreamPtr;
typedef GObjectPtr<GFileOutputStream> GFileOutputStreamPtr;

typedef GObjectPtr<GIcon> GIconPtr;

typedef GObjectPtr<GVolumeMonitor> GVolumeMonitorPtr;
typedef GObjectPtr<GVolume> GVolumePtr;
typedef GObjectPtr<GMount> GMountPtr;

typedef GObjectPtr<GAppInfo> GAppInfoPtr;


class GErrorPtr {
public:
    GErrorPtr(): err_{nullptr} {
    }

    GErrorPtr(GError*&& err) noexcept: err_{err} {
        err = nullptr;
    }

    GErrorPtr(const GErrorPtr& other) = delete;

    GErrorPtr(GErrorPtr&& other) noexcept: err_{other.err_} {
        other.err_ = nullptr;
    }

    GErrorPtr(std::uint32_t domain, unsigned int code, const char* msg):
        GErrorPtr{g_error_new_literal(domain, code, msg)} {
    }

    GErrorPtr(std::uint32_t domain, unsigned int code, const QString& msg):
        GErrorPtr{domain, code, msg.toUtf8().constData()} {
    }

    ~GErrorPtr() {
        reset();
    }

    std::uint32_t domain() const {
        if(err_ != nullptr) {
            return err_->domain;
        }
        return 0;
    }

    unsigned int code() const {
        if(err_ != nullptr) {
            return err_->code;
        }
        return 0;
    }

    QString message() const {
        if(err_ != nullptr) {
            return QString::fromUtf8(err_->message);
        }
        return QString();
    }

    void reset() {
        if(err_) {
            g_error_free(err_);
        }
        err_ = nullptr;
    }

    GError* get() const {
        return err_;
    }

    GErrorPtr& operator = (const GErrorPtr& other) = delete;

    GErrorPtr& operator = (GErrorPtr&& other) noexcept {
        reset();
        err_ = other.err_;
        other.err_ = nullptr;
        return *this;
    }

    GErrorPtr& operator = (GError*&& err) {
        reset();
        err_ = err;
        return *this;
    }

    GError** operator&() {
        return &err_;
    }

    GError* operator->() {
        return err_;
    }

    const GError* operator->() const {
        return err_;
    }

    bool operator == (const GErrorPtr& other) const {
        return err_ == other.err_;
    }

    bool operator == (GError* err) const {
        return err_ == err;
    }

    bool operator != (std::nullptr_t) const {
        return err_ != nullptr;
    }

    operator bool() const {
        return err_ != nullptr;
    }

private:
    GError* err_;
};

} //namespace Fm

#endif // GIOPTRS_H
