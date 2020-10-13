/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2006 The Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 */
 
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include "java-utils.h"


/* 
 * The following code conforms to the JVM specification.(Java 2 Platform)
 * For further changes to the classfile structure, please update the 
 * following macros.
 */


/* Tags that identify structures */

#define CONST_CLASS				 7
#define CONST_FIELDREF				 9
#define CONST_METHODREF				10
#define CONST_INTERFACEMETHODREF		11
#define CONST_STRING				 8
#define CONST_INTEGER				 3
#define CONST_FLOAT				 4
#define CONST_LONG				 5
#define CONST_DOUBLE				 6
#define CONST_NAMEANDTYPE			12
#define CONST_UTF8				 1

/* Sizes of structures */

#define CONST_CLASS_INFO			 2
#define CONST_FIELDREF_INFO			 4
#define CONST_METHODREF_INFO	 		 4
#define CONST_INTERFACEMETHODREF_INFO		 4
#define CONST_STRING_INFO			 2
#define CONST_INTEGER_INFO			 4
#define CONST_FLOAT_INFO			 4
#define CONST_LONG_INFO				 8
#define CONST_DOUBLE_INFO			 8
#define CONST_NAMEANDTYPE_INFO			 4


/* represents the utf8 strings in class file */
struct utf_string	
{
	guint16  index;
	guint16  length;
	char    *str;
};

/* structure that holds class information in a class file */
struct class_info
{
	guint16 index;
	guint16 name_index; /* index into the utf_strings */
};

typedef struct {
	int     fd;
	
	guint32 magic_no;		/* 0xCAFEBABE (JVM Specification) :) */

	guint16 major;			/* versions */
	guint16 minor;

	guint16 const_pool_count;
	GSList *const_pool_class;	/* (const_pool_count - 1) elements of tye 'CONST_class_info' */
	GSList *const_pool_utf;		/* (const_pool_count - 1) elements of type 'utf_strings' */

	guint16 access_flags;
	guint16 this_class;		/* the index of the class the file is named after. */
	
#if 0 /* not needed */
	guint16         super_class;
	guint16         interfaces_count;
	guint16        *interfaces;
	guint16         fields_count;
	field_info     *fields;
	guint16         methods_count;
	method_info    *methods;
	guint16         attributes_count;
	attribute_info *attributes;
#endif
} JavaClassFile;


static JavaClassFile*
java_class_file_new (void)
{
	JavaClassFile *cfile;
	
	cfile = g_new0 (JavaClassFile, 1);
	cfile->fd = -1;
	
	return cfile;
}


static void
java_class_file_free (JavaClassFile *cfile)
{
	GSList *scan;

	if (cfile->const_pool_class != NULL) {
		g_slist_foreach (cfile->const_pool_class, (GFunc)g_free, NULL);
		g_slist_free (cfile->const_pool_class);
	}

	for (scan = cfile->const_pool_utf; scan ; scan = scan->next) {
		struct utf_string *string = scan->data;
		g_free (string->str);
	}
	
	if (cfile->const_pool_utf != NULL) {
		g_slist_foreach (cfile->const_pool_utf, (GFunc)g_free, NULL);
		g_slist_free (cfile->const_pool_utf);
	}

	if (cfile->fd != -1)
		close (cfile->fd);

	g_free (cfile);
}


/* The following function loads the utf8 strings and class structures from the 
 * class file. */
static void
load_constant_pool_utfs (JavaClassFile *cfile)
{
	guint8  tag;
	guint16 i = 0;		/* should be comparable with const_pool_count */
	
	while ((i < cfile->const_pool_count - 1) && (read (cfile->fd, &tag, 1) != -1)) {
		struct utf_string *txt = NULL;
		struct class_info *class = NULL;
		
		switch (tag) {
		case CONST_CLASS:
			class = g_new0 (struct class_info, 1);
			class->index = i + 1;
			if (read (cfile->fd, &class->name_index, 2) != 2) {
				g_free (class);
				return;	/* error reading */
			}
			class->name_index = GUINT16_FROM_BE (class->name_index);
			cfile->const_pool_class = g_slist_append (cfile->const_pool_class, class);
			break;
		
		case CONST_FIELDREF:
			lseek (cfile->fd, CONST_FIELDREF_INFO, SEEK_CUR);
			break;
		
		case CONST_METHODREF:
			lseek (cfile->fd, CONST_METHODREF_INFO, SEEK_CUR);
			break;
		
		case CONST_INTERFACEMETHODREF:
			lseek (cfile->fd, CONST_INTERFACEMETHODREF_INFO, SEEK_CUR);
			break;
		
		case CONST_STRING:
			lseek (cfile->fd, CONST_STRING_INFO, SEEK_CUR);
			break;
		
		case CONST_INTEGER:
			lseek (cfile->fd, CONST_INTEGER_INFO, SEEK_CUR);
			break;
		
		case CONST_FLOAT:
			lseek (cfile->fd, CONST_FLOAT_INFO, SEEK_CUR);
			break;
		
		case CONST_LONG:
			lseek (cfile->fd, CONST_LONG_INFO, SEEK_CUR);
			break;
		
		case CONST_DOUBLE:
			lseek (cfile->fd, CONST_DOUBLE_INFO, SEEK_CUR);
			break;
		
		case CONST_NAMEANDTYPE:
			lseek (cfile->fd, CONST_NAMEANDTYPE_INFO, SEEK_CUR);
			break;
		
		case CONST_UTF8:
			txt = g_new0 (struct utf_string, 1);
			txt->index = i + 1;
			if (read (cfile->fd, &(txt->length), 2) == -1) {
				g_free (txt);
				return;	/* error while reading */
			}
			txt->length = GUINT16_FROM_BE (txt->length);
			txt->str = g_new0 (char, txt->length);
			if (read (cfile->fd, txt->str, txt->length) == -1) {
				g_free (txt);
				return;	/* error while reading */
			}
			cfile->const_pool_utf = g_slist_append (cfile->const_pool_utf, txt);
			break;
		
		default:
			return;	/* error - unknown tag in class file */
			break;
		}
		i++;
	}
	
#ifdef DEBUG
	g_print( "Number of Entries: %d\n", i );
#endif
}


static char*
close_and_exit (JavaClassFile *cfile) 
{
	java_class_file_free (cfile);
	return NULL;
}


/* This function extracts the package name from a class file */
char*
get_package_name_from_class_file (char *fname)
{
	char          *package = NULL;
	JavaClassFile *cfile;
	guint16        length = 0, end = 0, utf_index = 0;
	guint32        magic;
	guint16        major, minor, count;
	int            i = 0;
	
	if (! g_file_test (fname, G_FILE_TEST_EXISTS))
		return NULL;

	cfile = java_class_file_new ();
	cfile->fd = open (fname, O_RDONLY);
	if (cfile->fd == -1)
		return close_and_exit (cfile);

	if ((i = read (cfile->fd, &magic, 4)) != 4)
		return close_and_exit (cfile);
	cfile->magic_no = GUINT32_FROM_BE (magic);

	if (read (cfile->fd, &major, 2 ) != 2)
		return close_and_exit (cfile);
	cfile->major = GUINT16_FROM_BE (major);

	if (read (cfile->fd, &minor, 2) != 2)
		return close_and_exit (cfile);
	cfile->minor = GUINT16_FROM_BE (minor);

	if (read (cfile->fd, &count, 2) != 2)
		return close_and_exit (cfile);
	cfile->const_pool_count = GUINT16_FROM_BE(count);
	load_constant_pool_utfs (cfile);

	if (read (cfile->fd, &cfile->access_flags, 2) != 2)
		return close_and_exit (cfile);
	cfile->access_flags = GUINT16_FROM_BE (cfile->access_flags);

	if (read (cfile->fd, &cfile->this_class, 2) != 2)
		return close_and_exit (cfile);
	cfile->this_class = GUINT16_FROM_BE(cfile->this_class);

	/* now search for the class structure with index = cfile->this_class */
	
	for (i = 0; (i < g_slist_length (cfile->const_pool_class)) && (utf_index == 0); i++ ) {
		struct class_info *class = g_slist_nth_data (cfile->const_pool_class, i);
		if (class->index == cfile->this_class)
			utf_index = class->name_index; /* terminates loop */
	}

	/* now search for the utf8 string with index = utf_index */
	
	for (i = 0; i < g_slist_length (cfile->const_pool_utf); i++) {
		struct utf_string *data = g_slist_nth_data (cfile->const_pool_utf, i);
		if (data->index == utf_index) {
			package = g_strndup (data->str, data->length);
			length = data->length;
			break;
		}
	}

	if (package != NULL) {
		for (i = length; (i >= 0) && (end == 0); i-- )
			if (package[i] == '/')
				end = i;
                char *package_padded = g_strndup (package, end);
                g_free(package);
                package = package_padded;
	}

	java_class_file_free (cfile);
	
	return package;
}


/* This function consumes a comment from the java file 
 * multiline = TRUE implies that comment is multiline */
static void
consume_comment (int      fdesc,
		 gboolean multiline)
{
	gboolean escaped = FALSE;
	gboolean star = FALSE;
	char     ch;
		
	while (read (fdesc, &ch, 1) == 1) {
		switch (ch) {
		case '/':
			if (escaped)
				break;
			else if (star)
				return;
			break;
			
		case '\n':
			if (! multiline)
				return;
			break;
			
		case '*':
			escaped = FALSE;
			star = TRUE;
			break;
			
		case '\\':
			escaped = ! escaped;
			break;
			
		default:
			escaped = FALSE;
			star = FALSE;
			break;
		}
	}
}


/* This function extracts package name from a java file */
char*
get_package_name_from_java_file (char *fname)
{
	char          *package = NULL;
	JavaClassFile *cfile;
	gboolean       prev_char_is_bslash = FALSE;
	gboolean       valid_char_found = FALSE;
	char           ch;
	
	if (! g_file_test (fname, G_FILE_TEST_EXISTS))
		return NULL;

	cfile = java_class_file_new ();
	cfile->fd = open (fname, O_RDONLY);
	if (cfile->fd == -1)
		return close_and_exit (cfile);
	
	while (! valid_char_found && (read (cfile->fd, &ch, 1) == 1)) {
		switch (ch) {
		case '/':
			if (prev_char_is_bslash == TRUE) { 
				consume_comment (cfile->fd, FALSE);
				prev_char_is_bslash = FALSE;
			}
			else
				prev_char_is_bslash = TRUE;
			break;
				
		case '*':
			if (prev_char_is_bslash == TRUE)
				consume_comment (cfile->fd, TRUE);
			prev_char_is_bslash = FALSE;
			break;
			
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			prev_char_is_bslash = FALSE;
			break;
				
		default:
			prev_char_is_bslash = FALSE;
			valid_char_found = TRUE;
			break;
		}
	}
	
	if (ch == 'p')	{
		char first_valid_word[8] = "";
		
		first_valid_word[0] = 'p';
		if (read (cfile->fd, &first_valid_word[1], 6) != 6) 
			return close_and_exit (cfile);
			
		first_valid_word[7] = 0;
		if (g_ascii_strcasecmp (first_valid_word, "package") == 0) {
			char buffer[500];
			int  index = 0;
			
			while (read (cfile->fd, &ch, 1) == 1) {
				if (ch == ';')
					break;
				if (ch == '.')
					buffer[index++] = '/';
				else 
					buffer[index++] = ch;
			}
			buffer[index] = 0;
			package = g_strdup (buffer);
		}
	}

	java_class_file_free (cfile);

	return package;
}
