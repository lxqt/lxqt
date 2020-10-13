#! /bin/sh
AC_VERSION=

test -n "$SRC_DIR" || SRC_DIR=$(dirname "$0")
test -n "$SRC_DIR" || SRC_DIR=.

OLD_DIR=$(pwd)
cd "$SRC_DIR"

AUTOMAKE=${AUTOMAKE:-automake}
AM_INSTALLED_VERSION=$($AUTOMAKE --version | sed -e '2,$ d' -e 's/.* \([0-9]*\.[0-9]*\).*/\1/')

# FIXME: we need a better way for version check later.
case "$AM_INSTALLED_VERSION" in
    1.1[0-9])
	;;
    *)
	echo
	echo "You must have automake 1.10 or newer installed."
	echo "Install the appropriate package for your distribution,"
	echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
	exit 1
	;;
esac

if [ "x${ACLOCAL_DIR}" != "x" ]; then
  ACLOCAL_ARG="-I ${ACLOCAL_DIR}"
fi

test -d m4 || mkdir m4

if gtkdocize --copy; then
    echo "Files needed by gtk-doc are generated."
else
    echo "You need gtk-doc to build this package."
    echo "http://www.gtk.org/gtk-doc/"
    exit 1
fi

set -x

${ACLOCAL:-aclocal$AM_VERSION} ${ACLOCAL_ARG}
${AUTOHEADER:-autoheader$AC_VERSION} --force
AUTOMAKE=$AUTOMAKE libtoolize -c --automake --force
$AUTOMAKE --add-missing --copy --include-deps
${AUTOCONF:-autoconf$AC_VERSION}

rm -rf autom4te.cache

if test -n "$DOCONFIGURE"; then
    ./configure $@
fi

cd "$OLD_DIR"
