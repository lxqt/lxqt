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


#include "preferencesdialog.h"
#include "application.h"
#include "settings.h"
#include <QDir>
#include <QHash>
#include <QStringBuilder>
#include <QSettings>
#include <QPushButton>
#include <libfm-qt/folderview.h>
#include <libfm-qt/core/terminal.h>
#include <libfm-qt/core/archiver.h>

namespace PCManFM {

PreferencesDialog::PreferencesDialog(QString activePage, QWidget* parent):
    QDialog(parent) {
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    warningCounter_ = 0;
    ui.warningLabel->hide();
    ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    // resize the list widget according to the width of its content.
    ui.listWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    ui.listWidget->setMaximumWidth(ui.listWidget->sizeHintForColumn(0) + ui.listWidget->frameWidth() * 2 + 4);
    initFromSettings();

    selectPage(activePage);
    adjustSize();
}

PreferencesDialog::~PreferencesDialog() {

}

static void findIconThemesInDir(QHash<QString, QString>& iconThemes, QString dirName) {
    QDir dir(dirName);
    const QStringList subDirs = dir.entryList(QDir::AllDirs);
    GKeyFile* kf = g_key_file_new();
    for(const QString& subDir : subDirs) {
        QString indexFile = dirName + QLatin1Char('/') + subDir + QStringLiteral("/index.theme");
        if(g_key_file_load_from_file(kf, indexFile.toLocal8Bit().constData(), GKeyFileFlags(0), nullptr)) {
            // FIXME: skip hidden ones
            // icon theme must have this key, so it has icons if it has this key
            // otherwise, it might be a cursor theme or any other kind of theme.
            if(g_key_file_has_key(kf, "Icon Theme", "Directories", nullptr)) {
                char* dispName = g_key_file_get_locale_string(kf, "Icon Theme", "Name", nullptr, nullptr);
                // char* comment = g_key_file_get_locale_string(kf, "Icon Theme", "Comment", nullptr, nullptr);
                iconThemes[subDir] = QString::fromUtf8(dispName);
                g_free(dispName);
            }
        }
    }
    g_key_file_free(kf);
}

void PreferencesDialog::initIconThemes(Settings& settings) {
    // check if auto-detection is done (for example, from xsettings)
    if(settings.useFallbackIconTheme()) { // auto-detection failed
        // load xdg icon themes and select the current one
        QHash<QString, QString> iconThemes;
        // user customed icon themes
        findIconThemesInDir(iconThemes, QString::fromUtf8(g_get_home_dir()) + QStringLiteral("/.icons"));

        // search for icons in system data dir
        const char* const* dataDirs = g_get_system_data_dirs();
        for(const char* const* dataDir = dataDirs; *dataDir; ++dataDir) {
            findIconThemesInDir(iconThemes, QString::fromUtf8(*dataDir) + QStringLiteral("/icons"));
        }

        iconThemes.remove(QStringLiteral("hicolor")); // remove hicolor, which is only a fallback
        QHash<QString, QString>::const_iterator it;
        for(it = iconThemes.constBegin(); it != iconThemes.constEnd(); ++it) {
            ui.iconTheme->addItem(it.value(), it.key());
        }
        ui.iconTheme->model()->sort(0); // sort the list of icon theme names

        // select current theme name
        int n = ui.iconTheme->count();
        int i;
        for(i = 0; i < n; ++i) {
            QVariant itemData = ui.iconTheme->itemData(i);
            if(itemData == settings.fallbackIconThemeName()) {
                break;
            }
        }
        if(i >= n) {
            i = 0;
        }
        ui.iconTheme->setCurrentIndex(i);
    }
    else { // auto-detection of icon theme works, hide the fallback icon theme combo box.
        ui.iconThemeLabel->hide();
        ui.iconTheme->hide();
    }

    ui.hMargin->setValue(settings.folderViewCellMargins().width());
    ui.vMargin->setValue(settings.folderViewCellMargins().height());
    connect(ui.lockMargins, &QAbstractButton::clicked, this, &PreferencesDialog::lockMargins);
}

void PreferencesDialog::initArchivers(Settings& settings) {
    auto& allArchivers = Fm::Archiver::allArchivers();
    for(int i = 0; i < int(allArchivers.size()); ++i) {
        auto& archiver = allArchivers[i];
        ui.archiver->addItem(QString::fromUtf8(archiver->program()), QString::fromUtf8(archiver->program()));
        if(QString::fromUtf8(archiver->program()) == settings.archiver()) {
            ui.archiver->setCurrentIndex(i);
        }
    }
}

void PreferencesDialog::initDisplayPage(Settings& settings) {
    initIconThemes(settings);
    // icon sizes
    int i = 0;
    for (const auto & size : Settings::iconSizes(Settings::Big)) {
        ui.bigIconSize->addItem(QStringLiteral("%1 x %1").arg(size), size);
        if(settings.bigIconSize() == size) {
            ui.bigIconSize->setCurrentIndex(i);
        }
        ++i;
    }
    i = 0;
    for (const auto & size : Settings::iconSizes(Settings::Small)) {
        QString text = QStringLiteral("%1 x %1").arg(size);
        ui.smallIconSize->addItem(text, size);
        if(settings.smallIconSize() == size) {
            ui.smallIconSize->setCurrentIndex(i);
        }

        ui.sidePaneIconSize->addItem(text, size);
        if(settings.sidePaneIconSize() == size) {
            ui.sidePaneIconSize->setCurrentIndex(i);
        }
        ++i;
    }
    i = 0;
    for (const auto & size : Settings::iconSizes(Settings::Thumbnail)) {
        ui.thumbnailIconSize->addItem(QStringLiteral("%1 x %1").arg(size), size);
        if(settings.thumbnailIconSize() == size) {
            ui.thumbnailIconSize->setCurrentIndex(i);
        }
        ++i;
    }

    ui.siUnit->setChecked(settings.siUnit());
    ui.backupAsHidden->setChecked(settings.backupAsHidden());

    ui.showFullNames->setChecked(settings.showFullNames());
    ui.shadowHidden->setChecked(settings.shadowHidden());
    ui.noItemTooltip->setChecked(settings.noItemTooltip());

    // app restart warning
    connect(ui.showFullNames, &QAbstractButton::toggled, [this, &settings] (bool checked) {
       restartWarning(settings.showFullNames() != checked);
    });
    connect(ui.shadowHidden, &QAbstractButton::toggled, [this, &settings] (bool checked) {
       restartWarning(settings.shadowHidden() != checked);
    });
}

void PreferencesDialog::initUiPage(Settings& settings) {
    ui.alwaysShowTabs->setChecked(settings.alwaysShowTabs());
    ui.showTabClose->setChecked(settings.showTabClose());
    ui.switchToNewTab->setChecked(settings.switchToNewTab());
    ui.reopenLastTabs->setChecked(settings.reopenLastTabs());
    ui.rememberWindowSize->setChecked(settings.rememberWindowSize());
    ui.fixedWindowWidth->setValue(settings.fixedWindowWidth());
    ui.fixedWindowHeight->setValue(settings.fixedWindowHeight());
}

void PreferencesDialog::initBehaviorPage(Settings& settings) {
    ui.singleClick->setChecked(settings.singleClick());
    ui.autoSelectionDelay->setValue(double(settings.autoSelectionDelay()) / 1000);
    ui.ctrlRightClick->setChecked(settings.ctrlRightClick());

    ui.bookmarkOpenMethod->setCurrentIndex(settings.bookmarkOpenMethod());

    ui.viewMode->addItem(tr("Icon View"), (int)Fm::FolderView::IconMode);
    ui.viewMode->addItem(tr("Compact View"), (int)Fm::FolderView::CompactMode);
    ui.viewMode->addItem(tr("Thumbnail View"), (int)Fm::FolderView::ThumbnailMode);
    ui.viewMode->addItem(tr("Detailed List View"), (int)Fm::FolderView::DetailedListMode);
    const Fm::FolderView::ViewMode modes[] = {
        Fm::FolderView::IconMode,
        Fm::FolderView::CompactMode,
        Fm::FolderView::ThumbnailMode,
        Fm::FolderView::DetailedListMode
    };
    for(std::size_t i = 0; i < G_N_ELEMENTS(modes); ++i) {
        if(modes[i] == settings.viewMode()) {
            ui.viewMode->setCurrentIndex(i);
            break;
        }
    }

    ui.configmDelete->setChecked(settings.confirmDelete());

    if(settings.supportTrash()) {
        ui.useTrash->setChecked(settings.useTrash());
    }
    else {
        ui.useTrash->hide();
    }

    ui.noUsbTrash->setChecked(settings.noUsbTrash());
    ui.confirmTrash->setChecked(settings.confirmTrash());
    ui.quickExec->setChecked(settings.quickExec());
    ui.selectNewFiles->setChecked(settings.selectNewFiles());
    ui.singleWindowMode->setChecked(settings.singleWindowMode());

    // app restart warning
    connect(ui.quickExec, &QAbstractButton::toggled, [this, &settings] (bool checked) {
       restartWarning(settings.quickExec() != checked);
    });
}

void PreferencesDialog::initThumbnailPage(Settings& settings) {
    ui.showThumbnails->setChecked(settings.showThumbnails());
    ui.thumbnailLocal->setChecked(settings.thumbnailLocalFilesOnly());
    ui.maxThumbnailFileSize->setValue(settings.maxThumbnailFileSize());
}

void PreferencesDialog::initVolumePage(Settings& settings) {
    ui.mountOnStartup->setChecked(settings.mountOnStartup());
    ui.mountRemovable->setChecked(settings.mountRemovable());
    ui.autoRun->setChecked(settings.autoRun());
    if(settings.closeOnUnmount()) {
        ui.closeOnUnmount->setChecked(true);
    }
    else {
        ui.goHomeOnUnmount->setChecked(true);
    }
}

void PreferencesDialog::initTerminals(Settings& settings) {
    // load the known terminal list from the terminal.list file of libfm
    for(auto& terminal: Fm::allKnownTerminals()) {
        ui.terminal->addItem(QString::fromUtf8(terminal.get()));
    }
    ui.terminal->setEditText(settings.terminal());
}

void PreferencesDialog::initAdvancedPage(Settings& settings) {
    initArchivers(settings);
    initTerminals(settings);
    ui.suCommand->setText(settings.suCommand());

    ui.onlyUserTemplates->setChecked(settings.onlyUserTemplates());
    ui.templateTypeOnce->setChecked(settings.templateTypeOnce());

    ui.templateRunApp->setChecked(settings.templateRunApp());

    // FIXME: Hide options that we don't support yet.
    ui.templateRunApp->hide();
}

void PreferencesDialog::initFromSettings() {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    initDisplayPage(settings);
    initUiPage(settings);
    initBehaviorPage(settings);
    initThumbnailPage(settings);
    initVolumePage(settings);
    initAdvancedPage(settings);
}

void PreferencesDialog::applyDisplayPage(Settings& settings) {
    if(settings.useFallbackIconTheme()) {
        // only apply the value if icon theme combo box is in use
        // the combo box is hidden when auto-detection of icon theme from xsettings works.
        QString newIconTheme = ui.iconTheme->itemData(ui.iconTheme->currentIndex()).toString();
        if(newIconTheme != settings.fallbackIconThemeName()) {
            settings.setFallbackIconThemeName(newIconTheme);
            QIcon::setThemeName(settings.fallbackIconThemeName());
            // update the UI by emitting a style change event
            const auto widgets = QApplication::allWidgets();
            for(QWidget* widget : widgets) {
                QEvent event(QEvent::StyleChange);
                QApplication::sendEvent(widget, &event);
            }
        }
    }

    settings.setBigIconSize(ui.bigIconSize->itemData(ui.bigIconSize->currentIndex()).toInt());
    settings.setSmallIconSize(ui.smallIconSize->itemData(ui.smallIconSize->currentIndex()).toInt());
    settings.setThumbnailIconSize(ui.thumbnailIconSize->itemData(ui.thumbnailIconSize->currentIndex()).toInt());
    settings.setSidePaneIconSize(ui.sidePaneIconSize->itemData(ui.sidePaneIconSize->currentIndex()).toInt());

    settings.setSiUnit(ui.siUnit->isChecked());
    settings.setBackupAsHidden(ui.backupAsHidden->isChecked());
    settings.setShowFullNames(ui.showFullNames->isChecked());
    settings.setShadowHidden(ui.shadowHidden->isChecked());
    settings.setNoItemTooltip(ui.noItemTooltip->isChecked());
    settings.setFolderViewCellMargins(QSize(ui.hMargin->value(), ui.vMargin->value()));
}

void PreferencesDialog::applyUiPage(Settings& settings) {
    settings.setAlwaysShowTabs(ui.alwaysShowTabs->isChecked());
    settings.setShowTabClose(ui.showTabClose->isChecked());
    settings.setSwitchToNewTab(ui.switchToNewTab->isChecked());
    settings.setReopenLastTabs(ui.reopenLastTabs->isChecked());
    settings.setRememberWindowSize(ui.rememberWindowSize->isChecked());
    settings.setFixedWindowWidth(ui.fixedWindowWidth->value());
    settings.setFixedWindowHeight(ui.fixedWindowHeight->value());
}

void PreferencesDialog::applyBehaviorPage(Settings& settings) {
    settings.setSingleClick(ui.singleClick->isChecked());
    settings.setAutoSelectionDelay(int(ui.autoSelectionDelay->value() * 1000));
    settings.setCtrlRightClick(ui.ctrlRightClick->isChecked());

    settings.setBookmarkOpenMethod(OpenDirTargetType(ui.bookmarkOpenMethod->currentIndex()));

    // FIXME: bug here?
    Fm::FolderView::ViewMode mode = Fm::FolderView::ViewMode(ui.viewMode->itemData(ui.viewMode->currentIndex()).toInt());
    settings.setViewMode(mode);
    settings.setConfirmDelete(ui.configmDelete->isChecked());

    if(settings.supportTrash()) {
        settings.setUseTrash(ui.useTrash->isChecked());
    }

    settings.setNoUsbTrash(ui.noUsbTrash->isChecked());
    settings.setConfirmTrash(ui.confirmTrash->isChecked());
    settings.setQuickExec(ui.quickExec->isChecked());
    settings.setSelectNewFiles(ui.selectNewFiles->isChecked());
    settings.setSingleWindowMode(ui.singleWindowMode->isChecked());
}

void PreferencesDialog::applyThumbnailPage(Settings& settings) {
    settings.setShowThumbnails(ui.showThumbnails->isChecked());
    settings.setThumbnailLocalFilesOnly(ui.thumbnailLocal->isChecked());
    settings.setMaxThumbnailFileSize(ui.maxThumbnailFileSize->value());
}

void PreferencesDialog::applyVolumePage(Settings& settings) {
    settings.setAutoRun(ui.autoRun->isChecked());
    settings.setMountOnStartup(ui.mountOnStartup->isChecked());
    settings.setMountRemovable(ui.mountRemovable->isChecked());
    settings.setCloseOnUnmount(ui.closeOnUnmount->isChecked());
}

void PreferencesDialog::applyAdvancedPage(Settings& settings) {
    settings.setTerminal(ui.terminal->currentText());
    settings.setSuCommand(ui.suCommand->text());
    settings.setArchiver(ui.archiver->itemData(ui.archiver->currentIndex()).toString());

    settings.setOnlyUserTemplates(ui.onlyUserTemplates->isChecked());
    settings.setTemplateTypeOnce(ui.templateTypeOnce->isChecked());
    settings.setTemplateRunApp(ui.templateRunApp->isChecked());
}


void PreferencesDialog::applySettings() {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    applyDisplayPage(settings);
    applyUiPage(settings);
    applyBehaviorPage(settings);
    applyThumbnailPage(settings);
    applyVolumePage(settings);
    applyAdvancedPage(settings);

    settings.save();

    Application* app = static_cast<Application*>(qApp);
    app->updateFromSettings();
}

void PreferencesDialog::accept() {
    applySettings();
    QDialog::accept();
}

void PreferencesDialog::selectPage(QString name) {
    if(!name.isEmpty()) {
        QWidget* page = findChild<QWidget*>(name + QStringLiteral("Page"));
        if(page) {
            int index = ui.stackedWidget->indexOf(page);
            ui.listWidget->setCurrentRow(index);
        }
    }
}

void PreferencesDialog::lockMargins(bool lock) {
    ui.vMargin->setDisabled(lock);
    if(lock) {
        ui.vMargin->setValue(ui.hMargin->value());
        connect(ui.hMargin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui.vMargin, &QSpinBox::setValue);
    }
    else {
        disconnect(ui.hMargin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui.vMargin, &QSpinBox::setValue);
    }
}

void PreferencesDialog::restartWarning(bool warn) {
    if(warn) {
        ++warningCounter_;
    }
    else {
        --warningCounter_;
    }
    ui.warningLabel->setVisible(warningCounter_ > 0);
}

} // namespace PCManFM
