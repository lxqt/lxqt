/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef PCMANFM_SETTINGS_H
#define PCMANFM_SETTINGS_H

#include <QObject>
#include <libfm-qt/folderview.h>
#include <libfm-qt/foldermodel.h>
#include "desktopwindow.h"
#include <libfm-qt/sidepane.h>
#include <libfm-qt/core/thumbnailjob.h>
#include <libfm-qt/core/archiver.h>
#include <libfm-qt/core/legacy/fm-config.h>

namespace PCManFM {

enum OpenDirTargetType {
    OpenInCurrentTab,
    OpenInNewTab,
    OpenInNewWindow,
    OpenInLastActiveWindow
};

class FolderSettings {
public:
    FolderSettings():
        isCustomized_(false),
        // NOTE: The default values of the following variables should be
        // the same as those of their corresponding variables in Settings:
        sortOrder_(Qt::AscendingOrder),
        sortColumn_(Fm::FolderModel::ColumnFileName),
        viewMode_(Fm::FolderView::IconMode),
        showHidden_(false),
        sortFolderFirst_(true),
        sortHiddenLast_(false),
        sortCaseSensitive_(true) {
    }

    bool isCustomized() const {
        return isCustomized_;
    }

    void setCustomized(bool value) {
        isCustomized_ = value;
    }

    Qt::SortOrder sortOrder() const {
        return sortOrder_;
    }

    void setSortOrder(Qt::SortOrder value) {
        sortOrder_ = value;
    }

    Fm::FolderModel::ColumnId sortColumn() const {
        return sortColumn_;
    }

    void setSortColumn(Fm::FolderModel::ColumnId value) {
        sortColumn_ = value;
    }

    Fm::FolderView::ViewMode viewMode() const {
        return viewMode_;
    }

    void setViewMode(Fm::FolderView::ViewMode value) {
        viewMode_ = value;
    }

    bool sortFolderFirst() const {
        return sortFolderFirst_;
    }

    void setSortFolderFirst(bool value) {
        sortFolderFirst_ = value;
    }

    bool sortHiddenLast() const {
        return sortHiddenLast_;
    }

    void setSortHiddenLast(bool value) {
        sortHiddenLast_ = value;
    }

    bool showHidden() const {
        return showHidden_;
    }

    void setShowHidden(bool value) {
        showHidden_ = value;
    }

    bool sortCaseSensitive() const {
        return sortCaseSensitive_;
    }

    void setSortCaseSensitive(bool value) {
        sortCaseSensitive_ = value;
    }

private:
    bool isCustomized_;
    Qt::SortOrder sortOrder_;
    Fm::FolderModel::ColumnId sortColumn_;
    Fm::FolderView::ViewMode viewMode_;
    bool showHidden_;
    bool sortFolderFirst_;
    bool sortHiddenLast_;
    bool sortCaseSensitive_;
    // columns?
};


class Settings : public QObject {
    Q_OBJECT
public:
    enum IconType {
        Small,
        Big,
        Thumbnail
    };

    Settings();
    virtual ~Settings();

    bool load(QString profile = QStringLiteral("default"));
    bool save(QString profile = QString());

    bool loadFile(QString filePath);
    bool saveFile(QString filePath);

    static QString xdgUserConfigDir();
    static const QList<int> & iconSizes(IconType type);

    QString profileDir(QString profile, bool useFallback = false);

    // setter/getter functions
    QString profileName() const {
        return profileName_;
    }

    bool supportTrash() const {
        return supportTrash_;
    }

    QString fallbackIconThemeName() const {
        return fallbackIconThemeName_;
    }

    bool useFallbackIconTheme() const {
        return useFallbackIconTheme_;
    }

    void setFallbackIconThemeName(QString iconThemeName) {
        fallbackIconThemeName_ = iconThemeName;
    }

    bool singleWindowMode() const {
        return singleWindowMode_;
    }

    void setSingleWindowMode(bool singleWindowMode) {
        singleWindowMode_ = singleWindowMode;
    }

    OpenDirTargetType bookmarkOpenMethod() {
        return bookmarkOpenMethod_;
    }

    void setBookmarkOpenMethod(OpenDirTargetType bookmarkOpenMethod) {
        bookmarkOpenMethod_ = bookmarkOpenMethod;
    }

    QString suCommand() const {
        return suCommand_;
    }

    void setSuCommand(QString suCommand) {
        suCommand_ = suCommand;
    }

    QString terminal() {
        return terminal_;
    }
    void setTerminal(QString terminalCommand);

    QString archiver() const {
        return archiver_;
    }

    void setArchiver(QString archiver) {
        archiver_ = archiver;
        Fm::Archiver::setDefaultArchiverByName(archiver_.toLocal8Bit().constData());
    }

    bool mountOnStartup() const {
        return mountOnStartup_;
    }

    void setMountOnStartup(bool mountOnStartup) {
        mountOnStartup_ = mountOnStartup;
    }

    bool mountRemovable() {
        return mountRemovable_;
    }

    void setMountRemovable(bool mountRemovable) {
        mountRemovable_ = mountRemovable;
    }

    bool autoRun() const {
        return autoRun_;
    }

    void setAutoRun(bool autoRun) {
        autoRun_ = autoRun;
    }

    bool closeOnUnmount() const {
        return closeOnUnmount_;
    }

    void setCloseOnUnmount(bool value) {
        closeOnUnmount_ = value;
    }

    DesktopWindow::WallpaperMode wallpaperMode() const {
        return DesktopWindow::WallpaperMode(wallpaperMode_);
    }

    void setWallpaperMode(int wallpaperMode) {
        wallpaperMode_ = wallpaperMode;
    }

    QString wallpaper() const {
        return wallpaper_;
    }

    void setWallpaper(QString wallpaper) {
        wallpaper_ = wallpaper;
    }

    QSize wallpaperDialogSize() const {
        return wallpaperDialogSize_;
    }

    void setWallpaperDialogSize(const QSize& size) {
        wallpaperDialogSize_ = size;
    }

    int wallpaperDialogSplitterPos() const {
        return wallpaperDialogSplitterPos_;
    }

    void setWallpaperDialogSplitterPos(int pos) {
        wallpaperDialogSplitterPos_ = pos;
    }

    QString wallpaperDir() const {
        return wallpaperDir_;
    }

    void setLastSlide(QString wallpaper) {
        lastSlide_ = wallpaper;
    }

    QString lastSlide() const {
        return lastSlide_;
    }

    void setWallpaperDir(QString dir) {
        wallpaperDir_ = dir;
    }

    int slideShowInterval() const {
        return slideShowInterval_;
    }

    void setSlideShowInterval(int interval) {
        slideShowInterval_ = interval;
    }

    bool wallpaperRandomize() const {
       return wallpaperRandomize_;
    }

    void setWallpaperRandomize(bool randomize) {
        wallpaperRandomize_ = randomize;
    }

    bool transformWallpaper() const {
       return transformWallpaper_;
    }

    void setTransformWallpaper(bool tr) {
        transformWallpaper_ = tr;
    }

    bool perScreenWallpaper() const {
       return perScreenWallpaper_;
    }

    void setPerScreenWallpaper(bool tr) {
        perScreenWallpaper_ = tr;
    }

    const QColor& desktopBgColor() const {
        return desktopBgColor_;
    }

    void setDesktopBgColor(QColor desktopBgColor) {
        desktopBgColor_ = desktopBgColor;
    }

    const QColor& desktopFgColor() const {
        return desktopFgColor_;
    }

    void setDesktopFgColor(QColor desktopFgColor) {
        desktopFgColor_ = desktopFgColor;
    }

    const QColor& desktopShadowColor() const {
        return desktopShadowColor_;
    }

    void setDesktopShadowColor(QColor desktopShadowColor) {
        desktopShadowColor_ = desktopShadowColor;
    }

    QFont desktopFont() const {
        return desktopFont_;
    }

    void setDesktopFont(QFont font) {
        desktopFont_ = font;
    }

    int desktopIconSize() const {
        return desktopIconSize_;
    }

    void setDesktopIconSize(int desktopIconSize) {
        desktopIconSize_ = desktopIconSize;
    }

    QStringList desktopShortcuts() const {
        return desktopShortcuts_;
    }

    void setDesktopShortcuts(const QStringList& list) {
        desktopShortcuts_ = list;
    }

    bool desktopShowHidden() const {
        return desktopShowHidden_;
    }

    void setDesktopShowHidden(bool desktopShowHidden) {
        desktopShowHidden_ = desktopShowHidden;
    }

    bool desktopHideItems() const {
        return desktopHideItems_;
    }

    void setDesktopHideItems(bool hide) {
        desktopHideItems_ = hide;
    }

    Qt::SortOrder desktopSortOrder() const {
        return desktopSortOrder_;
    }

    void setDesktopSortOrder(Qt::SortOrder desktopSortOrder) {
        desktopSortOrder_ = desktopSortOrder;
    }

    Fm::FolderModel::ColumnId desktopSortColumn() const {
        return desktopSortColumn_;
    }

    void setDesktopSortColumn(Fm::FolderModel::ColumnId desktopSortColumn) {
        desktopSortColumn_ = desktopSortColumn;
    }

    bool desktopSortFolderFirst() const {
        return desktopSortFolderFirst_;
    }

    void setDesktopSortFolderFirst(bool desktopFolderFirst) {
        desktopSortFolderFirst_ = desktopFolderFirst;
    }

    bool desktopSortHiddenLast() const {
        return desktopSortHiddenLast_;
    }

    void setDesktopSortHiddenLast(bool desktopHiddenLast) {
        desktopSortHiddenLast_ = desktopHiddenLast;
    }

    bool alwaysShowTabs() const {
        return alwaysShowTabs_;
    }

    void setAlwaysShowTabs(bool alwaysShowTabs) {
        alwaysShowTabs_ = alwaysShowTabs;
    }

    bool showTabClose() const {
        return showTabClose_;
    }

    void setShowTabClose(bool showTabClose) {
        showTabClose_ = showTabClose;
    }

    bool switchToNewTab() const {
        return switchToNewTab_;
    }

    void setSwitchToNewTab(bool showTabClose) {
        switchToNewTab_ = showTabClose;
    }

    bool reopenLastTabs() const {
        return reopenLastTabs_;
    }

    void setReopenLastTabs(bool reopenLastTabs) {
        reopenLastTabs_ = reopenLastTabs;
    }

    QStringList tabPaths() const {
        return tabPaths_;
    }

    void setTabPaths(const QStringList& tabPaths) {
        tabPaths_ = tabPaths;
    }

    bool rememberWindowSize() const {
        return rememberWindowSize_;
    }

    void setRememberWindowSize(bool rememberWindowSize) {
        rememberWindowSize_ = rememberWindowSize;
    }

    int windowWidth() const {
        if(rememberWindowSize_) {
            return lastWindowWidth_;
        }
        else {
            return fixedWindowWidth_;
        }
    }

    int windowHeight() const {
        if(rememberWindowSize_) {
            return lastWindowHeight_;
        }
        else {
            return fixedWindowHeight_;
        }
    }

    bool windowMaximized() const {
        if(rememberWindowSize_) {
            return lastWindowMaximized_;
        }
        else {
            return false;
        }
    }

    int fixedWindowWidth() const {
        return fixedWindowWidth_;
    }

    void setFixedWindowWidth(int fixedWindowWidth) {
        fixedWindowWidth_ = fixedWindowWidth;
    }

    int fixedWindowHeight() const {
        return fixedWindowHeight_;
    }

    void setFixedWindowHeight(int fixedWindowHeight) {
        fixedWindowHeight_ = fixedWindowHeight;
    }

    void setLastWindowWidth(int lastWindowWidth) {
        lastWindowWidth_ = lastWindowWidth;
    }

    void setLastWindowHeight(int lastWindowHeight) {
        lastWindowHeight_ = lastWindowHeight;
    }

    void setLastWindowMaximized(bool lastWindowMaximized) {
        lastWindowMaximized_ = lastWindowMaximized;
    }

    int splitterPos() const {
        return splitterPos_;
    }

    void setSplitterPos(int splitterPos) {
        splitterPos_ = splitterPos;
    }

    bool isSidePaneVisible() const {
        return sidePaneVisible_;
    }

    void showSidePane(bool show) {
        sidePaneVisible_ = show;
    }

    Fm::SidePane::Mode sidePaneMode() const {
        return sidePaneMode_;
    }

    void setSidePaneMode(Fm::SidePane::Mode sidePaneMode) {
        sidePaneMode_ = sidePaneMode;
    }

    bool showMenuBar() const {
        return showMenuBar_;
    }

    void setShowMenuBar(bool showMenuBar) {
        showMenuBar_ = showMenuBar;
    }

    bool splitView() const {
        return splitView_;
    }

    void setSplitView(bool split) {
        splitView_ = split;
    }

    Fm::FolderView::ViewMode viewMode() const {
        return viewMode_;
    }

    void setViewMode(Fm::FolderView::ViewMode viewMode) {
        viewMode_ = viewMode;
    }

    bool showHidden() const {
        return showHidden_;
    }

    void setShowHidden(bool showHidden) {
        showHidden_ = showHidden;
    }

    bool sortCaseSensitive() const {
        return sortCaseSensitive_;
    }

    void setSortCaseSensitive(bool value) {
        sortCaseSensitive_ = value;
    }

    QSet<QString> getHiddenPlaces() const {
        return hiddenPlaces_;
    }

    void setHiddenPlace(const QString& str, bool hide) {
        if(hide) {
            hiddenPlaces_ << str;
        }
        else {
            hiddenPlaces_.remove(str);
        }
    }

    Qt::SortOrder sortOrder() const {
        return sortOrder_;
    }

    void setSortOrder(Qt::SortOrder sortOrder) {
        sortOrder_ = sortOrder;
    }

    Fm::FolderModel::ColumnId sortColumn() const {
        return sortColumn_;
    }

    void setSortColumn(Fm::FolderModel::ColumnId sortColumn) {
        sortColumn_ = sortColumn;
    }

    bool sortFolderFirst() const {
        return sortFolderFirst_;
    }

    void setSortFolderFirst(bool folderFirst) {
        sortFolderFirst_ = folderFirst;
    }

    bool sortHiddenLast() const {
        return sortHiddenLast_;
    }

    void setSortHiddenLast(bool hiddenLast) {
        sortHiddenLast_ = hiddenLast;
    }

    bool showFilter() const {
        return showFilter_;
    }

    void setShowFilter(bool value) {
        showFilter_ = value;
    }

    bool pathBarButtons() const {
        return pathBarButtons_;
    }

    void setPathBarButtons(bool value) {
        pathBarButtons_ = value;
    }

    // settings for use with libfm
    bool singleClick() const {
        return singleClick_;
    }

    void setSingleClick(bool singleClick) {
        singleClick_ = singleClick;
    }

    int autoSelectionDelay() const {
        return autoSelectionDelay_;
    }

    void setAutoSelectionDelay(int value) {
        autoSelectionDelay_ = value;
    }

    bool ctrlRightClick() const {
        return ctrlRightClick_;
    }

    void setCtrlRightClick(bool value) {
        ctrlRightClick_ = value;
    }

    bool useTrash() const {
        if(!supportTrash_) {
            return false;
        }
        return useTrash_;
    }

    void setUseTrash(bool useTrash) {
        useTrash_ = useTrash;
    }

    bool confirmDelete() const {
        return confirmDelete_;
    }

    void setConfirmDelete(bool confirmDelete) {
        confirmDelete_ = confirmDelete;
    }

    bool noUsbTrash() const {
        return noUsbTrash_;
    }

    void setNoUsbTrash(bool noUsbTrash) {
        noUsbTrash_ = noUsbTrash;
        fm_config->no_usb_trash = noUsbTrash_; // also set this to libfm since FmFileOpsJob reads this config value before trashing files.
    }

    bool confirmTrash() const {
        return confirmTrash_;
    }

    void setConfirmTrash(bool value) {
        confirmTrash_ = value;
    }

    bool quickExec() const {
        return quickExec_;
    }

    void setQuickExec(bool value) {
        quickExec_ = value;
        fm_config->quick_exec = quickExec_;
    }

    bool selectNewFiles() const {
        return selectNewFiles_;
    }

    void setSelectNewFiles(bool value) {
        selectNewFiles_ = value;
    }

    // bool thumbnailLocal_;
    // bool thumbnailMax;

    int bigIconSize() const {
        return bigIconSize_;
    }

    void setBigIconSize(int bigIconSize) {
        bigIconSize_ = bigIconSize;
    }

    int smallIconSize() const {
        return smallIconSize_;
    }

    void setSmallIconSize(int smallIconSize) {
        smallIconSize_ = smallIconSize;
    }

    int sidePaneIconSize() const {
        return sidePaneIconSize_;
    }

    void setSidePaneIconSize(int sidePaneIconSize) {
        sidePaneIconSize_ = sidePaneIconSize;
    }

    int thumbnailIconSize() const {
        return thumbnailIconSize_;
    }

    QSize folderViewCellMargins() const {
        return folderViewCellMargins_;
    }

    void setFolderViewCellMargins(QSize size) {
        folderViewCellMargins_ = size;
    }

    QSize desktopCellMargins() const {
        return desktopCellMargins_;
    }

    void setDesktopCellMargins(QSize size) {
        desktopCellMargins_ = size;
    }


    bool showThumbnails() {
        return showThumbnails_;
    }

    void setShowThumbnails(bool show) {
        showThumbnails_ = show;
    }

    void setThumbnailLocalFilesOnly(bool value) {
        Fm::ThumbnailJob::setLocalFilesOnly(value);
    }

    bool thumbnailLocalFilesOnly() const {
        return Fm::ThumbnailJob::localFilesOnly();
    }

    int maxThumbnailFileSize() const {
        return Fm::ThumbnailJob::maxThumbnailFileSize();
    }

    void setMaxThumbnailFileSize(int size) {
        Fm::ThumbnailJob::setMaxThumbnailFileSize(size);
    }

    void setThumbnailIconSize(int thumbnailIconSize) {
        thumbnailIconSize_ = thumbnailIconSize;
    }

    bool siUnit() {
        return siUnit_;
    }

    void setSiUnit(bool siUnit) {
        siUnit_ = siUnit;
        // override libfm FmConfig settings. FIXME: should we do this?
        fm_config->si_unit = (gboolean)siUnit_;
    }

    bool backupAsHidden() const {
        return backupAsHidden_;
    }

    void setBackupAsHidden(bool value) {
        backupAsHidden_ = value;
        fm_config->backup_as_hidden = backupAsHidden_; // also set this to libfm since fm_file_info_is_hidden() reads this value internally.
    }

    bool showFullNames() const {
        return showFullNames_;
    }

    void setShowFullNames(bool value) {
        showFullNames_ = value;
    }

    bool shadowHidden() const {
        return shadowHidden_;
    }

    void setShadowHidden(bool value) {
        shadowHidden_ = value;
    }

    bool noItemTooltip() const {
        return noItemTooltip_;
    }

    void setNoItemTooltip(bool noTooltip) {
        noItemTooltip_ = noTooltip;
    }

    bool onlyUserTemplates() const {
        return onlyUserTemplates_;
    }

    void setOnlyUserTemplates(bool value) {
        onlyUserTemplates_ = value;
        fm_config->only_user_templates = onlyUserTemplates_;
    }

    bool templateTypeOnce() const {
        return templateTypeOnce_;
    }

    void setTemplateTypeOnce(bool value) {
        templateTypeOnce_ = value;
        fm_config->template_type_once = templateTypeOnce_;
    }

    bool templateRunApp() const {
        return templateRunApp_;
    }

    void setTemplateRunApp(bool value) {
        templateRunApp_ = value;
        fm_config->template_run_app = templateRunApp_;
    }

    // per-folder settings
    FolderSettings loadFolderSettings(const Fm::FilePath& path) const;

    void saveFolderSettings(const Fm::FilePath& path, const FolderSettings& settings);

    void clearFolderSettings(const Fm::FilePath& path) const;

    bool searchNameCaseInsensitive() const {
        return searchNameCaseInsensitive_;
    }

    void setSearchNameCaseInsensitive(bool caseInsensitive) {
        searchNameCaseInsensitive_ = caseInsensitive;
    }

    bool searchContentCaseInsensitive() const {
        return searchContentCaseInsensitive_;
    }

    void setsearchContentCaseInsensitive(bool caseInsensitive) {
        searchContentCaseInsensitive_ = caseInsensitive;
    }

    bool searchNameRegexp() const {
        return searchNameRegexp_;
    }

    void setSearchNameRegexp(bool reg) {
        searchNameRegexp_ = reg;
    }

    bool searchContentRegexp() const {
        return searchContentRegexp_;
    }

    void setSearchContentRegexp(bool reg) {
        searchContentRegexp_ = reg;
    }

    bool searchRecursive() const {
        return searchRecursive_;
    }

    void setSearchRecursive(bool rec) {
        searchRecursive_ = rec;
    }

    bool searchhHidden() const {
        return searchhHidden_;
    }

    void setSearchhHidden(bool hidden) {
        searchhHidden_ = hidden;
    }

    QList<int> getCustomColumnWidths() const {
        QList<int> l;
        for(auto width : qAsConst(customColumnWidths_)) {
            l << width.toInt();
        }
        return l;
    }

    void setCustomColumnWidths(const QList<int> &widths) {
        customColumnWidths_.clear();
        for(auto width : widths) {
            customColumnWidths_ << QVariant(width);
        }
    }

    QList<int> getHiddenColumns() const {
        QList<int> l;
        for(auto width : qAsConst(hiddenColumns_)) {
            l << width.toInt();
        }
        return l;
    }

    void setHiddenColumns(const QList<int> &columns) {
        hiddenColumns_.clear();
        for(auto column : columns) {
            hiddenColumns_ << QVariant(column);
        }
    }

private:
    int toIconSize(int size, IconType type) const;

    QString profileName_;
    bool supportTrash_;

    // PCManFM specific
    QString fallbackIconThemeName_;
    bool useFallbackIconTheme_;

    bool singleWindowMode_;
    OpenDirTargetType bookmarkOpenMethod_;
    QString suCommand_;
    QString terminal_;
    bool mountOnStartup_;
    bool mountRemovable_;
    bool autoRun_;
    bool closeOnUnmount_;

    int wallpaperMode_;
    QString wallpaper_;
    QSize wallpaperDialogSize_;
    int wallpaperDialogSplitterPos_;
    QString lastSlide_;
    QString wallpaperDir_;
    int slideShowInterval_;
    bool wallpaperRandomize_;
    bool transformWallpaper_;
    bool perScreenWallpaper_;
    QColor desktopBgColor_;
    QColor desktopFgColor_;
    QColor desktopShadowColor_;
    QFont desktopFont_;
    int desktopIconSize_;
    QStringList desktopShortcuts_;

    bool desktopShowHidden_;
    bool desktopHideItems_;
    Qt::SortOrder desktopSortOrder_;
    Fm::FolderModel::ColumnId desktopSortColumn_;
    bool desktopSortFolderFirst_;
    bool desktopSortHiddenLast_;

    bool alwaysShowTabs_;
    bool showTabClose_;
    bool switchToNewTab_;
    bool reopenLastTabs_;
    QStringList tabPaths_;
    bool rememberWindowSize_;
    int fixedWindowWidth_;
    int fixedWindowHeight_;
    int lastWindowWidth_;
    int lastWindowHeight_;
    bool lastWindowMaximized_;
    int splitterPos_;
    bool sidePaneVisible_;
    Fm::SidePane::Mode sidePaneMode_;
    bool showMenuBar_;
    bool splitView_;

    Fm::FolderView::ViewMode viewMode_;
    bool showHidden_;
    Qt::SortOrder sortOrder_;
    Fm::FolderModel::ColumnId sortColumn_;
    bool sortFolderFirst_;
    bool sortHiddenLast_;
    bool sortCaseSensitive_;
    bool showFilter_;
    bool pathBarButtons_;

    // settings for use with libfm
    bool singleClick_;
    int autoSelectionDelay_;
    bool ctrlRightClick_;
    bool useTrash_;
    bool confirmDelete_;
    bool noUsbTrash_; // do not trash files on usb removable devices
    bool confirmTrash_; // Confirm before moving files into "trash can"
    bool quickExec_; // Don't ask options on launch executable file
    bool selectNewFiles_;

    bool showThumbnails_;

    QString archiver_;
    bool siUnit_;
    bool backupAsHidden_;
    bool showFullNames_;
    bool shadowHidden_;
    bool noItemTooltip_;

    QSet<QString> hiddenPlaces_;

    int bigIconSize_;
    int smallIconSize_;
    int sidePaneIconSize_;
    int thumbnailIconSize_;

    bool onlyUserTemplates_;
    bool templateTypeOnce_;
    bool templateRunApp_;

    QSize folderViewCellMargins_;
    QSize desktopCellMargins_;

    // search settings
    bool searchNameCaseInsensitive_;
    bool searchContentCaseInsensitive_;
    bool searchNameRegexp_;
    bool searchContentRegexp_;
    bool searchRecursive_;
    bool searchhHidden_;

    // detailed list columns
    QList<QVariant> customColumnWidths_;
    QList<QVariant> hiddenColumns_;
};

}

#endif // PCMANFM_SETTINGS_H
