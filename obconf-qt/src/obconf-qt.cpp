/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-
 *
 * main.c for ObConf, the configuration tool for Openbox
 * Copyright (c) 2003-2008   Dana Jansens
 * Copyright (c) 2003        Tim Riley
 * Copyright (C) 2013        Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See the COPYING file for a copy of the GNU General Public License.
 */

#include <glib.h>

#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include <QX11Info>
#include <QMessageBox>
#include "maindialog.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <QPushButton>
#include "obconf-qt.h"
#include "archive.h"
#include "preview_update.h"
#include <stdlib.h>
#include <iostream>

using namespace Obconf;

xmlDocPtr doc;
xmlNodePtr root;
RrInstance* rrinst;
gchar* obc_config_file = NULL;
ObtPaths* paths;
ObtXmlInst* parse_i;

static gchar* obc_theme_install = NULL;
static gchar* obc_theme_archive = NULL;

void obconf_error(gchar* msg, gboolean modal) {
  // FIXME: we did not handle modal
  QMessageBox::critical(NULL, QObject::tr("ObConf Error"), QString::fromUtf8(msg));
}

static void print_version() {
  // g_print("ObConf %s\n", PACKAGE_VERSION);
  QString output = QObject::tr(
    "Copyright (c) 2003-2008   Dana Jansens\n"
    "Copyright (c) 2003        Tim Riley\n"
    "Copyright (c) 2007        Javeed Shaikh\n"
    "Copyright (c) 2013        Hong Jen Yee (PCMan)\n\n"
    "This program comes with ABSOLUTELY NO WARRANTY.\n"
    "This is free software, and you are welcome to redistribute it\n"
    "under certain conditions. See the file COPYING for details.\n\n"
  );
  std::cout << output.toUtf8().constData();

  exit(EXIT_SUCCESS);
}

static void print_help() {
  QString output = QObject::tr(
    "Syntax: obconf [options] [ARCHIVE.obt]\n"
    "\nOptions:\n"
    "  --help                Display this help and exit\n"
    "  --version             Display the version and exit\n"
    "  --install ARCHIVE.obt Install the given theme archive and select it\n"
    "  --archive THEME       Create a theme archive from the given theme directory\n"
    "  --config-file FILE    Specify the path to the config file to use\n"
  );
  std::cout << output.toUtf8().constData();

  exit(EXIT_SUCCESS);
}

static void parse_args(int argc, char** argv) {
  int i;
  for(i = 1; i < argc; ++i) {
    if(!strcmp(argv[i], "--help"))
      print_help();

    if(!strcmp(argv[i], "--version"))
      print_version();
    else if(!strcmp(argv[i], "--install")) {
      if(i == argc - 1)  /* no args left */
        std::cerr << QObject::tr("--install requires an argument\n").toUtf8().constData();
      else
        obc_theme_install = argv[++i];
    }
    else if(!strcmp(argv[i], "--archive")) {
      if(i == argc - 1)  /* no args left */
        std::cerr << QObject::tr("--archive requires an argument\n").toUtf8().constData();
      else
        obc_theme_archive = argv[++i];
    }
    else if(!strcmp(argv[i], "--config-file")) {
      if(i == argc - 1)  /* no args left */
        std::cerr << QObject::tr("--config-file requires an argument\n").toUtf8().constData();
      else
        obc_config_file = argv[++i];
    }
    else
      obc_theme_install = argv[i];
  }
}

static gboolean get_all(Window win, Atom prop, Atom type, gint size,
                        guchar** data, guint* num) {
  gboolean ret = FALSE;
  gint res;
  guchar* xdata = NULL;
  Atom ret_type;
  gint ret_size;
  gulong ret_items, bytes_left;

  res = XGetWindowProperty(QX11Info::display(), win, prop, 0l, G_MAXLONG,
                           FALSE, type, &ret_type, &ret_size,
                           &ret_items, &bytes_left, &xdata);

  if(res == Success) {
    if(ret_size == size && ret_items > 0) {
      guint i;
      *data = (guchar*)g_malloc(ret_items * (size / 8));

      for(i = 0; i < ret_items; ++i)
        switch(size) {
          case 8:
            (*data)[i] = xdata[i];
            break;
          case 16:
            ((guint16*)*data)[i] = ((gushort*)xdata)[i];
            break;
          case 32:
            ((guint32*)*data)[i] = ((gulong*)xdata)[i];
            break;
          default:
            g_assert_not_reached(); /* unhandled size */
        }

      *num = ret_items;
      ret = TRUE;
    }

    XFree(xdata);
  }

  return ret;
}

static gboolean prop_get_string_utf8(Window win, Atom prop, gchar** ret) {
  gchar* raw;
  gchar* str;
  guint num;

  if(get_all(win, prop, XInternAtom(QX11Info::display(), "UTF8_STRING", 0), 8, (guchar**)&raw, &num)) {
    str = g_strndup(raw, num); /* grab the first string from the list */
    g_free(raw);

    if(g_utf8_validate(str, -1, NULL)) {
      *ret = str;
      return TRUE;
    }

    g_free(str);
  }

  return FALSE;
}

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

  // load translations
  QTranslator qtTranslator, translator;

  // install the translations built-into Qt itself
  qtTranslator.load(QStringLiteral("qt_") + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  app.installTranslator(&qtTranslator);

  // install our own tranlations
  translator.load(QStringLiteral("obconf-qt_") + QLocale::system().name(), QStringLiteral(PACKAGE_DATA_DIR) + QStringLiteral("/translations"));
  app.installTranslator(&translator);

  // load configurations

  parse_args(argc, argv);

  if(obc_theme_archive) {
    archive_create(obc_theme_archive);
    return 0;
  }

  paths = obt_paths_new();
  parse_i = obt_xml_instance_new();
  int screen = QX11Info::appScreen();
  rrinst = RrInstanceNew(QX11Info::display(), screen);
  if(!obc_config_file) {
    gchar* p;
    if(prop_get_string_utf8(QX11Info::appRootWindow(screen),
                            XInternAtom(QX11Info::display(), "_OB_CONFIG_FILE", 0), &p)) {
      obc_config_file = g_filename_from_utf8(p, -1, NULL, NULL, NULL);
      g_free(p);
    }
  }
  xmlIndentTreeOutput = 1;

  if(!((obc_config_file &&
        obt_xml_load_file(parse_i, obc_config_file, "openbox_config")) ||
       obt_xml_load_config_file(parse_i, "openbox", "rc.xml",
                                "openbox_config"))) {
    QMessageBox::critical(NULL, QObject::tr("Error"),
                          QObject::tr("Failed to load an rc.xml. You have probably failed to install Openbox properly."));
  }
  else {
    doc = obt_xml_doc(parse_i);
    root = obt_xml_root(parse_i);
  }

  /* look for parsing errors */
  {
    xmlErrorPtr e = xmlGetLastError();

    if(e) {
      QString message = QObject::tr("Error while parsing the Openbox configuration file.  Your configuration file is not valid XML.\n\nMessage: %1")
        .arg(QString::fromUtf8(e->message));
      QMessageBox::critical(NULL, QObject::tr("Error"), message);
    }
  }

  // build our GUI
  MainDialog dlg;
  if(obc_theme_install)
    dlg.theme_install(obc_theme_install);
  else
    dlg.theme_load_all();
  dlg.exec();

  /*
  preview_update_set_active_font(NULL);
  preview_update_set_inactive_font(NULL);
  preview_update_set_menu_header_font(NULL);
  preview_update_set_menu_item_font(NULL);
  preview_update_set_osd_active_font(NULL);
  preview_update_set_osd_inactive_font(NULL);
  preview_update_set_title_layout(NULL);
  */

  RrInstanceFree(rrinst);
  obt_xml_instance_unref(parse_i);
  obt_paths_unref(paths);
  xmlFreeDoc(doc);

  return 0;
}
