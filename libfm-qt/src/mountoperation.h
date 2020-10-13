/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#ifndef FM_MOUNTOPERATION_H
#define FM_MOUNTOPERATION_H

#include "libfmqtglobals.h"
#include <QWidget>
#include <QDialog>
#include <gio/gio.h>
#include <QPointer>

#include "core/filepath.h"

class QEventLoop;

namespace Fm {

// FIXME: the original APIs in gtk+ version of libfm for mounting devices is poor.
// Need to find a better API design which make things fully async and cancellable.

// FIXME: parent_ does not work. All dialogs shown by the mount operation has no parent window assigned.
// FIXME: Need to reconsider the propery way of API design. Blocking sync calls are handy, but
// indeed causes some problems. :-(

class LIBFM_QT_API MountOperation: public QObject {
    Q_OBJECT

public:
    explicit MountOperation(bool interactive = true, QWidget* parent = nullptr);
    ~MountOperation() override;

    FM_QT_DEPRECATED
    void mount(const Fm::FilePath& path) {
        mountEnclosingVolume(path);
    }

    void mountEnclosingVolume(const Fm::FilePath& path);

    void mountMountable(const Fm::FilePath& mountable);

    void mount(GVolume* volume) {
        g_volume_mount(volume, G_MOUNT_MOUNT_NONE, op, cancellable_, (GAsyncReadyCallback)onMountVolumeFinished, new QPointer<MountOperation>(this));
    }

    void unmount(GMount* mount) {
        prepareUnmount(mount);
        g_mount_unmount_with_operation(mount, G_MOUNT_UNMOUNT_NONE, op, cancellable_, (GAsyncReadyCallback)onUnmountMountFinished, new QPointer<MountOperation>(this));
    }

    void unmount(GVolume* volume) {
        GMount* mount = g_volume_get_mount(volume);
        if(!mount) {
            return;
        }
        unmount(mount);
        g_object_unref(mount);
    }

    void eject(GMount* mount) {
        prepareUnmount(mount);
        g_mount_eject_with_operation(mount, G_MOUNT_UNMOUNT_NONE, op, cancellable_, (GAsyncReadyCallback)onEjectMountFinished, new QPointer<MountOperation>(this));
    }

    void eject(GVolume* volume) {
        GMount* mnt = g_volume_get_mount(volume);
        prepareUnmount(mnt);
        g_object_unref(mnt);
        g_volume_eject_with_operation(volume, G_MOUNT_UNMOUNT_NONE, op, cancellable_, (GAsyncReadyCallback)onEjectVolumeFinished, new QPointer<MountOperation>(this));
    }

    QWidget* parent() const {
        return parent_;
    }

    void setParent(QWidget* parent) {
        parent_ = parent;
    }

    GCancellable* cancellable() const {
        return cancellable_;
    }

    GMountOperation* mountOperation() {
        return op;
    }

    void cancel() {
        g_cancellable_cancel(cancellable_);
    }

    bool isRunning() const {
        return running;
    }

    // block the operation used an internal QEventLoop and returns
    // only after the whole operation is finished.
    bool wait();

    bool autoDestroy() {
        return autoDestroy_;
    }

    void setAutoDestroy(bool destroy = true) {
        autoDestroy_ = destroy;
    }

Q_SIGNALS:
    void finished(GError* error = nullptr);

private:
    void prepareUnmount(GMount* mount);

    static void onAskPassword(GMountOperation* _op, gchar* message, gchar* default_user, gchar* default_domain, GAskPasswordFlags flags, MountOperation* pThis);
    static void onAskQuestion(GMountOperation* _op, gchar* message, GStrv choices, MountOperation* pThis);
    // static void onReply(GMountOperation *_op, GMountOperationResult result, MountOperation* pThis);

    static void onAbort(GMountOperation* _op, MountOperation* pThis);
    static void onShowProcesses(GMountOperation* _op, gchar* message, GArray* processes, GStrv choices, MountOperation* pThis);
    static void onShowUnmountProgress(GMountOperation* _op, gchar* message, gint64 time_left, gint64 bytes_left, MountOperation* pThis);

    // it's possible that this object is freed when the callback is called by gio, so guarding with QPointer is needed here.
    static void onMountFileFinished(GFile* file, GAsyncResult* res, QPointer<MountOperation>* pThis);
    static void onMountMountableFinished(GFile* file, GAsyncResult* res, QPointer<MountOperation>* pThis);
    static void onMountVolumeFinished(GVolume* volume, GAsyncResult* res, QPointer<MountOperation>* pThis);
    static void onUnmountMountFinished(GMount* mount, GAsyncResult* res, QPointer<MountOperation>* pThis);
    static void onEjectMountFinished(GMount* mount, GAsyncResult* res, QPointer<MountOperation>* pThis);
    static void onEjectVolumeFinished(GVolume* volume, GAsyncResult* res, QPointer<MountOperation>* pThis);

    void handleFinish(GError* error);

private:
    GMountOperation* op;
    GCancellable* cancellable_;
    QWidget* parent_;
    bool running;
    bool interactive_;
    QEventLoop* eventLoop;
    bool autoDestroy_;
};

}

#endif // FM_MOUNTOPERATION_H
