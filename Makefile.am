AUTOMAKE_OPTIONS = foreign # Ignore need for README/AUTHORS... etc

AM_CFLAGS=-Wall

# Include buildinc target
include $(top_srcdir)/buildinc.mk

bin_PROGRAMS=fb_udp fb_flash_util fb_reflector

fb_udp_SOURCES=fb_udp.c ver.h

fb_flash_util_SOURCES=fb_flash_util.c seed.c

fb_reflector_SOURCES=fb_reflector.c

fb_udp_LDADD = @LIBOBJS@ @LIBFB@ @LIBPCAP@ @LIBNET@ @LIBREADLINE@
fb_flash_util_LDADD = @LIBOBJS@ @LIBFB@ @LIBPCAP@ @LIBNET@
fb_reflector_LDADD = @LIBOBJS@


