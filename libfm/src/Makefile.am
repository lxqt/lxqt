# for use in libtool -version-info
ABI_VERSION=5:2:1

NULL=

# current dir needs to be built before tests
SUBDIRS= \
	$(ACTIONS_SUBDIR) \
	. \
	$(NULL)

if !EXTRALIB_ONLY
SUBDIRS += \
	modules \
	tests \
	$(NULL)
endif

AM_CPPFLAGS = \
	-I$(srcdir)/base \
	-I$(srcdir)/job \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-Werror-implicit-function-declaration \
	$(NULL)

glib_compat_SOURCES= \
	glib-compat.c \
	glib-compat.h \
	$(NULL)

gtk_compat_SOURCES= \
	$(glib_compat_SOURCES) \
	gtk-compat.c \
	gtk-compat.h \
	$(NULL)

if ENABLE_UDISKS
udisks_SOURCES= \
	udisks/fm-udisks.c \
	udisks/fm-udisks.h \
	udisks/g-udisks-volume-monitor.c \
	udisks/g-udisks-volume-monitor.h \
	udisks/g-udisks-device.c \
	udisks/g-udisks-device.h \
	udisks/g-udisks-volume.c \
	udisks/g-udisks-volume.h \
	udisks/g-udisks-drive.c \
	udisks/g-udisks-drive.h \
	udisks/g-udisks-mount.c \
	udisks/g-udisks-mount.h \
	udisks/dbus-utils.c \
	udisks/dbus-utils.h \
	$(NULL)
else
udisks_SOURCES=
endif

base_SOURCES = \
	base/fm-action.c \
	base/fm-app-info.c \
	base/fm-archiver.c \
	base/fm-bookmarks.c \
	base/fm-config.c \
	base/fm-dummy-monitor.c \
	base/fm-file.c \
	base/fm-file-info.c \
	base/fm-file-launcher.c \
	base/fm-folder.c \
	base/fm-folder-config.c \
	base/fm-icon.c \
	base/fm-list.c \
	base/fm-marshal.c \
	base/fm-mime-type.c \
	base/fm-module.c \
	base/fm-monitor.c \
	base/fm-nav-history.c \
	base/fm-path.c \
	base/fm-templates.c \
	base/fm-terminal.c \
	base/fm-thumbnail-loader.c \
	base/fm-thumbnailer.c \
	base/fm-utils.c \
	$(NULL)

job_SOURCES = \
	job/fm-deep-count-job.c  \
	job/fm-dir-list-job.c \
	job/fm-file-info-job.c \
	job/fm-file-ops-job.c \
	job/fm-file-ops-job-change-attr.c \
	job/fm-file-ops-job-delete.c \
	job/fm-file-ops-job-xfer.c \
	job/fm-job.c \
	job/fm-simple-job.c \
	$(NULL)

libfm_SOURCES = \
	fm.c \
	$(glib_compat_SOURCES) \
	$(base_SOURCES) \
	$(job_SOURCES) \
	$(udisks_SOURCES) \
	$(NULL)

libfm_extra_SOURCES = \
	extra/fm-xml-file.c \
	$(NULL)

exo_SOURCES = \
	gtk/exo/exo-icon-view.c \
	gtk/exo/exo-private.c \
	gtk/exo/exo-tree-view.c \
	gtk/exo/exo-icon-view.h \
	gtk/exo/exo-private.h \
	gtk/exo/exo-tree-view.h \
	gtk/exo/exo-marshal.c \
	gtk/exo/exo-string.c \
	gtk/exo/exo-marshal.h \
	gtk/exo/exo-string.h \
	$(NULL)

libfm_gtk_SOURCES = \
	fm-gtk.c \
	$(gtk_compat_SOURCES) \
	$(exo_SOURCES) \
	gtk/fm-app-chooser-combo-box.c \
	gtk/fm-app-chooser-dlg.c \
	gtk/fm-app-menu-view.c \
	gtk/fm-cell-renderer-pixbuf.c \
	gtk/fm-cell-renderer-text.c \
	gtk/fm-clipboard.c \
	gtk/fm-dir-tree-model.c \
	gtk/fm-dir-tree-view.c \
	gtk/fm-dnd-auto-scroll.c \
	gtk/fm-dnd-dest.c \
	gtk/fm-dnd-src.c \
	gtk/fm-file-menu.c \
	gtk/fm-file-properties.c \
	gtk/fm-folder-model.c \
	gtk/fm-folder-view.c \
	gtk/fm-gtk-file-launcher.c \
	gtk/fm-gtk-utils.c \
	gtk/fm-gtk-marshal.c \
	gtk/fm-icon-pixbuf.c \
	gtk/fm-menu-tool-item.c \
	gtk/fm-path-bar.c \
	gtk/fm-path-entry.c \
	gtk/fm-places-model.c \
	gtk/fm-places-view.c \
	gtk/fm-progress-dlg.c \
	gtk/fm-side-pane.c \
	gtk/fm-standard-view.c \
	gtk/fm-tab-label.c \
	gtk/fm-thumbnail.c \
	$(NULL)

# we use common include directory for all headers
pkgincludedir = $(includedir)/@PACKAGE@-@FMLIBVER@

if EXTRALIB_ONLY
pkginclude_HEADERS = $(LIBFM_EXTRA_INCLUDES)
else
LIBFM_INCLUDES = \
	fm.h \
	fm-seal.h \
	base/fm-action.h \
	base/fm-app-info.h \
	base/fm-archiver.h \
	base/fm-bookmarks.h \
	base/fm-config.h \
	base/fm-dummy-monitor.h \
	base/fm-file.h \
	base/fm-file-info.h \
	base/fm-file-launcher.h \
	base/fm-folder.h \
	base/fm-folder-config.h \
	base/fm-icon.h \
	base/fm-list.h \
	base/fm-marshal.h \
	base/fm-mime-type.h \
	base/fm-module.h \
	base/fm-monitor.h \
	base/fm-nav-history.h \
	base/fm-path.h \
	base/fm-templates.h \
	base/fm-terminal.h \
	base/fm-thumbnail-loader.h \
	base/fm-thumbnailer.h \
	base/fm-utils.h \
	job/fm-deep-count-job.h \
	job/fm-dir-list-job.h \
	job/fm-file-info-job.h \
	job/fm-file-ops-job.h \
	job/fm-file-ops-job-change-attr.h \
	job/fm-file-ops-job-delete.h \
	job/fm-file-ops-job-xfer.h \
	job/fm-job.h \
	job/fm-simple-job.h \
	$(NULL)

LIBFM_GTK_INCLUDES = \
	fm-gtk.h \
	gtk/fm-app-chooser-combo-box.h \
	gtk/fm-app-chooser-dlg.h \
	gtk/fm-app-menu-view.h \
	gtk/fm-cell-renderer-pixbuf.h \
	gtk/fm-cell-renderer-text.h \
	gtk/fm-clipboard.h \
	gtk/fm-dir-tree-model.h \
	gtk/fm-dir-tree-view.h \
	gtk/fm-dnd-auto-scroll.h \
	gtk/fm-dnd-dest.h \
	gtk/fm-dnd-src.h \
	gtk/fm-file-menu.h \
	gtk/fm-file-properties.h \
	gtk/fm-folder-model.h \
	gtk/fm-folder-view.h \
	gtk/fm-gtk-file-launcher.h \
	gtk/fm-gtk-marshal.h \
	gtk/fm-gtk-utils.h \
	gtk/fm-icon-pixbuf.h \
	gtk/fm-menu-tool-item.h \
	gtk/fm-path-bar.h \
	gtk/fm-path-entry.h \
	gtk/fm-places-model.h \
	gtk/fm-places-view.h \
	gtk/fm-progress-dlg.h \
	gtk/fm-side-pane.h \
	gtk/fm-sortable.h \
	gtk/fm-standard-view.h \
	gtk/fm-tab-label.h \
	gtk/fm-thumbnail.h \
	$(NULL)

pkginclude_HEADERS = \
	$(LIBFM_INCLUDES) \
	$(LIBFM_EXTRA_INCLUDES) \
	$(LIBFM_GTK_INCLUDES) \
	$(NULL)
endif

LIBFM_EXTRA_INCLUDES = \
	fm-extra.h \
	fm-version.h \
	extra/fm-xml-file.h \
	$(NULL)

EXTRA_LTLIBRARIES = libfm-gtk.la libfm-gtk3.la

if EXTRALIB_ONLY
lib_LTLIBRARIES = libfm-extra.la
else
lib_LTLIBRARIES = libfm-extra.la libfm.la @LIBFM_GTK_LTLIBRARIES@
endif

libfm_la_SOURCES = \
	$(libfm_SOURCES) \
	$(NULL)

libfm_la_CFLAGS = \
	$(GIO_CFLAGS) \
	$(MENU_CACHE_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(EXIF_CFLAGS) \
	-DPACKAGE_DATA_DIR=\""$(datadir)/libfm"\" \
	-DPACKAGE_MODULES_DIR=\""$(libdir)/@PACKAGE@/modules"\" \
	$(NULL)

libfm_la_LIBADD = \
	$(GIO_LIBS) \
	$(MENU_CACHE_LIBS) \
	$(DBUS_LIBS) \
	$(EXIF_LIBS) \
	$(INTLLIBS) \
	$(NULL)

libfm_la_LDFLAGS = \
	-no-undefined \
	-export-symbols-regex ^fm \
	-version-info $(ABI_VERSION) \
	$(NULL)

if HAVE_OLD_ACTIONS
LIBFMACTIONS = $(top_builddir)/src/actions/libfmactions.la
libfm_la_LIBADD += $(LIBFMACTIONS)
libfm_la_DEPENDENCIES_EXTRA = $(LIBFMACTIONS)
endif

libfm_extra_la_SOURCES = \
	$(libfm_extra_SOURCES) \
	$(NULL)

libfm_extra_la_CFLAGS = \
	-I$(srcdir)/extra \
	$(GIO_CFLAGS) \
	$(NULL)

libfm_extra_la_LIBADD = \
	$(GIO_LIBS) \
	$(INTLLIBS) \
	$(NULL)

libfm_extra_la_LDFLAGS = \
	-no-undefined \
	-export-symbols-regex ^fm \
	-version-info $(ABI_VERSION) \
	$(NULL)

libfm_gtk_la_SOURCES = \
	$(libfm_gtk_SOURCES) \
	$(NULL)
libfm_gtk3_la_SOURCES = $(libfm_gtk_la_SOURCES)

libfm_gtk_la_CFLAGS = \
	-I$(srcdir)/gtk \
	-DPACKAGE_UI_DIR=\""$(datadir)/libfm/ui"\" \
	-DPACKAGE_THEME_DIR=\""$(datadir)/libfm/images"\" \
	$(GTK_CFLAGS) \
	$(MENU_CACHE_CFLAGS) \
	$(NULL)
libfm_gtk3_la_CFLAGS = $(libfm_gtk_la_CFLAGS)

libfm_gtk_la_LIBADD = \
	$(GTK_LIBS) \
	$(MENU_CACHE_LIBS) \
	$(INTLLIBS) \
	libfm.la \
	$(NULL)
libfm_gtk3_la_LIBADD = $(libfm_gtk_la_LIBADD)

libfm_gtk_la_LDFLAGS = \
	-no-undefined \
	-export-symbols-regex ^fm \
	-version-info $(ABI_VERSION) \
	-rpath $(libdir) \
	$(NULL)
libfm_gtk3_la_LDFLAGS = $(libfm_gtk_la_LDFLAGS)

EXTRA_PROGRAMS = libfm-pref-apps lxshortcut

if EXTRALIB_ONLY
bin_PROGRAMS =
else
bin_PROGRAMS = @LIBFM_PREF_APPS@
endif

libfm_pref_apps_SOURCES = \
	$(gtk_compat_SOURCES) \
	tools/libfm-pref-apps.c \
	$(NULL)

libfm_pref_apps_DEPENDENCIES = \
	libfm.la \
	@LIBFM_GTK_LTLIBRARIES@ \
	$(NULL)

libfm_pref_apps_CFLAGS = \
	-I$(srcdir)/gtk \
	$(GTK_CFLAGS) \
	$(MENU_CACHE_CFLAGS) \
	-DPACKAGE_UI_DIR=\""$(datadir)/libfm/ui"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(NULL)

libfm_pref_apps_LDADD = \
	$(GTK_LIBS) \
	$(MENU_CACHE_LIBS) \
	$(INTLLIBS) \
	libfm.la \
	@LIBFM_GTK_LTLIBRARIES@ \
	$(NULL)

lxshortcut_SOURCES = \
	tools/lxshortcut.c \
	$(NULL)

lxshortcut_DEPENDENCIES = \
	libfm.la \
	@LIBFM_GTK_LTLIBRARIES@ \
	$(NULL)

lxshortcut_CFLAGS = \
	-I$(srcdir)/gtk \
	$(GTK_CFLAGS) \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(NULL)

lxshortcut_LDADD = \
	$(GTK_LIBS) \
	$(INTLLIBS) \
	libfm.la \
	@LIBFM_GTK_LTLIBRARIES@ \
	$(NULL)


if ENABLE_DEMO
bin_PROGRAMS += libfm-demo
libfm_demo_SOURCES = \
	$(gtk_compat_SOURCES) \
	demo/libfm-demo.c \
	demo/main-win.c demo/main-win.h \
	$(NULL)

libfm_demo_DEPENDENCIES = \
	libfm.la \
	@LIBFM_GTK_LTLIBRARIES@ \
	$(NULL)

libfm_demo_CFLAGS = \
	-I$(srcdir)/gtk \
	$(GTK_CFLAGS) \
	$(NULL)

libfm_demo_LDADD = \
	$(GTK_LIBS) \
	$(INTLLIBS) \
	libfm.la \
	libfm-extra.la \
	@LIBFM_GTK_LTLIBRARIES@ \
	$(NULL)
endif

if !EXTRALIB_ONLY
# GIO module implementing some extension points
# This only works for glib < 2.27
# Glib >= 2.27 uses x-scheme-handler.
if !HAVE_SCHEME_HANDLER

giomodules_LTLIBRARIES = libgiofm.la

libgiofm_la_SOURCES= \
	$(glib_compat_SOURCES) \
	gio/module.c \
	gio/fm-app-lookup.c \
	gio/fm-app-lookup.h \
	$(NULL)

libgiofm_la_CFLAGS = \
	$(GIO_CFLAGS) \
	-DGIO_MODULE_DIR=\"$(giomodulesdir)\"	\
	-DGVFS_LOCALEDIR=\""$(localedir)"\"	\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DG_DISABLE_DEPRECATED \
	$(NULL)

libgiofm_la_LDFLAGS = 	\
	-export_dynamic \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^g_io_module_(load|unload|query)'
	$(NULL)

libgiofm_la_LIBADD = \
	$(GIO_LIBS) \
	$(INTLLIBS) \
	$(NULL)

endif

# GObject marshallers
base/fm-marshal.h: base/fm-marshal.list
	glib-genmarshal --header --prefix=fm_marshal $? > $@.tmp && mv $@.tmp $@

base/fm-marshal.c: base/fm-marshal.list
	glib-genmarshal --body --prefix=fm_marshal $? > $@.tmp && mv $@.tmp $@

gtk/fm-gtk-marshal.h: gtk/fm-gtk-marshal.list
	glib-genmarshal --header --prefix=fm_marshal $? > $@.tmp && mv $@.tmp $@

gtk/fm-gtk-marshal.c: gtk/fm-gtk-marshal.list
	glib-genmarshal --body --prefix=fm_marshal $? > $@.tmp && mv $@.tmp $@

# Workarounds to force the build
BUILT_SOURCES= \
	base/fm-marshal.h \
	base/fm-marshal.c \
	gtk/fm-gtk-marshal.h \
	gtk/fm-gtk-marshal.c \
	$(NULL)

# Vala vapi files
VAPI_FILES = \
	vapi/libfm.vapi \
	$(NULL)
endif

EXTRA_DIST = \
	base/fm-marshal.list \
	gtk/fm-gtk-marshal.list \
	gtk/fm-file-search-ui.c \
	gtk/exo/README \
	demo/main-win-ui.c \
	udisks/gen-binding \
	udisks/udisks.h \
	udisks/udisks-device.h \
	$(VAPI_FILES) \
	$(NULL)

CLEANFILES = \
	base/fm-marshal.h \
	base/fm-marshal.c \
	gtk/fm-gtk-marshal.h \
	gtk/fm-gtk-marshal.c \
	$(NULL)

# A lib to be loaede with LD_PRELOADS to override GtkFileChooserWidget
#lib_LTLIBRARIES=libfm-chooser-hack.la

#libfm_chooser_hack_la_SOURCES=libfm-chooser-hack.c
#libfm_chooser_hack_la_CFLAGS=$(GTK_CFLAGS)
#libfm_chooser_hack_la_LIBADD=$(GMODULE_LIBS)
#libfm_chooser_hack_la_LDFLAGS= \
#        -no-undefined \
#        -export-symbols-regex gtk

# do make test only against single directory
test:
	cd tests && $(MAKE) $(AM_MAKEFLAGS) $@

install-data-local:
	@if test -e "$(DESTDIR)$(includedir)/@PACKAGE@"; then \
		echo rm -rf "$(DESTDIR)$(includedir)/@PACKAGE@"; \
		rm -rf "$(DESTDIR)$(includedir)/@PACKAGE@"; \
	fi
	test -z "$(includedir)" || $(MKDIR_P) "$(DESTDIR)$(includedir)"
	@LN_S@ @PACKAGE@-@FMLIBVER@ "$(DESTDIR)$(includedir)/@PACKAGE@"

uninstall-local:
	@if test -L "$(DESTDIR)$(includedir)/@PACKAGE@"; then \
		echo rm -f "$(DESTDIR)$(includedir)/@PACKAGE@"; \
		rm -f "$(DESTDIR)$(includedir)/@PACKAGE@"; \
	fi
	-rmdir "$(DESTDIR)$(libfmincludedir)"
