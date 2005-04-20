# web100.m4: Configure paths for Web100.  Based off of Owen Taylor's
# gtk.m4 from GTK+-1.2.10.  Requires autoconf-2.5x.
#
# $Id: web100.m4,v 1.1 2005/04/20 18:31:35 jestabro Exp $

dnl AM_PATH_WEB100([EXACT-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for Web100, and define WEB100_CFLAGS and WEB100_LIBS
dnl
AC_DEFUN(AM_PATH_WEB100,
[
web100_success=""

AC_PATH_PROG([WEB100_CONFIG], [web100-config], [no])

AC_MSG_CHECKING(for Web100)

if test "$WEB100_CONFIG" != "no"; then
  WEB100_CFLAGS=`$WEB100_CONFIG --cflags`
  WEB100_LIBS=`$WEB100_CONFIG --libs`
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  echo "*** The web100-config script installed by Web100 could not be found"
  echo "*** If Web100 was installed in PREFIX, make sure PREFIX/bin is in"
  echo "*** your path, or set the WEB100_CONFIG environment variable to the"
  echo "*** full path to web100-config"
  web100_success="no"
fi

if test x$web100_success = x; then
  if test "x$1" != "x"; then
    AC_MSG_CHECKING(for Web100 - version $1)

    WEB100_VERSION=`$WEB100_CONFIG --version`
    if test "$WEB100_VERSION" = "$1"; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
      echo "*** The requested ($1) and installed ($WEB100_VERSION) versions"
      echo "*** of Web100 do not match."
      web100_success="no"
    fi
  fi
fi

if test x$web100_success = x; then
  web100_success="yes"
fi

if test x$web100_success = xyes; then
  m4_if([$2], [], [:], [$2])
else
  WEB100_CFLAGS=""
  WEB100_LIBS=""
  m4_if([$3], [], [:], [$3])
fi

AC_SUBST(WEB100_CFLAGS)
AC_SUBST(WEB100_LIBS)
])
