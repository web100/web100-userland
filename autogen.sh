#!/bin/sh

aclocal
libtoolize --force
autoheader
automake --add-missing --include-deps --foreign
autoconf
