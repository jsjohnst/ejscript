
/******************************************************************************/
/* 
 *  This file is an amalgamation of all the individual source code files for
 *  Michaels Portable Runtime 2.1.0.
 *
 *  Catenating all the source into a single file makes embedding simpler and
 *  the resulting application faster, as many compilers can do whole file
 *  optimization.
 *
 *  If you want to modify mpr, you can still get the whole source
 *  as individual files if you need.
 */


/************************************************************************/
/*
 *  Start of file "../include/mprOs.h"
 */
/************************************************************************/

/*
 *  mprOs.h -- Include O/S headers and smooth out per-O/S differences
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */


/*
 *  This header is part of the Michael's Portable Runtime and aims to include
 *  all necessary O/S headers and to unify the constants and declarations 
 *  required by Embedthis products. It can be included by C or C++ programs.
 */


#ifndef _h_MPR_OS_HDRS
#define _h_MPR_OS_HDRS 1

#include    "buildConfig.h"

/*
 *  Porters, add your CPU families here and update configure code. 
 */
#define MPR_CPU_UNKNOWN     0
#define MPR_CPU_IX86        1
#define MPR_CPU_PPC         2
#define MPR_CPU_SPARC       3
#define MPR_CPU_XSCALE      4
#define MPR_CPU_ARM         5
#define MPR_CPU_MIPS        6
#define MPR_CPU_68K         7
#define MPR_CPU_SIMNT       8           /* VxWorks NT simulator */
#define MPR_CPU_SIMSPARC    9           /* VxWorks sparc simulator */
#define MPR_CPU_IX64        10          /* AMD64 or EMT64 */
#define MPR_CPU_UNIVERSAL   11          /* MAC OS X universal binaries */
#define MPR_CPU_SH4         12


/* TODO merge in freebsd, VXWORKS & MACOSX */
 
#if BLD_UNIX_LIKE && !VXWORKS && !MACOSX
    #include    <sys/types.h>
    #include    <time.h>
    #include    <arpa/inet.h>
    #include    <ctype.h>
    #include    <dirent.h>
    #include    <dlfcn.h>
    #include    <fcntl.h>
    #include    <grp.h> 
    #include    <errno.h>
    #include    <libgen.h>
    #include    <limits.h>
    #include    <netdb.h>
    #include    <net/if.h>
    #include    <netinet/in.h>
    #include    <netinet/tcp.h>
    #include    <netinet/ip.h>
    #include    <pthread.h> 
    #include    <pwd.h> 
    #include    <sys/poll.h>
#if !CYGWIN
    #include    <resolv.h>
#endif
    #include    <setjmp.h>
    #include    <signal.h>
    #include    <stdarg.h>
    #include    <stdio.h>
    #include    <stdlib.h>
    #include    <string.h>
    #include    <syslog.h>
    #include    <sys/ioctl.h>
    #include    <sys/mman.h>
    #include    <sys/stat.h>
    #include    <sys/param.h>
    #include    <sys/resource.h>
    #include    <sys/sem.h>
    #include    <sys/shm.h>
    #include    <sys/socket.h>
    #include    <sys/select.h>
    #include    <sys/time.h>
    #include    <sys/times.h>
    #include    <sys/utsname.h>
    #include    <sys/uio.h>
    #include    <sys/wait.h>
    #include    <unistd.h>

#if LINUX && !__UCLIBC__
    #include    <sys/sendfile.h>
#endif

#if CYGWIN || LINUX
    #include    <stdint.h>
#else
    #include    <netinet/in_systm.h>
#endif

#if BLD_FEATURE_FLOATING_POINT
    #define __USE_ISOC99 1
    #include    <math.h>
#if !CYGWIN
    #include    <values.h>
#endif
#endif

#endif /* BLD_UNIX_LIKE */


#if VXWORKS
    #include    <vxWorks.h>
    #include    <envLib.h>
    #include    <sys/types.h>
    #include    <time.h>
    #include    <arpa/inet.h>
    #include    <ctype.h>
    #include    <dirent.h>
    #include    <fcntl.h>
    #include    <errno.h>
    #include    <limits.h>
    #include    <loadLib.h>
    #include    <netdb.h>
    #include    <net/if.h>
    #include    <netinet/tcp.h>
    #include    <netinet/in.h>
    #include    <netinet/ip.h>
    #include    <setjmp.h>
    #include    <signal.h>
    #include    <stdarg.h>
    #include    <stdio.h>
    #include    <stdlib.h>
    #include    <string.h>
    #include    <sysSymTbl.h>
    #include    <sys/fcntlcom.h>
    #include    <sys/ioctl.h>
    #include    <sys/stat.h>
    #include    <sys/socket.h>
    #include    <sys/times.h>
    #include    <unistd.h>
    #include    <unldLib.h>

#if _WRS_VXWORKS_MAJOR >= 6
    #include    <wait.h>
#endif

    #if BLD_FEATURE_FLOATING_POINT
    #include    <float.h>
    #define __USE_ISOC99 1
    #include    <math.h>
    #endif

    #include    <sockLib.h>
    #include    <inetLib.h>
    #include    <ioLib.h>
    #include    <pipeDrv.h>
    #include    <hostLib.h>
    #include    <netdb.h>
    #include    <tickLib.h>
    #include    <taskHookLib.h>
#endif /* VXWORKS */



#if MACOSX
    #include    <time.h>
    #include    <arpa/inet.h>
    #include    <ctype.h>
    #include    <dirent.h>
    #include    <dlfcn.h>
    #include    <fcntl.h>
    #include    <grp.h> 
    #include    <errno.h>
    #include    <libgen.h>
    #include    <limits.h>
    #include    <mach-o/dyld.h>
    #include    <netdb.h>
    #include    <net/if.h>
    #include    <netinet/in.h>
    #include    <netinet/tcp.h>
    #include    <sys/poll.h>
    #include    <pthread.h> 
    #include    <pwd.h> 
    #include    <resolv.h>
    #include    <setjmp.h>
    #include    <signal.h>
    #include    <stdarg.h>
    #include    <stdio.h>
    #include    <stdlib.h>
    #include    <stdint.h>
    #include    <string.h>
    #include    <syslog.h>
    #include    <sys/ioctl.h>
    #include    <sys/mman.h>
    #include    <sys/types.h>
    #include    <sys/stat.h>
    #include    <sys/param.h>
    #include    <sys/resource.h>
    #include    <sys/sem.h>
    #include    <sys/shm.h>
    #include    <sys/socket.h>
    #include    <sys/select.h>
    #include    <sys/time.h>
    #include    <sys/times.h>
    #include    <sys/types.h>
    #include    <sys/uio.h>
    #include    <sys/utsname.h>
    #include    <sys/wait.h>
    #include    <unistd.h>
    #include    <libkern/OSAtomic.h>

    #if BLD_FEATURE_FLOATING_POINT
    #include    <float.h>
    #define __USE_ISOC99 1
    #include    <math.h>
    #endif
#endif /* MACOSX */


#if FREEBSD
    #include    <time.h>
    #include    <arpa/inet.h>
    #include    <ctype.h>
    #include    <dirent.h>
    #include    <dlfcn.h>
    #include    <fcntl.h>
    #include    <grp.h> 
    #include    <errno.h>
    #include    <libgen.h>
    #include    <limits.h>
    #include    <netdb.h>
    #include    <sys/socket.h>
    #include    <net/if.h>
    #include    <netinet/in_systm.h>
    #include    <netinet/in.h>
    #include    <netinet/tcp.h>
    #include    <netinet/ip.h>
    #include    <pthread.h> 
    #include    <pwd.h> 
    #include    <resolv.h>
    #include    <signal.h>
    #include    <stdarg.h>
    #include    <stdio.h>
    #include    <stdlib.h>
    #include    <stdint.h>
    #include    <string.h>
    #include    <syslog.h>
    #include    <sys/ioctl.h>
    #include    <sys/types.h>
    #include    <sys/stat.h>
    #include    <sys/param.h>
    #include    <sys/resource.h>
    #include    <sys/sem.h>
    #include    <sys/shm.h>
    #include    <sys/select.h>
    #include    <sys/time.h>
    #include    <sys/times.h>
    #include    <sys/types.h>
    #include    <sys/utsname.h>
    #include    <sys/wait.h>
    #include    <unistd.h>

#if BLD_FEATURE_FLOATING_POINT
    #include    <float.h>
    #define __USE_ISOC99 1
    #include    <math.h>
#endif

    #define CLD_EXITED 1
    #define CLD_KILLED 2
#endif // FREEBSD

#if BLD_WIN_LIKE
    /*
     *  We replace insecure functions with Embedthis replacements
     */
    #define     _CRT_SECURE_NO_DEPRECATE 1

    /*
     *  Need this for the latest winsock APIs
     */
    #ifndef     _WIN32_WINNT
    #define     _WIN32_WINNT 0x501
    #endif

    #include    <winsock2.h>
    #include    <ws2tcpip.h>

    #include    <ctype.h>
    #include    <conio.h>
    #include    <direct.h>
    #include    <errno.h>
    #include    <fcntl.h>
    #include    <io.h>
    #include    <limits.h>
    #include    <malloc.h>
    #include    <process.h>
    #include    <sys/stat.h>
    #include    <sys/types.h>
    #include    <setjmp.h>
    #include    <stddef.h>
    #include    <stdio.h>
    #include    <stdlib.h>
    #include    <string.h>
    #include    <stdarg.h>
    #include    <time.h>
    #include    <windows.h>
    #if BLD_FEATURE_FLOATING_POINT
    #include    <math.h>
    #include    <float.h>
    #endif
    #include    <shlobj.h>
    #include    <shellapi.h>
    #include    <wincrypt.h>

#if BLD_DEBUG
    #include    <crtdbg.h>
#endif
    /*
     *  Make windows a bit more unix like
     */

    #undef     _WIN32_WINNT
#endif /* WIN */



#if BREW

    #if BLD_FEATURE_FLOATING_POINT
    #warning "Floating point is not supported on Brew"
    #endif

    #if BLD_FEATURE_MULTITHREAD
    #warning "Multithreading is not supported on Brew"
    #endif

    #include    <AEEModGen.h>
    #include    <AEEAppGen.h>
    #include    <BREWVersion.h>

    #if BREW_MAJ_VER == 2
        /*
         *  Fix for BREW 2.X
         */
        #ifdef __GNUC__
        #define __inline extern __inline__
        #endif
        #include    <AEENet.h>
        #undef __inline
    #endif

    #include    <AEE.h>
    #include    <AEEBitmap.h>
    #include    <AEEDisp.h>
    #include    <AEEFile.h>
    #include    <AEEHeap.h>
    #include    <AEEImageCtl.h>
    #include    <AEEMedia.h>
    #include    <AEEMediaUtil.h>
    #include    <AEEMimeTypes.h>
    #include    <AEEStdLib.h>
    #include    <AEEShell.h>
    #include    <AEESoundPlayer.h>
    #include    <AEEText.h>
    #include    <AEETransform.h>
    #include    <AEEWeb.h>

    #if BREW_MAJ_VER >= 3
    #include    <AEESMS.h>
    #endif

    #include    <AEETAPI.h>
#endif /* BREW */


#ifndef BITSPERBYTE
#define BITSPERBYTE     (8 * sizeof(char))
#endif

#define BITS(type)      (BITSPERBYTE * (int) sizeof(type))

#ifndef MAXINT
#if INT_MAX
    #define MAXINT      INT_MAX
#else
    #define MAXINT      0x7fffffff
#endif
    #define MAXINT64    INT64(0x7fffffffffffffff)
#endif

/*
 *  Byte orderings
 */
#define MPR_LITTLE_ENDIAN   1
#define MPR_BIG_ENDIAN      2

/*
 *  Current endian ordering
 */
#define MPR_ENDIAN          BLD_ENDIAN

/*
 *  Conversions between integer and pointer. Caller must ensure data is not lost
 */
#if __WORDSIZE == 64 || BLD_CPU_ARCH == MPR_CPU_IX64
    #define MPR_64_BIT 1
    #define ITOP(i)         ((void*) ((int64) i))
    #define PTOI(i)         ((int) ((int64) i))
    #define LTOP(i)         ((void*) ((int64) i))
    #define PTOL(i)         ((int64) i)
#else
    #define MPR_64_BIT 0
    #define ITOP(i)         ((void*) ((int) i))
    #define PTOI(i)         ((int) i)
    #define LTOP(i)         ((void*) ((int) i))
    #define PTOL(i)         ((int64) (int) i)
#endif

#ifndef max
    #define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
    #define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#define MPR_ARRAY_SIZE(type)    (sizeof(type) / sizeof(type[0]))

#ifndef PRINTF_ATTRIBUTE
#if (__GNUC__ >= 3) && !DOXYGEN && BLD_DEBUG
/** 
 *  Use gcc attribute to check printf fns.  a1 is the 1-based index of the parameter containing the format, 
 *  and a2 the index of the first argument. Note that some gcc 2.x versions don't handle this properly 
 */     
#define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define PRINTF_ATTRIBUTE(a1, a2)
#endif
#endif


#undef likely
#undef unlikely
#if (__GNUC__ >= 3)
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

/*
 *  Abbreviation for const types
 */
typedef const char cchar;
typedef const unsigned char cuchar;
typedef const void cvoid;

#ifdef __cplusplus
extern "C" {
#else

#if !MACOSX
typedef int bool;
#endif
#endif

//  TODO
#define MPR_INLINE


#if CYGWIN || LINUX

#if CYGWIN
    typedef unsigned long ulong;
#endif

    typedef unsigned char uchar;

    __extension__ typedef long long int int64;
    __extension__ typedef unsigned long long int uint64;

    #define INT64(x) (x##LL)
    #define UINT64(x) (x##ULL)
    #define closesocket(x)  close(x)
    #define MPR_BINARY      ""
    #define MPR_TEXT        ""
    #define SOCKET_ERROR    -1
    #define SET_SOCKOPT_CAST void*

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       MAXFLOAT
#endif

#define isNan(f) (f == FP_NAN)

#if CYGWIN
    #ifndef PTHREAD_MUTEX_RECURSIVE_NP
    #define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
    #endif
    #define __WALL          0

#else
    #define O_BINARY        0
    #define O_TEXT          0
    /*
     *  For some reason it is removed from fedora 6 pthreads.h and only
     *  comes in for UNIX96
     */
    extern int pthread_mutexattr_gettype (__const pthread_mutexattr_t *__restrict
                          __attr, int *__restrict __kind) __THROW;
    /* 
     *  Set the mutex kind attribute in *ATTR to KIND (either PTHREAD_MUTEX_NORMAL,
     *  PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ERRORCHECK, or PTHREAD_MUTEX_DEFAULT).  
     */
    extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind) __THROW;
#endif

#if LINUX
/* To avoid XOPEN define */
extern char *strptime(__const char *__restrict __s, __const char *__restrict __fmt, struct tm *__tp) __THROW;
extern char **environ;
#endif

    #define true 1
    #define false 0

#endif  /* CYGWIN || LINUX  */


#if VXWORKS

    typedef unsigned char uchar;
    typedef unsigned int uint;
    typedef unsigned long ulong;
    typedef long long int int64;
    typedef unsigned long long int uint64;

    #define HAVE_SOCKLEN_T
    #define INT64(x) (x##LL)
    #define UINT64(x) (x##ULL)

    #define closesocket(x)  close(x)
    #define getpid()        taskIdSelf()

    #define MPR_BINARY      ""
    #define MPR_TEXT        ""
    #define O_BINARY        0
    #define O_TEXT          0
    #define SOCKET_ERROR    -1
    #define MSG_NOSIGNAL    0
    #define __WALL          0

    //  TODO - refactor - rename
    #define SET_SOCKOPT_CAST char*

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       FLT_MAX
#endif

    #undef R_OK
    #define R_OK    4
    #undef W_OK
    #define W_OK    2
    #undef X_OK
    #define X_OK    1
    #undef F_OK
    #define F_OK    0

    extern int sysClkRateGet();

    #ifndef SHUT_RDWR
    #define SHUT_RDWR       2
    #endif

#if _WRS_VXWORKS_MAJOR < 6
    #define NI_MAXHOST      128
    extern STATUS access(const char *path, int mode);
    typedef int     socklen_t;
    struct sockaddr_storage {
        char        pad[1024];
    };
#else
    #if BLD_HOST_CPU_ARCH == MPR_CPU_PPC
        #define __va_copy(dest, src) *(dest) = *(src)
    #endif
    #define HAVE_SOCKLEN_T
#endif

#endif  /* VXWORKS */



#if MACOSX
    typedef unsigned long ulong;
    typedef unsigned char uchar;

    __extension__ typedef long long int int64;
    __extension__ typedef unsigned long long int uint64;

    #define INT64(x) (x##LL)
    #define UINT64(x) (x##ULL)

    #define closesocket(x)  close(x)
    #define MPR_BINARY      ""
    #define MPR_TEXT        ""
    #define O_BINARY        0
    #define O_TEXT          0
    #define SOCKET_ERROR    -1
    #define MSG_NOSIGNAL    0
    #define __WALL          0           /* 0x40000000 */
    #define SET_SOCKOPT_CAST void*
    #define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       MAXFLOAT
#endif
    
    /*
     *  Fix for MAC OS X - getenv
     */
    #if !HAVE_DECL_ENVIRON
    #ifdef __APPLE__
        #include <crt_externs.h>
        #define environ (*_NSGetEnviron())
    #else
        extern char **environ;
    #endif
    #endif
#endif /* MACOSX */


#if FREEBSD
    typedef unsigned long ulong;
    typedef unsigned char uchar;

    __extension__ typedef long long int int64;
    __extension__ typedef unsigned long long int uint64;
    #define INT64(x) (x##LL)

    typedef socklen_t       MprSocklen;
    #define SocketLenPtr    MprSocklen*
    #define closesocket(x)  close(x)
    #define MPR_BINARY      ""
    #define MPR_TEXT        ""
    #define O_BINARY        0
    #define O_TEXT          0
    #define SOCKET_ERROR    -1
    #define MPR_DLL_EXT     ".dylib"
    #define __WALL          0x40000000
    #define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       MAXFLOAT
#endif

#endif /* FREEBSD */


#if WIN
    typedef unsigned char uchar;
    typedef unsigned int uint;
    typedef unsigned long ulong;
    typedef unsigned short ushort;
    typedef __int64 int64;
    typedef unsigned __int64 uint64;

    typedef int     uid_t;
    typedef void    *handle;
    typedef char    *caddr_t;
    typedef long    pid_t;
    typedef int     gid_t;
    typedef ushort  mode_t;
    typedef void    *siginfo_t;
    typedef int     socklen_t;

    #define HAVE_SOCKLEN_T
    #define INT64(x) (x##i64)
    #define UINT64(x) (x##Ui64)

    #undef R_OK
    #define R_OK    4
    #undef W_OK
    #define W_OK    2

    /*
     *  On windows map X_OK to R_OK
     */
    #undef X_OK
    #define X_OK    4
    #undef F_OK
    #define F_OK    0

    #undef SHUT_RDWR
    #define SHUT_RDWR       2
    
    #ifndef EADDRINUSE
    #define EADDRINUSE      46
    #endif

    #ifndef EWOULDBLOCK
    #define EWOULDBLOCK     EAGAIN
    #endif

    #ifndef ENETDOWN
    #define ENETDOWN        43
    #endif

    #ifndef ECONNRESET
    #define ECONNRESET      44
    #endif

    #ifndef ECONNREFUSED
    #define ECONNREFUSED    45
    #endif

    #define MSG_NOSIGNAL    0
    #define MPR_BINARY      "b"
    #define MPR_TEXT        "t"

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       DBL_MAX
#endif

#ifndef FILE_FLAG_FIRST_PIPE_INSTANCE
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000
#endif

    #define SET_SOCKOPT_CAST const char*

    #define inline __inline
    #define chmod _chmod

    #define isNan(f) (_isnan(f))

    /*
     *  PHP can't handle this
     */
    #if !BUILDING_PHP
    #define popen _popen
    #define pclose _pclose
    #endif

/*
 *  When time began
 */
#define GENESIS UINT64(11644473600000000)

struct timezone {
  int  tz_minuteswest;      /* minutes W of Greenwich */
  int  tz_dsttime;          /* type of dst correction */
};

static int gettimeofday(struct timeval *tv, struct timezone *tz);
static char *strptime(cchar *buf, cchar *fmt, struct tm *tm);

    #define true 1
    #define false 0
#endif /* WIN */



#if SOLARIS
    typedef unsigned char uchar;
    typedef long long int int64;
    typedef unsigned long long int uint64;

    #define INT64(x) (x##LL)
    #define UINT64(x) (x##ULL)

    #define closesocket(x)  close(x)
    #define MPR_BINARY      ""
    #define MPR_TEXT        ""
    #define O_BINARY        0
    #define O_TEXT          0
    #define SOCKET_ERROR    -1
    #define MSG_NOSIGNAL    0
    #define INADDR_NONE     ((in_addr_t) 0xffffffff)
    #define __WALL  0
    #define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE

#if BLD_FEATURE_FLOATING_POINT
    #define MAX_FLOAT       MAXFLOAT
#endif

#endif /* SOLARIS */




#if BREW
    typedef unsigned char uchar;
    typedef unsigned int uint;
    typedef unsigned long ulong;
    typedef unsigned short ushort;
    typedef uint    off_t;
    typedef long    pid_t;

    #define O_RDONLY        0
    #define O_WRONLY        1
    #define O_RDWR          2
    #define O_CREAT         0x200
    #define O_TRUNC         0x400
    #define O_BINARY        0
    #define O_TEXT          0x20000
    #define O_EXCL          0x40000
    #define O_APPEND        0x80000

    #define R_OK    4
    #define W_OK    2
    #define X_OK    1
    #define F_OK    0

    #define SEEK_SET    0
    #define SEEK_CUR    1
    #define SEEK_END    2

    extern int  getpid();
    extern int  isalnum(int c);
    extern int  isalpha(int c);
    extern int  isdigit(int c);
    extern int  islower(int c);
    extern int  isupper(int c);
    extern int  isspace(int c);
    extern int  isxdigit(int c);

    extern uint strlen(const char *str);
    extern char *strstr(const char *string, const char *strSet);
    extern void *memset(const void *dest, int c, uint count);
    extern void exit(int status);
    extern char *strpbrk(const char *str, const char *set);
    extern uint strspn(const char *str, const char *set);
    extern int  tolower(int c);
    extern int  toupper(int c);
    extern void *memcpy(void *dest, const void *src, uint count);
    extern void *memmove(void *dest, const void *src, uint count);

    extern int  atoi(const char *str);
    extern void free(void *ptr);
    extern void *malloc(uint size);
    extern void *realloc(void *ptr, uint size);
    extern char *strcat(char *dest, const char *src);
    extern char *strchr(const char *str, int c);
    extern int  strcmp(const char *s1, const char *s2);
    extern int  strncmp(const char *s1, const char *s2, uint count);
    extern char *strcpy(char *dest, const char *src);
    extern char *strncpy(char *dest, const char *src, uint count);
    extern char *strrchr(const char *str, int c);

    #undef  printf
    #define printf DBGPRINTF

    #if BREWSIM && BLD_DEBUG
        extern _CRTIMP int __cdecl _CrtCheckMemory(void);
        extern _CRTIMP int __cdecl _CrtSetReportHook();
    #endif

    #ifndef _TIME_T_DEFINED
        typedef int64 time_t;
    #endif

    /*
     *  When time began
     */
    #define GENESIS UINT64(11644473600000000)

    struct timezone {
      int  tz_minuteswest;      /* minutes W of Greenwich */
      int  tz_dsttime;          /* type of dst correction */
    };

    static int gettimeofday(struct timeval *tv, struct timezone *tz);
    static char *strptime(cchar *buf, cchar *fmt, struct tm *tm);

#endif /* BREW */


typedef off_t MprOffset;

//  TODO fix. Use intptr_t
#if BLD_UNIX_LIKE
    typedef pthread_t   MprOsThread;
#elif BLD_CPU_ARCH == MPR_CPU_IX64
    typedef int64       MprOsThread;
#else
    typedef int         MprOsThread;
#endif


#ifdef __cplusplus
}
#endif

#endif /* _h_MPR_OS_HDRS */

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../include/mprOs.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/mprTune.h"
 */
/************************************************************************/

/*
 *  mprTune.h - Header for the Michael's Portable Runtime (MPR) Base.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
 *  See mpr.dox for additional documentation.
 */


#ifndef _h_MPR_TUNE
#define _h_MPR_TUNE 1


#include    "buildConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Build tuning
 */
#define MPR_TUNE_SIZE       1       /* Tune for size */
#define MPR_TUNE_BALANCED   2       /* Tune balancing speed and size */
#define MPR_TUNE_SPEED      3       /* Tune for speed */

#ifndef BLD_TUNE
#define BLD_TUNE MPR_TUNE_BALANCED
#endif


#if BLD_TUNE == MPR_TUNE_SIZE || DOXYGEN
    /*
     *  Squeeze mode optimizes to reduce memory usage
     */
    #define MPR_MAX_FNAME           128         /**< Reasonable filename size */
    #define MPR_MAX_PATH            256         /**< Reasonable path name size */
    #define MPR_DEFAULT_STACK       32768       /**< Default stack size (32K) */
    #define MPR_MAX_STRING          1024        /**< Maximum (stack) string size */
    
    #define MPR_DEFAULT_ALLOC       64          /**< Default small alloc size */
    #define MPR_DEFAULT_HASH_SIZE   23          /**< Default size of hash table */ 
    #define MPR_MAX_ARGC            128         /**< Reasonable max of args */
    #define MPR_MAX_LOG_STRING      512         /**< Maximum log message */
    #define MPR_BUFSIZE             1024        /**< Reasonable size for buffers */
    #define MPR_XML_BUFSIZE         512         /**< XML read buffer size */
    #define MPR_LIST_INCR           8           /**< Default list growth inc */
    #define MPR_BUF_INCR            1024        /**< Default buffer growth inc */
    #define MPR_MAX_BUF             4194304     /**< Max buffer size */
    #define MPR_HTTP_BUFSIZE        2048        /**< HTTP buffer size. Must fit complete HTTP headers */
    #define MPR_SSL_BUFSIZE         2048        /**< SSL has 16K max*/
    #define MPR_FILES_HASH_SIZE     29          /** Hash size for rom file system */
    #define MPR_TIME_HASH_SIZE      67          /** Hash size for time token lookup */
    #define MPR_HTTP_MAX_PASS       64          /**< Size of password */
    #define MPR_HTTP_MAX_USER       64          /**< Size of user name */
    #define MPR_HTTP_MAX_SECRET     32          /**< Random bytes to use */
    
#elif BLD_TUNE == MPR_TUNE_BALANCED
    
    /*
     *  Tune balancing speed and size
     */
    #define MPR_MAX_FNAME           256
    #define MPR_MAX_PATH            1024
    #define MPR_DEFAULT_STACK       65536
    #define MPR_MAX_STRING          2048
    #define MPR_DEFAULT_ALLOC       256
    #define MPR_DEFAULT_HASH_SIZE   43
    #define MPR_MAX_ARGC            256
    #define MPR_MAX_LOG_STRING      8192
    #define MPR_BUFSIZE             1024
    #define MPR_XML_BUFSIZE         1024
    #define MPR_LIST_INCR           16
    #define MPR_BUF_INCR            1024
    #define MPR_MAX_BUF             -1
    #define MPR_HTTP_BUFSIZE        4096
    #define MPR_SSL_BUFSIZE         4096
    #define MPR_FILES_HASH_SIZE     61
    #define MPR_TIME_HASH_SIZE      89
    
    #define MPR_HTTP_MAX_PASS       128
    #define MPR_HTTP_MAX_USER       64
    #define MPR_HTTP_MAX_SECRET     32
    
#else
    /*
     *  Tune for speed
     */
    #define MPR_MAX_FNAME           1024
    #define MPR_MAX_PATH            2048
    #define MPR_DEFAULT_STACK       131072
    #define MPR_MAX_STRING          4096
    #define MPR_DEFAULT_ALLOC       512
    #define MPR_DEFAULT_HASH_SIZE   97
    #define MPR_MAX_ARGC            512
    #define MPR_MAX_LOG_STRING      8192
    #define MPR_BUFSIZE             1024
    #define MPR_XML_BUFSIZE         1024
    #define MPR_LIST_INCR           16
    #define MPR_BUF_INCR            1024
    #define MPR_MAX_BUF             -1
    #define MPR_HTTP_BUFSIZE        8192
    #define MPR_SSL_BUFSIZE         4096
    #define MPR_FILES_HASH_SIZE     61
    #define MPR_TIME_HASH_SIZE      97
    
    #define MPR_HTTP_MAX_PASS       128
    #define MPR_HTTP_MAX_USER       64
    #define MPR_HTTP_MAX_SECRET     32
#endif


#if BLD_FEATURE_IPV6
#define MPR_MAX_IP_NAME         NI_MAXHOST      /**< Maximum size of a host name string */
#define MPR_MAX_IP_ADDR         128             /**< Maximum size of an IP address */
#define MPR_MAX_IP_PORT         6               /**< MMaximum size of a port number */

#define MPR_MAX_IP_ADDR_PORT    (MPR_MAX_IP_ADDR + NI_MAXSERV)  /**< Maximum size of an IP address with port number */

#else
/*
 *  IPv4 support only
 */
#define MPR_MAX_IP_NAME         128
#define MPR_MAX_IP_ADDR         16
#define MPR_MAX_IP_PORT         6
#define MPR_MAX_IP_ADDR_PORT    32
#endif

/*
 *  Signal sent on Unix to break out of a select call.
 */
#define MPR_WAIT_SIGNAL         (SIGUSR2)

/*
 *  Socket event message
 */
#define MPR_SOCKET_MESSAGE      (WM_USER + 32)

/*
 *  Priorities
 */
#define MPR_BACKGROUND_PRIORITY 15          /**< May only get CPU if idle */
#define MPR_LOW_PRIORITY        25
#define MPR_NORMAL_PRIORITY     50          /**< Normal (default) priority */
#define MPR_HIGH_PRIORITY       75
#define MPR_CRITICAL_PRIORITY   99          /**< May not yield */

#define MPR_EVENT_PRIORITY      75          /**< Run service event thread at higher priority */
#define MPR_POOL_PRIORITY       60          /**< Slightly elevated priority */
#define MPR_REQUEST_PRIORITY    50          /**< Normal priority */

/* 
 *  Timeouts
 */
#define MPR_TICKS_PER_SEC       1000        /**< Time ticks per second */
#define MPR_HTTP_TIMEOUT        60000       /**< HTTP Request timeout (60 sec) */
#define MPR_TIMEOUT_LOG_STAMP   3600000     /**< Time between log time stamps (1 hr) */
#define MPR_TIMEOUT_PRUNER      600000      /**< Time between pruner runs (10 min) */
#define MPR_TIMEOUT_START_TASK  2000        /**< Time to start tasks running */
#define MPR_TIMEOUT_STOP_TASK   10000       /**< Time to stop running tasks */
#define MPR_TIMEOUT_STOP_THREAD 10000       /**< Time to stop running threads */
#define MPR_TIMEOUT_STOP        5000        /**< Wait when stopping resources */
#define MPR_TIMEOUT_LINGER      2000        /**< Close socket linger timeout */


/*
 *  Default thread counts
 */
#if BLD_FEATURE_MULTITHREAD || DOXYGEN
#define MPR_DEFAULT_MIN_THREADS 0           /**< Default min threads (0) */
#define MPR_DEFAULT_MAX_THREADS 10          /**< Default max threads (10) */
#else
#define MPR_DEFAULT_MIN_THREADS 0
#define MPR_DEFAULT_MAX_THREADS 0
#endif

/*
 *  Debug control
 */
#define MPR_MAX_BLOCKED_LOCKS   100         /* Max threads blocked on lock */
#define MPR_MAX_RECURSION       15          /* Max recursion with one thread */
#define MPR_MAX_LOCKS           512         /* Total lock count max */
#define MPR_MAX_LOCK_TIME       (60 * 1000) /* Time in msec to hold a lock */

#define MPR_TIMER_TOLERANCE     2           /* Used in timer calculations */

/*
 *  Events
 */
#define MPR_EVENT_TIME_SLICE    20          /* 20 msec */


/*
 *  HTTP
 */
#define MPR_HTTP_RETRIES        (2)

#ifdef __cplusplus
}
#endif

#endif /* _h_MPR_TUNE */


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../include/mprTune.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/mpr.h"
 */
/************************************************************************/

/*
 *  mpr.h -- Header for the Michael's Portable Runtime (MPR).
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/**
 *  @file mpr.h
 *  Michael's Portable Runtime (MPR) is a portable runtime core for embedded applications.
 *  The MPR provides management for logging, error handling, events, files, http, memory, ssl, sockets, strings, 
 *  xml parsing, and date/time functions. It also provides a foundation of safe routines for secure programming, 
 *  that help to prevent buffer overflows and other security threats. It is correctly handles null arguments without
 *  crashing. The MPR is a library and a C API that can be used in both C and C++ programs.
 *  \n\n
 *  The MPR uses by convention a set extended typedefs for common types. These include: bool, cchar, cvoid, uchar, 
 *  short, ushort, int, uint, long, ulong, int64, uint64, float, and double. The cchar type is a const char, 
 *  cvoid is const void, and several types have "u" prefixes to denote unsigned qualifiers.
 *  \n\n
 *  The MPR includes a memory manager to minimize memory leaks and maximize allocation efficiency. It utilizes 
 *  a heap and slab allocators with tree links. All memory allocated is connected to a parent memory block thus forming a
 *  tree. When any block is freed, all child blocks are also freed. Most MPR APIs take a memory parent context 
 *  as the first parameter.
 *  \n\n
 *  Many of these APIs are not thread-safe. If utilizing multithreaded programming on a supporting 
 *      operating system, be careful if you modify or delete the underlying data while accessing the resource 
 *      from another thread.
 */

#ifndef _h_MPR
#define _h_MPR 1




#ifdef __cplusplus
extern "C" {
#endif


struct  Mpr;
struct  MprBlk;
struct  MprBuf;
struct  MprEvent;
struct  MprEventService;
struct  MprFile;
struct  MprFileInfo;
struct  MprFileService;
struct  MprHeap;
struct  MprHttp;
struct  MprModule;
struct  MprOsService;
struct  MprSocket;
struct  MprSocketService;
struct  MprSsl;
struct  MprWaitService;
struct  MprWaitHandler;
struct  MprXml;

#if BLD_FEATURE_MULTITHREAD
struct  MprCond;
struct  MprMutex;
struct  MprThreadService;
struct  MprThread;
struct  MprPoolService;
struct  MprPoolThread;
#endif


//  TODO - make these consistent: CANT vs NOT or NO

/**
 *  Standard MPR return and error codes
 */
#define MPR_ERR_OK                      0       /**< Success */
#define MPR_ERR_BASE                    -1      /*   Base error code */
#define MPR_ERR                         -1      /**< Default error code */
#define MPR_ERR_GENERAL                 -1      /**< General error */
#define MPR_ERR_ABORTED                 -2      /**< Action aborted */
#define MPR_ERR_ALREADY_EXISTS          -3      /**< Item already exists */
#define MPR_ERR_BAD_ARGS                -4      /**< Bad arguments or paramaeters */
#define MPR_ERR_BAD_FORMAT              -5      /**< Bad input format */
#define MPR_ERR_BAD_HANDLE              -6
#define MPR_ERR_BAD_STATE               -7      /**< Module is in a bad state */
#define MPR_ERR_BAD_SYNTAX              -8      /**< Input has bad syntax */
#define MPR_ERR_BAD_TYPE                -9
#define MPR_ERR_BAD_VALUE               -10
#define MPR_ERR_BUSY                    -11
#define MPR_ERR_CANT_ACCESS             -12     /**< Can't access the file or resource */
#define MPR_ERR_CANT_COMPLETE           -13
#define MPR_ERR_CANT_CREATE             -14     /**< Can't create the file or resource */
#define MPR_ERR_CANT_INITIALIZE         -15
#define MPR_ERR_CANT_OPEN               -16     /**< Can't open the file or resource */
#define MPR_ERR_CANT_READ               -17     /**< Can't read from the file or resource */
#define MPR_ERR_CANT_WRITE              -18     /**< Can't write to the file or resource */
#define MPR_ERR_DELETED                 -19
#define MPR_ERR_NETWORK                 -20
#define MPR_ERR_NOT_FOUND               -21
#define MPR_ERR_NOT_INITIALIZED         -22     /**< Module or resource is not initialized */
#define MPR_ERR_NOT_READY               -23
#define MPR_ERR_READ_ONLY               -24     /**< The operation timed out */
#define MPR_ERR_TIMEOUT                 -25
#define MPR_ERR_TOO_MANY                -26
#define MPR_ERR_WONT_FIT                -27
#define MPR_ERR_WOULD_BLOCK             -28
#define MPR_ERR_CANT_ALLOCATE           -29
#define MPR_ERR_NO_MEMORY               -30     /**< Memory allocation error */
#define MPR_ERR_CANT_DELETE             -31
#define MPR_ERR_CANT_CONNECT            -32
#define MPR_ERR_MAX                     -33

/**
 *  Standard logging trace levels are 0 to 9 with 0 being the most verbose. These are ored with the error source
 *  and type flags. The MPR_LOG_MASK is used to extract the trace level from a flags word. We expect most apps
 *  to run with level 2 trace enabled.
 */
#define MPR_ERROR       1       /* Hard error trace level */
#define MPR_WARN        2       /* Soft warning trace level */
#define MPR_CONFIG      2       /* Configuration settings trace level. */
#define MPR_INFO        3       /* Informational trace only */
#define MPR_DEBUG       4       /* Debug information trace level */
#define MPR_VERBOSE     9       /* Highest level of trace */
#define MPR_LEVEL_MASK  0xf     /* Level mask */

/*
 *  Error source flags
 */
#define MPR_ERROR_SRC   0x10    /* Originated from mprError */
#define MPR_LOG_SRC     0x20    /* Originated from mprLog */
#define MPR_ASSERT_SRC  0x40    /* Originated from mprAssert */
#define MPR_FATAL_SRC   0x80    /* Fatal error. Log and exit */

/*
 *  Log message type flags. Specify what kind of log / error message it is. Listener handlers examine this flag
 *  to determine if they should process the message.Assert messages are trapped when in DEV mode. Otherwise ignored.
 */
#define MPR_LOG_MSG     0x100   /* Log trace message - not an error */
#define MPR_ERROR_MSG   0x200   /* General error */
#define MPR_ASSERT_MSG  0x400   /* Assert flags -- trap in debugger */
#define MPR_USER_MSG    0x800   /* User message */

/*
 *  Log output modifiers
 */
#define MPR_RAW         0x1000  /* Raw trace output */

/*
 *  Error line number information.
 */
#define MPR_LINE(s)     #s
#define MPR_LINE2(s)    MPR_LINE(s)
#define MPR_LINE3       MPR_LINE2(__LINE__)
#define MPR_LOC        __FILE__ ":" MPR_LINE3

#define MPR_STRINGIFY(s) #s

/**
 *  Trigger a breakpoint.
 *  @description Triggers a breakpoint and traps to the debugger. 
 *  @ingroup Mpr
 */
extern void mprBreakpoint();

#if BLD_FEATURE_ASSERT
    #define mprAssert(C)    if (C) ; else mprStaticAssert(MPR_LOC, #C)
#else
    #define mprAssert(C)    if (1) ; else
#endif

/*
 *  Parameter values for serviceEvents(loopOnce)
 */
#define MPR_LOOP_ONCE           1
#define MPR_LOOP_FOREVER        0

#define MPR_TEST_TIMEOUT        10000       /* Ten seconds */
#define MPR_TEST_LONG_TIMEOUT   300000      /* 5 minutes */
#define MPR_TEST_SHORT_TIMEOUT  200         /* 1/5 sec */
#define MPR_TEST_NAP            50          /* Short timeout to prevent busy waiting */

/**
 *  Memory Allocation Service.
 *  @description The MPR provides a memory manager that sits above malloc. This layer provides arena and slab 
 *  based allocations with a tree structured allocation mechanism. The goal of the layer is to provide 
 *  a fast, secure, scalable memory allocator suited for embedded applications in multithreaded environments. 
 *  \n\n
 *  By using a tree structured network of memory contexts, error recovery in applications and memory freeing becomes
 *  much easier and more reliable. When a memory block is allocated a parent memory block must be specified. When
 *  the parent block is freed, all its children are automatically freed. 
 *  \n\n
 *  The MPR handles memory allocation errors globally. The application can configure a memory limits and redline
 *  so that memory depletion can be proactively detected and handled. This relieves most cost from detecting and
 *  handling allocation errors. 
 *  @stability Evolving
 *  @defgroup MprMem MprMem
 *  @see MprCtx, mprFree, mprRealloc, mprAlloc, mprAllocObject, mprAllocObjectZeroed, mprAllocZeroed, mprGetParent, 
 *      mprCreate, mprSetAllocLimits, mprAllocObjWithDestructor, mprAllocObjWithDestructorZeroed,
 *      mprHasAllocError mprResetAllocError, mprMemdup, mprStrndup, mprMemcpy, 
 */
typedef struct MprMem { int dummy; } MprMem;

/**
 *  Memory context type.
 *  @description Blocks of memory are allocated using a memory context as the parent. Any allocated memory block
 *      may serve as the memory context for subsequent memory allocations. Freeing a block via \ref mprFree
 *      will release the allocated block and all child blocks.
 *  @ingroup MprMem
 */
typedef cvoid *MprCtx;

/**
 *  Safe String Module
 *  @description The MPR provides a suite of safe string manipulation routines to help prevent buffer overflows
 *      and other potential security traps.
 *  @see MprString, mprAllocSprintf, mprAllocStrcat, mprAllocStrcpy, mprAllocVsprintf, mprAtoi, mprItoa, mprMemcpy,
 *      mprPrintf, mprReallocStrcat, mprSprintf, mprStaticPrintf, mprStrLower, mprStrTok, mprStrTrim, mprStrUpper,
 *      mprStrcmpAnyCase, mprStrcmpAnyCaseCount, mprStrcpy, mprStrlen, mprVsprintf, mprErrorPrintf,
 *      mprAllocStrcat, mprAllocStrcpy, mprReallocStrcat, mprAllocSprintf, mprAllocVsprintf, mprAllocSprintf
 */
typedef struct MprString { int dummy; } MprString;

//  TODO - rename
/**
 *  Print a formatted message to the standard error channel
 *  @description This is a secure replacement for fprintf(stderr. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string
 *  @return Returns the number of bytes written
 *  @ingroup MprString
 */
extern int mprErrorPrintf(MprCtx ctx, cchar *fmt, ...);

/**
 *  Print a message to the applications standard output without allocating memory.
 *  @description This is a secure replacement for printf that will not allocate memory.
 *  @param ctx Any memory context allocated by the MPR. This is used to locate the standard output channel and not
 *      to allocate memory.
 *  @param fmt Printf style format string
 *  @return Returns the number of bytes written
 *  @remarks The maximum output is MPR_MAX_STRING - 1.
 *  @ingroup MprString
 */
extern int mprStaticPrintf(MprCtx ctx, cchar *fmt, ...);

/**
 *  Print a message to the standard error channel without allocating memory.
 *  @description This is a secure replacement for fprintf(stderr that will not allocate memory.
 *  @param ctx Any memory context allocated by the MPR. This is used to locate the standard output channel and not
 *      to allocate memory.
 *  @param fmt Printf style format string
 *  @return Returns the number of bytes written
 *  @remarks The maximum output is MPR_MAX_STRING - 1.
 *  @ingroup MprString
 */
extern int mprStaticErrorPrintf(MprCtx ctx, cchar *fmt, ...);

/**
 *  Formatted print. This is a secure verion of printf that can handle null args.
 *  @description This is a secure replacement for printf. It can handle null arguments without crashes.
 *      minimal footprint. The MPR can be build without using any printf routines.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string
 *  @return Returns the number of bytes written
 *  @ingroup MprString
 */
extern int mprPrintf(MprCtx ctx, cchar *fmt, ...);

/**
 *  Print a formatted message to a file descriptor
 *  @description This is a replacement for fprintf as part of the safe string MPR library. It minimizes 
 *      memory use and uses a file descriptor instead of a File pointer.
 *  @param file MprFile object returned via mprOpen.
 *  @param fmt Printf style format string
 *  @return Returns the number of bytes written
 *  @ingroup MprString
 */
extern int mprFprintf(struct MprFile *file, cchar *fmt, ...);

/**
 *  Format a string into a statically allocated buffer.
 *  @description This call format a string using printf style formatting arguments. A trailing null will 
 *      always be appended. The call returns the size of the allocated string excluding the null.
 *  @param buf Pointer to the buffer.
 *  @param maxSize Size of the buffer.
 *  @param fmt Printf style format string
 *  @return Returns the number of characters in the string.
 *  @ingroup MprString
 */
extern int mprSprintf(char *buf, int maxSize, cchar *fmt, ...);

/**
 *  Format a string into a statically allocated buffer.
 *  @description This call format a string using printf style formatting arguments. A trailing null will 
 *      always be appended. The call returns the size of the allocated string excluding the null.
 *  @param buf Pointer to the buffer.
 *  @param maxSize Size of the buffer.
 *  @param fmt Printf style format string
 *  @param args Varargs argument obtained from va_start.
 *  @return Returns the number of characters in the string.
 *  @ingroup MprString
 */
extern int mprVsprintf(char *buf, int maxSize, cchar *fmt, va_list args);

/**
 *  Convert an integer to a string.
 *  @description This call converts the supplied integer into a string formatted into the supplied buffer.
 *  @param buf Pointer to the buffer that will hold the string.
 *  @param size Size of the buffer.
 *  @param value Integer value to convert
 *  @param radix The base radix to use when encoding the number
 *  @return Returns the number of characters in the string.
 *  @ingroup MprString
 */
extern char *mprItoa(char *buf, int size, int value, int radix);

/**
 *  Convert a string to an integer.
 *  @description This call converts the supplied string to an integer using the specified radix (base).
 *  @param str Pointer to the string to parse.
 *  @param radix Base to use when parsing the string
 *  @return Returns the integer equivalent value of the string. 
 *  @ingroup MprString
 */
extern int mprAtoi(cchar *str, int radix);

/**
 *  Get the next word token.
 *  @description Split a string into word tokens using the supplied delimiter.
 *  @param buf Buffer to use to hold the word token
 *  @param bufsize Size of the buffer
 *  @param str Input string to tokenize. Note this cannot be a const string. It will be written.
 *  @param delim String of delimiter characters to use when tokenizing
 *  @param tok Pointer to a word to hold a pointer to the next token in the original string.
 *  @return Returns the number of bytes in the allocated block.
 *  @ingroup MprString
 */
extern char *mprGetWordTok(char *buf, int bufsize, cchar *str, cchar *delim, cchar **tok);

/**
 *  Safe copy for a block of data.
 *  @description Safely copy a block of data into an existing memory block. The call ensures the destination 
 *      block is not overflowed and returns the size of the block actually copied. This is similar to memcpy, but 
 *      is a safer alternative.
 *  @param dest Pointer to the destination block.
 *  @param destMax Maximum size of the destination block.
 *  @param src Block to copy
 *  @param nbytes Size of the source block
 *  @return Returns the number of characters in the allocated block.
 *  @ingroup MprString
 */
extern int mprMemcpy(void *dest, int destMax, cvoid *src, int nbytes);

/**
 *  Compare two byte strings.
 *  @description Safely compare two byte strings. This is a safe replacement for memcmp.
 *  @param b1 Pointer to the first byte string.
 *  @param b1Len Length of the first byte string.
 *  @param b2 Pointer to the second byte string.
 *  @param b2Len Length of the second byte string.
 *  @return Returns zero if the byte strings are identical. Otherwise returns -1 if the first string is less than the 
 *      second. Returns 1 if the first is greater than the first.
 *  @ingroup MprString
 */
extern int mprMemcmp(cvoid *b1, int b1Len, cvoid *b2, int b2Len);

// TODO - should remove delimiter
/**
 *  Catenate strings.
 *  @description Safe replacement for strcat. Catenates a string onto an existing string. This call accepts 
 *      a variable list of strings to append. The list of strings is terminated by a null argument. The call
 *      returns the length of the resulting string. This call is similar to strcat, but it will enforce a 
 *      maximum size for the resulting string and will ensure it is terminated with a null.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param max Maximum size of the new block.
 *  @param delim String of delimiter characters to insert between appended strings.
 *  @param src Variable list of strings to append. The final string argument must be null.
 *  @return Returns the number of characters in the resulting string.
 *  @ingroup MprString
 */
extern int mprStrcat(char *dest, int max, cchar *delim, cchar *src, ...);
extern int mprCoreStrcat(MprCtx ctx, char **destp, int destMax, int existingLen, cchar *delim, cchar *src, va_list args);

/**
 *  Copy a string.
 *  @description Safe replacement for strcpy. Copy a string and ensure the target string is not overflowed. 
 *      The call returns the length of the resultant string or an error code if it will not fit into the target
 *      string. This is similar to strcpy, but it will enforce a maximum size for the copied string and will 
 *      ensure it is terminated with a null.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param destMax Maximum size of the target string.
 *  @param src String to copy
 *  @return Returns the number of characters in the target string.
 *  @ingroup MprString
 */
extern int mprStrcpy(char *dest, int destMax, cchar *src);

/**
 *  Find a substring.
 *  @description Locate the first occurrence of pattern in a string, but do not search more than the given length. 
 *  @param str Pointer to the string to search.
 *  @param pattern String pattern to search for.
 *  @param len Count of characters in the pattern to actually search for.
 *  @return Returns the number of characters in the target string.
 *  @ingroup MprString
 */
extern char *mprStrnstr(cchar *str, cchar *pattern, int len);

/**
 *  Compare strings.
 *  @description Compare two strings. This is a safe replacement for strcmp. It can handle null args.
 *  @param str1 First string to compare.
 *  @param str2 Second string to compare.
 *  @return Returns zero if the strings are identical. Return -1 if the first string is less than the second. Return 1
 *      if the first string is greater than the second.
 *  @ingroup MprString
 */
extern int mprStrcmp(cchar *str1, cchar *str2);

/**
 *  Compare strings ignoring case.
 *  @description Compare two strings ignoring case differences. This call operates similarly to strcmp.
 *  @param str1 First string to compare.
 *  @param str2 Second string to compare. 
 *  @return Returns zero if the strings are equivalent, < 0 if s1 sorts lower than s2 in the collating sequence 
 *      or > 0 if it sorts higher.
 *  @ingroup MprString
 */
extern int mprStrcmpAnyCase(cchar *str1, cchar *str2);

/**
 *  Compare strings ignoring case.
 *  @description Compare two strings ignoring case differences for a given string length. This call operates 
 *      similarly to strncmp.
 *  @param str1 First string to compare.
 *  @param str2 Second string to compare.
 *  @param len Length of characters to compare.
 *  @return Returns zero if the strings are equivalent, < 0 if s1 sorts lower than s2 in the collating sequence 
 *      or > 0 if it sorts higher.
 *  @ingroup MprString
 */
extern int mprStrcmpAnyCaseCount(cchar *str1, cchar *str2, int len);

/**
 *  Return the length of a string.
 *  @description Safe replacement for strlen. This call returns the length of a string and tests if the length is 
 *      less than a given maximum.
 *  @param src String to measure.
 *  @param max Maximum length for the string
 *  @return Returns the length of the string or MPR_ERR_WONT_FIT if the length is greater than \a max.
 *  @ingroup MprString
 */
extern int mprStrlen(cchar *src, int max);

/**
 *  Convert a string to lower case.
 *  @description Convert a string to its lower case equivalent.
 *  @param str String to convert.
 *  @return Returns a pointer to the converted string. Will always equal str.
 *  @ingroup MprString
 */
extern char *mprStrLower(char *str);


/**
 *  Convert a string to upper case.
 *  @description Convert a string to its upper case equivalent.
 *  @param str String to convert.
 *  @return Returns a pointer to the converted string. Will always equal str.
 *  @ingroup MprString
 */
extern char *mprStrUpper(char *str);

/**
 *  Trim a string.
 *  @description Trim leading and trailing characters off a string.
 *  @param str String to trim.
 *  @param set String of characters to remove.
 *  @return Returns a pointer to the trimmed string. May not equal \a str. If \a str was dynamically allocated, 
 *      do not call mprFree on the returned trimmed pointer. You must use \a str when calling mprFree.
 *  @ingroup MprString
 */
extern char *mprStrTrim(char *str, cchar *set);

/**
 *  Tokenize a string
 *  @description Split a string into tokens.
 *  @param str String to tokenize.
 *  @param delim String of characters to use as token separators.
 *  @param last Last token pointer.
 *  @return Returns a pointer to the next token.
 *  @ingroup MprString
 */
extern char *mprStrTok(char *str, cchar *delim, char **last);

/**
 *  Duplicate a string.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @description Copy a string into a newly allocated block. The new block will be sized to the maximum of the 
 *      length of the existing string (plus a null) and the requested size.
 *  @param str Pointer to the block to duplicate.
 *  @param size Requested minimum size of the allocated block holding the duplicated string.
 *  @return Returns an allocated block. Caller must free via #mprFree.
 *  @ingroup MprMem
 */
extern char *mprStrndup(MprCtx ctx, cchar *str, uint size);

/**
 *  Safe replacement for strdup
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @description mprStrdup() should be used as a replacement for \b strdup wherever possible. It allows the 
 *      strdup to be copied to be NULL, in which case it will allocate an empty string. 
 *  @param str Pointer to string to duplicate. If \b str is NULL, allocate a new string containing only a 
 *      trailing NULL character.
 *  @return Returns an allocated string including trailing null.
 *  @remarks Memory allocated via mprStrdup() must be freed via mprFree().
 *  @ingroup MprMem
 */
extern char *mprStrdup(MprCtx ctx, cchar *str);

/**
 *  Allocate a buffer of sufficient length to hold the formatted string.
 *  @description This call will dynamically allocate a buffer up to the specified maximum size and will format the 
 *      supplied arguments into the buffer.  A trailing null will always be appended. The call returns
 *      the size of the allocated string excluding the null.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param buf Pointer to a location to hold the buffer pointer.
 *  @param maxSize Maximum size to allocate for the buffer including the trailing null.
 *  @param fmt Printf style format string
 *  @return Returns the number of characters in the string.
 *  @ingroup MprString
 */
extern int mprAllocSprintf(MprCtx ctx, char **buf, int maxSize, cchar *fmt, ...);

/**
 *  Allocate a buffer of sufficient length to hold the formatted string.
 *  @description This call will dynamically allocate a buffer up to the specified maximum size and will format 
 *      the supplied arguments into the buffer. A trailing null will always be appended. The call returns
 *      the size of the allocated string excluding the null.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param buf Pointer to a location to hold the buffer pointer.
 *  @param maxSize Maximum size to allocate for the buffer including the trailing null.
 *  @param fmt Printf style format string
 *  @param arg Varargs argument obtained from va_start.
 *  @return Returns the number of characters in the string.
 *  @ingroup MprString
 */
extern int mprAllocVsprintf(MprCtx ctx, char **buf, int maxSize, cchar *fmt, va_list arg);

/**
 *  Allocate a new block to hold catenated strings.
 *  @description Allocate a new memory block of the required size and catenate
 *      the supplied strings delimited by the supplied delimiter. A variable
 *      number of strings may be supplied. The last argument must be a null.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param max Maximum size of the new block.
 *  @param delim Delimiter string to insert between the input strings
 *  @param src First string to catenate. There may be a variable number of of strings supplied.
 *  @return Returns the number of characters in the allocated block.
 *  @ingroup MprString
 */
extern int mprAllocStrcat(MprCtx ctx, char **dest, int max, cchar *delim, cchar *src, ...);

/**
 *  Copy a string into a newly allocated block.
 *  @description Allocate a new memory block of the required size and copy
 *      a string into it. The call returns the size of the allocated 
 *      block. This is similar to mprStrdup, but it will enforce a maximum
 *      size for the copied string and will ensure it is terminated with a null.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param max Maximum size of the new block.
 *  @param src Block to copy
 *  @return Returns the number of characters in the allocated block.
 *  @ingroup MprString
 */
extern int mprAllocStrcpy(MprCtx ctx, char **dest, int max, cchar *src);

/**
 *  Append strings to an existing string and reallocate as required.
 *  @description Append a list of strings to an existing string. The list of strings is terminated by a 
 *      null argument. The call returns the size of the allocated block. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param max Maximum size of the new block.
 *  @param existingLen Length of any existing string.
 *  @param delim String of delimiter characters to insert between appended strings.
 *  @param src Variable list of strings to append. The final string argument must be null.
 *  @return Returns the number of characters in the resulting string.
 *  @ingroup MprString
 */
extern int mprReallocStrcat(MprCtx ctx, char **dest, int max, int existingLen, cchar *delim, cchar *src, ...);

/**
 *  Buffer refill callback function
 *  @description Function to call when the buffer is depleted and needs more data.
 *  @param buf Instance of an MprBuf
 *  @param arg Data argument supplied to #mprSetBufRefillProc
 *  @returns The callback should return 0 if successful, otherwise a negative error code.
 *  @ingroup MprBuf
 */
typedef int (*MprBufProc)(struct MprBuf* bp, void *arg);

/**
 *  Dynamic Buffer Module
 *  @description MprBuf is a flexible, dynamic growable buffer structure. It has start and end pointers to the
 *      data buffer which act as read/write pointers. Routines are provided to get and put data into and out of the
 *      buffer and automatically advance the appropriate start/end pointer. By definition, the buffer is empty when
 *      the start pointer == the end pointer. Buffers can be created with a fixed size or can grow dynamically as 
 *      more data is added to the buffer. 
 *  \n\n
 *  For performance, the specification of MprBuf is deliberately exposed. All members of MprBuf are implicitly public.
 *  However, it is still recommended that wherever possible, you use the accessor routines provided.
 *  @stability Evolving.
 *  @see MprBuf, mprCreateBuf, mprSetBufMax, mprStealBuf, mprAdjustBufStart, mprAdjustBufEnd, mprCopyBufDown,
 *      mprFlushBuf, mprGetCharFromBuf, mprGetBlockFromBuf, mprGetBufLength, mprGetBufOrigin, mprGetBufSize,
 *      mprGetBufEnd, mprGetBufSpace, mprGetGrowBuf, mprGrowBuf, mprInsertCharToBuf,
 *      mprLookAtNextCharInBuf, mprLookAtLastCharInBuf, mprPutCharToBuf, mprPutBlockToBuf, mprPutIntToBuf,
 *      mprPutStringToBuf, mprPutFmtToBuf, mprRefillBuf, mprResetBufIfEmpty, mprSetBufSize, mprGetBufRefillProc,
 *      mprSetBufRefillProc, mprFree, MprBufProc
 *  @defgroup MprBuf MprBuf
 */
typedef struct MprBuf {
    uchar           *data;              /**< Actual buffer for data */
    uchar           *endbuf;            /**< Pointer one past the end of buffer */
    uchar           *start;             /**< Pointer to next data char */
    uchar           *end;               /**< Pointer one past the last data chr */
    int             buflen;             /**< Current size of buffer */
    int             maxsize;            /**< Max size the buffer can ever grow */
    int             growBy;             /**< Next growth increment to use */
    MprBufProc      refillProc;         /**< Auto-refill procedure */
    void            *refillArg;         /**< Refill arg */
} MprBuf;


/**
 *  Create a new buffer
 *  @description Create a new buffer. Use mprFree to free the buffer
 *  @param ctx Any memory context allocated by the MPR
 *  @param initialSize Initial size of the buffer
 *  @param maxSize Maximum size the buffer can grow to
 *  @return a new buffer
 *  @ingroup MprBuf
 */
extern MprBuf *mprCreateBuf(MprCtx ctx, int initialSize, int maxSize);

/**
 *  Set the maximum buffer size
 *  @description Update the maximum buffer size set when the buffer was created
 *  @param buf Buffer created via mprCreateBuf
 *  @param maxSize New maximum size the buffer can grow to
 *  @ingroup MprBuf
 */
extern void mprSetBufMax(MprBuf *buf, int maxSize);

/**
 *  Steal the buffer memory from a buffer
 *  @description Steal ownership of the buffer memory from the buffer structure. All MPR memory is owned by a 
 *      memory context and the contents of the buffer is owned by the MprBuf object. Stealing the buffer content 
 *      memory is useful to preserve the buffer contents after the buffer is freed
 *  @param ctx Memory context to won the memory for the buffer
 *  @param buf Buffer created via mprCreateBuf
 *  @return pointer to the buffer contents. Use mprGetBufLength before calling mprStealBuf to determine the resulting
 *      size of the contents.
 *  @ingroup MprBuf
 */
extern char *mprStealBuf(MprCtx ctx, MprBuf *buf);

/**
 *  Add a null character to the buffer contents.
 *  @description Add a null byte but do not change the buffer content lengths. The null is added outside the
 *      "official" content length. This is useful when calling #mprGetBufStart and using the returned pointer 
 *      as a "C" string pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @ingroup MprBuf
 */
extern void mprAddNullToBuf(MprBuf *buf);

/**
 *  Adjust the buffer start position
 *  @description Adjust the buffer start position by the specified amount. This is typically used to advance the
 *      start position as content is consumed. Adjusting the start or end position will change the value returned
 *      by #mprGetBufLength. If using the mprGetBlock or mprGetChar routines, adjusting the start position is
 *      done automatically.
 *  @param buf Buffer created via mprCreateBuf
 *  @param count Positive or negative count of bytes to adjust the start position.
 *  @ingroup MprBuf
 */
extern void mprAdjustBufStart(MprBuf *buf, int count);

/**
 *  Adjust the buffer end position
 *  @description Adjust the buffer start end position by the specified amount. This is typically used to advance the
 *      end position as content is appended to the buffer. Adjusting the start or end position will change the value 
 *      returned by #mprGetBufLength. If using the mprPutBlock or mprPutChar routines, adjusting the end position is
 *      done automatically.
 *  @param buf Buffer created via mprCreateBuf
 *  @param count Positive or negative count of bytes to adjust the start position.
 *  @ingroup MprBuf
 */
extern void mprAdjustBufEnd(MprBuf *buf, int count);

/**
 *  Compact the buffer contents
 *  @description Compact the buffer contents by copying the contents down to start the the buffer origin.
 *  @param buf Buffer created via mprCreateBuf
 *  @ingroup MprBuf
 */
extern void mprCompactBuf(MprBuf *buf);

/**
 *  Flush the buffer contents
 *  @description Discard the buffer contents and reset the start end content pointers.
 *  @param buf Buffer created via mprCreateBuf
 *  @ingroup MprBuf
 */
extern void mprFlushBuf(MprBuf *buf);

/**
 *  Get a character from the buffer
 *  @description Get the next byte from the buffer start and advance the start position.
 *  @param buf Buffer created via mprCreateBuf
 *  @return The character or -1 if the buffer is empty.
 *  @ingroup MprBuf
 */
extern int mprGetCharFromBuf(MprBuf *buf);

/**
 *  Get a block of data from the buffer
 *  @description Get a block of data from the buffer start and advance the start position. If the requested
 *      length is greater than the available buffer content, then return whatever data is available.
 *  @param buf Buffer created via mprCreateBuf
 *  @param blk Destination block for the read data. 
 *  @param count Count of bytes to read from the buffer.
 *  @return The count of bytes rread into the block or -1 if the buffer is empty.
 *  @ingroup MprBuf
 */
extern int mprGetBlockFromBuf(MprBuf *buf, uchar *blk, int count);

/**
 *  Get the buffer content length.
 *  @description Get the length of the buffer contents. This is not the same as the buffer size which may be larger.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns The length of the content stored in the buffer.
 *  @ingroup MprBuf
 */
extern int mprGetBufLength(MprBuf *buf);

/**
 *  Get the origin of the buffer content storage.
 *  @description Get a pointer to the start of the buffer content storage. This may not be equal to the start of
 *      the buffer content if #mprAdjustBufStart has been called. Use #mprGetBufSize to determine the length
 *      of the buffer content storage array. 
 *  @param buf Buffer created via mprCreateBuf
 *  @returns A pointer to the buffer content storage.
 *  @ingroup MprBuf
 */
extern char *mprGetBufOrigin(MprBuf *buf);

/**
 *  Get the current size of the buffer content storage.
 *  @description This returns the size of the memory block allocated for storing the buffer contents.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns The size of the buffer content storage.
 *  @ingroup MprBuf
 */
extern int mprGetBufSize(MprBuf *buf);

/**
 *  Get the space available to store content
 *  @description Get the number of bytes available to store content in the buffer
 *  @param buf Buffer created via mprCreateBuf
 *  @returns The number of bytes available
 *  @ingroup MprBuf
 */
extern int mprGetBufSpace(MprBuf *buf);

/**
 *  Get the start of the buffer contents
 *  @description Get a pointer to the start of the buffer contents. Use #mprGetBufLength to determine the length
 *      of the content. Use #mprGetBufEnd to get a pointer to the location after the end of the content.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns Pointer to the start of the buffer data contents
 *  @ingroup MprBuf
 */
extern char *mprGetBufStart(MprBuf *buf);

/**
 *  Get a reference to the end of the buffer contents
 *  @description Get a pointer to the location immediately after the end of the buffer contents.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns Pointer to the end of the buffer data contents. Points to the location one after the last data byte.
 *  @ingroup MprBuf
 */
extern char *mprGetBufEnd(MprBuf *buf);

/**
 *  Grow the buffer
 *  @description Grow the storage allocated for content for the buffer. The new size must be less than the maximum
 *      limit specified via #mprCreateBuf or #mprSetBufSize.
 *  @param buf Buffer created via mprCreateBuf
 *  @param count Count of bytes by which to grow the buffer content size. 
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprGrowBuf(MprBuf *buf, int count);

/**
 *  Insert a character into the buffer
 *  @description Insert a character into to the buffer prior to the current buffer start point.
 *  @param buf Buffer created via mprCreateBuf
 *  @param c Character to append.
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprInsertCharToBuf(MprBuf *buf, int c);

/**
 *  Peek at the next character in the buffer
 *  @description Non-destructively return the next character from the start position in the buffer. 
 *      The character is returned and the start position is not altered.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprLookAtNextCharInBuf(MprBuf *buf);

/**
 *  Peek at the last character in the buffer
 *  @description Non-destructively return the last character from just prior to the end position in the buffer. 
 *      The character is returned and the end position is not altered.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprLookAtLastCharInBuf(MprBuf *buf);

/**
 *  Put a character to the buffer.
 *  @description Append a character to the buffer at the end position and increment the end pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @param c Character to append
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprPutCharToBuf(MprBuf *buf, int c);

/**
 *  Put a block to the buffer.
 *  @description Append a block of data  to the buffer at the end position and increment the end pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @param ptr Block to append
 *  @param size Size of block to append
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprPutBlockToBuf(MprBuf *buf, cchar *ptr, int size);

/**
 *  Put an integer to the buffer.
 *  @description Append a integer to the buffer at the end position and increment the end pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @param i Integer to append to the buffer
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprPutIntToBuf(MprBuf *buf, int i);

/**
 *  Put a string to the buffer.
 *  @description Append a null terminated string to the buffer at the end position and increment the end pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @param str String to append
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprPutStringToBuf(MprBuf *buf, cchar *str);

/**
 *  Put a formatted string to the buffer.
 *  @description Format a string and Append to the buffer at the end position and increment the end pointer.
 *  @param buf Buffer created via mprCreateBuf
 *  @param fmt Printf style format string
 *  @param ... Variable arguments for the format string
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprPutFmtToBuf(MprBuf *buf, cchar *fmt, ...);

/**
 *  Refill the buffer with data
 *  @description Refill the buffer by calling the refill procedure specified via #mprSetBufRefillProc
 *  @param buf Buffer created via mprCreateBuf
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprRefillBuf(MprBuf *buf);

/**
 *  Reset the buffer
 *  @description If the buffer is empty, reset the buffer start and end pointers to the beginning of the buffer.
 *  @param buf Buffer created via mprCreateBuf
 *  @ingroup MprBuf
 */
extern void mprResetBufIfEmpty(MprBuf *buf);

/**
 *  Set the buffer size
 *  @description Set the current buffer content size and maximum size limit. Setting a current size will
 *      immediately grow the buffer to be this size. If the size is less than the current buffer size, 
 *      the requested size will be ignored. ie. this call will not shrink the buffer. Setting a maxSize 
 *      will define a maximum limit for how big the buffer contents can grow. Set either argument to 
 *      -1 to be ignored.
 *  @param buf Buffer created via mprCreateBuf
 *  @param size Size to immediately make the buffer. If size is less than the current buffer size, it will be ignored.
 *      Set to -1 to ignore this parameter.
 *  @param maxSize Maximum size the buffer contents can grow to.
 *  @returns Zero if successful and otherwise a negative error code 
 *  @ingroup MprBuf
 */
extern int mprSetBufSize(MprBuf *buf, int size, int maxSize);

/**
 *  Get the buffer refill procedure
 *  @description Return the buffer refill callback function.
 *  @param buf Buffer created via mprCreateBuf
 *  @returns The refill call back function if defined.
 *  @ingroup MprBuf
 */
extern MprBufProc mprGetBufRefillProc(MprBuf *buf);

/**
 *  Set the buffer refill procedure
 *  @description Define a buffer refill procedure. The MprBuf module will not invoke or manage this refill procedure.
 *      It is simply stored to allow upper layers to use and provide their own auto-refill mechanism.
 *  @param buf Buffer created via mprCreateBuf
 *  @param fn Callback function to store.
 *  @param arg Callback data argument.
 *  @ingroup MprBuf
 */
extern void mprSetBufRefillProc(MprBuf *buf, MprBufProc fn, void *arg);

/**
 *  Date and Time Service
 *  @stability Evolving
 *  @see MprTime, mprCtime, mprAsctime, mprGetTime, mprLocaltime, mprRfcTime
 *  @defgroup MprDate MprDate
 */
typedef struct MprDate { int dummy; } MprDate;

/**
 *  Mpr time structure.
 *  @description MprTime is the cross platform time abstraction structure. Time is stored as milliseconds
 *      since the epoch: 00:00:00 UTC Jan 1 1970. MprTime is typically a 64 bit quantity.
 *  @ingroup MprDate
 */
typedef int64 MprTime;
struct tm;

extern int mprCreateTimeService(MprCtx ctx);

/**
 *  Get the system time.
 *  @description Get the system time in milliseconds.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @return Returns the time in milliseconds since boot.
 *  @ingroup MprDate
 */
extern MprTime  mprGetTime(MprCtx ctx);

extern MprTime  mprGetTimeRemaining(MprCtx ctx, MprTime mark, uint timeout);
extern MprTime  mprGetElapsedTime(MprCtx ctx, MprTime mark);
extern int      mprCompareTime(MprTime t1, MprTime t2);

extern MprTime  mprMakeLocalTime(MprCtx ctx, struct tm *timep);
extern int      mprParseTime(MprCtx ctx, MprTime *time, cchar *str);


/**
 *  Format time as a string
 *  @description Safe replacement for asctime. This call formats the time value supplied via \a timeptr into the 
 *      supplied buffer. The supplied buffer size will be observed and the formatted time string will always
 *      have a terminating null.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param timeptr Time to format
 *  @param buf Buffer to hold the formatted string
 *  @param bufsize Maximum length for the string
 *  @return Returns the length of the string or MPR_ERR_WONT_FIT if the length is greater than \a max.
 *  @ingroup MprDate
 */
extern int mprAsctime(MprCtx ctx, char *buf, int bufsize, const struct tm *timeptr);

/**
 *  Convert time to local time and format as a string.
 *  @description Safe replacement for ctime. This call formats the time value supplied via \a timer into 
 *      the supplied buffer. The supplied buffer size will be observed and the formatted time string will always
 *      have a terminating null.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param buf Buffer to hold the formatted string
 *  @param bufsize Maximum length for the string
 *  @param time Time to format. Use mprGetTime to retrieve the current time.
 *  @return Returns the length of the string or MPR_ERR_WONT_FIT if the length is greater than \a max.
 *  @ingroup MprDate
 */
extern int mprCtime(MprCtx ctx, char *buf, int bufsize, MprTime time);

/**
 *  Convert time to local representation.
 *  @description Safe replacement for localtime. This call converts the time value to local time and formats 
 *      the as a struct tm.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param timep Pointer to a tm structure to hold the result
 *  @param time Time to format
 *  @return Returns a pointer to the tmBuf.
 *  @ingroup MprDate
 */
extern struct tm *mprLocaltime(MprCtx ctx, struct tm *timep, MprTime time);

/**
 *  Convert time to UTC and parse into a time structure
 *  @description Safe replacement for gmtime. This call converts the supplied time value
 *      to UTC time and parses the result into a tm structure.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param timep Pointer to a tm structure to hold the result.
 *  @param time The time to format
 *  @return Returns the tm structure reference
 *  @ingroup MprDate
 */
extern struct tm *mprGmtime(MprCtx ctx, struct tm *timep, MprTime time);

/**
 *  Format time according to RFC822.
 *  @description Thread-safe formatting of time dates according to RFC822.
 *      For example: "Fri, 07 Jan 2003 12:12:21 GMT"
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param buf Time to format
 *  @param bufsize Size of \a buf.
 *  @param timep Input pointer to a tm structure holding the time to format.
 *  @return Returns zero on success. Returns MPR_ERR_WONT_FIT if the bufsize is too small.
 */
extern int      mprRfctime(MprCtx ctx, char *buf, int bufsize, const struct tm *timep);

extern int      mprStrftime(MprCtx ctx, char *buf, int bufsize, cchar *fmt, const struct tm *timep);

/**
 *  List Module.
 *  @description The MprList is a dynamic growable list suitable for storing pointers to arbitrary objects.
 *  @stability Evolving.
 *  @see MprList, mprAddItem, mprGetItem, mprCreateList, mprClearList, mprLookupItem, mprFree, 
 *      mprGetFirstItem, mprGetListCapacity, mprGetListCount, mprGetNextItem, mprGetPrevItem, 
 *      mprRemoveItem, mprRemoveItemByIndex, mprRemoveRangeOfItems, mprAppendList, mprSortList, 
 *      mprDupList, MprListCompareProc, mprFree, mprCreateKeyPair
 *  @defgroup MprList MprList
 */
typedef struct MprList {
    void    **items;                    /**< List item data */
    int     length;                     /**< Current length of the list contents */
    int     capacity;                   /**< Current list size */ 
    int     maxSize;                    /**< Maximum capacity */
} MprList;

/**
 *  List comparison procedure for sorting
 *  @description Callback function signature used by #mprSortList
 *  @param arg1 First list item to compare
 *  @param arg2 Second list item to compare
 *  @returns Return zero if the items are equal. Return -1 if the first arg is less than the second. Otherwise return 1.
 *  @ingroup MprList
 */
typedef int (*MprListCompareProc)(cvoid *arg1, cvoid *arg2);


/**
 *  Add an item to a list
 *  @description Add the specified item to the list. The list must have been previously created via 
 *      mprCreateList. The list will grow as required to store the item
 *  @param list List pointer returned from #mprCreateList
 *  @param item Pointer to item to store
 *  @return Returns a positive integer list index for the inserted item. If the item cannot be inserted due 
 *      to a memory allocation failure, -1 is returned
 *  @ingroup MprList
 */
extern int mprAddItem(MprList *list, cvoid *item);


/**
 *  Append a list
 *  @description Append the contents of one list to another. The list will grow as required to store the item
 *  @param list List pointer returned from #mprCreateList
 *  @param add List whose contents are added
 *  @return Returns a pointer to the original list if successful. Returns NULL on memory allocation errors.
 *  @ingroup MprList
 */
extern MprList *mprAppendList(MprList *list, MprList *add);


/**
 *  Create a list.
 *  @description Creates an empty list. MprList's can store generic pointers. They automatically grow as 
 *      required when items are added to the list. Callers should invoke mprFree when finished with the
 *      list to release allocated storage.
 *  @param ctx Any memory context allocated by the MPR.
 *  @return Returns a pointer to the list. 
 *  @ingroup MprList
 */
extern MprList *mprCreateList(MprCtx ctx);

/**
 *  Copy a list
 *  @description Copy the contents of a list into an existing list. The destination list is cleared first and 
 *      has its dimensions set to that of the source clist.
 *  @param dest Destination list for the copy
 *  @param src Source list
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprCopyList(MprList *dest, MprList *src);

/**
 *  Duplicate a list
 *  @description Copy the contents of a list into a new list. 
 *  @param ctx Memory context from which to allocate the list. See #mprAlloc
 *  @param src Source list to copy
 *  @return Returns a new list reference
 *  @ingroup MprList
 */
extern MprList *mprDupList(MprCtx ctx, MprList *src);

/**
 *  Clears the list of all items.
 *  @description Resets the list length to zero and clears all items. Existing items are not freed, they 
 *      are only removed from the list.
 *  @param list List pointer returned from mprCreateList.
 *  @ingroup MprList
 */
extern void mprClearList(MprList *list);

/**
 *  Find an item and return its index.
 *  @description Search for an item in the list and return its index.
 *  @param list List pointer returned from mprCreateList.
 *  @param item Pointer to value stored in the list.
 *  @ingroup MprList
 */
extern int mprLookupItem(MprList *list, cvoid *item);

/**
 *  Get the first item in the list.
 *  @description Returns the value of the first item in the list. After calling this routine, the remaining 
 *      list items can be walked using mprGetNextItem.
 *  @param list List pointer returned from mprCreateList.
 *  @ingroup MprList
 */
extern void *mprGetFirstItem(MprList *list);

/**
 *  Get the last item in the list.
 *  @description Returns the value of the last item in the list. After calling this routine, the remaining 
 *      list items can be walked using mprGetPrevItem.
 *  @param list List pointer returned from mprCreateList.
 *  @ingroup MprList
 */
extern void *mprGetLastItem(MprList *list);

/**
 *  Get an list item.
 *  @description Get an list item specified by its index.
 *  @param list List pointer returned from mprCreateList.
 *  @param index Item index into the list. Indexes have a range from zero to the lenghth of the list - 1.
 *  @ingroup MprList
 */
extern void *mprGetItem(MprList *list, int index);

/**
 *  Get the current capacity of the list.
 *  @description Returns the capacity of the list. This will always be equal to or greater than the list length.
 *  @param list List pointer returned from mprCreateList.
 *  @ingroup MprList
 */
extern int mprGetListCapacity(MprList *list);

/**
 *  Get the number of items in the list.
 *  @description Returns the number of items in the list. This will always be less than or equal to the list capacity.
 *  @param list List pointer returned from mprCreateList.
 *  @ingroup MprList
 */
extern int mprGetListCount(MprList *list);

/**
 *  Get the next item in the list.
 *  @description Returns the value of the next item in the list. Before calling
 *      this routine, mprGetFirstItem must be called to initialize the traversal of the list.
 *  @param list List pointer returned from mprCreateList.
 *  @param lastIndex Pointer to an integer that will hold the last index retrieved.
 *  @ingroup MprList
 */
extern void *mprGetNextItem(MprList *list, int *lastIndex);

/**
 *  Get the previous item in the list.
 *  @description Returns the value of the previous item in the list. Before 
 *      calling this routine, mprGetFirstItem and/or mprGetNextItem must be
 *      called to initialize the traversal of the list.
 *  @param list List pointer returned from mprCreateList.
 *  @param lastIndex Pointer to an integer that will hold the last index retrieved.
 *  @ingroup MprList
 */
extern void *mprGetPrevItem(MprList *list, int *lastIndex);

/**
 *  Initialize a list structure
 *  @description If a list is statically declared inside another structure, mprInitList can be used to 
 *      initialize it before use.
 *  @param list Reference to the MprList struct.
 *  @ingroup MprList
 */
extern void mprInitList(MprList *list);

/**
 *  Insert an item into a list at a specific position
 *  @description Insert the item into the list before the specified position. The list will grow as required 
 *      to store the item
 *  @param list List pointer returned from #mprCreateList
 *  @param index Location at which to store the item. The previous item at this index is moved up to make room.
 *  @param item Pointer to item to store
 *  @return Returns the position index (positive integer) if successful. If the item cannot be inserted due 
 *      to a memory allocation failure, -1 is returned
 *  @ingroup MprList
 */
extern int mprInsertItemAtPos(MprList *list, int index, cvoid *item);

/**
 *  Remove an item from the list
 *  @description Search for a specified item and then remove it from the list.
 *      Existing items are not freed, they are only removed from the list.
 *  @param list List pointer returned from mprCreateList.
 *  @param item Item pointer to remove. 
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprRemoveItem(MprList *list, void *item);

/**
 *  Remove an item from the list
 *  @description Removes the element specified by \a index, from the list. The
 *      list index is provided by mprInsertItem.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprRemoveItemAtPos(MprList *list, int index);

/**
 *  Remove the last item from the list
 *  @description Remove the item at the highest index position.
 *      Existing items are not freed, they are only removed from the list.
 *  @param list List pointer returned from mprCreateList.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprRemoveLastItem(MprList *list);

/**
 *  Remove a range of items from the list.
 *  @description Remove a range of items from the list. The range is specified
 *      from the \a start index up to and including the \a end index.
 *  @param list List pointer returned from mprCreateList.
 *  @param start Starting item index to remove (inclusive)
 *  @param end Ending item index to remove (inclusive)
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprRemoveRangeOfItems(MprList *list, int start, int end);

/**
 *  Set a list item
 *  @description Update the list item stored at the specified index
 *  @param list List pointer returned from mprCreateList.
 *  @param index Location to update
 *  @param item Pointer to item to store
 *  @return Returns the old item previously at that location index
 *  @ingroup MprList
 */
extern void *mprSetItem(MprList *list, int index, cvoid *item);

/**
 *  Define the list size limits
 *  @description Define the list initial size and maximum size it can grow to.
 *  @param list List pointer returned from mprCreateList.
 *  @param initialSize Initial size for the list. This call will allocate space for at least this number of items.
 *  @param maxSize Set the maximum limit the list can grow to become.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprList
 */
extern int mprSetListLimits(MprList *list, int initialSize, int maxSize);

/**
 *  Sort a list
 *  @description Sort a list using the sort ordering dictated by the supplied compare function.
 *  @param list List pointer returned from mprCreateList.
 *  @param compare Comparison function. If null, then a default string comparison is used.
 *  @ingroup MprList
 */
extern void mprSortList(MprList *list, MprListCompareProc compare);

/**
 *  Key value pairs for use with MprList or MprHash
 *  @ingroup MprList
 */
typedef struct MprKeyValue {
    char        *key;               /**< Key string */
    char        *value;             /**< Associated value for the key */
} MprKeyValue;


/**
 *  Create a key / value pair
 *  @description Allocate and initialize a key value pair for use by the MprList or MprHash modules.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param key Key string
 *  @param value Key value string
 *  @returns An initialized MprKeyValue
 *  @ingroup MprList
 */
extern MprKeyValue *mprCreateKeyPair(MprCtx ctx, cchar *key, cchar *value);

/**
 *  Logging Services
 *  @stability Evolving
 *  @defgroup MprLog MprLog
 *  @see mprError, mprLog, mprSetLogHandler, mprSetLogLevel, mprUserError, mprRawLog, mprFatalError, MprLogHandler
 *      mprGetLogHandler, mprMemoryError, mprStaticAssert, mprStaticError
 */
typedef struct MprLog { int dummy; } MprLog;

/**
 *  Log handler callback type.
 *  @description Callback prototype for the log handler. Used by mprSetLogHandler to define 
 *      a message logging handler to process log and error messages. 
 *  @param file Source filename. Derived by using __FILE__.
 *  @param line Source line number. Derived by using __LINE__.
 *  @param flags Error flags.
 *  @param level Message logging level. Levels are 0-9 with zero being the most verbose.
 *  @param msg Message being logged.
 *  @ingroup MprLog
 */
typedef void (*MprLogHandler)(MprCtx ctx, int flags, int level, cchar *msg);

/**
 *  Set an MPR debug log handler.
 *  @description Defines a callback handler for MPR debug and error log messages. When output is sent to 
 *      the debug channel, the log handler will be invoked to accept the output message.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param handler Callback handler
 *  @param handlerData Callback handler data
 *  @ingroup MprLog
 */
extern void mprSetLogHandler(MprCtx ctx, MprLogHandler handler, void *handlerData);

/**
 *  Get the current MPR debug log handler.
 *  @description Get the log handler defined via #mprSetLogHandler
 *  @param ctx Any memory context allocated by the MPR.
 *  @returns A function of the signature #MprLogHandler
 *  @ingroup MprLog
 */
extern MprLogHandler mprGetLogHandler(MprCtx ctx);

/**
 *  Log an error message.
 *  @description Send an error message to the MPR debug logging subsystem. The 
 *      message will be to the log handler defined by #mprSetLogHandler. It 
 *      is up to the log handler to respond appropriately and log the message.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @ingroup MprLog
 */
extern void mprError(MprCtx ctx, cchar *fmt, ...);

/**
 *  Log a fatal error message and exit.
 *  @description Send a fatal error message to the MPR debug logging subsystem and then exit the application by
 *      calling exit(). The message will be to the log handler defined by #mprSetLogHandler. It 
 *      is up to the log handler to respond appropriately and log the message.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @ingroup MprLog
 */
extern void mprFatalError(MprCtx ctx, cchar *fmt, ...);

/**
 *  Log a memory error message.
 *  @description Send a memory error message to the MPR debug logging subsystem. The message will be 
 *      passed to the log handler defined by #mprSetLogHandler. It is up to the log handler to respond appropriately
 *      to the fatal message, the MPR takes no other action other than logging the message. Typically, a memory 
 *      message will be logged and the application will be shutdown. The preferred method of operation is to define
 *      a memory depletion callback via #mprCreate. This will be invoked whenever a memory allocation error occurs.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @ingroup MprLog
 */
extern void mprMemoryError(MprCtx ctx, cchar *fmt, ...);

/**
 *  Write a message to the diagnostic log file.
 *  @description Send a message to the MPR logging subsystem.
 *  @param level Logging level for this message. The level is 0-9 with zero being the most verbose.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @remarks mprLog is highly useful as a debugging aid when integrating or when developing new modules. 
 *  @ingroup MprLog
 */
extern void mprLog(MprCtx ctx, int level, cchar *fmt, ...);

/**
 *  Write a raw log message to the diagnostic log file.
 *  @description Send a raw message to the MPR logging subsystem. Raw messages do not have any application prefix
 *      attached to the message and do not append a newline to the message.
 *  @param level Logging level for this message. The level is 0-9 with zero being the most verbose.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @remarks mprLog is highly useful as a debugging aid when integrating or when developing new modules. 
 *  @ingroup MprLog
 */
extern void mprRawLog(MprCtx ctx, int level, cchar *fmt, ...);

/**
 *  Output an assertion failed message.
 *  @description This will emit an assertion failed message to the standard error output. It will bypass the logging
 *      system.
 *  @param loc Source code location string. Use MPR_LOC to define a file name and line number string suitable for this
 *      parameter.
 *  @param msg Simple string message to output
 *  @ingroup MprLog
 */
extern void mprStaticAssert(cchar *loc, cchar *msg);

/**
 *  Write a message to the diagnostic log file without allocating any memory. Useful for log messages from within the
 *      memory allocator.
 *  @description Send a message to the MPR logging subsystem. This will not allocate any memory while formatting the 
 *      message. The formatted message string will be truncated in size to #MPR_MAX_STRING bytes. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @ingroup MprLog
 */
extern void mprStaticError(MprCtx ctx, cchar *fmt, ...);


/**
 *  Display an error message to the user.
 *  @description Display an error message to the user and then send it to the 
 *      MPR debug logging subsystem. The message will be passed to the log 
 *      handler defined by mprSetLogHandler. It is up to the log handler to 
 *      respond appropriately and display the message to the user.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param fmt Printf style format string. Variable number of arguments to 
 *  @param ... Variable number of arguments for printf data
 *  @ingroup MprLog
 */
extern void mprUserError(MprCtx ctx, cchar *fmt, ...);

/**
 *  Hash table entry structure.
 *  @description Each hash entry has a descriptor entry. This is used to manage the hash table link chains.
 *  @see MprHash, mprAddHash, mprAddDuplicateHash, mprCopyHash, mprCreateHash, mprGetFirstHash, mprGetNextHash,
 *      mprGethashCount, mprLookupHash, mprLookupHashEntry, mprRemoveHash, mprFree, mprCreateKeyPair
 *  @stability Evolving.
 *  @defgroup MprHash MprHash
 */
typedef struct MprHash {
    struct MprHash *next;               /**< Next symbol in hash chain */
    char            *key;               /**< Hash key */
    cvoid           *data;              /**< Pointer to symbol data */
    int             bucket;             /**< Hash bucket index */
} MprHash;


/**
 *  Hash table control structure
 */
typedef struct MprHashTable {
    MprHash         **buckets;          /**< Hash collision bucket table */
    int             hashSize;           /**< Size of the buckets array */
    int             count;              /**< Number of symbols in the table */
} MprHashTable;


/**
 *  Add a symbol value into the hash table
 *  @description Associate an arbitrary value with a string symbol key and insert into the symbol table.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param key String key of the symbole entry to delete.
 *  @param ptr Arbitrary pointer to associate with the key in the table.
 *  @return Integer count of the number of entries.
 *  @ingroup MprHash
 */
extern MprHash *mprAddHash(MprHashTable *table, cchar *key, cvoid *ptr);

/**
 *  Add a duplicate symbol value into the hash table
 *  @description Add a symbol to the hash which may clash with an existing entry. Duplicate symbols can be added to
 *      the hash, but only one may be retrieved via #mprLookupHash. To recover duplicate entries walk the hash using
 *      #mprGetNextHash.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param key String key of the symbole entry to delete.
 *  @param ptr Arbitrary pointer to associate with the key in the table.
 *  @return Integer count of the number of entries.
 *  @ingroup MprHash
 */
extern MprHash *mprAddDuplicateHash(MprHashTable *table, cchar *key, cvoid *ptr);

/**
 *  Copy a hash table
 *  @description Create a new hash table and copy all the entries from an existing table.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @return A new hash table initialized with the contents of the original hash table.
 *  @ingroup MprHash
 */
extern MprHashTable *mprCopyHash(MprCtx ctx, MprHashTable *table);

/**
 *  Create a hash table
 *  @description Creates a hash table that can store arbitrary objects associated with string key values.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param hashSize Size of the hash table for the symbol table. Should be a prime number.
 *  @return Returns a pointer to the allocated symbol table. Caller should use mprFree to dispose of the table 
 *      when complete.
 *  @ingroup MprHash
 */
extern MprHashTable *mprCreateHash(MprCtx ctx, int hashSize);

/**
 *  Return the first symbol in a symbol entry
 *  @description Prepares for walking the contents of a symbol table by returning the first entry in the symbol table.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @return Pointer to the first entry in the symbol table.
 *  @ingroup MprHash
 */
extern MprHash *mprGetFirstHash(MprHashTable *table);

/**
 *  Return the next symbol in a symbol entry
 *  @description Continues walking the contents of a symbol table by returning
 *      the next entry in the symbol table. A previous call to mprGetFirstSymbol
 *      or mprGetNextSymbol is required to supply the value of the \a last
 *      argument.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param last Symbol table entry returned via mprGetFirstSymbol or mprGetNextSymbol.
 *  @return Pointer to the first entry in the symbol table.
 *  @ingroup MprHash
 */
extern MprHash *mprGetNextHash(MprHashTable *table, MprHash *last);

/**
 *  Return the count of symbols in a symbol entry
 *  @description Returns the number of symbols currently existing in a symbol table.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @return Integer count of the number of entries.
 *  @ingroup MprHash
 */
extern int mprGetHashCount(MprHashTable *table);

/**
 *  Lookup a symbol in the hash table.
 *  @description Lookup a symbol key and return the value associated with that key.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param key String key of the symbole entry to delete.
 *  @return Value associated with the key when the entry was inserted via mprInsertSymbol.
 *  @ingroup MprHash
 */
extern cvoid *mprLookupHash(MprHashTable *table, cchar *key);

/**
 *  Lookup a symbol in the hash table and return the hash entry
 *  @description Lookup a symbol key and return the hash table descriptor associated with that key.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param key String key of the symbole entry to delete.
 *  @return MprHash table structure for the entry
 *  @ingroup MprHash
 */
extern MprHash *mprLookupHashEntry(MprHashTable *table, cchar *key);

/**
 *  Remove a symbol entry from the hash table.
 *  @description Removes a symbol entry from the symbol table. The entry is looked up via the supplied \a key.
 *  @param table Symbol table returned via mprCreateSymbolTable.
 *  @param key String key of the symbole entry to delete.
 *  @return Returns zero if successful, otherwise a negative MPR error code is returned.
 *  @ingroup MprHash
 */
extern int mprRemoveHash(MprHashTable *table, cchar *key);


/*
 *  Flags for mprSearchForFile
 */
#define MPR_SEARCH_EXE      0x1         /* Search for an executable */
#define MPR_SEARCH_EJSMOD   0x4         /* Do ejscript a.b.c style searching */
#if UNUSED
#define MPR_SEARCH_DSO      0x2         /* Search for a DSO */
#define MPR_SEARCH_BIN      0x4         /* Expect to find in a bin directory */
#define MPR_SEARCH_LIB      0x4         /* Expect to find in a lib directory */
#define MPR_SEARCH_APP      0x4         /* Try to find relative to the current app executable */
#endif

/*
 *  Search path delimiter
 */
#if BLD_WIN_LIKE
#define MPR_SEARCH_DELIM ";"
#else
#define MPR_SEARCH_DELIM ":"
#endif

typedef bool            (*MprAccessFileProc)(struct MprFileService *fs, cchar *path, int omode);
typedef int             (*MprDeleteFileProc)(struct MprFileService *fs, cchar *path);
typedef int             (*MprDeleteDirProc)(struct MprFileService *fs, cchar *path);
typedef int             (*MprGetFileInfoProc)(struct MprFileService *fs, cchar *path, struct MprFileInfo *info);
typedef int             (*MprMakeDirProc)(struct MprFileService *fs, cchar *path, int perms);
typedef struct MprFile* (*MprOpenFileProc)(MprCtx ctx, struct MprFileService *fs, cchar *path, int omode, int perms);
typedef void            (*MprCloseFileProc)(struct MprFile *file);
typedef int             (*MprReadFileProc)(struct MprFile *file, void *buf, uint size);
typedef long            (*MprSeekFileProc)(struct MprFile *file, int seekType, long distance);
typedef int             (*MprSetBufferedProc)(struct MprFile *file, int initialSize, int maxSize);
typedef int             (*MprWriteFileProc)(struct MprFile *file, cvoid *buf, uint count);

/**
 *  File system service
 *  @description The MPR provides a file system abstraction to support non-disk based file access such as flash or 
 *      other ROM based file systems. The MprFileService structure defines a virtual file system interface that
 *      will be invoked by the various MPR file routines.
 */
typedef struct MprFileService {
    MprAccessFileProc   accessFile;     /**< Virtual access file routine */
    MprDeleteFileProc   deleteFile;     /**< Virtual delete file routine */
    MprDeleteDirProc    deleteDir;      /**< Virtual delete directory routine */
    MprGetFileInfoProc  getFileInfo;    /**< Virtual get file information routine */
    MprMakeDirProc      makeDir;        /**< Virtual make directory routine */
    MprOpenFileProc     openFile;       /**< Virtual open file routine */
    MprCloseFileProc    closeFile;      /**< Virtual close file routine */
    MprReadFileProc     readFile;       /**< Virtual read file routine */
    MprSeekFileProc     seekFile;       /**< Virtual seek file routine */
    MprSetBufferedProc  setBuffered;    /**< Virtual set buffered I/O routine */
    MprWriteFileProc    writeFile;      /**< Virtual write file routine */

    struct MprFile  *console;           /**< Standard output file */
    struct MprFile  *error;             /**< Standard error file */

} MprFileService;


#if BLD_FEATURE_ROMFS
/*
 *  A RomInode is created for each file in the Rom file system.
 */
typedef struct  MprRomInode {
    char            *path;              /* File path */
    uchar           *data;              /* Pointer to file data */
    int             size;               /* Size of file */
    int             num;                /* Inode number */
} MprRomInode;


typedef struct MprRomFileService {
    MprFileService  fileService;
    MprHashTable    *fileIndex;
    MprRomInode     *romInodes;
    char            *root;
    int             rootLen;
} MprRomFileService;
#elif BREW


typedef struct MprBrewFileService {
    MprFileService  fileService;
    IFileMgr        *fileMgr;           /* File manager */
} MprBrewFileService;
#else

typedef MprFileService MprDiskFileService;
#endif


/**
 *  File information structure
 *  @description MprFileInfo is the cross platform File information structure.
 *  @ingroup MprFile
 */
typedef struct MprFileInfo {
    int64           size;               /**< File length */
    MprTime         atime;              /**< Access time */
    MprTime         ctime;              /**< Create time */
    MprTime         mtime;              /**< Modified time */
    uint            inode;              /**< Inode number */
    bool            isDir;              /**< Set if directory */
    bool            isReg;              /**< Set if a regular file */
    bool            caseMatters;        /**< Case comparisons matter */
    int             perms;              /**< Permission mask */
    int             valid;              /**< Valid data bit */
} MprFileInfo;


/*
 *  TODO - need a printf routine for MprFile
 */
/**
 *  File Services Module
 *  @description MprFile is the cross platform File I/O abstraction control structure. An instance will be
 *       created when a file is created or opened via #mprOpen.
 *  @stability Evolving.
 *  @see MprFile mprClose mprGetFileInfo mprGets mprMakeDir mprOpen mprPuts mprRead mprDelete mprDeleteDir 
 *      mprSeek mprWrite mprFlush MprFile mprGetBaseName mprGetDirName mprGetExtension mprGetc
 *      mprGetParentDir mprMakeDir mprMakeDirPath mprMakeTempFileName mprGetRelFilename mprCleanFilename
 *      mprGetAbsFilename mprGetFileNewline mprGetFileDelimiter mprGetUnixFilename mprGetWinFilename 
 *      mprMapDelimiters MprFileInfo mprAccess mprCompareFilename mprCopyFile mprDisableFileBuffering
 *      mprEnableFileBuffering mprGetDirList
 *
 *  @defgroup MprFile MprFile
 */
typedef struct MprFile {
#if BLD_DEBUG
    cchar           *path;              /**< Filename */
#endif
    MprBuf          *buf;               /**< Buffer for I/O if buffered */
    MprOffset       pos;                /**< Current read position  */
    MprOffset       size;               /**< Current file size */
    int             mode;               /**< File open mode */
    int             perms;              /**< File permissions */
#if BLD_FEATURE_ROMFS
    MprRomInode     *inode;             /**< Reference to ROM file */
#else
#if BREW
    IFile           *fd;                /**< File handle */
#else
    int             fd;
#endif /* BREW */
#endif /* BLD_FEATURE_ROMFS */
    MprFileService  *fileService;       /**< File system implementaion */
} MprFile;


/**
 *  Directory entry description
 *  @description The MprGetDirList will create a list of directory entries.
 */
typedef struct MprDirEntry {
    char            *name;              /**< Name of the file */
    MprTime         lastModified;       /**< Time the file was last modified */
    MprOffset       size;               /**< Size of the file */
    bool            isDir;              /**< True if the file is a directory */
} MprDirEntry;


/*
 *  File system initialization routines
 */
extern MprFileService *mprCreateFileService(MprCtx ctx);
#if BLD_FEATURE_ROMFS
    extern MprRomFileService *mprCreateRomFileService(MprCtx ctx);
    extern int      mprSetRomFileSystem(MprCtx ctx, MprRomInode *inodeList);
#elif BREW
    extern MprBrewFileService *mprCreateBrewFileService(MprCtx ctx);
#else
    extern MprDiskFileService *mprCreateDiskFileService(MprCtx ctx);
#endif

/**
 *  Determine if a file can be accessed
 *  @description Test if a file can be accessed for a given mode
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to test
 *  @param omode Posix style file open mode mask. See #mprOpen for the various modes.
 *  @returns True if the file exists and can be accessed
 *  @ingroup MprFile
 */
extern bool mprAccess(MprCtx ctx, cchar *filename, int omode);

/**
 *  Copy a file
 *  @description Create a new copy of a file with the specified open permissions mode.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param from Filename of the existing file to copy
 *  @param to Name of the new file copy
 *  @param omode Posix style file open mode mask. See #mprOpen for the various modes.
 *  @returns True if the file exists and can be accessed
 *  @ingroup MprFile
 */
extern int mprCopyFile(MprCtx ctx, cchar *from, cchar *to, int omode);

/**
 *  Delete a file.
 *  @description Delete a file.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename String containing the filename to delete.
 *  @return Returns zero if successful otherwise a negative MPR error code is returned.
 *  @ingroup MprFile
 */
extern int mprDelete(MprCtx ctx, cchar *filename);

/**
 *  Delete a directory.
 *  @description Delete a directory.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename String containing the directory name to delete.
 *  @return Returns zero if successful otherwise a negative MPR error code is returned.
 *  @ingroup MprFile
 */
extern int mprDeleteDir(MprCtx ctx, cchar *filename);

/**
 *  Disable file buffering
 *  @description Disable any buffering of data when using the buffer.
 *  @param file File instance returned from #mprOpen
 *  @ingroup MprFile
 */
extern void mprDisableFileBuffering(MprFile *file);

/**
 *  Enable file buffering
 *  @description Enable data buffering when using the buffer.
 *  @param file File instance returned from #mprOpen
 *  @param size Size to allocate for the buffer.
 *  @param maxSize Maximum size the data buffer can grow to
 *  @ingroup MprFile
 */
extern int mprEnableFileBuffering(MprFile *file, int size, int maxSize);

/**
 *  Get the base portion of a filename
 *  @description Get the base portion of a filename by stripping off all directory components
 *  @param filename File name to examine
 *  @returns A filename without any directory portion. The filename is a reference into the original file string and 
 *      should not be freed. 
 *  @ingroup MprFile
 */
extern cchar *mprGetBaseName(cchar *filename);

/**
 *  Get the directory portion of a filename
 *  @description Get the directory portion of a filename by stripping off the base name.
 *  @param buf Buffer to hold the directory name
 *  @param bufsize Size of buf
 *  @param filename File name to examine
 *  @returns A reference to the buffer passed in the buf parameter.
 *  @ingroup MprFile
 */
extern char *mprGetDirName(char *buf, int bufsize, cchar *filename);

/**
 *  Get the file extension portion of a filename
 *  @description Get the file extension portion of a filename. The file extension is the portion after the last "."
 *      in the filename.
 *  @param filename File name to examine
 *  @returns A filename extension. The extension is a reference into the original file string and should not be freed.
 *  @ingroup MprFile
 */
extern cchar *mprGetExtension(cchar *filename);

/**
 *  Create a directory list of files.
 *  @description Get the list of files in a directory and return a list.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename Directory to list.
 *  @param enumDirs Set to true to enumerate directory entries as well as regular filenames. 
 *  @returns A list (MprList) of directory filenames. Each filename is a regular string owned by the list object.
 *      Use #mprFree to free the memory for the list and directory filenames.
 *  @ingroup MprFile
 */
extern MprList *mprGetDirList(MprCtx ctx, cchar *filename, bool enumDirs);

/**
 *  Return information about a file.
 *  @description Returns file status information regarding the \a filename.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename String containing the filename to query.
 *  @param info Pointer to a pre-allocated MprFileInfo structure.
 *  @return Returns zero if successful, otherwise a negative MPR error code is returned.
 *  @ingroup MprFile
 */
extern int mprGetFileInfo(MprCtx ctx, cchar *filename, MprFileInfo *info);

/**
 *  Return the current file position
 *  @description Return the current read/write file position.
 *  @param file A file object returned from #mprOpen
 *  @returns The current file offset position if successful. Returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern MprOffset mprGetFilePosition(MprFile *file);

/**
 *  Get the size of the file
 *  @description Return the current file size
 *  @param file A file object returned from #mprOpen
 *  @returns The current file size if successful. Returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern MprOffset mprGetFileSize(MprFile *file);

/**
 *  Read a line from the file.
 *  @description Read a single line from the file and advance the read position. Lines are delimited by the 
 *      newline character. The newline is not included in the returned buffer.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param buf Pre-allocated buffer to contain the line of data.
 *  @param size Size of \a buf.
 *  @return The number of characters read into \a buf.
 *  @ingroup MprFile
 */
extern char *mprGets(MprFile *file, char *buf, uint size);

/**
 *  Read a character from the file.
 *  @description Read a single character from the file and advance the read position.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @return If successful, return the character just read. Otherwise return a negative MPR error code.
 *      End of file is signified by reading 0.
 *  @ingroup MprFile
 */
extern int mprGetc(MprFile *file);

/**
 *  Make a directory
 *  @description Make a directory using the supplied path. Intermediate directories are created as required.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path String containing the directory pathname to create.
 *  @param perms Posix style file permissions mask.
 *  @return Returns zero if successful, otherwise a negative MPR error code is returned.
 *  @ingroup MprFile
 */
extern int mprMakeDir(MprCtx ctx, cchar *path, int perms);

/**
 *  Make a directory and all required intervening directories.
 *  @description Make a directory with all the necessary intervening directories.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path Directory path name.
 *  @param perms Posix style file permissions mask.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprFile
 */
extern int mprMakeDirPath(MprCtx ctx, cchar *path, int perms);

/**
 *  Make a temporary filename.
 *  @description Thread-safe way to make a unique temporary filename. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param buf Buffer to hold the name of the filename.
 *  @param bufsize Maximum length for the generated filename.
 *  @param tmpDir Base directory in which the temp file will be allocated.
 *  @return Returns zero if successful. Otherwise it returns MPR_ERR_CANT_CREATE.
 *  @ingroup MprFile
 */
extern int mprMakeTempFileName(MprCtx ctx, char *buf, int bufsize, cchar *tmpDir);

/**
 *  Open a file
 *  @description Open a file and return a file object.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename String containing the filename to open or create.
 *  @param omode Posix style file open mode mask. The open mode may contain 
 *      the following mask values ored together:
 *      @li O_RDONLY Open read only
 *      @li O_WRONLY Open write only
 *      @li O_RDWR Open for read and write
 *      @li O_CREAT Create or re-create
 *      @li O_TRUNC Truncate
 *      @li O_BINARY Open for binary data
 *      @li O_TEXT Open for text data
 *      @li O_EXCL Open with an exclusive lock
 *      @li O_APPEND Open to append
 *  @param perms Posix style file permissions mask.
 *  @return Returns an MprFile object to use in other file operations.
 *  @ingroup MprFile
 */
extern MprFile *mprOpen(MprCtx ctx, cchar *filename, int omode, int perms);

/**
 *  Non-destructively read a character from the file.
 *  @description Read a single character from the file without advancing the read position.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @return If successful, return the character just read. Otherwise return a negative MPR error code.
 *      End of file is signified by reading 0.
 *  @ingroup MprFile
 */
extern int mprPeekc(MprFile *file);

/**
 *  Write a character to the file.
 *  @description Writes a single character to the file. Output is buffered and is
 *      flushed as required or when mprClose is called.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param c Character to write
 *  @return One if successful, otherwise returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern int mprPutc(MprFile *file, int c);

/**
 *  Write a line to the file.
 *  @description Writes a single line to the file. Output is buffered and is
 *      flushed as required or when mprClose is called.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param buf Buffer containing the line to write
 *  @param size Size of \a buf in characters to write
 *  @return The number of characters written to the file. Returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern int mprPuts(MprFile *file, cchar *buf, uint size);

/**
 *  Read data from a file.
 *  @description Reads data from a file. 
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param buf Buffer to contain the read data.
 *  @param size Size of \a buf in characters.
 *  @return The number of characters read from the file. Returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern int mprRead(MprFile *file, void *buf, uint size);

/**
 *  Seek the I/O pointer to a new location in the file.
 *  @description Move the position in the file to/from which I/O will be performed in the file. Seeking prior 
 *      to a read or write will cause the next I/O to occur at that location.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param seekType Seek type may be one of the following three values:
 *      @li SEEK_SET    Seek to a position relative to the start of the file
 *      @li SEEK_CUR    Seek relative to the current position
 *      @li SEEK_END    Seek relative to the end of the file
 *  @param distance A positive or negative byte offset.
 *  @return Returns zero if successful otherwise a negative MPR error code is returned.
 *  @ingroup MprFile
 */
extern long mprSeek(MprFile *file, int seekType, long distance);

/**
 *  Write data to a file.
 *  @description Writes data to a file. 
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @param buf Buffer containing the data to write.
 *  @param count Cound of characters in \a buf to write
 *  @return The number of characters actually written to the file. Returns a negative MPR error code on errors.
 *  @ingroup MprFile
 */
extern int mprWrite(MprFile *file, cvoid *buf, uint count);

/**
 *  Flush any buffered write data
 *  @description Write buffered write data and then reset the internal buffers.
 *  @param file Pointer to an MprFile object returned via MprOpen.
 *  @return Zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprFile
 */
extern int mprFlush(MprFile *file);

/**
 *  Compare two filenames
 *  @description Compare two filenames to see if they are equal. This does not convert filenames to absolute form first,
 *      that is the callers responsibility. It does handle case sensitivity appropriately. The len parameter 
 *      if non-zero, specifies how many characters of the filenames to compare.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path1 First filename to compare
 *  @param path2 Second filename to compare
 *  @param len How many characters to compare.
 *  @returns True if the file exists and can be accessed
 *  @ingroup MprFile
 */
extern int mprCompareFilename(MprCtx ctx, cchar *path1, cchar *path2, int len);

/**
 *  Clean a filename
 *  @description Clean a filename by removing redundant segments. This will remove "./", "../dir" and duplicate 
 *      filename delimiters.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename First filename to compare
 *  @returns A newly allocated, clean filename. Caller should free via #mprFree
 *  @ingroup MprFile
 */
extern char *mprCleanFilename(MprCtx ctx, cchar *filename);

/**
 *  Convert a filename to an absolute filename
 *  @description Get an absolute (canonical) equivalent representation of a filename. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename Filename to examine
 *  @returns An absolute filename. Caller should free via #mprFree
 *  @ingroup MprFile
 */
extern char *mprGetAbsFilename(MprCtx ctx, cchar *filename);

/**
 *  Get the filename directory delimiter.
 *  Return the delimiter character used to separate directories on a given file system. Typically "/" or "\"
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path Use this path to specify either the root of the file system or a file on the file system.
 *  @returns The character used as the filename directory delimiter.
 *  @ingroup MprFile
 */
extern int mprGetFileDelimiter(MprCtx ctx, cchar *path);

/**
 *  Get the file newline character string
 *  Return the character string used to delimit new lines in text files.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path Use this path to specify either the root of the file system or a file on the file system.
 *  @returns A string used to delimit new lines. This is typically "\n" or "\r\n"
 *  @ingroup MprFile
 */
extern cchar *mprGetFileNewline(MprCtx ctx, cchar *path);

//  TODO - inconsistent vs mprGetDirName. I prefer this form.
/**
 *  Get the parent directory of a filename
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to examine
 *  @returns An allocated string containing the parent directory. Caller must free using #mprFree.
 *  @ingroup MprFile
 */
extern char *mprGetParentDir(MprCtx ctx, cchar *filename);

/**
 *  Get a relative filename
 *  @description Get an equivalent filename that is relative to the application's current working directory.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to examine
 *  @returns An allocated string containing the relative directory. Caller must free using #mprFree.
 *  @ingroup MprFile
 */
extern char *mprGetRelFilename(MprCtx ctx, cchar *filename);

/**
 *  Get a filename formatted like a Unix path name
 *  @description Get an equivalent filename that is relative to the application's current working directory and is 
 *      formatted like a Unix path name. This means it will use forward slashes ("/") as the directory delimiter and will
 *      not contain any drive specifications.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to examine
 *  @returns An allocated string containing the new filename. Caller must free using #mprFree.
 *  @ingroup MprFile
 */
extern char *mprGetUnixFilename(MprCtx ctx, cchar *filename);

/**
 *  Get a filename formatted like a Windows path name
 *  @description Get an equivalent filename that is relative to the application's current working directory and is 
 *      formatted like a Windows path name. This means it will use backward slashes ("\") as the directory delimiter 
 *      and will contain a drive specification.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to examine
 *  @returns An allocated string containing the new filename. Caller must free using #mprFree.
 *  @ingroup MprFile
 */
extern char *mprGetWinFilename(MprCtx ctx, cchar *filename);

/**
 *  Map the delimiters in a filename.
 *  @description Map the directory delimiters in a filename to the specified delimiter. This is useful to change from
 *      backward to forward slashes when dealing with Windows filenames.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename File name to examine
 *  @param delimiter Character string to use as a directory delimiter. Typically "/" or "\".
 *  @returns An allocated string containing the parent directory. Caller must free using #mprFree.
 *  @ingroup MprFile
 */
extern void mprMapDelimiters(MprCtx ctx, char *filename, int delimiter);

/*
 *  These are internal APIs
 */
extern void mprSetFileNewline(MprCtx ctx, cchar *filename, cchar *newline);
extern void mprSetFileDelimiter(MprCtx ctx, cchar *filename, int delimiter);

extern char *mprSearchForFile(MprCtx ctx, cchar *file, int flags, cchar *search, ...);


typedef struct MprOsService {
    int             dummy;
} MprOsService;

extern MprOsService *mprCreateOsService(MprCtx ctx);
extern int          mprStartOsService(MprOsService *os);
extern void         mprStopOsService(MprOsService *os);

typedef struct MprModuleService {
    MprList         *modules;
    char            *searchPath;
#if BLD_FEATURE_MULTITHREAD
    struct MprMutex *mutex;
#endif
} MprModuleService;


extern MprModuleService *mprCreateModuleService(MprCtx ctx);
extern int              mprStartModuleService(MprModuleService *os);
extern void             mprStopModuleService(MprModuleService *os);

/**
 *  Module start/stop point function signature
 *  @param mp Module object reference returned from #mprCreateModule
 *  @returns zero if successful, otherwise return a negative MPR error code.
 */ 
typedef int (*MprModuleProc)(struct MprModule *mp);

/**
 *  Loadable Module Service
 *  @description The MPR provides services to load and unload shared libraries.
 *  @see MprModule, mprGetModuleSearchPath, mprSetModuleSearchPath, mprLoadModule, mprUnloadModule, 
 *      mprCreateModule, mprLookupModule, MprModuleProc
 *  @stability Evolving.
 *  @defgroup MprModule MprModule
 */
typedef struct MprModule {
    char            *name;              /**< Unique module name */
    char            *version;           /**< Module version */
    void            *moduleData;        /**< Module specific data */
    void            *handle;            /**< O/S shared library load handle */
    MprModuleProc   start;              /**< Start the module service */
    MprModuleProc   stop;               /**< Stop the module service */
} MprModule;


/**
 *  Loadable module entry point signature. 
 *  @description Loadable modules can have an entry point that is invoked automatically when a module is loaded. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param path Actual path to the module
 *  @return a new MprModule structure for the module. Return NULL if the module can't be initialized.
 *  @ingroup MprModule
 */
typedef MprModule *(*MprModuleEntry)(MprCtx ctx, cchar *path);


/**
 *  Get the module search path
 *  @description Get the directory search path used by the MPR when loading dynamic modules. This is a colon separated (or 
 *      semicolon on Windows) set of directories.
 *  @param ctx Any memory context allocated by the MPR.
 *  @returns The module search path. Caller must not free.
 *  @ingroup MprModule
 */
extern cchar *mprGetModuleSearchPath(MprCtx ctx);

/**
 *  Set the module search path
 *  @description Set the directory search path used by the MPR when loading dynamic modules. This path string must
 *      should be a colon separated (or semicolon on Windows) set of directories. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param searchPath Colon separated set of directories
 *  @returns The module search path. Caller must not free.
 *  @ingroup MprModule
 */
extern void mprSetModuleSearchPath(MprCtx ctx, char *searchPath);


/**
 *  Create a module
 *  @description This call will create a module object for a loadable module. This should be invoked by the 
 *      module itself in its module entry point to register itself with the MPR.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param name Name of the module
 *  @param version Version string of the form: Major.Minor.patch
 *  @param moduleData to associate with this module
 *  @param start Start function to invoke to start module services
 *  @param stop Stop function to invoke to stop module services
 *  @returns A module object for this module
 *  @ingroup MprModule
 */
extern MprModule *mprCreateModule(MprCtx ctx, cchar *name, cchar *version, void *moduleData, MprModuleProc start,
        MprModuleProc stop);

/**
 *  Load a module
 *  @description Load a module into the MPR. This will load a dynamic shared object (shared library) and call the
 *      modules entry point. If the module has already been loaded, it this call will do nothing and will just
 *      return the already defined module object. 
 *  @param ctx Any memory context allocated by the MPR.
 *  @param filename Name of the module to load. The module will be searched using the defined module search path 
 *      (see #mprSetModuleSearchPath). The filename may or may not include a platform specific shared library extension such
 *      as .dll, .so or .dylib. By omitting the library extension, code can portably load shared libraries.
 *  @param entryPoint Name of function to invoke after loading the module.
 *  @returns A module object for this module created in the module entry point by calling #mprCreateModule
 *  @ingroup MprModule
 */
extern MprModule *mprLoadModule(MprCtx ctx, cchar *filename, cchar *entryPoint);

/**
 *  Lookup a module
 *  @description Lookup a module by name and return the module object.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param name Name of the module specified to #mprCreateModule.
 *  @returns A module object for this module created in the module entry point by calling #mprCreateModule
 *  @ingroup MprModule
 */
extern MprModule *mprLookupModule(MprCtx ctx, cchar *name);

/**
 *  Unload a module
 *  @description Unload a module from the MPR. This will unload a dynamic shared object (shared library). This routine
 *      is not fully supported by the MPR and is often fraught with issues. A module must usually be completely inactive 
 *      with no allocated memory when it is unloaded.
 *  @param mp Module object returned via #mprLookupModule
 *  @ingroup MprModule
 */
extern void mprUnloadModule(MprModule *mp);

/*
 *  Event flags
 */
#define MPR_EVENT_CONTINUOUS    0x1     /**< Auto reschedule the event */
#define MPR_EVENT_THREAD        0x2     /**< Run proc using pool thread */
#define MPR_EVENT_RUNNING       0x4     /**< Event currently executing */

/**
 *  Event callback function
 *  @ingroup MprEvent
 */
typedef void (*MprEventProc)(void *data, struct MprEvent *event);

/**
 *  Event object
 *  @description The MPR provides a powerful priority based eventing mechanism. Events are described by MprEvent objects
 *      which are created and queued via #mprCreateEvent. Each event may have a priority and may be one-shot or 
 *      be continuously rescheduled according to a specified period. The event subsystem provides the basis for 
 *      callback timers. 
 *  @see MprEvent, mprCreateEvent, mprCreateTimerEvent, mprRescheduleEvent, mprStopContinuousEvent, 
 *      mprRestartContinuousEvent, MprEventProc
 *  @defgroup MprEvent MprEvent
 */
typedef struct MprEvent {
    MprEventProc        proc;           /**< Callback procedure */
    MprTime             timestamp;      /**< When was the event created */
    int                 priority;       /**< Priority 0-99. 99 is highest */
    int                 period;         /**< Reschedule period */
    int                 flags;          /**< Event flags */
    MprTime             due;            /**< When is the event due */
    void                *data;          /**< Event private data */
    struct MprEvent     *next;          /**< Next event linkage */
    struct MprEvent     *prev;          /**< Previous event linkage */
    struct MprEventService *service;    /* Event service */
} MprEvent;


/*
 *  Event service
 */
typedef struct MprEventService {
    MprEvent        eventQ;             /* Event queue */
    MprEvent        timerQ;             /* Queue of future events */
    MprEvent        taskQ;              /* Task queue */
    MprTime         lastEventDue;       /* When the last event is due */
    MprTime         lastRan;            /* When last checked queues */
    MprTime         now;                /* Current notion of time */
    int             eventCounter;       /* Incremented for each event (wraps) */

#if BLD_FEATURE_MULTITHREAD
    struct MprSpin  *spin;              /* Multi-thread sync */
#endif

} MprEventService;


extern MprEventService *mprCreateEventService(MprCtx ctx);
extern int      mprStartEventService(MprEventService *ts);
extern int      mprStopEventService(MprEventService *ts);
extern int      mprGetEventCounter(MprEventService *es);

/*
 *  ServiceEvents parameters
 */
#define MPR_SERVICE_ONE_THING   0x1     /**< Wait for one event or one I/O */

/**
 *  Service events
 *  @description Service the event queue. This call will block for the given delay until an event is ready to be
 *      serviced. Flags may modify the calls behavior.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param delay Time in milliseconds to block until an event occurs.
 *  @param flags If set to MPR_SERVICE_ONE_THING, this call will service at most one event. Otherwise set to zero.
 *  @returns A count of the number of events serviced
 *  @ingroup MprEvent
 */
extern int mprServiceEvents(MprCtx ctx, int delay, int flags);

extern void     mprDoEvent(MprEvent *event, void *poolThread);
extern MprEvent *mprGetNextEvent(MprEventService *es);
extern int      mprGetIdleTime(MprEventService *es);

/**
 *  Create a new event
 *  @description Create and queue a new event for service
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param proc Function to invoke when the event is run
 *  @param period Time in milliseconds used by continuous events between firing of the event.
 *  @param priority Priority to associate with the event. Priorities are integer values between 0 and 100 inclusive with
 *      50 being a normal priority. Useful constants are: 
 *      @li MPR_LOW_PRIORITY
 *      @li MPR_NORMAL_PRIORITY
 *      @li MPR_HIGH_PRIORITY
 *  @param data Data to associate with the event and stored in event->data.
 *  @param flags Flags to modify the behavior of the event. Valid values are: MPR_EVENT_CONTINUOUS to create an 
 *      event which will be automatically rescheduled accoring to the specified period.
 *  @ingroup MprEvent
 */
extern MprEvent *mprCreateEvent(MprCtx ctx, MprEventProc proc, int period, int priority, void *data, int flags);

/**
 *  Remove an event
 *  @description Remove a queued event. This is useful to remove continuous events from the event queue.
 *  @param event Event object returned from #mprCreateEvent
 *  @ingroup MprEvent
 */
extern void mprRemoveEvent(MprEvent *event);

/**
 *  Stop an event
 *  @description Stop a continuous event and remove from the queue. The event object is not freed, but simply removed
 *      from the event queue.
 *  @param event Event object returned from #mprCreateEvent
 *  @ingroup MprEvent
 */
extern void mprStopContinuousEvent(MprEvent *event);

/**
 *  Restart an event
 *  @description Restart a continuous event after it has been stopped via #mprStopContinuousEvent. This call will 
 *      add the event to the event queue and it will run after the configured event period has expired.
 *  @param event Event object returned from #mprCreateEvent
 *  @ingroup MprEvent
 */
extern void mprRestartContinuousEvent(MprEvent *event);

/**
 *  Create a timer event
 *  @description Create and queue a timer event for service. This is a convenience wrapper to create continuous
 *      events over the #mprCreateEvent call.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param proc Function to invoke when the event is run
 *  @param period Time in milliseconds used by continuous events between firing of the event.
 *  @param priority Priority to associate with the event. Priorities are integer values between 0 and 100 inclusive with
 *      50 being a normal priority. Useful constants are: 
 *      @li MPR_LOW_PRIORITY
 *      @li MPR_NORMAL_PRIORITY
 *      @li MPR_HIGH_PRIORITY
 *  @param data Data to associate with the event and stored in event->data.
 *  @param flags Not used.
 *  @ingroup MprEvent
 */
extern MprEvent *mprCreateTimerEvent(MprCtx ctx, MprEventProc proc, int period, int priority, void *data, int flags);

/**
 *  Reschedule an event
 *  @description Reschedule a continuous event by modifying its period.
 *  @param event Event object returned from #mprCreateEvent
 *  @param period Time in milliseconds used by continuous events between firing of the event.
 *  @ingroup MprEvent
 */
extern void mprRescheduleEvent(MprEvent *event, int period);

#if BLD_FEATURE_XML
/*
 *  XML parser states. The states that are passed to the user handler have "U" appended to the comment.
 *  The error states (ERR and EOF) must be negative.
 */
#define MPR_XML_ERR                 -1      /* Error */
#define MPR_XML_EOF                 -2      /* End of input */
#define MPR_XML_BEGIN               1       /* Before next tag               */
#define MPR_XML_AFTER_LS            2       /* Seen "<"                      */
#define MPR_XML_COMMENT             3       /* Seen "<!--" (usr)        U    */
#define MPR_XML_NEW_ELT             4       /* Seen "<tag" (usr)        U    */
#define MPR_XML_ATT_NAME            5       /* Seen "<tag att"               */
#define MPR_XML_ATT_EQ              6       /* Seen "<tag att" =             */
#define MPR_XML_NEW_ATT             7       /* Seen "<tag att = "val"   U    */
#define MPR_XML_SOLO_ELT_DEFINED    8       /* Seen "<tag../>"          U    */
#define MPR_XML_ELT_DEFINED         9       /* Seen "<tag...>"          U    */
#define MPR_XML_ELT_DATA            10      /* Seen "<tag>....<"        U    */
#define MPR_XML_END_ELT             11      /* Seen "<tag>....</tag>"   U    */
#define MPR_XML_PI                  12      /* Seen "<?processingInst"  U    */
#define MPR_XML_CDATA               13      /* Seen "<![CDATA["         U    */

/*
 *  Lex tokens
 */
typedef enum MprXmlToken {
    MPR_XMLTOK_ERR,
    MPR_XMLTOK_TOO_BIG,                     /* Token is too big */
    MPR_XMLTOK_CDATA,
    MPR_XMLTOK_COMMENT,
    MPR_XMLTOK_INSTRUCTIONS,
    MPR_XMLTOK_LS,                          /* "<" -- Opening a tag */
    MPR_XMLTOK_LS_SLASH,                    /* "</" -- Closing a tag */
    MPR_XMLTOK_GR,                          /* ">" -- End of an open tag */
    MPR_XMLTOK_SLASH_GR,                    /* "/>" -- End of a solo tag */
    MPR_XMLTOK_TEXT,
    MPR_XMLTOK_EQ,
    MPR_XMLTOK_EOF,
    MPR_XMLTOK_SPACE,
} MprXmlToken;

typedef int (*MprXmlHandler)(struct MprXml *xp, int state, cchar *tagName, cchar* attName, cchar* value);
typedef int (*MprXmlInputStream)(struct MprXml *xp, void *arg, char *buf, int size);

/*
 *  Per XML session structure
 */
typedef struct MprXml {
    MprXmlHandler       handler;            /* Callback function */
    MprXmlInputStream   readFn;             /* Read data function */
    MprBuf              *inBuf;             /* Input data queue */
    MprBuf              *tokBuf;            /* Parsed token buffer */
    int                 quoteChar;          /* XdbAtt quote char */
    int                 lineNumber;         /* Current line no for debug */
    void                *parseArg;          /* Arg passed to mprXmlParse() */
    void                *inputArg;          /* Arg for mprXmlSetInputStream() */
    char                *errMsg;            /* Error message text */
} MprXml;

extern MprXml   *mprXmlOpen(MprCtx ctx, int initialSize, int maxSize);
extern void     mprXmlSetParserHandler(MprXml *xp, MprXmlHandler h);
extern void     mprXmlSetInputStream(MprXml *xp, MprXmlInputStream s, void *arg);
extern int      mprXmlParse(MprXml *xp);
extern void     mprXmlSetParseArg(MprXml *xp, void *parseArg);
extern void     *mprXmlGetParseArg(MprXml *xp);
extern cchar    *mprXmlGetErrorMsg(MprXml *xp);
extern int      mprXmlGetLineNumber(MprXml *xp);

#endif /* BLD_FEATURE_XML */


#if BLD_FEATURE_MULTITHREAD
/**
 *  Multithreaded Synchronization Services
 *  @see MprMutex, mprCreateStaticLock, mprFree, mprLock, mprTryLock, mprUnlock, mprGlobalLock, mprGlobalUnlock, 
 *      MprSpin, mprCreateSpinLock, MprCond, mprCreateCond, mprWaitForCond, mprSignalCond, mprFree
 *  @stability Evolving.
 *  @defgroup MprSynch MprSynch
 */
typedef struct MprSynch { int dummy; } MprSynch;

/**
 *  Multithreading lock control structure
 *  @description MprMutex is used for multithread locking in multithreaded applications.
 *  @ingroup MprSynch
 */
typedef struct MprMutex {
    #if BLD_WIN_LIKE
        CRITICAL_SECTION cs;            /**< Internal mutex critical section */
    #elif VXWORKS
        SEM_ID      cs;
    #elif BLD_UNIX_LIKE
        pthread_mutex_t  cs;
    #else
        error("Unsupported OS");
    #endif
} MprMutex;


#if !__UCLIBC__
#define BLD_HAS_SPINLOCK    1
#endif

/**
 *  Multithreading spin lock control structure
 *  @description    MprSpin is used for multithread locking in multithreaded applications.
 *  @ingroup MprSynch
 */
typedef struct MprSpin {
    #if USE_MPR_LOCK
        MprMutex            cs;
    #elif BLD_WIN_LIKE
        CRITICAL_SECTION    cs;            /**< Internal mutex critical section */
    #elif VXWORKS
        SEM_ID              cs;
    #elif MACOSX
        OSSpinLock          cs;
    #elif BLD_UNIX_LIKE && BLD_HAS_SPINLOCK
        pthread_spinlock_t  cs;
    #elif BLD_UNIX_LIKE
        pthread_mutex_t     cs;
    #else
        error("Unsupported OS");
    #endif
    #if BLD_DEBUG
        MprOsThread         owner;
    #endif
} MprSpin;


/**
 *  Create a Mutex lock object.
 *  @description This call creates a Mutex lock object that can be used in #mprLock, #mprTryLock and #mprUnlock calls. 
 *      Use #mprFree to destroy the lock.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @ingroup MprSynch
 */
extern MprMutex *mprCreateLock(MprCtx ctx);

//  TODO - rename mprInitLock
/**
 *  Initialize a statically allocated Mutex lock object.
 *  @description This call initialized a Mutex lock object without allocation. The object can then be used used 
 *      in #mprLock, #mprTryLock and #mprUnlock calls.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param mutex Reference to an MprMutex structure to initialize
 *  @returns A reference to the supplied mutex. Returns null on errors.
 *  @ingroup MprSynch
 */
extern MprMutex *mprCreateStaticLock(MprCtx ctx, MprMutex *mutex);

/**
 *  Attempt to lock access.
 *  @description This call attempts to assert a lock on the given \a lock mutex so that other threads calling 
 *      mprLock or mprTryLock will block until the current thread calls mprUnlock.
 *  @returns Returns zero if the successful in locking the mutex. Returns a negative MPR error code if unsuccessful.
 *  @ingroup MprSynch
 */
extern bool mprTryLock(MprMutex *lock);

/**
 *  Create a spin lock lock object.
 *  @description This call creates a spinlock object that can be used in #mprSpinLock, and #mprSpinUnlock calls. Spin locks
 *      using MprSpin are much faster than MprMutex based locks on some systems.
 *      Use #mprFree to destroy the lock.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @ingroup MprSynch
 */
extern MprSpin *mprCreateSpinLock(MprCtx ctx);


//  TODO - rename mprInitSpinLock
/**
 *  Initialize a statically allocated spinlock object.
 *  @description This call initialized a spinlock lock object without allocation. The object can then be used used 
 *      in #mprSpinLock and #mprSpinUnlock calls.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param lock Reference to a static #MprSpin  object.
 *  @returns A reference to the MprSpin object. Returns null on errors.
 *  @ingroup MprSynch
 */
extern MprSpin *mprCreateStaticSpinLock(MprCtx ctx, MprSpin *lock);


/**
 *  Attempt to lock access on a spin lock
 *  @description This call attempts to assert a lock on the given \a spin lock so that other threads calling 
 *      mprSpinLock or mprTrySpinLock will block until the current thread calls mprSpinUnlock.
 *  @returns Returns zero if the successful in locking the spinlock. Returns a negative MPR error code if unsuccessful.
 *  @ingroup MprSynch
 */
extern bool mprTrySpinLock(MprSpin *lock);

/*
 *  For maximum performance, use the spin lock/unlock routines macros
 */
#if BLD_USE_LOCK_MACROS && !DOXYGEN
    /*
     *  Spin lock macros
     */
    #if MACOSX
        #define mprSpinLock(lock)   OSSpinLockLock(&((lock)->cs))
        #define mprSpinUnlock(lock) OSSpinLockUnlock(&((lock)->cs))
    #elif BLD_UNIX_LIKE && BLD_HAS_SPINLOCK
        #define mprSpinLock(lock)   pthread_spin_lock(&((lock)->cs))
        #define mprSpinUnlock(lock) pthread_spin_unlock(&((lock)->cs))
    #elif BLD_UNIX_LIKE
        #define mprSpinLock(lock)   pthread_mutex_lock(&((lock)->cs))
        #define mprSpinUnlock(lock) pthread_mutex_unlock(&((lock)->cs))
    #elif BLD_WIN_LIKE
        #define mprSpinLock(lock)   EnterCriticalSection(&((lock)->cs))
        #define mprSpinUnlock(lock) LeaveCriticalSection(&((lock)->cs))
    #elif VXWORKS
        #define mprSpinLock(lock)   semTake((lock)->cs, WAIT_FOREVER)
        #define mprSpinUnlock(lock) semGive((lock)->cs)
    #endif

    /*
     *  Lock macros
     */
    #if BLD_UNIX_LIKE
        #define mprLock(lock)       pthread_mutex_lock(&((lock)->cs))
        #define mprUnlock(lock)     pthread_mutex_unlock(&((lock)->cs))
    #elif BLD_WIN_LIKE
        #define mprUnlock(lock)     LeaveCriticalSection(&((lock)->cs))
        #define mprLock(lock)       EnterCriticalSection(&((lock)->cs))
    #elif VXWORKS
        #define mprUnlock(lock)     semGive((lock)->cs)
        #define mprLock(lock)       semTake((lock)->cs, WAIT_FOREVER)
    #endif
#else

    /**
     *  Lock access.
     *  @description This call asserts a lock on the given \a lock mutex so that other threads calling mprLock will 
     *      block until the current thread calls mprUnlock.
     *  @ingroup MprSynch
     */
    extern void mprLock(MprMutex *lock);

    /**
     *  Unlock a mutex.
     *  @description This call unlocks a mutex previously locked via mprLock or mprTryLock.
     *  @ingroup MprSynch
     */
    extern void mprUnlock(MprMutex *lock);

    /**
     *  Lock a spinlock.
     *  @description This call asserts a lock on the given \a spinlock so that other threads calling mprSpinLock will
     *      block until the curren thread calls mprSpinUnlock.
     *  @ingroup MprSynch
     */
    extern void mprSpinLock(MprSpin *lock);

    /**
     *  Unlock a spinlock.
     *  @description This call unlocks a spinlock previously locked via mprSpinLock or mprTrySpinLock.
     *  @ingroup MprSynch
     */
    extern void mprSpinUnlock(MprSpin *lock);
#endif

/**
 *  Globally lock the application.
 *  @description This call asserts the application global lock so that other threads calling mprGlobalLock will 
 *      block until the current thread calls mprGlobalUnlock.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @ingroup MprSynch
 */
extern void mprGlobalLock(MprCtx ctx);

/**
 *  Unlock the global mutex.
 *  @description This call unlocks the global mutex previously locked via mprGlobalLock.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @ingroup MprSynch
 */
extern void mprGlobalUnlock(MprCtx ctx);

/**
 *  Condition variable for multi-thread synchronization. Condition variables can be used to coordinate threads 
 *  when running in a multi-threaded mode. These variables are level triggered in that a condition can be 
 *  signalled prior to another thread waiting. That thread will then not block if it calls waitForCond().
 *  @ingroup MprSynch
 */
typedef struct MprCond {
    #if BLD_UNIX_LIKE
        pthread_cond_t cv;              /**< Unix pthreads condition variable */
    #elif BLD_WIN_LIKE
        HANDLE      cv;                 /* Windows event handle */
        int         numWaiting;         /* Number waiting to be signalled */
    #elif VXWORKS
        SEM_ID      cv;                 /* Condition variable */
    #else
        error("Unsupported OS");
    #endif
        MprMutex    *mutex;             /**< Thread synchronization mutex */
        int         triggered;          /**< Value of the condition */
} MprCond;


/**
 *  Create a condition lock variable.
 *  @description This call creates a condition variable object that can be used in #mprWaitForCond and #mprSignalCond calls. 
 *      Use #mprFree to destroy the condition variable.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @ingroup MprSynch
 */
extern MprCond *mprCreateCond(MprCtx ctx);

/**
 *  Wait for a condition lock variable.
 *  @description Wait for a condition lock variable to be signaled. If the condition is signaled before the timeout
 *      expires this call will reset the condition variable and return. This way, it automatically resets the variable
 *      for future waiters.
 *  @param cond Condition variable object created via #mprCreateCond
 *  @param timeout Time in milliseconds to wait for the condition variable to be signaled.
 *  @ingroup MprSynch
 */
extern int mprWaitForCond(MprCond *cond, int timeout);

/**
 *  Signal a condition lock variable.
*   @description Signal a condition variable and set it to the \a triggered status. Existing or future callers of
*       #mprWaitForCond will be awakened.
 *  @param cond Condition variable object created via #mprCreateCond
 *  @ingroup MprSynch
 */
extern void mprSignalCond(MprCond *cond);


/*
 *  Thread service
 */
typedef struct MprThreadService {
    MprList         *threads;           /* List of all threads */
    struct MprThread *mainThread;       /* Main application Mpr thread id */
    MprMutex        *mutex;             /* Multi-thread sync */
    int             stackSize;          /* Default thread stack size */
} MprThreadService;


typedef void (*MprThreadProc)(void *arg, struct MprThread *tp);

extern MprThreadService *mprCreateThreadService(struct Mpr *mpr);
extern int mprStartThreadService(MprThreadService *ts);
extern int mprStopThreadService(MprThreadService *ts, int timeout);

/**
 *  Thread Service. 
 *  @description The MPR provides a cross-platform thread abstraction above O/S native threads. It supports 
 *      arbitrary thread creation, thread priorities, thread management and thread local storage. By using these
 *      thread primitives with the locking and synchronization primitives offered by #MprMutex, #MprSpin and 
 *      #MprCond - you can create cross platform multi-threaded applications.
 *  @stability Evolving
 *  @see MprThread, mprCreateThread, mprStartThread, mprGetThreadName, mprGetThreadPriority, 
 *      mprSetThreadPriority, mprGetCurrentThread, mprGetCurrentOsThread, mprSetThreadPriority, 
 *      mprSetThreadData, mprGetThreadData, mprCreateThreadLocal
 *  @defgroup MprThread MprThread
 */
typedef struct MprThread {
    MprOsThread     osThreadID;         /**< O/S thread id */

#if BLD_WIN_LIKE
    handle          threadHandle;       /**< Threads OS handle for WIN */
#endif
    void            *data;              /**< Data argument */
    MprThreadProc   entry;              /**< Users thread entry point */
    char            *name;              /**< Name of thead for trace */
    MprMutex        *mutex;             /**< Multi-thread synchronization */
    ulong           pid;                /**< Owning process id */
    int             priority;           /**< Current priority */
    int             stackSize;          /**< Only VxWorks implements */
} MprThread;


/**
 *  Thread local data storage
 */
typedef struct MprThreadLocal {
#if BLD_UNIX_LIKE
    pthread_key_t   key;                /**< Data key */
#elif BLD_WIN_LIKE
    DWORD           key;
#endif
} MprThreadLocal;


/**
 *  Create a new thread
 *  @description MPR threads are usually real O/S threads and can be used with the various locking services (#MprMutex,
 *      #MprCond, #MprSpin) to enable scalable multithreaded applications.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param name Unique name to give the thread
 *  @param proc Entry point function for the thread. #mprStartThread will invoke this function to start the thread
 *  @param data Thread private data stored in MprThread.data
 *  @param priority Priority to associate with the thread. Mpr thread priorities are are integer values between 0 
 *      and 100 inclusive with 50 being a normal priority. The MPR maps these priorities in a linear fashion onto 
 *      native O/S priorites. Useful constants are: 
 *      @li MPR_LOW_PRIORITY
 *      @li MPR_NORMAL_PRIORITY
 *      @li MPR_HIGH_PRIORITY
 *  @param stackSize Stack size to use for the thread. On VM based systems, increasing this value, does not 
 *      necessarily incurr a real memory (working-set) increase. Set to zero for a default stack size.
 *  @returns A MprThread object
 *  @ingroup MprThread
 */
extern MprThread *mprCreateThread(MprCtx ctx, cchar *name, MprThreadProc proc, void *data, int priority, int stackSize);

/**
 *  Start a thread
 *  @description Start a thread previously created via #mprCreateThread. The thread will begin at the entry function 
 *      defined in #mprCreateThread.
 *  @param thread Thread object returned from #mprCreateThread
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @ingroup MprThread
 */
extern int mprStartThread(MprThread *thread);

/**
 *  Get the thread name.
 *  @description MPR threads are usually real O/S threads and can be used with the various locking services (#MprMutex,
 *      #MprCond, #MprSpin) to enable scalable multithreaded applications.
 *  @param thread Thread object returned from #mprCreateThread
 *  @return Returns a string name for the thread. Caller must not free.
 *  @ingroup MprThread
 */
extern cchar *mprGetThreadName(MprThread *thread);

/**
 *  Get the thread priroity
 *  @description Get the current priority for the specified thread.
 *  @param thread Thread object returned by #mprCreateThread
 *  @returns An integer MPR thread priority between 0 and 100 inclusive.
 *  @ingroup MprThread
 */
extern int mprGetThreadPriority(MprThread *thread);

/**
 *  Set the thread priroity
 *  @description Set the current priority for the specified thread.
 *  @param thread Thread object returned by #mprCreateThread
 *  @param priority Priority to associate with the thread. Mpr thread priorities are are integer values between 0 
 *      and 100 inclusive with 50 being a normal priority. The MPR maps these priorities in a linear fashion onto 
 *      native O/S priorites. Useful constants are: 
 *      @li MPR_LOW_PRIORITY
 *      @li MPR_NORMAL_PRIORITY
 *      @li MPR_HIGH_PRIORITY
 *  @ingroup MprThread
 */
extern void mprSetThreadPriority(MprThread *thread, int priority);

/**
 *  Get the currently executing thread.
 *  @description Get the thread object for the currently executing O/S thread.
 *  @param ctx Any memory context allocated by the MPR.
 *  @return Returns a thread object representing the current O/S thread.
 *  @ingroup MprThread
 */
extern MprThread *mprGetCurrentThread(MprCtx ctx);

/**
 *  Get the O/S thread
 *  @description Get the O/S thread ID for the currently executing thread.
 *  @return Returns a platform specific O/S thread ID. On Unix, this is a pthread reference. On other systems it is
 *      a thread integer value.
 *  @ingroup MprThread
 */
extern MprOsThread mprGetCurrentOsThread();

/**
 *  Set the thread priroity for the current thread.
 *  @param ctx Any memory context allocated by the MPR.
 *  @description Set the current priority for the specified thread.
 *  @param priority Priority to associate with the thread. Mpr thread priorities are are integer values between 0 
 *      and 100 inclusive with 50 being a normal priority. The MPR maps these priorities in a linear fashion onto 
 *      native O/S priorites. Useful constants are: 
 *      @li MPR_LOW_PRIORITY
 *      @li MPR_NORMAL_PRIORITY
 *      @li MPR_HIGH_PRIORITY
 *  @ingroup MprThread
 */
extern void mprSetCurrentThreadPriority(MprCtx ctx, int priority);

/*
 *  Somewhat internal APIs
 */
extern int mprMapMprPriorityToOs(int mprPriority);
extern int mprMapOsPriorityToMpr(int nativePriority);
extern void mprSetThreadStackSize(MprCtx ctx, int size);
extern int mprSetThreadData(MprThreadLocal *tls, void *value);
extern void *mprGetThreadData(MprThreadLocal *tls);
extern MprThreadLocal *mprCreateThreadLocal();

#else /* !BLD_FEATURE_MULTITHREAD */

typedef struct MprThreadLocal {
    int             dummy;
} MprThreadLocal;
typedef void *MprThread;

#define mprInitThreads(ctx, mpr)
#define mprTermThreads(mpr)
#define mprCreateLock(ctx)
#define mprLock(lock)
#define mprTryLock(lock)
#define mprUnlock(lock)
#define mprCreateSpinLock(ctx)
#define mprSpinLock(lock)
#define mprTrySpinLock(lock)
#define mprSpinUnlock(lock)
#define mprGlobalLock(mpr)
#define mprGlobalUnlock(mpr)
#define mprSetThreadData(tls, value)
#define mprGetThreadData(tls) NULL
#define mprCreateThreadLocal(ejs) ((void*) 1)
#endif /* BLD_FEATURE_MULTITHREAD */

extern cchar        *mprGetCurrentThreadName(MprCtx ctx);

/*
 *  Magic number to identify blocks. Only used in debug mode.
 */
#define MPR_ALLOC_MAGIC     0xe814ecab

/**
 *  Memory allocation error callback. Notifiers are called if mprSetNotifier has been called on a context and a 
 *  memory allocation fails. All notifiers up the parent context chain are called in order.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param size Size of memory allocation request that failed
 *  @param total Total memory allocations so far
 *  @param granted Set to true if the request was actually granted, but the application is now exceeding its redline
 *      memory limit.
 *  @ingroup MprMem
 */
typedef void (*MprAllocNotifier)(MprCtx ctx, uint size, uint total, bool granted);

/**
 *  Mpr memory block destructors prototype
 *  @param ctx Any memory context allocated by the MPR.
 *  @return Return zero if the memory was actually freed. Return non-zero to prevent the memory being freed.
 *  @ingroup MprMem
 */
typedef int (*MprDestructor)(MprCtx ctx);

/*
 *  Align blocks on 8 byte boundaries.
 */
#define MPR_ALLOC_ALIGN(x)  (((x) + 7 ) & ~7)
#define MPR_PAGE_ALIGN(x, pagesize) (((x) + (pagesize) - 1) & ~(pagesize - 1))

#if BLD_DEBUG
#define BLD_FEATURE_MEMORY_DEBUG    1       /* Enable memory debug assist. Fill blocks, verifies block integrity. */
#define BLD_FEATURE_MEMORY_STATS    1       /* Include memory stats routines */
#else
#define BLD_FEATURE_MEMORY_STATS    1
#endif

/*
 *  MprBlk flags
 */
#define MPR_ALLOC_HAS_DESTRUCTOR    0x1     /* Block has a destructor to be called when freed */
#define MPR_ALLOC_HAS_ERROR         0x2     /* Memory context has had allocation errors */
#define MPR_ALLOC_IS_HEAP           0x4     /* Block is a heap context */
#define MPR_ALLOC_FROM_MALLOC       0x8     /* Block allocated from a malloc heap */

/**
 *  Memory Allocation Block Header.
 *  @ingroup MprMem
 */
typedef struct MprBlk {
    struct MprBlk   *parent;                /* Parent block */
    struct MprBlk   *children;              /* First child block. Flags stored in low order bits. */
    struct MprBlk   *next;                  /* Next sibling. Flags stored in low order bits. */

    uint            size: 28;               /* Size of the block (not counting header) */
    uint            flags: 4;               /* Flags */

#if BLD_FEATURE_MEMORY_DEBUG
    /*
     *  For debug, we don't worry about this bloating the MprBlk and messing up alignment.
     */
    uint            magic;                  /* Unique signature. Messes up alignment for debug */
#endif
} MprBlk;


//  TODO - refactor. Since we now use bit fields in MprBlk - dont need some of these
#define MPR_ALLOC_HDR_SIZE      (MPR_ALLOC_ALIGN(sizeof(struct MprBlk)))
#define MPR_GET_BLK(ptr)        ((MprBlk*) (((char*) (ptr)) - MPR_ALLOC_HDR_SIZE))
#define MPR_GET_PTR(bp)         ((char*) (((char*) (bp)) + MPR_ALLOC_HDR_SIZE))
#define MPR_GET_BLK_SIZE(bp)    ((bp)->size)
#define MPR_SET_SIZE(bp, len)   ((bp)->size = len)
#define mprGetBlockSize(ptr)    ((ptr) ? (MPR_GET_BLK_SIZE(MPR_GET_BLK(ptr)) - MPR_ALLOC_HDR_SIZE): 0)

/*
 *  Region of memory. Regions are used to describe chunks of memory used by Heaps.
 */
typedef struct MprRegion {
    struct MprRegion *next;                 /* Next region in chain */
    char            *memory;                /* Region memory data */
    char            *nextMem;               /* Pointer to next free byte in memory */
    int             vmSize;                 /* Size of virtual memory containing the region struct plus region memory */
    int             size;                   /* Original size of region */
    int             remaining;              /* Remaining bytes in the region */
} MprRegion;


/*
 *  Heap flags
 */
#define MPR_ALLOC_PAGE_HEAP     0x1         /* Page based heap. Used for allocating arenas and slabs */
#define MPR_ALLOC_ARENA_HEAP    0x2         /* Heap is an arena. All allocations are done from one or more regions */
#define MPR_ALLOC_SLAB_HEAP     0x4         /* Heap is a slab. Constant sized objects use slab heaps */
#define MPR_ALLOC_FREE_CHILDREN 0x8         /* Heap must be accessed in a thread safe fashion */
#define MPR_ALLOC_THREAD_SAFE   0x10        /* Heap must be accessed in a thread safe fashion */

/*
 *  The heap context supports arena and slab based allocations. Layout of allocated heap blocks:
 *      HDR
 *      MprHeap
 *      MprRegion
 *      Heap Data
 *      Destructor
 */
typedef struct MprHeap {
    cchar           *name;                  /* Debugging name of the heap */
    MprDestructor   destructor;             /* Heap destructor routine */
    MprRegion       *region;                /* Current region of memory for allocation */
    MprRegion       *depleted;              /* Depleted regions. All useful memory has been allocated */
    int             flags;                  /* Heap flags */

    /*
     *  Slab allocation object information and free list
     */
    int             objSize;                /* Size of each heap object */
    MprBlk          *freeList;              /* Linked list of free objects */

    /*
     *  Heap stats
     */
    int            allocBytes;             /* Number of bytes allocated for this heap */
    int            peakAllocBytes;         /* Peak allocated (max allocBytes) */
    int            allocBlocks;            /* Number of alloced blocks for this heap */
    int            peakAllocBlocks;        /* Peak allocated blocks */
    int            totalAllocCalls;        /* Total count of allocation calls */
    int            freeListCount;          /* Count of objects on freeList */
    int            peakFreeListCount;      /* Peak count of blocks on the free list */
    int            reuseCount;             /* Count of allocations from the freelist */
    int            reservedBytes;          /* Virtual allocations for page heaps */

    MprAllocNotifier notifier;              /* Memory allocation failure callback */

#if BLD_FEATURE_MULTITHREAD
    MprSpin         spin;
#endif
} MprHeap;


/*
 *  Memory allocation control
 */
typedef struct MprAlloc {
    MprHeap         pageHeap;               /* Page based heap for Arena allocations */
    uint            pageSize;               /* System page size */
    int             inAllocException;       /* Recursive protect */
    uint            bytesAllocated;         /* Bytes currently allocated */
    uint            peakAllocated;          /* Peak bytes allocated */
    uint            errors;                 /* Allocation errors */
    uint            peakStack;              /* Peak stack usage */
    uint            numCpu;                 /* Number of CPUs */
    uint            redLine;                /* Warn if allocation exceeds this level */
    uint            maxMemory;              /* Max memory to allocate */
    void            *stackStart;            /* Start of app stack */
} MprAlloc;


#if BLD_WIN_LIKE || VXWORKS
#define MPR_MAP_READ        0x1
#define MPR_MAP_WRITE       0x2
#define MPR_MAP_EXECUTE     0x4
#else
#define MPR_MAP_READ        PROT_READ
#define MPR_MAP_WRITE       PROT_WRITE
#define MPR_MAP_EXECUTE     PROT_EXECUTE
#endif

extern struct Mpr *mprCreateAllocService(MprAllocNotifier cback, MprDestructor destructor);
extern MprHeap  *mprAllocArena(MprCtx ctx, cchar *name, uint arenaSize, bool threadSafe, MprDestructor destructor);
extern MprHeap  *mprAllocSlab(MprCtx ctx, cchar *name, uint objSize, uint count, bool threadSafe, MprDestructor destructor);
extern void     mprSetAllocNotifier(MprCtx ctx, MprAllocNotifier cback);
extern void     mprInitBlock(MprCtx ctx, void *ptr, uint size);

/**
 *  Allocate a block of memory
 *  @description Allocates a block of memory using the supplied memory context \a ctx as the parent. #mprAlloc 
 *      manages a tree structure of memory blocks. Freeing a block via mprFree will release the allocated block
 *      and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param size Size of the memory block to allocate.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @ingroup MprMem
 */
extern void *mprAlloc(MprCtx ctx, uint size);

//  TODO - Refactor all this naming. the naming with respect to mprAllocObj
/**
 *  Allocate an object block of memory
 *  @description Allocates a block of memory using the supplied memory context \a ctx as the parent. #mprAllocObject
 *      associates a destructor function with an object. This function will be invoked when the object is freed. 
 *      Freeing a block will first call the destructor and if that returns zero, mprFree will release the allocated 
 *      block and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param size Size of the memory block to allocate.
 *  @param destructor Destructor function to invoke when the allocation is freed via #mprFree.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
extern void *mprAllocObject(MprCtx ctx, uint size, MprDestructor destructor);

/**
 *  Allocate an object block of memory and zero it.
 *  @description Allocates a zeroed block of memory using the supplied memory context \a ctx as the parent. #mprAllocObject
 *      associates a destructor function with an object. This function will be invoked when the object is freed. 
 *      Freeing a block will first call the destructor and if that returns zero, mprFree will release the allocated 
 *      block and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param size Size of the memory block to allocate.
 *  @param destructor Destructor function to invoke when the allocation is freed via #mprFree.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
extern void *mprAllocObjectZeroed(MprCtx ctx, uint size, MprDestructor destructor);

/**
 *  Allocate a zeroed block of memory
 *  @description Allocates a zeroed block of memory using the supplied memory context \a ctx as the parent. #mprAlloc 
 *      manages a tree structure of memory blocks. Freeing a block via mprFree will release the allocated block
 *      and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param size Size of the memory block to allocate.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @ingroup MprMem
 */
extern void *mprAllocZeroed(MprCtx ctx, uint size);

/**
 *  Reallocate a block
 *  @description Reallocates a block increasing its size. If the specified size is less than the current block size,
 *      the call will ignore the request and simply return the existing block.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param ptr Memory to reallocate. If NULL, call malloc.
 *  @param size New size of the required memory block.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to realloc and mprRealloc.
 *  @ingroup MprMem
 */
extern void *mprRealloc(MprCtx ctx, void *ptr, uint size);

/**
 *  Free a block of memory.
 *  @description mprFree should be used to free memory allocated by mprAlloc, or mprRealloc. This call will ignore
 *      calls to free a null pointer, thus it is an acceptable idiom to free a pointer without testing its value for null.
 *      When mprFree is called it will first invoke any object destructor function for the allocated block. If this
 *      destructor returns zero, it will then proceed and free all allocated children before finally releasing the block.
 *  @param ptr Memory to free. If NULL, take no action.
 *  @return Returns zero if the block was actually freed. If the destructor prevented the freeing, a non-zero value will
 *      be returned. 
 *  @ingroup MprMem
 */
extern int mprFree(void *ptr);

/**
 *  Update the destructor for a block of memory.
 *  @description This call updates the destructor for a block of memory allocated via mprAllocObject.
 *  @param ptr Memory to free. If NULL, take no action.
 *  @param destructor Destructor function to invoke when #mprFree is called.
 *  @ingroup MprMem
 */
extern void     mprSetDestructor(void *ptr, MprDestructor destructor);
extern void     mprFreeChildren(void *ptr);
extern int      mprStealBlock(MprCtx ctx, cvoid *ptr);

#if BLD_FEATURE_MEMORY_DEBUG
extern void     mprValidateBlock(MprCtx ctx);
#endif
#if BLD_FEATURE_MEMORY_STATS
extern void     mprPrintAllocReport(MprCtx ctx, cchar *msg);
#endif

#if DOXYGEN
typedef void *Type;
//  TODO - refactor these names
/**
 *  Allocate an object of a given type
 *  @description Allocates a block of memory large enough to hold an instance of the specified type. This uses the 
 *      supplied memory context \a ctx as the parent. This is implemented as a macro
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param type Type of the object to allocate
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
void *mprAllocObj(MprCtx ctx, Type type) { return 0; }

/**
 *  Allocate a zeroed object of a given type
 *  @description Allocates a zeroed block of memory large enough to hold an instance of the specified type. This uses the 
 *      supplied memory context \a ctx as the parent. This is implemented as a macro
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param type Type of the object to allocate
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
void *mprAllocObjZeroed(MprCtx ctx, Type type) { return 0; }

/**
 *  Allocate an object of a given type with a destructor
 *  @description Allocates a block of memory large enough to hold an instance of the specified type with a destructor. 
 *      This uses the supplied memory context \a ctx as the parent. This is implemented as a macro.
 *      this call associates a destructor function with an object. This function will be invoked when the object is freed. 
 *      Freeing a block will first call the destructor and if that returns zero, mprFree will release the allocated 
 *      block and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param type Type of the object to allocate
 *  @param destructor Destructor function to invoke when the allocation is freed via #mprFree.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
extern void *mprAllocObjWithDestructor(MprCtx ctx, Type type, MprDestructor destructor)

/**
 *  Allocate a zeroed object of a given type with a destructor
 *  @description Allocates a zeroed block of memory large enough to hold an instance of the specified type with a 
 *      destructor. This uses the supplied memory context \a ctx as the parent. This is implemented as a macro.
 *      this call associates a destructor function with an object. This function will be invoked when the object is freed. 
 *      Freeing a block will first call the destructor and if that returns zero, mprFree will release the allocated 
 *      block and all child blocks.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param type Type of the object to allocate
 *  @param destructor Destructor function to invoke when the allocation is freed via #mprFree.
 *  @return Returns a pointer to the allocated block. If memory is not available the memory exhaustion handler 
 *      specified via mprCreate will be called to allow global recovery.
 *  @remarks Do not mix calls to malloc and mprAlloc.
 *  @stability Prototype. This function names are highly likely to be refactored.
 *  @ingroup MprMem
 */
void *mprAllocObjWithDestructorZeroed(MprCtx ctx, Type type, MprDestructor destructor) { return 0;}

#else
/*
 *  Macros for typed based allocations
 */
#define mprAllocObj(ctx, type) \
    ((type*) mprAlloc(ctx, sizeof(type)))
#define mprAllocObjZeroed(ctx, type) \
    ((type*) mprAllocZeroed(ctx, sizeof(type)))
#define mprAllocObjWithDestructor(ctx, type, destructor) \
    ((type*) mprAllocObject(ctx, sizeof(type), (MprDestructor) destructor))
#define mprAllocObjWithDestructorZeroed(ctx, type, destructor) \
    ((type*) mprAllocObjectZeroed(ctx, sizeof(type), (MprDestructor) destructor))
#endif

/**
 *  Determine if the MPR has encountered memory allocation errors.
 *  @description Returns true if the MPR has had a memory allocation error. Allocation errors occur if any
 *      memory allocation would cause the application to exceed the configured redline limit, or if any O/S memory
 *      allocation request fails.
 *  @param ctx Any memory context allocated by the MPR.
 *  @return TRUE if a memory allocation error has occurred. Otherwise returns FALSE.
 *  @ingroup MprMem
 */
extern bool mprHasAllocError(MprCtx ctx);

/**
 *  Reset the memory allocation error flag
 *  @description Reset the alloc error flag triggered.
 *  @param ctx Any memory context allocated by the MPR.
 *  @ingroup MprMem
 */
extern void mprResetAllocError(MprCtx ctx);

extern void mprSetAllocError(MprCtx ctx);

/**
 *  Get the memory parent of a block.
 *  @description Return the parent memory context for a block
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @return Return the memory owning this block
 *  @ingroup MprMem
 */
extern void *mprGetParent(MprCtx ctx);

extern int      mprIsValid(MprCtx ctx);
extern bool     mprStackCheck(MprCtx ctx);

/**
 *  Configure the application memory limits
 *  @description Configure memory limits to constrain memory usage by the application. The memory allocation subsystem
 *      will check these limits before granting memory allocation requrests. The redLine is a soft limit that if exceeded
 *      will invoke the memory allocation callback, but will still honor the request. The maxMemory limit is a hard limit.
 *      The MPR will prevent allocations which exceed this maximum. The memory callback handler is defined via 
 *      the #mprCreate call.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @param redline Soft memory limit. If exceeded, the request will be granted, but the memory handler will be invoked.
 *  @param maxMemory Hard memory limit. If exceeded, the request will not be granted, and the memory handler will be invoked.
 *  @ingroup MprMem
 */
extern void mprSetAllocLimits(MprCtx ctx, uint redline, uint maxMemory);

extern MprAlloc *mprGetAllocStats(MprCtx ctx);
extern int      mprGetUsedMemory(MprCtx ctx);

/**
 *  Duplicate a block of memory.
 *  @param ctx Any memory context allocated by mprAlloc or mprCreate.
 *  @description Copy a block of memory into a newly allocated block.
 *  @param ptr Pointer to the block to duplicate.
 *  @param size Size of the block to copy.
 *  @return Returns an allocated block. Caller must free via #mprFree.
 *  @ingroup MprMem
 */
extern void *mprMemdup(MprCtx ctx, cvoid *ptr, uint size);

/**
 *  Copy a memory block into an allocated block.
 *  @description Allocate a new memory block of the required size and copy a source block into it. 
 *      The call returns the size of the allocated block.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param dest Pointer to a pointer that will hold the address of the allocated block.
 *  @param destMax Maximum size of the new block.
 *  @param src Block to copy
 *  @param nbytes Size of the \a src block to copy
 *  @return Returns the number of bytes in the allocated block.
 *  @ingroup MprString
 */
extern int mprAllocMemcpy(MprCtx ctx, char **dest, int destMax, cvoid *src, int nbytes);

/*
 *  Memory mapping. Used for stack memory
 */
extern void *mprMapAlloc(uint size, int mode);
extern void mprMapFree(void *ptr, uint size);
extern int mprGetPageSize(MprCtx ctx);

/*
 *  Wait service.
 */
/*
 *  Standard wait for IO options
 */
#define MPR_READABLE            0x2
#define MPR_WRITEABLE           0x4

/*
 *  Wait service flags
 */
#define MPR_BREAK_REQUESTED     0x1         /* Pending wakeup on service thread */
#define MPR_NEED_RECALL         0x2         /* A handler needs to be recalled */

#define MPR_READ_PIPE           0
#define MPR_WRITE_PIPE          1

typedef void    (*MprWaitProc)(void *data, int mask, int isMprPoolThread);

#if BLD_WIN_LIKE
typedef long    (*MprMsgCallback)(HWND hwnd, uint msg, uint wp, long lp);
#endif

typedef struct MprWaitService {
    MprList         *list;                  /* List of handlers */
    int             flags;                  /* State flags */
    int             listGeneration;         /* Generation number for list changes */
    int             maskGeneration;         /* Generation number for mask changes */
    int             lastMaskGeneration;     /* Last generation number for mask changes */
    int             rebuildMasks;           /* IO mask rebuild required */

#if BLD_UNIX_LIKE
    struct pollfd   *fds;                   /* File descriptors to select on */
    int             fdsCount;               /* Count of active fds in array */
    int             fdsSize;                /* Size of fds array */
    int             breakPipe[2];           /* Pipe to wakeup select when multithreaded */
#endif
#if BLD_WIN_LIKE
#if USE_EVENTS
    WSAEVENT        *events;                /* Array of events to select on */
    int             eventsCount;            /* Count of active events in array */
    int             eventsSize;             /* Size of the events array */
    WSAEVENT        breakEvent;             /* Event to wakeup select when multithreaded */
#else
    HWND            hwnd;                   /* Window handle */
    int             socketMessage;          /* Message id for socket events */
    MprMsgCallback  msgCallback;            /* Message handler callback */
#endif
#endif

#if BLD_FEATURE_MULTITHREAD
    MprThread       *serviceThread;         /* Dedicated service thread */
    MprMutex        *mutex;                 /* General multi-thread sync */
#endif

} MprWaitService;


extern MprWaitService *mprCreateWaitService(struct Mpr *mpr);
extern int  mprInitSelectWait(MprWaitService *ws);
extern int  mprStartWaitService(MprWaitService *ws);
extern int  mprStopWaitService(MprWaitService *ws);

#if BLD_WIN_LIKE
extern int  mprInitWindow(MprWaitService *ws);
extern void mprSetWinMsgCallback(MprWaitService *ws, MprMsgCallback callback);
extern void mprServiceWinIO(MprWaitService *ws, int sockFd, int winMask);
#endif

#if BLD_FEATURE_MULTITHREAD
    extern void mprSetWaitServiceThread(MprWaitService *ws, MprThread *thread);
    extern void mprAwakenWaitService(MprWaitService *ws);
#else
    #define mprAwakenWaitService(ws)
#endif

extern int mprWaitForIO(MprWaitService *ws, int timeout);


/*
 *  Handler Flags
 */
#define MPR_WAIT_CLIENT_CLOSED  0x2     /* Client disconnection received */
#define MPR_WAIT_RECALL_HANDLER 0x4     /* Must recall the handler asap */
#define MPR_WAIT_THREAD         0x8     /* Run callback via thread pool */

/**
 *  Wait Handler Service
 *  @description Wait handlers provide callbacks for when I/O events occur. They provide a wait to service many
 *      I/O file descriptors without requiring a thread per descriptor.
 *  @see mprSetWaitInterest, mprWaitForSingleIO, mprSetWaitCallback, mprDisableWaitEvents, mprEnableWaitEvents,
 *      mprRecallWaitHandler, MprWaitHandler, mprCreateEvent, mprServiceEvents, MprEvent
 *  @defgroup MprWaitHandler MprWaitHandler
 */
typedef struct MprWaitHandler {
    int             desiredMask;        /**< Mask of desired events */
    int             disableMask;        /**< Mask of disabled events */
    int             presentMask;        /**< Mask of current events */
    int             fd;                 /**< O/S File descriptor (sp->sock) */
    int             flags;              /**< Control flags */
    void            *handlerData;       /**< Argument to pass to proc */
#if BLD_WIN_LIKE
#if USE_EVENTS
    WSAEVENT        *event;             /**< Wait event handle */
#endif
#endif

#if BLD_FEATURE_MULTITHREAD
    int             priority;           /**< Thread priority */
    struct MprEvent *threadEvent;       /**< Event reference */
#endif

    MprWaitService  *waitService;       /**< Wait service pointer */
    MprWaitProc     proc;               /**< Wait handler procedure */

    struct MprWaitHandler *next;        /**< List linkage */
    struct MprWaitHandler *prev;

} MprWaitHandler;


/**
 *  Create a wait handler
 *  @description Create a wait handler that will be invoked when I/O of interest occurs on the specified file handle
 *      The wait handler is registered with the MPR event I/O mechanism.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param fd File descriptor
 *  @param mask Mask of events of interest. This is made by oring MPR_READABLE and MPR_WRITEABLE
 *  @param proc Callback function to invoke when an I/O event of interest has occurred.
 *  @param data Data item to pass to the callback
 *  @param priority MPR priority to associate with the callback. This is only used if the MPR_WAIT_THREAD is specified
 *      in the flags and the MPR is build multithreaded.
 *  @param flags Flags may be set to MPR_WAIT_THREAD if the callback function should be invoked using a thread from
 *      the thread pool.
 *  @returns A new wait handler registered with the MPR event mechanism
 *  @ingroup MprWaitHandler
 */
extern MprWaitHandler *mprCreateWaitHandler(MprCtx ctx, int fd, int mask, MprWaitProc proc, void *data,
        int priority, int flags);

/**
 *  Disable wait events
 *  @description Disable wait events for a given file descriptor.
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @param wakeup Set to true if it should wakeup the MPR service thread.
 *  @ingroup MprWaitHandler
 */
extern void mprDisableWaitEvents(MprWaitHandler *wp, bool wakeup);

/**
 *  Enable wait events
 *  @description Enable wait events for a given file descriptor.
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @param wakeup Set to true if it should wakeup the MPR service thread. This should normally be set to true.
 *  @ingroup MprWaitHandler
 */
extern void mprEnableWaitEvents(MprWaitHandler *wp, bool wakeup);
extern int  mprInsertWaitHandler(MprWaitService *ws, struct MprWaitHandler *wp);
extern int  mprModifyWaitHandler(MprWaitService *ws, struct MprWaitHandler *wp, bool wakeup);
extern void mprInvokeWaitCallback(MprWaitHandler *wp, void *poolThread);

/**
 *  Recall a wait handler
 *  @description Signal that a wait handler should be recalled a the earliest opportunity. This is useful
 *      when a protocol stack has buffered data that must be processed regardless of whether more I/O occurs. 
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @ingroup MprWaitHandler
 */
extern void mprRecallWaitHandler(MprWaitHandler *wp);
extern void mprRemoveWaitHandler(MprWaitService *ws, struct MprWaitHandler *wp);
extern int  mprRunWaitHandler(MprWaitHandler *wp);

/**
 *  Define the events of interest for a wait handler
 *  @description Define the events of interest for a wait handler. The mask describes whether readable or writable
 *      events should be signalled to the wait handler. Disconnection events are passed via read events.
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @param mask Mask of MPR_READABLE and MPR_WRITEABLE
 *  @ingroup MprWaitHandler
 */
extern void mprSetWaitInterest(MprWaitHandler *wp, int mask);

/**
 *  Define the wait handler callback
 *  @description This updates the callback function for the wait handler. Callback functions are originally specified
 *      via #mprCreateWaitHandler.
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @param proc Callback function to invoke when an I/O event of interest has occurred.
 *  @param mask Mask of MPR_READABLE and MPR_WRITEABLE
 *  @ingroup MprWaitHandler
 */
extern void mprSetWaitCallback(MprWaitHandler *wp, MprWaitProc proc, int mask);

//  TODO - remove either the wp or fd argument
/**
 *  Wait for I/O on an event handler
 *  @description This call will block for a given timeout until I/O of interest occurs on the given 
 *      serviced. Flags may modify the calls behavior.
 *  @param wp Wait handler created vai #mprCreateWaitHandler
 *  @param fd File descriptor to wait on.
 *  @param mask Mask of MPR_READABLE and MPR_WRITEABLE
 *  @param timeout Time in milliseconds to block
 *  @returns Zero on success, otherwise a negative MPR error code.
 *  @ingroup MprWaitHandler
 */
extern int mprWaitForSingleIO(MprWaitHandler *wp, int fd, int mask, int timeout);

/**
 *  Socket I/O callback procedure
 */
typedef void (*MprSocketProc)(void *data, struct MprSocket *sp, int mask, bool isPoolThread);

/**
 *  Socket connection acceptance callback procedure
 */
typedef void (*MprSocketAcceptProc)(void *data, struct MprSocket *sp, cchar *ip, int port);

/*
 *  Socket service provider interface.
 */
typedef struct MprSocketProvider {
    cchar           *name;

#if BLD_FEATURE_SSL
    struct MprSsl   *defaultSsl;
#endif

    struct MprSocket *(*acceptSocket)(struct MprSocket *sp, bool invokeCallback);
    void            (*closeSocket)(struct MprSocket *socket, bool gracefully);
    int             (*configureSsl)(struct MprSsl *ssl);
    int             (*connectSocket)(struct MprSocket *socket, cchar *host, int port, int flags);
    struct MprSocket *(*createSocket)(MprCtx ctx, struct MprSsl *ssl);
    int             (*flushSocket)(struct MprSocket *socket);
    int             (*listenSocket)(struct MprSocket *socket, cchar *host, int port, MprSocketAcceptProc acceptFn, 
                        void *data, int flags);
    int             (*readSocket)(struct MprSocket *socket, void *buf, int len);
    int             (*writeSocket)(struct MprSocket *socket, void *buf, int len);
} MprSocketProvider;


/*
 *  Mpr socket service class
 */
typedef struct MprSocketService {
    int             maxClients;
    int             numClients;

    MprSocketProvider *standardProvider;
    MprSocketProvider *secureProvider;

#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;
#endif
} MprSocketService;


extern MprSocketService *mprCreateSocketService(MprCtx ctx);
extern int  mprStartSocketService(MprSocketService *ss);
extern void mprStopSocketService(MprSocketService *ss);
extern int  mprSetMaxSocketClients(MprCtx ctx, int max);
extern void mprSetSecureProvider(MprCtx ctx, MprSocketProvider *provider);
extern bool mprHasSecureSockets(MprCtx ctx);

/*
 *  Socket close flags
 */
#define MPR_SOCKET_GRACEFUL     1           /* Do a graceful shutdown */

/*
 *  Socket event types
 */
#define MPR_SOCKET_READABLE     0x2
#define MPR_SOCKET_WRITABLE     0x4
#define MPR_SOCKET_EXCEPTION    0x8

/*
 *  Socket Flags
 */
#define MPR_SOCKET_BLOCK        0x1         /**< Use blocking I/O */
#define MPR_SOCKET_BROADCAST    0x2         /**< Broadcast mode */
#define MPR_SOCKET_CLOSED       0x4         /**< MprSocket has been closed */
#define MPR_SOCKET_CONNECTING   0x8         /**< MprSocket has been closed */
#define MPR_SOCKET_DATAGRAM     0x10        /**< Use datagrams */
#define MPR_SOCKET_EOF          0x20        /**< Seen end of file */
#define MPR_SOCKET_LISTENER     0x40        /**< MprSocket is server listener */
#define MPR_SOCKET_NOREUSE      0x80        /**< Dont set SO_REUSEADDR option */
#define MPR_SOCKET_NODELAY      0x100       /**< Disable Nagle algorithm */
#define MPR_SOCKET_THREAD       0x400       /**< Process callbacks on a pool thread */
#define MPR_SOCKET_CLIENT       0x800       /**< Socket is a client */


/**
 *  Socket Service
 *  @description The MPR Socket service provides IPv4 and IPv6 capabilities for both client and server endpoints.
 *  Datagrams, Broadcast and point to point services are supported. The APIs can be used in both blocking and
 *  non-blocking modes.
 *  \n\n
 *  The socket service integrates with the MPR thread pool and eventing services. Socket connections can be handled
 *  by threads from the thread pool for scalable, multithreaded applications.
 *
 *  @stability Evolving
 *  @see MprSocket, mprCreateSocket, mprOpenClientSocket, mprOpenServerSocket, mprCloseSocket, mprFree, mprFlushSocket,
 *      mprWriteSocket, mprWriteSocketString, mprReadSocket, mprSetSocketCallback, mprSetSocketEventMask, 
 *      mprGetSocketBlockingMode, mprGetSocketEof, mprGetSocketFd, mprGetSocketPort, mprGetSocketBlockingMode, 
 *      mprSetSocketNoDelay, mprGetSocketError, mprParseIp, mprSendFileToSocket, mprSetSocketEof, mprSocketIsSecure
 *      mprWriteSocketVector
 *  @defgroup MprSocket MprSocket
 */
typedef struct MprSocket {
    MprSocketService *service;          /**< Socket service */
    MprSocketAcceptProc
                    acceptCallback;     /**< Accept callback */
    void            *acceptData;        /**< User accept callback data */
    int             currentEvents;      /**< Mask of ready events (FD_x) */
    int             error;              /**< Last error */
    int             handlerMask;        /**< Handler events of interest */
    int             handlerPriority;    /**< Handler priority */
    int             interestEvents;     /**< Mask of events to watch for */
    MprSocketProc   ioCallback;         /**< User I/O callback */
    void            *ioData;            /**< User io callback data */
    void            *ioData2;           /**< Secondary user io callback data */
    char            *ipAddr;            /**< Server side ip address */
    char            *clientIpAddr;      /**< Client side ip address */
    int             port;               /**< Port to listen on */
    int             waitForEvents;      /**< Events being waited on */

#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;             /**< Multi-thread sync */
#endif

    MprWaitHandler  *handler;           /**< Wait handler */
    int             fd;                 /**< Actual socket file handle */
    int             flags;              /**< Current state flags */

    MprSocketProvider *provider;        /**< Socket implementation provider */
    struct MprSocket *listenSock;       /**< Listening socket */

    struct MprSslSocket *sslSocket;     /**< Extended ssl socket state. If set, then using ssl */
    struct MprSsl   *ssl;               /**< SSL configuration */
} MprSocket;


/*
 *  Vectored write array
 */
typedef struct MprIOVec {
    char            *start;
    size_t          len;
} MprIOVec;


/**
 *  Flag for mprCreateSocket to use the default SSL provider
 */ 
#define MPR_SECURE_CLIENT ((struct MprSsl*) 1)

//  TODO - some of these names are not very consistent SocketIsSecure
/**
 *  Create a socket
 *  @description Create a new socket
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param ssl An optional SSL context if the socket is to support SSL. Use the #MPR_SECURE_CLIENT define to specify
 *      that mprCreateSocket should use the default SSL provider.
 *  @return A new socket object
 *  @ingroup MprSocket
 */
extern MprSocket *mprCreateSocket(MprCtx ctx, struct MprSsl *ssl);

/**
 *  Open a client socket
 *  @description Open a client connection
 *  @param sp Socket object returned via #mprCreateSocket
 *  @param hostName Host or IP address to connect to.
 *  @param port TCP/IP port number to connect to.
 *  @param flags Socket flags may use the following flags ored together:
 *      @li MPR_SOCKET_BLOCK - to use blocking I/O. The default is non-blocking.
 *      @li MPR_SOCKET_BROADCAST - Use IPv4 broadcast
 *      @li MPR_SOCKET_DATAGRAM - Use IPv4 datagrams
 *      @li MPR_SOCKET_NOREUSE - Set NOREUSE flag on the socket
 *      @li MPR_SOCKET_NODELAY - Set NODELAY on the socket
 *      @li MPR_SOCKET_THREAD - Process callbacks on a separate thread.
 *  @return Zero if the connection is successful. Otherwise a negative MPR error code.
 *  @ingroup MprSocket
 */
extern int mprOpenClientSocket(MprSocket *sp, cchar *hostName, int port, int flags);

/**
 *  Open a server socket
 *  @description Open a server socket and listen for client connections.
 *  @param sp Socket object returned via #mprCreateSocket
 *  @param ipAddr IP address to bind to. Set to 0.0.0.0 to bind to all possible addresses on a given port.
 *  @param port TCP/IP port number to connect to. 
 *  @param acceptFn Callback function to invoke to accept incoming client connections.
 *  @param data Opaque data reference to pass to the accept function.
 *  @param flags Socket flags may use the following flags ored together:
 *      @li MPR_SOCKET_BLOCK - to use blocking I/O. The default is non-blocking.
 *      @li MPR_SOCKET_BROADCAST - Use IPv4 broadcast
 *      @li MPR_SOCKET_DATAGRAM - Use IPv4 datagrams
 *      @li MPR_SOCKET_NOREUSE - Set NOREUSE flag on the socket
 *      @li MPR_SOCKET_NODELAY - Set NODELAY on the socket
 *      @li MPR_SOCKET_THREAD - Process callbacks on a separate thread.
 *  @return Zero if the connection is successful. Otherwise a negative MPR error code.
 *  @ingroup MprSocket
 */
extern int mprOpenServerSocket(MprSocket *sp, cchar *ipAddr, int port, MprSocketAcceptProc acceptFn, void *data, int flags);

/**
 *  Close a socket
 *  @description Close a socket. If the \a graceful option is true, the socket will first wait for written data to drain
 *      before doing a graceful close.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param graceful Set to true to do a graceful close. Otherwise, an abortive close will be performed.
 *  @ingroup MprSocket
 */
extern void mprCloseSocket(MprSocket *sp, bool graceful);

/**
 *  Flush a socket
 *  @description Flush any buffered data in a socket. Standard sockets do not use buffering and this call will do nothing.
 *      SSL sockets do buffer and calling mprFlushSocket will write pending written data.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return A count of bytes actually written. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern int mprFlushSocket(MprSocket *sp);


/**
 *  Write to a socket
 *  @description Write a block of data to a socket. If the socket is in non-blocking mode (the default), the write
 *      may return having written less than the required bytes. 
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param buf Reference to a block to write to the socket
 *  @param len Length of data to write. This may be less than the requested write length if the socket is in non-blocking
 *      mode. Will return a negative MPR error code on errors.
 *  @return A count of bytes actually written. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern int mprWriteSocket(MprSocket *sp, void *buf, int len);

/**
 *  Write to a string to a socket
 *  @description Write a string  to a socket. If the socket is in non-blocking mode (the default), the write
 *      may return having written less than the required bytes. 
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param str Null terminated string to write.
 *  @return A count of bytes actually written. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern int mprWriteSocketString(MprSocket *sp, cchar *str);

/**
 *  Read from a socket
 *  @description Read data from a socket. The read will return with whatever bytes are available. If none and the socket
 *      is in blocking mode, it will block untill there is some data available or the socket is disconnected.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param buf Pointer to a buffer to hold the read data. 
 *  @param size Size of the buffer.
 *  @return A count of bytes actually read. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern int mprReadSocket(MprSocket *sp, void *buf, int size);

/**
 *  Set the socket callback.
 *  @description Define a socket callback function to invoke in response to socket I/O events.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param fn Callback function.
 *  @param data Data to pass with the callback.
 *  @param data2 More data to pass with the callback.
 *  @param mask Bit mask of events of interest. Set to MPR_READABLE and/or MPR_WRITABLE.
 *  @param priority Priority to associate with the event. Priorities are integer values between 0 and 100 inclusive with
 *      50 being a normal priority. (See #MPR_NORMAL_PRIORITY).
 *  @ingroup MprSocket
 */
extern void mprSetSocketCallback(MprSocket *sp, MprSocketProc fn, void *data, void *data2, int mask, int priority);

/**
 *  Define the events of interest for a socket
 *  @description Define an event mask of interest for a socket. The mask is made by oring the MPR_READABLE and MPR_WRITEABLE
 *      flags as requried
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param mask Set to true to do a graceful close. Otherwise, an abortive close will be performed.
 *  @ingroup MprSocket
 */
extern void mprSetSocketEventMask(MprSocket *sp, int mask);

/**
 *  Get the socket blocking mode.
 *  @description Return the current blocking mode setting.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return True if the socket is in blocking mode. Otherwise false.
 *  @ingroup MprSocket
 */
extern bool mprGetSocketBlockingMode(MprSocket *sp);

/**
 *  Test if the other end of the socket has been closed.
 *  @description Determine if the other end of the socket has been closed and the socket is at end-of-file.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return True if the socket is at end-of-file.
 *  @ingroup MprSocket
 */
extern bool mprGetSocketEof(MprSocket *sp);

/**
 *  Get the socket file descriptor.
 *  @description Get the file descriptor associated with a socket.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return The integer file descriptor used by the O/S for the socket.
 *  @ingroup MprSocket
 */
extern int mprGetSocketFd(MprSocket *sp);

/**
 *  Get the port used by a socket
 *  @description Get the TCP/IP port number used by the socket.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return The integer TCP/IP port number used by the socket.
 *  @ingroup MprSocket
 */
extern int mprGetSocketPort(MprSocket *sp);

/**
 *  Set the socket blocking mode.
 *  @description Set the blocking mode for a socket. By default a socket is in non-blocking mode where read / write
 *      calls will not block.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param on Set to zero to put the socket into non-blocking mode. Set to non-zero to enable blocking mode.
 *  @return The old blocking mode if successful or a negative MPR error code.
 *  @ingroup MprSocket
 */
extern int mprSetSocketBlockingMode(MprSocket *sp, bool on);

/**
 *  Set the socket delay mode.
 *  @description Set the socket delay behavior (nagle algorithm). By default a socket will partial packet writes
 *      a little to try to accumulate data and coalesce TCP/IP packages. Setting the delay mode to false may
 *      result in higher performance for interactive applications.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param on Set to non-zero to put the socket into no delay mode. Set to zero to enable the nagle algorithm.
 *  @return The old delay mode if successful or a negative MPR error code.
 *  @ingroup MprSocket
 */
extern int mprSetSocketNoDelay(MprSocket *sp, bool on);

/**
 *  Get a socket error code
 *  @description This will map a Windows socket error code into a posix error code.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return A posix error code. 
 *  @ingroup MprSocket
 */
extern int mprGetSocketError(MprSocket *sp);

//  TODO reverse file and sock args
/**
 *  Send a file to a socket
 *  @description Write the contents of a file to a socket. If the socket is in non-blocking mode (the default), the write
 *      may return having written less than the required bytes. This API permits the writing of data before and after
 *      the file contents. 
 *  @param file File to write to the socket
 *  @param sock Socket object returned from #mprCreateSocket
 *  @param offset offset within the file from which to read data
 *  @param bytes Length of file data to write
 *  @param beforeVec Vector of data to write before the file contents
 *  @param beforeCount Count of entries in beforeVect
 *  @param afterVec Vector of data to write after the file contents
 *  @param afterCount Count of entries in afterCount
 *  @return A count of bytes actually written. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern MprOffset mprSendFileToSocket(MprFile *file, MprSocket *sock, MprOffset offset, int bytes, MprIOVec *beforeVec, 
    int beforeCount, MprIOVec *afterVec, int afterCount);

extern void mprSetSocketEof(MprSocket *sp, bool eof);

/**
 *  Determine if the socket is secure
 *  @description Determine if the socket is using SSL to provide enhanced security.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @return True if the socket is using SSL, otherwise zero.
 *  @ingroup MprSocket
 */
extern bool mprSocketIsSecure(MprSocket *sp);

/**
 *  Write a vector to a socket
 *  @description Do scatter/gather I/O by writing a vector of buffers to a socket.
 *  @param sp Socket object returned from #mprCreateSocket
 *  @param iovec Vector of data to write before the file contents
 *  @param count Count of entries in beforeVect
 *  @return A count of bytes actually written. Return a negative MPR error code on errors.
 *  @ingroup MprSocket
 */
extern int mprWriteSocketVector(MprSocket *sp, MprIOVec *iovec, int count);

extern int mprParseIp(MprCtx ctx, cchar *ipSpec, char **ipAddrRef, int *port, int defaultPort);

#if BLD_FEATURE_SSL
/*
 *  Put these here to reduce namespace clutter, so users who want SSL don't have to include mprSsl.h and thus 
 *  pull in ssl headers.
 */
extern MprModule   *mprLoadSsl(MprCtx ctx, bool lazy);
extern void         mprConfigureSsl(struct MprSsl *ssl);
#endif

#if BLD_FEATURE_MULTITHREAD

#if BLD_DEBUG
//  TODO - rename MprThreadPoolStats
typedef struct MprPoolStats {
    int             maxThreads;         /* Configured max number of threads */
    int             minThreads;         /* Configured minimum */
    int             numThreads;         /* Configured minimum */
    int             maxUse;             /* Max used */
    int             pruneHighWater;     /* Peak thread use in last minute */
    int             idleThreads;        /* Current idle */
    int             busyThreads;        /* Current busy */
} MprPoolStats;
#endif


//  TODO - rename MprThreadPoolService
/**
 *  Thread Pool Service
 *  @description The MPR provides a thread pool for rapid starting and assignment of threads to tasks.
 *  @stability Evolving
 *  @see MprPoolService, mprAvailablePoolThreads, mprSetMaxPoolThreads, mprSetMinPoolThreads
 *  @defgroup MprPoolService MprPoolService
 */
typedef struct MprPoolService {
    int             nextTaskNum;        /* Unique next task number */
    MprList         *runningTasks;      /* List of executing tasks */
    int             stackSize;          /* Stack size for worker threads */
    MprList         *tasks;             /* Prioritized list of pending tasks */

    MprList         *busyThreads;       /* List of threads to service tasks */
    MprList         *idleThreads;       /* List of threads to service tasks */
    int             maxThreads;         /* Max # threads in pool */
    int             maxUseThreads;      /* Max threads ever used */
    int             minThreads;         /* Max # threads in pool */
    MprMutex        *mutex;             /* Per task synchronization */
    int             nextThreadNum;      /* Unique next thread number */
    int             numThreads;         /* Current number of threads in pool */
    int             pruneHighWater;     /* Peak thread use in last minute */
    struct MprEvent *pruneTimer;        /* Timer for excess threads pruner */
} MprPoolService;


extern MprPoolService *mprCreatePoolService(MprCtx ctx);
extern int mprStartPoolService(MprPoolService *ps);
extern void mprStopPoolService(MprPoolService *ps, int timeout);

//  TODO - rename mprGetAvailablePoolThreads
/**
 *  Get the count of available pool threads
 *  Return the count of free threads in the thread pool.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @returns An integer count of pool threads.
 *  @ingroup MprPoolService
 */
extern int  mprAvailablePoolThreads(MprCtx ctx);

extern void mprSetPoolThreadStackSize(MprCtx ctx, int n);

/**
 *  Set the minimum count of pool threads
 *  Set the count of threads the pool will have. This will cause the pool to pre-create at least this many threads.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param count Minimum count of threads to use.
 *  @ingroup MprPoolService
 */
extern void mprSetMinPoolThreads(MprCtx ctx, int count);

/**
 *  Set the maximum count of pool threads
 *  Set the maximum number of pool threads for the MPR. If this number if less than the current number of threads,
 *      excess threads will be gracefully pruned as they exit.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param count Maximum limit of threads to define.
 *  @ingroup MprPoolService
 */
extern void mprSetMaxPoolThreads(MprCtx ctx, int count);

/**
 *  Get the maximum count of pool threads
 *  Get the maximum limit of pool threads. 
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @return The maximum count of pool threads.
 *  @ingroup MprPoolService
 */
extern int mprGetMaxPoolThreads(MprCtx ctx);

#if BLD_DEBUG
extern void mprGetPoolServiceStats(MprPoolService *ps, MprPoolStats *stats);
#endif

/*
 *  State
 */
#define MPR_POOL_THREAD_SLEEPING    0x1
#define MPR_POOL_THREAD_IDLE        0x2
#define MPR_POOL_THREAD_BUSY        0x4
#define MPR_POOL_THREAD_PRUNED      0x8

typedef void        (*MprPoolProc)(void *data, struct MprPoolThread *tp);

/*
 *  Threads in the thread pool
 */
typedef struct MprPoolThread {
    MprPoolProc     proc;               /* Procedure to run */
    void            *data;
    int             priority;
    int             state;
    MprPoolService  *pool;              /* Pool service */

    struct MprThread *thread;           /* Associated thread */
    MprCond         *idleCond;          /* Used to wait for work */
} MprPoolThread;


extern int mprStartPoolThread(MprCtx ctx, MprPoolProc proc, void *data, int priority);

#endif /* BLD_FEATURE_MULTITHREAD */



extern int  mprDecode64(char *buffer, int bufsize, cchar *str);
extern void mprEncode64(char *buffer, int bufsize, cchar *str);
extern char *mprGetMD5Hash(MprCtx ctx, uchar *buf, int len, cchar *prefix);
extern int  mprCalcDigestNonce(MprCtx ctx, char **nonce, cchar *secret, cchar *etag, cchar *realm);
extern int  mprCalcDigest(MprCtx ctx, char **digest, cchar *userName, cchar *password, cchar *realm,
                cchar *uri, cchar *nonce, cchar *qop, cchar *nc, cchar *cnonce, cchar *method);

/**
 *  URI management
 *  @description The MPR provides routines for formatting and parsing URIs. Routines are also provided
 *      to escape dangerous characters for URIs as well as HTML content and shell commands.
 *  @stability Evolving
 *  @see MprHttp, mprFormatUri, mprEscapeCmd, mprEscapeHtml, mprUrlEncode, mprUrlDecode, mprValidateUrl
 *  @defgroup MprUri MprUri
 */
typedef struct MprUri {
    char        *originalUri;           /**< Original URI */
    char        *parsedUriBuf;          /**< Allocated storage for parsed uri */

    /*
     *  These are just pointers into the parsedUriBuf. None of these fields are Url decoded.
     */
    char        *scheme;                /**< URI scheme (http|https|...) */
    char        *host;                  /**< Url host name */
    int         port;                   /**< Port number */
    char        *url;                   /**< Url path name (without scheme, host, query or fragements) */
    char        *ext;                   /**< Document extension */
    char        *query;                 /**< Query string */
    bool        secure;                 /**< Using https */
} MprUri;


/*
 *  Character escaping masks
 */
#define MPR_HTTP_ESCAPE_HTML            0x1
#define MPR_HTTP_ESCAPE_SHELL           0x2
#define MPR_HTTP_ESCAPE_URL             0x4


/**
 *  Parse a URI
 *  @description Parse a uri and return a tokenized MprUri structure.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param uri Uri string to parse
 *  @return A newly allocated MprUri structure. Caller must free using #mprFree.
 *  @ingroup MprUri
 */
extern MprUri *mprParseUri(MprCtx ctx, cchar *uri);

/**
 *  Format a URI
 *  @description Format a URI string using the input components.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param protocol Protocol string for the uri. Example: "http"
 *  @param host Host or IP address
 *  @param port TCP/IP port number
 *  @param path URL path
 *  @param query Additiona query parameters.
 *  @return A newly allocated uri string. Caller must free using #mprFree.
 *  @ingroup MprUri
 */
extern char *mprFormatUri(MprCtx ctx, cchar *protocol, cchar *host, int port, cchar *path, cchar *query);

/**
 *  Encode a string escaping typical command (shell) characters
 *  @description Encode a string escaping all dangerous characters that have meaning for the unix or MS-DOS command shells.
 *  @param buf Buffer to hold the encoded string.
 *  @param len Length of the buffer to hold the encoded string.
 *  @param cmd Command string to encode
 *  @param escChar Escape character to use when encoding the command.
 *  @return A reference to the buf argument.
 *  @ingroup MprUri
 */
extern char *mprEscapeCmd(char *buf, int len, cchar *cmd, int escChar);

/**
 *  Encode a string by escaping typical HTML characters
 *  @description Encode a string escaping all dangerous characters that have meaning in HTML documents
 *  @param buf Buffer to hold the encoded string.
 *  @param len Length of the buffer to hold the encoded string.
 *  @param html HTML content to encode
 *  @return A reference to the buf argument.
 *  @ingroup MprUri
 */
extern char *mprEscapeHtml(char *buf, int len, cchar *html);

/**
 *  Encode a string by escaping URL characters
 *  @description Encode a string escaping all characters that have meaning for URLs.
 *  @param buf Buffer to hold the encoded string.
 *  @param len Length of the buffer to hold the encoded string.
 *  @param url URL to encode
 *  @return A reference to the buf argument.
 *  @ingroup MprUri
 */
extern char *mprUrlEncode(char *buf, int len, cchar *url);

/**
 *  Decode a URL string by de-scaping URL characters
 *  @description Decode a string with www-encoded characters that have meaning for URLs.
 *  @param url Buffer to hold the decoded string.
 *  @param urlSize Length of the buffer to hold the decoded string.
 *  @param buf URL to decode
 *  @return A reference to the buf argument.
 *  @ingroup MprUri
 */
extern char *mprUrlDecode(char *url, int urlSize, cchar *buf);

//  TODO - should not operate in-situ.
/**
 *  Validate a URL
 *  @description Validate and canonicalize a URL. This removes redundant "./" sequences and simplifies "../dir" 
 *      references. This operates in-situ and modifies the existing string.
 *  @param url Url string to validate
 *  @return A reference to the original url.
 *  @ingroup MprUri
 */
extern char *mprValidateUrl(char *url);

#if BLD_FEATURE_HTTP

#define MPR_HTTP_NAME                   "Embedthis-http/" BLD_VERSION

/*
 *  Standard HTTP/1.1 response codes. See url.c for the actual strings used for each code.
 */
#define MPR_HTTP_CODE_CONTINUE                  100
#define MPR_HTTP_CODE_OK                        200
#define MPR_HTTP_CODE_CREATED                   201
#define MPR_HTTP_CODE_ACCEPTED                  202
#define MPR_HTTP_CODE_NOT_AUTHORITATIVE         203
#define MPR_HTTP_CODE_NO_CONTENT                204
#define MPR_HTTP_CODE_RESET                     205
#define MPR_HTTP_CODE_PARTIAL                   206
#define MPR_HTTP_CODE_MOVED_PERMANENTLY         301
#define MPR_HTTP_CODE_MOVED_TEMPORARILY         302
#define MPR_HTTP_CODE_NOT_MODIFIED              304
#define MPR_HTTP_CODE_USE_PROXY                 305
#define MPR_HTTP_CODE_TEMPORARY_REDIRECT        307
#define MPR_HTTP_CODE_BAD_REQUEST               400
#define MPR_HTTP_CODE_UNAUTHORIZED              401
#define MPR_HTTP_CODE_PAYMENT_REQUIRED          402
#define MPR_HTTP_CODE_FORBIDDEN                 403
#define MPR_HTTP_CODE_NOT_FOUND                 404
#define MPR_HTTP_CODE_BAD_METHOD                405
#define MPR_HTTP_CODE_NOT_ACCEPTABLE            406
#define MPR_HTTP_CODE_REQUEST_TIME_OUT          408
#define MPR_HTTP_CODE_CONFLICT                  409
#define MPR_HTTP_CODE_GONE                      410
#define MPR_HTTP_CODE_LENGTH_REQUIRED           411
#define MPR_HTTP_CODE_REQUEST_TOO_LARGE         413
#define MPR_HTTP_CODE_REQUEST_URL_TOO_LARGE     414
#define MPR_HTTP_CODE_UNSUPPORTED_MEDIA_TYPE    415
#define MPR_HTTP_CODE_RANGE_NOT_SATISFIABLE     416
#define MPR_HTTP_CODE_EXPECTATION_FAILED        417
#define MPR_HTTP_CODE_INTERNAL_SERVER_ERROR     500
#define MPR_HTTP_CODE_NOT_IMPLEMENTED           501
#define MPR_HTTP_CODE_BAD_GATEWAY               502
#define MPR_HTTP_CODE_SERVICE_UNAVAILABLE       503
#define MPR_HTTP_CODE_GATEWAY_TIME_OUT          504
#define MPR_HTTP_CODE_BAD_VERSION               505
#define MPR_HTTP_CODE_INSUFFICIENT_STORAGE      507

/*
 *  Proprietary HTTP codes.
 */
#define MPR_HTTP_CODE_COMMS_ERROR               550
#define MPR_HTTP_CODE_CLIENT_ERROR              551

/*
 *  Overall HTTP service
 */
typedef struct MprHttpService {
    MprHashTable    *codes;                                 /* Http response code hash */
    char            *secret;                                /* Random bytes to use in authentication */
#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;                                 /* Mutli-thread sync */
#endif
} MprHttpService;

extern MprHttpService *mprCreateHttpService(MprCtx ctx);
extern int      mprStartHttpService(MprHttpService *hs);
extern int      mprStopHttpService(MprHttpService *hs);

/*
 *  Request states
 */
#define MPR_HTTP_STATE_BEGIN            1                   /* Ready for a new request */
#define MPR_HTTP_STATE_WAIT             2                   /* Waiting for the response */
#define MPR_HTTP_STATE_CONTENT          3                   /* Reading posted content */
#define MPR_HTTP_STATE_CHUNK            4                   /* Reading chunk length */
#define MPR_HTTP_STATE_PROCESSING       5                   /* Reading posted content */
#define MPR_HTTP_STATE_COMPLETE         6                   /* Processing complete */

/*
 *  HTTP protocol versions
 */
#define MPR_HTTP_1_0                    0                   /* HTTP/1.0 */
#define MPR_HTTP_1_1                    1                   /* HTTP/1.1 */

/**
 *  Get the Http reponse code as a string
 *  @description Get the Http response code as a string.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @param code Http status code
 *  @return A reference to the response code string. Callers must not free this reference.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpCodeString(MprCtx ctx, int code);

#endif /* BLD_FEATURE_HTTP */



#if BLD_FEATURE_HTTP_CLIENT
/*
 *  Callback flags
 */
#define MPR_HTTP_CALL_POST_DATA         1                   /* Time for caller to supply post data */
#define MPR_HTTP_CALL_RESPONSE_DATA     2                   /* Progressive reading of response data */
#define MPR_HTTP_CALL_COMPLETE          3                   /* Request is complete */

/*
 *  mprHttpRequst flags
 */
#define MPR_HTTP_DONT_BLOCK             0x1                 /**< Don't wait for a response */

/**
 *  Http callback procedure
 *  @param http Http object created via #mprCreateHttp
 *  @param nbytes Number of bytes read
 */
typedef void        (*MprHttpProc)(struct MprHttp *http, int nbytes);


/**
 *  HTTP Per-request structure
 */
typedef struct MprHttpRequest {
    struct MprHttp  *http;              /**< Reference to Http service object */
    char            *method;            /**< Request method GET, HEAD, POST, DELETE, OPTIONS, PUT, TRACE */
    MprUri          *uri;               /**< Request uri */
    MprHashTable    *headers;           /**< Headers keyword values */
    MprBuf          *outBuf;            /**< Request output buffer */
    char            *uploadFilename;    /**< Upload filename */
    char            *bodyData;          /**< Form post data */
    int             bodyLen;            /**< Length of bodyData */
    char            *formData;          /**< Form post data */
    int             formLen;            /**< Length of formData */
    int             sentCredentials;    /**< Credentials sent with request */
} MprHttpRequest;


/*
 *  Response flags
 */
#define MPR_HTTP_RESP_COMPLETE          0x4     /* Request complete */
#define MPR_HTTP_RESP_INPUT_CHUNKED     0x8     /* Using HTTP/1.1 chunked */
#define MPR_HTTP_RESP_BUFFER_SIZED      0x10    /* Input buffer has been resized */

/**
 *  HTTP Per-response structure
 */
typedef struct MprHttpResponse {
    struct MprHttp  *http;              /**< Reference to Http service object */

    int             length;             /**< Actual length of content received */
    int             contentLength;      /**< Content length header */
    int             contentRemaining;   /**< Remaining content data to read */
    int             chunkRemaining;     /**< Remaining content data to read in this chunk */

    MprHashTable    *headers;           /**< Headers keyword values */
    MprBuf          *content;           /**< Response content data */

    int             code;               /**< Http response status code */
    char            *message;           /**< Http response status message */
    char            *protocol;          /**< Response protocol "HTTP/1.0" or "HTTP/1.1" */
    char            *location;          /**< Redirect location */
    char            *authAlgorithm;     /**< Authentication algorithm */
    char            *authStale;         /**< Stale handshake value */
    int             flags;              /**< Control flags */
} MprHttpResponse;


/**
 *  Http per-connection structure. 
 *  @description The HTTP service provides a Http client with optional SSL capabilities. It supports 
 *      response chunking and ranged requests.
 *  @stability Prototype.
 *  @see mprCreateHttp mprCreateHttpSecret mprDisconnectHttp mprGetHttpFlags mprGetHttpState mprGetHttpDefaultHost
 *      mprGetHttpDefaultPort mprGetHttpCodeString mprGetHttpCode mprGetHttpMessage mprGetHttpContentLength
 *      mprGetHttpContent mprGetHttpError mprGetHttpHeader mprGetHttpHeaders mprGetHttpHeadersHash mprHttpRequest
 *      mprResetHttpCredentials mprSetHttpBody mprSetHttpCallback mprSetHttpCredentials mprSetHttpBufferSize
 *      mprSetHttpDefaultHost mprSetHttpDefaultPort mprSetHttpFollowRedirects mprSetHttpForm mprAddHttpFormItem
 *      mprSetHttpHeader mprSetHttpKeepAlive mprSetHttpProtocol mprSetHttpProxy mprSetHttpRetries mprSetHttpTimeout
 *      mprSetHttpUpload mprWriteHttpBody
 *  @defgroup MprHttp MprHttp
 */
typedef struct MprHttp {
    MprHttpService  *httpService;

    MprHttpRequest  *request;           /**< Request object */
    MprHttpResponse *response;          /**< Response object */
    MprSocket       *sock;              /**< Underlying socket handle */
    MprBuf          *headerBuf;         /**< Header buffer */

    int             state;              /**< Connection state  */
    int             userFlags;          /**< User flags to mprHttpRequest */

    char            *currentHost;       /**< Current connection host */
    int             currentPort;        /**< Current connection port */

    char            *protocol;          /**< HTTP protocol to use */
    char            *defaultHost;       /**< Default target host (if unspecified) */
    char            *proxyHost;         /**< Proxy host to connect via */
    int             defaultPort;        /**< Default target port (if unspecified) */
    int             proxyPort;          /**< Proxy port to connect via */

    MprTime         timestamp;          /**< Timeout timestamp for last I/O  */
    MprEvent        *timer;             /**< Timeout event handle  */
    int             timeoutPeriod;      /**< Timeout value */
    int             retries;            /**< Max number of retry attempts */

    MprHttpProc     callback;           /**< Response callback structure  */
    void            *callbackArg;       /**< Argument to callback  */

    /*
     *  Auth details
     */
    char            *authCnonce;        /**< Digest authentication cnonce value */
    char            *authDomain;        /**< Authentication domain */
    char            *authNonce;         /**< Nonce value used in digest authentication */
    int             authNc;             /**< Digest authentication nc value */
    char            *authOpaque;        /**< Opaque value used to calculate digest session */
    char            *authRealm;         /**< Authentication realm */
    char            *authQop;           /**< Digest authentication qop value */
    char            *authType;          /**< Basic or Digest */
    char            *password;          /**< As the name says */
    char            *user;              /**< User account name */

    char            *error;             /**< Error message if failure  */
    char            *userHeaders;       /**< User headers */

    bool            useKeepAlive;       /**< Use connection keep-alive for all connections */
    bool            keepAlive;          /**< Use keep-alive for this connection */
    bool            followRedirects;    /**< Follow redirects */

    int             bufsize;            /**< Initial buffer size */
    int             bufmax;             /**< Maximum buffer size. -1 is no max */
    int             protocolVersion;    /**< HTTP protocol version to request */

#if BLD_FEATURE_MULTITHREAD
    MprCond         *completeCond;      /**< Signalled when request is complete */
    MprMutex        *mutex;             /**< Mutli-thread sync */
#endif
} MprHttp;



/**
 *  Create a Http connection object
 *  @description Create a new http connection object. This creates an object that can be initialized and then
 *      used with mprHttpRequest
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @return A newly allocated MprHttp structure. Caller must free using #mprFree.
 *  @ingroup MprHttp
 */
extern MprHttp *mprCreateHttp(MprCtx ctx);

/**
 *  Create a Http secret
 *  @description Create http secret data that is used to seed SSL based communications.
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprCreateHttpSecret(MprCtx ctx);

/**
 *  Disconnect a Http connection
 *  @description Disconned any keep-alive connection associated with this http object.
 *  @param http Http object created via #mprCreateHttp
 *  @ingroup MprHttp
 */
extern void mprDisconnectHttp(MprHttp *http);

/**
 *  Return the http flags
 *  @description Get the current http flags. The valid flags are:
 *      @li MPR_HTTP_DONT_BLOCK  - For non-blocking connections
 *  @param http Http object created via #mprCreateHttp
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprGetHttpFlags(MprHttp *http);

/**
 *  Get the http state
 *  @description Get the current http state. The valid state are:
 *      @li  MPR_HTTP_STATE_BEGIN            - Ready for a new request
 *      @li  MPR_HTTP_STATE_WAIT             - Waiting for the response
 *      @li  MPR_HTTP_STATE_CONTENT          - Reading posted content
 *      @li  MPR_HTTP_STATE_CHUNK            - Reading chunk length
 *      @li  MPR_HTTP_STATE_PROCESSING       - Reading posted content
 *      @li  MPR_HTTP_STATE_COMPLETE         - Processing complete
 *  @param http Http object created via #mprCreateHttp
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprGetHttpState(MprHttp *http);

/**
 *  Get the default host
 *  @description A default host can be defined which will be used for URIs that omit a host specification.
 *  @param http Http object created via #mprCreateHttp
 *  @return A reference to the default host string. Callers must not free this reference.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpDefaultHost(MprHttp *http);

/**
 *  Get the default port
 *  @description A default port can be defined which will be used for URIs that omit a host:port specification.
 *  @param http Http object created via #mprCreateHttp
 *  @return The default port number
 *  @ingroup MprHttp
 */
extern int mprGetHttpDefaultPort(MprHttp *http);

/**
 *  Get the Http response code
 *  @description Get the http response code for the last request.
 *  @param http Http object created via #mprCreateHttp
 *  @return An integer Http response code. Typically 200 is success.
 *  @ingroup MprHttp
 */
extern int mprGetHttpCode(MprHttp *http);


/**
 *  Get the Http response message
 *  @description Get the Http response message supplied on the first line of the Http response.
 *  @param http Http object created via #mprCreateHttp
 *  @return A reference to the response message string. Callers must not free this reference.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpMessage(MprHttp *http);

/**
 *  Get the response content length
 *  @description Get the length of the response content (if any)
 *  @param http Http object created via #mprCreateHttp
 *  @return A count of the response content data in bytes.
 *  @ingroup MprHttp
 */
extern int mprGetHttpContentLength(MprHttp *http);

/**
 *  Get the response contenat
 *  @description Get the response content as a string
 *  @param http Http object created via #mprCreateHttp
 *  @return A reference to the response content. This is a reference into the Http content buffering. This call cannot
 *      be used if a callback has been configured on the Http response object. In that case, it is the callers
 *      responsibility to save the response content during each callback invocation.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpContent(MprHttp *http);

/**
 *  Get the Http error message
 *  @description Get a Http error message. Error messages may be generated for internal or client side errors.
 *  @param http Http object created via #mprCreateHttp
 *  @return A error string. The caller must not free this reference.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpError(MprHttp *http);

/**
 *  Get a http response header.
 *  @description Get a http response header for a given header key.
 *  @param http Http object created via #mprCreateHttp
 *  @param key Name of the header to retrieve. This should be a lower case header name. For example: "Connection"
 *  @return Value associated with the header key or null if the key did not exist in the response.
 *  @ingroup MprHttp
 */
extern cchar *mprGetHttpHeader(MprHttp *http, cchar *key);

/**
 *  Get all the http response headers.
 *  @description Get all the response headers. The returned string formats all the headers in the form:
 *      key: value\\nkey2: value2\\n...
 *  @param http Http object created via #mprCreateHttp
 *  @return String containing all the headers. The caller must free this returned string.
 *  @ingroup MprHttp
 */
extern char *mprGetHttpHeaders(MprHttp *http);

/**
 *  Get the hash table of response Http headers
 *  @description Get the internal hash table of response headers
 *  @param http Http object created via #mprCreateHttp
 *  @return Hash table. See MprHash for how to access the hash table.
 *  @ingroup MprHttp
 */
extern MprHashTable *mprGetHttpHeadersHash(MprHttp *http);

/**
 *  Issue a new Http request
 *  @description Issue a new Http request on the http object. 
 *  @param http Http object created via #mprCreateHttp
 *  @param method Http method to use. Valid methods include: "GET", "POST", "PUT", "DELETE", "OPTIONS" and "TRACE" 
 *  @param uri URI to fetch
 *  @param flags Request modification flags. Valid flags are: #MPR_HTTP_DONT_BLOCK for non-blocking I/O
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprHttpRequest(MprHttp *http, cchar *method, cchar *uri, int flags);

/**
 *  Reset Http credential.
 *  @description Reset any previously defined Http credentials on this http object.
 *  @param http Http object created via #mprCreateHttp
 *  @ingroup MprHttp
 */
extern void mprResetHttpCredentials(MprHttp *http);

/**
 *  Set the http request body content
 *  @description Define content to be sent with the Http request. 
 *  @param http Http object created via #mprCreateHttp
 *  @param body Pointer to the body content.
 *  @param len Length of the body content
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprSetHttpBody(MprHttp *http, cchar *body, int len);

/**
 *  Define a Http callback.
 *  @description If define, the http callabck will be invoked as response data or state is received.
 *  @param http Http object created via #mprCreateHttp
 *  @param fn Callback function. 
 *  @param arg Data argument to provide to the callback function.
 *  @ingroup MprHttp
 */
extern void mprSetHttpCallback(MprHttp *http, MprHttpProc fn, void *arg);

/**
 *  Set the Http credentials
 *  @description Define a username and password to use with Http authentication for sites that require it.
 *  @param http Http object created via #mprCreateHttp
 *  @param username String username
 *  @param password Un-encrypted password string
 *  @ingroup MprHttp
 */
extern void mprSetHttpCredentials(MprHttp *http, cchar *username, cchar *password);

/**
 *  Set the Http buffer size.
 *  @description Define an initial and maximum limit for the response content buffering. By default, the 
 *      buffer will grow to accomodate all response data.
 *  @param http Http object created via #mprCreateHttp
 *  @param initialSize Starting size of the response content buffer
 *  @param maxSize Maximum size of the response content buffer.
 *  @ingroup MprHttp
 */
extern void mprSetHttpBufferSize(MprHttp *http, int initialSize, int maxSize);

/**
 *  Define a default host
 *  @description Define a default host to use for connections if the URI does not specify a host
 *  @param http Http object created via #mprCreateHttp
 *  @param host Host or IP address
 *  @ingroup MprHttp
 */
extern void mprSetHttpDefaultHost(MprHttp *http, cchar *host);

/**
 *  Define a default port
 *  @description Define a default port to use for connections if the URI does not define a port
 *  @param http Http object created via #mprCreateHttp
 *  @param port Integer port number
 *  @ingroup MprHttp
 */
extern void mprSetHttpDefaultPort(MprHttp *http, int port);

/**
 *  Follow redirctions
 *  @description Enabling follow redirects enables the Http service to transparently follow 301 and 302 redirections
 *      and fetch the redirected URI.
 *  @param http Http object created via #mprCreateHttp
 *  @param follow Set to true to enable transparent redirections
 *  @ingroup MprHttp
 */
extern void mprSetHttpFollowRedirects(MprHttp *http, bool follow);

/**
 *  Set Http request form content
 *  @description Define request content that is formatted using www-form-urlencoded formatting. This is the 
 *      traditional way to post form data. For example: "name=John+Smith&City=Seattle". Multiple calls may be 
 *      made to this routine and the form data will be catenated.
 *  @param http Http object created via #mprCreateHttp
 *  @param form String containing the encoded form data.
 *  @param len Length of the form data
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprSetHttpForm(MprHttp *http, cchar *form, int len);

/**
 *  Add a form item
 *  @description Add a key/value pair to the request form data.
 *  @param http Http object created via #mprCreateHttp
 *  @param key Name of the form entity to define.
 *  @param value Value of the form entity.
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprAddHttpFormItem(MprHttp *http, cchar *key, cchar *value);

/**
 *  Add a request header
 *  @description Add a Http header to send with the request
 *  @param http Http object created via #mprCreateHttp
 *  @param key Http header key
 *  @param value Http header value
 *  @param overwrite Set to true to overwrite any previous header of the same key. Set to false to allow duplicate
 *      headers of the same key value.
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern int mprSetHttpHeader(MprHttp *http, cchar *key, cchar *value, bool overwrite);

/**
 *  Control Http Keep-Alive
 *  @description Http keep alive means that the TCP/IP connection is preserved accross multiple requests. This
 *      typically means much higher performance and better response. Http keep alive is enabled by default 
 *      for Http/1.1 (the default). Disable keep alive when talking
 *      to old, broken HTTP servers.
 *  @param http Http object created via #mprCreateHttp
 *  @param on Set to true to enable http keep-alive
 *  @return Zero if successful, otherwise a negative MPR error code
 *  @ingroup MprHttp
 */
extern void mprSetHttpKeepAlive(MprHttp *http, bool on);

/**
 *  Set the Http protocol variant.
 *  @description Set the Http protocol variant to use. 
 *  @param http Http object created via #mprCreateHttp
 *  @param protocol  String representing the protocol variant. Valid values are:
 *      @li "HTTP/1.0"
 *      @li "HTTP/1.1"
 *  Use HTTP/1.1 wherever possible.
 *  @ingroup MprHttp
 */
extern void mprSetHttpProtocol(MprHttp *http, cchar *protocol);

/**
 *  Define a Http proxy host
 *  @description Define a http proxy host to communicate via when accessing the net.
 *  @param http Http object created via #mprCreateHttp
 *  @param host Proxy host name or IP address
 *  @param port Proxy host port number.
 *  @ingroup MprHttp
 */
extern void mprSetHttpProxy(MprHttp *http, cchar *host, int port);

/**
 *  Set the Http retry count
 *  @description Define the number of retries before failing a request. It is normative for network errors
 *      to require that requests be sometimes retried. The default retries is set to (2).
 *  @param http Http object created via #mprCreateHttp
 *  @param retries Count of retries
 *  @ingroup MprHttp
 */
extern void mprSetHttpRetries(MprHttp *http, int retries);

/**
 *  Set the Http inactivity timeout
 *  @description Define an inactivity timeout after which the Http connection will be closed. 
 *  @param http Http object created via #mprCreateHttp
 *  @param timeout Timeout in milliseconds
 *  @ingroup MprHttp
 */
extern void mprSetHttpTimeout(MprHttp *http, int timeout);

//  TODO - not implemented
extern int mprSetHttpUpload(MprHttp *http, cchar *key, cchar *value);

/**
 *  Write Http request content data
 *  @description Write request content data. This routine is used when a callback has been defined for the 
 *      Http object.
 *  @param http Http object created via #mprCreateHttp
 *  @param buf Pointer to buffer containing the data to write
 *  @param len Length of data to write
 *  @param block Set to true to block while the write is completed. If set to false, the call may return with fewer
 *      bytes written. It is then the callers responsibility to retry the write.
 *  @return Number of bytes successfully written.
 *  @ingroup MprHttp
 */
extern int mprWriteHttpBody(MprHttp *http, cchar *buf, int len, bool block);

#endif /* BLD_FEATURE_HTTP_CLIENT */

/* ********************************* MprCmd ************************************/
#if BLD_FEATURE_CMD

/*
 *  Child status structure. Designed to be async-thread safe.
 */
typedef struct MprCmdChild {
    ulong           pid;                /*  Process ID */
    int             exitStatus;         /*  Exit status */
} MprCmdChild;


#define MPR_CMD_EOF_COUNT       2

/*
 *  Channels for clientFd and serverFd
 */
#define MPR_CMD_STDIN           0       /* Stdout for the client side */
#define MPR_CMD_STDOUT          1       /* Stdin for the client side */
#define MPR_CMD_STDERR          2       /* Stderr for the client side */
#define MPR_CMD_MAX_PIPE        3

/*
 *  Cmd procs must return the number of bytes read or -1 for errors.
 */
struct MprCmd;
typedef void    (*MprCmdProc)(struct MprCmd *cmd, int fd, int channel, void *data);

/*
 *  Flags
 */
#define MPR_CMD_NEW_SESSION     0x1     /* Create a new session on unix */
#define MPR_CMD_SHOW            0x2     /* Show the window of the created process on windows */
#define MPR_CMD_DETACHED        0x4     /* Detach the child process and don't wait */
#define MPR_CMD_IN              0x1000  /* Connect to stdin */
#define MPR_CMD_OUT             0x2000  /* Capture stdout */
#define MPR_CMD_ERR             0x4000  /* Capture stdout */

typedef struct MprCmdFile {
    char            *name;
    int             fd;
    int             clientFd;
#if BLD_WIN_LIKE
    HANDLE          handle;
#endif
} MprCmdFile;


typedef struct MprCmd {
    char            *program;           /* Program path name */
    char            **argv;             /* List of args. Null terminated */
    char            **env;              /* List of environment variables. Null terminated */
    char            *dir;               /* Current working dir for the process */
    int             argc;               /* Count of args in argv */
    int             status;             /* Command exit status */
    int             flags;              /* Control flags (userFlags not here) */
    int             eofCount;           /* Count of end-of-files */
    int             requiredEof;        /* Number of EOFs required for an exit */
    bool            completed;          /* Command is complete */
    MprCmdFile      files[MPR_CMD_MAX_PIPE]; /* Stdin, stdout for the command */
    MprWaitHandler  *handlers[MPR_CMD_MAX_PIPE];
    MprCmdProc      callback;           /* Handler for client output and completion */
    void            *callbackData;
    MprBuf          *stdoutBuf;         /* Standard output from the client */
    MprBuf          *stderrBuf;         /* Standard error output from the client */

    uint64          process;            /* Id/handle of the created process */

    void            *userData;          /* User data storage */
    int             userFlags;          /* User flags storage */

#if WIN
    HANDLE          thread;             /* Handle of the primary thread for the created process */
    char            *command;           /* Windows command line */          
#endif
#if VXWORKS && UNUSED
    MprSelectHandler *handler;
    int             waitFd;             /* Pipe to await child exit */
#endif

#if VXWORKS
    /*
     *  Don't use MprCond so we can build single-threaded and still use MprCmd
     */
    SEM_ID          startCond;          /* Synchronization semaphore for task start */
    SEM_ID          exitCond;           /* Synchronization semaphore for task exit */
#endif

#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;
#endif
} MprCmd;


extern void mprCloseCmdFd(MprCmd *cmd, int channel);
extern MprCmd *mprCreateCmd(MprCtx ctx);
extern void mprDisableCmdEvents(MprCmd *cmd, int channel);
extern void mprEnableCmdEvents(MprCmd *cmd, int channel);
extern int mprGetCmdExitStatus(MprCmd *cmd, int *statusp);
extern int mprGetCmdFd(MprCmd *cmd, int channel);
extern MprBuf *mprGetCmdBuf(MprCmd *cmd, int channel);
extern bool mprIsCmdRunning(MprCmd *cmd);
extern int mprMakeCmdIO(MprCmd *cmd);
extern void mprPollCmd(MprCmd *cmd);
extern int mprReadCmdPipe(MprCmd *cmd, int channel, char *buf, int bufsize);
extern int mprReapCmd(MprCmd *cmd, int timeout);
extern int mprRunCmd(MprCmd *cmd, cchar *command, char **out, char **err, int flags);
extern int mprRunCmdV(MprCmd *cmd, int argc, char **argv, char **out, char **err, int flags);
extern void mprSetCmdCallback(MprCmd *cmd, MprCmdProc callback, void *data);
extern void mprSetCmdDir(MprCmd *cmd, cchar *dir);
extern void mprSetCmdEnv(MprCmd *cmd, cchar **env);
extern int  mprStartCmd(MprCmd *cmd, int argc, char **argv, char **envp, int flags);
extern void mprStopCmd(MprCmd *cmd);
extern int mprWaitForCmd(MprCmd *cmd, int timeout);
extern int mprWriteCmdPipe(MprCmd *cmd, int channel, char *buf, int bufsize);

#endif /* BLD_FEATURE_CMD */

/* *********************************** Mpr ************************************/
/*
 *  Mpr flags
 */
#define MPR_EXITING                 0x1     /* App is exiting */
#define MPR_SERVICE_THREAD          0x4     /* Using a service thread for events */
#define MPR_STOPPED                 0x8     /* Mpr services stopped */
#define MPR_STARTED                 0x10    /* Mpr services started */
#define MPR_SSL_PROVIDER_LOADED     0x20    /* SSL provider loaded */

#define MPR_USER_START_FLAGS        (MPR_SERVICE_THREAD)

/**
 *  Primary MPR application control structure
 *  @description The Mpr structure stores critical application state information and is the root memory allocation
 *      context block. It is used as the MprCtx context for other memory allocations and is thus
 *      the ultimate parent of all allocated memory.
 *  @stability Evolving.
 *  @see mprGetApp, mprCreateEx, mprIsExiting, mprSignalExit, mprTerminate, mprGetKeyValue, mprRemoveKeyValue,
 *      mprSetDebugMode, mprGetErrorMsg, mprGetOsError, mprBreakpoint
 *  @defgroup Mpr Mpr
 */
typedef struct Mpr {
    MprHeap         heap;               /**< Top level memory pool */
    MprHeap         pageHeap;           /**< Heap for arenas and slabs. Always page oriented */

    bool            debugMode;          /**< Run in debug mode (no timers) */
    int             logLevel;           /**< Log trace level */
    MprLogHandler   logHandler;         /**< Current log handler callback */
    void            *logHandlerData;    /**< Handle data for log handler */
    MprHashTable    *table;             /**< TODO - what is this for ? */
    MprHashTable    *timeTokens;        /**< Date/Time parsing tokens */
    char            *name;              /**< Product name */
    char            *title;             /**< Product title */
    char            *version;           /**< Product version */

    int             argc;               /**< Count of command line args */
    char            **argv;             /**< Application command line args */
    char            *domainName;        /**< Domain portion */
    char            *hostName;          /**< Host name (fully qualified name) */
    char            *serverName;        /**< Server name portion (no domain) */

    int             flags;              /**< Processing state */
    int             timezone;           /**< Minutes west of Greenwich */

    bool            caseSensitive;      /**< File name comparisons are case sensitive */
    int             hasEventsThread;    /**< Running an events thread */
    int             pathDelimiter;      /**< Filename path delimiter */
    cchar           *newline;           /**< Newline delimiter for text files */

#if WIN
    char            *cygdrive;          /**< Cygwin drive root */
#endif

    /**<
     *  Service pointers
     */
    struct MprFileService   *fileService;   /**< File system service object */
    struct MprOsService     *osService;     /**< O/S service object */

    struct MprEventService  *eventService;  /**< Event service object */
    struct MprPoolService   *poolService;   /**< Pool service object */
    struct MprWaitService   *waitService;   /**< IO Waiting service object */
    struct MprSocketService *socketService; /**< Socket service object */
#if BLD_FEATURE_HTTP
    struct MprHttpService   *httpService;   /**< HTTP service object */
#endif
#if BLD_FEATURE_CMD
    struct MprCmdService    *cmdService;    /**< Command service object */
#endif

    struct MprModuleService *moduleService; /**< Module service object */

#if BLD_FEATURE_MULTITHREAD
    struct MprThreadService *threadService; /**< Thread service object */

    MprMutex        *mutex;             /**< Thread synchronization */
    MprSpin         *spin;              /**< Quick thread synchronization */
#endif

#if BLD_WIN_LIKE
    long            appInstance;        /**< Application instance (windows) */
#endif

#if BREW
    uint            classId;            /**< Brew class ID */
    IShell          *shell;             /**< Brew shell object */
    IDisplay        *display;           /**< Brew display object */
    ITAPI           *tapi;              /**< TAPI object */
    int             displayHeight;      /**< Display height */
    int             displayWidth;       /**< Display width */
    char            *args;              /**< Command line args */
#endif

} Mpr;


#if !BLD_WIN_LIKE || DOXYGEN
/* TODO OPT - fix to allow _globalMpr on windows */
extern Mpr  *_globalMpr;                /* Mpr singleton */
#define mprGetMpr(ctx) _globalMpr
#else

/**
 *  Return the MPR control instance.
 *  @description Return the MPR singleton control object. 
 *  @param ctx Any memory allocation context created by MprAlloc
 *  @return Returns the MPR control object.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern struct Mpr *mprGetMpr(MprCtx ctx);
#endif


/**
 *  Create an instance of the MPR.
 *  @description Initializes the MPR and creates an Mpr control object. The Mpr Object manages Mpr facilities 
 *      and is the top level memory context. It may be used wherever a MprCtx parameter is required. This 
 *      function must be called prior to calling any other Mpr API.
 *  @param argc Count of command line args
 *  @param argv Command line arguments for the application. Arguments may be passed into the Mpr for retrieval
 *      by the unit test framework.
 *  @param cback Memory allocation failure notification callback.
 *  @return Returns a pointer to the Mpr object. 
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern Mpr *mprCreate(int argc, char **argv, MprAllocNotifier cback);


/**
 *  Create an instance of the MPR.
 *  @description Alternate API to create and initialize the MPR. The Mpr object manages Mpr facilities and 
 *      is the top level memory context. It may be used wherever a MprCtx parameter is required. This 
 *      function, or #mprCreate must be called prior to calling any other Mpr API.
 *  @param argc Count of arguments supplied in argv
 *  @param argv Program arguments. The MPR can store the program arguments for retrieval by other parts of the program.
 *  @param cback Callback function to be invoked on memory allocation errors. Set to null if not required.
 *  @param shell Optional reference to an O/S implementation dependent shell object. Used by Brew.
 *  @return Returns a pointer to the Mpr object. 
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern Mpr *mprCreateEx(int argc, char **argv, MprAllocNotifier cback, void *shell);

extern int mprStart(Mpr *mpr, int startFlags);
extern void mprStop(Mpr *mpr);

/**
 *  Signal the MPR to exit gracefully.
 *  @description Set the must exit flag for the MPR.
 *  @param ctx Any memory context allocated by the MPR.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern void mprSignalExit(MprCtx ctx);

/**
 *  Determine if the MPR should exit
 *  @description Returns true if the MPR should exit gracefully.
 *  @param ctx Any memory context allocated by the MPR.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern bool mprIsExiting(MprCtx ctx);

//  TODO - do we need these?
/**
 *  Set the value of a key.
 *  @description Sets the value of a key. Keys are stored by the MPR and may be
 *      retrieved using mprGetKeyValue. This routine is a solution for systems
 *      that do not allow global or static variables.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param key String key name.
 *  @param ptr Value to associate with the key.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern int mprSetKeyValue(MprCtx ctx, cchar *key, void *ptr);


/* TODO -- should this be delete or remove or unset */
/**
 *  Remove a key
 *  @description Removes a key and any associated value.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param key String key name.
 *  @return Returns zero if successful, otherwise a negative MPR error code.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern int mprRemoveKeyValue(MprCtx ctx, cchar *key);

/**
 *  Get the value of a key.
 *  @description Gets the value of a key. Keys are stored by the MPR and may be set using mprSetKeyValue. 
 *      This routine is a solution for systems that do not allow global or static variables.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param key String key name.
 *  @return Returns the value associated with the key via mprSetKeyValue.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern cvoid *mprGetKeyValue(MprCtx ctx, cchar *key);

extern int      mprSetAppName(MprCtx ctx, cchar *name, cchar *title, cchar *version);
extern cchar    *mprGetAppName(MprCtx ctx);
extern char     *mprGetAppPath(MprCtx ctx, char *path, int pathsize);
extern char     *mprGetAppDir(MprCtx ctx, char *path, int pathsize);
extern cchar    *mprGetAppTitle(MprCtx ctx);
extern cchar    *mprGetAppVersion(MprCtx ctx);
extern void     mprSetHostName(MprCtx ctx, cchar *s);
extern cchar    *mprGetHostName(MprCtx ctx);
extern void     mprSetServerName(MprCtx ctx, cchar *s);
extern cchar    *mprGetServerName(MprCtx ctx);
extern void     mprSetDomainName(MprCtx ctx, cchar *s);
extern cchar    *mprGetDomainName(MprCtx ctx);

/**
 *  Get the debug mode.
 *  @description Returns whether the debug mode is enabled. Some modules
 *      observe debug mode and disable timeouts and timers so that single-step
 *      debugging can be used.
 *  @param ctx Any memory context allocated by the MPR.
 *  @return Returns true if debug mode is enabled, otherwise returns false.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern bool     mprGetDebugMode(MprCtx ctx);

extern int      mprGetLogLevel(MprCtx ctx);

/**
 *  Return the O/S error code.
 *  @description Returns an O/S error code from the most recent system call. 
 *      This returns errno on Unix systems or GetLastError() on Windows..
 *  @return The O/S error code.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern int      mprGetOsError();

extern int      mprMakeArgv(MprCtx ctx, cchar *prog, cchar *cmd, int *argc, char ***argv);

/** 
 *  Turn on debug mode.
 *  @description Debug mode disables timeouts and timers. This makes debugging
 *      much easier.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param on Set to true to enable debugging mode.
 *  @stability Evolving.
 */
extern void     mprSetDebugMode(MprCtx ctx, bool on);

/**
 *  Set the current logging level.
 *  @description This call defines the maximum level of messages that will be
 *      logged. Calls to mprLog specify a message level. If the message level
 *      is greater than the defined logging level, the message is ignored.
 *  @param ctx Any memory context allocated by the MPR.
 *  @param level New logging level. Must be 0-9 inclusive.
 *  @return Returns the previous logging level.
 *  @stability Evolving.
 *  @ingroup MprLog
 */
extern void     mprSetLogLevel(MprCtx ctx, int level);

extern void     mprSleep(MprCtx ctx, int msec);
extern void     mprSetShell(MprCtx ctx, void *shell);
extern void     *mprGetShell(MprCtx ctx);
extern void     mprSetClassId(MprCtx ctx, uint classId);
extern uint     mprGetClassId(MprCtx ctx);

#if BREW
extern void     mprSetDisplay(MprCtx ctx, void *display);
extern void     *mprGetDisplay(MprCtx ctx);
#endif

#if BLD_WIN_LIKE
extern int      mprReadRegistry(MprCtx ctx, char **buf, int max, cchar *key, cchar *val);
extern int      mprWriteRegistry(MprCtx ctx, cchar *key, cchar *name, cchar *value);
#endif

extern int      mprStartEventsThread(Mpr *mpr);

/**
 *  Terminate the MPR.
 *  @description Terminates the MPR and disposes of all allocated resources. The mprTerminate
 *      function will recursively free all memory allocated by the MPR.
 *  @param ctx Any memory context object returned by #mprAlloc.
 *  @param graceful Shutdown gracefully waiting for all events to drain. Otherise exit immediately
 *      without waiting for any threads or events to complete.
 *  @stability Evolving.
 *  @ingroup Mpr
 */
extern void     mprTerminate(MprCtx ctx, bool graceful);

extern bool     mprIsRunningEventsThread(MprCtx ctx);
extern bool     mprIsService(Mpr *mpr);

extern void     mprSetPriority(Mpr *mpr, int pri);
extern void     mprWriteToOsLog(MprCtx ctx, cchar *msg, int flags, int level);

#if BLD_WIN_LIKE
extern HWND     mprGetHwnd(MprCtx ctx);
extern void     mprSetHwnd(MprCtx ctx, HWND h);
extern long     mprGetInst(MprCtx ctx);
extern void     mprSetInst(MprCtx ctx, long inst);
extern void     mprSetSocketMessage(MprCtx ctx, int message);
#endif

extern int      mprGetRandomBytes(MprCtx ctx, uchar *buf, int size, int block);

extern int      mprGetEndian(MprCtx ctx);

/* ******************************** Unicode ***********************************/

#define BLD_FEATURE_UTF16  1
#if BLD_FEATURE_UTF16
    typedef short MprUsData;
#else
    typedef char MprUsData;
#endif

typedef struct MprUs {
    MprUsData   *str;
    int         length;
} MprUs;


extern MprUs    *mprAllocUs(MprCtx ctx);

extern int      mprCopyUs(MprUs *dest, MprUs *src);
extern int      mprCatUs(MprUs *dest, MprUs *src);
extern int      mprCatUsArgs(MprUs *dest, MprUs *src, ...);
extern MprUs    *mprDupUs(MprUs *us);

extern int      mprCopyStrToUs(MprUs *dest, cchar *str);

extern int      mprGetUsLength(MprUs *us);

extern int      mprContainsChar(MprUs *us, int charPat);
extern int      mprContainsUs(MprUs *us, MprUs *pat);
extern int      mprContainsCaselessUs(MprUs *us, MprUs *pat);
extern int      mprContainsStr(MprUs *us, cchar *pat);

#if FUTURE
extern int      mprContainsPattern(MprUs *us, MprRegex *pat);
#endif

extern MprUs    *mprTrimUs(MprUs *dest, MprUs *pat);
extern int      mprTruncateUs(MprUs *dest, int len);
extern MprUs    *mprSubUs(MprUs *dest, int start, int len);

extern MprUs    *mprMemToUs(MprCtx ctx, const uchar *buf, int len);
extern MprUs    *mprStrToUs(MprCtx ctx, cchar *str);
extern char     *mprUsToStr(MprUs *us);

extern void     mprUsToLower(MprUs *us);
extern void     mprUsToUpper(MprUs *us);

extern MprUs    *mprTokenizeUs(MprUs *us, MprUs *delim, int *last);

extern int      mprFormatUs(MprUs *us, int maxSize, cchar *fmt, ...);
extern int      mprScanUs(MprUs *us, cchar *fmt, ...);

/*
 *  What about:
 *      isdigit, isalpha, isalnum, isupper, islower, isspace
 *      replace
 *      reverse
 *      split
 *          extern MprList *mprSplit(MprUs *us, MprUs *delim)
 */
/* MprFree */


/* ****************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* _h_MPR */

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../include/mpr.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/mprTest.h"
 */
/************************************************************************/

/*
 *  mprTest.h - Header for the Embedthis Unit Test Framework
 *  
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */


#ifndef _h_MPR_TEST
#define _h_MPR_TEST 1



#if BLD_FEATURE_TEST
/*
 *  Tunable constants
 */
#define MPR_TEST_POLL_NAP   25
#define MPR_TEST_SLEEP      (60 * 1000)
#define MPR_TEST_MAX_STACK  (16)

/*
 *  Unit test definition structures
 */
struct MprTestGroup;
typedef void        (*MprTestProc)(struct MprTestGroup *tp);

typedef struct MprTestCase {
    char            *name;
    int             level;
    MprTestProc     proc;
    int             (*init)(struct MprTestGroup *gp);
    int             (*term)(struct MprTestGroup *gp);
} MprTestCase;

typedef struct MprTestDef {
    char                *name;
    struct MprTestDef   **groupDefs;
    int                 (*init)(struct MprTestGroup *gp);
    int                 (*term)(struct MprTestGroup *gp);
    MprTestCase         caseDefs[32];
} MprTestDef;


/*
 *  Assert macros for use by unit tests
 */
#undef  assert
#define assert(C)   assertTrue(gp, MPR_LOC, C, #C)

#define MPR_TEST(level, functionName) { #functionName, level, functionName, 0, 0 }

typedef struct MprTestService {
    int             argc;                   /* Count of arguments */
    char            **argv;                 /* Arguments for test */
    int             activeThreadCount;      /* Currently active test threads */
    char            commandLine[MPR_MAX_STRING];
    bool            continueOnFailures;     /* Keep testing on failures */
    bool            debugOnFailures;        /* Break to the debugger */
    int             echoCmdLine;            /* Echo the command line */
    int             firstArg;               /* Count of arguments */
    MprList         *groups;                /* Master list of test groups */
    int             iterations;             /* Times to run the test */
    bool            singleStep;             /* Pause between tests */
    cchar           *name;                  /* Name for entire test */
    int             numThreads;             /* Number of test threads */
    int             poolThreads;            /* Count of pool threads */
    cchar           *testLog;               /* Log file for test results */
    MprTime         start;                  /* When testing began */
    int             testDepth;              /* Depth of entire test */
    MprList         *perThreadGroups;       /* Per thread copy of groups */
    int             totalFailedCount;       /* Total count of failing tests */
    int             totalTestCount;         /* Total count of all tests */
    MprList         *testFilter;            /* Test groups to run */
    int             verbose;                /* Output activity trace */

#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;                 /* Multi-thread sync */
#endif
} MprTestService;

extern MprTestService *mprCreateTestService(MprCtx ctx);
extern int          mprParseTestArgs(MprTestService *ts, int argc, char **argv);
extern int          mprRunTests(MprTestService *sp);
extern void         mprReportTestResults(MprTestService *sp);


/*
 *  A test group is a group of tests to cover a unit of functionality. A test group may contain other test groups.
 */
typedef struct MprTestGroup {
    char            *name;                  /* Name of test */
    char            *fullName;              /* Fully qualified name of test */
    int             testDepth;              /* Depth at which test should run */
    bool            skip;                   /* Skip this test */

    bool            success;                /* Result of last run */
    int             failedCount;            /* Total failures of this test */
    int             testCount;              /* Count of tests */
    MprList         *failures;              /* List of all failures */

    MprTestService  *service;               /* Reference to the service */

    volatile bool   condWakeFlag;           /* Used when single-threaded */
    volatile bool   cond2WakeFlag;          /* Used when single-threaded */

    struct MprTestGroup *parent;            /* Parent test group */
    MprList         *groups;                /* List of groups */
    MprList         *cases;                 /* List of tests in this group */
    MprTestDef      *def;                   /* Test definition ref */

#if BLD_FEATURE_MULTITHREAD
    MprCond         *cond;                  /* Multi-thread sync */
    MprCond         *cond2;                 /* Second multi-thread sync */
    MprMutex        *mutex;                 /* Multi-thread sync */
#endif

    void            *data;                  /* Test specific data */
    MprCtx          ctx;                    /* Memory context for unit tests to use */
    
} MprTestGroup;


extern MprTestGroup *mprAddTestGroup(MprTestService *ts, MprTestDef *def);
extern void         mprResetTestGroup(MprTestGroup *gp);
extern bool         assertTrue(MprTestGroup *gp, cchar *loc, bool success, cchar *msg);
extern void         mprSignalTestComplete(MprTestGroup *gp);
extern void         mprSignalTest2Complete(MprTestGroup *gp);
extern bool         mprWaitForTestToComplete(MprTestGroup *gp, int timeout);
extern bool         mprWaitForTest2ToComplete(MprTestGroup *gp, int timeout);


typedef struct MprTestFailure {
    char            *loc;
    char            *message;
} MprTestFailure;


#endif /* BLD_FEATURE_TEST */
#endif /* _h_MPR_TEST */


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../include/mprTest.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/mprUnix.h"
 */
/************************************************************************/

/*
 *  mprUnix.h - Make windows a bit more unix like
 *  
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#ifndef _h_MPR_UNIX
#define _h_MPR_UNIX 1

#if BLD_WIN_LIKE
#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Define BLD_NO_POSIX_REMAP if these defines mess with your app
 */
#if BLD_WIN_LIKE && !BLD_NO_POSIX_REMAP
#define access      _access
#define close       _close
#define fileno      _fileno
#define fstat       _fstat
#define getpid      _getpid
#define open        _open
#define putenv      _putenv
#define read        _read
#define stat        _stat
#define umask       _umask
#define unlink      _unlink
#define write       _write
#define strdup      _strdup
#define lseek       _lseek
#define getcwd      _getcwd
#define chdir       _chdir

#define mkdir(a,b)  _mkdir(a)
#define rmdir(a)    _rmdir(a)

#define R_OK        4
#define W_OK        2
#define MPR_TEXT    "t"

extern void         srand48(long);
extern long         lrand48(void);
extern long         ulimit(int, ...);
extern long         nap(long);
extern int          getuid(void);
extern int          geteuid(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* BLD_WIN_LIKE */
#endif /* _h_MPR_UNIX */

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../include/mprUnix.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/pcre.h"
 */
/************************************************************************/

/*************************************************
*       Perl-Compatible Regular Expressions      *
*************************************************/

/* This is the public header file for the PCRE library, to be #included by
applications that call the PCRE functions.

           Copyright (c) 1997-2008 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

#ifndef _PCRE_H
#define _PCRE_H

/* The current PCRE version information. */

#define PCRE_MAJOR          7
#define PCRE_MINOR          7
#define PCRE_PRERELEASE     
#define PCRE_DATE           2008-05-07

/* When an application links to a PCRE DLL in Windows, the symbols that are
imported have to be identified as such. When building PCRE, the appropriate
export setting is defined in pcre_internal.h, which includes this file. So we
don't change existing definitions of PCRE_EXP_DECL and PCRECPP_EXP_DECL. */

#if BLD_FEATURE_STATIC
#define PCRE_STATIC
#endif

#if BLD_ALL_IN_ONE
    /*
     *  When building all-in-one, we must use internal definitions
     */
    #ifndef PCRE_EXP_DECL
    #  ifdef _WIN32
    #    ifndef PCRE_STATIC
    #      define PCRE_EXP_DECL       extern __declspec(dllexport)
    #      define PCRE_EXP_DEFN       __declspec(dllexport)
    #      define PCRE_EXP_DATA_DEFN  __declspec(dllexport)
    #    else
    #      define PCRE_EXP_DECL       extern
    #      define PCRE_EXP_DEFN
    #      define PCRE_EXP_DATA_DEFN
    #    endif
    #  else
    #    ifdef __cplusplus
    #      define PCRE_EXP_DECL       extern "C"
    #    else
    #      define PCRE_EXP_DECL       extern
    #    endif
    #    ifndef PCRE_EXP_DEFN
    #      define PCRE_EXP_DEFN       PCRE_EXP_DECL
    #    endif
    #    ifndef PCRE_EXP_DATA_DEFN
    #      define PCRE_EXP_DATA_DEFN
    #    endif
    #  endif
    #endif
    
    /* EMBEDTHIS */
    #    ifndef PCRE_EXP_DEFN
    #      define PCRE_EXP_DEFN       PCRE_EXP_DECL
    #    endif
    #    ifndef PCRE_EXP_DATA_DEFN
    #      define PCRE_EXP_DATA_DEFN
    #    endif
#else
    #if defined(_WIN32) && !defined(PCRE_STATIC)
    #  ifndef PCRE_EXP_DECL
    #    define PCRE_EXP_DECL  extern __declspec(dllimport)
    #  endif
    #  ifdef __cplusplus
    #    ifndef PCRECPP_EXP_DECL
    #      define PCRECPP_EXP_DECL  extern __declspec(dllimport)
    #    endif
    #    ifndef PCRECPP_EXP_DEFN
    #      define PCRECPP_EXP_DEFN  __declspec(dllimport)
    #    endif
    #  endif
    #endif
    
    /* By default, we use the standard "extern" declarations. */
    
    #ifndef PCRE_EXP_DECL
    #  ifdef __cplusplus
    #    define PCRE_EXP_DECL  extern "C"
    #  else
    #    define PCRE_EXP_DECL  extern
    #  endif
    #endif
    
    #ifdef __cplusplus
    #  ifndef PCRECPP_EXP_DECL
    #    define PCRECPP_EXP_DECL  extern
    #  endif
    #  ifndef PCRECPP_EXP_DEFN
    #    define PCRECPP_EXP_DEFN
    #  endif
    #endif
#endif

/* Have to include stdlib.h in order to ensure that size_t is defined;
it is needed here for malloc. */

#include <stdlib.h>

/* Allow for C++ users */

#ifdef __cplusplus
extern "C" {
#endif

/* Options */

#define PCRE_CASELESS           0x00000001
#define PCRE_MULTILINE          0x00000002
#define PCRE_DOTALL             0x00000004
#define PCRE_EXTENDED           0x00000008
#define PCRE_ANCHORED           0x00000010
#define PCRE_DOLLAR_ENDONLY     0x00000020
#define PCRE_EXTRA              0x00000040
#define PCRE_NOTBOL             0x00000080
#define PCRE_NOTEOL             0x00000100
#define PCRE_UNGREEDY           0x00000200
#define PCRE_NOTEMPTY           0x00000400
#define PCRE_UTF8               0x00000800
#define PCRE_NO_AUTO_CAPTURE    0x00001000
#define PCRE_NO_UTF8_CHECK      0x00002000
#define PCRE_AUTO_CALLOUT       0x00004000
#define PCRE_PARTIAL            0x00008000
#define PCRE_DFA_SHORTEST       0x00010000
#define PCRE_DFA_RESTART        0x00020000
#define PCRE_FIRSTLINE          0x00040000
#define PCRE_DUPNAMES           0x00080000
#define PCRE_NEWLINE_CR         0x00100000
#define PCRE_NEWLINE_LF         0x00200000
#define PCRE_NEWLINE_CRLF       0x00300000
#define PCRE_NEWLINE_ANY        0x00400000
#define PCRE_NEWLINE_ANYCRLF    0x00500000
#define PCRE_BSR_ANYCRLF        0x00800000
#define PCRE_BSR_UNICODE        0x01000000
#define PCRE_JAVASCRIPT_COMPAT  0x02000000

/* Exec-time and get/set-time error codes */

#define PCRE_ERROR_NOMATCH         (-1)
#define PCRE_ERROR_NULL            (-2)
#define PCRE_ERROR_BADOPTION       (-3)
#define PCRE_ERROR_BADMAGIC        (-4)
#define PCRE_ERROR_UNKNOWN_OPCODE  (-5)
#define PCRE_ERROR_UNKNOWN_NODE    (-5)  /* For backward compatibility */
#define PCRE_ERROR_NOMEMORY        (-6)
#define PCRE_ERROR_NOSUBSTRING     (-7)
#define PCRE_ERROR_MATCHLIMIT      (-8)
#define PCRE_ERROR_CALLOUT         (-9)  /* Never used by PCRE itself */
#define PCRE_ERROR_BADUTF8        (-10)
#define PCRE_ERROR_BADUTF8_OFFSET (-11)
#define PCRE_ERROR_PARTIAL        (-12)
#define PCRE_ERROR_BADPARTIAL     (-13)
#define PCRE_ERROR_INTERNAL       (-14)
#define PCRE_ERROR_BADCOUNT       (-15)
#define PCRE_ERROR_DFA_UITEM      (-16)
#define PCRE_ERROR_DFA_UCOND      (-17)
#define PCRE_ERROR_DFA_UMLIMIT    (-18)
#define PCRE_ERROR_DFA_WSSIZE     (-19)
#define PCRE_ERROR_DFA_RECURSE    (-20)
#define PCRE_ERROR_RECURSIONLIMIT (-21)
#define PCRE_ERROR_NULLWSLIMIT    (-22)  /* No longer actually used */
#define PCRE_ERROR_BADNEWLINE     (-23)

/* Request types for pcre_fullinfo() */

#define PCRE_INFO_OPTIONS            0
#define PCRE_INFO_SIZE               1
#define PCRE_INFO_CAPTURECOUNT       2
#define PCRE_INFO_BACKREFMAX         3
#define PCRE_INFO_FIRSTBYTE          4
#define PCRE_INFO_FIRSTCHAR          4  /* For backwards compatibility */
#define PCRE_INFO_FIRSTTABLE         5
#define PCRE_INFO_LASTLITERAL        6
#define PCRE_INFO_NAMEENTRYSIZE      7
#define PCRE_INFO_NAMECOUNT          8
#define PCRE_INFO_NAMETABLE          9
#define PCRE_INFO_STUDYSIZE         10
#define PCRE_INFO_DEFAULT_TABLES    11
#define PCRE_INFO_OKPARTIAL         12
#define PCRE_INFO_JCHANGED          13
#define PCRE_INFO_HASCRORLF         14

/* Request types for pcre_config(). Do not re-arrange, in order to remain
compatible. */

#define PCRE_CONFIG_UTF8                    0
#define PCRE_CONFIG_NEWLINE                 1
#define PCRE_CONFIG_LINK_SIZE               2
#define PCRE_CONFIG_POSIX_MALLOC_THRESHOLD  3
#define PCRE_CONFIG_MATCH_LIMIT             4
#define PCRE_CONFIG_STACKRECURSE            5
#define PCRE_CONFIG_UNICODE_PROPERTIES      6
#define PCRE_CONFIG_MATCH_LIMIT_RECURSION   7
#define PCRE_CONFIG_BSR                     8

/* Bit flags for the pcre_extra structure. Do not re-arrange or redefine
these bits, just add new ones on the end, in order to remain compatible. */

#define PCRE_EXTRA_STUDY_DATA             0x0001
#define PCRE_EXTRA_MATCH_LIMIT            0x0002
#define PCRE_EXTRA_CALLOUT_DATA           0x0004
#define PCRE_EXTRA_TABLES                 0x0008
#define PCRE_EXTRA_MATCH_LIMIT_RECURSION  0x0010

/* Types */

struct real_pcre;                 /* declaration; the definition is private  */
typedef struct real_pcre pcre;

/* When PCRE is compiled as a C++ library, the subject pointer type can be
replaced with a custom type. For conventional use, the public interface is a
const char *. */

#ifndef PCRE_SPTR
#define PCRE_SPTR const char *
#endif

/* The structure for passing additional data to pcre_exec(). This is defined in
such as way as to be extensible. Always add new fields at the end, in order to
remain compatible. */

typedef struct pcre_extra {
  unsigned long int flags;        /* Bits for which fields are set */
  void *study_data;               /* Opaque data from pcre_study() */
  unsigned long int match_limit;  /* Maximum number of calls to match() */
  void *callout_data;             /* Data passed back in callouts */
  const unsigned char *tables;    /* Pointer to character tables */
  unsigned long int match_limit_recursion; /* Max recursive calls to match() */
} pcre_extra;

/* The structure for passing out data via the pcre_callout_function. We use a
structure so that new fields can be added on the end in future versions,
without changing the API of the function, thereby allowing old clients to work
without modification. */

typedef struct pcre_callout_block {
  int          version;           /* Identifies version of block */
  /* ------------------------ Version 0 ------------------------------- */
  int          callout_number;    /* Number compiled into pattern */
  int         *offset_vector;     /* The offset vector */
  PCRE_SPTR    subject;           /* The subject being matched */
  int          subject_length;    /* The length of the subject */
  int          start_match;       /* Offset to start of this match attempt */
  int          current_position;  /* Where we currently are in the subject */
  int          capture_top;       /* Max current capture */
  int          capture_last;      /* Most recently closed capture */
  void        *callout_data;      /* Data passed in with the call */
  /* ------------------- Added for Version 1 -------------------------- */
  int          pattern_position;  /* Offset to next item in the pattern */
  int          next_item_length;  /* Length of next item in the pattern */
  /* ------------------------------------------------------------------ */
} pcre_callout_block;

/* Indirection for store get and free functions. These can be set to
alternative malloc/free functions if required. Special ones are used in the
non-recursive case for "frames". There is also an optional callout function
that is triggered by the (?) regex item. For Virtual Pascal, these definitions
have to take another form. */

#ifndef VPCOMPAT
PCRE_EXP_DECL void *(*pcre_malloc)(size_t);
PCRE_EXP_DECL void  (*pcre_free)(void *);
PCRE_EXP_DECL void *(*pcre_stack_malloc)(size_t);
PCRE_EXP_DECL void  (*pcre_stack_free)(void *);
PCRE_EXP_DECL int   (*pcre_callout)(pcre_callout_block *);
#else   /* VPCOMPAT */
PCRE_EXP_DECL void *pcre_malloc(size_t);
PCRE_EXP_DECL void  pcre_free(void *);
PCRE_EXP_DECL void *pcre_stack_malloc(size_t);
PCRE_EXP_DECL void  pcre_stack_free(void *);
PCRE_EXP_DECL int   pcre_callout(pcre_callout_block *);
#endif  /* VPCOMPAT */

/* Exported PCRE functions */

PCRE_EXP_DECL pcre *pcre_compile(const char *, int, const char **, int *,
                  const unsigned char *);
PCRE_EXP_DECL pcre *pcre_compile2(const char *, int, int *, const char **,
                  int *, const unsigned char *);
PCRE_EXP_DECL int  pcre_config(int, void *);
PCRE_EXP_DECL int  pcre_copy_named_substring(const pcre *, const char *,
                  int *, int, const char *, char *, int);
PCRE_EXP_DECL int  pcre_copy_substring(const char *, int *, int, int, char *,
                  int);
PCRE_EXP_DECL int  pcre_dfa_exec(const pcre *, const pcre_extra *,
                  const char *, int, int, int, int *, int , int *, int);
PCRE_EXP_DECL int  pcre_exec(const pcre *, const pcre_extra *, PCRE_SPTR,
                   int, int, int, int *, int);
PCRE_EXP_DECL void pcre_free_substring(const char *);
PCRE_EXP_DECL void pcre_free_substring_list(const char **);
PCRE_EXP_DECL int  pcre_fullinfo(const pcre *, const pcre_extra *, int,
                  void *);
PCRE_EXP_DECL int  pcre_get_named_substring(const pcre *, const char *,
                  int *, int, const char *, const char **);
PCRE_EXP_DECL int  pcre_get_stringnumber(const pcre *, const char *);
PCRE_EXP_DECL int  pcre_get_stringtable_entries(const pcre *, const char *,
                  char **, char **);
PCRE_EXP_DECL int  pcre_get_substring(const char *, int *, int, int,
                  const char **);
PCRE_EXP_DECL int  pcre_get_substring_list(const char *, int *, int,
                  const char ***);
PCRE_EXP_DECL int  pcre_info(const pcre *, int *, int *);
PCRE_EXP_DECL const unsigned char *pcre_maketables(void);
PCRE_EXP_DECL int  pcre_refcount(pcre *, int);
PCRE_EXP_DECL pcre_extra *pcre_study(const pcre *, int, const char **);
PCRE_EXP_DECL const char *pcre_version(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* End of pcre.h */
/************************************************************************/
/*
 *  End of file "../include/pcre.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../include/mprSsl.h"
 */
/************************************************************************/

/*
 *  mprSsl.h - Header for SSL services. Currently supporting OpenSSL and MatrixSSL.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#ifndef _h_MPR_SSL
#define _h_MPR_SSL 1


#if BLD_FEATURE_OPENSSL
    /* Clashes with WinCrypt.h */
    #undef OCSP_RESPONSE
    #include    <openssl/ssl.h>
    #include    <openssl/evp.h>
    #include    <openssl/rand.h>
    #include    <openssl/err.h>
#endif

#if BLD_FEATURE_MATRIXSSL
    #include    "matrixSsl.h"
#endif 

#ifdef __cplusplus
extern "C" {
#endif

/* *********************************** Defines ********************************/
/*
 *  SSL protocols
 */
#define MPR_HTTP_PROTO_SSLV2    0x1
#define MPR_HTTP_PROTO_SSLV3    0x2
#define MPR_HTTP_PROTO_TLSV1    0x4
#define MPR_HTTP_PROTO_ALL      0x7

/*
 *  Default SSL configuration
 */
#define MPR_DEFAULT_CIPHER_SUITE        "ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:+LOW:+SSLv2:+EXP:+eNULL"
#define MPR_DEFAULT_SERVER_CERT_FILE    "server.crt"
#define MPR_DEFAULT_SERVER_KEY_FILE     "server.key.pem"
#define MPR_DEFAULT_CLIENT_CERT_FILE    "client.crt"
#define MPR_DEFAULT_CLIENT_CERT_PATH    "certs"

typedef struct MprSsl {
    /*
     *  Server key and certificate configuration
     */
    char            *keyFile;
    char            *certFile;
    char            *caFile;
    char            *caPath;
    char            *ciphers;

    /*
     *  Client configuration
     */
    int             verifyClient;
    int             verifyDepth;

    int             protocols;
    bool            initialized;
    bool            connTraced;

    /*
     *  Per-SSL provider context information
     */
#if BLD_FEATURE_MATRIXSSL
    sslKeys_t       *keys;
#endif

#if BLD_FEATURE_OPENSSL
    SSL_CTX         *context;
    RSA             *rsaKey512;
    RSA             *rsaKey1024;
    DH              *dhKey512;
    DH              *dhKey1024;
#endif
} MprSsl;


typedef struct MprSslSocket
{
    MprSocket       *sock;
    MprSsl          *ssl;
#if BLD_FEATURE_OPENSSL
    SSL             *osslStruct;
    BIO             *bio;
#endif
#if BLD_FEATURE_MATRIXSSL
    ssl_t           *mssl;
    sslBuf_t        insock;             /* Cached ciphertext from socket */
    sslBuf_t        inbuf;              /* Cached (decoded) plaintext */
    sslBuf_t        outsock;            /* Cached ciphertext to socket */
    int             outBufferCount;     /* Count of outgoing data we've buffered */
#endif
} MprSslSocket;


extern MprModule *mprSslInit(MprCtx ctx, cchar *path);
extern MprSsl *mprCreateSsl(MprCtx ctx);
extern void mprSetSslCiphers(MprSsl *ssl, cchar *ciphers);
extern void mprSetSslKeyFile(MprSsl *ssl, cchar *keyFile);
extern void mprSetSslCertFile(MprSsl *ssl, cchar *certFile);
extern void mprSetSslCaFile(MprSsl *ssl, cchar *caFile);
extern void mprSetSslCaPath(MprSsl *ssl, cchar *caPath);
extern void mprSetSslProtocols(MprSsl *ssl, int protocols);
extern void mprVerifySslClients(MprSsl *ssl, bool on);

#if BLD_FEATURE_OPENSSL
extern int mprCreateOpenSslModule(MprCtx ctx, bool lazy);
#endif

#if BLD_FEATURE_MATRIXSSL
extern int mprCreateMatrixSslModule(MprCtx ctx, bool lazy);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _h_MPR_SSL */

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../include/mprSsl.h"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../ssl/mprMatrixssl.c"
 */
/************************************************************************/

/*
 *  matrixSslModule.c -- Support for secure sockets via MatrixSSL
 *
 *  TODO - this module needs thought about locking
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_MATRIXSSL

static MprSocket *acceptMss(MprSocket *sp, bool invokeCallback);
static void     closeMss(MprSocket *sp, bool gracefully);
static int      configureMss(MprSsl *ssl);
static int      connectMss(MprSocket *sp, cchar *host, int port, int flags);
static MprSocketProvider *createMatrixSslProvider(MprCtx ctx);
static MprSocket *createMss(MprCtx ctx, MprSsl *ssl);
static int      doHandshake(MprSocket *sp, short cipherSuite);
static int      flushMss(MprSocket *sp);
static MprSsl   *getDefaultMatrixSsl(MprCtx ctx);
static int      innerRead(MprSocket *sp, char *userBuf, int len);
static int      listenMss(MprSocket *sp, cchar *host, int port, MprSocketAcceptProc acceptFn, void *data, int flags);
static int      matrixSslDestructor(MprSsl *ssl);
static int      matrixSslSocketDestructor(MprSslSocket *msp);
static int      readMss(MprSocket *sp, void *buf, int len);
static int      writeMss(MprSocket *sp, void *buf, int len);


int mprCreateMatrixSslModule(MprCtx ctx, bool lazy)
{
    MprSocketService    *ss;
    MprSocketProvider   *provider;

    mprAssert(ctx);
    
    ss = mprGetMpr(ctx)->socketService;

#if FIX
    ssl = mprAllocObjWithDestructorZeroed(ss, MprSsl, matrixSslDestructor);
    if (ssl == 0) {
        return ;
    }

    ssl->config.protocols = MPR_HTTP_PROTO_SSLV3 /* | MPR_HTTP_PROTO_TLSV1 */;
    ssl->config.verifyDepth = 6;

    /*
     *  Default initialization
     */
    ssl->config.ciphers = mprStrdup(ssl, MPR_DEFAULT_CIPHER_SUITE);
    ssl->config.protocols = MPR_HTTP_PROTO_SSLV3 | MPR_HTTP_PROTO_TLSV1;
#endif

    /*
     *  Install this module as the SSL provider (can only have 1)
     */
    if ((provider = createMatrixSslProvider(ctx)) == 0) {
        return 0;
    }
    mprSetSecureProvider(ss, provider);

    if (matrixSslOpen() < 0) {
        return 0;
    }

    if (!lazy) {
        getDefaultMatrixSsl(ctx);
    }

    return 0;
}


static MprSsl *getDefaultMatrixSsl(MprCtx ctx)
{
    MprSocketService    *ss;
    MprSsl              *ssl;

    ss = mprGetMpr(ctx)->socketService;

    if (ss->secureProvider->defaultSsl) {
        return ss->secureProvider->defaultSsl;
    }

    if ((ssl = mprCreateSsl(ctx)) == 0) {
        return 0;
    }
    ss->secureProvider->defaultSsl = ssl;
    return ssl;
}


static MprSocketProvider *createMatrixSslProvider(MprCtx ctx)
{
    MprSocketProvider   *provider;

    provider = mprAllocObjZeroed(ctx, MprSocketProvider);
    if (provider == 0) {
        return 0;
    }

    provider->name = "MatrixSsl";
    provider->acceptSocket = acceptMss;
    provider->closeSocket = closeMss;
    provider->configureSsl = configureMss;
    provider->connectSocket = connectMss;
    provider->createSocket = createMss;
    provider->flushSocket = flushMss;
    provider->listenSocket = listenMss;
    provider->readSocket = readMss;
    provider->writeSocket = writeMss;
    return provider;
}


static int configureMss(MprSsl *ssl)
{
    MprSocketService    *ss;
    char                *password;

    ss = mprGetMpr(ssl)->socketService;

    mprSetDestructor(ssl, (MprDestructor) matrixSslDestructor);

    /*
     *  Read the certificate and the key file for this server. FUTURE - If using encrypted private keys, 
     *  we should prompt through a dialog box or on the console, for the user to enter the password
     *  rather than using NULL as the password here.
     */
    password = NULL;
    mprAssert(ssl->keys == NULL);

    if (matrixSslReadKeys(&ssl->keys, ssl->certFile, ssl->keyFile, password, NULL) < 0) {
        mprError(ssl, "MatrixSSL: Could not read or decode certificate or key file."); 
        return MPR_ERR_CANT_INITIALIZE;
    }

    /*
     *  Select the required protocols. MatrixSSL supports only SSLv3.
     */
    if (ssl->protocols & MPR_HTTP_PROTO_SSLV2) {
        mprError(ssl, "MatrixSSL: SSLv2 unsupported"); 
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (!(ssl->protocols & MPR_HTTP_PROTO_SSLV3)) {
        mprError(ssl, "MatrixSSL: SSLv3 not enabled, unable to continue"); 
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (ssl->protocols & MPR_HTTP_PROTO_TLSV1) {
        mprLog(ssl, 2, "MatrixSSL: Warning, TLSv1 not supported. Using SSLv3 only.");
    }
    return 0;
}


static int matrixSslDestructor(MprSsl *ssl)
{
    if (ssl->keys) {
        matrixSslFreeKeys(ssl->keys);
    }
    return 0;
}


/*
 *  Create a new Matrix socket
 */
static MprSocket *createMss(MprCtx ctx, MprSsl *ssl)
{
    MprSocketService    *ss;
    MprSocket           *sp;
    MprSslSocket        *msp;
    
    if (ssl == MPR_SECURE_CLIENT) {
        ssl = 0;
    }

    /*
     *  First get a standard socket
     */
    ss = mprGetMpr(ctx)->socketService;
    sp = ss->standardProvider->createSocket(ctx, ssl);
    if (sp == 0) {
        return 0;
    }
    sp->provider = ss->secureProvider;

    msp = (MprSslSocket*) mprAllocObjWithDestructorZeroed(sp, MprSslSocket, matrixSslSocketDestructor);
    if (msp == 0) {
        return 0;
    }
    sp->sslSocket = msp;
    sp->ssl = ssl;
    msp->sock = sp;

    if (ssl) {
        msp->ssl = ssl;
    }
    return sp;
}


/*
 *  Called on mprFree
 */
static int matrixSslSocketDestructor(MprSslSocket *msp)
{
    if (msp->ssl) {
        mprFree(msp->insock.buf);
        mprFree(msp->outsock.buf);
        if (msp->inbuf.buf) {
            mprFree(msp->inbuf.buf);
        }
        matrixSslDeleteSession(msp->mssl);
    }
    return 0;
}


/*
 *  Close a slock
 */
static void closeMss(MprSocket *sp, bool gracefully)
{
    MprSslSocket  *msp;

    mprAssert(sp);

    msp = sp->sslSocket;
    mprAssert(msp);

    if (!(sp->flags & MPR_SOCKET_EOF) && msp->ssl && msp->outsock.buf) {
        /*
         *  Flush data. Append a closure alert to any buffered output data, and try to send it.
         *  Don't bother retrying or blocking, we're just closing anyway.
         */
        matrixSslEncodeClosureAlert(msp->mssl, &msp->outsock);
        if (msp->outsock.start < msp->outsock.end) {
            sp->service->standardProvider->writeSocket(sp, msp->outsock.start, msp->outsock.end - msp->outsock.start);
        }
    }

    sp->service->standardProvider->closeSocket(sp, gracefully);
}


static int listenMss(MprSocket *sp, cchar *host, int port, MprSocketAcceptProc acceptFn, void *data, int flags)
{
    sp->service->standardProvider->listenSocket(sp, host, port, acceptFn, data, flags);
    return 0;
}


/*
 *  Called to accept an incoming connection request
 */
static MprSocket *acceptMss(MprSocket *listen, bool invokeCallback)
{
    MprSocket       *sp;
    MprSslSocket    *msp;

    /*
     *  Do the standard accept stuff
     */
    sp = listen->service->standardProvider->acceptSocket(listen, 0);
    if (sp == 0) {
        return 0;
    }

    msp = sp->sslSocket;
    mprAssert(msp);
    mprAssert(msp->ssl);

    /* 
     *  Associate a new ssl session with this socket.  The session represents the state of the ssl protocol over this socket. 
     *  Session caching is handled automatically by this api.
     */
    if (matrixSslNewSession(&msp->mssl, msp->ssl->keys, NULL, SSL_FLAGS_SERVER) < 0) {
        return 0;
    }

    /* 
     *  MatrixSSL doesn't provide buffers for data internally. Define them here to support buffered reading and writing 
     *  for non-blocking sockets. 
     */
    msp->insock.size = MPR_SSL_BUFSIZE;
    msp->insock.start = (uchar*) mprAlloc(msp, msp->insock.size);
    msp->insock.end = msp->insock.buf = msp->insock.start;

    msp->outsock.size = MPR_SSL_BUFSIZE;
    msp->outsock.start = (uchar*) mprAlloc(msp, msp->outsock.size);
    msp->outsock.end = msp->outsock.buf = msp->outsock.start;

    msp->inbuf.size = 0;
    msp->inbuf.start = msp->inbuf.end = msp->inbuf.buf = 0;
    msp->outBufferCount = 0;

    /*
     *  Call the user accept callback. We do not remember the socket handle, it is up to the callback to manage it 
     *  from here on. The callback can delete the socket.
     */
    if (invokeCallback) {
        if (sp->acceptCallback) {
            (sp->acceptCallback)(sp->acceptData, sp, sp->clientIpAddr, sp->port);
        } else {
            mprFree(sp);
            return 0;
        }
    }
    return sp;
}


/*
 *  Validate the certificate. TODO expand.
 */
static int certValidator(sslCertInfo_t *cert, void *arg)
{
    sslCertInfo_t   *next;
    
    /*
     *    Make sure we are checking the last cert in the chain
     */
    next = cert;
    while (next->next != NULL) {
        next = next->next;
    }
    
    /*
     *  Flag a non-authenticated server correctly. Call matrixSslGetAnonStatus later to 
     *  see the status of this connection.
     */
    if (next->verified != 1) {
        return SSL_ALLOW_ANON_CONNECTION;
    }
    return next->verified;
}


/*
 *  Connect as a client
 */
static int connectMss(MprSocket *sp, cchar *host, int port, int flags)
{
    MprSocketService    *ss;
    MprSslSocket        *msp;
    MprSsl              *ssl;
    
    ss = sp->service;

    if (sp->service->standardProvider->connectSocket(sp, host, port, flags) < 0) {
        return MPR_ERR_CANT_CONNECT;
    }

    msp = sp->sslSocket;
    mprAssert(msp);

    if (ss->secureProvider->defaultSsl == 0) {
        if ((ssl = getDefaultMatrixSsl(sp)) == 0) {
            mprAssert(0);
            return MPR_ERR_CANT_INITIALIZE;
        }
    } else {
        ssl = ss->secureProvider->defaultSsl;
    }
    msp->ssl = ssl;

    if (matrixSslNewSession(&msp->mssl, ssl->keys, NULL, /* SSL_FLAGS_CLIENT_AUTH */ 0) < 0) {
        return -1;
    }
    
    /*
     *  Configure the certificate validator and do the SSL handshake
     */
    matrixSslSetCertValidator(msp->mssl, certValidator, NULL);
    
    if (doHandshake(sp, 0) < 0) {
        return -1;
    }

    return 0;
}


static int blockingWrite(MprSocket *sp, sslBuf_t *out)
{
    MprSocketProvider   *standard;
    uchar               *s;
    int                 bytes;

    standard = sp->service->standardProvider;
    
    s = out->start;
    while (out->start < out->end) {
        bytes = standard->writeSocket(sp, out->start, (int) (out->end - out->start));
        if (bytes < 0) {
            return -1;
            
        } else if (bytes == 0) {
            mprSleep(sp, 10);
        }
        out->start += bytes;
    }
    return (int) (out->start - s);
}


/*
 *  Construct the initial HELLO message to send to the server and initiate
 *  the SSL handshake. Can be used in the re-handshake scenario as well.
 */
static int doHandshake(MprSocket *sp, short cipherSuite)
{
    MprSslSocket    *msp;
    char            buf[MPR_SSL_BUFSIZE];
    int             bytes, rc;

    msp = sp->sslSocket;

    /*
     *  MatrixSSL doesn't provide buffers for data internally.  Define them here to support buffered reading 
     *  and writing for non-blocking sockets. Although it causes quite a bit more work, we support dynamically growing
     *  the buffers as needed.
     */
    msp->insock.size = MPR_SSL_BUFSIZE;
    msp->insock.start = msp->insock.end = msp->insock.buf = mprAlloc(msp, msp->insock.size);
    msp->outsock.size = MPR_SSL_BUFSIZE;
    msp->outsock.start = msp->outsock.end = msp->outsock.buf = mprAlloc(msp, msp->outsock.size);
    msp->inbuf.size = 0;
    msp->inbuf.start = msp->inbuf.end = msp->inbuf.buf = NULL;

    bytes = matrixSslEncodeClientHello(msp->mssl, &msp->outsock, cipherSuite);
    if (bytes < 0) {
        mprAssert(bytes < 0);
        goto error;
    }

    /*
     *  Send the hello with a blocking write
     */
    if (blockingWrite(sp, &msp->outsock) < 0) {
        mprError(msp, "MatrixSSL: Error in socketWrite");
        goto error;
    }
    msp->outsock.start = msp->outsock.end = msp->outsock.buf;

    /*
     *  Call sslRead to work through the handshake. Not actually expecting data back, so the finished case 
     *  is simply when the handshake is complete.
     */
readMore:
    rc = innerRead(sp, buf, sizeof(buf));
    
    /*
     *  Reading handshake records should always return 0 bytes, we aren't expecting any data yet.
     */
    if (rc == 0) {
        if (mprGetSocketEof(sp)) {
            goto error;
        }
#if UNUSED
        if (status == SSLSOCKET_EOF || status == SSLSOCKET_CLOSE_NOTIFY) {
            goto error;
        }
#endif
        if (matrixSslHandshakeIsComplete(msp->mssl) == 0) {
            goto readMore;
        }

    } else if (rc > 0) {
        mprError(msp, "MatrixSSL: sslRead got %d data in sslDoHandshake %s", rc, buf);
        goto readMore;

    } else {
        mprError(msp, "MatrixSSL: sslRead error in sslDoHandhake");
        goto error;
    }

    return 0;

error:
    return MPR_ERR_CANT_INITIALIZE;
}


/*
 *  Determine if there is buffered data available
 */
static bool isBufferedData(MprSslSocket *msp)
{
    if (msp->ssl == NULL) {
        return 0;
    }

    //  TODO - locking
    if (msp->inbuf.buf && msp->inbuf.start < msp->inbuf.end) {
        return 1;
    }

    if (msp->insock.start < msp->insock.end) {
        return 1;
    }

    return 0;
}


static int innerRead(MprSocket *sp, char *userBuf, int len)
{
    MprSslSocket  *msp;
    MprSocketProvider   *standard;
    sslBuf_t            *inbuf;     /* Cached decoded plain text */
    sslBuf_t            *insock;    /* Cached ciphertext to socket */
    uchar               *buf, error, alertLevel, alertDescription;
    int                 bytes, rc, space, performRead, remaining;

    msp = (MprSslSocket*) sp->sslSocket;
    buf = (uchar*) userBuf;

    inbuf = &msp->inbuf;
    insock = &msp->insock;
    standard = sp->service->standardProvider;

    /*
     *  If inbuf is valid, then we have previously decoded data that must be returned, return as much as possible.  
     *  Once all buffered data is returned, free the inbuf.
     */
    if (inbuf->buf) {

        if (inbuf->start < inbuf->end) {
            remaining = (int) (inbuf->end - inbuf->start);
            bytes = min(len, remaining);
            memcpy(buf, inbuf->start, bytes);
            inbuf->start += bytes;
            return bytes;
        }

        mprFree(inbuf->buf);
        inbuf->buf = NULL;
    }

    /*
     *  Pack the buffered socket data (if any) so that start is at zero.
     */
    if (insock->buf < insock->start) {
        if (insock->start == insock->end) {
            insock->start = insock->end = insock->buf;
        } else {
            memmove(insock->buf, insock->start, insock->end - insock->start);
            insock->end -= (insock->start - insock->buf);
            insock->start = insock->buf;
        }
    }

    performRead = 0;

    /*
     *  If we have data still in the buffer, we must process if we can without another read (incase there 
     *  is no more data to read and we block).
     */
    if (insock->end == insock->start) {
readMore:
        //
        //  Read up to as many bytes as there are remaining in the buffer.
        //
        if ((insock->end - insock->start) < insock->size) {
            performRead = 1;
            bytes = standard->readSocket(sp, insock->end, (insock->buf + insock->size) - insock->end);
            if (bytes <= 0 && (insock->end == insock->start)) {
                return bytes;
            }
            insock->end += bytes;
        }
    }

    /*
     *  Define a temporary sslBuf
     */
    inbuf->start = inbuf->end = inbuf->buf = (uchar*) mprAlloc(msp, len);
    inbuf->size = len;

decodeMore:
    /*
     *  Decode the data we just read from the socket
     */
    error = 0;
    alertLevel = 0;
    alertDescription = 0;

    rc = matrixSslDecode(msp->mssl, insock, inbuf, &error, &alertLevel, &alertDescription);
    switch (rc) {

    /*
     *  Successfully decoded a record that did not return data or require a response.
     */
    case SSL_SUCCESS:
        if (insock->end > insock->start) {
            goto decodeMore;
        }
        return 0;

    /*
     *  Successfully decoded an application data record, and placed in tmp buf
     */
    case SSL_PROCESS_DATA:
        //
        //  Copy as much as we can from the temp buffer into the caller's buffer
        //  and leave the remainder in inbuf until the next call to read
        //
        space = (inbuf->end - inbuf->start);
        len = min(space, len);
        memcpy(buf, inbuf->start, len);
        inbuf->start += len;
        return len;

    /*
     *  We've decoded a record that requires a response into tmp If there is no data to be flushed in the out 
     *  buffer, we can write out the contents of the tmp buffer.  Otherwise, we need to append the data 
     *  to the outgoing data buffer and flush it out.
     */
    case SSL_SEND_RESPONSE:
        bytes = standard->writeSocket(sp, inbuf->start, inbuf->end - inbuf->start);
        inbuf->start += bytes;
        if (inbuf->start < inbuf->end) {
            mprSetSocketBlockingMode(sp, 1);
            while (inbuf->start < inbuf->end) {
                bytes = standard->writeSocket(sp, inbuf->start, inbuf->end - inbuf->start);
                if (bytes < 0) {
                    goto readError;
                }
                inbuf->start += bytes;
            }
            mprSetSocketBlockingMode(sp, 0);
        }
        inbuf->start = inbuf->end = inbuf->buf;
        if (insock->end > insock->start) {
            goto decodeMore;
        }
        return 0;

    /*
     *  There was an error decoding the data, or encoding the out buffer. There may be a response data in the out 
     *  buffer, so try to send. We try a single hail-mary send of the data, and then close the socket. Since we're 
     *  closing on error, we don't worry too much about a clean flush.
     */
    case SSL_ERROR:
        mprLog(sp, 4, "MatrixSSL: Closing on protocol error %d", error);
        if (inbuf->start < inbuf->end) {
            mprSetSocketBlockingMode(sp, 0);
            bytes = standard->writeSocket(sp, inbuf->start, inbuf->end - inbuf->start);
        }
        goto readError;

    /*
     *  We've decoded an alert.  The level and description passed into matrixSslDecode are filled in with the specifics.
     */
    case SSL_ALERT:
        if (alertDescription == SSL_ALERT_CLOSE_NOTIFY) {
            //  TODO - should this be a close?
            goto readZero;
        }
        mprLog(sp, 4, "MatrixSSL: Closing on client alert %d: %d", alertLevel, alertDescription);
        goto readError;

    /*
     *  We have a partial record, we need to read more data off the socket. If we have a completely full insock buffer,
     *  we'll need to grow it here so that we CAN read more data when called the next time.
     */
    case SSL_PARTIAL:
        if (insock->start == insock->buf && insock->end == (insock->buf + insock->size)) {
            if (insock->size > SSL_MAX_BUF_SIZE) {
                goto readError;
            }
            insock->size *= 2;
            insock->start = insock->buf = (uchar*) mprRealloc(msp, insock->buf, insock->size);
            insock->end = insock->buf + (insock->size / 2);
        }
        if (!performRead) {
            performRead = 1;
            mprFree(inbuf->buf);
            inbuf->buf = 0;
            goto readMore;
        }
        goto readZero;

    /*
     *  The out buffer is too small to fit the decoded or response data. Increase the size of the buffer and 
     *  call decode again.
     */
    case SSL_FULL:
        mprAssert(inbuf->start == inbuf->end);
        inbuf->size *= 2;
        if (inbuf->buf != buf) {
            mprFree(inbuf->buf);
            inbuf->buf = 0;
        }
        inbuf->start = (uchar*) mprAlloc(msp, inbuf->size);
        inbuf->end = inbuf->buf = inbuf->start;
        goto decodeMore;
    }

readZero:
    return 0;

readError:
    sp->flags |= MPR_SOCKET_EOF;
    return -1;
}


static int readMss(MprSocket *sp, void *buf, int len)
{
    MprSslSocket  *msp;
    int                 bytes;

    msp = (MprSslSocket*) sp->sslSocket;

    if (msp->ssl == NULL || len <= 0) {
        return -1;
    }

    bytes = innerRead(sp, buf, len);

    /*
     *  If there is more data buffered locally here, then ensure the select handler will recall us again even 
     *  if there is no more IO events
     */
    if (isBufferedData(msp)) {
        //  TODO - locking
        if (sp->handler) {
            mprRecallWaitHandler(sp->handler);
        }
    }
    return bytes;
}


/*
 *  Write data. Return the number of bytes written or -1 on errors.
 *
 *  Encode caller's data buffer into an SSL record and write to socket. The encoded data will always be 
 *  bigger than the incoming data because of the record header (5 bytes) and MAC (16 bytes MD5 / 20 bytes SHA1)
 *  This would be fine if we were using blocking sockets, but non-blocking presents an interesting problem.  Example:
 *
 *      A 100 byte input record is encoded to a 125 byte SSL record
 *      We can send 124 bytes without blocking, leaving one buffered byte
 *      We can't return 124 to the caller because it's more than they requested
 *      We can't return 100 to the caller because they would assume all data
 *      has been written, and we wouldn't get re-called to send the last byte
 *
 *  We handle the above case by returning 0 to the caller if the entire encoded record could not be sent. Returning 
 *  0 will prompt us to select this socket for write events, and we'll be called again when the socket is writable.  
 *  We'll use this mechanism to flush the remaining encoded data, ignoring the bytes sent in, as they have already 
 *  been encoded.  When it is completely flushed, we return the originally requested length, and resume normal 
 *  processing.
 */
static int writeMss(MprSocket *sp, void *buf, int len)
{
    MprSslSocket    *msp;
    sslBuf_t        *outsock;
    int             rc;

    msp = (MprSslSocket*) sp->sslSocket;
    outsock = &msp->outsock;

    if (len > SSL_MAX_PLAINTEXT_LEN) {
        len = SSL_MAX_PLAINTEXT_LEN;
    }

    /*
     *  Pack the buffered socket data (if any) so that start is at zero.
     */
    if (outsock->buf < outsock->start) {
        if (outsock->start == outsock->end) {
            outsock->start = outsock->end = outsock->buf;
        } else {
            memmove(outsock->buf, outsock->start, outsock->end - outsock->start);
            outsock->end -= (outsock->start - outsock->buf);
            outsock->start = outsock->buf;
        }
    }

    /*
     *  If there is buffered output data, the caller must be trying to send the same amount of data as last time.  
     *  We don't support sending additional data until the original buffered request has been completely sent.
     */
    if (msp->outBufferCount > 0 && len != msp->outBufferCount) {
        mprAssert(len != msp->outBufferCount);
        return -1;
    }
    
    /*
     *  If we don't have buffered data, encode the caller's data
     */
    if (msp->outBufferCount == 0) {
retryEncode:
        rc = matrixSslEncode(msp->mssl, (uchar*) buf, len, outsock);
        switch (rc) {
        case SSL_ERROR:
            return -1;

        case SSL_FULL:
            if (outsock->size > SSL_MAX_BUF_SIZE) {
                return -1;
            }
            outsock->size *= 2;
            outsock->buf = (uchar*) mprRealloc(msp, outsock->buf, outsock->size);
            outsock->end = outsock->buf + (outsock->end - outsock->start);
            outsock->start = outsock->buf;
            goto retryEncode;
        }
    }

    /*
     *  We've got data to send.  Try to write it all out.
     */
    rc = sp->service->standardProvider->writeSocket(sp, outsock->start, outsock->end - outsock->start);
    if (rc <= 0) {
        return rc;
    }
    outsock->start += rc;

    /*
     *  If we wrote it all return the length, otherwise remember the number of bytes passed in, and return 0 
     *  to be called again later.
     */
    if (outsock->start == outsock->end) {
        msp->outBufferCount = 0;
        return len;
    }
    msp->outBufferCount = len;
    return 0;
}


static int flushMss(MprSocket *sp)
{
    MprSslSocket  *msp;

    msp = (MprSslSocket*) sp->sslSocket;

    //  TODO - what about locking
    if (msp->outsock.start < msp->outsock.end) {
        return sp->service->standardProvider->writeSocket(sp, msp->outsock.start, msp->outsock.end - msp->outsock.start);
    }
    return 0;
}

 
#if UNUSED
/*
 *  Return true if end of file
 */
bool MaMss::getEof()
{
    return 0;
}
#endif


#else
void mprMatrixSslModuleDummy() {}
#endif /* BLD_FEATURE_MATRIXSSL */


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../ssl/mprMatrixssl.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../ssl/mprOpenssl.c"
 */
/************************************************************************/

/*
 *  mprOpenSsl.c - Support for secure sockets via OpenSSL
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_OPENSSL

#include    <openssl/dh.h>

/*
 *  OpenSSL requires this static code. Ugh!
 */
typedef struct RandBuf {
    MprTime     now;
    int         pid;
} RandBuf;

#if BLD_FEATURE_MULTITHREAD
static MprMutex **locks;
static int      numLocks;

struct CRYPTO_dynlock_value {
    MprMutex    *mutex;
};
typedef struct CRYPTO_dynlock_value DynLock;
#endif


static MprSocket *acceptOss(MprSocket *sp, bool invokeCallback);
static void     closeOss(MprSocket *sp, bool gracefully);
static MprSsl   *getDefaultOpenSsl(MprCtx ctx);
static int      configureCertificates(MprSsl *ssl, SSL_CTX *ctx, char *key, char *cert);
static int      configureOss(MprSsl *ssl);
static int      connectOss(MprSocket *sp, cchar *host, int port, int flags);
static MprSocketProvider *createOpenSslProvider(MprCtx ctx);
static MprSocket *createOss(MprCtx ctx, MprSsl *ssl);
static DH       *dhCallback(SSL *ssl, int isExport, int keyLength);
static int      flushOss(MprSocket *sp);
static int      listenOss(MprSocket *sp, cchar *host, int port, MprSocketAcceptProc acceptFn, void *data, int flags);
static int      openSslDestructor(MprSsl *ssl);
static int      openSslSocketDestructor(MprSslSocket *ssp);
static int      readOss(MprSocket *sp, void *buf, int len);
static RSA      *rsaCallback(SSL *ssl, int isExport, int keyLength);
static int      verifyX509Certificate(int ok, X509_STORE_CTX *ctx);
static int      writeOss(MprSocket *sp, void *buf, int len);

#if BLD_FEATURE_MULTITHREAD
static DynLock  *sslCreateDynLock(const char *file, int line);
static void     sslDynLock(int mode, DynLock *dl, const char *file, int line);
static void     sslDestroyDynLock(DynLock *dl, const char *file, int line);
static void     sslStaticLock(int mode, int n, const char *file, int line);
static ulong    sslThreadId(void);
#endif

static DH       *get_dh512();
static DH       *get_dh1024();


int mprCreateOpenSslModule(MprCtx ctx, bool lazy)
{
    MprSocketService    *ss;
    MprSocketProvider   *provider;
    RandBuf             randBuf;

    ss = mprGetMpr(ctx)->socketService;

    /*
     *  Get some random bytes
     */
    randBuf.now = mprGetTime(ss);
    randBuf.pid = getpid();
    RAND_seed((void*) &randBuf, sizeof(randBuf));

#if SOLARIS || LINUX || MACOSX || FREEBSD
    mprLog(ctx, 6, "OpenSsl: Before calling RAND_load_file");
    RAND_load_file("/dev/urandom", 256);
    mprLog(ctx, 6, "OpenSsl: After calling RAND_load_file");
#endif

#if BLD_FEATURE_MULTITHREAD
    {
    int                 i;
    /*
     *  Configure the global locks
     */
    numLocks = CRYPTO_num_locks();
    locks = (MprMutex**) mprAlloc(ctx, numLocks * sizeof(MprMutex*));
    for (i = 0; i < numLocks; i++) {
        locks[i] = mprCreateLock(ctx);
    }
    CRYPTO_set_id_callback(sslThreadId);
    CRYPTO_set_locking_callback(sslStaticLock);

    CRYPTO_set_dynlock_create_callback(sslCreateDynLock);
    CRYPTO_set_dynlock_destroy_callback(sslDestroyDynLock);
    CRYPTO_set_dynlock_lock_callback(sslDynLock);
    }
#endif

#if !BLD_WIN_LIKE
    OpenSSL_add_all_algorithms();
#endif

    SSL_library_init();

    if ((provider = createOpenSslProvider(ctx)) == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    mprSetSecureProvider(ss, provider);

    if (!lazy) {
        getDefaultOpenSsl(ctx);
    }
    
    return 0;
}


static MprSsl *getDefaultOpenSsl(MprCtx ctx)
{
    MprSocketService    *ss;
    MprSsl              *ssl;

    ss = mprGetMpr(ctx)->socketService;

    if (ss->secureProvider->defaultSsl) {
        return ss->secureProvider->defaultSsl;
    }

    if ((ssl = mprCreateSsl(ctx)) == 0) {
        return 0;
    }

    /*
     *  Pre-generate some keys that are slow to compute.
     */
    ssl->rsaKey512 = RSA_generate_key(512, RSA_F4, 0, 0);
    ssl->rsaKey1024 = RSA_generate_key(1024, RSA_F4, 0, 0);
    ssl->dhKey512 = get_dh512();
    ssl->dhKey1024 = get_dh1024();

    ss->secureProvider->defaultSsl = ssl;

    return ssl;
}


/*
 *  Initialize a provider structure for OpenSSL
 */
static MprSocketProvider *createOpenSslProvider(MprCtx ctx)
{
    MprSocketProvider   *provider;

    provider = mprAllocObjZeroed(ctx, MprSocketProvider);
    if (provider == 0) {
        return 0;
    }

    provider->name = "OpenSsl";
    provider->acceptSocket = acceptOss;
    provider->closeSocket = closeOss;
    provider->configureSsl = configureOss;
    provider->connectSocket = connectOss;
    provider->createSocket = createOss;
    provider->flushSocket = flushOss;
    provider->listenSocket = listenOss;
    provider->readSocket = readOss;
    provider->writeSocket = writeOss;
    return provider;
}


/*
 *  Configure the SSL configuration. Called from connect or explicitly in server code
 *  to setup various SSL contexts. Appweb uses this from location.c.
 */
static int configureOss(MprSsl *ssl)
{
    MprSocketService    *ss;
    MprSsl              *defaultSsl;
    SSL_CTX             *context;

    ss = mprGetMpr(ssl)->socketService;

    mprSetDestructor(ssl, (MprDestructor) openSslDestructor);

    context = SSL_CTX_new(SSLv23_method());
    if (context == 0) {
        mprError(ssl, "OpenSSL: Unable to create SSL context"); 
        return MPR_ERR_CANT_CREATE;
    }

    SSL_CTX_set_app_data(context, (void*) ssl);
    SSL_CTX_set_quiet_shutdown(context, 1);
    SSL_CTX_sess_set_cache_size(context, 512);

    /*
     *  Configure the certificates
     */
    if (ssl->keyFile || ssl->certFile) {
        if (configureCertificates(ssl, context, ssl->keyFile, ssl->certFile) != 0) {
            SSL_CTX_free(context);
            return MPR_ERR_CANT_INITIALIZE;
        }
    }

    mprLog(ssl, 4, "OpenSSL: Using ciphers %s", ssl->ciphers);
    SSL_CTX_set_cipher_list(context, ssl->ciphers);

    /*
     *  Configure the client verification certificate locations
     */
    if (ssl->verifyClient) {
        if (ssl->caFile == 0 && ssl->caPath == 0) {
            mprError(ssl, "OpenSSL: Must define CA certificates if using client verification");
            SSL_CTX_free(context);
            return MPR_ERR_BAD_STATE;
        }
        if (ssl->caFile || ssl->caPath) {
            if ((!SSL_CTX_load_verify_locations(context, ssl->caFile, ssl->caPath)) ||
                    (!SSL_CTX_set_default_verify_paths(context))) {
                mprError(ssl, "OpenSSL: Unable to set certificate locations"); 
                SSL_CTX_free(context);
                return MPR_ERR_CANT_ACCESS;
            }
            if (ssl->caFile) {
                STACK_OF(X509_NAME)     *certNames;
                certNames = SSL_load_client_CA_file(ssl->caFile);
                if (certNames == 0) {
                    /*  TODO - what here? */
                } else {
                    /*
                     *  Define the list of CA certificates to send to the client
                     *  before they send their client certificate for validation
                     */
                    SSL_CTX_set_client_CA_list(context, certNames);
                }
            }
        }

        mprLog(ssl, 4, "OpenSSL: enable verification of client connections");

        if (ssl->caFile) {
            mprLog(ssl, 4, "OpenSSL: Using certificates from %s", ssl->caFile);

        } else if (ssl->caPath) {
            mprLog(ssl, 4, "OpenSSL: Using certificates from directory %s", ssl->caPath);
        }

        SSL_CTX_set_verify(context, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, verifyX509Certificate);
        SSL_CTX_set_verify_depth(context, ssl->verifyDepth);

    } else {
        SSL_CTX_set_verify(context, SSL_VERIFY_NONE, verifyX509Certificate);
    }

    /*
     *  Define callbacks
     */
    SSL_CTX_set_tmp_rsa_callback(context, rsaCallback);
    SSL_CTX_set_tmp_dh_callback(context, dhCallback);

    /*
     *  Enable all buggy client work-arounds 
     */
    SSL_CTX_set_options(context, SSL_OP_ALL);
    SSL_CTX_set_mode(context, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_AUTO_RETRY);

    /*
     *  Select the required protocols
     */
    if (!(ssl->protocols & MPR_HTTP_PROTO_SSLV2)) {
        SSL_CTX_set_options(context, SSL_OP_NO_SSLv2);
        mprLog(ssl, 4, "OpenSSL: Disabling SSLv2");
    }
    if (!(ssl->protocols & MPR_HTTP_PROTO_SSLV3)) {
        SSL_CTX_set_options(context, SSL_OP_NO_SSLv3);
        mprLog(ssl, 4, "OpenSSL: Disabling SSLv3");
    }
    if (!(ssl->protocols & MPR_HTTP_PROTO_TLSV1)) {
        SSL_CTX_set_options(context, SSL_OP_NO_TLSv1);
        mprLog(ssl, 4, "OpenSSL: Disabling TLSv1");
    }

    /* 
     *  Ensure we generate a new private key for each connection
     */
    SSL_CTX_set_options(context, SSL_OP_SINGLE_DH_USE);

    if ((defaultSsl = getDefaultOpenSsl(ss)) == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    if (ssl != defaultSsl) {
        ssl->rsaKey512 = defaultSsl->rsaKey512;
        ssl->rsaKey1024 = defaultSsl->rsaKey1024;
        ssl->dhKey512 = defaultSsl->dhKey512;
        ssl->dhKey1024 = defaultSsl->dhKey1024;
    }
    ssl->context = context;

    return 0;
}


/*
 *  Update the destructor for the MprSsl object. 
 */
static int openSslDestructor(MprSsl *ssl)
{
    if (ssl->context != 0) {
        SSL_CTX_free(ssl->context);
    }
    if (ssl->rsaKey512) {
        RSA_free(ssl->rsaKey512);
    }
    if (ssl->rsaKey1024) {
        RSA_free(ssl->rsaKey1024);
    }
    if (ssl->dhKey512) {
        DH_free(ssl->dhKey512);
    }
    if (ssl->dhKey1024) {
        DH_free(ssl->dhKey1024);
    }
    return 0;
}


/*
 *  Configure the SSL certificate information configureOss
 */
static int configureCertificates(MprSsl *ssl, SSL_CTX *ctx, char *key, char *cert)
{
    mprAssert(ctx);

    if (cert == 0) {
        return 0;
    }

    if (cert && SSL_CTX_use_certificate_chain_file(ctx, cert) <= 0) {
        mprError(ssl, "OpenSSL: Can't define certificate file: %s", cert); 
        return -1;
    }

    key = (key == 0) ? cert : key;
    if (key) {
        if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0) {
            mprError(ssl, "OpenSSL: Can't define private key file: %s", key); 
            return -1;
        }

        if (!SSL_CTX_check_private_key(ctx)) {
            mprError(ssl, "OpenSSL: Check of private key file failed: %s", key);
            return -1;
        }
    }
    return 0;
}


/*
 *  Create a new socket. If listen is set, this is a socket for an accepting connection.
 */
static MprSocket *createOss(MprCtx ctx, MprSsl *ssl)
{
    MprSocketService    *ss;
    MprSocket           *sp;
    MprSslSocket        *osp;
    
    if (ssl == MPR_SECURE_CLIENT) {
        ssl = 0;
    }

    /*
     *  First get a standard socket
     */
    ss = mprGetMpr(ctx)->socketService;
    sp = ss->standardProvider->createSocket(ctx, ssl);
    if (sp == 0) {
        return 0;
    }
    sp->provider = ss->secureProvider;

    /*
     *  Create a SslSocket object for ssl state. This logically extends MprSocket.
     */
    osp = (MprSslSocket*) mprAllocObjWithDestructorZeroed(sp, MprSslSocket, openSslSocketDestructor);
    if (osp == 0) {
        return 0;
    }
    sp->sslSocket = osp;
    sp->ssl = ssl;
    osp->sock = sp;

    if (ssl) {
        osp->ssl = ssl;
    }
    return sp;
}


/*
 *  Destructor for an MprSslSocket object
 */
static int openSslSocketDestructor(MprSslSocket *osp)
{
    if (osp->osslStruct) {
        SSL_set_shutdown(osp->osslStruct, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
    }
    if (osp->bio) {
        BIO_free_all(osp->bio);
        osp->bio = 0;
    }
    return 0;
}


static void closeOss(MprSocket *sp, bool gracefully)
{
    sp->service->standardProvider->closeSocket(sp, gracefully);
}


/*
 *  Initialize a new server-side connection. Called by listenOss and by acceptOss.
 */
static int listenOss(MprSocket *sp, cchar *host, int port, MprSocketAcceptProc acceptFn, void *data, int flags)
{
    sp->service->standardProvider->listenSocket(sp, host, port, acceptFn, data, flags);
    return 0;
}


/*
 *  Initialize a new server-side connection
 */
static MprSocket *acceptOss(MprSocket *listen, bool invokeCallback)
{
    MprSocket       *sp;
    MprSslSocket    *osp;
    BIO             *bioSock;
    SSL             *osslStruct;

    sp = listen->service->standardProvider->acceptSocket(listen, 0);
    if (sp == 0) {
        return 0;
    }

    osp = sp->sslSocket;
    mprAssert(osp);
    mprAssert(osp->ssl);

    /*
     *  Create and configure the SSL struct
     */
    osslStruct = osp->osslStruct = (SSL*) SSL_new(osp->ssl->context);
    mprAssert(osslStruct);
    if (osslStruct == 0) {
        mprAssert(osslStruct == 0);
        return 0;
    }
    SSL_set_app_data(osslStruct, (void*) osp);

    /*
     *  Create a socket bio
     */
    bioSock = BIO_new_socket(sp->fd, BIO_NOCLOSE);
    mprAssert(bioSock);
    SSL_set_bio(osslStruct, bioSock, bioSock);
    SSL_set_accept_state(osslStruct);
    osp->bio = bioSock;

    /*
     *  Call the user accept callback. We do not remember the socket handle, it is up to the callback to manage it 
     *  from here on. The callback can delete the socket.
     */
    if (invokeCallback) {
        if (sp->acceptCallback) {
            (sp->acceptCallback)(sp->acceptData, sp, sp->clientIpAddr, sp->port);
        } else {
            mprFree(sp);
            return 0;
        }
    }
    return sp;
}


/*
 *  Initialize a new client connection
 */
static int connectOss(MprSocket *sp, cchar *host, int port, int flags)
{
    MprSocketService    *ss;
    MprSslSocket        *osp;
    MprSsl              *ssl;
    BIO                 *bioSock;
    int                 rc;
    
    ss = sp->service;
    if (ss->standardProvider->connectSocket(sp, host, port, flags) < 0) {
        return MPR_ERR_CANT_CONNECT;
    }
    
    osp = sp->sslSocket;
    mprAssert(osp);

    if (ss->secureProvider->defaultSsl == 0) {
        if ((ssl = getDefaultOpenSsl(sp)) == 0) {
            mprAssert(0);
            return MPR_ERR_CANT_INITIALIZE;
        }
    } else {
        ssl = ss->secureProvider->defaultSsl;
    }
    osp->ssl = ssl;

    if (ssl->context == 0 && configureOss(ssl) < 0) {
        mprAssert(0);
        return MPR_ERR_CANT_INITIALIZE;
    }

    /*
     *  Create and configure the SSL struct
     */
    osp->osslStruct = (SSL*) SSL_new(ssl->context);
    mprAssert(osp->osslStruct);
    if (osp->osslStruct == 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }
    SSL_set_app_data(osp->osslStruct, (void*) osp);

    /*
     *  Create a socket bio
     */
    bioSock = BIO_new_socket(sp->fd, BIO_NOCLOSE);
    mprAssert(bioSock);
    SSL_set_bio(osp->osslStruct, bioSock, bioSock);

    osp->bio = bioSock;

    /*
     *  Make the socket blocking while we connect
     */
    mprSetSocketBlockingMode(sp, 1);
    
    rc = SSL_connect(osp->osslStruct);
    if (rc < 1) {
#if TODO && UNUSED
        rc = SSL_get_error(osp->osslStruct, rc);
        if (rc == SSL_ERROR_WANT_READ) {
            rc = SSL_connect(osp->osslStruct);
        }
#endif
        return MPR_ERR_CANT_CONNECT;
    }
    
    mprSetSocketBlockingMode(sp, 0);

    return 0;
}


static int readOss(MprSocket *sp, void *buf, int len)
{
    MprSslSocket    *osp;
    int             rc, error;

    osp = (MprSslSocket*) sp->sslSocket;
    mprAssert(osp);

    if (osp->osslStruct == 0) {
        mprAssert(osp->osslStruct);
        return -1;
    }

    rc = SSL_read(osp->osslStruct, buf, len);

#if DEBUG
    if (rc > 0 && !connTraced) {
        X509_NAME   *xSubject;
        X509        *cert;
        char        subject[260], issuer[260], peer[260];

        mprLog(ssl, 4, "%d: OpenSSL Connected using: \"%s\"", sock, SSL_get_cipher(ssl));

        cert = SSL_get_peer_certificate(ssl);
        if (cert == 0) {
            mprLog(ssl, 4, "%d: OpenSSL Details: client supplied no certificate", sock);

        } else {
            xSubject = X509_get_subject_name(cert);
            X509_NAME_oneline(xSubject, subject, sizeof(subject) -1);
            X509_NAME_oneline(X509_get_issuer_name(cert), issuer, sizeof(issuer) -1);
            X509_NAME_get_text_by_NID(xSubject, NID_commonName, peer, sizeof(peer) - 1);
            mprLog(ssl, 4, "%d: OpenSSL Subject %s", sock, subject);
            mprLog(ssl, 4, "%d: OpenSSL Issuer: %s", sock, issuer);
            mprLog(ssl, 4, "%d: OpenSSL Peer: %s", sock, peer);
            X509_free(cert);
        }
        connTraced = 1;
    }
#endif

    if (rc <= 0) {
        error = SSL_get_error(osp->osslStruct, rc);
        if (error == SSL_ERROR_WANT_WRITE) {
            //  TODO - rethink
            mprSleep(sp, 10);
            rc = 0;
                
        } else if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_CONNECT || error == SSL_ERROR_WANT_ACCEPT) {
            mprRecallWaitHandler(sp->handler);
            rc = 0;

        } else if (error != SSL_ERROR_ZERO_RETURN) {
            /* SSL_ERROR_SYSCALL | SSL_ERROR_SSL */
            rc = -1;
        }

    } else if (SSL_pending(osp->osslStruct) > 0) {
        mprRecallWaitHandler(sp->handler);
    }

    return rc;
}


/*
 *  Write data. Return the number of bytes written or -1 on errors.
 */
static int writeOss(MprSocket *sp, void *buf, int len)
{
    MprSslSocket    *osp;
    int             rc, totalWritten;

    osp = (MprSslSocket*) sp->sslSocket;

    if (osp->bio == 0 || osp->osslStruct == 0 || len <= 0) {
        mprAssert(0);
        return -1;
    }

    totalWritten = 0;
    ERR_clear_error();

    do {
        rc = SSL_write(osp->osslStruct, buf, len);
        
        mprLog(osp, 7, "OpenSSL: written %d, requested len %d", rc, len);

        if (rc <= 0) {
            rc = SSL_get_error(osp->osslStruct, rc);
            if (rc == SSL_ERROR_WANT_WRITE) {
                //  TODO - rethink
                mprSleep(sp, 10);
                continue;
                
            } else if (rc == SSL_ERROR_WANT_READ) {
                //  AUTO-RETRY should stop this
                mprAssert(0);
                return -1;
            } else {
                return -1;
            }
        }

        totalWritten += rc;
        buf = (void*) ((char*) buf + rc);
        len -= rc;

        mprLog(osp, 7, "OpenSSL: write: len %d, written %d, total %d, error %d", len, rc, totalWritten, 
            SSL_get_error(osp->osslStruct, rc));

    } while (len > 0);

    return totalWritten;
}


/*
 *  Called to verify X509 client certificates
 */
static int verifyX509Certificate(int ok, X509_STORE_CTX *xContext)
{
    X509            *cert;
    SSL             *osslStruct;
    MprSslSocket    *osp;
    MprSsl          *ssl;
    char            subject[260], issuer[260], peer[260];
    int             error, depth;
    
//  TODO - Clients
    return 1;

    subject[0] = issuer[0] = '\0';

    osslStruct = (SSL*) X509_STORE_CTX_get_app_data(xContext);
    osp = (MprSslSocket*) SSL_get_app_data(osslStruct);
    ssl = (MprSsl*) osp->ssl;

    cert = X509_STORE_CTX_get_current_cert(xContext);
    depth = X509_STORE_CTX_get_error_depth(xContext);
    error = X509_STORE_CTX_get_error(xContext);

    if (X509_NAME_oneline(X509_get_subject_name(cert), subject, sizeof(subject) - 1) < 0) {
        ok = 0;
    }

    /*
     *  TODO -- should compare subject name and host name. Need smart compare
     */
    if (X509_NAME_oneline(X509_get_issuer_name(xContext->current_cert), issuer, sizeof(issuer) - 1) < 0) {
        ok = 0;
    }
    if (X509_NAME_get_text_by_NID(X509_get_subject_name(xContext->current_cert), NID_commonName, peer, sizeof(peer) - 1) < 0) {
        ok = 0;
    }

    /*
     *  Customizers: add your own code here to validate client certificates
     */
    if (ok && ssl->verifyDepth < depth) {
        if (error == 0) {
            error = X509_V_ERR_CERT_CHAIN_TOO_LONG;
        }
        ok = 0;
    }

    if (error != 0) {
        mprAssert(!ok);
    }

#if UNUSED
    switch (error) {
    case X509_V_ERR_CERT_HAS_EXPIRED:
    case X509_V_ERR_CERT_NOT_YET_VALID:
    case X509_V_ERR_CERT_REJECTED:
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
    case X509_V_ERR_CERT_UNTRUSTED:
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
    case X509_V_ERR_INVALID_CA:
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
    default:
        ok = 0;
        break;
    }
#endif

    if (!ok) {
        mprLog(ssl, 0, "OpenSSL: Certification failed: subject %s", subject);
        mprLog(ssl, 4, "OpenSSL: Issuer: %s", issuer);
        mprLog(ssl, 4, "OpenSSL: Peer: %s", peer);
        mprLog(ssl, 4, "OpenSSL: Error: %d: %s", error, X509_verify_cert_error_string(error));

    } else {
        mprLog(ssl, 0, "OpenSSL: Certificate verified: subject %s", subject);
        mprLog(ssl, 4, "OpenSSL: Issuer: %s", issuer);
        mprLog(ssl, 4, "OpenSSL: Peer: %s", peer);
    }
    return ok;
}


static int flushOss(MprSocket *sp)
{
#if UNUSED
    MprSslSocket    *osp;

    osp = (MprSslSocket*) sp->sslSocket;

    mprAssert(0);
    return BIO_flush(osp->bio);
#endif
    return 0;
}

 
#if BLD_FEATURE_MULTITHREAD
static ulong sslThreadId()
{
    return (long) mprGetCurrentOsThread();
}


static void sslStaticLock(int mode, int n, const char *file, int line)
{
    mprAssert(0 <= n && n < numLocks);
    if (mode & CRYPTO_LOCK) {
        mprLock(locks[n]);
    } else {
        mprUnlock(locks[n]);
    }
}


static DynLock *sslCreateDynLock(const char *file, int line)
{
    DynLock     *dl;

    dl = mprAllocObjZeroed(0, DynLock);
    dl->mutex = mprCreateLock(dl);
    return dl;
}


static void sslDestroyDynLock(DynLock *dl, const char *file, int line)
{
    mprFree(dl);
}


static void sslDynLock(int mode, DynLock *dl, const char *file, int line)
{
    if (mode & CRYPTO_LOCK) {
        mprLock(dl->mutex);
    } else {
        mprUnlock(dl->mutex);
    }
}
#endif /* BLD_FEATURE_MULTITHREAD */


/*
 *  Used for ephemeral RSA keys
 */
static RSA *rsaCallback(SSL *osslStruct, int isExport, int keyLength)
{
    MprSslSocket    *osp;
    MprSsl          *ssl;
    RSA             *key;

    osp = (MprSslSocket*) SSL_get_app_data(osslStruct);
    ssl = (MprSsl*) osp->ssl;

    key = 0;
    switch (keyLength) {
    case 512:
        key = ssl->rsaKey512;
        break;

    case 1024:
    default:
        key = ssl->rsaKey1024;
    }
    return key;
}


/*
 *  Used for ephemeral DH keys
 */
static DH *dhCallback(SSL *osslStruct, int isExport, int keyLength)
{
    MprSslSocket    *osp;
    MprSsl          *ssl;
    DH              *key;

    osp = (MprSslSocket*) SSL_get_app_data(osslStruct);
    ssl = (MprSsl*) osp->ssl;

    key = 0;
    switch (keyLength) {
    case 512:
        key = ssl->dhKey512;
        break;

    case 1024:
    default:
        key = ssl->dhKey1024;
    }
    return key;
}


/*
 *  openSslDh.c - OpenSSL DH get routines. Generated by openssl.
 */
static DH *get_dh512()
{
    static unsigned char dh512_p[] = {
        0x8E,0xFD,0xBE,0xD3,0x92,0x1D,0x0C,0x0A,0x58,0xBF,0xFF,0xE4,
        0x51,0x54,0x36,0x39,0x13,0xEA,0xD8,0xD2,0x70,0xBB,0xE3,0x8C,
        0x86,0xA6,0x31,0xA1,0x04,0x2A,0x09,0xE4,0xD0,0x33,0x88,0x5F,
        0xEF,0xB1,0x70,0xEA,0x42,0xB6,0x0E,0x58,0x60,0xD5,0xC1,0x0C,
        0xD1,0x12,0x16,0x99,0xBC,0x7E,0x55,0x7C,0xE4,0xC1,0x5D,0x15,
        0xF6,0x45,0xBC,0x73,
    };

    static unsigned char dh512_g[] = {
        0x02,
    };

    DH *dh;

    if ((dh=DH_new()) == NULL) {
        return(NULL);
    }

    dh->p=BN_bin2bn(dh512_p,sizeof(dh512_p),NULL);
    dh->g=BN_bin2bn(dh512_g,sizeof(dh512_g),NULL);

    if ((dh->p == NULL) || (dh->g == NULL)) { 
        DH_free(dh); return(NULL); 
    }
    return dh;
}


static DH *get_dh1024()
{
    static unsigned char dh1024_p[] = {
        0xCD,0x02,0x2C,0x11,0x43,0xCD,0xAD,0xF5,0x54,0x5F,0xED,0xB1,
        0x28,0x56,0xDF,0x99,0xFA,0x80,0x2C,0x70,0xB5,0xC8,0xA8,0x12,
        0xC3,0xCD,0x38,0x0D,0x3B,0xE1,0xE3,0xA3,0xE4,0xE9,0xCB,0x58,
        0x78,0x7E,0xA6,0x80,0x7E,0xFC,0xC9,0x93,0x3A,0x86,0x1C,0x8E,
        0x0B,0xA2,0x1C,0xD0,0x09,0x99,0x29,0x9B,0xC1,0x53,0xB8,0xF3,
        0x98,0xA7,0xD8,0x46,0xBE,0x5B,0xB9,0x64,0x31,0xCF,0x02,0x63,
        0x0F,0x5D,0xF2,0xBE,0xEF,0xF6,0x55,0x8B,0xFB,0xF0,0xB8,0xF7,
        0xA5,0x2E,0xD2,0x6F,0x58,0x1E,0x46,0x3F,0x74,0x3C,0x02,0x41,
        0x2F,0x65,0x53,0x7F,0x1C,0x7B,0x8A,0x72,0x22,0x1D,0x2B,0xE9,
        0xA3,0x0F,0x50,0xC3,0x13,0x12,0x6C,0xD2,0x17,0xA9,0xA5,0x82,
        0xFC,0x91,0xE3,0x3E,0x28,0x8A,0x97,0x73,
    };

    static unsigned char dh1024_g[] = {
        0x02,
    };

    DH *dh;

    if ((dh=DH_new()) == NULL) return(NULL);
    dh->p=BN_bin2bn(dh1024_p,sizeof(dh1024_p),NULL);
    dh->g=BN_bin2bn(dh1024_g,sizeof(dh1024_g),NULL);
    if ((dh->p == NULL) || (dh->g == NULL)) {
        DH_free(dh); 
        return(NULL); 
    }
    return dh;
}

#else
void __mprOpenSslModuleDummy() {}
#endif /* BLD_FEATURE_OPENSSL */

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../ssl/mprOpenssl.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../ssl/mprSsl.c"
 */
/************************************************************************/

/**
 *  mprSsl.c -- Load and manage the SSL providers.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_SSL
/*
 *  Load the ssl provider
 */
static MprModule *loadSsl(MprCtx ctx, bool lazy)
{
    Mpr         *mpr;
    MprModule   *mp;

    mpr = mprGetMpr(ctx);

    if (mpr->flags & MPR_SSL_PROVIDER_LOADED) {
        return 0;
    }

    mprLog(ctx, MPR_CONFIG, "Activating the SSL provider");
#if BLD_FEATURE_OPENSSL
    /*
     *  NOTE: preference given to open ssl if both are enabled
     */
    mprLog(ctx, 2, "Loading OpenSSL module");
    if (mprCreateOpenSslModule(ctx, lazy) < 0) {
        return 0;
    }

#elif BLD_FEATURE_MATRIXSSL
    mprLog(ctx, 2, "Loading MatrixSSL module");
    if (mprCreateMatrixSslModule(ctx, lazy) < 0) {
        return 0;
    }
#endif
    if ((mp = mprCreateModule(ctx, "ssl", BLD_VERSION, (void*) NULL, NULL, NULL)) == 0) {
        return 0;
    }
    mpr->flags |= MPR_SSL_PROVIDER_LOADED;
    return mp;
}


MprModule *mprLoadSsl(MprCtx ctx, bool lazy)
{
    return loadSsl(ctx, lazy);
}


/*
 *  Loadable module interface. 
 */
MprModule *mprSslInit(MprCtx ctx, cchar *path)
{
    return loadSsl(ctx, 1);
}


/*
 *  Create a new Ssl context object
 */
MprSsl *mprCreateSsl(MprCtx ctx)
{
    MprSsl      *ssl;

    /*
     *  Create with a null destructor so the provider can install one if required
     */
    ssl =  mprAllocObjWithDestructorZeroed(ctx, MprSsl, NULL);
    if (ssl == 0) {
        return 0;
    }
    ssl->ciphers = mprStrdup(ssl, MPR_DEFAULT_CIPHER_SUITE);
    ssl->protocols = MPR_HTTP_PROTO_SSLV3 | MPR_HTTP_PROTO_TLSV1;
    ssl->verifyDepth = 6;
    return ssl;
}


void mprConfigureSsl(MprSsl *ssl)
{
    mprAssert(mprGetMpr(ssl)->socketService->secureProvider);
    mprGetMpr(ssl)->socketService->secureProvider->configureSsl(ssl);
}


void mprSetSslCiphers(MprSsl *ssl, cchar *ciphers)
{
    mprAssert(ssl);
    
    mprFree(ssl->ciphers);
    ssl->ciphers = mprStrdup(ssl, ciphers);
}


void mprSetSslKeyFile(MprSsl *ssl, cchar *keyFile)
{
    mprAssert(ssl);
    
    mprFree(ssl->keyFile);
    ssl->keyFile = mprStrdup(ssl, keyFile);
}


void mprSetSslCertFile(MprSsl *ssl, cchar *certFile)
{
    mprAssert(ssl);
    
    mprFree(ssl->certFile);
    ssl->certFile = mprStrdup(ssl, certFile);
}


void mprSetSslCaFile(MprSsl *ssl, cchar *caFile)
{
    mprAssert(ssl);
    
    mprFree(ssl->caFile);
    ssl->caFile = mprStrdup(ssl, caFile);
}


void mprSetSslCaPath(MprSsl *ssl, cchar *caPath)
{
    mprAssert(ssl);
    
    mprFree(ssl->caPath);
    ssl->caPath = mprStrdup(ssl, caPath);
}


void mprSetSslProtocols(MprSsl *ssl, int protocols)
{
    ssl->protocols = protocols;
}


void mprSetSocketSslConfig(MprSocket *sp, MprSsl *ssl)
{
    if (sp->sslSocket) {
        sp->sslSocket->ssl = ssl;
    }
}


void mprVerifySslClients(MprSsl *ssl, bool on)
{
    ssl->verifyClient = on;
}


#endif /* SSL */


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../ssl/mprSsl.c"
 */
/************************************************************************/

