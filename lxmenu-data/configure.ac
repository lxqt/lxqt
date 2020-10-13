AC_INIT([lxmenu-data], [0.1.5],
        [http://lxde.org/])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz])

GETTEXT_PACKAGE=lxmenu-data
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of default gettext domain])
AC_SUBST(GETTEXT_PACKAGE)

# Generate po/LINGUAS on the fly rather than relying on translators
# to maintain it manually. This also overcome the problem that Transifex
# cannot add a language to po/LINGUAS if a new po file is submitted.
rm -f $srcdir/po/LINGUAS
for po_file in `ls $srcdir/po/*.po | sort`;
do
    lang=`echo "$po_file" | sed "s|.*/po/\(.*\)\.po|\1|g"`
    echo $lang >> $srcdir/po/LINGUAS
done

AM_GLIB_GNU_GETTEXT

IT_PROG_INTLTOOL([0.40.0])

dnl dirty hack used to prevent the installation of useless mo files.
USE_NLS=no
AC_SUBST(USE_NLS)

AC_OUTPUT([
Makefile
layout/Makefile
desktop-directories/Makefile
po/Makefile.in
])
