AC_INIT([xwm], [20201209], [chrsmeca@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
AC_PROG_CC
AC_PATH_XTRA
AS_IF([test "X$no_x" = "Xyes"],
  [AC_MSG_ERROR([Could not find X11])])
AC_OUTPUT
