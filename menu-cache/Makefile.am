## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS= -I m4

NULL =

SUBDIRS_DOCS = docs

EXTRA_DIST_DOCS = \
	gtk-doc.make \
	docs/Makefile.am \
	docs/Makefile.in \
	docs/reference/Makefile.am \
	docs/reference/Makefile.in \
	docs/reference/libmenu-cache/Makefile.am \
	docs/reference/libmenu-cache/Makefile.in \
	$(NULL)

ALL_SUBDIRS = 			\
	libmenu-cache		\
	menu-cache-gen		\
	menu-cache-daemon	\
	$(NULL)

SUBDIRS = $(ALL_SUBDIRS)
DIST_SUBDIRS = $(ALL_SUBDIRS)

EXTRA_DIST = 			\
	$(NULL)

if ENABLE_GTK_DOC
DIST_SUBDIRS += $(SUBDIRS_DOCS)
SUBDIRS += $(SUBDIRS_DOCS)
else
EXTRA_DIST += $(EXTRA_DIST_DOCS)
endif

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc
