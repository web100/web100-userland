#!/bin/sh

# autogen.sh: Automatically generate the files necessary to build using
# autoconf.  This must be run on a fresh checkout from CVS and also if any
# autoconf files (configure.in, etc.) are updated.

aclocal -I m4 || exit 1
libtoolize --force || exit 1
autoheader || exit 1
automake --add-missing --include-deps --foreign || exit 1
autoconf || exit 1
