INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir) \
	@STRIP_END@

# Libraries to compile and install
lib_LTLIBRARIES = libweb100.la

# Version info
libweb100_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-export-dynamic \
	@STRIP_END@

# Header files for public installation
web100_pub_h_sources = \
	web100.h

# Header files not installed
web100_pri_h_sources = \
	web100-int.h

# C sources to build the library from
web100_c_sources = \
	web100.c

libweb100_la_SOURCES = $(web100_c_sources)

# Install headers into include/web100-<version>/web100
libweb100includedir = $(includedir)/$(PACKAGE)-$(VERSION)/$(PACKAGE)
libweb100include_HEADERS = $(web100_pub_h_sources)

# Make sure private headers are included as part of 'make dist'
EXTRA_DIST += $(web100_pri_h_sources)
