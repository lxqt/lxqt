#include <libfm-qt/core/filepath.h>
#include "desktopentrydialog.h"
#include "ui_desktopentrydialog.h"
#include "xdgdir.h"
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QFileDialog>
#include <QPushButton>
namespace PCManFM {

DesktopEntryDialog::DesktopEntryDialog(QWidget* parent):
    QDialog(parent) {
    ui.setupUi(this);
    ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(ui.typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DesktopEntryDialog::onChangingType);
    connect(ui.iconButton, &QAbstractButton::clicked, this, &DesktopEntryDialog::onClickingIconButton);
    connect(ui.commandButton, &QAbstractButton::clicked, this, &DesktopEntryDialog::onClickingCommandButton);
}
DesktopEntryDialog::~DesktopEntryDialog() {
}

void DesktopEntryDialog::onChangingType(int type) {
    if(type == 0) {
        ui.commandLabel->setText(tr("Command:"));
    }
    else if(type == 1) {
        ui.commandLabel->setText(tr("URL:"));
    }
}

void DesktopEntryDialog::onClickingIconButton() {
    QString iconDir;
    QString iconThemeName = QIcon::themeName();
    QStringList icons = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                  QStringLiteral("icons"),
                                                  QStandardPaths::LocateDirectory);
    for(QStringList::ConstIterator it = icons.constBegin(); it != icons.constEnd(); ++it) {
        QString iconThemeFolder = *it + QLatin1String("/") + iconThemeName;
        if (QDir(iconThemeFolder).exists() && QFileInfo(iconThemeFolder).permission(QFileDevice::ReadUser)) {
            iconDir = iconThemeFolder;
            break;
        }
    }
    if(iconDir.isEmpty()) {
        iconDir = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                         QStringLiteral("icons"),
                                         QStandardPaths::LocateDirectory);
        if(iconDir.isEmpty()) {
            return;
        }
    }
    const QString iconPath = QFileDialog::getOpenFileName(this, tr("Select an icon"),
                                                          iconDir,
                                                          tr("Images (*.png *.xpm *.svg *.svgz )"));
    if(!iconPath.isEmpty()) {
        if(iconPath.startsWith(iconDir)) { // a theme icon
            QStringList parts = iconPath.split(QStringLiteral("/"), QString::SkipEmptyParts);
            if(!parts.isEmpty()) {
                QString iconName = parts.at(parts.count() - 1);
                int ln = iconName.lastIndexOf(QLatin1String("."));
                if(ln > -1) {
                    iconName.remove(ln, iconName.length() - ln);
                    ui.iconEdit->setText(iconName);
                }
            }
        }
        else { // an image file
            ui.iconEdit->setText(iconPath);
        }
    }
}

void DesktopEntryDialog::onClickingCommandButton() {
    if(ui.typeCombo->currentIndex() == 0) {
        const QString path = QFileDialog::getOpenFileName(this,
                                                          tr("Select an executable file"),
                                                          QString::fromUtf8(Fm::FilePath::homeDir().toString().get()));
        if(!path.isEmpty()) {
            ui.commandEdit->setText(path);
        }
    }
    else {
        const QUrl url = QFileDialog::getOpenFileUrl(this,
                                                     tr("Select a file"),
                                                     QUrl(QString::fromUtf8(Fm::FilePath::homeDir().toString().get())));
        if(!url.isEmpty()) {
            ui.commandEdit->setText(url.toString());
        }
    }
}

void DesktopEntryDialog::accept() {
    QString name = ui.nameEdit->text();
    if(name.isEmpty()) {
        name = QLatin1String("launcher");
    }
    GKeyFile* kf = g_key_file_new();
    g_key_file_set_string(kf, "Desktop Entry", "Name", name.toStdString().c_str());
    g_key_file_set_string(kf, "Desktop Entry", "GenericName", ui.descriptionEdit->text().toStdString().c_str());
    g_key_file_set_string(kf, "Desktop Entry", "Comment", ui.commentEdit->text().toStdString().c_str());
    if(ui.typeCombo->currentIndex() == 0) {
        g_key_file_set_string(kf, "Desktop Entry", "Exec", ui.commandEdit->text().toStdString().c_str());
        g_key_file_set_string(kf, "Desktop Entry", "Type", "Application");
    }
    else {
        QString cmd = ui.commandEdit->text();
        // correct the type if this is a special place
        if(cmd.startsWith(QLatin1String("computer:///"))
           || cmd.startsWith(QLatin1String("network:///"))
           || cmd.startsWith(QLatin1String("trash:///"))
           || cmd.startsWith(QLatin1String("menu://applications/"))) {
            cmd = QLatin1String("pcmanfm-qt ") + cmd;
            g_key_file_set_string(kf, "Desktop Entry", "Exec", cmd.toStdString().c_str());
            g_key_file_set_string(kf, "Desktop Entry", "Type", "Application");
        }
        else {
            g_key_file_set_string(kf, "Desktop Entry", "URL", cmd.toStdString().c_str());
            g_key_file_set_string(kf, "Desktop Entry", "Type", "Link");
        }
    }
    g_key_file_set_string(kf, "Desktop Entry", "Icon", ui.iconEdit->text().toStdString().c_str());
    g_key_file_set_string(kf, "Desktop Entry", "Terminal",
                          ui.terminalCombo->currentIndex() == 0 ? "false" : "true");

    // make file name from entry name but so that it doesn't exist on Desktop
    name = name.simplified();
    name.replace(QChar(QChar::Space), QLatin1Char('_'));
    QString suffix;
    int i = 0;
    while(QFile::exists(XdgDir::readDesktopDir() + QLatin1String("/") + name + suffix + QLatin1String(".desktop"))) {
        suffix = QString::number(i);
        i++;
    }
    name += suffix + QLatin1String(".desktop");

    auto desktopPath = Fm::FilePath::fromLocalPath(XdgDir::readDesktopDir().toStdString().c_str()).localPath();
    auto launcher = Fm::CStrPtr{g_build_filename(desktopPath.get(), name.toStdString().c_str(), nullptr)};
    g_key_file_save_to_file(kf, launcher.get(), nullptr);
    g_key_file_free(kf);

    Q_EMIT desktopEntryCreated(name);

    QDialog::accept();
}

} // namespace Fm
