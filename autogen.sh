#!/bin/sh

# autogen.sh: Automatically generate the files necessary to build using
# autoconf.  This must be run on a fresh checkout from CVS and also if any
# autoconf files (configure.in, etc.) are updated.

for i in m4 /usr/local/share/aclocal; do
    if [ -d $i ]; then
        ACLOCAL_OPTS="$ACLOCAL_OPTS -I $i"
    fi
done
aclocal $ACLOCAL_OPTS || exit 1

autoheader || exit 1
autoconf || exit 1

# Handle Mac OS X's non-standard naming of libtoolize
for i in libtoolize glibtoolize; do
    if which $i > /dev/null; then
        LIBTOOLIZE=$i
    fi
done
$LIBTOOLIZE --automake || exit 1

automake --add-missing --include-deps --foreign || exit 1
