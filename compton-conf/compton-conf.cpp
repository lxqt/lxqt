#include "compton-conf.h"
#include <QApplication>
#include <QLocale>
#include <QLibraryInfo>
#include <QTranslator>
#include <QCommandLineParser>
#include "maindialog.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Compton Conf"));
    const QString VERINFO = QStringLiteral(COMPTONCONF_VERSION
                                           "\nQt        " QT_VERSION_STR);
    app.setApplicationVersion(VERINFO);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    // load translations
    QTranslator qtTranslator, translator;
    // install the translations built-into Qt itself
    qtTranslator.load(QStringLiteral("qt_") + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);
    // install our own tranlations
    translator.load(QStringLiteral("compton-conf_") + QLocale::system().name(), QStringLiteral(COMPTON_CONF_DATA_DIR) + QStringLiteral("/translations"));
    app.installTranslator(&translator);

    MainDialog dlg;
    dlg.exec();
    return 0;
}
