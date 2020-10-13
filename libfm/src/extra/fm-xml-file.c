/*
 *      fm-xml-file.c
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

/**
 * SECTION:fm-xml-file
 * @short_description: Simple XML parser.
 * @title: FmXmlFile
 *
 * @include: libfm/fm-extra.h
 *
 * The FmXmlFile represents content of some XML file in form that can
 * be altered and saved later.
 *
 * This parser has some simplifications on XML parsing:
 * * Only UTF-8 encoding is supported
 * * No user-defined entities, those should be converted externally
 * * Processing instructions, comments and the doctype declaration are parsed but are not interpreted in any way
 * The markup format does support:
 * * Elements
 * * Attributes
 * * 5 standard entities: &amp;amp; &amp;lt; &amp;gt; &amp;quot; &amp;apos;
 * * Character references
 * * Sections marked as CDATA
 *
 * The application should respect g_type_init() if this parser is used
 * without usage of libfm.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-xml-file.h"

#include <glib/gi18n-lib.h>
#include "glib-compat.h"

#include <stdlib.h>
#include <errno.h>

typedef struct
{
    gchar *name;
    FmXmlFileHandler handler;
    gboolean in_line : 1;
} FmXmlFileTagDesc;

struct _FmXmlFile
{
    GObject parent;
    GList *items;
    GString *data;
    char *comment_pre;
    FmXmlFileItem *current_item;
    FmXmlFileTagDesc *tags; /* tags[0].name contains DTD */
    guint n_tags; /* number of elements in tags */
    guint line, pos;
};

struct _FmXmlFileClass
{
    GObjectClass parent_class;
};

struct _FmXmlFileItem
{
    FmXmlFileTag tag;
    union {
        gchar *tag_name; /* only for tag == FM_XML_FILE_TAG_NOT_HANDLED */
        gchar *text; /* only for tag == FM_XML_FILE_TEXT, NULL if directive */
    };
    char **attribute_names;
    char **attribute_values;
    FmXmlFile *file;
    FmXmlFileItem *parent;
    GList **parent_list; /* points to file->items or to parent->children */
    GList *children;
    gchar *comment; /* a little trick: it is equal to text if it is CDATA */
};


G_DEFINE_TYPE(FmXmlFile, fm_xml_file, G_TYPE_OBJECT);

static void fm_xml_file_finalize(GObject *object)
{
    FmXmlFile *self;
    guint i;

    g_return_if_fail(object != NULL);
    g_return_if_fail(FM_IS_XML_FILE(object));

    self = (FmXmlFile*)object;
    self->current_item = NULL; /* we destroying it */
    while (self->items)
    {
        g_assert(((FmXmlFileItem*)self->items->data)->file == self);
        g_assert(((FmXmlFileItem*)self->items->data)->parent == NULL);
        fm_xml_file_item_destroy(self->items->data);
    }
    for (i = 0; i < self->n_tags; i++)
        g_free(self->tags[i].name);
    g_free(self->tags);
    if (self->data)
        g_string_free(self->data, TRUE);
    g_free(self->comment_pre);

    G_OBJECT_CLASS(fm_xml_file_parent_class)->finalize(object);
}

static void fm_xml_file_class_init(FmXmlFileClass *klass)
{
    GObjectClass *g_object_class;

    g_object_class = G_OBJECT_CLASS(klass);
    g_object_class->finalize = fm_xml_file_finalize;
}

static void fm_xml_file_init(FmXmlFile *self)
{
    self->tags = g_new0(FmXmlFileTagDesc, 1);
    self->n_tags = 1;
    self->line = 1;
}

/**
 * fm_xml_file_new
 * @sibling: (allow-none): container to copy handlers data
 *
 * Creates new empty #FmXmlFile container. If @sibling is not %NULL
 * then new container will have callbacks identical to set in @sibling.
 * Use @sibling parameter if you need to work with few XML files that
 * share the same schema or if you need to use the same tag ids for more
 * than one file.
 *
 * Returns: (transfer full): newly created object.
 *
 * Since: 1.2.0
 */
FmXmlFile *fm_xml_file_new(FmXmlFile *sibling)
{
    FmXmlFile *self;
    FmXmlFileTag i;

    self = (FmXmlFile*)g_object_new(FM_XML_FILE_TYPE, NULL);
    if (sibling && sibling->n_tags > 1)
    {
        self->n_tags = sibling->n_tags;
        self->tags = g_renew(FmXmlFileTagDesc, self->tags, self->n_tags);
        for (i = 1; i < self->n_tags; i++)
        {
            self->tags[i].name = g_strdup(sibling->tags[i].name);
            self->tags[i].handler = sibling->tags[i].handler;
        }
    }
    return self;
}

/**
 * fm_xml_file_set_handler
 * @file: the parser container
 * @tag: tag to use @handler for
 * @handler: callback for @tag
 * @in_line: %TRUE if tag should not receive new line by fm_xml_file_to_data()
 * @error: (allow-none) (out): location to save error
 *
 * Sets @handler for @file to be called on parse when @tag is found
 * in XML data. This function will fail if some handler for @tag was
 * aready set, in this case @error will be set appropriately.
 *
 * Returns: id for the @tag.
 *
 * Since: 1.2.0
 */
FmXmlFileTag fm_xml_file_set_handler(FmXmlFile *file, const char *tag,
                                     FmXmlFileHandler handler, gboolean in_line,
                                     GError **error)
{
    FmXmlFileTag i;

    g_return_val_if_fail(file != NULL && FM_IS_XML_FILE(file), FM_XML_FILE_TAG_NOT_HANDLED);
    g_return_val_if_fail(handler != NULL, FM_XML_FILE_TAG_NOT_HANDLED);
    g_return_val_if_fail(tag != NULL, FM_XML_FILE_TAG_NOT_HANDLED);
    for (i = 1; i < file->n_tags; i++)
        if (strcmp(file->tags[i].name, tag) == 0)
        {
            g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                        _("Duplicate handler for tag <%s>"), tag);
            return i;
        }
    file->tags = g_renew(FmXmlFileTagDesc, file->tags, i + 1);
    file->tags[i].name = g_strdup(tag);
    file->tags[i].handler = handler;
    file->tags[i].in_line = in_line;
    file->n_tags = i + 1;
    /* g_debug("XML parser: added handler '%s' id %u", tag, (guint)i); */
    return i;
}


/* parse */

/*
 * This function was taken from GLib sources and adapted to be used here.
 * Copyright 2000, 2003 Red Hat, Inc.
 *
 * re-write the GString in-place, unescaping anything that escaped.
 * most XML does not contain entities, or escaping.
 */
static gboolean
unescape_gstring_inplace (//GMarkupParseContext  *context,
                          GString              *string,
                          //gboolean             *is_ascii,
                          guint                *line_num,
                          guint                *pos,
                          gboolean              normalize_attribute,
                          GError              **error)
{
  //char mask, *to;
  char *to;
  //int line_num = 1;
  const char *from, *sol;

  //*is_ascii = FALSE;

  /*
   * Meeks' theorum: unescaping can only shrink text.
   * for &lt; etc. this is obvious, for &#xffff; more
   * thought is required, but this is patently so.
   */
  //mask = 0;
  for (from = to = string->str; *from != '\0'; from++, to++)
    {
      *to = *from;

      //mask |= *to;
      if (*to == '\n')
      {
        (*line_num)++;
        *pos = 0;
      }
      if (normalize_attribute && (*to == '\t' || *to == '\n'))
        *to = ' ';
      if (*to == '\r')
        {
          *to = normalize_attribute ? ' ' : '\n';
          if (from[1] == '\n')
          {
            from++;
            (*line_num)++;
            *pos = 0;
          }
        }
      sol = from;
      if (*from == '&')
        {
          from++;
          if (*from == '#')
            {
              gboolean is_hex = FALSE;
              gulong l;
              gchar *end = NULL;

              from++;

              if (*from == 'x')
                {
                  is_hex = TRUE;
                  from++;
                }

              /* digit is between start and p */
              errno = 0;
              if (is_hex)
                l = strtoul (from, &end, 16);
              else
                l = strtoul (from, &end, 10);

              if (end == from || errno != 0)
                {
                  g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                              _("Failed to parse '%-.*s', which "
                                "should have been a digit "
                                "inside a character reference "
                                "(&#234; for example) - perhaps "
                                "the digit is too large"),
                              (int)(end - from), from);
                  return FALSE;
                }
              else if (*end != ';')
                {
                  g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                              _("Character reference did not end with a "
                                "semicolon; "
                                "most likely you used an ampersand "
                                "character without intending to start "
                                "an entity - escape ampersand as &amp;"));
                  return FALSE;
                }
              else
                {
                  /* characters XML 1.1 permits */
                  if ((0 < l && l <= 0xD7FF) ||
                      (0xE000 <= l && l <= 0xFFFD) ||
                      (0x10000 <= l && l <= 0x10FFFF))
                    {
                      gchar buf[8];
                      memset (buf, 0, 8);
                      g_unichar_to_utf8 (l, buf);
                      strncpy (to, buf, 8);
                      to += strlen (buf) - 1;
                      from = end;
                      //if (l >= 0x80) /* not ascii */
                        //mask |= 0x80;
                    }
                  else
                    {
                      g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                                  _("Character reference '%-.*s' does not "
                                    "encode a permitted character"),
                                  (int)(end - from), from);
                      return FALSE;
                    }
                }
            }

          else if (strncmp (from, "lt;", 3) == 0)
            {
              *to = '<';
              from += 2;
            }
          else if (strncmp (from, "gt;", 3) == 0)
            {
              *to = '>';
              from += 2;
            }
          else if (strncmp (from, "amp;", 4) == 0)
            {
              *to = '&';
              from += 3;
            }
          else if (strncmp (from, "quot;", 5) == 0)
            {
              *to = '"';
              from += 4;
            }
          else if (strncmp (from, "apos;", 5) == 0)
            {
              *to = '\'';
              from += 4;
            }
          else
            {
              if (*from == ';')
                g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                            _("Empty entity '&;' seen; valid "
                              "entities are: &amp; &quot; &lt; &gt; &apos;"));
              else
                {
                  const char *end = strchr (from, ';');
                  if (end)
                    g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                                _("Entity name '%-.*s' is not known"),
                                (int)(end-from), from);
                  else
                    g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                                _("Entity did not end with a semicolon; "
                                  "most likely you used an ampersand "
                                  "character without intending to start "
                                  "an entity - escape ampersand as &amp;"));
                }
              return FALSE;
            }
        }
      *pos += (from - sol) + 1;
    }

  /* g_debug("unescape_gstring_inplace completed"); */
  g_assert (to - string->str <= (gint)string->len);
  if (to - string->str != (gint)string->len)
    g_string_truncate (string, to - string->str);

  //*is_ascii = !(mask & 0x80);

  return TRUE;
}


static inline void _update_file_ptr_part(FmXmlFile *file, const char *start,
                                         const char *end)
{
    while (start < end)
    {
        if (*start == '\n')
        {
            file->line++;
            file->pos = 0;
        }
        else
            /* FIXME: advance by chars not bytes? */
            file->pos++;
        start++;
    }
}

static inline void _update_file_ptr(FmXmlFile *file, int add_cols)
{
    guint i;
    char *p;

    for (i = file->data->len, p = file->data->str; i > 0; i--, p++)
    {
        if (*p == '\n')
        {
            file->line++;
            file->pos = 0;
        }
        else
            /* FIXME: advance by chars not bytes? */
            file->pos++;
    }
    file->pos += add_cols;
}

static inline gboolean _is_space(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

/**
 * fm_xml_file_parse_data
 * @file: the parser container
 * @text: data to parse
 * @size: size of @text
 * @error: (allow-none) (out): location to save error
 * @user_data: data to pass to handlers
 *
 * Parses next chunk of @text data. Parsing stops at end of data or at any
 * error. In latter case @error will be set appropriately.
 *
 * See also: fm_xml_file_finish_parse().
 *
 * Returns: %FALSE if parsing failed.
 *
 * Since: 1.2.0
 */
gboolean fm_xml_file_parse_data(FmXmlFile *file, const char *text,
                                gsize size, GError **error, gpointer user_data)
{
    gsize ptr, len;
    char *dst, *end, *tag, *name, *value;
    GString *buff;
    FmXmlFileItem *item;
    gboolean closing, selfdo;
    FmXmlFileTag i;
    char **attrib_names, **attrib_values;
    guint attribs;
    char quote;

    g_return_val_if_fail(file != NULL && FM_IS_XML_FILE(file), FALSE);
_restart:
    if (size == 0)
        return TRUE;
    /* if file->data has '<' as first char then we stopped at tag */
    if (file->data && file->data->len && file->data->str[0] == '<')
    {
        for (ptr = 0; ptr < size; ptr++)
            if (text[ptr] == '>')
                break;
        if (ptr == size) /* still no end of that tag */
        {
            g_string_append_len(file->data, text, size);
            return TRUE;
        }
        /* we got a complete tag, nice, let parse it */
        g_string_append_len(file->data, text, ptr);
        ptr++;
        text += ptr;
        size -= ptr;
        /* check for CDATA first */
        if (file->data->len >= 11 /* <![CDATA[]] */ &&
            strncmp(file->data->str, "<![CDATA[", 9) == 0)
        {
            end = file->data->str + file->data->len;
            if (end[-2] != ']' || end[-1] != ']') /* find end of CDATA */
            {
                g_string_append_c(file->data, '>');
                goto _restart;
            }
            if (file->current_item == NULL) /* CDATA at top level! */
                g_warning("FmXmlFile: line %u: junk CDATA in XML file ignored",
                          file->line);
            else
            {
                item = fm_xml_file_item_new(FM_XML_FILE_TEXT);
                item->text = item->comment = g_strndup(&file->data->str[9],
                                                       file->data->len - 11);
                fm_xml_file_item_append_child(file->current_item, item);
            }
            _update_file_ptr(file, 1);
            g_string_truncate(file->data, 0);
            goto _restart;
        }
        /* check for comment */
        if (file->data->len >= 7 /* <!-- -- */ &&
            strncmp(file->data->str, "<!--", 4) == 0)
        {
            end = file->data->str + file->data->len;
            if (end[-2] != '-' || end[-1] != '-') /* find end of comment */
            {
                g_string_append_c(file->data, '>');
                goto _restart;
            }
            g_free(file->comment_pre);
            /* FIXME: not ignore duplicate comments */
            if (_is_space(end[-3]))
                file->comment_pre = g_strndup(&file->data->str[5],
                                              file->data->len - 8);
            else /* FIXME: check: XML spec says it should be not '-' */
                file->comment_pre = g_strndup(&file->data->str[5],
                                              file->data->len - 7);
            _update_file_ptr(file, 1);
            g_string_truncate(file->data, 0);
            goto _restart;
        }
        /* check for DTD - it may be only at top level */
        if (file->current_item == NULL && file->data->len >= 10 &&
            strncmp(file->data->str, "<!DOCTYPE", 9) == 0 &&
            _is_space(file->data->str[9]))
        {
            /* FIXME: can DTD contain any tags? count '<' and '>' pairs */
            if (file->tags[0].name) /* duplicate DTD! */
                g_warning("FmXmlFile: line %u: duplicate DTD, ignored",
                          file->line);
            else
                file->tags[0].name = g_strndup(&file->data->str[10],
                                               file->data->len - 10);
            _update_file_ptr(file, 1);
            g_string_truncate(file->data, 0);
            goto _restart;
        }
        /* support directives such as <?xml ..... ?> */
        if (file->data->len >= 4 /* <?x? */ &&
            file->data->str[1] == '?' &&
            file->data->str[file->data->len-1] == '?')
        {
            item = fm_xml_file_item_new(FM_XML_FILE_TEXT);
            item->comment = g_strndup(&file->data->str[2], file->data->len - 3);
            if (file->current_item != NULL)
                fm_xml_file_item_append_child(file->current_item, item);
            else
            {
                item->file = file;
                item->parent_list = &file->items;
                file->items = g_list_append(file->items, item);
            }
            _update_file_ptr(file, 1);
            g_string_truncate(file->data, 0);
            goto _restart;
        }
        closing = (file->data->str[1] == '/');
        end = file->data->str + file->data->len;
        selfdo = (!closing && end[-1] == '/');
        if (selfdo)
            end--;
        tag = closing ? &file->data->str[2] : &file->data->str[1];
        for (dst = tag; dst < end; dst++)
            if (_is_space(*dst))
                break;
        _update_file_ptr_part(file, file->data->str, dst + 1);
        *dst = '\0'; /* terminate the tag */
        if (closing)
        {
            if (dst != end) /* we got a space char in closing tag */
            {
                g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                                    _("Space isn't allowed in the close tag"));
                return FALSE;
            }
            /* g_debug("XML parser: found closing tag '%s' for %p at %d:%d", tag,
                    file->current_item, file->line, file->pos); */
            item = file->current_item;
            if (item == NULL) /* no tag to close */
            {
                g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                            _("Element '%s' was closed but no element was opened"),
                            tag);
                return FALSE;
            }
            else
            {
                char *tagname;

                if (item->tag == FM_XML_FILE_TAG_NOT_HANDLED)
                    tagname = item->tag_name;
                else
                    tagname = file->tags[item->tag].name;
                if (strcmp(tag, tagname)) /* closing tag doesn't match */
                {
                    /* FIXME: validate tag so be more verbose on error */
                    g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                                _("Element '%s' was closed but the currently "
                                  "open element is '%s'"), tag, tagname);
                    return FALSE;
                }
                file->current_item = item->parent;
_close_the_tag:
                /* g_debug("XML parser: close the tag '%s'", tag); */
                g_string_truncate(file->data, 0);
                if (item->tag != FM_XML_FILE_TAG_NOT_HANDLED)
                {
                    if (!file->tags[item->tag].handler(item, item->children,
                                                       item->attribute_names,
                                                       item->attribute_values,
                                                       item->attribute_names ? g_strv_length(item->attribute_names) : 0,
                                                       file->line,
                                                       file->pos,
                                                       error, user_data))
                        return FALSE;
                }
                file->pos++; /* '>' */
                goto _restart;
            }
        }
        else /* opening tag */
        {
            /* g_debug("XML parser: found opening tag '%s'", tag); */
            /* parse and check tag name */
            for (i = 1; i < file->n_tags; i++)
                if (strcmp(file->tags[i].name, tag) == 0)
                    break;
            if (i == file->n_tags)
                /* FIXME: do name validation */
                i = FM_XML_FILE_TAG_NOT_HANDLED;
            /* parse and check attributes */
            attribs = 0;
            attrib_names = attrib_values = NULL;
            while (dst < end)
            {
                name = &dst[1]; /* skip this space */
                while (name < end && _is_space(*name))
                    name++;
                value = name;
                while (value < end && !_is_space(*value) && *value != '=')
                    value++;
                len = value - name;
                _update_file_ptr_part(file, dst, value);
                /* FIXME: skip spaces before =? */
                if (value + 3 <= end && *value == '=') /* minimum is ="" */
                {
                    value++;
                    file->pos++; /* '=' */
                    /* FIXME: skip spaces after =? */
                    quote = *value++;
                    if (quote != '\'' && quote != '"')
                    {
                        g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                                    _("Invalid char '%c' at start of attribute value"),
                                    quote);
                        goto _attr_error;
                    }
                    file->pos++; /* quote char */
                    for (ptr = 0; &value[ptr] < end; ptr++)
                        if (value[ptr] == quote)
                            break;
                    if (&value[ptr] == end)
                    {
                        g_set_error(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                                    _("Invalid char '%c' at end of attribute value,"
                                      " expected '%c'"), value[ptr-1], quote);
                        goto _attr_error;
                    }
                    buff = g_string_new_len(value, ptr);
                    if (!unescape_gstring_inplace(buff, &file->line,
                                                  &file->pos, TRUE, error))
                    {
                        g_string_free(buff, TRUE);
_attr_error:
                        for (i = 0; i < attribs; i++)
                        {
                            g_free(attrib_names[i]);
                            g_free(attrib_values[i]);
                        }
                        g_free(attrib_names);
                        g_free(attrib_values);
                        return FALSE;
                    }
                    dst = &value[ptr+1];
                    value = g_string_free(buff, FALSE);
                    file->pos++; /* end quote char */
                }
                else
                {
                    dst = value;
                    value = NULL;
                    /* FIXME: isn't it error? */
                }
                attrib_names = g_renew(char *, attrib_names, attribs + 2);
                attrib_values = g_renew(char *, attrib_values, attribs + 2);
                attrib_names[attribs] = g_strndup(name, len);
                attrib_values[attribs] = value;
                attribs++;
            }
            if (attribs > 0)
            {
                attrib_names[attribs] = NULL;
                attrib_values[attribs] = NULL;
            }
            /* create new item */
            item = fm_xml_file_item_new(i);
            item->attribute_names = attrib_names;
            item->attribute_values = attrib_values;
            if (i == FM_XML_FILE_TAG_NOT_HANDLED)
                item->tag_name = g_strdup(tag);
            /* insert new item into the container */
            item->comment = file->comment_pre;
            file->comment_pre = NULL;
            if (file->current_item)
                fm_xml_file_item_append_child(file->current_item, item);
            else
            {
                item->file = file;
                item->parent_list = &file->items;
                file->items = g_list_append(file->items, item);
            }
            file->pos++; /* '>' or '/' */
            if (selfdo) /* simple self-closing tag */
                goto _close_the_tag;
            file->current_item = item;
            g_string_truncate(file->data, 0);
            goto _restart;
        }
    }
    /* otherwise we stopped at some data somewhere */
    else
    {
        if (!file->data || file->data->len == 0) while (size > 0)
        {
            /* skip leading spaces */
            if (*text == '\n')
            {
                file->line++;
                file->pos = 0;
            }
            else if (*text == ' ' || *text == '\t' || *text == '\r')
                file->pos++;
            else
                break;
            text++;
            size--;
        }
        for (ptr = 0; ptr < size; ptr++)
            if (text[ptr] == '<')
                break;
        if (file->data == NULL)
            file->data = g_string_new_len(text, ptr);
        else if (ptr > 0)
            g_string_append_len(file->data, text, ptr);
        if (ptr == size) /* still no end of text */
            return TRUE;
        /* if(ptr>0) g_debug("XML parser: got text '%s'", file->data->str); */
        if (ptr == 0) ; /* no text */
        else if (file->current_item == NULL) /* text at top level! */
        {
            g_warning("FmXmlFile: line %u: junk data in XML file ignored",
                      file->line);
            _update_file_ptr(file, 0);
        }
        else if (unescape_gstring_inplace(file->data, &file->line,
                                          &file->pos, FALSE, error))
        {
            item = fm_xml_file_item_new(FM_XML_FILE_TEXT);
            item->text = g_strndup(file->data->str, file->data->len);
            item->comment = file->comment_pre;
            file->comment_pre = NULL;
            fm_xml_file_item_append_child(file->current_item, item);
            /* FIXME: truncate ending spaces from item->text */
        }
        else
            return FALSE;
        ptr++;
        text += ptr;
        size -= ptr;
        g_string_assign(file->data, "<");
        goto _restart;
    }
    /* error if reached */
}

/**
 * fm_xml_file_finish_parse
 * @file: the parser container
 * @error: (allow-none) (out): location to save error
 *
 * Ends parsing of data and retrieves final status. If XML was invalid
 * then returns %NULL and sets @error appropriately.
 * This function can be called more than once.
 *
 * See also: fm_xml_file_parse_data().
 * See also: fm_xml_file_item_get_children().
 *
 * Returns: (transfer container) (element-type FmXmlFileItem): contents of XML
 *
 * Since: 1.2.0
 */
GList *fm_xml_file_finish_parse(FmXmlFile *file, GError **error)
{
    g_return_val_if_fail(file != NULL && FM_IS_XML_FILE(file), NULL);
    if (file->current_item)
    {
        if (file->current_item->tag == FM_XML_FILE_TEXT &&
            file->current_item->parent == NULL)
            g_warning("FmXmlFile: junk at end of XML");
        else
        {
            g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                                _("Document ended unexpectedly"));
            /* FIXME: analize content of file->data to be more verbose */
            return NULL;
        }
    }
    else if (file->items == NULL)
    {
        g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_EMPTY,
                            _("Document was empty or contained only whitespace"));
        return NULL;
    }
    /* FIXME: check if file->comment_pre is NULL */
    return g_list_copy(file->items);
}

/**
 * fm_xml_file_get_current_line
 * @file: the parser container
 * @pos: (allow-none) (out): location to save line position
 *
 * Retrieves the line where parser has stopped.
 *
 * Returns: line num (starting from 1).
 *
 * Since: 1.2.0
 */
gint fm_xml_file_get_current_line(FmXmlFile *file, gint *pos)
{
    if (file == NULL || !FM_IS_XML_FILE(file))
        return 0;
    if (pos)
        *pos = file->pos;
    return file->line;
}

/**
 * fm_xml_file_get_dtd
 * @file: the parser container
 *
 * Retrieves DTD description for XML data in the container. Returned data
 * are owned by @file and should not be modified by caller.
 *
 * Returns: (transfer none): DTD description.
 *
 * Since: 1.2.0
 */
const char *fm_xml_file_get_dtd(FmXmlFile *file)
{
    if(file == NULL)
        return NULL;
    return file->tags[0].name;
}


/* item manipulations */

/**
 * fm_xml_file_item_new
 * @tag: tag id for new item
 *
 * Creates new unattached XML item.
 *
 * Returns: (transfer full): newly allocated #FmXmlFileItem.
 *
 * Since: 1.2.0
 */
FmXmlFileItem *fm_xml_file_item_new(FmXmlFileTag tag)
{
    FmXmlFileItem *item = g_slice_new0(FmXmlFileItem);

    item->tag = tag;
    return item;
}

/**
 * fm_xml_file_item_append_text
 * @item: item to append text
 * @text: text to append
 * @text_size: length of text in bytes, or -1 if the text is nul-terminated
 * @cdata: %TRUE if @text should be saved as CDATA array
 *
 * Appends @text after last element contained in @item.
 *
 * Since: 1.2.0
 */
void fm_xml_file_item_append_text(FmXmlFileItem *item, const char *text,
                                  gssize text_size, gboolean cdata)
{
    FmXmlFileItem *text_item;

    g_return_if_fail(item != NULL);
    if (text == NULL || text_size == 0)
        return;
    text_item = fm_xml_file_item_new(FM_XML_FILE_TEXT);
    if (text_size > 0)
        text_item->text = g_strndup(text, text_size);
    else
        text_item->text = g_strdup(text);
    if (cdata)
        text_item->comment = text_item->text;
    fm_xml_file_item_append_child(item, text_item);
}

static inline gboolean _xml_item_is_busy(FmXmlFileItem *item)
{
    register FmXmlFileItem *test;

    if (item->file)
        for (test = item->file->current_item; test; test = test->parent)
            if (test == item)
            {
                /* g_debug("*** item %p is busy", item); */
                return TRUE;
            }
    return FALSE;
}

static void _reassign_xml_file(FmXmlFileItem *item, FmXmlFile *file)
{
    GList *chl;

    /* do it recursively */
    for (chl = item->children; chl; chl = chl->next)
        _reassign_xml_file(chl->data, file);
    item->file = file;
}

/**
 * fm_xml_file_item_append_child
 * @item: item to append child
 * @child: the child item to append
 *
 * Appends @child after last element contained in @item. If the @child
 * already was in the XML structure then it will be moved to the new
 * place instead.
 * Behavior after moving between different containers is undefined.
 *
 * Returns: %FALSE if @child is busy thus cannot be moved.
 *
 * Since: 1.2.0
 */
gboolean fm_xml_file_item_append_child(FmXmlFileItem *item, FmXmlFileItem *child)
{
    g_return_val_if_fail(item != NULL && child != NULL, FALSE);
    if (_xml_item_is_busy(child))
        return FALSE; /* cannot move item right now */
    if (child->parent_list) /* remove from old list */
    {
        /* g_debug("moving item %p(%d) from parser %p into %p as child of %p", child, (int)child->tag, child->file, item->file, item); */
        g_assert(child->file != NULL && g_list_find(*child->parent_list, child) != NULL);
        *child->parent_list = g_list_remove(*child->parent_list, child);
    }
    /* else
        g_debug("adding item %p(%d) into parser %p as child of %p", child, (int)child->tag, item->file, item); */
    item->children = g_list_append(item->children, child);
    child->parent_list = &item->children;
    child->parent = item;
    if (child->file != item->file)
        _reassign_xml_file(child, item->file);
    return TRUE;
}

/**
 * fm_xml_file_item_set_comment
 * @item: element to set
 * @comment: (allow-none): new comment
 *
 * Changes comment that is prepended to @item.
 *
 * Since: 1.2.0
 */
void fm_xml_file_item_set_comment(FmXmlFileItem *item, const char *comment)
{
    g_return_if_fail(item != NULL);
    g_free(item->comment);
    item->comment = g_strdup(comment);
}

/**
 * fm_xml_file_item_set_attribute
 * @item: element to update
 * @name: attribute name
 * @value: (allow-none): attribute data
 *
 * Changes data for the attribute of some @item with new @value. If such
 * attribute wasn't set then adds it for the @item. If @value is %NULL
 * then the attribute will be unset from the @item.
 *
 * Returns: %TRUE if attribute was set successfully.
 *
 * Since: 1.2.0
 */
gboolean fm_xml_file_item_set_attribute(FmXmlFileItem *item,
                                        const char *name, const char *value)
{
    int i, n_attr;

    g_return_val_if_fail(item != NULL, FALSE);
    g_return_val_if_fail(name != NULL, FALSE);
    if (item->attribute_names == NULL && value == NULL)
        return TRUE;
    if (item->attribute_names == NULL)
    {
        item->attribute_names = g_new(char *, 2);
        item->attribute_values = g_new(char *, 2);
        item->attribute_names[0] = g_strdup(name);
        item->attribute_values[0] = g_strdup(value);
        item->attribute_names[1] = NULL;
        item->attribute_values[1] = NULL;
        return TRUE;
    }
    for (i = -1, n_attr = 0; item->attribute_names[n_attr] != NULL; n_attr++)
        if (strcmp(item->attribute_names[n_attr], name) == 0)
            i = n_attr;
    if (i < 0)
    {
        if (value == NULL) /* already unset */
            return TRUE;
        item->attribute_names = g_renew(char *, item->attribute_names, n_attr + 2);
        item->attribute_values = g_renew(char *, item->attribute_values, n_attr + 2);
        item->attribute_names[n_attr] = g_strdup(name);
        item->attribute_values[n_attr] = g_strdup(value);
        item->attribute_names[n_attr+1] = NULL;
        item->attribute_values[n_attr+1] = NULL;
    }
    else if (value != NULL) /* value changed */
    {
        g_free(item->attribute_values[i]);
        item->attribute_values[i] = g_strdup(value);
    }
    else if (n_attr == 1) /* no more attributes left */
    {
        g_strfreev(item->attribute_names);
        g_strfreev(item->attribute_values);
        item->attribute_names = NULL;
        item->attribute_values = NULL;
    }
    else /* replace removed attribute with last one if it wasn't last */
    {
        g_free(item->attribute_names[i]);
        g_free(item->attribute_values[i]);
        if (i < n_attr - 1)
        {
            item->attribute_names[i] = item->attribute_names[n_attr-1];
            item->attribute_values[i] = item->attribute_values[n_attr-1];
        }
        item->attribute_names[n_attr-1] = NULL;
        item->attribute_values[n_attr-1] = NULL;
    }
    return TRUE;
}

/**
 * fm_xml_file_item_destroy
 * @item: element to destroy
 *
 * Removes element and its children from its parent, and frees all
 * data.
 *
 * Returns: %FALSE if @item is busy thus cannot be destroyed.
 *
 * Since: 1.2.0
 */
gboolean fm_xml_file_item_destroy(FmXmlFileItem *item)
{
    g_return_val_if_fail(item != NULL, FALSE);
    if (_xml_item_is_busy(item))
        return FALSE;
    while (item->children)
    {
        g_assert(((FmXmlFileItem*)item->children->data)->file == item->file);
        g_assert(((FmXmlFileItem*)item->children->data)->parent == item);
        fm_xml_file_item_destroy(item->children->data);
    }
    if (item->parent_list)
    {
        /* g_debug("removing item %p from parser %p", item, item->file); */
        g_assert(item->file != NULL && g_list_find(*item->parent_list, item) != NULL);
        *item->parent_list = g_list_remove(*item->parent_list, item);
    }
    if (item->text != item->comment)
        g_free(item->comment);
    g_free(item->text);
    g_strfreev(item->attribute_names);
    g_strfreev(item->attribute_values);
    g_slice_free(FmXmlFileItem, item);
    return TRUE;
}

/**
 * fm_xml_file_insert_before
 * @item: item to insert before it
 * @new_item: new item to insert
 *
 * Inserts @new_item before @item that is already in XML structure. If
 * @new_item is already in the XML structure then it will be moved to
 * the new place instead.
 * Behavior after moving between defferent containers is undefined.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 1.2.0
 */
gboolean fm_xml_file_insert_before(FmXmlFileItem *item, FmXmlFileItem *new_item)
{
    GList *sibling;

    g_return_val_if_fail(item != NULL && new_item != NULL, FALSE);
    sibling = g_list_find(*item->parent_list, item);
    if (sibling == NULL) /* no such item found */
    {
        /* g_critical("item %p not found in %p", item, item->parent); */
        return FALSE;
    }
    if (_xml_item_is_busy(new_item))
        return FALSE; /* cannot move item right now */
    if (new_item->parent_list) /* remove from old list */
    {
        /* g_debug("moving item %p (parent=%p) from parser %p into %p", new_item, item, new_item->file, item->file); */
        g_assert(new_item->file != NULL && g_list_find(*new_item->parent_list, new_item) != NULL);
        *new_item->parent_list = g_list_remove(*new_item->parent_list, new_item);
    }
    /* else
        g_debug("inserting item %p (parent=%p) into parser %p", item, new_item, item->file); */
    *item->parent_list = g_list_insert_before(*item->parent_list, sibling, new_item);
    new_item->parent_list = item->parent_list;
    new_item->parent = item->parent;
    if (new_item->file != item->file)
        _reassign_xml_file(new_item, item->file);
    return TRUE;
}

/**
 * fm_xml_file_insert_first
 * @file: the parser container
 * @new_item: new item to insert
 *
 * Inserts @new_item as very first element of XML data in container.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 1.2.0
 */
gboolean fm_xml_file_insert_first(FmXmlFile *file, FmXmlFileItem *new_item)
{
    g_return_val_if_fail(file != NULL && FM_IS_XML_FILE(file), FALSE);
    g_return_val_if_fail(new_item != NULL, FALSE);
    if (_xml_item_is_busy(new_item))
        return FALSE; /* cannot move item right now */
    if (new_item->parent_list)
    {
        /* g_debug("moving item %p from parser %p into %p", new_item, new_item->file, file); */
        g_assert(new_item->file != NULL && g_list_find(*new_item->parent_list, new_item) != NULL);
        *new_item->parent_list = g_list_remove(*new_item->parent_list, new_item);
    }
    file->items = g_list_prepend(file->items, new_item);
    new_item->parent_list = &file->items;
    new_item->parent = NULL;
    if (new_item->file != file)
        _reassign_xml_file(new_item, file);
    return TRUE;
}


/* save XML */

/**
 * fm_xml_file_set_dtd
 * @file: the parser container
 * @dtd: DTD description for XML data
 * @error: (allow-none) (out): location to save error
 *
 * Changes DTD description for XML data in the container.
 *
 * Since: 1.2.0
 */
void fm_xml_file_set_dtd(FmXmlFile *file, const char *dtd, GError **error)
{
    if(file == NULL)
        return;
    /* FIXME: validate dtd */
    g_free(file->tags[0].name);
    file->tags[0].name = g_strdup(dtd);
}

static gboolean _parser_item_to_gstring(FmXmlFile *file, GString *string,
                                        FmXmlFileItem *item, GString *prefix,
                                        gboolean *has_nl, GError **error)
{
    const char *tag_name;
    GList *l;

    /* open the tag */
    switch (item->tag)
    {
    case FM_XML_FILE_TAG_NOT_HANDLED:
        if (item->tag_name == NULL)
            goto _no_tag;
        tag_name = item->tag_name;
        goto _do_tag;
    case FM_XML_FILE_TEXT:
        if (item->text == item->comment) /* CDATA */
            g_string_append_printf(string, "<![CDATA[%s]]>", item->text);
        else if (item->text) /* just text */
        {
            char *escaped;

            if (item->comment != NULL)
                g_string_append_printf(string, "<!-- %s -->", item->comment);
            escaped = g_markup_escape_text(item->text, -1);
            g_string_append(string, escaped);
            g_free(escaped);
        }
        else /* processing directive */
        {
            g_string_append_printf(string, "%s<?%s?>", prefix->str, item->comment);
            *has_nl = TRUE;
        }
        return TRUE;
    default:
        if (item->tag >= file->n_tags)
        {
_no_tag:
            g_set_error_literal(error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                                _("fm_xml_file_to_data: XML data error"));
            return FALSE;
        }
        tag_name = file->tags[item->tag].name;
_do_tag:
        /* do comment */
        if (item->comment != NULL)
            g_string_append_printf(string, "%s<!-- %s -->", prefix->str,
                                   item->comment);
        else if (item->attribute_names == NULL && item->children == NULL &&
                 file->tags[item->tag].in_line)
        {
            /* don't add prefix if it is simple tag such as <br/> */
            g_string_append_printf(string, "<%s/>", tag_name);
            return TRUE;
        }
        /* start the tag */
        g_string_append_printf(string, "%s<%s", prefix->str, tag_name);
        /* do attributes */
        if (item->attribute_names)
        {
            char **name = item->attribute_names;
            char **value = item->attribute_values;
            while (*name)
            {
                if (*value)
                {
                    char *escaped = g_markup_escape_text(*value, -1);
                    g_string_append_printf(string, " %s='%s'", *name, escaped);
                    g_free(escaped);
                } /* else error? */
                name++;
                value++;
            }
        }
        if (item->children == NULL)
        {
            /* handle empty tags such as <tag attr='value'/> */
            g_string_append(string, "/>");
            *has_nl = TRUE;
            return TRUE;
        }
        g_string_append_c(string, '>');
    }
    /* do with children */
    *has_nl = FALSE; /* to collect data from nested elements */
    g_string_append(prefix, "    ");
    for (l = item->children; l; l = l->next)
        if (!_parser_item_to_gstring(file, string, l->data, prefix, has_nl, error))
            break;
    g_string_truncate(prefix, prefix->len - 4);
    if (l != NULL) /* failed */
        return FALSE;
    /* close the tag */
    g_string_append_printf(string, "%s</%s>", (*has_nl) ? prefix->str : "",
                           tag_name);
    *has_nl = TRUE; /* it was prefixed above */
    return TRUE;
}

/**
 * fm_xml_file_to_data
 * @file: the parser container
 * @text_size: (allow-none) (out): location to save size of returned data
 * @error: (allow-none) (out): location to save error
 *
 * Prepares string representation (XML text) for the data that are in
 * the container. Returned data should be freed with g_free() after
 * usage.
 *
 * Returns: (transfer full): XML text representing data in @file.
 *
 * Since: 1.2.0
 */
char *fm_xml_file_to_data(FmXmlFile *file, gsize *text_size, GError **error)
{
    GString *string, *prefix;
    GList *l;
    gboolean has_nl = FALSE;

    g_return_val_if_fail(file != NULL && FM_IS_XML_FILE(file), NULL);
    string = g_string_sized_new(512);
    prefix = g_string_new("\n");
    if (G_LIKELY(file->tags[0].name))
        g_string_printf(string, "<!DOCTYPE %s>", file->tags[0].name);
    for (l = file->items; l; l = l->next)
        if (!_parser_item_to_gstring(file, string, l->data, prefix, &has_nl, error))
            break; /* if failed then l != NULL */
    g_string_free(prefix, TRUE);
    if (text_size)
        *text_size = string->len;
    return g_string_free(string, (l != NULL)); /* returns NULL if failed */
}


/* item data accessor functions */

/**
 * fm_xml_file_item_get_comment
 * @item: the file element to inspect
 *
 * If an element @item has a comment ahead of it then retrieves that
 * comment. The returned data are owned by @item and should not be freed
 * nor otherwise altered by caller.
 *
 * Returns: (transfer none): comment or %NULL if no comment is set.
 *
 * Since: 1.2.0
 */
const char *fm_xml_file_item_get_comment(FmXmlFileItem *item)
{
    g_return_val_if_fail(item != NULL, NULL);
    return item->comment;
}

/**
 * fm_xml_file_item_get_children
 * @item: the file element to inspect
 *
 * Retrieves list of children for @item that are known to the parser.
 * Returned list should be freed by g_list_free() after usage.
 *
 * Note: any text between opening tag and closing tag such as
 * |[ &lt;Tag&gt;Some text&lt;/Tag&gt; ]|
 * is presented as a child of special type #FM_XML_FILE_TEXT and can be
 * retrieved from that child, not from the item representing &lt;Tag&gt;.
 *
 * See also: fm_xml_file_item_get_data().
 *
 * Returns: (transfer container) (element-type FmXmlFileItem): children list.
 *
 * Since: 1.2.0
 */
GList *fm_xml_file_item_get_children(FmXmlFileItem *item)
{
    g_return_val_if_fail(item != NULL, NULL);
    return g_list_copy(item->children);
}

/**
 * fm_xml_file_item_find_child
 * @item: the file element to inspect
 * @tag: tag id to search among children
 *
 * Searches for first child of @item which have child with tag id @tag.
 * Returned data are owned by file and should not be freed by caller.
 *
 * Returns: (transfer none): found child or %NULL if no such child was found.
 *
 * Since: 1.2.0
 */
FmXmlFileItem *fm_xml_file_item_find_child(FmXmlFileItem *item, FmXmlFileTag tag)
{
    GList *l;

    for (l = item->children; l; l = l->next)
        if (((FmXmlFileItem*)l->data)->tag == tag)
            return (FmXmlFileItem*)l->data;
    return NULL;
}

/**
 * fm_xml_file_item_get_tag
 * @item: the file element to inspect
 *
 * Retrieves tag id of @item.
 *
 * Returns: tag id.
 *
 * Since: 1.2.0
 */
FmXmlFileTag fm_xml_file_item_get_tag(FmXmlFileItem *item)
{
    g_return_val_if_fail(item != NULL, FM_XML_FILE_TAG_NOT_HANDLED);
    return item->tag;
}

/**
 * fm_xml_file_item_get_data
 * @item: the file element to inspect
 * @text_size: (allow-none) (out): location to save data size
 *
 * Retrieves text data from @item of type #FM_XML_FILE_TEXT. Returned
 * data are owned by file and should not be freed nor altered.
 *
 * Returns: (transfer none): text data or %NULL if @item isn't text data.
 *
 * Since: 1.2.0
 */
const char *fm_xml_file_item_get_data(FmXmlFileItem *item, gsize *text_size)
{
    if (text_size)
        *text_size = 0;
    g_return_val_if_fail(item != NULL, NULL);
    if (item->tag != FM_XML_FILE_TEXT)
        return NULL;
    if (text_size && item->text != NULL)
        *text_size = strlen(item->text);
    return item->text;
}

/**
 * fm_xml_file_item_get_parent
 * @item: the file element to inspect
 *
 * Retrieves parent element of @item if the @item has one. Returned data
 * are owned by file and should not be freed by caller.
 *
 * Returns: (transfer none): parent element or %NULL if element has no parent.
 *
 * Since: 1.2.0
 */
FmXmlFileItem *fm_xml_file_item_get_parent(FmXmlFileItem *item)
{
    g_return_val_if_fail(item != NULL, NULL);
    return item->parent;
}

/**
 * fm_xml_file_get_tag_name
 * @file: the parser container
 * @tag: the tag id to inspect
 *
 * Retrieves tag for its id. Returned data are owned by @file and should
 * not be modified by caller.
 *
 * Returns: (transfer none): tag string representation.
 *
 * Since: 1.2.0
 */
const char *fm_xml_file_get_tag_name(FmXmlFile *file, FmXmlFileTag tag)
{
    g_return_val_if_fail(file != NULL && FM_IS_XML_FILE(file), NULL);
    g_return_val_if_fail(tag > 0 && tag < file->n_tags, NULL);
    return file->tags[tag].name;
}

/**
 * fm_xml_file_item_get_tag_name
 * @item: the file element to inspect
 *
 * Retrieves tag for its id. Returned data are owned by @item and should
 * not be modified by caller.
 *
 * Returns: (transfer none): tag string representation or %NULL if @item is text.
 *
 * Since: 1.2.0
 */
const char *fm_xml_file_item_get_tag_name(FmXmlFileItem *item)
{
    g_return_val_if_fail(item != NULL, NULL);
    switch (item->tag)
    {
    case FM_XML_FILE_TEXT:
        return NULL;
    case FM_XML_FILE_TAG_NOT_HANDLED:
        return item->tag_name;
    default:
        return item->file->tags[item->tag].name;
    }
}
