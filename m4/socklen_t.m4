AC_DEFUN(AC_TYPE_SOCKLEN_T,
  [
    AC_CACHE_CHECK(
      [for socklen_t],
      ac_cv_type_socklen_t,
      [
        AC_TRY_COMPILE(
          [#include <sys/socket.h>],
          [socklen_t len = 42; return len;],
          ac_cv_type_socklen_t=yes,
          ac_cv_type_socklen_t=no)
      ]
    )
    if test $ac_cv_type_socklen_t != yes; then
      AC_DEFINE([socklen_t], [int],
                [Redefine socklen_t to the correct type for the system])
    fi
  ]
)
