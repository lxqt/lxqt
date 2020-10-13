NULL =

AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-Werror-implicit-function-declaration \
	-DMENUCACHE_LIBEXECDIR="\"$(pkglibexecdir)\"" \
	-DG_LOG_DOMAIN=\"Menu-Cache\" \
	$(NULL)

lib_LTLIBRARIES = libmenu-cache.la

libmenu_cache_la_SOURCES =		\
	menu-cache.c			\
	$(NULL)

libmenu_cache_la_LIBADD = 		\
	$(GLIB_LIBS)			\
	$(NULL)

libmenu_cache_la_LDFLAGS =			\
	-no-undefined				\
	-export-symbols-regex menu_cache	\
	-version-info 5:0:2 \
	$(NULL)

lib_menu_cache_includedir = $(includedir)/menu-cache

nodist_lib_menu_cache_include_HEADERS =		\
	menu-cache.h			\
	$(NULL)

EXTRA_DIST =				\
	version.h			\
	libmenu-cache.pc.in		\
	$(NULL)

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = libmenu-cache.pc
