/*
    Copyright (C) 2016  P.L. Lucas <selairi@gmail.com>

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

#include "fastmenu.h"
#include "timeoutdialog.h"
#include "kscreenutils.h"

#include <QComboBox>
#include <QPoint>
#include <KScreen/Output>
#include <KScreen/Mode>
#include <KScreen/SetConfigOperation>

#include <algorithm>

enum Options
{
	None=0, Extended=1, Unified=2, OnlyFirst=3, OnlySecond=4
};

FastMenu::FastMenu(KScreen::ConfigPtr config, QWidget* parent) :
    QGroupBox(parent)
{
    this->mConfig = config;
    this->mOldConfig = mConfig->clone();

    ui.setupUi(this);

    connect(ui.comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onSeleccionChanged(int)));
}

FastMenu::~FastMenu()
{
}

static bool sizeBiggerThan(const QSize &sizeA, const QSize &sizeB)
{
    return sizeA.width() * sizeA.height() > sizeB.width() * sizeB.height();
}

void FastMenu::unified()
{
    const KScreen::OutputList outputs = mConfig->outputs();
    // Look for common size
    QList<QSize> commonSizes;
    for (const KScreen::OutputPtr &output : outputs) {
        if( !output->isConnected() )
            continue;

        const auto modes = output->modes();
        for(const KScreen::ModePtr &mode : modes) {
            commonSizes.append(mode->size());
        }
        break;
    }
    for (const KScreen::OutputPtr &output : outputs) {
        if( !output->isConnected() )
            continue;
        QList<QSize> sizes;
        const auto modes = output->modes();
        for(const KScreen::ModePtr &mode : modes) {
            if( commonSizes.contains(mode->size()) )
                sizes.append(mode->size());
        }
        commonSizes = sizes;
    }
    // Select the bigest common size
    std::sort(commonSizes.begin(), commonSizes.end(), sizeBiggerThan);
    if(commonSizes.isEmpty())
        return;
    QSize commonSize = commonSizes[0];
    // Put all monitors in (0,0) position and set size
    for (const KScreen::OutputPtr &output : outputs) {
        if( !output->isConnected() )
            continue;
        QPoint pos = output->pos();
        pos.setX(0);
        pos.setY(0);
        output->setPos(pos);
        output->setEnabled(true);
        // Select mode with the biggest refresh rate
        float maxRefreshRate = 0.0;
        const auto outputs = output->modes();
        for(const KScreen::ModePtr &mode : outputs) {
            if(mode->size() == commonSize && maxRefreshRate < mode->refreshRate()) {
                output->setCurrentModeId(mode->id());
                maxRefreshRate = mode->refreshRate();
            }
        }
    }
}

void FastMenu::onlyFirst()
{
    bool foundOk = false;
    const KScreen::OutputList outputs = mConfig->outputs();
    for (const KScreen::OutputPtr &output : outputs) {
        if( !output->isConnected() )
            continue;
        QPoint pos = output->pos();
        pos.setX(0);
        pos.setY(0);
        output->setPos(pos);
        output->setEnabled(!foundOk);
        foundOk = true;
    }
}

void FastMenu::onlySecond()
{
    bool foundOk = true;
    const KScreen::OutputList outputs = mConfig->outputs();
    for (const KScreen::OutputPtr &output : outputs) {
        if( !output->isConnected() )
            continue;
        QPoint pos = output->pos();
        pos.setX(0);
        pos.setY(0);
        output->setPos(pos);
        output->setEnabled(!foundOk);
        foundOk = false;
    }
}

void FastMenu::onSeleccionChanged(int index)
{
    switch((Options) index) {
    case Extended:
        KScreenUtils::extended(mConfig);
        break;
    case Unified:
        unified();
        break;
    case OnlyFirst:
        onlyFirst();
        break;
    case OnlySecond:
        onlySecond();
        break;
    case None:
        return;
        break;
    };

    if( KScreenUtils::applyConfig(mConfig, mOldConfig) )
        mOldConfig = mConfig->clone();
}
