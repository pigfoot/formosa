dnl Process this file with autoconf to produce a configure script.
AC_INIT(include/bbs.h)
AC_CONFIG_AUX_DIR(cfgaux)
AC_VALIDATE_CACHED_SYSTEM_TUPLE(rm -f config.cache)
AC_CANONICAL_SYSTEM
BBS_UID_DEFAULT=9999
BBS_GID_DEFAULT=999
BBS_TCP_PORT_DEFAULT=23

LDFLAGS="-g"

case ${target} in
    *sun-sunos*)
	systype=SunOS
	OS_DEF="-DSUNOS"
#	CC="cc"
	LIBS="-ltermcap"
	;;

    *solaris*)
	systype=Solaris
	OS_DEF="-DSOLARIS -DSYSV"
##	OS_DEF="-DSOLARIS -DSYSV -DTERMIOS"
#	CC="gcc"
#	CFLAGS="-traditional -O2"
##	CFLAGS="-O -I/usr/ucbinclude -traditional"
	CFLAGS="-O2 -Wunused"
	LIBS="-ltermcap -lsocket -lnsl"
##	LIBS="-ltermcap -lsocket -lnsl -lucb -L/usr/ucblib"
	;;

    *-freebsd*|*-netbsd*|*-bsdi*)
	systype=NetBSD_OR_FreeBSD
	OS_DEF="-DBSD44"
#	OS_DEF="-DBSD44 -DTERMIOS"
#	CC="cc"
#	CFLAGS=""
	LIBS="-ltermcap"
	;;

    *hp-hpux*)
	systype=HP_UX
	OS_DEF="-DHP_UX -DSYSV -DTERMIOS"
#	CC="gcc"
#	CFLAGS="-traditional -O2"
	LIBS="-ltermcap"
	;;

    *ibm-aix*)
	systype=IBM_AIX
	OS_DEF="-DAIX -DSYSV"
#	CC="cc"
	LIBS="-ltermcap -lbsd"
	;;

    *sgi-irix*)
	systype=SGI_IRIX
	OS_DEF="-DIRIX -DSYSV -DTERMIOS"
#	CC="cc"
	LIBS="-ltermcap"
	;;

    *dec-osf*)
	systype=DEC_OSF
	OS_DEF=""
#	CC="gcc"
	LIBS="-ltermcap"
	;;

    *-linux*)
	systype=LINUX
	OS_DEF="-DSYSV -DLINUX -DTERMIOS"
#	CC="cc"
#	CFLAGS="-O2 -I/usr/include/bsd"
##	CFLAGS="-O6 -s -pipe -fomit-frame-pointer -fstrength-reduce"
#    CFLAGS="-O2 -pipe"
#	LIBS="-ltermcap"
	LIBS=""
	;;

    *unixware*)
	systype=UNIXWARE
	OS_DEF="-DUNIXWARE"
#	CC="cc"
	LIBS="-ltermcap -lsocket"
	;;

    *)
	systype=UNKNOWN
	LIBS="-ltermcap"
	;;
esac

dnl Checks for programs.
#AC_PROG_AWK
#AC_PROG_YACC
AC_PROG_CC

if test -z "${MAKE}"; then
	dnl Find GNU Make. 1) search $PATH, 2) search specified path
	AC_CHECK_PROG([MAKE], [gmake], [gmake], [])
	if test -z "${MAKE}"; then
		dnl use AC_PATH_PROG because we need to set variable to the absolute name of gmake if found.
		AC_PATH_PROG([MAKE], [gmake], [no], [/usr/bin:/usr/local/bin:/usr/sfw/bin:/usr/local/gnu/bin])
	fi
fi
makever=`${MAKE} --version 2>&1 | grep "GNU Make"`
if test -z "$makever"; then
	AC_MSG_ERROR(GNU Make is not installed.)
fi

AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PATH_PROG(TRUE,true)
AC_PATH_PROG(PERL,perl)
AC_PATH_PROG(STRIP,strip)

dnl XXX: is this macro depreated?
dnl AC_PROG_ECHO

dnl Checks for libraries.
dnl Replace `main' with a function in -lbbs:
#AC_CHECK_LIB(bbs, main)
dnl Replace `main' with a function in -lbsd:
#AC_CHECK_LIB(bsd, main)

#if test $ac_cv_lib_crypt_crypt = "yes"; then
#  LIBS="$LIBS -lcrypt"
#fi
case "$target" in
	*solaris*)
		;;
	*)
		dnl Replace `main' with a function in -lcrypt:
		AC_CHECK_LIB(crypt, main)
#		echo "Removing -lcrypt for SOLARIS..."
#		LIBS=`echo $LIBS | sed -e s/-lcrypt//`
		;;
esac

#WEBBBSLIBS=`echo $LIBS | sed -e s/-ltermcap//`

dnl Replace `main' with a function in -lnsl:
#AC_CHECK_LIB(nsl, main)
dnl Replace `main' with a function in -lposix:
#AC_CHECK_LIB(posix, main)
dnl Replace `main' with a function in -lseq:
#AC_CHECK_LIB(seq, main)
dnl Replace `main' with a function in -lsocket:
#AC_CHECK_LIB(socket, main)
dnl Replace `main' with a function in -ltermcap:
#AC_CHECK_LIB(termcap, main)

dnl
AC_CONFIG_HEADER(config.h)

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
##AC_CHECK_HEADERS(fcntl.h limits.h malloc.h sgtty.h strings.h sys/file.h sys/ioctl.h sys/time.h syslog.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
##AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
##AC_STRUCT_ST_BLOCKS
##AC_STRUCT_ST_RDEV
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
#AC_PROG_GCC_TRADITIONAL
##AC_FUNC_MEMCMP
AC_HEADER_MAJOR
AC_FUNC_MMAP
AC_FUNC_SETPGRP
#AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
#AC_FUNC_UTIME_NULL
#AC_FUNC_VFORK
AC_FUNC_VPRINTF
AC_FUNC_WAIT3
AC_CHECK_FUNCS(snprintf strsignal strerror vsnprintf)
##AC_CHECK_FUNCS(vfork flock getcwd gethostname mkdir mktime rmdir select \
##               socket strstr strtol poll crypt)
#AC_CHECK_FUNCS(gettimeofday getwd putenv strdup strspn)

#
# Checking poll()
#
AC_CHECK_HEADERS(poll.h sys/poll.h)
AC_MSG_CHECKING([for poll()])
AC_TRY_COMPILE([
		#if defined(HAVE_POLL_H)
		#include <poll.h>
		#elif defined(HAVE_SYS_POLL_H)
		#include <sys/poll.h>
		#endif
	],[
		struct pollfd fds[1];
		fds[0].fd = 0;
		fds[0].events = POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL;

		poll(fds, 1, 1000);
	], enableval=yes, enableval=no)
AC_MSG_RESULT($enableval)
if test "${enableval}" = "yes"; then
	AC_DEFINE(HAVE_POLL, 1, [Define if poll is available in <poll.h> or <sys/poll.h>.])
else
	echo "*******************************"
	echo "* System MUST support poll(2) *"
	echo "*******************************"
	exit
fi

AC_CACHE_CHECK(if sys_siglist is already defined, ac_cv_needs_sys_siglist,
  AC_TRY_COMPILE([#include <stdio.h>],[char *s = sys_siglist[0];],
    ac_cv_needs_sys_siglist="no",
    ac_cv_needs_sys_siglist="yes")
)
if test "${ac_cv_needs_sys_siglist}" = "yes" ; then
  AC_DEFINE(NEED_SYS_SIGLIST, 1, [Description])
fi

ECHO_N=${ac_n}
ECHO_C=${ac_c}
ECHO_T=${ac_t}

AC_PREFIX_DEFAULT(/home/bbs)

AC_DEFINE_UNQUOTED(HOMEBBS,"$prefix",[Define the home directory of BBS])

AC_ARG_WITH(bbs-uid,
    [  --with-bbs-uid=number
                          BBS User ID (default 9999)],
    [ BBS_UID=$withval ],
    [ BBS_UID=$BBS_UID_DEFAULT ]
    )

AC_SUBST(BBS_UID)
AC_SUBST(BBS_UID_DEFAULT)

AC_ARG_WITH(bbs-gid,
    [  --with-bbs-gid=number
                          BBS Group ID (default 999)],
    [ BBS_GID=$withval ],
    [ BBS_GID=$BBS_GID_DEFAULT ]
    )

AC_SUBST(BBS_GID)
AC_SUBST(BBS_GID_DEFAULT)

AC_ARG_WITH(tcp-port,
    [  --with-tcp-port=number
                          Which port to use for BBS services (default 23)],
    [ BBS_TCP_PORT=$withval ],
    [ BBS_TCP_PORT=$BBS_TCP_PORT_DEFAULT ]
    )

AC_SUBST(BBS_TCP_PORT)
AC_SUBST(BBS_TCP_PORT_DEFAULT)

#AC_SUBST(BBS_UID)
#AC_SUBST(BBS_GID)
#AC_SUBST(HOMEBBS)
#AC_SUBST(BBS_NAME)
#AC_SUBST(BBS_ID)
#AC_SUBST(MAXACTIVE)
#AC_SUBST(MAXBOARD)
#AC_SUBST(USE_IDENT)
#AC_SUBST(EMAIL_LIMIT)
#AC_SUBST(SITE_DEF)
#AC_SUBST(WEBBBSLIBS)

AC_SUBST(systype)
AC_SUBST(OS_DEF)

dnl
AC_SUBST(ac_n)

dnl

AC_OUTPUT(Makefile bbsweb/Makefile csbbs/Makefile lib/Makefile src/Makefile
          src/util/Makefile util/Makefile bbsweb/util/Makefile)

cat << EOF

........................................................................
You may review the Makefile now. Then, type 'make' to build your system.

Final, type 'make install' to install your system.
........................................................................

EOF
