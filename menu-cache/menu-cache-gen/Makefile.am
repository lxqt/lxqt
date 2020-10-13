NULL =

AM_CPPFLAGS = \
	-I$(top_builddir)/libmenu-cache \
	-I$(top_srcdir)/libmenu-cache \
	$(GLIB_CFLAGS) \
	$(LIBFM_EXTRA_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-Werror-implicit-function-declaration \
	$(NULL)

pkglibexec_PROGRAMS = menu-cache-gen

menu_cache_gen_SOURCES = \
	main.c \
	menu-merge.c \
	menu-compose.c \
	$(NULL)

menu_cache_gen_LDADD = \
	$(GLIB_LIBS) \
	$(LIBFM_EXTRA_LIBS) \
	$(NULL)

EXTRA_DIST = \
	menu-tags.h \
	$(NULL)
