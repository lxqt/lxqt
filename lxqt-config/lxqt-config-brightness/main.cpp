/*
    Copyright (C) 2016  P.L. Lucas <selairi@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "xrandrbrightness.h"
#include "brightnesswatcher.h"

#include <QDebug>
#include <QTimer>
#include <LXQt/SingleApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include "brightnesssettings.h"

#include <iostream>

enum CommandLineParseResult {
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

enum UiMode {
    GUI,
    TUI
};

struct BrightnessConfigData {
    BrightnessConfigData();

    UiMode mode;
    bool increaseBrightness;
    bool decreaseBrightness;
    bool setBrightness;
    float brightnessValue;
    bool resetGamma;
};

BrightnessConfigData::BrightnessConfigData()
    : mode(UiMode::GUI),
      increaseBrightness(false),
      decreaseBrightness(false),
      setBrightness(false),
      brightnessValue(0.0f),
      resetGamma(false)
{
}

static CommandLineParseResult parseCommandLine(QCommandLineParser *parser, BrightnessConfigData *config, QString *errorMessage)
{
    parser->setApplicationDescription(QStringLiteral("LXQt Config Brightness"));
    QCommandLineOption increaseOption(QStringList() << QStringLiteral("i") << QStringLiteral("increase"),
            QObject::tr("Increase brightness."));
    QCommandLineOption decreaseOption(QStringList() << QStringLiteral("d") << QStringLiteral("decrease"),
            QObject::tr("Decrease brightness."));
    QCommandLineOption setOption(QStringList() << QStringLiteral("s") << QStringLiteral("set"),
            QObject::tr("Set brightness from 1 to 100."), QStringLiteral("brightness"));
    QCommandLineOption resetGammaOption(QStringList() << QStringLiteral("r") << QStringLiteral("reset"),
            QObject::tr("Reset gamma to default value."));
    QCommandLineOption helpOption = parser->addHelpOption();
    QCommandLineOption versionOption = parser->addVersionOption();
    parser->addOption(increaseOption);
    parser->addOption(decreaseOption);
    parser->addOption(setOption);
    parser->addOption(resetGammaOption);

    const QStringList args = QCoreApplication::arguments();
    if (args.size() <= 1) { // no arguments given. GUI mode
        config->mode = UiMode::GUI;
        return CommandLineOk;
    } else {
        config->mode = UiMode::TUI;
    }

    if (!parser->parse(QCoreApplication::arguments())) {
        *errorMessage = parser->errorText();
        return CommandLineError;
    }

    if (parser->isSet(versionOption))
        return CommandLineVersionRequested;

    if (parser->isSet(helpOption))
        return CommandLineHelpRequested;

    const bool isIncreaseSet = parser->isSet(increaseOption);
    const bool isDecreaseSet = parser->isSet(decreaseOption);

    const bool isSetBrightnessSet = parser->isSet(setOption);
    float brightnessValue = 0.0f;
    if (isSetBrightnessSet)
        brightnessValue = parser->value(setOption).toFloat();

    const bool isResetGammaSet = parser->isSet(resetGammaOption);

    if ((isIncreaseSet || isDecreaseSet) && isSetBrightnessSet) {
        *errorMessage = QObject::tr("%1: Can't use increase/decrease and set in conjunction").arg(QCoreApplication::applicationName());
        return CommandLineError;
    }

    if (isIncreaseSet && isDecreaseSet) {
        *errorMessage = QObject::tr("%1: Can't use increase and decrease options in conjunction").arg(QCoreApplication::applicationName());
        return CommandLineError;
    }

    config->increaseBrightness = isIncreaseSet;
    config->decreaseBrightness = isDecreaseSet;
    config->setBrightness = isSetBrightnessSet;
    config->brightnessValue = brightnessValue;
    config->resetGamma = isResetGammaSet;

    return CommandLineOk;
}

int main(int argn, char* argv[])
{
    LXQt::SingleApplication app(argn, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    const QString VERINFO = QStringLiteral(LXQT_CONFIG_VERSION
                                           "\nliblxqt   " LXQT_VERSION
                                           "\nQt        " QT_VERSION_STR);
    app.setApplicationVersion(VERINFO);

    // Command line options
    QCommandLineParser parser;

    BrightnessConfigData config;
    QString errorMessage;

    switch(parseCommandLine(&parser, &config, &errorMessage)) {
    case CommandLineOk:
        break;
    case CommandLineError:
        std::cerr << qPrintable(errorMessage);
        std::cerr << "\n\n";
        std::cerr << qPrintable(parser.helpText());
        return EXIT_FAILURE;
    case CommandLineVersionRequested:
        parser.showVersion();
        Q_UNREACHABLE();
    case CommandLineHelpRequested:
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (config.mode == UiMode::GUI) {
        BrightnessSettings brightnessSettings;
        brightnessSettings.setWindowIcon(QIcon(QLatin1String(ICON_DIR) + QStringLiteral("/brightnesssettings.svg")));
        brightnessSettings.show();
        return app.exec();
    }

    // TUi mode
    float sign = (config.decreaseBrightness) ? -1.0f : 1.0f;
    float brightnessValue = qMin( qMax(config.brightnessValue, 0.0f), 100.0f ) / 100.0f;

    // Checks if backlight driver is available
    LXQt::Backlight *mBacklight = new LXQt::Backlight(&app);
    if (mBacklight->isBacklightAvailable() && !config.resetGamma) { // Use backlight driver
        BrightnessWatcher *brightnessWatcher = new BrightnessWatcher(&app);

        // Qt::QueuedConnection needed. The event loop isn't running, yet.
        QObject::connect(mBacklight, &LXQt::Backlight::backlightChanged,
            brightnessWatcher, &BrightnessWatcher::changed, Qt::QueuedConnection);

        if (config.setBrightness)
            sign = 0.0f;

        const int currentBacklight = mBacklight->getBacklight();
        const int maxBacklight = mBacklight->getMaxBacklight();
        int backlight = ( currentBacklight + sign*(maxBacklight/50 + 1) )*qAbs(sign) + brightnessValue*maxBacklight;

        mBacklight->setBacklight(backlight);

        // Timeout for situations where LXQtBacklight::setBacklight() doesn't
        // produce a changed signal.
        // QTimer::singleShot() doesn't allow to choose the type of connection.
        // Qt::QueuedConnection needed. The event loop isn't running, yet.
        QTimer *timeout = new QTimer(&app);
        QObject::connect(timeout, &QTimer::timeout, &app, LXQt::SingleApplication::quit, Qt::QueuedConnection);
        timeout->setSingleShot(true);
        timeout->setInterval(2000);
        timeout->start();
    } else { // Use XRandr driver
        XRandrBrightness *brightness = new XRandrBrightness();
        const QList<MonitorInfo> monitors = brightness->getMonitorsInfo();
        QList<MonitorInfo> monitorsChanged;
        for(MonitorInfo monitor : monitors)
        {
            if(config.resetGamma)
            {
                monitor.setBrightness(1.0f);
                monitorsChanged.append(monitor);
                continue;
            }

            if(monitor.isBacklightSupported() )
            {
                long backlight = ( monitor.backlight() + sign*(monitor.backlightMax()/50 + 1) )*qAbs(sign) + brightnessValue*monitor.backlightMax();
                if(backlight<monitor.backlightMax() && backlight>0)
                {
                    monitor.setBacklight(backlight);
                    monitorsChanged.append(monitor);
                }
            }
            else
            {
                float brightness = (monitor.brightness() + 0.1f *sign)*qAbs(sign) + brightnessValue * 2.0f;
                if(brightness < 2.0f && brightness > 0.0f)
                {
                    monitor.setBrightness(brightness);
                    monitorsChanged.append(monitor);
                }
            }
        }
        brightness->setMonitorsSettings(monitorsChanged);
        delete brightness;
        return 0;
    }

    // We need to start the event loop. Otherwise the signal/slot mechanism
    // won't work.
    return app.exec();
}
