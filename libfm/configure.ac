#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libfm], [1.3.1], [http://pcmanfm.sourceforge.net/])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects no-dist-gzip dist-xz -Wno-portability])
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_HEADERS([config.h])

dnl Support silent build rules. Disable by either passing --disable-silent-rules
dnl to configure or passing V=1 to make
AM_SILENT_RULES([yes])

dnl version of API generation
FMLIBVER="1.0"

# automake 1.12 requires AM_PROG_AR.
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_LIBTOOL

# Test if we address libfm-extra compilation only
AC_ARG_WITH(extra-only,
    [AC_HELP_STRING([--with-extra-only],
        [disable all components but libfm-extra library])],
    [extra_only="${withval}"],
    [extra_only=no]
)
AM_CONDITIONAL(EXTRALIB_ONLY, [test x$extra_only = xyes])

# Vala
AC_ARG_ENABLE(old-actions,
    [AC_HELP_STRING([--disable-old-actions],
        [disable old custom actions code if you have no decent Vala compiler])],
    [enable_actions="${enableval}"],
    [enable_actions=yes]
)
AM_COND_IF(EXTRALIB_ONLY,
    [enable_actions=no])
if test x"$enable_actions" = xyes; then
    dnl no vala compiler required when sources are from dist tarball
    if test -f src/actions/libfmactions_la_vala.stamp; then
        AM_PROG_VALAC
    else
        AM_PROG_VALAC([0.13.0])
        if test x"$VALAC" = xvalac; then
            AC_ERROR([No Vala compiler found but it is required.])
        fi
    fi
    ACTIONS_SUBDIR='actions'
    AC_DEFINE(HAVE_OLD_ACTIONS, 1, [Define to 1 to enable old custom actions code])
else
    ACTIONS_SUBDIR=
fi
AC_SUBST(ACTIONS_SUBDIR)
AM_CONDITIONAL(HAVE_OLD_ACTIONS, [test x$ACTIONS_SUBDIR = xactions])

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
dnl AC_FUNC_MMAP
AC_SEARCH_LIBS([pow], [m])
AC_SEARCH_LIBS(dlopen, dl)

# Large file support
AC_ARG_ENABLE([largefile],
    AS_HELP_STRING([--enable-largefile],
        [enable Large file support (default: yes)]),
    largefile=$enableval, largefile="yes")

if test x"$largefile" = x"yes"; then
  CPPFLAGS="$CPPFLAGS -D_LARGEFILE_SOURCE -D_REENTRANT -D_FILE_OFFSET_BITS=64"
  AC_DEFINE(_LARGEFILE64_SOURCE, 1, [Whether to enable large file support])
  AC_DEFINE(_FILE_OFFSET_BITS, 64, [File offset bits])
fi


# intltool
IT_PROG_INTLTOOL([0.40.0])

dnl libgtk version
AC_ARG_WITH(gtk,
    [AC_HELP_STRING([--with-gtk=VER],
        [choose Gtk+ version for libfm-gtk build (default=2)])
AC_HELP_STRING([--without-gtk],
        [disable libfm-gtk build])],
    [ac_with_gtk="${withval}"],
    [ac_with_gtk=2]
)
AM_COND_IF(EXTRALIB_ONLY,
    [ac_with_gtk=no])

# modules required
if test x"$enable_actions" = xyes; then
  gio_modules="gio-unix-2.0 >= 2.26.0 glib-2.0 >= 2.26.0"
else
  gio_modules="glib-2.0 >= 2.22.0 gio-unix-2.0"
fi
gio_modules="$gio_modules gthread-2.0 gobject-2.0"

PKG_CHECK_MODULES(GIO, [$gio_modules])
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

if test x"$ac_with_gtk" != xno; then
  case "$ac_with_gtk" in
    3|3.*)
      pkg_modules="$gio_modules gtk+-3.0 pango >= 1.16.0 cairo >= 1.8.0"
      LIBFM_GTK_LTLIBRARIES=libfm-gtk3.la
      LIBFM_GTK_PC=libfm-gtk3.pc
      ac_with_gtk="3.0"
      ;;
    *)
      pkg_modules="$gio_modules gtk+-2.0 >= 2.18.0 pango >= 1.16.0 cairo >= 1.8.0"
      LIBFM_GTK_LTLIBRARIES=libfm-gtk.la
      LIBFM_GTK_PC=libfm-gtk.pc
      ac_with_gtk="2.0"
      ;;
  esac
  PKG_CHECK_MODULES(GTK, [$pkg_modules])
  LIBFM_PREF_APPS="libfm-pref-apps lxshortcut"
else
  dnl automake uses GTK_CFLAGS for glib-compat.c compilation
  GTK_CFLAGS="${GIO_CFLAGS}"
  GTK_LIBS=
  LIBFM_GTK_LTLIBRARIES=
  LIBFM_GTK_PC=
  ac_with_gtk="none"
  LIBFM_PREF_APPS=
fi
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(LIBFM_GTK_LTLIBRARIES)
AC_SUBST(LIBFM_PREF_APPS)
AC_SUBST(LIBFM_GTK_PC)
AM_CONDITIONAL([ENABLE_GTK], [test x$ac_with_gtk != xnone])

dnl pango_modules="pango >= 1.16.0"
dnl PKG_CHECK_MODULES(PANGO, [$pango_modules])
dnl AC_SUBST(PANGO_CFLAGS)
dnl AC_SUBST(PANGO_LIBS)

dnl gmodule may be used for libfm-chooser-hack.la
#gmodule_modules="gmodule-2.0"
#PKG_CHECK_MODULES(GMODULE, [$gmodule_modules])
#AC_SUBST(GMODULE_CFLAGS)
#AC_SUBST(GMODULE_LIBS)

AM_COND_IF(EXTRALIB_ONLY,
    [MENU_CACHE_CFLAGS=
    MENU_CACHE_LIBS=],
    [menu_modules="libmenu-cache >= 0.3.2"
    PKG_CHECK_MODULES(MENU_CACHE, [$menu_modules])])
AC_SUBST(MENU_CACHE_CFLAGS)
AC_SUBST(MENU_CACHE_LIBS)

dnl check for menu-cache versions 0.4.x since no macro MENU_CACHE_CHECK_VERSION
dnl is available in those versions
LIBS_save="${LIBS}"
LIBS="${LIBS} ${MENU_CACHE_LIBS}"
AC_CHECK_FUNCS(menu_cache_dir_list_children)
LIBS="${LIBS_save}"

# special checks for glib/gio 2.27 since it contains backward imcompatible changes.
# glib 2.26 uses G_DESKTOP_APP_INFO_LOOKUP_EXTENSION_POINT_NAME extension point while
# glib 2.27 uses x-scheme-handler/* mime-type to register handlers.
PKG_CHECK_MODULES(GLIB2_27, [glib-2.0 >= 2.27.0],
    [have_scheme_handler="yes"],
    [have_scheme_handler="no"])
AM_CONDITIONAL(HAVE_SCHEME_HANDLER, test "x$have_scheme_handler" = "xyes")
if test "x$have_scheme_handler" = "xyes"; then
    AC_DEFINE_UNQUOTED(HAVE_SCHEME_HANDLER, [1], [Have x-scheme/uri handler support])
fi

#optional features

AC_ARG_ENABLE(udisks,
    [AC_HELP_STRING([--enable-udisks],
        [build libfm with udisks support (Linux only) @<:@default=yes@:>@])],
    [enable_udisks="${enableval}"],
    [enable_udisks=no]
)
AM_COND_IF(EXTRALIB_ONLY,
    [enable_udisks=no])
AM_CONDITIONAL(ENABLE_UDISKS, test x"$enable_udisks" = x"yes")
if test x"$enable_udisks" = x"yes"; then
    # turn on udisks support
    AC_DEFINE_UNQUOTED(USE_UDISKS, [1], [Enable UDisks support])

    # test for availability of dbus
    dbus_modules="dbus-glib-1"
    PKG_CHECK_MODULES(DBUS, [$dbus_modules])
    AC_SUBST(DBUS_CFLAGS)
    AC_SUBST(DBUS_LIBS)
fi

AC_ARG_ENABLE([exif],
    AS_HELP_STRING([--disable-exif],
        [disable libexif which is used for faster thumbnail loading.]),
    [enable_exif="${enableval}"],
    [enable_exif=auto]
)
AM_COND_IF(EXTRALIB_ONLY,
    [enable_exif=no])
AM_CONDITIONAL(ENABLE_EXIF, test x"$enable_exif" != x"no")
AS_IF([test x"$enable_exif" != x"no"], [
    # test for availability of libexif
    exif_modules="libexif"
    PKG_CHECK_MODULES(EXIF, [$exif_modules],
        [# turn on libexif support
        enable_exif=yes
        AC_DEFINE_UNQUOTED(USE_EXIF, [1], [Enable libexif])
        AC_SUBST(EXIF_CFLAGS)
        AC_SUBST(EXIF_LIBS)],
        [AS_IF([test x"$enable_exif" = x"auto"], [enable_exif=no], [
            AC_ERROR([Package requirements (libexif) were not met:

$EXIF_PKG_ERRORS
])])])])

#check for gtk-doc
GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

AC_ARG_ENABLE(demo,
    [AC_HELP_STRING([--enable-demo],
        [build libfm-demo file manager @<:@default=no@:>@])],
    [enable_demo="${enableval}"],
    [enable_demo=no]
)
AM_COND_IF(EXTRALIB_ONLY,
    [enable_demo=no])
AM_CONDITIONAL(ENABLE_DEMO, test x"$enable_demo" = x"yes")

# GIO extension module
giomodulesdir=$libdir/gio/modules
AC_SUBST(giomodulesdir)


GETTEXT_PACKAGE=libfm
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
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

AC_SUBST(FMLIBVER)
if test `eval "echo $includedir"` = /usr/include
then
    FMINCLUDEADD=''
else
    FMINCLUDEADD='-I${includedir}'
fi
AC_SUBST(FMINCLUDEADD)

AC_ARG_ENABLE(debug,
    [AC_HELP_STRING([--enable-debug],
        [build libfm with debug support @<:@default=no@:>@])],
    [enable_debug="${enableval}"],
    [enable_debug=no]
)
if test "$enable_debug" = "yes"; then
    # turn on debug and disable optimization
    CPPFLAGS="$CPPFLAGS -DG_ENABLE_DEBUG -O0 -g"
    case "$CC" in
    gcc*)
        CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
        ;;
    *)
        ;;
    esac
    dnl Be more strict on portability
    CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=700"
else
    # turn off glib debug checks
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

dnl Check linker flags for modules
AC_CHECK_LDFLAG(ac_cv_ld_z_defs, [-Wl,-z,defs], LDFLAGS)

dnl Check if OS respects POSIX.1-2001 `environ' declaration
AC_CHECK_DECLS([environ], [], [], [[#include <unistd.h>]])

dnl Fix invalid sysconfdir when --prefix=/usr
if test `eval "echo $sysconfdir"` = /usr/etc
then
    sysconfdir=/etc
fi

dnl Make version subfields for FM_CHECK_VERSION macro
ac_version_subst_str=`echo $VERSION | awk -F '.' '/.*/ { printf "FM_VERSION_MAJOR=%d FM_VERSION_MINOR=%d FM_VERSION_MICRO=%d", $1, $2, $3 }'`
eval ${ac_version_subst_str}
AC_SUBST(FM_VERSION_MAJOR)
AC_SUBST(FM_VERSION_MINOR)
AC_SUBST(FM_VERSION_MICRO)

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/actions/Makefile
    src/tests/Makefile
    src/modules/Makefile
    src/fm-version.h
    data/Makefile
    data/libfm-pref-apps.1
    data/lxshortcut.1
    data/ui/Makefile
    po/Makefile.in
    docs/Makefile
    docs/reference/Makefile
    docs/reference/libfm/Makefile
    docs/reference/libfm/version.xml
    libfm.pc
    libfm-gtk.pc
    libfm-gtk3.pc
    libfm-extra.pc
])

AC_OUTPUT

echo
echo "Libfm $VERSION Configuration Summary:"
echo
echo "prefix:                                                 $prefix"
echo "sysconfdir:                                             $sysconfdir"
echo "Enable compiler flags and other support for debugging:  $enable_debug"
echo "Build udisks support (Linux only, experimental):        $enable_udisks"
echo "Build with libexif for faster thumbnail loading:        $enable_exif"
echo "Build demo program src/demo/libfm-demo:                 $enable_demo"
echo "Build old custom actions API (requires Vala):           $enable_actions"
echo "Large file support:                                     $largefile"
if test x"$have_scheme_handler" = x"no"; then
echo "GIO module for preferred apps (for glib < 2.28 only):   $giomodulesdir"
else
echo "GIO module for preferred apps (for glib < 2.28 only):   not required"
fi
echo "Build libfm-gtk for Gtk+ version:                       $ac_with_gtk"
echo "Build API doc with gtk-doc (recommended for make dist): $enable_gtk_doc"
echo
echo

if test x"$sysconfdir" != x'/etc'; then
    echo
    echo
    echo 'Warning: sysconfdir is not /etc.'
    echo 'Please consider passing --sysconfdir=/etc to configure.';
    echo 'Otherwise default config files will be installed to wrong place.'
    echo
    echo
fi
