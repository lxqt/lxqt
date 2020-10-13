/*
 *      fm-templates.h
 *
 *      Copyright 2012 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __FM_TEMPLATES_H__
#define __FM_TEMPLATES_H__

#include <glib.h>

#include "fm-mime-type.h"

G_BEGIN_DECLS

#define FM_TEMPLATE_TYPE               (fm_template_get_type())
#define FM_IS_TEMPLATE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj), FM_TEMPLATE_TYPE))

typedef struct _FmTemplate              FmTemplate;
typedef struct _FmTemplateClass         FmTemplateClass;

GType fm_template_get_type(void);

void _fm_templates_init(void);
void _fm_templates_finalize(void);

GList *fm_template_list_all(gboolean user_only);
const gchar *fm_template_get_name(FmTemplate *templ, gint *nlen);
FmMimeType *fm_template_get_mime_type(FmTemplate *templ);
FmIcon *fm_template_get_icon(FmTemplate *templ);
const gchar *fm_template_get_prompt(FmTemplate *templ);
const gchar *fm_template_get_label(FmTemplate *templ);
gboolean fm_template_is_directory(FmTemplate *templ);
gboolean fm_template_create_file(FmTemplate *templ, GFile *path, GError **error,
                                 gboolean run_default);

G_END_DECLS

#endif /* __FM_TEMPLATES_H__ */
