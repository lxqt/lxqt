NULL=

AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(srcdir)../ \
	-I$(srcdir)/../base \
	-I$(srcdir)/../actions \
	-I$(srcdir)/../job \
	-DPACKAGE_DATA_DIR=\""$(datadir)/libfm"\" \
	-DPACKAGE_LIB_DIR=\""$(libdir)/libfm"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-Wno-unused-variable \
	$(NULL)

noinst_LTLIBRARIES = libfmactions.la

libfmactions_la_SOURCES = \
	action.vala \
	condition.vala \
	profile.vala \
	parameters.vala \
	utils.vala \
	$(NULL)

libfmactions_la_CFLAGS = \
	$(GIO_CFLAGS) \
	-Werror-implicit-function-declaration \
	$(NULL)

AM_VALAFLAGS = \
	--thread \
	--vapidir=$(srcdir)/../vapi \
	--pkg gio-2.0 \
    --pkg posix \
    --pkg libfm \
    --vapi $(srcdir)/../vapi/fm-actions.vapi \
    --header fm-actions.h \
	$(NULL)

libfmactionsincludedir = $(includedir)/@PACKAGE@-@FMLIBVER@
libfmactionsinclude_HEADERS = \
	fm-actions.h \
	$(NULL)

libfmactions_la_LDFLAGS = \
	$(NULL)

libfmactions_la_LIBADD = \
	$(GIO_LIBS) \
	$(INTLLIBS) \
	$(NULL)

fm-actions.h: libfmactions_la_vala.stamp
