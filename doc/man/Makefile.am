dist-hook:
	doctext -mpath $(distdir) -quotefmt $(top_srcdir)/lib/*.[ch]

autogen_manpages := $(wildcard *.3)
normal_manpages := web100-config.1

man_MANS := $(autogen_manpages) $(normal_manpages)

EXTRA_DIST += $(normal_manpages)
