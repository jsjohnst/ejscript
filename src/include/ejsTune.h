/*
 *  ejsTune.h - Tunable parameters for the C VM and compiler
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#ifndef _h_EJS_TUNE
#define _h_EJS_TUNE 1

/********************************* Includes ***********************************/

#include    "mpr.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************** Defines ***********************************/
/*
 *  Tunable Constants
 *  TODO - consistency of names is poor.
 */
#if BLD_TUNE == MPR_TUNE_SIZE || DOXYGEN
    /*
     *  Tune for size
     */
    #define EJS_GC_WORK_QUOTA       512             /**< Allocations required before garbage colllection */
    #define EJS_NUM_PROP            8               /**< Default object number of properties */
    #define EJS_NUM_GLOBAL          256             /**< Number of globals slots to pre-create */
    #define EJS_LOTSA_PROP          256             /**< Object with lots of properties. Grow by bigger chunks */
    #define EJS_E4X_BUF_MAX         (256 * 1024)    /* Max XML document size */
    #define EJS_MAX_RECURSION       10000           /* Maximum recursion */
    #define EJS_MAX_REGEX_MATCHES   32              /* Maximum regular sub-expressions */
    #define EJS_MAX_DB_MEM          (2*1024*1024)   /* Maximum regular sub-expressions */

    #define E4X_BUF_SIZE            512             /* Initial buffer size for tokens */
    #define E4X_BUF_MAX             (32 * 1024)     /* Max size for tokens */
    #define E4X_MAX_NODE_DEPTH      24              /* Max nesting of tags */

    #define EJS_WEB_TOK_INCR        1024
    #define EJS_WEB_MAX_HEADER      1024
    #define EJS_MAX_DEBUG_NAME      32
    #define EJS_MAX_TYPE            256             /**< Maximum number of types */
    #define EJS_NUM_CROSS_GEN       256             /* Number of cross generational GC root objects */

    #define EJS_CGI_MIN_BUF         (32 * 1024)     /* CGI output buffering */
    #define EJS_CGI_MAX_BUF         (128 * 1024)
    #define EJS_CGI_HDR_HASH        (31)

#elif BLD_TUNE == MPR_TUNE_BALANCED

    /*
     *  Tune balancing speed and size
     */
    #define EJS_GC_WORK_QUOTA       1024
    #define EJS_NUM_PROP            8
    #define EJS_NUM_GLOBAL          512
    #define EJS_LOTSA_PROP          256
    #define EJS_E4X_BUF_MAX         (1024 * 1024)
    #define EJS_MAX_RECURSION       (1000000)
    #define EJS_MAX_REGEX_MATCHES   64
    #define EJS_MAX_DB_MEM          (20*1024*1024)

    #define E4X_BUF_SIZE            4096
    #define E4X_BUF_MAX             (128 * 1024)
    #define E4X_MAX_NODE_DEPTH      128

    #define EJS_WEB_TOK_INCR        4096
    #define EJS_WEB_MAX_HEADER      4096
    #define EJS_MAX_DEBUG_NAME      64
    #define EJS_MAX_TYPE            512
    #define EJS_NUM_CROSS_GEN       1024 

    #define EJS_CGI_MIN_BUF         (64 * 1024)     /* CGI output buffering */
    #define EJS_CGI_MAX_BUF         (256 * 1024)
    #define EJS_CGI_HDR_HASH        (51)

#else
    /*
     *  Tune for speed
     */
    #define EJS_GC_WORK_QUOTA       2048
    #define EJS_NUM_PROP            8
    #define EJS_NUM_GLOBAL          1024
    #define EJS_LOTSA_PROP          1024
    #define EJS_E4X_BUF_MAX         (1024 * 1024)
    #define EJS_MAX_RECURSION       (1000000)
    #define EJS_MAX_REGEX_MATCHES   128
    #define EJS_MAX_DB_MEM          (20*1024*1024)

    #define E4X_BUF_SIZE            4096
    #define E4X_BUF_MAX             (128 * 1024)
    #define E4X_MAX_NODE_DEPTH      128

    #define EJS_WEB_TOK_INCR        4096
    #define EJS_WEB_MAX_HEADER      4096
    #define EJS_MAX_DEBUG_NAME      96
    #define EJS_MAX_TYPE            1024
    #define EJS_NUM_CROSS_GEN       4096 

    #define EJS_CGI_MIN_BUF         (128 * 1024)     /* CGI output buffering */
    #define EJS_CGI_MAX_BUF         (512 * 1024)
    #define EJS_CGI_HDR_HASH        (101)
#endif

#define EJS_SESSION_TIMEOUT         1800
#define EJS_TIMER_PERIOD            1000            /* Timer checks ever 1 second */

/*
 *  Object Property hash constants
 */
#define EJS_HASH_MIN_PROP           8               /**< Min props to hash */

#if BLD_FEATURE_MMU
    #if BLD_TUNE == MPR_TUNE_SIZE
        #define EJS_STACK_MAX       (1024 * 1024)   /* Stack size on virtual memory systems */
    #elif BLD_TUNE == MPR_TUNE_BALANCED
        #define EJS_STACK_MAX       (1024 * 1024 * 4)
    #else
        #define EJS_STACK_MAX       (1024 * 1024 * 16)
    #endif
#else
    /*
     *  Highly recursive workloads may need to increase the stack values
     */
    #if BLD_TUNE == MPR_TUNE_SIZE
        #define EJS_STACK_MAX       (1024 * 32)     /* Stack size without MMU */
    #elif BLD_TUNE == MPR_TUNE_BALANCED
        #define EJS_STACK_MAX       (1024 * 64)
    #else
        #define EJS_STACK_MAX       (1024 * 128)
    #endif
#endif

#if UNUSED && KEEP
/*
 *  Number of objects instances to create per type when allocating
 */
#define EJS_NUM_BOOLEAN         2
#define EJS_NUM_CMETHOD         1
#define EJS_NUM_DATE            1
#define EJS_NUM_DOUBLE          1
#define EJS_NUM_GLOBAL          1
#define EJS_NUM_INTEGER         32
#define EJS_NUM_LONG            32
#define EJS_NUM_NAMESPACE       8
#define EJS_NUM_NULL            1
#define EJS_NUM_NUMBER          1
#define EJS_NUM_OBJECT          64
#define EJS_NUM_METHOD          64
#define EJS_NUM_STRING          64
#define EJS_NUM_POINTER         1
#define EJS_NUM_TYPE            48
#define EJS_NUM_VOID            1
#endif

/*
 *  Sanity constants. Only for sanity checking. Set large enough to never be a
 *  real limit but low enough to catch some errors in development.
 */
#define EJS_MAX_PROP            (8192)          /* Max properties per class */
#define EJS_MAX_POOL            (4*1024*1024)   /* Size of constant pool */
#define EJS_MAX_ARGS            (1024)          /* Max number of args */
#define EJS_MAX_LOCALS          (10*1024)       /* Max number of locals */
#define EJS_MAX_EXCEPTIONS      (1024)          /* Max number of exceptions */
#define EJS_MAX_TRAITS          (0x7fff)        /* Max number of declared properties per block */

/*
 *  Should not need to change these
 */
#define EJS_INC_ARGS            8               /* Frame stack increment */


#define EJS_MAX_BASE_CLASSES    256             /* Max inheritance chain */
#define EJS_DOC_HASH_SIZE       10007           /* Size of doc hash table */


/*
 *  Compiler constants
 */
#define EC_MAX_INCLUDE          32              /* Max number of nested includes */
#define EC_LINE_INCR            1024            /* Growth increment for input lines */
#define EC_TOKEN_INCR           256             /* Growth increment for tokens */
#define EC_MAX_LOOK_AHEAD       8
#define EC_BUFSIZE              4096            /* General buffer size */
#define EC_MAX_ERRORS           25              /* Max compilation errors before giving up */

//  TODO - resize smaller
#define EC_CODE_BUFSIZE         (64*1024)       /* Initial size of code gen buffer */
#define EC_NUM_PAK_PROP         32              /* Initial number of properties */

/*
 *  File extensions
 */
#define EJS_MODULE_EXT          ".mod"
#define EJS_SOURCE_EXT          ".es"
#define EJS_LISTING_EXT         ".lst"

#ifdef __cplusplus
}
#endif
#endif /* _h_EJS_TUNE */

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
