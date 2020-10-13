#include "volumemanager.h"

namespace Fm {

std::mutex VolumeManager::mutex_;
std::weak_ptr<VolumeManager> VolumeManager::globalInstance_;

VolumeManager::VolumeManager():
    QObject(),
    monitor_{g_volume_monitor_get(), false} {

    // connect gobject signal handlers
    g_signal_connect(monitor_.get(), "volume-added", G_CALLBACK(_onGVolumeAdded), this);
    g_signal_connect(monitor_.get(), "volume-removed", G_CALLBACK(_onGVolumeRemoved), this);
    g_signal_connect(monitor_.get(), "volume-changed", G_CALLBACK(_onGVolumeChanged), this);

    g_signal_connect(monitor_.get(), "mount-added", G_CALLBACK(_onGMountAdded), this);
    g_signal_connect(monitor_.get(), "mount-removed", G_CALLBACK(_onGMountRemoved), this);
    g_signal_connect(monitor_.get(), "mount-changed", G_CALLBACK(_onGMountChanged), this);

    // g_get_volume_monitor() is a slow blocking call, so call it in a low priority thread
    auto job = new GetGVolumeMonitorJob();
    job->setAutoDelete(true);
    connect(job, &GetGVolumeMonitorJob::finished, this, &VolumeManager::onGetGVolumeMonitorFinished, Qt::BlockingQueuedConnection);
    job->runAsync(QThread::LowPriority);
}

VolumeManager::~VolumeManager() {
    if(monitor_) {
        g_signal_handlers_disconnect_by_data(monitor_.get(), this);
    }
}

std::shared_ptr<VolumeManager> VolumeManager::globalInstance() {
    std::lock_guard<std::mutex> lock{mutex_};
    auto mon = globalInstance_.lock();
    if(mon == nullptr) {
        mon = std::make_shared<VolumeManager>();
        globalInstance_ = mon;
    }
    return mon;
}

void VolumeManager::onGetGVolumeMonitorFinished() {
    auto job = static_cast<GetGVolumeMonitorJob*>(sender());
    monitor_ = std::move(job->monitor_);
    GList* vols = g_volume_monitor_get_volumes(monitor_.get());
    for(GList* l = vols; l != nullptr; l = l->next) {
        volumes_.push_back(Volume{G_VOLUME(l->data), false});
        Q_EMIT volumeAdded(volumes_.back());
    }
    g_list_free(vols);

    GList* mnts = g_volume_monitor_get_mounts(monitor_.get());
    for(GList* l = mnts; l != nullptr; l = l->next) {
        mounts_.push_back(Mount{G_MOUNT(l->data), false});
        Q_EMIT mountAdded(mounts_.back());
    }
    g_list_free(mnts);
}

void VolumeManager::onGVolumeAdded(GVolume* vol) {
    if(std::find(volumes_.cbegin(), volumes_.cend(), vol) != volumes_.cend())
        return;
    volumes_.push_back(Volume{vol, true});
    Q_EMIT volumeAdded(volumes_.back());
}

void VolumeManager::onGVolumeRemoved(GVolume* vol) {
    auto it = std::find(volumes_.begin(), volumes_.end(), vol);
    if(it == volumes_.end())
        return;
    Q_EMIT volumeRemoved(*it);
    volumes_.erase(it);
}

void VolumeManager::onGVolumeChanged(GVolume* vol) {
    auto it = std::find(volumes_.begin(), volumes_.end(), vol);
    if(it == volumes_.end())
        return;
    Q_EMIT volumeChanged(*it);
}

void VolumeManager::onGMountAdded(GMount* mnt) {
    if(std::find(mounts_.cbegin(), mounts_.cend(), mnt) != mounts_.cend())
        return;
    mounts_.push_back(Mount{mnt, true});
    Q_EMIT mountAdded(mounts_.back());
}

void VolumeManager::onGMountRemoved(GMount* mnt) {
    auto it = std::find(mounts_.begin(), mounts_.end(), mnt);
    if(it == mounts_.end())
        return;
    Q_EMIT mountRemoved(*it);
    mounts_.erase(it);
}

void VolumeManager::onGMountChanged(GMount* mnt) {
    auto it = std::find(mounts_.begin(), mounts_.end(), mnt);
    if(it == mounts_.end())
        return;
    Q_EMIT mountChanged(*it);
}

void VolumeManager::GetGVolumeMonitorJob::exec() {
    monitor_ = GVolumeMonitorPtr{g_volume_monitor_get(), false};
}


} // namespace Fm
