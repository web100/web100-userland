bin_PROGRAMS = gutil
RC_FILES = web100.2.rc web100.1.rc lcl.rc rmt.rc
IMAGE_FILES = state_w2.gif display_w2.gif triage_w2.gif list_w2.gif sndtuner_w2.gif rcvtuner_w2.gif

INCLUDES = @STRIP_BEGIN@ \
	@GTK_CFLAGS@ \
	-I$(top_srcdir)/lib \
	-DWEB100_CONF_DIR=\"$(WEB100_CONF_DIR)\" \
	-DWEB100_IMAGE_DIR=\"$(WEB100_IMAGE_DIR)\" \
	@STRIP_END@

gutil_LDADD = \
	$(top_builddir)/lib/libweb100.la \
	@GTK_LIBS@
gutil_SOURCES = \
	gutil.c \
	connectioninfo.c \
	connectioninfo.h \
	connectioninfo-int.h \
	connectionlist.c \
	connectionlist.h \
	connectionselect.c \
	connectionselect.h \
	display.c \
	display.h \
	rcvtuner.c \
	rcvtuner.h \
	sndtuner.c \
	sndtuner.h \
	toolbox.c \
	toolbox.h \
	triage.c \
	triage.h \
	utillaunch.c \
	utillaunch.h \
	variablelist.c \
	variablelist.h \
	wcgraph.c \
	wcgraph.h \
	wcpie.c \
	wcpie.h \
	web100poll.c \
	web100poll.h \
	web100object.c \
	web100object.h \
	web100widget.c \
	web100widget.h

confdir = $(WEB100_CONF_DIR)
conf_DATA = $(RC_FILES)

imagedir = $(WEB100_IMAGE_DIR)
image_DATA = $(IMAGE_FILES)

EXTRA_DIST = $(RC_FILES) $(IMAGE_FILES)
