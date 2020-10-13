NULL=

SUBDIRS=ui

man_files = libfm-pref-apps.1 lxshortcut.1
if ENABLE_GTK
man_MANS = $(man_files)
endif

mimedir = $(datadir)/mime/packages
mime_DATA = libfm.xml

libfmconfdir = $(sysconfdir)/xdg/libfm
libfmconf_DATA = \
	libfm.conf \
	$(NULL)

desktop_in_files= \
	libfm-pref-apps.desktop.in \
	lxshortcut.desktop.in \
	$(NULL)
desktop_files = $(desktop_in_files:.desktop.in=.desktop)
if ENABLE_GTK
desktopdir=$(datadir)/applications
desktop_DATA = $(desktop_files)
endif
@INTLTOOL_DESKTOP_RULE@

libfmdatafilesdir = $(datadir)/libfm
libfmdatafiles_DATA = \
	archivers.list \
	terminals.list \
	$(NULL)

libfmimages_files = \
	images/unknown.png \
	images/folder.png \
	$(NULL)
if ENABLE_GTK
libfmimagesdir = $(datadir)/libfm/images
libfmimages_DATA = $(libfmimages_files)
endif

EXTRA_DIST = \
	$(libfmconf_DATA) \
	$(man_files) \
	$(mime_DATA) \
	$(libfmdatafiles_DATA) \
	$(desktop_in_files) \
	$(desktop_files) \
	$(libfmimages_files) \
	$(NULL)

test:
