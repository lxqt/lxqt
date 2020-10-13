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


#include "settings.h"
#include <QDir>
#include <QFile>
#include <QStringBuilder>
#include <QSettings>
#include <QApplication>
#include "desktopwindow.h"
#include <libfm-qt/utilities.h>
#include <libfm-qt/core/folderconfig.h>
#include <libfm-qt/core/terminal.h>
#include <QStandardPaths>

namespace PCManFM {

inline static const char* bookmarkOpenMethodToString(OpenDirTargetType value);
inline static OpenDirTargetType bookmarkOpenMethodFromString(const QString str);

inline static const char* wallpaperModeToString(int value);
inline static int wallpaperModeFromString(const QString str);

inline static const char* viewModeToString(Fm::FolderView::ViewMode value);
inline static Fm::FolderView::ViewMode viewModeFromString(const QString str);

inline static const char* sidePaneModeToString(Fm::SidePane::Mode value);
inline static Fm::SidePane::Mode sidePaneModeFromString(const QString& str);

inline static const char* sortOrderToString(Qt::SortOrder order);
inline static Qt::SortOrder sortOrderFromString(const QString str);

inline static const char* sortColumnToString(Fm::FolderModel::ColumnId value);
inline static Fm::FolderModel::ColumnId sortColumnFromString(const QString str);

Settings::Settings():
    QObject(),
    supportTrash_(Fm::uriExists("trash:///")), // check if trash:/// is supported
    fallbackIconThemeName_(),
    useFallbackIconTheme_(QIcon::themeName().isEmpty() || QIcon::themeName() == QLatin1String("hicolor")),
    singleWindowMode_(false),
    bookmarkOpenMethod_(OpenInCurrentTab),
    suCommand_(),
    terminal_(),
    mountOnStartup_(true),
    mountRemovable_(true),
    autoRun_(true),
    closeOnUnmount_(false),
    wallpaperMode_(0),
    wallpaper_(),
    wallpaperDialogSize_(QSize(700, 500)),
    wallpaperDialogSplitterPos_(200),
    lastSlide_(),
    wallpaperDir_(),
    slideShowInterval_(0),
    wallpaperRandomize_(false),
    transformWallpaper_(false),
    perScreenWallpaper_(false),
    desktopBgColor_(),
    desktopFgColor_(),
    desktopShadowColor_(),
    desktopIconSize_(48),
    desktopShowHidden_(false),
    desktopHideItems_(false),
    desktopSortOrder_(Qt::AscendingOrder),
    desktopSortColumn_(Fm::FolderModel::ColumnFileMTime),
    desktopSortFolderFirst_(true),
    desktopSortHiddenLast_(false),
    alwaysShowTabs_(true),
    showTabClose_(true),
    switchToNewTab_(false),
    reopenLastTabs_(false),
    rememberWindowSize_(true),
    fixedWindowWidth_(640),
    fixedWindowHeight_(480),
    lastWindowWidth_(640),
    lastWindowHeight_(480),
    lastWindowMaximized_(false),
    splitterPos_(120),
    sidePaneVisible_(true),
    sidePaneMode_(Fm::SidePane::ModePlaces),
    showMenuBar_(true),
    splitView_(false),
    viewMode_(Fm::FolderView::IconMode),
    showHidden_(false),
    sortOrder_(Qt::AscendingOrder),
    sortColumn_(Fm::FolderModel::ColumnFileName),
    sortFolderFirst_(true),
    sortHiddenLast_(false),
    sortCaseSensitive_(false),
    showFilter_(false),
    pathBarButtons_(true),
    // settings for use with libfm
    singleClick_(false),
    autoSelectionDelay_(600),
    ctrlRightClick_(false),
    useTrash_(true),
    confirmDelete_(true),
    noUsbTrash_(false),
    confirmTrash_(false),
    quickExec_(false),
    selectNewFiles_(false),
    showThumbnails_(true),
    archiver_(),
    siUnit_(false),
    backupAsHidden_(false),
    showFullNames_(true),
    shadowHidden_(true),
    noItemTooltip_(false),
    bigIconSize_(48),
    smallIconSize_(24),
    sidePaneIconSize_(24),
    thumbnailIconSize_(128),
    folderViewCellMargins_(QSize(3, 3)),
    desktopCellMargins_(QSize(3, 1)),
    searchNameCaseInsensitive_(false),
    searchContentCaseInsensitive_(false),
    searchNameRegexp_(true),
    searchContentRegexp_(true),
    searchRecursive_(false),
    searchhHidden_(false) {
}

Settings::~Settings() {

}

QString Settings::xdgUserConfigDir() {
    QString dirName;
    // WARNING: Don't use XDG_CONFIG_HOME with root because it might
    // give the user config directory if gksu-properties is set to su.
    if(geteuid() != 0) { // non-root user
        dirName = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    }
    if(dirName.isEmpty()) {
        dirName = QDir::homePath() + QLatin1String("/.config");
    }
    return dirName;
}

QString Settings::profileDir(QString profile, bool useFallback) {
    // try user-specific config file first
    QString dirName = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                      + QStringLiteral("/pcmanfm-qt/") + profile;
    QDir dir(dirName);

    // if user config dir does not exist, try system-wide config dirs instead
    if(!dir.exists() && useFallback) {
        QString fallbackDir;
        const QStringList confList = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
        for(const auto &thisConf : confList) {
            fallbackDir = thisConf + QStringLiteral("/pcmanfm-qt/") + profile;
            if(fallbackDir == dirName) {
                continue;
            }
            dir.setPath(fallbackDir);
            if(dir.exists()) {
                dirName = fallbackDir;
                break;
            }
        }
    }
    return dirName;
}

bool Settings::load(QString profile) {
    profileName_ = profile;
    QString fileName = profileDir(profile, true) + QStringLiteral("/settings.conf");
    return loadFile(fileName);
}

bool Settings::save(QString profile) {
    QString fileName = profileDir(profile.isEmpty() ? profileName_ : profile) + QStringLiteral("/settings.conf");
    return saveFile(fileName);
}

bool Settings::loadFile(QString filePath) {
    QSettings settings(filePath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("System"));
    fallbackIconThemeName_ = settings.value(QStringLiteral("FallbackIconThemeName")).toString();
    if(fallbackIconThemeName_.isEmpty()) {
        // FIXME: we should choose one from installed icon themes or get
        // the value from XSETTINGS instead of hard code a fallback value.
        fallbackIconThemeName_ = QLatin1String("oxygen"); // fallback icon theme name
    }
    suCommand_ = settings.value(QStringLiteral("SuCommand"), QStringLiteral("lxqt-sudo %s")).toString();
    setTerminal(settings.value(QStringLiteral("Terminal"), QStringLiteral("xterm")).toString());
    setArchiver(settings.value(QStringLiteral("Archiver"), QStringLiteral("file-roller")).toString());
    setSiUnit(settings.value(QStringLiteral("SIUnit"), false).toBool());

    setOnlyUserTemplates(settings.value(QStringLiteral("OnlyUserTemplates"), false).toBool());
    setTemplateTypeOnce(settings.value(QStringLiteral("TemplateTypeOnce"), false).toBool());
    setTemplateRunApp(settings.value(QStringLiteral("TemplateRunApp"), false).toBool());

    settings.endGroup();

    settings.beginGroup(QStringLiteral("Behavior"));
    singleWindowMode_ = settings.value(QStringLiteral("SingleWindowMode"), false).toBool();
    bookmarkOpenMethod_ = bookmarkOpenMethodFromString(settings.value(QStringLiteral("BookmarkOpenMethod")).toString());
    // settings for use with libfm
    useTrash_ = settings.value(QStringLiteral("UseTrash"), true).toBool();
    singleClick_ = settings.value(QStringLiteral("SingleClick"), false).toBool();
    autoSelectionDelay_ = settings.value(QStringLiteral("AutoSelectionDelay"), 600).toInt();
    ctrlRightClick_ = settings.value(QStringLiteral("CtrlRightClick"), false).toBool();
    confirmDelete_ = settings.value(QStringLiteral("ConfirmDelete"), true).toBool();
    setNoUsbTrash(settings.value(QStringLiteral("NoUsbTrash"), false).toBool());
    confirmTrash_ = settings.value(QStringLiteral("ConfirmTrash"), false).toBool();
    setQuickExec(settings.value(QStringLiteral("QuickExec"), false).toBool());
    selectNewFiles_ = settings.value(QStringLiteral("SelectNewFiles"), false).toBool();
    // bool thumbnailLocal_;
    // bool thumbnailMax;
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Desktop"));
    wallpaperMode_ = wallpaperModeFromString(settings.value(QStringLiteral("WallpaperMode")).toString());
    wallpaper_ = settings.value(QStringLiteral("Wallpaper")).toString();
    wallpaperDialogSize_ = settings.value(QStringLiteral("WallpaperDialogSize"), QSize(700, 500)).toSize();
    wallpaperDialogSplitterPos_ = settings.value(QStringLiteral("WallpaperDialogSplitterPos"), 200).toInt();
    lastSlide_ = settings.value(QStringLiteral("LastSlide")).toString();
    wallpaperDir_ = settings.value(QStringLiteral("WallpaperDirectory")).toString();
    slideShowInterval_ = settings.value(QStringLiteral("SlideShowInterval"), 0).toInt();
    wallpaperRandomize_ = settings.value(QStringLiteral("WallpaperRandomize")).toBool();
    transformWallpaper_ = settings.value(QStringLiteral("TransformWallpaper")).toBool();
    perScreenWallpaper_ = settings.value(QStringLiteral("PerScreenWallpaper")).toBool();
    desktopBgColor_.setNamedColor(settings.value(QStringLiteral("BgColor"), QStringLiteral("#000000")).toString());
    desktopFgColor_.setNamedColor(settings.value(QStringLiteral("FgColor"), QStringLiteral("#ffffff")).toString());
    desktopShadowColor_.setNamedColor(settings.value(QStringLiteral("ShadowColor"), QStringLiteral("#000000")).toString());
    if(settings.contains(QStringLiteral("Font"))) {
        desktopFont_.fromString(settings.value(QStringLiteral("Font")).toString());
    }
    else {
        desktopFont_ = QApplication::font();
    }
    desktopIconSize_ = settings.value(QStringLiteral("DesktopIconSize"), 48).toInt();
    desktopShortcuts_ = settings.value(QStringLiteral("DesktopShortcuts")).toStringList();
    desktopShowHidden_ = settings.value(QStringLiteral("ShowHidden"), false).toBool();
    desktopHideItems_ = settings.value(QStringLiteral("HideItems"), false).toBool();

    desktopSortOrder_ = sortOrderFromString(settings.value(QStringLiteral("SortOrder")).toString());
    desktopSortColumn_ = sortColumnFromString(settings.value(QStringLiteral("SortColumn")).toString());
    desktopSortFolderFirst_ = settings.value(QStringLiteral("SortFolderFirst"), true).toBool();
    desktopSortHiddenLast_ = settings.value(QStringLiteral("SortHiddenLast"), false).toBool();

    desktopCellMargins_ = (settings.value(QStringLiteral("DesktopCellMargins"), QSize(3, 1)).toSize()
                           .expandedTo(QSize(0, 0))).boundedTo(QSize(48, 48));
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Volume"));
    mountOnStartup_ = settings.value(QStringLiteral("MountOnStartup"), true).toBool();
    mountRemovable_ = settings.value(QStringLiteral("MountRemovable"), true).toBool();
    autoRun_ = settings.value(QStringLiteral("AutoRun"), true).toBool();
    closeOnUnmount_ = settings.value(QStringLiteral("CloseOnUnmount"), true).toBool();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Thumbnail"));
    showThumbnails_ = settings.value(QStringLiteral("ShowThumbnails"), true).toBool();
    setMaxThumbnailFileSize(settings.value(QStringLiteral("MaxThumbnailFileSize"), 4096).toInt());
    setThumbnailLocalFilesOnly(settings.value(QStringLiteral("ThumbnailLocalFilesOnly"), true).toBool());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("FolderView"));
    viewMode_ = viewModeFromString(settings.value(QStringLiteral("Mode"), Fm::FolderView::IconMode).toString());
    showHidden_ = settings.value(QStringLiteral("ShowHidden"), false).toBool();
    sortOrder_ = sortOrderFromString(settings.value(QStringLiteral("SortOrder")).toString());
    sortColumn_ = sortColumnFromString(settings.value(QStringLiteral("SortColumn")).toString());
    sortFolderFirst_ = settings.value(QStringLiteral("SortFolderFirst"), true).toBool();
    sortHiddenLast_ = settings.value(QStringLiteral("SortHiddenLast"), false).toBool();
    sortCaseSensitive_ = settings.value(QStringLiteral("SortCaseSensitive"), false).toBool();
    showFilter_ = settings.value(QStringLiteral("ShowFilter"), false).toBool();

    setBackupAsHidden(settings.value(QStringLiteral("BackupAsHidden"), false).toBool());
    showFullNames_ = settings.value(QStringLiteral("ShowFullNames"), true).toBool();
    shadowHidden_ = settings.value(QStringLiteral("ShadowHidden"), true).toBool();
    noItemTooltip_ = settings.value(QStringLiteral("NoItemTooltip"), false).toBool();

    // override config in libfm's FmConfig
    bigIconSize_ = toIconSize(settings.value(QStringLiteral("BigIconSize"), 48).toInt(), Big);
    smallIconSize_ = toIconSize(settings.value(QStringLiteral("SmallIconSize"), 24).toInt(), Small);
    sidePaneIconSize_ = toIconSize(settings.value(QStringLiteral("SidePaneIconSize"), 24).toInt(), Small);
    thumbnailIconSize_ = toIconSize(settings.value(QStringLiteral("ThumbnailIconSize"), 128).toInt(), Thumbnail);

    folderViewCellMargins_ = (settings.value(QStringLiteral("FolderViewCellMargins"), QSize(3, 3)).toSize()
                              .expandedTo(QSize(0, 0))).boundedTo(QSize(48, 48));

    // detailed list columns
    customColumnWidths_ = settings.value(QStringLiteral("CustomColumnWidths")).toList();
    hiddenColumns_ = settings.value(QStringLiteral("HiddenColumns")).toList();

    settings.endGroup();

    settings.beginGroup(QStringLiteral("Places"));
    hiddenPlaces_ = settings.value(QStringLiteral("HiddenPlaces")).toStringList().toSet();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Window"));
    fixedWindowWidth_ = settings.value(QStringLiteral("FixedWidth"), 640).toInt();
    fixedWindowHeight_ = settings.value(QStringLiteral("FixedHeight"), 480).toInt();
    lastWindowWidth_ = settings.value(QStringLiteral("LastWindowWidth"), 640).toInt();
    lastWindowHeight_ = settings.value(QStringLiteral("LastWindowHeight"), 480).toInt();
    lastWindowMaximized_ = settings.value(QStringLiteral("LastWindowMaximized"), false).toBool();
    rememberWindowSize_ = settings.value(QStringLiteral("RememberWindowSize"), true).toBool();
    alwaysShowTabs_ = settings.value(QStringLiteral("AlwaysShowTabs"), true).toBool();
    showTabClose_ = settings.value(QStringLiteral("ShowTabClose"), true).toBool();
    switchToNewTab_ = settings.value(QStringLiteral("SwitchToNewTab"), false).toBool();
    reopenLastTabs_ = settings.value(QStringLiteral("ReopenLastTabs"), false).toBool();
    tabPaths_ = settings.value(QStringLiteral("TabPaths")).toStringList();
    splitterPos_ = settings.value(QStringLiteral("SplitterPos"), 150).toInt();
    sidePaneVisible_ = settings.value(QStringLiteral("SidePaneVisible"), true).toBool();
    sidePaneMode_ = sidePaneModeFromString(settings.value(QStringLiteral("SidePaneMode")).toString());
    showMenuBar_ = settings.value(QStringLiteral("ShowMenuBar"), true).toBool();
    splitView_ = settings.value(QStringLiteral("SplitView"), false).toBool();
    pathBarButtons_ = settings.value(QStringLiteral("PathBarButtons"), true).toBool();
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Search"));
    searchNameCaseInsensitive_ = settings.value(QStringLiteral("searchNameCaseInsensitive"), false).toBool();
    searchContentCaseInsensitive_ = settings.value(QStringLiteral("searchContentCaseInsensitive"), false).toBool();
    searchNameRegexp_ = settings.value(QStringLiteral("searchNameRegexp"), true).toBool();
    searchContentRegexp_ = settings.value(QStringLiteral("searchContentRegexp"), true).toBool();
    searchRecursive_ = settings.value(QStringLiteral("searchRecursive"), false).toBool();
    searchhHidden_ = settings.value(QStringLiteral("searchhHidden"), false).toBool();
    settings.endGroup();

    return true;
}

bool Settings::saveFile(QString filePath) {
    QSettings settings(filePath, QSettings::IniFormat);

    settings.beginGroup(QStringLiteral("System"));
    settings.setValue(QStringLiteral("FallbackIconThemeName"), fallbackIconThemeName_);
    settings.setValue(QStringLiteral("SuCommand"), suCommand_);
    settings.setValue(QStringLiteral("Terminal"), terminal_);
    settings.setValue(QStringLiteral("Archiver"), archiver_);
    settings.setValue(QStringLiteral("SIUnit"), siUnit_);

    settings.setValue(QStringLiteral("OnlyUserTemplates"), onlyUserTemplates_);
    settings.setValue(QStringLiteral("TemplateTypeOnce"), templateTypeOnce_);
    settings.setValue(QStringLiteral("TemplateRunApp"), templateRunApp_);

    settings.endGroup();

    settings.beginGroup(QStringLiteral("Behavior"));
    settings.setValue(QStringLiteral("SingleWindowMode"), singleWindowMode_);
    settings.setValue(QStringLiteral("BookmarkOpenMethod"), QString::fromUtf8(bookmarkOpenMethodToString(bookmarkOpenMethod_)));
    // settings for use with libfm
    settings.setValue(QStringLiteral("UseTrash"), useTrash_);
    settings.setValue(QStringLiteral("SingleClick"), singleClick_);
    settings.setValue(QStringLiteral("AutoSelectionDelay"), autoSelectionDelay_);
    settings.setValue(QStringLiteral("CtrlRightClick"), ctrlRightClick_);
    settings.setValue(QStringLiteral("ConfirmDelete"), confirmDelete_);
    settings.setValue(QStringLiteral("NoUsbTrash"), noUsbTrash_);
    settings.setValue(QStringLiteral("ConfirmTrash"), confirmTrash_);
    settings.setValue(QStringLiteral("QuickExec"), quickExec_);
    settings.setValue(QStringLiteral("SelectNewFiles"), selectNewFiles_);
    // bool thumbnailLocal_;
    // bool thumbnailMax;
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Desktop"));
    settings.setValue(QStringLiteral("WallpaperMode"), QString::fromUtf8(wallpaperModeToString(wallpaperMode_)));
    settings.setValue(QStringLiteral("Wallpaper"), wallpaper_);
    settings.setValue(QStringLiteral("WallpaperDialogSize"), wallpaperDialogSize_);
    settings.setValue(QStringLiteral("WallpaperDialogSplitterPos"), wallpaperDialogSplitterPos_);
    settings.setValue(QStringLiteral("LastSlide"), lastSlide_);
    settings.setValue(QStringLiteral("WallpaperDirectory"), wallpaperDir_);
    settings.setValue(QStringLiteral("SlideShowInterval"), slideShowInterval_);
    settings.setValue(QStringLiteral("WallpaperRandomize"), wallpaperRandomize_);
    settings.setValue(QStringLiteral("TransformWallpaper"), transformWallpaper_);
    settings.setValue(QStringLiteral("PerScreenWallpaper"), perScreenWallpaper_);
    settings.setValue(QStringLiteral("BgColor"), desktopBgColor_.name());
    settings.setValue(QStringLiteral("FgColor"), desktopFgColor_.name());
    settings.setValue(QStringLiteral("ShadowColor"), desktopShadowColor_.name());
    settings.setValue(QStringLiteral("Font"), desktopFont_.toString());
    settings.setValue(QStringLiteral("DesktopIconSize"), desktopIconSize_);
    settings.setValue(QStringLiteral("DesktopShortcuts"), desktopShortcuts_);
    settings.setValue(QStringLiteral("ShowHidden"), desktopShowHidden_);
    settings.setValue(QStringLiteral("HideItems"), desktopHideItems_);
    settings.setValue(QStringLiteral("SortOrder"), QString::fromUtf8(sortOrderToString(desktopSortOrder_)));
    settings.setValue(QStringLiteral("SortColumn"), QString::fromUtf8(sortColumnToString(desktopSortColumn_)));
    settings.setValue(QStringLiteral("SortFolderFirst"), desktopSortFolderFirst_);
    settings.setValue(QStringLiteral("SortHiddenLast"), desktopSortHiddenLast_);
    settings.setValue(QStringLiteral("DesktopCellMargins"), desktopCellMargins_);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Volume"));
    settings.setValue(QStringLiteral("MountOnStartup"), mountOnStartup_);
    settings.setValue(QStringLiteral("MountRemovable"), mountRemovable_);
    settings.setValue(QStringLiteral("AutoRun"), autoRun_);
    settings.setValue(QStringLiteral("CloseOnUnmount"), closeOnUnmount_);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Thumbnail"));
    settings.setValue(QStringLiteral("ShowThumbnails"), showThumbnails_);
    settings.setValue(QStringLiteral("MaxThumbnailFileSize"), maxThumbnailFileSize());
    settings.setValue(QStringLiteral("ThumbnailLocalFilesOnly"), thumbnailLocalFilesOnly());
    settings.endGroup();

    settings.beginGroup(QStringLiteral("FolderView"));
    settings.setValue(QStringLiteral("Mode"), QString::fromUtf8(viewModeToString(viewMode_)));
    settings.setValue(QStringLiteral("ShowHidden"), showHidden_);
    settings.setValue(QStringLiteral("SortOrder"), QString::fromUtf8(sortOrderToString(sortOrder_)));
    settings.setValue(QStringLiteral("SortColumn"), QString::fromUtf8(sortColumnToString(sortColumn_)));
    settings.setValue(QStringLiteral("SortFolderFirst"), sortFolderFirst_);
    settings.setValue(QStringLiteral("SortHiddenLast"), sortHiddenLast_);
    settings.setValue(QStringLiteral("SortCaseSensitive"), sortCaseSensitive_);
    settings.setValue(QStringLiteral("ShowFilter"), showFilter_);

    settings.setValue(QStringLiteral("BackupAsHidden"), backupAsHidden_);
    settings.setValue(QStringLiteral("ShowFullNames"), showFullNames_);
    settings.setValue(QStringLiteral("ShadowHidden"), shadowHidden_);
    settings.setValue(QStringLiteral("NoItemTooltip"), noItemTooltip_);

    // override config in libfm's FmConfig
    settings.setValue(QStringLiteral("BigIconSize"), bigIconSize_);
    settings.setValue(QStringLiteral("SmallIconSize"), smallIconSize_);
    settings.setValue(QStringLiteral("SidePaneIconSize"), sidePaneIconSize_);
    settings.setValue(QStringLiteral("ThumbnailIconSize"), thumbnailIconSize_);

    settings.setValue(QStringLiteral("FolderViewCellMargins"), folderViewCellMargins_);

    // detailed list columns
    settings.setValue(QStringLiteral("CustomColumnWidths"), customColumnWidths_);
    std::sort(hiddenColumns_.begin(), hiddenColumns_.end());
    settings.setValue(QStringLiteral("HiddenColumns"), hiddenColumns_);

    settings.endGroup();

    settings.beginGroup(QStringLiteral("Places"));
    QStringList hiddenPlaces = hiddenPlaces_.toList();
    settings.setValue(QStringLiteral("HiddenPlaces"), hiddenPlaces);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("Window"));
    settings.setValue(QStringLiteral("FixedWidth"), fixedWindowWidth_);
    settings.setValue(QStringLiteral("FixedHeight"), fixedWindowHeight_);
    settings.setValue(QStringLiteral("LastWindowWidth"), lastWindowWidth_);
    settings.setValue(QStringLiteral("LastWindowHeight"), lastWindowHeight_);
    settings.setValue(QStringLiteral("LastWindowMaximized"), lastWindowMaximized_);
    settings.setValue(QStringLiteral("RememberWindowSize"), rememberWindowSize_);
    settings.setValue(QStringLiteral("AlwaysShowTabs"), alwaysShowTabs_);
    settings.setValue(QStringLiteral("ShowTabClose"), showTabClose_);
    settings.setValue(QStringLiteral("SwitchToNewTab"), switchToNewTab_);
    settings.setValue(QStringLiteral("ReopenLastTabs"), reopenLastTabs_);
    settings.setValue(QStringLiteral("TabPaths"), tabPaths_);
    settings.setValue(QStringLiteral("SplitterPos"), splitterPos_);
    settings.setValue(QStringLiteral("SidePaneVisible"), sidePaneVisible_);
    settings.setValue(QStringLiteral("SidePaneMode"), QString::fromUtf8(sidePaneModeToString(sidePaneMode_)));
    settings.setValue(QStringLiteral("ShowMenuBar"), showMenuBar_);
    settings.setValue(QStringLiteral("SplitView"), splitView_);
    settings.setValue(QStringLiteral("PathBarButtons"), pathBarButtons_);
    settings.endGroup();

    // save per-folder settings
    Fm::FolderConfig::saveCache();

    settings.beginGroup(QStringLiteral("Search"));
    settings.setValue(QStringLiteral("searchNameCaseInsensitive"), searchNameCaseInsensitive_);
    settings.setValue(QStringLiteral("searchContentCaseInsensitive"), searchContentCaseInsensitive_);
    settings.setValue(QStringLiteral("searchNameRegexp"), searchNameRegexp_);
    settings.setValue(QStringLiteral("searchContentRegexp"), searchContentRegexp_);
    settings.setValue(QStringLiteral("searchRecursive"), searchRecursive_);
    settings.setValue(QStringLiteral("searchhHidden"), searchhHidden_);
    settings.endGroup();

    return true;
}

const QList<int> & Settings::iconSizes(IconType type) {
    static const QList<int> sizes_big = {96, 72, 64, 48, 32};
    static const QList<int> sizes_thumbnail = {256, 224, 192, 160, 128, 96, 64};
    static const QList<int> sizes_small = {48, 32, 24, 22, 16};
    switch(type) {
    case Big:
        return sizes_big;
        break;
    case Thumbnail:
        return sizes_thumbnail;
        break;
    case Small:
    default:
        return sizes_small;
        break;
    }
}

int Settings::toIconSize(int size, IconType type) const {
    const QList<int> & sizes = iconSizes(type);
    for (const auto & s : sizes) {
        if(size >= s) {
            return s;
        }
    }
    return sizes.back();
}

static const char* bookmarkOpenMethodToString(OpenDirTargetType value) {
    switch(value) {
    case OpenInCurrentTab:
    default:
        return "current_tab";
    case OpenInNewTab:
        return "new_tab";
    case OpenInNewWindow:
        return "new_window";
    case OpenInLastActiveWindow:
        return "last_window";
    }
    return "";
}

static OpenDirTargetType bookmarkOpenMethodFromString(const QString str) {

    if(str == QStringLiteral("new_tab")) {
        return OpenInNewTab;
    }
    else if(str == QStringLiteral("new_window")) {
        return OpenInNewWindow;
    }
    else if(str == QStringLiteral("last_window")) {
        return OpenInLastActiveWindow;
    }
    return OpenInCurrentTab;
}

static const char* viewModeToString(Fm::FolderView::ViewMode value) {
    const char* ret;
    switch(value) {
    case Fm::FolderView::IconMode:
    default:
        ret = "icon";
        break;
    case Fm::FolderView::CompactMode:
        ret = "compact";
        break;
    case Fm::FolderView::DetailedListMode:
        ret = "detailed";
        break;
    case Fm::FolderView::ThumbnailMode:
        ret = "thumbnail";
        break;
    }
    return ret;
}

Fm::FolderView::ViewMode viewModeFromString(const QString str) {
    Fm::FolderView::ViewMode ret;
    if(str == QLatin1String("icon")) {
        ret = Fm::FolderView::IconMode;
    }
    else if(str == QLatin1String("compact")) {
        ret = Fm::FolderView::CompactMode;
    }
    else if(str == QLatin1String("detailed")) {
        ret = Fm::FolderView::DetailedListMode;
    }
    else if(str == QLatin1String("thumbnail")) {
        ret = Fm::FolderView::ThumbnailMode;
    }
    else {
        ret = Fm::FolderView::IconMode;
    }
    return ret;
}

static const char* sortOrderToString(Qt::SortOrder order) {
    return (order == Qt::DescendingOrder ? "descending" : "ascending");
}

static Qt::SortOrder sortOrderFromString(const QString str) {
    return (str == QLatin1String("descending") ? Qt::DescendingOrder : Qt::AscendingOrder);
}

static const char* sortColumnToString(Fm::FolderModel::ColumnId value) {
    const char* ret;
    switch(value) {
    case Fm::FolderModel::ColumnFileName:
    default:
        ret = "name";
        break;
    case Fm::FolderModel::ColumnFileType:
        ret = "type";
        break;
    case Fm::FolderModel::ColumnFileSize:
        ret = "size";
        break;
    case Fm::FolderModel::ColumnFileMTime:
        ret = "mtime";
        break;
    case Fm::FolderModel::ColumnFileDTime:
        ret = "dtime";
        break;
    case Fm::FolderModel::ColumnFileOwner:
        ret = "owner";
        break;
    case Fm::FolderModel::ColumnFileGroup:
        ret = "group";
        break;
    }
    return ret;
}

static Fm::FolderModel::ColumnId sortColumnFromString(const QString str) {
    Fm::FolderModel::ColumnId ret;
    if(str == QLatin1String("name")) {
        ret = Fm::FolderModel::ColumnFileName;
    }
    else if(str == QLatin1String("type")) {
        ret = Fm::FolderModel::ColumnFileType;
    }
    else if(str == QLatin1String("size")) {
        ret = Fm::FolderModel::ColumnFileSize;
    }
    else if(str == QLatin1String("mtime")) {
        ret = Fm::FolderModel::ColumnFileMTime;
    }
    else if(str == QLatin1String("dtime")) {
        ret = Fm::FolderModel::ColumnFileDTime;
    }
    else if(str == QLatin1String("owner")) {
        ret = Fm::FolderModel::ColumnFileOwner;
    }
    else if(str == QLatin1String("group")) {
        ret = Fm::FolderModel::ColumnFileGroup;
    }
    else {
        ret = Fm::FolderModel::ColumnFileName;
    }
    return ret;
}

static const char* wallpaperModeToString(int value) {
    const char* ret;
    switch(value) {
    case DesktopWindow::WallpaperNone:
    default:
        ret = "none";
        break;
    case DesktopWindow::WallpaperStretch:
        ret = "stretch";
        break;
    case DesktopWindow::WallpaperFit:
        ret = "fit";
        break;
    case DesktopWindow::WallpaperCenter:
        ret = "center";
        break;
    case DesktopWindow::WallpaperTile:
        ret = "tile";
        break;
    case DesktopWindow::WallpaperZoom:
        ret = "zoom";
        break;
    }
    return ret;
}

static int wallpaperModeFromString(const QString str) {
    int ret;
    if(str == QLatin1String("stretch")) {
        ret = DesktopWindow::WallpaperStretch;
    }
    else if(str == QLatin1String("fit")) {
        ret = DesktopWindow::WallpaperFit;
    }
    else if(str == QLatin1String("center")) {
        ret = DesktopWindow::WallpaperCenter;
    }
    else if(str == QLatin1String("tile")) {
        ret = DesktopWindow::WallpaperTile;
    }
    else if(str == QLatin1String("zoom")) {
        ret = DesktopWindow::WallpaperZoom;
    }
    else {
        ret = DesktopWindow::WallpaperNone;
    }
    return ret;
}

static const char* sidePaneModeToString(Fm::SidePane::Mode value) {
    const char* ret;
    switch(value) {
    case Fm::SidePane::ModePlaces:
    default:
        ret = "places";
        break;
    case Fm::SidePane::ModeDirTree:
        ret = "dirtree";
        break;
    case Fm::SidePane::ModeNone:
        ret = "none";
        break;
    }
    return ret;
}

static Fm::SidePane::Mode sidePaneModeFromString(const QString& str) {
    Fm::SidePane::Mode ret;
    if(str == QLatin1String("none")) {
        ret = Fm::SidePane::ModeNone;
    }
    else if(str == QLatin1String("dirtree")) {
        ret = Fm::SidePane::ModeDirTree;
    }
    else {
        ret = Fm::SidePane::ModePlaces;
    }
    return ret;
}

void Settings::setTerminal(QString terminalCommand) {
    terminal_ = terminalCommand;
    Fm::setDefaultTerminal(terminal_.toStdString());
}


// per-folder settings
FolderSettings Settings::loadFolderSettings(const Fm::FilePath& path) const {
    FolderSettings settings;
    Fm::FolderConfig cfg(path);
    if(cfg.isEmpty()) {
        // the folder is not customized; use the general settings
        settings.setSortOrder(sortOrder());
        settings.setSortColumn(sortColumn());
        settings.setViewMode(viewMode());
        settings.setShowHidden(showHidden());
        settings.setSortFolderFirst(sortFolderFirst());
        settings.setSortHiddenLast(sortHiddenLast());
        settings.setSortCaseSensitive(sortCaseSensitive());
    }
    else {
        // the folder is customized; load folder-specific settings
        settings.setCustomized(true);

        char* str;
        // load sorting
        str = cfg.getString("SortOrder");
        if(str != nullptr) {
            settings.setSortOrder(sortOrderFromString(QString::fromUtf8(str)));
            g_free(str);
        }

        str = cfg.getString("SortColumn");
        if(str != nullptr) {
            settings.setSortColumn(sortColumnFromString(QString::fromUtf8(str)));
            g_free(str);
        }

        str = cfg.getString("ViewMode");
        if(str != nullptr) {
            // set view mode
            settings.setViewMode(viewModeFromString(QString::fromUtf8(str)));
            g_free(str);
        }

        bool show_hidden;
        if(cfg.getBoolean("ShowHidden", &show_hidden)) {
            settings.setShowHidden(show_hidden);
        }

        bool folder_first;
        if(cfg.getBoolean("SortFolderFirst", &folder_first)) {
            settings.setSortFolderFirst(folder_first);
        }

        bool hidden_last;
        if(cfg.getBoolean("SortHiddenLast", &hidden_last)) {
            settings.setSortHiddenLast(hidden_last);
        }

        bool case_sensitive;
        if(cfg.getBoolean("SortCaseSensitive", &case_sensitive)) {
            settings.setSortCaseSensitive(case_sensitive);
        }
    }
    return settings;
}

void Settings::saveFolderSettings(const Fm::FilePath& path, const FolderSettings& settings) {
    if(path) {
        // ensure that we have the libfm dir
        QString dirName = xdgUserConfigDir() + QStringLiteral("/libfm");
        QDir().mkpath(dirName);  // if libfm config dir does not exist, create it

        Fm::FolderConfig cfg(path);
        cfg.setString("SortOrder", sortOrderToString(settings.sortOrder()));
        cfg.setString("SortColumn", sortColumnToString(settings.sortColumn()));
        cfg.setString("ViewMode", viewModeToString(settings.viewMode()));
        cfg.setBoolean("ShowHidden", settings.showHidden());
        cfg.setBoolean("SortFolderFirst", settings.sortFolderFirst());
        cfg.setBoolean("SortHiddenLast", settings.sortHiddenLast());
        cfg.setBoolean("SortCaseSensitive", settings.sortCaseSensitive());
    }
}

void Settings::clearFolderSettings(const Fm::FilePath& path) const {
    if(path) {
        Fm::FolderConfig cfg(path);
        cfg.purge();
    }
}


} // namespace PCManFM
