AC_INIT([xteetotaller],[0.1])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AC_CONFIG_FILES([Makefile])

AC_PROG_CC

AC_C_INLINE
AC_C_RESTRICT

AC_DEFINE(_XOPEN_SOURCE, 600, "Enable SUS v3 features")
AC_DEFINE(_CONST_X_STRING, 1, "Use const for X11 String type")

dnl DO NOT CHANGE THE ORDER OF LIBRARIES or expect a "BadWindow" error
AC_SEARCH_LIBS(XCreateWindow, X11)
AC_SEARCH_LIBS(XtOpenApplication, Xt)
AC_SEARCH_LIBS(XmuInternAtom, Xmu)
AC_SEARCH_LIBS(XmStringCreateLocalized, Xm)

dnl Add -std and -fno-strict-aliasing CFLAGS if compiler is GCC or compatible.
dnl Motif has several strict aliasing violations with struct pointer casts in
dnl callbacks.
if test x"$GCC" = x"yes"; then
        CFLAGS="$CFLAGS -std=c99 -fno-strict-aliasing"
fi

AC_OUTPUT
