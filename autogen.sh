#!/bin/sh

# autogen.sh: Automatically generate the files necessary to build using
# autoconf.  This must be run on a fresh checkout from CVS and also if any
# autoconf files (configure.in, etc.) are updated.
aclocal
libtoolize --force
autoheader
automake --add-missing --include-deps --foreign
autoconf
