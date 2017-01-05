include_HEADERS = config.h
SUBDIRS = src
EXTRA_DIST = $(subdirs) libphash.spec
ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = phash.pc

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: @LIBTOOL_DEPS@
	$(SHELL) ./config.status --recheck

dist-hook:
	rm -rf `find $(distdir) -name \.svn`
