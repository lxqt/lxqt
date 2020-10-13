/* coded by Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 * (c)DWTFYW
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */
//#include <QtCore>
#include <QDebug>

#include <XdgIcon>
#include <LXQt/Settings>
#include "main.h"
#include "lxqttranslate.h"

#include <LXQt/Application>
#include <QFile>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QTextCodec>
#include <QTextStream>


///////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[])
{
    //QTextCodec::setCodecForCStrings(QTextCodec::codecForName("koi8-r"));
    //QTextCodec::setCodecForLocale(QTextCodec::codecForName("koi8-r"));

    LXQt::Application app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    TRANSLATE_APP;

    //qDebug() << findDefaultTheme() << getCurrentTheme();

    SelectWnd *sw = new SelectWnd;
    sw->show();
    sw->setCurrent();
    return app.exec();
}
