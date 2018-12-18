AUTOMAKE_OPTIONS = foreign
SUBDIRS = argparse device ihex os/linux string updi

bin_PROGRAMS = cupdi
cupdi_SOURCES = cupdi.c
cupdi_LDADD = argparse/libargparse.a device/libdevice.a ihex/libihex.a string/libstring.a updi/libupdi.a os/linux/libos.a
include_HEADERS = cupdi.h
#AM_CPPFLAGS = os/platform.h
#cupdi_CFLAGS = -static
