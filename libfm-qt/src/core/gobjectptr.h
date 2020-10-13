#ifndef FM2_GOBJECTPTR_H
#define FM2_GOBJECTPTR_H

#include "../libfmqtglobals.h"
#include <glib.h>
#include <glib-object.h>
#include <cstddef>
#include <QDebug>

namespace Fm {

template <typename T>
class LIBFM_QT_API GObjectPtr {
public:

    explicit GObjectPtr(): gobj_{nullptr} {
    }

    explicit GObjectPtr(T* gobj, bool add_ref = true): gobj_{gobj} {
        if(gobj_ != nullptr && add_ref)
            g_object_ref(gobj_);
    }

    GObjectPtr(const GObjectPtr& other): gobj_{other.gobj_ ? reinterpret_cast<T*>(g_object_ref(other.gobj_)) : nullptr} {
    }

    GObjectPtr(GObjectPtr&& other) noexcept: gobj_{other.release()} {
    }

    ~GObjectPtr() {
        if(gobj_ != nullptr)
            g_object_unref(gobj_);
    }

    T* get() const {
        return gobj_;
    }

    T* release() {
        T* tmp = gobj_;
        gobj_ = nullptr;
        return tmp;
    }

    void reset() {
        if(gobj_ != nullptr)
            g_object_unref(gobj_);
        gobj_ = nullptr;
    }

    GObjectPtr& operator = (const GObjectPtr& other) {
        if (*this == other)
            return *this;

        if(gobj_ != nullptr)
            g_object_unref(gobj_);
        gobj_ = other.gobj_ ? reinterpret_cast<T*>(g_object_ref(other.gobj_)) : nullptr;
        return *this;
    }

    GObjectPtr& operator = (GObjectPtr&& other) noexcept {
        if (this == &other)
            return *this;

        if(gobj_ != nullptr)
            g_object_unref(gobj_);
        gobj_ = other.release();
        return *this;
    }

    GObjectPtr& operator = (T* gobj) {
        if (*this == gobj)
            return *this;

        if(gobj_ != nullptr)
            g_object_unref(gobj_);
        gobj_ = gobj ? reinterpret_cast<T*>(g_object_ref(gobj_)) : nullptr;
        return *this;
    }

    bool operator == (const GObjectPtr& other) const {
        return gobj_ == other.gobj_;
    }

    bool operator == (T* gobj) const {
        return gobj_ == gobj;
    }

    bool operator != (std::nullptr_t) const {
        return gobj_ != nullptr;
    }

    operator bool() const {
        return gobj_ != nullptr;
    }

private:
    mutable T* gobj_;
};


} // namespace Fm

#endif // FM2_GOBJECTPTR_H
