# Header files for public installation
web100_pub_h_sources = \
	web100.h

# Header files not installed
web100_pri_h_sources = \
	web100-int.h

# C sources to build the library from
web100_c_sources = \
	web100.c

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir) \
	@STRIP_END@

# Libraries to compile and install
lib_LTLIBRARIES = libweb100.la
libweb100_la_LDFLAGS = @STRIP_BEGIN@ \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-export-dynamic \
	@STRIP_END@
libweb100_la_SOURCES = $(web100_c_sources) $(web100_pub_h_sources) $(web100_pri_h_sources)

# Install headers into include/web100
# NOTE: If you update this, be sure to update web100-config
libweb100includedir = $(WEB100_INCLUDE_DIR)/web100
libweb100include_HEADERS = $(web100_pub_h_sources)
