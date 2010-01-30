dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.59])
AC_INIT([FormosaBBS], [1.5.0], [formosabbs@googlegroups.com])
AC_CONFIG_SRCDIR([include/bbs.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR(cfgaux)
AC_VALIDATE_CACHED_SYSTEM_TUPLE(rm -f config.cache)
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AC_SUBST(PACKAGE_VERSION)

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_RANLIB

dnl Checking perl path
AC_ARG_VAR(PERL,local path to the perl interpreter)
perl_possible_path="/usr/bin:/usr/local/bin:/bin:/opt/perl/bin:/opt/perl/usr/bin:/opt/perl/usr/local/bin"
AC_PATH_PROG(PERL,perl,/usr/bin/env perl,$perl_possible_path)
dnl Checking strip path
AC_ARG_VAR(STRIP,local path to the strip)
strip_possible_path="/usr/bin:/usr/local/bin:/bin:/opt/bin:/opt/usr/bin:/opt/usr/local/bin"
AC_PATH_PROG(STRIP,strip,$strip_possible_path)
dnl Checking true path
AC_PATH_PROG(TRUE,true)

dnl Checking make utility
if test -z "${MAKE}"; then
	dnl Find GNU Make.	1) search $PATH
	dnl			2) search specified path
	dnl			3) search make for linux
	AC_CHECK_PROG([MAKE], [gmake], [gmake], [])
	if test -z "${MAKE}"; then
	dnl use AC_PATH_PROG because we need to set variable to the absolute name of gmake if found.
		AC_PATH_PROG([MAKE], [gmake], [], [/usr/bin:/usr/local/bin:/usr/sfw/bin:/usr/local/gnu/bin])
	fi
	if test -z "${MAKE}"; then
		AC_CHECK_PROG([MAKE], [make], [make], [])
	fi
	if test -z "${MAKE}"; then
		AC_PATH_PROG([MAKE], [make], [no], [/bin:/usr/bin])
	fi
fi
makever=`${MAKE} --version 2>&1 | grep "GNU Make"`
if test -z "$makever"; then
		AC_MSG_ERROR(GNU Make is not installed.)
fi

case $host in
	*-solaris*)
	systype=SOLARIS
	OS_DEF="-DSOLARIS -DSYSV"
	CFLAGS="-O2 -Wunused"
	LIBS="-ltermcap -lsocket -lnsl"
	;;

	*-freebsd*|*-netbsd*|*-bsdi*|*-apple-darwin*)
	systype=BSD
	OS_DEF="-DBSD44"
	LIBS="-ltermcap"
	;;

	*-linux-*)
	systype=LINUX
	OS_DEF="-DSYSV -DLINUX -DTERMIOS"
	LIBS=""
	;;

	*)
	systype=UNKNOWN
	LIBS="-ltermcap"
	;;
esac
AC_SUBST(systype)
AC_SUBST(OS_DEF)

AC_CACHE_CHECK(if sys_siglist is already defined, ac_cv_needs_sys_siglist,
			AC_TRY_COMPILE([#include <stdio.h>],[char *s = sys_siglist[0];],
				ac_cv_needs_sys_siglist="no",
				ac_cv_needs_sys_siglist="yes")
			  )
if test "${ac_cv_needs_sys_siglist}" = "yes" ; then
	AC_DEFINE(NEED_SYS_SIGLIST, [1], [Define sys_siglist support])
fi

ECHO_N=${ac_n}
AC_SUBST(ECHO_N)

AC_PREFIX_DEFAULT(/home/bbs)
test "x$prefix" = "xNONE" && prefix=$ac_default_prefix
AC_DEFINE([HOMEBBS], [], [Define the home directory of BBS])
AC_DEFINE_UNQUOTED(HOMEBBS, "$prefix")
HOMEBBS=$prefix
AC_SUBST(HOMEBBS)

BBS_USERNAME_DEFAULT="bbs"
AC_ARG_WITH(bbs-username,
	[  --with-bbs-username=name
			  BBS User Name (default bbs)],
	[ BBS_USERNAME=$withval ],
	[ BBS_USERNAME=$BBS_USERNAME_DEFAULT ]
)
AC_DEFINE([BBS_USERNAME], [], [Define the UserName of BBS])
AC_DEFINE_UNQUOTED(BBS_USERNAME, "$BBS_USERNAME")
AC_SUBST(BBS_USERNAME)

BBS_GROUPNAME_DEFAULT="bbs"
AC_ARG_WITH(bbs-groupname,
	[  --with-bbs-groupname=name
			  BBS Group Name (default bbs)],
	[ BBS_GROUPNAME=$withval ],
	[ BBS_GROUPNAME=$BBS_GROUPNAME_DEFAULT ]
)
AC_DEFINE([BBS_GROUPNAME], [], [Define the GroupName of BBS])
AC_DEFINE_UNQUOTED(BBS_GROUPNAME, "$BBS_GROUPNAME")
AC_SUBST(BBS_GROUPNAME)

BBS_TCP_PORT_DEFAULT=23
AC_ARG_WITH(tcp-port,
	[  --with-tcp-port=number
			  Which port to use for BBS services (default 23)],
	[ BBS_TCP_PORT=$withval ],
	[ BBS_TCP_PORT=$BBS_TCP_PORT_DEFAULT ]
)
AC_DEFINE([BBS_TCP_PORT], [], [Define the port to use for BBS services])
AC_DEFINE_UNQUOTED(BBS_TCP_PORT, $BBS_TCP_PORT)
AC_SUBST(BBS_TCP_PORT)

dnl Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h memory.h netdb.h netinet/in.h sgtty.h stdint.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h sys/vfs.h syslog.h termios.h unistd.h utime.h utmp.h utmpx.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_CHECK_DECLS([sys_siglist])
AC_STRUCT_TM
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

dnl Checks for library functions.
AC_HEADER_MAJOR
AC_FUNC_CHOWN
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SETPGRP
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_FUNC_WAIT3
AC_CHECK_FUNCS([alarm bzero dup2 ftruncate getcwd gethostbyaddr gethostbyname gethostname getpagesize gettimeofday inet_ntoa isascii memchr memmove memset mkdir munmap putenv rmdir select setenv snprintf socket strcasecmp strchr strdup strerror strncasecmp strpbrk strrchr strsignal strspn strstr strtol tzset utime vsnprintf])
AC_CHECK_LIB(crypt, main)

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
	AC_MSG_ERROR([System MUST support poll(2)])
fi

dnl Checking BBS User
AC_MSG_CHECKING([BBS User: $BBS_USERNAME])
AC_TRY_RUN([
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
				int main()
				{
				struct passwd *pwd = NULL;
				if (NULL == (pwd = getpwnam("$BBS_USERNAME"))) return -1;
				return 0;
				} ],
				[ enableval=yes ],
				[ enableval=no ])
AC_MSG_RESULT($enableval)
if test "x${enableval}" = "xno"; then
AC_MSG_ERROR([make sure bbs account existed or use --with-bbs-username instead.])
fi

dnl Checking BBS Group
AC_MSG_CHECKING([BBS Group: $BBS_GROUPNAME])
AC_TRY_RUN([
#include <string.h>
#include <sys/types.h>
#include <grp.h>
				int main()
				{
				struct group *grp = NULL;
				if (NULL == (grp = getgrnam("$BBS_GROUPNAME"))) return -1;
				return 0;
				} ],
				[ enableval=yes ],
				[ enableval=no ])
AC_MSG_RESULT($enableval)
if test "x${enableval}" = "xno"; then
AC_MSG_ERROR([make sure bbs group existed or use --with-bbs-groupname instead.])
fi

AC_CONFIG_FILES([Makefile
				bbsweb/Makefile
				bbsweb/util/Makefile
				csbbs/Makefile
				docs/Makefile
				lib/Makefile
				src/Makefile
				src/util/Makefile
				util/Makefile])

AC_OUTPUT
