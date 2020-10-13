NULL =

AM_CPPFLAGS = \
	-I$(top_srcdir)/libmenu-cache \
	-I$(top_builddir)/libmenu-cache \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-DMENUCACHE_LIBEXECDIR="\"$(pkglibexecdir)\"" \
	-Werror-implicit-function-declaration \
	$(NULL)

pkglibexec_PROGRAMS = menu-cached

menu_cached_SOURCES =		\
	menu-cached.c			\
	$(NULL)

menu_cached_LDADD = 		\
	$(GLIB_LIBS)					\
	$(NULL)
menu_cached_LDFLAGS =			\
	-no-undefined				\
	$(NULL)
