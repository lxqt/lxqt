/*
 *      fm-xml-file.h
 *
 *      Copyright 2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of libfm-extra package.
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

#ifndef __FM_XML_FILE_H__
#define __FM_XML_FILE_H__ 1

#include <glib-object.h>

G_BEGIN_DECLS

#define FM_XML_FILE_TYPE           (fm_xml_file_get_type())
#define FM_IS_XML_FILE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), FM_XML_FILE_TYPE))

typedef struct _FmXmlFile           FmXmlFile;
typedef struct _FmXmlFileClass      FmXmlFileClass;

GType fm_xml_file_get_type(void);

typedef struct _FmXmlFileItem       FmXmlFileItem;
typedef guint                       FmXmlFileTag;

/**
 * FM_XML_FILE_TAG_NOT_HANDLED:
 *
 * Value of FmXmlFileTag which means this element has no handler installed.
 */
#define FM_XML_FILE_TAG_NOT_HANDLED 0

/**
 * FM_XML_FILE_TEXT:
 *
 * Value of FmXmlFileTag which means this element has parsed character data.
 */
#define FM_XML_FILE_TEXT (FmXmlFileTag)-1

/**
 * FmXmlFileHandler
 * @item: XML element being parsed
 * @children: (element-type FmXmlFileItem): elements found in @item
 * @attribute_names: attributes names list for @item
 * @attribute_values: attributes values list for @item
 * @n_attributes: list length of @attribute_names and @attribute_values
 * @line: current line number in the file (starting from 1)
 * @pos: current pos number in the file (starting from 0)
 * @error: (allow-none) (out): location to save error
 * @user_data: data passed to fm_xml_file_parse_data()
 *
 * Callback for processing some element in XML file.
 * It will be called at closing tag.
 *
 * Returns: %TRUE if no errors were found by handler.
 *
 * Since: 1.2.0
 */
typedef gboolean (*FmXmlFileHandler)(FmXmlFileItem *item, GList *children,
                                     char * const *attribute_names,
                                     char * const *attribute_values,
                                     guint n_attributes, gint line, gint pos,
                                     GError **error, gpointer user_data);

/* setup */
FmXmlFile *fm_xml_file_new(FmXmlFile *sibling);
FmXmlFileTag fm_xml_file_set_handler(FmXmlFile *file, const char *tag,
                                     FmXmlFileHandler handler, gboolean in_line,
                                     GError **error);

/* parse */
gboolean fm_xml_file_parse_data(FmXmlFile *file, const char *text,
                                gsize size, GError **error, gpointer user_data);
GList *fm_xml_file_finish_parse(FmXmlFile *file, GError **error);
gint fm_xml_file_get_current_line(FmXmlFile *file, gint *pos);
const char *fm_xml_file_get_dtd(FmXmlFile *file);

/* item manipulations */
FmXmlFileItem *fm_xml_file_item_new(FmXmlFileTag tag);
void fm_xml_file_item_append_text(FmXmlFileItem *item, const char *text,
                                  gssize text_size, gboolean cdata);
gboolean fm_xml_file_item_append_child(FmXmlFileItem *item, FmXmlFileItem *child);
void fm_xml_file_item_set_comment(FmXmlFileItem *item, const char *comment);
gboolean fm_xml_file_item_set_attribute(FmXmlFileItem *item,
                                        const char *name, const char *value);
gboolean fm_xml_file_item_destroy(FmXmlFileItem *item);

gboolean fm_xml_file_insert_before(FmXmlFileItem *item, FmXmlFileItem *new_item);
gboolean fm_xml_file_insert_first(FmXmlFile *file, FmXmlFileItem *new_item);

/* save XML */
void fm_xml_file_set_dtd(FmXmlFile *file, const char *dtd, GError **error);
char *fm_xml_file_to_data(FmXmlFile *file, gsize *text_size, GError **error);

/* item data accessor functions */
const char *fm_xml_file_item_get_comment(FmXmlFileItem *item);
GList *fm_xml_file_item_get_children(FmXmlFileItem *item);
FmXmlFileItem *fm_xml_file_item_find_child(FmXmlFileItem *item, FmXmlFileTag tag);
FmXmlFileTag fm_xml_file_item_get_tag(FmXmlFileItem *item);
const char *fm_xml_file_item_get_data(FmXmlFileItem *item, gsize *text_size);
FmXmlFileItem *fm_xml_file_item_get_parent(FmXmlFileItem *item);
const char *fm_xml_file_item_get_tag_name(FmXmlFileItem *item);
const char *fm_xml_file_get_tag_name(FmXmlFile *file, FmXmlFileTag tag);

G_END_DECLS

#endif /* __FM_XML_FILE_H__ */
