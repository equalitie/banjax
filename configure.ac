#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#AC_PREREQ([2.69])
AC_INIT(banjax, 0.2, vmon@equalit.ie)
AC_CONFIG_SRCDIR([src/banjax.cpp])
AM_INIT_AUTOMAKE()


#libtool
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AC_PROG_LIBTOOL()

#AC_CONFIG_HEADERS([config.h])
AC_LANG([C++])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
# We require version 1.0.1 for SHA256 support.
PKG_CHECK_MODULES([libcrypto], [libcrypto >= 1.0.1])
# configuration reader
PKG_CHECK_MODULES([libconfig], [libconfig++ >= 1.4.8])
# 0mq pubsub
PKG_CHECK_MODULES([libzmq], [libzmq >= 2.2.0])
# fast regex recognition
#PKG_CHECK_MODULES([libre], [libre2 >= 2.0.0])
#check for lib yaml-cpp
#PKG_CHECK_MODULES([libyaml], [libyaml-cpp >= 0.5.1])

LIBS="$libcrypto_LIBS $libconfig_LIBS $libzmq_LIBS $libre2_LIBS $libyaml_LIBS"

lib_CPPFLAGS="$libcrypto_CFLAGS $libconfig_CFLAGS"
AC_SUBST(lib_CPPFLAGS)

lib_LIBS="$LIBS"
LIBS=
AC_SUBST(lib_LIBS)
AC_SUBST(lib_CPPFLAGS)

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h, ts/ts.h, ])

# Checks for typedefs, structures, and compiler characteristics.
# AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_CHECK_FUNCS([inet_ntoa memset socket])

#AC_CONFIG_FILES(config.h)
#AC_OUTPUT
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_CONFIG_SRCDIR([src/banjax.cpp])
#AC_CONFIG_HEADERS([config.h])
AC_LANG([C++])

#AC_CONFIG_FILES([Makefile])
AC_OUTPUT(Makefile src/Makefile doc/Makefile test/Makefile)

 
