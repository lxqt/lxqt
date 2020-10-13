/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   tree.c for ObConf, the configuration tool for Openbox
   Copyright (c) 2003-2007   Dana Jansens

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   See the COPYING file for a copy of the GNU General Public License.
*/

#include <QX11Info>
#include <QMessageBox>

#include "tree.h"
#include "obconf-qt.h"

#include <obt/xml.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

xmlNodePtr tree_get_node(const gchar *path, const gchar *def)
{
    xmlNodePtr n, c;
    gchar **nodes;
    gchar **it, **next;

    n = obt_xml_root(parse_i);

    nodes = g_strsplit(path, "/", 0);
    for (it = nodes; *it; it = next) {
        gchar **attrs;
        gboolean ok = FALSE;

        attrs = g_strsplit(*it, ":", 0);
        next = it + 1;

        /* match attributes */
        c = obt_xml_find_node(n->children, attrs[0]);
        while (c && !ok) {
            gint i;

            ok = TRUE;
            for (i = 1; attrs[i]; ++i) {
                gchar **eq = g_strsplit(attrs[i], "=", 2);
                if (eq[1] && !obt_xml_attr_contains(c, eq[0], eq[1]))
                    ok = FALSE;
                g_strfreev(eq);
            }
            if (!ok)
                c = obt_xml_find_node(c->next, attrs[0]);
        }

        if (!c) {
            gint i;

            c = xmlNewTextChild(n, NULL, (xmlChar*)attrs[0], (xmlChar*)(*next ? NULL : def));

            for (i = 1; attrs[i]; ++i) {
                gchar **eq = g_strsplit(attrs[i], "=", 2);
                if (eq[1])
                  xmlNewProp(c, (xmlChar*)eq[0], (xmlChar*)eq[1]);
                g_strfreev(eq);
            }
        }
        n = c;

        g_strfreev(attrs);
    }

    g_strfreev(nodes);

    return n;
}

void tree_delete_node(const gchar *path)
{
    xmlNodePtr n;

    n = tree_get_node(path, NULL);
    xmlUnlinkNode(n);
    xmlFreeNode(n);
}

void tree_apply()
{
    gchar *p, *d;
    gboolean err = FALSE;

    if (obc_config_file)
        p = g_strdup(obc_config_file);
    else
        p = g_build_filename(obt_paths_config_home(paths), "openbox",
                             "rc.xml", NULL);

    d = g_path_get_dirname(p);
    obt_paths_mkdir_path(d, 0700);
    g_free(d);

    if (!obt_xml_save_file(parse_i, p, TRUE)) {
        gchar *s;
        s = g_strdup_printf("An error occurred while saving the "
                            "config file '%s'", p);
        // obconf_error(s, FALSE);
        QMessageBox::critical(NULL, QString(), QString::fromUtf8(s));
        g_free(s);
    }
    g_free(p);

    if (!err) {
        XEvent ce;

        ce.xclient.type = ClientMessage;
        ce.xclient.display = QX11Info::display();
        ce.xclient.message_type = XInternAtom(ce.xclient.display, "_OB_CONTROL", false);
        Window root = QX11Info::appRootWindow();
        ce.xclient.window = root;
        ce.xclient.format = 32;
        ce.xclient.data.l[0] = 1; /* reconfigure */
        ce.xclient.data.l[1] = 0;
        ce.xclient.data.l[2] = 0;
        ce.xclient.data.l[3] = 0;
        ce.xclient.data.l[4] = 0;
        XSendEvent(ce.xclient.display, root, FALSE,
                   SubstructureNotifyMask | SubstructureRedirectMask,
                   &ce);
    }
}

void tree_set_string(const gchar *node, const gchar *value)
{
    xmlNodePtr n;

    n = tree_get_node(node, NULL);
    xmlNodeSetContent(n, (const xmlChar*) value);

    tree_apply();
}

void tree_set_int(const gchar *node, const gint value)
{
    xmlNodePtr n;
    gchar *s;

    n = tree_get_node(node, NULL);
    s = g_strdup_printf("%d", value);
    xmlNodeSetContent(n, (const xmlChar*) s);
    g_free(s);

    tree_apply();
}

void tree_set_bool(const gchar *node, const gboolean value)
{
    xmlNodePtr n;

    n = tree_get_node(node, NULL);
    xmlNodeSetContent(n, (const xmlChar*) (value ? "yes" : "no"));

    tree_apply();
}

gchar* tree_get_string(const gchar *node, const gchar *def)
{
    xmlNodePtr n;

    n = tree_get_node(node, def);
    return obt_xml_node_string(n);
}

gint tree_get_int(const gchar *node, gint def)
{
    xmlNodePtr n;
    gchar *d;

    d = g_strdup_printf("%d", def);
    n = tree_get_node(node, d);
    g_free(d);
    return obt_xml_node_int(n);
}

gboolean tree_get_bool(const gchar *node, gboolean def)
{
    xmlNodePtr n;

    n = tree_get_node(node, (def ? "yes" : "no"));
    return obt_xml_node_bool(n);
}
