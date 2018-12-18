#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([CUPDI], [1.0], [rocup.wan@rocup.wan@microchip.com])
AM_INIT_AUTOMAKE
#Generate static lib
#AC_PROC_RANLIB
AC_CONFIG_SRCDIR([cupdi.c])
AC_CONFIG_HEADERS([config.h])
AC_PROG_RANLIB

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memmove memset strerror strtol])

AC_CONFIG_FILES([Makefile
                 argparse/Makefile
		 crc/Makefile
                 device/Makefile
		 file/Makefile
                 ihex/Makefile
                 os/linux/Makefile
		 regex/Makefile
                 string/Makefile
                 updi/Makefile])
AC_OUTPUT
