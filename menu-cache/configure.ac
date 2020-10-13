dnl Process this file with autoconf to produce a configure script.

AC_INIT([menu-cache], [1.1.0],
        [http://lxde.org/])

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([no-dist-gzip dist-xz])
AM_CONFIG_HEADER(config.h)

# Support silent build rules. Disable by either passing --disable-silent-rules
# to configure or passing V=1 to make
AM_SILENT_RULES([yes])

AM_MAINTAINER_MODE([enable])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
dnl AC_ARG_PROGRAM
AM_PROG_LIBTOOL

dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS")

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.16.0 gio-2.0 >= 2.15.2)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(LIBFM_EXTRA, libfm-extra)
AC_SUBST(LIBFM_EXTRA_CFLAGS)
AC_SUBST(LIBFM_EXTRA_LIBS)

AC_ARG_ENABLE(more_warnings,
       [AC_HELP_STRING([--enable-more-warnings],
               [Add more warnings @<:@default=no@:>@])],
       [enable_more_warnings="${enableval}"],
       [enable_more_warnings=no]
)

if test x"$enable_more_warnings" = x"yes"; then
  ADDITIONAL_FLAGS="-Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-unused-parameter -Wmissing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Werror=inline -Wformat-nonliteral -Wformat-security -Winit-self -Wmissing-include-dirs -Wundef -Waggregate-return -Wmissing-format-attribute -Wnested-externs -fno-strict-aliasing -fmessage-length=0 -Wp,-D_FORTIFY_SOURCE=2 -DG_DISABLE_DEPRECATED -DG_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_SINGLE_INCLUDES"
fi
AC_SUBST(ADDITIONAL_FLAGS)

dnl --enable-debug=(yes|minimum|no)
AC_ARG_ENABLE(debug, [  --enable-debug=[no/yes]   turn on debugging [[default=no]]],,enable_debug=no)
if test "$enable_debug" = "yes"; then
  DEBUG_CFLAGS="-DG_ENABLE_DEBUG"
else
  if test "x$enable_debug" = "xno"; then
    DEBUG_CFLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS"
  else
    DEBUG_CFLAGS=""
  fi
fi
AC_SUBST(DEBUG_CFLAGS)

dnl check for gtk-doc
m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.14],[--flavour no-tmpl])
],[
AM_CONDITIONAL([ENABLE_GTK_DOC], false)
])

dnl Make version subfields for MENU_CACHE_CHECK_VERSION macro
ac_version_subst_str=`echo $VERSION | awk -F '.' '/.*/ { printf "MC_VERSION_MAJOR=%d MC_VERSION_MINOR=%d MC_VERSION_MICRO=%d", $1, $2, $3 }'`
eval ${ac_version_subst_str}
AC_SUBST(MC_VERSION_MAJOR)
AC_SUBST(MC_VERSION_MINOR)
AC_SUBST(MC_VERSION_MICRO)

exec_prefix_save="$exec_prefix"
test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"
if test `eval echo "$libdir"` = '/usr/lib'
then
    PCFILE_LIBDIR=
else
    PCFILE_LIBDIR=' -L${libdir}'
fi
exec_prefix="$exec_prefix_save"
AC_SUBST(PCFILE_LIBDIR)

AC_OUTPUT([
Makefile
libmenu-cache/Makefile
libmenu-cache/libmenu-cache.pc
libmenu-cache/menu-cache.h
menu-cache-gen/Makefile
menu-cache-daemon/Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/libmenu-cache/Makefile
])
