AUTOMAKE_OPTIONS = foreign
SUBDIRS = argparse crc device file ihex os/linux regex string updi infoblock

bin_PROGRAMS = cupdi
cupdi_SOURCES = cupdi.c
cupdi_LDADD = argparse/libargparse.a crc/libcrc.a device/libdevice.a file/libfile.a ihex/libihex.a regex/libre.a string/libstring.a updi/libupdi.a os/linux/libos.a infoblock/libinfoblock.a
include_HEADERS = cupdi.h
#AM_CPPFLAGS = os/platform.h
#cupdi_CFLAGS = -static
