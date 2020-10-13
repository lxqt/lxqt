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

#include "desktoppreferencesdialog.h"
#include "desktopwindow.h"
#include "settings.h"
#include "application.h"
#include "xdgdir.h"
#include <QFileDialog>
#include <QImageReader>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QRegExp>
#include <QDebug>
#include <QStandardPaths>
#include <libfm-qt/filedialog.h>
#include <QPushButton>
namespace PCManFM {

static int iconSizes[] = {96, 72, 64, 48, 36, 32, 24, 20};

DesktopPreferencesDialog::DesktopPreferencesDialog(QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    editDesktopFolderEnabled(false),
    desktopFolderWidget(0),
    desktopFolder() {

    setAttribute(Qt::WA_DeleteOnClose);
    Settings& settings = static_cast<Application*>(qApp)->settings();
    ui.setupUi(this);
    ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui.buttonBox_3->button(QDialogButtonBox::Apply)->setText(tr("Apply"));
    // setup wallpaper modes
    connect(ui.wallpaperMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DesktopPreferencesDialog::onWallpaperModeChanged);
    ui.wallpaperMode->addItem(tr("Fill with background color only"), DesktopWindow::WallpaperNone);
    ui.wallpaperMode->addItem(tr("Stretch to fill the entire screen"), DesktopWindow::WallpaperStretch);
    ui.wallpaperMode->addItem(tr("Stretch to fit the screen"), DesktopWindow::WallpaperFit);
    ui.wallpaperMode->addItem(tr("Center on the screen"), DesktopWindow::WallpaperCenter);
    ui.wallpaperMode->addItem(tr("Tile the image to fill the entire screen"), DesktopWindow::WallpaperTile);
    ui.wallpaperMode->addItem(tr("Zoom the image to fill the entire screen"), DesktopWindow::WallpaperZoom);
    int i;
    switch(settings.wallpaperMode()) {
    case DesktopWindow::WallpaperNone:
        i = 0;
        break;
    case DesktopWindow::WallpaperStretch:
        i = 1;
        break;
    case DesktopWindow::WallpaperFit:
        i = 2;
        break;
    case DesktopWindow::WallpaperCenter:
        i = 3;
        break;
    case DesktopWindow::WallpaperTile:
        i = 4;
        break;
    case DesktopWindow::WallpaperZoom:
        i = 5;
        break;
    default:
        i = 0;
    }
    ui.wallpaperMode->setCurrentIndex(i);
    int mode = ui.wallpaperMode->itemData(ui.wallpaperMode->currentIndex()).toInt();

    if (mode == DesktopWindow::WallpaperStretch || mode == DesktopWindow::WallpaperCenter
            || mode == DesktopWindow::WallpaperFit || mode == DesktopWindow::WallpaperZoom) {
        ui.perScreenWallpaper->setEnabled(true);
    }
    else
        ui.perScreenWallpaper->setEnabled(false);

    connect(ui.browse, &QPushButton::clicked, this, &DesktopPreferencesDialog::onBrowseClicked);
    qDebug("wallpaper: %s", settings.wallpaper().toUtf8().data());
    ui.imageFile->setText(settings.wallpaper());
    ui.transformImage->setChecked(settings.transformWallpaper());
    ui.perScreenWallpaper->setChecked(settings.perScreenWallpaper());

    ui.slideShow->setChecked(settings.slideShowInterval() > 0);
    ui.imageFolder->setText(settings.wallpaperDir());
    int minutes = qMax(settings.slideShowInterval() / 60000, 5); // 5 min at least
    ui.hours->setValue(minutes / 60);
    ui.minutes->setValue(minutes % 60);
    ui.randomize->setChecked(settings.wallpaperRandomize());
    connect(ui.folderBrowse, &QPushButton::clicked, this, &DesktopPreferencesDialog::onFolderBrowseClicked);

    for(std::size_t i = 0; i < G_N_ELEMENTS(iconSizes); ++i) {
        int size = iconSizes[i];
        ui.iconSize->addItem(QStringLiteral("%1 x %1").arg(size), size);
        if(settings.desktopIconSize() == size)
            ui.iconSize->setCurrentIndex(i);
    }

    ui.font->setFont(settings.desktopFont());

    ui.backgroundColor->setColor(settings.desktopBgColor());
    ui.textColor->setColor(settings.desktopFgColor());
    ui.shadowColor->setColor(settings.desktopShadowColor());

    const QStringList ds = settings.desktopShortcuts();
    ui.homeBox->setChecked(ds.contains(QLatin1String("Home")));
    ui.trashBox->setChecked(ds.contains(QLatin1String("Trash")));
    ui.computerBox->setChecked(ds.contains(QLatin1String("Computer")));
    ui.networkBox->setChecked(ds.contains(QLatin1String("Network")));

    connect(ui.buttonBox_3->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &DesktopPreferencesDialog::onApplyClicked);

    ui.hMargin->setValue(settings.desktopCellMargins().width());
    ui.vMargin->setValue(settings.desktopCellMargins().height());
    connect(ui.lockMargins, &QAbstractButton::clicked, this, &DesktopPreferencesDialog::lockMargins);

    resize(sizeHint()); // show it compact
}

DesktopPreferencesDialog::~DesktopPreferencesDialog() {
}

void DesktopPreferencesDialog::setupDesktopFolderUi()
{
    desktopFolderWidget = new QWidget();
    uiDesktopFolder.setupUi(desktopFolderWidget);
    ui.advancedPageLayout->insertWidget(1, desktopFolderWidget);
    uiDesktopFolder.verticalLayout->setMargin(0);

    desktopFolder = XdgDir::readDesktopDir();
    qDebug("desktop folder: %s", desktopFolder.toStdString().c_str());

    uiDesktopFolder.desktopFolder->setText(desktopFolder);

    connect(uiDesktopFolder.browseDesktopFolder, &QPushButton::clicked,
            this, &DesktopPreferencesDialog::onBrowseDesktopFolderClicked);
}

void DesktopPreferencesDialog::lockMargins(bool lock) {
    ui.vMargin->setDisabled(lock);
    if(lock) {
        ui.vMargin->setValue(ui.hMargin->value());
        connect(ui.hMargin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui.vMargin, &QSpinBox::setValue);
    }
    else
        disconnect(ui.hMargin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui.vMargin, &QSpinBox::setValue);
}

void DesktopPreferencesDialog::applySettings()
{
    Settings& settings = static_cast<Application*>(qApp)->settings();

    if (editDesktopFolderEnabled)
        XdgDir::setDesktopDir(uiDesktopFolder.desktopFolder->text());

    settings.setWallpaper(ui.imageFile->text());
    settings.setTransformWallpaper(ui.transformImage->isChecked());
    settings.setPerScreenWallpaper(ui.perScreenWallpaper->isChecked());
    int mode = ui.wallpaperMode->itemData(ui.wallpaperMode->currentIndex()).toInt();
    settings.setWallpaperMode(mode);

    settings.setWallpaperDir(ui.imageFolder->text());
    int interval = 0;
    if(ui.slideShow->isChecked())
        interval = (ui.minutes->value() + 60 * ui.hours->value()) * 60000;
    settings.setSlideShowInterval(interval);
    settings.setWallpaperRandomize(ui.randomize->isChecked());

    settings.setDesktopIconSize(ui.iconSize->itemData(ui.iconSize->currentIndex()).toInt());

    settings.setDesktopFont(ui.font->font());
    settings.setDesktopBgColor(ui.backgroundColor->color());
    settings.setDesktopFgColor(ui.textColor->color());
    settings.setDesktopShadowColor(ui.shadowColor->color());

    QStringList ds;
    if(ui.homeBox->isChecked()) {
        ds << QLatin1String("Home");
    }
    if(ui.trashBox->isChecked()) {
        ds << QLatin1String("Trash");
    }
    if(ui.computerBox->isChecked()) {
        ds << QLatin1String("Computer");
    }
    if(ui.networkBox->isChecked()) {
        ds << QLatin1String("Network");
    }
    settings.setDesktopShortcuts(ds);

    settings.setDesktopCellMargins(QSize(ui.hMargin->value(), ui.vMargin->value()));

    settings.save();
}

void DesktopPreferencesDialog::onApplyClicked()
{
    applySettings();
    static_cast<Application*>(qApp)->updateDesktopsFromSettings();
}

void DesktopPreferencesDialog::accept() {
    applySettings();
    static_cast<Application*>(qApp)->updateDesktopsFromSettings(false); // don't change slide wallpaper on clicking OK
    QDialog::accept();
}

void DesktopPreferencesDialog::onWallpaperModeChanged(int index) {
    int mode = ui.wallpaperMode->itemData(index).toInt();

    bool enable = (mode != DesktopWindow::WallpaperNone);
    ui.imageFile->setEnabled(enable);
    ui.browse->setEnabled(enable);
    ui.transformImage->setEnabled(enable);

    if (mode == DesktopWindow::WallpaperStretch || mode == DesktopWindow::WallpaperCenter
            || mode == DesktopWindow::WallpaperFit || mode == DesktopWindow::WallpaperZoom) {
        ui.perScreenWallpaper->setEnabled(true);
    }
    else
        ui.perScreenWallpaper->setEnabled(false);
}

void DesktopPreferencesDialog::onBrowseClicked() {
    Settings& settings = static_cast<Application*>(qApp)->settings();

    // use LXQt file dialog directly to set its view to thumbnail mode
    Fm::FileDialog dlg(this);
    dlg.resize(settings.wallpaperDialogSize());
    dlg.setSplitterPos(settings.wallpaperDialogSplitterPos());
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setWindowTitle(tr("Select Wallpaper"));
    // compose a name filter from QImageReader
    QString filter;
    filter.reserve(256);
    filter = tr("Image Files");
    filter += QLatin1String(" (");
    const QList<QByteArray> formats = QImageReader::supportedImageFormats();
    for(const QByteArray& format : formats) {
        filter += QLatin1String("*.");
        filter += QString::fromUtf8(format.toLower());
        filter += QLatin1Char(' ');
    }
    filter += QLatin1Char(')');
    dlg.setNameFilters(QStringList() << filter);
    dlg.setViewMode(Fm::FolderView::ThumbnailMode);

    // select an appropriate file
    QString path = ui.imageFile->text();
    if(path.isEmpty() || !QFile::exists(path)) {
        dlg.setDirectory(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)));
    }
    else if(QFileInfo(path).isDir()) {
        dlg.setDirectory(QUrl::fromLocalFile(path));
    }
    else { // the path is a file
        dlg.setDirectory(QUrl::fromLocalFile(path.section(QLatin1String("/"), 0, -2)));
        dlg.selectFile(QUrl::fromLocalFile(path));
    }

    connect(&dlg, &QDialog::finished, [this, &dlg, &settings](int result) {
        if(result == QDialog::Accepted) {
            QString filename;
            filename = dlg.selectedFiles().constFirst().toLocalFile();
            ui.imageFile->setText(filename);
        }
        settings.setWallpaperDialogSplitterPos(dlg.splitterPos());
        settings.setWallpaperDialogSize(dlg.size());
    });
    dlg.exec();
}

void DesktopPreferencesDialog::onFolderBrowseClicked() {
    QFileDialog dlg;
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setOption(QFileDialog::ShowDirsOnly);
    dlg.setWindowTitle(tr("Select Wallpaper Folder"));

    // select an appropriate dir
    QString path = ui.imageFolder->text();
    if(path.isEmpty() || !QFile::exists(path)) {
        path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }
    else if(!QFileInfo(path).isDir()) {
        path = path.section(QLatin1String("/"), 0, -2);
    }
    dlg.setDirectory(path.section(QLatin1String("/"), 0, -2));
    dlg.selectFile(path);

    if(dlg.exec() == QDialog::Accepted) {
        QString foldername;
        foldername = dlg.selectedFiles().constFirst();
        ui.imageFolder->setText(foldername);
    }
}

void DesktopPreferencesDialog::onBrowseDesktopFolderClicked()
{
    QFileDialog dlg;
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::DirectoryOnly);
    if (dlg.exec() == QDialog::Accepted) {
        QString dir;
        dir = dlg.selectedFiles().constFirst();
        uiDesktopFolder.desktopFolder->setText(dir);
    }
}

void DesktopPreferencesDialog::selectPage(QString name) {
    QWidget* page = findChild<QWidget*>(name + QStringLiteral("Page"));
    if(page)
        ui.tabWidget->setCurrentWidget(page);
}

void DesktopPreferencesDialog::setEditDesktopFolder(const bool enabled)
{
    editDesktopFolderEnabled = enabled;
    if (editDesktopFolderEnabled)
        setupDesktopFolderUi();
}

} // namespace PCManFM
