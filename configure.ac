AC_PREREQ(2.52)
AC_INIT(web100_userland, 1.5)

dnl libweb100 libtool versioning
LT_CURRENT=3
LT_REVISION=0
LT_AGE=3

AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

dnl Web100 versioning
WEB100_VERSION=$PACKAGE_VERSION
AC_SUBST(WEB100_VERSION)

dnl Initialize automake
AM_INIT_AUTOMAKE

dnl Specify config header
AM_CONFIG_HEADER(config.h)

dnl Check for programs
AC_CANONICAL_HOST
AC_PROG_CC
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PYTHON_DEVEL
AC_PROG_SWIG(1.3.17)
SWIG_MULTI_MODULE_SUPPORT
SWIG_PYTHON

dnl Initialize libtool
AM_PROG_LIBTOOL

dnl Config options
AC_ARG_ENABLE(warnings,
              AC_HELP_STRING([--enable-warnings],
                             [turn on compile warnings]),
              enable_warnings=$enableval,
              enable_warnings=no)
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],
                             [turn on debugging compiler options]),
              enable_debug=$enableval,
              enable_debug=no)
AC_ARG_ENABLE(gtk-tools,
              AC_HELP_STRING([--disable-gtk-tools],
                             [do not build the GTK+-based testing tools]),
              enable_gtk_tools=$enableval,
              enable_gtk_tools=yes)
AC_ARG_ENABLE(gtk2,
              AC_HELP_STRING([--disable-gtk2],
                             [do not build the GTK2-based testing tools]),
              enable_gtk2=$enableval,
              enable_gtk2=yes)
AC_ARG_ENABLE(python,
              AC_HELP_STRING([--enable-python],
		             [build python module]),
	      enable_python=$enableleval,
	      enable_python=yes)

dnl Define directories
WEB100_DOC_DIR=${prefix}/doc/web100
WEB100_CONF_DIR=${sysconfdir}
WEB100_INCLUDE_DIR=${includedir}
WEB100_IMAGE_DIR=${datadir}/web100

AC_SUBST(WEB100_DOC_DIR)
AC_SUBST(WEB100_CONF_DIR)
AC_SUBST(WEB100_INCLUDE_DIR)
AC_SUBST(WEB100_IMAGE_DIR)

dnl Checks for libraries

dnl - GTK
build_gtk_tools="no"
if test "x$enable_gtk_tools" = "xyes" ; then
    if test "x$enable_gtk2" = "xyes" ; then
        AM_PATH_GTK_2_0(2.0.0, [have_gtk2="yes"], [have_gtk2="no"])
        if test "x$have_gtk2" = "xyes" ; then
            AC_SUBST(GTK_LIBS)
            AC_SUBST(GTK_CFLAGS)
            AC_DEFINE([GTK2], 1,
                      [Define if GTK2 libs are found on system.])
            build_gtk_tools="yes"
        else
            AC_MSG_WARN([GTK2 not found.])
            AM_PATH_GTK(1.2.0, [have_gtk="yes"], [have_gtk="no"])
            if test "x$have_gtk" = "xyes" ; then
                AC_SUBST(GTK_LIBS)
                AC_SUBST(GTK_CFLAGS)
                build_gtk_tools="yes"
            else
                AC_MSG_WARN([GTK+ not found.  The GUI tools will not be built.])
            fi 
        fi
    else 
        AM_PATH_GTK(1.2.0, [have_gtk="yes"], [have_gtk="no"])
        if test "x$have_gtk" = "xyes" ; then
            AC_SUBST(GTK_LIBS)
            AC_SUBST(GTK_CFLAGS)
            build_gtk_tools="yes"
        else
            AC_MSG_WARN([GTK+ not found.  The GUI tools will not be built.])
        fi
    fi
fi
AM_CONDITIONAL(BUILD_GTK_TOOLS, [test "x$build_gtk_tools" = "xyes"])


dnl Checks for header files
AC_CHECK_HEADER(malloc.h,
                [AC_DEFINE([HAVE_MALLOC_H], 1,
                           [Define if malloc.h is found in the system.])],
                [])

dnl Checks for typedefs
AC_TYPE_SOCKLEN_T

dnl Checks for structures
dnl Checks for compiler characteristics

if ! test "x$GCC" = "xyes"; then
    AC_MSG_ERROR(compiling Web100 requires GCC)
fi

if test "x$enable_debug" = "xyes"; then
    if test -z "`echo $CFLAGS | grep "\-g" 2> /dev/null`"; then
        CFLAGS="$CFLAGS -g"
    fi
    CFLAGS=`echo $CFLAGS | sed -e 's/-O2//'`
    AC_DEFINE([DEBUG], 1,
              [Define to enable debugging when building.])
else
    AC_DEFINE([NDEBUG], 1,
              [Define to disable debugging (including asserts) when building.])
fi

if test "x$enable_warnings" = "xyes"; then
    for i in -Wall -Wtraditional -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes; do
        if test -z "`echo $CFLAGS | grep "$i" 2> /dev/null`"; then
            CFLAGS="$CFLAGS $i"
        fi
    done
fi

build_python="no"
if test x$enable_python = xyes ; then
    AM_PATH_PYTHON(1.6)
fi
AM_CONDITIONAL(BUILD_PYTHON, [test x$enable_python = xyes])


dnl Checks for system services

dnl GNU make allows us to use the $(strip ...) builtin which eliminates a
dnl large amount of extra whitespace in compile lines.
AC_MSG_CHECKING(whether make is GNU make)
STRIP_BEGIN=
STRIP_END=
if $ac_make --version 2> /dev/null | grep '^GNU Make ' > /dev/null ; then
    STRIP_BEGIN='$(strip $(STRIP_DUMMY)'
    STRIP_END=')'
    AC_MSG_RESULT(yes)
else
    AC_MSG_RESULT(no)
fi
dnl some Make 3.79 $(strip ) versions are broken and require an empty arg
STRIP_DUMMY=
AC_SUBST(STRIP_DUMMY)
AC_SUBST(STRIP_BEGIN)
AC_SUBST(STRIP_END)

AC_CONFIG_FILES([
Makefile
doc/Makefile
doc/man/Makefile
doc/man/man1/Makefile
doc/man/man3/Makefile
doc/man/man7/Makefile
lib/Makefile
python/Makefile
util/Makefile
util/scripts/Makefile
util/gui/Makefile
])
AC_OUTPUT
