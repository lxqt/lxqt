NULL=

# GtkBuilder UI definition files
ui_in_files= \
	file-prop.glade \
	progress.glade \
	ask-rename.glade \
	app-chooser.glade \
	preferred-apps.glade \
	exec-file.glade \
	filesearch.glade \
	$(NULL)
ui_files = $(ui_in_files:.glade=.ui) choose-icon.ui
if ENABLE_GTK
uidir=$(datadir)/libfm/ui
ui_DATA = $(ui_files)
endif

EXTRA_DIST = \
	$(ui_in_files) \
	$(ui_files) \
	$(NULL)

# Purge GtkBuilder UI files
.glade.ui:
	sed 's/<!--.*-->//' < $< | sed ':a;N;$$!ba;s/ *\n *</</g' > $@
