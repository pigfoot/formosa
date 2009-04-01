/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have a working `mmap' system call.  */
#define HAVE_MMAP 1

/* Define if your struct stat has st_blocks.  */
/* #undef HAVE_ST_BLOCKS */

/* Define if your struct stat has st_rdev.  */
/* #undef HAVE_ST_RDEV */

/* Define if you have the strftime function.  */
#define HAVE_STRFTIME 1

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if utime(file, NULL) sets file's timestamp to the present.  */
/* #undef HAVE_UTIME_NULL */

/* Define if you have <vfork.h>.  */
/* #undef HAVE_VFORK_H */

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define if you have the wait3 system call.  */
#define HAVE_WAIT3 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if major, minor, and makedev are declared in <mkdev.h>.  */
/* #undef MAJOR_IN_MKDEV */

/* Define if major, minor, and makedev are declared in <sysmacros.h>.  */
/* #undef MAJOR_IN_SYSMACROS */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if the `setpgrp' function takes no argument.  */
#define SETPGRP_VOID 1

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define vfork as fork if vfork does not work.  */
/* #undef vfork */

/* Define if you have the getcwd function.  */
/* #undef HAVE_GETCWD */

/* Define if you have the gethostname function.  */
/* #undef HAVE_GETHOSTNAME */

/* Define if you have the getpagesize function.  */
#define HAVE_GETPAGESIZE 1

/* Define if you have the gettimeofday function.  */
/* #undef HAVE_GETTIMEOFDAY */

/* Define if you have the getwd function.  */
/* #undef HAVE_GETWD */

/* Define if you have the mkdir function.  */
/* #undef HAVE_MKDIR */

/* Define if you have the mktime function.  */
/* #undef HAVE_MKTIME */

/* Define if you have the putenv function.  */
/* #undef HAVE_PUTENV */

/* Define if you have the rmdir function.  */
/* #undef HAVE_RMDIR */

/* Define if you have the select function.  */
/* #undef HAVE_SELECT */

/* Define if you have the socket function.  */
/* #undef HAVE_SOCKET */

/* Define if you have the strdup function.  */
/* #undef HAVE_STRDUP */

/* Define if you have the strerror function.  */
/* #undef HAVE_STRERROR */

/* Define if you have the strspn function.  */
/* #undef HAVE_STRSPN */

/* Define if you have the strstr function.  */
/* #undef HAVE_STRSTR */

/* Define if you have the strtol function.  */
/* #undef HAVE_STRTOL */

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <fcntl.h> header file.  */
/* #undef HAVE_FCNTL_H */

/* Define if you have the <limits.h> header file.  */
/* #undef HAVE_LIMITS_H */

/* Define if you have the <malloc.h> header file.  */
/* #undef HAVE_MALLOC_H */

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <sgtty.h> header file.  */
/* #undef HAVE_SGTTY_H */

/* Define if you have the <strings.h> header file.  */
/* #undef HAVE_STRINGS_H */

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/file.h> header file.  */
/* #undef HAVE_SYS_FILE_H */

/* Define if you have the <sys/ioctl.h> header file.  */
/* #undef HAVE_SYS_IOCTL_H */

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/time.h> header file.  */
/* #undef HAVE_SYS_TIME_H */

/* Define if you have the <syslog.h> header file.  */
/* #undef HAVE_SYSLOG_H */

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the bsd library (-lbsd).  */
/* #undef HAVE_LIBBSD */

/* Define if you have the crypt library (-lcrypt).  */
#define HAVE_LIBCRYPT 1

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the posix library (-lposix).  */
/* #undef HAVE_LIBPOSIX */

/* Define if you have the seq library (-lseq).  */
/* #undef HAVE_LIBSEQ */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* Define if you have the termcap library (-ltermcap).  */
/* #undef HAVE_LIBTERMCAP */

/* If we need to declare sys_siglist[] as external */
#define NEED_SYS_SIGLIST 1

/* Define if you have the strsignal function.  */
#define HAVE_STRSIGNAL 1

/* If on Linux, maybe need define this for bbsweb --by lmj */
#if defined(LINUX)
# define HAVE_STRERROR
#endif
