AM_CPPFLAGS =					\
	$(XORG_INCS)				\
	-I$(srcdir)/../ddc			\
	-I$(srcdir)/../ramdac			\
	-I$(srcdir)/../i2c			\
	-I$(srcdir)/../parser			\
	-I$(srcdir)/../modes

libxwayland_la_LTLIBRARIES = libxwayland.la
AM_CFLAGS = \
	-DHAVE_XORG_CONFIG_H \
	@DIX_CFLAGS@ @XORG_CFLAGS@ @XWAYLAND_CFLAGS@ \
	-I$(top_srcdir)/hw/xfree86/common \
	-I$(top_srcdir)/hw/xfree86/os-support/bus

libxwayland_la_LDFLAGS = -module -avoid-version @XWAYLAND_LIBS@
libxwayland_ladir = $(moduledir)/extensions
libxwayland_la_SOURCES =			\
	xwayland.c				\
	xwayland-input.c			\
	xwayland-output.c			\
	xwayland-cursor.c			\
	xwayland-window.c			\
	xwayland-private.h			\
	drm-client-protocol.h			\
	drm-protocol.c				\
	xserver-client-protocol.h		\
	xserver-protocol.c

if DRM
AM_CFLAGS += @LIBDRM_CFLAGS@
libxwayland_la_LDFLAGS += @LIBDRM_LIBS@
libxwayland_la_SOURCES += xwayland-drm.c
endif

sdk_HEADERS = xwayland.h

BUILT_SOURCES =					\
	drm-client-protocol.h			\
	drm-protocol.c				\
	xserver-client-protocol.h		\
	xserver-protocol.c

CLEANFILES = $(BUILT_SOURCES)

@wayland_scanner_rules@
