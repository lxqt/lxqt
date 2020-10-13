dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2.1 of the License, or (at your option) any later version.
dnl
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

dnl AC_CHECK_LDFLAG(CACHEVAR, FLAGS, APPENDVAR)
AC_DEFUN([AC_CHECK_LDFLAG],
[AC_CACHE_CHECK([whether the target compiler accepts $2], $1,
    [$1=no
    echo 'int main(){return 0;}' > conftest.c
    if test -z "`${CC} $2 -o conftest conftest.c 2>&1`"; then
        $1=yes
    else
        $1=no
    fi
    rm -f conftest*
    ])
if test x${$1} = xyes; then $3="$2 ${$3}"; fi
])
