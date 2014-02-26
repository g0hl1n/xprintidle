#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.62)
AC_INIT([xprintidle], [0.2], [henoch@dtek.chalmers.se])
AM_INIT_AUTOMAKE([dist-bzip2 -Wall])
AC_CONFIG_SRCDIR([xprintidle.c])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for header files.
AC_PATH_X
if test "$no_x" = yes; then
  AC_MSG_ERROR([X not found])
fi
AC_SUBST([x_includes])
AC_SUBST([x_libraries])

# Checks for libraries.
user_LIBS=$LIBS
LIBS="$LIBS -L$x_libraries"
AC_CHECK_LIB([X11], [XOpenDisplay], , [AC_MSG_ERROR([libX11 not found])])
AC_CHECK_LIB([Xss], [XScreenSaverQueryExtension], , [AC_MSG_ERROR([libXss not found])])
LIBS=$user_LIBS

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
