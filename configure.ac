#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ctests], [1.0], [https://github.com/serge-v])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([asyncread.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lodbc':
AC_CHECK_LIB([odbc], [main])
# FIXME: Replace `main' with a function in `-lrt':

# Checks for header files.
AC_HEADER_RESOLV
AC_CHECK_HEADERS([\
arpa/inet.h \
fcntl.h limits.h malloc.h memory.h netdb.h netinet/in.h \
stdint.h stdlib.h string.h \
sys/ioctl.h \
sys/socket.h \
sys/time.h \
sys/epoll.h \
sys/event.h \
unistd.h \
])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_CHECK_FUNCS([bzero gettimeofday inet_ntoa memset mkfifo socket strchr strdup strtol strtoul tzset])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

