#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# ===========================================================================
# aenig4, Enigma M4 cipher machine emulator.
# ===========================================================================

AC_INIT([aenig4],
	[1.04],
	[jorge.giner@hotmail.com],
	aenig4,
	[https://jorgicor.niobe.org/aenig4])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(-Wall -Werror -Wportability subdir-objects
		 color-tests parallel-tests)
AC_CONFIG_HEADERS([config.h])

AC_DEFINE([COPYRIGHT_YEARS], ["2018"], [Years of copyright])
AC_SUBST(COPYRIGHT_YEARS, [2018])

# Checks for programs.
AC_PROG_CC
AM_MISSING_PROG(HELP2MAN, help2man, $missing_dir)

# Supported flags.
AX_CHECK_COMPILE_FLAG([-Wall], [WARN_CFLAGS="-Wall"], [], [])
AX_CHECK_COMPILE_FLAG([-Wdeclaration-after-statement],
	      [WARN_CFLAGS="$WARN_CFLAGS -Wdeclaration-after-statement"],
	      [], [])
AC_SUBST(WARN_CFLAGS)

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
		 doc/Makefile
		 src/Makefile])
AC_OUTPUT
