#include "kscreenutils.h"
#include "timeoutdialog.h"

#include <KScreen/Output>
#include <KScreen/Config>
#include <KScreen/GetConfigOperation>
#include <KScreen/SetConfigOperation>
#include <KScreen/EDID>


void KScreenUtils::updateScreenSize(KScreen::ConfigPtr &config)
{
    const KScreen::OutputList outputs = config->outputs();
    int width, height;
    width = height = 0;
    for (const KScreen::OutputPtr &output : outputs) {
        if( !output->isConnected() )
            continue;
        QPoint pos = output->pos();
        if (output->currentMode()) {
            KScreen::ModePtr mode(output->currentMode());
            width = qMax(pos.x() + mode->size().width(), width);
            height = qMax(pos.y() + mode->size().height(), height);
        }
    }
    if (width != 0 && height != 0)
        config->screen()->setCurrentSize(QSize(width, height));
}


bool KScreenUtils::applyConfig(KScreen::ConfigPtr &config, KScreen::ConfigPtr &oldConfig)
{
    bool ok = false;
    KScreenUtils::updateScreenSize(config);
    if (config && KScreen::Config::canBeApplied(config)) {
        KScreen::SetConfigOperation(config).exec();

        TimeoutDialog mTimeoutDialog;
        if (mTimeoutDialog.exec() == QDialog::Rejected) {
            KScreenUtils::updateScreenSize(oldConfig);
            KScreen::SetConfigOperation(oldConfig).exec();
        }
        else
            ok = true;
    }

    return ok;
}


void KScreenUtils::extended(KScreen::ConfigPtr &config)
{
    int width = 0;
    const KScreen::OutputList outputs = config->outputs();
    for (const KScreen::OutputPtr &output : outputs) {
        if( !output->isConnected() )
            continue;
        qDebug() << "Output: " << output->name();
        QPoint pos = output->pos();
        pos.setX(width);
        pos.setY(0);
        output->setPos(pos);
        output->setEnabled(true);
        //first left one as primary
        output->setPrimary(width == 0);
        KScreen::ModePtr mode(output->currentMode());
        //if (!mode) 
        {
            // Set the biggest mode between preferred modes and the first mode.
            mode = output->modes().first();
            int modeArea = mode->size().width() * mode->size().height();
            const auto modes = output->modes();
            for(KScreen::ModePtr modeAux : modes) {
                if(output->preferredModes().contains(modeAux->id())) {
                    int modeAuxArea = modeAux->size().width() * modeAux->size().height();
                    if(modeArea < modeAuxArea) {
                        mode = modeAux;
                        modeArea = modeAuxArea;
                    }
                }
            }
            if (mode)
                output->setCurrentModeId(mode->id());
        }
        if (mode) 
            width += mode->size().width();
    }
}
