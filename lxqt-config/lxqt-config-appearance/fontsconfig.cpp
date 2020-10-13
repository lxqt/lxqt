/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2014 LXQt team
 * Authors:
 *   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "fontsconfig.h"
#include "ui_fontsconfig.h"
#include <QTreeWidget>
#include <QDebug>
#include <QSettings>
#include <QFont>
#include <QFile>
#include <QDir>
#include <QDesktopServices>
#include <QTextStream>
#include <QStringBuilder>
#include <QDomDocument>

#ifdef Q_WS_X11
extern void qt_x11_apply_settings_in_all_apps();
#endif

static const char* subpixelNames[] = {"none", "rgb", "bgr", "vrgb", "vbgr"};
static const char* hintStyleNames[] = {"hintnone", "hintslight", "hintmedium", "hintfull"};

FontsConfig::FontsConfig(LXQt::Settings* settings, QSettings* qtSettings, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::FontsConfig),
    mQtSettings(qtSettings),
    mSettings(settings),
    mFontConfigFile()
{
    ui->setupUi(this);

    initControls();

    connect(ui->fontName, QOverload<int>::of(&QComboBox::activated), this, &FontsConfig::settingsChanged);
    connect(ui->fontStyle, QOverload<int>::of(&QComboBox::activated), this, &FontsConfig::settingsChanged);
    connect(ui->fontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &FontsConfig::settingsChanged);
    connect(ui->antialias, &QAbstractButton::clicked, this, &FontsConfig::settingsChanged);
    connect(ui->subpixel, QOverload<int>::of(&QComboBox::activated), this, &FontsConfig::settingsChanged);
    connect(ui->hinting, &QAbstractButton::clicked, this, &FontsConfig::settingsChanged);
    connect(ui->hintStyle, QOverload<int>::of(&QComboBox::activated), this, &FontsConfig::settingsChanged);
    connect(ui->dpi, QOverload<int>::of(&QSpinBox::valueChanged), this, &FontsConfig::settingsChanged);
    connect(ui->autohint, &QAbstractButton::clicked, this, &FontsConfig::settingsChanged);
}


FontsConfig::~FontsConfig()
{
    delete ui;
}


void FontsConfig::initControls()
{
    // read Qt style settings from Qt Trolltech.conf config
    mQtSettings->beginGroup(QLatin1String("Qt"));

    QString fontName = mQtSettings->value(QStringLiteral("font")).toString();
    QFont font;
    font.fromString(fontName);
    ui->fontName->setCurrentFont(font);

    ui->fontSize->blockSignals(true);
    ui->fontSize->setValue(font.pointSize());
    ui->fontSize->blockSignals(false);

    int fontStyle = 0;
    if(font.bold())
      fontStyle = font.italic() ? 3 : 1;
    else if(font.italic())
      fontStyle = 2;
    ui->fontStyle->setCurrentIndex(fontStyle);

    mQtSettings->endGroup();

    // load fontconfig config
    ui->antialias->setChecked(mFontConfigFile.antialias());
    ui->autohint->setChecked(mFontConfigFile.autohint());

    QByteArray subpixelStr = mFontConfigFile.subpixel();
    int subpixel;
    for(subpixel = 0; subpixel < 5; ++subpixel)
    {
        if(subpixelStr == subpixelNames[subpixel])
            break;
    }
    if(subpixel < 5)
        ui->subpixel->setCurrentIndex(subpixel);

    ui->hinting->setChecked(mFontConfigFile.hinting());

    QByteArray hintStyleStr = mFontConfigFile.hintStyle();
    int hintStyle;
    for(hintStyle = 0; hintStyle < 4; ++hintStyle)
    {
        if(hintStyleStr == hintStyleNames[hintStyle])
            break;
    }
    if(hintStyle < 4)
        ui->hintStyle->setCurrentIndex(hintStyle);

    int dpi = mFontConfigFile.dpi();
    ui->dpi->blockSignals(true);
    ui->dpi->setValue(dpi);
    ui->dpi->blockSignals(false);

    update();
}

void FontsConfig::updateQtFont()
{
    if(mFontConfigFile.antialias() != ui->antialias->isChecked())
        mFontConfigFile.setAntialias(ui->antialias->isChecked());

    if(mFontConfigFile.dpi() != ui->dpi->value())
        mFontConfigFile.setDpi(ui->dpi->value());

    if(mFontConfigFile.hinting() != ui->hinting->isChecked())
        mFontConfigFile.setHinting(ui->hinting->isChecked());

    int index = ui->subpixel->currentIndex();
    if(index >= 0 && index <= 4 && mFontConfigFile.subpixel() != subpixelNames[index])
        mFontConfigFile.setSubpixel(subpixelNames[index]);

    index = ui->hintStyle->currentIndex();
    if(index >= 0 && index <= 3 && mFontConfigFile.hintStyle() != hintStyleNames[index])
        mFontConfigFile.setHintStyle(hintStyleNames[index]);

    if(mFontConfigFile.autohint() != ui->autohint->isChecked())
        mFontConfigFile.setAutohint(ui->autohint->isChecked());

    // FIXME: the change does not apply to some currently running Qt programs.
    // FIXME: does not work with KDE apps
    // TODO: also write the config values to GTK+ config files (gtk-2.0.rc and gtk3/settings.ini)
    // FIXME: the selected font does not apply to our own application. Why?

    QFont font = ui->fontName->currentFont();
    int size = ui->fontSize->value();
    bool bold = false;
    bool italic = false;
    switch(ui->fontStyle->currentIndex())
    {
        case 1:
            bold = true;
            break;
        case 2:
            italic = true;
            break;
        case 3:
          bold = italic = true;
    }

    font.setPointSize(size);
    font.setBold(bold);
    font.setItalic(italic);

    const QString fontStr = font.toString();
    if(mQtSettings->value(QLatin1String("Qt/font")).toString() != fontStr) {
        mQtSettings->beginGroup(QLatin1String("Qt"));
        mQtSettings->setValue(QStringLiteral("font"), fontStr);
        mQtSettings->endGroup();
        mQtSettings->sync();

        emit updateOtherSettings();

#ifdef Q_WS_X11
        qt_x11_apply_settings_in_all_apps();
#endif

        update();
    }
}
