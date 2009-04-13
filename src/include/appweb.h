#include "appwebConfig.h"
/*
 *  appweb.h -- Primary header for the Embedthis Appweb Library
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_APPWEB_h
#define _h_APPWEB_h 1

#include    "mpr.h"

#endif /* _h_APPWEB_h */


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
/**
 *  appwebMonitor.h - Monitor Header
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************* Includes ***********************************/

#ifndef _h_APPWEB_MONITOR
#define _h_APPWEB_MONITOR 1

#define APPWEB_MONITOR_MESSAGE      (WM_USER + 30)
#define APPWEB_QUERY_PORT_MESSAGE   (WM_USER + 31)
#define APPWEB_SOCKET_MESSAGE       (WM_USER + 32)

#endif /* _h_APPWEB_MONITOR  */

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
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

/**
 *  httpTune.h - Tunable parameters for the Embedthis Http Web Server
 *
 *  See httpServer.dox for additional documentation.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Copyright **********************************/

/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
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
 *  details at: http: *www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http: *www.embedthis.com
 *
 *  @end
 */

/********************************* Includes ***********************************/

#ifndef _h_HTTP_TUNE
#define _h_HTTP_TUNE 1

/********************************** Defines ***********************************/

#define MA_SERVER_NAME              "Embedthis-Appweb/" BLD_VERSION
#define MA_SERVER_DEFAULT_PORT_NUM  80

/*
 *  These constants can be overridden in http.conf
 */
#define HEAP_OVERHEAD               (MPR_ALLOC_HDR_SIZE + \
                                        MPR_ALLOC_ALIGN(sizeof(MprRegion) + sizeof(MprHeap) + sizeof(MprDestructor)))
#if BLD_TUNE == MPR_TUNE_SIZE || DOXYGEN
    /*
     *  Tune for size
     */
    #define MA_REQ_MEM              ((1 * 1024 * 1024) - HEAP_OVERHEAD) /**< Default per-request memory arena size */

    #define MA_MAX_BODY             (64 * 1024)         /* Maximum incoming request content body size */
    #define MA_MAX_CHUNK_SIZE       (8 * 1024)          /* Max buffer for any stage */
    #define MA_MAX_HEADERS          2048                /* Max size of the headers */
    #define MA_MAX_NUM_HEADERS      20                  /* Max number of header lines */
    #define MA_MAX_RESPONSE_BODY    (128 * 1024 * 1024) /* Max buffer for generated data */
    #define MA_MAX_STAGE_BUFFER     (8 * 1024)          /* Max buffer for any stage */
    #define MA_MAX_UPLOAD_SIZE      (10 * 1024 * 1024)  /* Max size of uploaded document */
    #define MA_MAX_URL              512                 /**< Max URL size. Also request URL size. */

    #define MA_BUFSIZE              (4 * 1024)          /**< Default I/O buffer size */
    #define MA_MAX_PASS             64                  /**< Size of password */
    #define MA_MAX_SECRET           32                  /**< Number of random bytes to use */
    #define MA_MAX_IOVEC            16                  /**< Number of fragments in a single socket write */

#elif BLD_TUNE == MPR_TUNE_BALANCED
    /*
     *  Tune balancing speed and size
     */
    #define MA_CONN_MEM             ((2 * 4096) - HEAP_OVERHEAD)
    #define MA_REQ_MEM              ((2 * 1024 * 1024) - HEAP_OVERHEAD)

    #define MA_MAX_BODY             (1024 * 1024)
    #define MA_MAX_CHUNK_SIZE       (8 * 1024)
    #define MA_MAX_HEADERS          (8 * 1024)
    #define MA_MAX_NUM_HEADERS      40
    #define MA_MAX_RESPONSE_BODY    (256 * 1024 * 1024)
    #define MA_MAX_STAGE_BUFFER     (32 * 1024)
    #define MA_MAX_UPLOAD_SIZE      0x7fffffff
    #define MA_MAX_URL              (4096)

    #define MA_BUFSIZE              (4 * 1024)
    #define MA_MAX_PASS             128
    #define MA_MAX_SECRET           32
    #define MA_MAX_IOVEC            24
#else
    /*
     *  Tune for speed
     */
    #define MA_CONN_MEM             ((2 * 4096) - HEAP_OVERHEAD)
    #define MA_REQ_MEM              ((4 * 1024 * 1024) - HEAP_OVERHEAD)

    #define MA_MAX_BODY             (1024 * 1024)
    #define MA_MAX_CHUNK_SIZE       (8 * 1024) 
    #define MA_MAX_HEADERS          (8 * 1024)
    #define MA_MAX_NUM_HEADERS      256
    #define MA_MAX_RESPONSE_BODY    0x7fffffff
    #define MA_MAX_RESPONSE_BODY    (128 * 1024)
    #define MA_MAX_STAGE_BUFFER     (64 * 1024)
    #define MA_MAX_UPLOAD_SIZE      0x7fffffff
    #define MA_MAX_URL              (4096)

    #define MA_BUFSIZE              (8 * 1024)
    #define MA_MAX_PASS             128
    #define MA_MAX_SECRET           32
    #define MA_MAX_IOVEC            32
#endif


#define MA_DEFAULT_MAX_THREADS  10              /**< Default number of threads */
#define MA_KEEP_TIMEOUT         60000           /**< Keep connection alive timeout */
#define MA_CGI_TIMEOUT          4000            /**< Time to wait to reap exit status */
#define MA_MAX_KEEP_ALIVE       100             /**< Default requests per TCP conn */
#define MA_TIMER_PERIOD         1000            /**< Timer checks ever 1 second */
#define MA_CGI_PERIOD           20              /**< CGI poll period (only for windows) */
#define MA_MAX_ACCESS_LOG       (20971520)      /**< Access file size (20 MB) */
#define MA_SERVER_TIMEOUT       (300 * 1000)
#define MA_MAX_CONFIG_DEPTH     (16)            /* Max nest of directives in config file */
#define MA_RANGE_BUFSIZE        (128)           /* Size of a range boundary */
#define MA_MAX_REWRITE          (10)            /* Maximum recursive URI rewrites */

/*
 *  Hash sizes (primes work best)
 */
#define MA_COOKIE_HASH_SIZE     11              /* Size of cookie hash */
#define MA_EGI_HASH_SIZE        31              /* Size of EGI hash */
#define MA_ERROR_HASH_SIZE      11              /* Size of error document hash */
#define MA_HEADER_HASH_SIZE     31              /* Size of header hash */
#define MA_MIME_HASH_SIZE       53              /* Mime type hash */
#define MA_VAR_HASH_SIZE        31              /* Size of query var hash */
#define MA_HANDLER_HASH_SIZE    17              /* Size of handler hash */
#define MA_ACTION_HASH_SIZE     13              /* Size of action program hash */

/*
 *  These constants are to sanity check user input in the http.conf
 */
#define MA_TOP_THREADS          100

#define MA_BOT_BODY             512
#define MA_TOP_BODY             (0x7fffffff)        /* 2 GB */

#define MA_BOT_CHUNK_SIZE       512
#define MA_TOP_CHUNK_SIZE       (4 * 1024 * 1024)   /* 4 MB */

#define MA_BOT_NUM_HEADERS      8
#define MA_TOP_NUM_HEADERS      (4 * 1024)

#define MA_BOT_HEADER           512
#define MA_TOP_HEADER           (1024 * 1024)

#define MA_BOT_URL              64
#define MA_TOP_URL              (255 * 1024)        /* 256 MB */

#define MA_BOT_RESPONSE_BODY    512
#define MA_TOP_RESPONSE_BODY    0x7fffffff          /* 2 GB */

#define MA_BOT_STACK            (16 * 1024)
#define MA_TOP_STACK            (4 * 1024 * 1024)

#define MA_BOT_STAGE_BUFFER     (2 * 1024)
#define MA_TOP_STAGE_BUFFER     (1 * 1024 * 1024)   /* 1 MB */

#define MA_BOT_UPLOAD_SIZE      1
#define MA_TOP_UPLOAD_SIZE      0x7fffffff          /* 2 GB */

#define MA_MAX_USER             MPR_HTTP_MAX_USER

#endif /* _h_HTTP_TUNE */


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
/*
 *  http.h -- Primary header for the Embedthis Appweb HTTP Web Server
 */

/********************************* Copyright **********************************/
/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
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
 *  details at: http: *www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http: *www.embedthis.com
 *
 *  @end
 */

#ifndef _h_HTTP_SERVER
#define _h_HTTP_SERVER 1

/********************************* Includes ***********************************/

#include    "mpr.h"

/****************************** Forward Declarations **************************/

#ifdef __cplusplus
extern "C" {
#endif

#if !DOXYGEN
struct MaConn;
struct MaPacket;
struct MaRequest;
struct MaResponse;
struct MaQueue;
struct MaServer;
struct MaStage;
struct MaSsl;
#endif

/********************************** Defaults **********************************/

#ifndef BLD_FEATURE_CONFIG_PARSE
#define BLD_FEATURE_CONFIG_PARSE 1
#endif

/********************************** Defines ***********************************/
/**
 *  Server limits
 *  @stability Evolving
 *  @defgroup MaLimits MaLimits
 *  @see MaLimits
 */
typedef struct MaLimits {
    int             maxBody;                /**< Max size of an incoming request */
    int             maxChunkSize;           /**< Max chunk size for transfer encoding */
    int             maxHeader;              /**< Max size of the total header */
    int             maxNumHeaders;          /**< Max number of lines of header */
    int             maxResponseBody;        /**< Max size of generated response content */
    int             maxStageBuffer;         /**< Max buffering by any pipeline stage */
    int             maxThreads;             /**< Max number of pool threads */
    int             minThreads;             /**< Min number of pool threads */
    int             maxUploadSize;          /**< Max size of an uploaded file */
    int             maxUrl;                 /**< Max size of a URL */
    int             threadStackSize;        /**< Stack size for each pool thread */
} MaLimits;


/**
 *  Http Service
 *  @description There is one instance of MaHttp per application. It manages a list of HTTP servers running in
 *      the application.
 *  @stability Evolving
 *  @defgroup MaHttp MaHttp
 *  @see MaHttp maCreateHttp maStartHttp maStopHttp
 */
typedef struct MaHttp {
    MprHashTable    *stages;                /**< Hash table of stages */
    struct MaServer *defaultServer;         /**< Default web server object */
    MprList         *servers;               /**< List of web servers objects */
    MaLimits        limits;                 /**< Security and resource limits */

    /*
     *  Some standard pipeline stages
     */
    struct MaStage  *netConnector;          /**< Network connector */
    struct MaStage  *sendConnector;         /**< Send file connector */
    struct MaStage  *authFilter;            /**< Authorization filter (digest and basic) */
    struct MaStage  *rangeFilter;           /**< Ranged requests filter */
    struct MaStage  *chunkFilter;           /**< Chunked transfer encoding filter */
    struct MaStage  *dirHandler;            /**< Directory listing handler */
    struct MaStage  *egiHandler;            /**< Embedded Gateway Interface (EGI) handler */
    struct MaStage  *ejsHandler;            /**< Ejscript Web Framework handler */
    struct MaStage  *fileHandler;           /**< Static file handler */
    struct MaStage  *passHandler;           /**< Pass through handler */

    char            *username;              /**< Http server user name */
    char            *groupname;             /**< Http server group name */
    int             uid;                    /**< User Id */
    int             gid;                    /**< Group Id */

#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;                 /**< Multi-thread sync */
#endif
} MaHttp;


/**
 *  Create the MaHttp object.
 *  @description The Appweb library uses a singleton MaHttp object to manage multiple embedded web servers
 *  @param ctx Any memory context object returned by mprAlloc
 *  @return A MaHttp object. Use mprFree to close and release.
 *  @ingroup MaHttp
 */
extern MaHttp *maCreateHttp(MprCtx ctx);

/**
 *  Start Http services
 *  @description This starts listening for requests on all configured servers.
 *  @param http MaHttp object created via #maCreateHttp
 *  @return Zero if successful, otherwise a negative Mpr error code. See the Appweb log for diagnostics.
 *  @ingroup MaHttp
 */
extern int maStartHttp(MaHttp *http);

/**
 *  Stop Http services
 *  @description This stops listening for requests on all configured servers. Shutdown is somewhat graceful.
 *  @param http MaHttp object created via #maCreateHttp
 *  @return Zero if successful, otherwise a negative Mpr error code. See the Appweb log for diagnostics.
 *  @ingroup MaHttp
 */
extern int maStopHttp(MaHttp *http);

/**
 *  Set the Http User
 *  @description Define the user name under which to run the Appweb service
 *  @param http MaHttp object created via #maCreateHttp
 *  @param user User name. Must be defined in the system password file.
 *  @return Zero if successful, otherwise a negative Mpr error code. See the Appweb log for diagnostics.
 *  @ingroup MaHttp
 */
extern int maSetHttpUser(MaHttp *http, cchar *user);

/**
 *  Set the Http Group
 *  @description Define the group name under which to run the Appweb service
 *  @param http MaHttp object created via #maCreateHttp
 *  @param group Group name. Must be defined in the system group file.
 *  @return Zero if successful, otherwise a negative Mpr error code. See the Appweb log for diagnostics.
 *  @ingroup MaHttp
 */
extern int maSetHttpGroup(MaHttp *http, cchar *group);

extern void         maAddServer(MaHttp *http, struct MaServer *server);
extern int          maApplyChangedGroup(MaHttp *http);
extern int          maApplyChangedUser(MaHttp *http);
extern struct MaServer *maLookupServer(MaHttp *http, cchar *name);
extern int          maLoadModule(MaHttp *http, cchar *name, cchar *libname);
extern void         maSetDefaultServer(MaHttp *http, struct MaServer *server);

/*
 *  Loadable module entry points
 */
extern MprModule *maAuthFilterInit(MaHttp *http, cchar *path);
extern MprModule *maCgiHandlerInit(MaHttp *http, cchar *path);
extern MprModule *maChunkFilterInit(MaHttp *http, cchar *path);
extern MprModule *maDirHandlerInit(MaHttp *http, cchar *path);
extern MprModule *maEgiHandlerInit(MaHttp *http, cchar *path);
extern MprModule *maEjsHandlerInit(MaHttp *http, cchar *path);
extern MprModule *maFileHandlerInit(MaHttp *http, cchar *path);
extern MprModule *maNetConnectorInit(MaHttp *http, cchar *path);
extern MprModule *maPhpHandlerInit(MaHttp *http, cchar *path);
extern MprModule *maRangeFilterInit(MaHttp *http, cchar *path);
extern MprModule *maSslModuleInit(MaHttp *http, cchar *path);
extern MprModule *maUploadHandlerInit(MaHttp *http, cchar *path);

/********************************* MaListen ***********************************/

#define MA_LISTEN_DEFAULT_PORT  0x1         /* Use default port 80 */
#define MA_LISTEN_WILD_PORT     0x2         /* Port spec missing */
#define MA_LISTEN_WILD_IP       0x4         /* IP spec missing (first endpoint) */
#define MA_LISTEN_WILD_IP2      0x8         /* IP spec missing (second+ endpoint) */

/**
 *  Listen endpoint
 *  @stability Evolving
 *  @defgroup MaListen MaListen
 *  @see MaListen
 */
typedef struct  MaListen {
    struct MaServer *server;                /**< Server owning this listening endpoint */
    char            *ipAddr;                /**< IP address on which to listen */
    int             port;                   /**< Port number to listen on */
    int             flags;                  /**< Listen flags */
#if UNUSED
    bool            secure;
#endif
    MprSocket       *sock;                  /**< Underlying socket */
#if BLD_FEATURE_SSL
    struct MprSsl   *ssl;                   /**< SSL configuration */
#endif
} MaListen;


extern MaListen *maCreateListen(struct MaServer *server, cchar *ipAddr, int port, int flags);
extern int maStartListening(MaListen *listen);
extern int maStopListening(MaListen *listen);

/******************************** MaHostAddress *******************************/
/*
 *  Flags
 */
#define MA_IPADDR_VHOST 0x1

/**
 *  Host Address Mapping
 *  @stability Evolving
 *  @defgroup MaHostAddress MaHostAddress
 *  @see MaHostAddress
 */
typedef struct MaHostAddress {
    char            *ipAddr;                /**< IP Address for this endpoint */
    int             port;                   /**< Port for this endpoint */
    int             flags;                  /**< Mapping flags */
    MprList         *vhosts;                /**< Vhosts using this address */
} MaHostAddress;


extern MaHostAddress *maCreateHostAddress(MprCtx ctx, cchar *ipAddr, int port);
extern MaHostAddress *maLookupHostAddress(struct MaServer *server, cchar *ipAddr, int port);
extern struct MaHost *maLookupVirtualHost(MaHostAddress *hostAddress, cchar *hostStr);
extern void maInsertVirtualHost(MaHostAddress *hostAddress, struct MaHost *vhost);
extern bool maIsNamedVirtualHostAddress(MaHostAddress *hostAddress);
extern void maSetNamedVirtualHostAddress(MaHostAddress *hostAddress);

/********************************** MaServer **********************************/
/**
 *  Http Server Control
 *  An application may have any number of HTTP servers, each managed by an instance of the Server class. Typically
 *  there will be only one server in an application. There may be multiple virtual hosts and one default host for
 *  each server class. A server will typically be configured by calling the configure method for each server which
 *  parses a file to define the server and virtual host configuration.
 *  @stability Evolving
 *  @defgroup MaServer MaServer
 *  @see MaServer maCreateWebServer maServiceWebServer maRunWebServer maRunSimpleWebServer maCreateServer 
 *      maConfigureServer maLoadStaticModules maUnloadStaticModules maSplitConfigValue
 */
typedef struct MaServer {
    MaHttp          *http;
    struct MaHost   *defaultHost;           /**< Primary host */
    MprList         *hosts;                 /**< List of host objects */
    MprList         *hostAddresses;         /**< List of HostAddress objects */
    MprList         *listens;               /**< List of listening sockets */
    int             maxConcurrentRequests;  /**< Maximum number of clients */
    char            *name;                  /**< Unique name for this server */
    char            *serverRoot;            /**< Server root */
    bool            alreadyLogging;         /**< Already logging */
} MaServer;

/**
 *  Create a web server
 *  @description Create a web server configuration based on the supplied config file. Once created, the
 *      web server should be run by calling #maServiceWebServer. Use this routine when you need access to the MaHttp
 *      object. If you want a one-line embedding of Appweb, use #maRunWebServer or #maRunSimpleWebServer.
 *  @param configFile File name of the Appweb configuration file (appweb.conf) that defines the web server configuration.
 *  @return MaHttp object.
 *  @ingroup MaServer
 */
extern MaHttp *maCreateWebServer(cchar *configFile);

/**
 *  Service a web server
 *  @description Run a web server configuration. This is will start http services via #maStartHttp and will service
 *      incoming Http requests until instructed to exit. This is often used in conjunction with #maCreateWebServer.
 *  @param http Http object created via #maCreateWebServer or #maCreateHttp.
 *  @return Zero if successful, otherwise a negative Mpr error code. See the Appweb log for diagnostics.
 *  @ingroup MaServer
 */
extern int maServiceWebServer(MaHttp *http);

/**
 *  Create and run a web server based on a configuration file
 *  @description Create a web server configuration based on the supplied config file. This routine provides 
 *      is a one-line embedding of Appweb. If you don't want to use a config file, try the #maRunSimpleWebServer 
 *      instead. If you need more control, try #maCreateWebServer which exposes the MaHttp object.
 *  @param configFile File name of the Appweb configuration file (appweb.conf) that defines the web server configuration.
 *  @return Zero if successful, otherwise a negative Mpr error code. See the Appweb log for diagnostics.
 *  @ingroup MaServer
 */
extern int maRunWebServer(cchar *configFile);

/**
 *  Create and run a simple web server listening on a single IP address.
 *  @description Create a simple web server without using a configuration file. The server is created to listen on
 *      the specified IP addresss and port. This routine provides is a one-line embedding of Appweb. If you want to 
 *      use a config file, try the #maRunWebServer instead. If you need more control, try #maCreateWebServer which 
 *      exposes the MaHttp object.
 *  @param ipAddress IP address on which to listen. Set to "0.0.0.0" to listen on all interfaces.
 *  @param port Port number to listen to
 *  @param docRoot Directory containing the documents to serve.
 *  @return Zero if successful, otherwise a negative Mpr error code. See the Appweb log for diagnostics.
 *  @ingroup MaServer
 */
extern int maRunSimpleWebServer(cchar *ipAddress, int port, cchar *docRoot);

/**
 *  Create a MaServer object
 *  @description Create new MaServer object. This routine creates a bare MaServer object, loads any required static
 *      modules  and performs minimal configuration. To use the server object created, more configuration will be 
 *      required before starting Http services.
 *      If you want a one-line embedding of Appweb, use #maRunWebServer or #maRunSimpleWebServer.
 *  @param http Http object returned from #maCreateHttp
 *  @param name Name of the web server. This name is used as the initial server name.
 *  @param root Server root directory
 *  @param ipAddr If not-null, create and open a listening endpoint on this IP address. If you are configuring via a
 *      config file, use #maConfigureServer and set ipAddr to null.
 *  @param port Port number to listen on. Set to -1 if you do not want to open a listening endpoint on ipAddr:port
 *  @return MaServer A newly created MaServer object. Use mprFree to free and release.
 *  @ingroup MaServer
 */
extern MaServer *maCreateServer(MaHttp *http, cchar *name, cchar *root, cchar *ipAddr, int port);

//  TODO - this seems to be missing the server root directory for when using IP:port. It has a document root.
//  TODO - Seems better to split this into 2 APIs. One for a config file and one for manual configuration.
/**
 *  Configure a web server.
 *  @description This will configure a web server based on either a configuration file or using the supplied
 *      IP address and port. 
 *  @param ctx Any memory context object returned by mprAlloc
 *  @param http MaHttp object created via #maCreateHttp
 *  @param server MaServer object created via #maCreateServer
 *  @param configFile File name of the Appweb configuration file (appweb.conf) that defines the web server configuration.
 *  @param ipAddr If using a config file, set to null. Otherwise, set to a host name or IP address.
 *  @param port If using a config file, set to -1. Otherwise, set to the port number to listen on.
 *  @param documentRoot If not using a config file, set this to the directory containing the web documents to serve.
 *  @return Zero if successful, otherwise a negative Mpr error code. See the Appweb log for diagnostics.
 *  @ingroup MaServer
 */
extern int maConfigureServer(MprCtx ctx, MaHttp *http, MaServer *server, cchar *configFile, cchar *ipAddr, int port,
    cchar *documentRoot);

/**
 *  Load static modules
 *  @description Load the statically configured modules, handlers, filters and connectors. The configure program
 *      can specify a static or shared build of Appweb. The #maCreateServer routine will call maLoadStaticModules
 *      automatically. It should not be called by in user programs.
 *  @param http MaHttp object created via #maCreateHttp
 *  @ingroup MaServer
 */
extern void maLoadStaticModules(MaHttp *http);

extern void     maAddHost(MaServer *server, struct MaHost *host);
extern void     maAddListen(MaServer *server, MaListen *listen);
extern int      maCreateHostAddresses(MaServer *server, struct MaHost *host, cchar *value);
extern struct MaHost *maLookupHost(MaServer *server, cchar *name);
extern int      maGetConfigValue(MprCtx ctx, char **arg, char *buf, char **nextToken, int quotes);
extern void     maSetDefaultHost(MaServer *server, struct MaHost *host);
extern void     maSetDefaultIndex(MaServer *server, cchar *path, cchar *fileName);
extern void     maSetServerRoot(MaServer *server, cchar *path);
extern int      maSplitConfigValue(MprCtx ctx, char **s1, char **s2, char *buf, int quotes);
extern int      maStartServer(MaServer *server);
extern int      maStopServer(MaServer *server);
extern int      maParseConfig(MaServer *server, cchar *configFile);
extern void     maUnloadStaticModules(MaHttp *http);

#if UNUSED
extern int      maSetSslListeners(struct MaHost *host, MaSslConfig *config);
#endif

/************************************* MaAuth *********************************/

typedef long MaAcl;                         /* Access control mask */

/*
 *  Deny/Allow order. TODO - this is not yet implemented.
 */
#define MA_ALLOW_DENY           1
#define MA_DENY_ALLOW           2

/*
 *  Authentication types
 */
#define MA_AUTH_UNKNOWN         0
#define MA_AUTH_BASIC           1           /* Basic HTTP authentication (clear text) */
#define MA_AUTH_DIGEST          2           /* Digest authentication */

//  TODO - remove - change to user, role, capability scheme
#define MA_ACL_ALL  (-1)                    /* All bits set */

/*
 *  Auth Flags
 */
#define MA_AUTH_REQUIRED        0x1         /* Dir/Location requires auth */

/*
 *  Authentication methods
 */
#define MA_AUTH_METHOD_FILE     1           /* Appweb httpPassword file based authentication */
#define MA_AUTH_METHOD_PAM      2           /* Plugable authentication module scheme (Unix) */

/**
 *  Authorization
 *  The MaAuth struct  is the foundation authorization object and is used as base class by MaDirectory and MaLocation.
 *  It stores the authorization configuration information required to determine if a client request should be
 *  permitted to the resource controlled by this object.
 *  @stability Evolving
 *  @defgroup MaAuth MaAuth
 *  @see MaAuth
 */
typedef struct MaAuth {
    char            *allow;
    bool            anyValidUser;
    int             type;
    char            *deny;
    int             method;                 /* Authorization method (PAM | FILE) */
    int             flags;
    int             order;
    char            *qop;

    char            *requiredRealm;
    char            *requiredGroups;
    char            *requiredUsers;

    //  TODO - should convert to use, role, capability and should be a
    MaAcl           requiredAcl;

#if BLD_FEATURE_AUTH_FILE
    /*
     *  State for file based authorization
     */
    char            *userFile;
    char            *groupFile;
    MprHashTable    *users;
    MprHashTable    *groups;
#endif
} MaAuth;

/*
 *  TODO - Create an interface for dynamic loading of backend methods
 */
extern void     maSetAuthAllow(MaAuth *auth, cchar *allow);
extern void     maSetAuthAnyValidUser(MaAuth *auth);
extern void     maSetAuthOrder(MaAuth *auth, int o);
extern void     maSetAuthQop(MaAuth *auth, cchar *qop);
extern void     maSetAuthType(MaAuth *auth, int type);
extern void     maSetAuthMethod(MaAuth *auth, int method);
extern void     maSetAuthDeny(MaAuth *auth, cchar *deny);
extern void     maSetAuthRealm(MaAuth *auth, cchar *realm);
extern void     maSetAuthRequiredGroups(MaAuth *auth, cchar *groups);
extern void     maSetAuthRequiredUsers(MaAuth *auth, cchar *users);


#if BLD_FEATURE_AUTH_FILE
/**
 *  User Authorization
 *  File based authorization backend
 *  @stability Evolving
 *  @defgroup MaUser
 *  @see MaUser
 */
typedef struct MaUser {
    bool            enabled;
    MaAcl           acl;                    /* Union (or) of all group Acls */
    char            *password;
    char            *realm;
    char            *name;
} MaUser;


/**
 *  Group Authorization
 *  @stability Evolving
 *  @defgroup MaGroup
 *  @see MaGroup
 */
typedef struct  MaGroup {
    MaAcl           acl;
    bool            enabled;
    char            *name;
    MprList         *users;                 /* List of users */
} MaGroup;

//DDD this about what of these should be documented
//  TODO - simplify this APP. Too elaborate?
//  TODO -- all these routines should be generic (not native) and use some switch table to vector to the right backend method
extern int      maAddGroup(MaAuth *auth, char *group, MaAcl acl, bool enabled);
extern int      maAddUser(MaAuth *auth, cchar *realm, cchar *user, cchar *password, bool enabled);
extern int      maAddUserToGroup(MaAuth *auth, MaGroup *gp, cchar *user);
extern int      maAddUsersToGroup(MaAuth *auth, cchar *group, cchar *users);
extern MaAuth   *maCreateAuth(MprCtx ctx, MaAuth *parent);
extern MaGroup  *maCreateGroup(MaAuth *auth, cchar *name, MaAcl acl, bool enabled);
extern MaUser   *maCreateUser(MaAuth *auth, cchar *realm, cchar *name, cchar *password, bool enabled);
extern int      maDisableGroup(MaAuth *auth, cchar *group);
extern int      maDisableUser(MaAuth *auth, cchar *realm, cchar *user);
extern int      maEnableGroup(MaAuth *auth, cchar *group);
extern int      maEnableUser(MaAuth *auth, cchar *realm, cchar *user);
extern MaAcl    maGetGroupAcl(MaAuth *auth, char *group);
extern cchar    *maGetNativePassword(struct MaConn *conn, cchar *realm, cchar *user);
extern bool     maIsGroupEnabled(MaAuth *auth, cchar *group);
extern bool     maIsUserEnabled(MaAuth *auth, cchar *realm, cchar *user);
extern MaAcl    maParseAcl(MaAuth *auth, cchar *aclStr);
extern int      maRemoveGroup(MaAuth *auth, cchar *group);
extern int      maReadGroupFile(MaServer *server, MaAuth *auth, char *path);
extern int      maReadUserFile(MaServer *server, MaAuth *auth, char *path);
extern int      maRemoveUser(MaAuth *auth, cchar *realm, cchar *user);
extern int      maRemoveUserFromGroup(MaGroup *gp, cchar *user);
extern int      maRemoveUsersFromGroup(MaAuth *auth, cchar *group, cchar *users);
extern int      maSetGroupAcl(MaAuth *auth, cchar *group, MaAcl acl);
extern void     maSetRequiredAcl(MaAuth *auth, MaAcl acl);
extern void     maUpdateUserAcls(MaAuth *auth);
extern int      maWriteUserFile(MaServer *server, MaAuth *auth, char *path);
extern int      maWriteGroupFile(MaServer *server, MaAuth *auth, char *path);
extern bool     maValidateNativeCredentials(struct MaConn *conn, cchar *realm, cchar *user, cchar *password, 
                    cchar *requiredPass, char **msg);
#endif

#if BLD_FEATURE_AUTH_PAM
extern cchar    *maGetPamPassword(struct MaConn *conn, cchar *realm, cchar *user);
extern bool     maValidatePamCredentials(struct MaConn *conn, cchar *realm, cchar *user, cchar *password, 
                    cchar *requiredPass, char **msg);
#endif

/************************************ MaDir ***********************************/
/**
 *  Directory Control
 *  @stability Evolving
 *  @defgroup MaDir MaDir
 *  @see MaDir
 */
typedef struct  MaDir {
    MaAuth          *auth;                  /**< Authorization control */
    //TODO - Hosts don't own directories. They are outside hosts
    struct MaHost   *host;                  /**< Host owning this directory */
    char            *indexName;             /**< Default index document name */
    char            *path;                  /**< Directory filename */
    int             pathLen;                /**< Length of the directory path */
} MaDir;

extern MaDir    *maCreateBareDir(struct MaHost *host, cchar *path);
extern MaDir    *maCreateDir(struct MaHost *host, cchar *path, MaDir *parent);
extern void     maSetDirIndex(MaDir *dir, cchar *name) ;
extern void     maSetDirPath(MaDir *dir, cchar *fileName);

/********************************** MaUploadFile *********************************/
/**
 *  Upload File. TODO not yet implemented.
 *  Each uploaded file has an MaUploadedFile entry. This is managed by the upload handler.
 *  @stability Evolving
 *  @defgroup MaUploadFile MaUploadFile
 *  @see MaUploadFile
 */
typedef struct MaUploadFile {
    cchar           *name;                  /* TODO */
    cchar           *filename;              /* Local (temp) name of the file */
    cchar           *clientFilename;        /* Client side name of the file */
    cchar           *contentType;           /* Content type */
    int             size;                   /* Uploaded file size */
} MaUploadFile;

typedef void (*MaUploadCallback)(struct MaConn *conn, void *data, MaUploadFile *file);

extern void maSetUploadCallback(MaHttp *http, MaUploadCallback userCallback, void *data);

/********************************** MaLocation ********************************/
/*
 *  Flags
 */
#if UNUSED
#define MA_LOC_PATH_INFO        0x1         /**< Do path info processing */
#endif
#define MA_LOC_APP              0x2         /**< Location defines an application */
#define MA_LOC_APP_DIR          0x4         /**< Location defines a directory of applications */
#define MA_LOC_AUTO_SESSION     0x8         /**< Auto create sessions in this location */
#define MA_LOC_BROWSER          0x10        /**< Send errors back to the browser for this location */

/**
 *  Location Control
 *  @stability Evolving
 *  @defgroup MaLocation MaLocation
 *  @see MaLocation
 */
typedef struct MaLocation {
    MaAuth          *auth;                  /**< Per location block authentication */
    int             flags;                  /**< Location flags */
    char            *prefix;                /**< Location prefix name */
    int             prefixLen;              /**< Length of the prefix name */
    int             sessionTimeout;         /**< Session timeout for this location */
    struct MaStage  *handler;               /**< Set handler */
    void            *handlerData;           /**< Data reserved for the handler */
    MprHashTable    *extensions;            /**< Hash of handlers by extensions */
    MprList         *handlers;              /**< List of handlers for this location */
    MprList         *inputStages;           /**< Input stages */
    MprList         *outputStages;          /**< Output stages */
    MprHashTable    *errorDocuments;
    struct MaStage  *connector;             /**< Network connector */
    struct MaLocation *parent;              /**< Parent location */
#if BLD_FEATURE_SSL
    struct MprSsl   *ssl;                   /**< SSL configuration */
#endif
} MaLocation;

extern void maAddErrorDocument(MaLocation *location, cchar *code, cchar *url);
extern MaLocation *maCreateBareLocation(MprCtx ctx);
extern MaLocation *maCreateLocation(MprCtx ctx, MaLocation *location);
extern void maFinalizeLocation(MaLocation *location);
extern struct MaStage *maGetHandlerByExtension(MaLocation *location, cchar *ext);
extern cchar *maLookupErrorDocument(MaLocation *location, int code);
extern void maResetPipeline(MaLocation *location);
extern void maSetLocationAuth(MaLocation *location, MaAuth *auth);
extern void maSetLocationHandler(MaLocation *location, cchar *name);
extern void maSetLocationPrefix(MaLocation *location, cchar *uri);
extern void maSetLocationFlags(MaLocation *location, int flags);

/*********************************** MaAlias **********************************/
/**
 *  Aliases 
 *  @stability Evolving
 *  @defgroup MaAlias MaAlias
 *  @see MaAlias maCreateAlias
 */
typedef struct MaAlias {
    char            *prefix;                /**< Original URI prefix */
    int             prefixLen;              /**< Prefix length */
    char            *filename;              /**< Alias to a physical path name */
    char            *uri;                   /**< Redirect to a uri */
    int             redirectCode;
} MaAlias;

extern MaAlias *maCreateAlias(MprCtx ctx, cchar *prefix, cchar *name, int code);

/******************************* MaMimeType ******************************/
/**
 *  Mime Type hash table entry (the URL extension is the key)
 *  @stability Evolving
 *  @defgroup MaMimeType MaMimeType
 *  @see MaMimeType
 */
typedef struct MaMimeType {
    char            *type;
    char            *actionProgram;
} MaMimeType;

/************************************ MaHost **********************************/
/*
 *  Flags
 */
#define MA_HOST_VHOST           0x2         /* Is a virtual host */
#define MA_HOST_NAMED_VHOST     0x4         /* Named virtual host */
#define MA_HOST_NO_TRACE        0x40        /* Prevent use of TRACE */

/**
 *  Host Object
 *  A Host object represents a single listening HTTP connection endpoint. This may be a default server or a
 *  virtual server. Multiple Hosts may be contained within a single Server.
 *  @stability Evolving
 *  @defgroup MaHost MaHost
 *  @see MaHost
 */
typedef struct MaHost {
    MaServer        *server;                /**< Server owning this host */
    struct MaHost   *parent;                /**< Parent host for virtual hosts */

    MprList         *connections;           /**< Currently open connection requests */

    MprFile         *accessLog;             /**< File object for access logging */
    MprList         *aliases;               /**< List of Alias definitions */
    MprList         *dirs;                  /**< List of Directory definitions */
    char            *documentRoot;
    int             flags;
    int             httpVersion;            /**< HTTP/1.X */
    char            *ipAddrPort;            /**< IP:PORT address (with wildcards) */
    MaLimits        *limits;                /**< Pointer to http->limits */
    MaLocation      *location;              /**< Default location */
    MprList         *locations;             /**< List of Location defintions */
    struct MaHost   *logHost;               /**< If set, use this hosts logs */
    char            *mimeFile;              /**< Name of the mime types file */
    char            *moduleDirs;            /**< Directories for modules */
    char            *name;                  /**< ServerName directive */
    char            *secret;                /**< Random bytes for authentication */
    int             timeout;                /**< Max time a request can take */
    bool            secure;                 /**< Host is a secure (SSL) host */
    MprEvent        *timer;                 /**< Admin service timer */

#if BLD_FEATURE_ACCESS_LOG
    char            *logFormat;             /**< Access log format */
    char            *logPath;               /**< Access log filename */
#endif

    int             keepAlive;              /**< Keep alive supported - TODO should be in flags */
    int             keepAliveTimeout;       /**< Timeout for keep-alive */
    int             maxKeepAlive;           /**< Max keep-alive requests */

    MprHashTable    *mimeTypes;             /**< Hash table of mime types (key is extension) */
    MprTime         whenCurrentDate;        /**< When was the current date last computed */
    char            *currentDate;           /**< Date string for HTTP response headers */

#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;
#endif

} MaHost;


/*
 *  All these APIs are internal
 */
extern void         maAddConn(MaHost *host, struct MaConn *conn);
extern void         maAddStandardMimeTypes(MaHost *host);
extern MaHost       *maCreateDefaultHost(MaServer *server, cchar *docRoot, cchar *ipAddr, int port);
extern int          maCreateRequestPipeline(MaHost *host, struct MaConn *conn, MaAlias *alias);
extern MaAlias      *maGetAlias(MaHost *host, cchar *uri);
extern MaAlias      *maLookupAlias(MaHost *host, cchar *prefix);
extern MaDir        *maLookupBestDir(MaHost *host, cchar *path);
extern MaDir        *maLookupDir(MaHost *host, cchar *path);
extern MaLocation   *maLookupBestLocation(MaHost *host, cchar *uri);
extern MaHost       *maCreateHost(MaServer *server, cchar *ipAddr, MaLocation *location);
extern MaHost       *maCreateVirtualHost(MaServer *server, cchar *ipAddrPort, MaHost *host);
extern MaLocation   *maLookupLocation(MaHost *host, cchar *prefix);
extern MaMimeType   *maAddMimeType(MaHost *host, cchar *ext, cchar *mimetype);
extern cchar        *maGetMimeActionProgram(MaHost *host, cchar *mimetype);
extern cchar        *maLookupMimeType(MaHost *host, cchar *ext);
extern int          maInsertAlias(MaHost *host, MaAlias *newAlias);
extern int          maAddLocation(MaHost *host, MaLocation *newLocation);
extern int          maInsertDir(MaHost *host, MaDir *newDir);
extern int          maOpenMimeTypes(MaHost *host, cchar *path);
extern void         maRemoveConn(MaHost *host, struct MaConn *conn);
extern void         maSetMaxKeepAlive(MaHost *host, int timeout);
extern int          maSetMimeActionProgram(MaHost *host, cchar *mimetype, cchar *actionProgram);
extern int          maStartHost(MaHost *host);
extern int          maStopHost(MaHost *host);
extern void         maSetDocumentRoot(MaHost *host, cchar *dir) ;
extern void         maSetHostIpAddrPort(MaHost *host, cchar *ipAddrPort);
extern void         maSetHostName(MaHost *host, cchar *name);
extern void         maSetHttpVersion(MaHost *host, int version);
extern void         maSetKeepAlive(MaHost *host, bool on);
extern void         maSetKeepAliveTimeout(MaHost *host, int timeout);
extern void         maSetNamedVirtualHost(MaHost *host);
extern void         maSecureHost(MaHost *host, struct MprSsl *ssl);
extern void         maSetTimeout(MaHost *host, int timeout);
extern void         maSetTraceMethod(MaHost *host, bool on);
extern void         maSetVirtualHost(MaHost *host);

#if UNUSED
extern void         maDestroySession(MaConn *conn);
extern void         maSetDefaultSessionTimeout(MaHost *host, int timeout);
extern void         maSetSessionAutoCreate(MaHost *host, bool on);
extern MaSession    *maLookupSession(MaHost *host, cchar *sessionId);
extern void         maAddSession(MaHost *host, struct MaSession *session);
extern void         maRemoveSession(MaHost *host, struct MaSession *session);
extern MaSession    *maCreateSession(MaConn *conn, int timeout);
#endif

#if UNUSED
//  TODO - must implement
/******************************** MaRequestMatch ******************************/
/*
 *  MaRequestMatch stores "If-Match" and "If-None-Match" request headers.
 */
typedef struct MaRequestMatch {
    MprList         etags;
    bool            ifMatch;
} MaRequestMatch;


/*
 *  MaRequestModified stores "If-Modified-Since" and "If-Unmodified-Since" request headers.
 */
typedef struct  MaRequestModified {
    MprTime         since;
    bool            ifModified;
} MaRequestModified;

#endif

/******************************************************************************/
/**
 *  Current config parse state
 *  @stability Evolving
 *  @defgroup MaConfigState MaConfigState
 *  @see MaConfigState
 */
typedef struct MaConfigState {
    MaServer    *server;                    /**< Current server */
    MaHost      *host;                      /**< Current host */
    MaDir       *dir;                       /**< Current directory block */
    MaLocation  *location;                  /**< Current location */
    MaAuth      *auth;                      /**< Current auth object */
    MprFile     *file;                      /**< Config file handle */
    char        *filename;                  /** Config file name */
    int         lineNumber;                 /**< Current line number */
    int         enabled;                    /**< True if the current block is enabled */
} MaConfigState;

extern MaLocation *maCreateLocationAlias(MaHttp *http, MaConfigState *state, cchar *prefix, cchar *path, 
        cchar *handlerName, int flags);

/******************************************************************************/
/**
 *  Content range structure
 *  @pre
 *      Range:  0,  49  First 50 bytes
 *      Range: -1, -50  Last 50 bytes
 *      Range:  1,  -1  Skip first byte then select content to the end
 *  @stability Evolving
 *  @defgroup MaRange MaRange
 *  @see MaRange
 */
typedef struct MaRange {
    //  TODO - should use MprOffset
    int             start;                  /**< Start of range */
    int             end;                    /**< End byte of range + 1 */
    int             len;                    /**< Redundant range length */
    struct MaRange  *next;                  /**< Next range */
} MaRange;

typedef int (*MaRangeFillProc)(struct MaQueue *q, struct MaPacket *packet);
extern void maRangeService(struct MaQueue *q, MaRangeFillProc fill);
extern void maCreateRangeBoundary(struct MaConn *conn);
extern bool maFixRangeLength(struct MaConn *conn);

/*
 *  Packet flags
 */
#define MA_PACKET_HEADER    0x1             /**< Packet contains HTTP headers */
#define MA_PACKET_RANGE     0x2             /**< Packet is a range boundary packet */
#define MA_PACKET_DATA      0x4             /**< Packet contains actual content data */
#define MA_PACKET_END       0x8             /**< End of stream packet */

/**
 *  Data packet. 
 *  @description The request/response pipeline sends data and control information in MaPacket objects. The output
 *      stream typically consists of a HEADER packet followed by zero or more data packets and terminated by an END
 *      packet. If the request has input data, the input stream is consists of one or more data packets followed by
 *      an END packet.
 *      \n\n
 *      Packets contain data and optional prefix or suffix headers. Packets can be split, joined, filled or emptied. 
 *      The pipeline stages will fill or transform packet data as required.
 *  @stability Evolving
 *  @defgroup MaPacket MaPacket
 *  @see MaPacket MaQueue maCreateDataPackage maCreatePacket maCreateEndPacket maJoinPacket maSplitPacket 
 *      maGetPacketLength maCreateHeaderPacket
 */
typedef struct MaPacket {
    MprBuf          *prefix;                /**< Prefix message to be emitted before the content */
    MprBuf          *content;               /**< Chunk content */
    MprBuf          *suffix;                /**< Prefix message to be emitted after the content */
    int             flags;                  /**< Packet flags */
    int             count;                  /**< Count of bytes in packet */
#if UNUSED
    int             pos;                    /**< Offset to seek in entity body for data */
#endif
    struct MaPacket *next;                  /**< Next packet in chain */
    struct MaConn   *conn;                  /**< Owning connection */
} MaPacket;

/**
 *  Create a data packet
 *  @description Create a packet of the required size.
 *  @param conn MaConn connection object
 *  @param size Size of the package data storage.
 *  @return MaPacket object.
 *  @ingroup MaPacket
 */
extern MaPacket *maCreatePacket(struct MaConn *conn, int size);

/**
 *  Create a data packet
 *  @description Create a packet and set the MA_PACKET_DATA flag
 *      Data packets convey data through the response pipeline.
 *  @param conn MaConn connection object
 *  @param size Size of the package data storage.
 *  @return MaPacket object.
 *  @ingroup MaPacket
 */
extern MaPacket *maCreateDataPacket(struct MaConn *conn, int size);

/**
 *  Create a response header packet
 *  @description Create a response header packet and set the MA_PACKET_HEADER flag. 
 *      A header packet is used by the pipeline to hold the response headers.
 *  @param conn MaConn connection object
 *  @return MaPacket object.
 *  @ingroup MaPacket
 */
extern MaPacket *maCreateHeaderPacket(struct MaConn *conn);

/**
 *  Create an end packet
 *  @description Create an end-of-stream packet and set the MA_PACKET_END flag. The end pack signifies the 
 *      end of data. It is used on both incoming and outgoing streams through the request/response pipeline.
 *  @param conn MaConn connection object
 *  @return MaPacket object.
 *  @ingroup MaPacket
 */
extern MaPacket *maCreateEndPacket(struct MaConn *conn);

/**
 *  Join tow packets
 *  @description Join the contents of one packet to another by copying the data from the \a other packet into 
 *      the first packet. 
 *  @param packet Destination packet
 *  @param other Other packet to copy data from.
 *  @return Zero if successful, otherwise a negative Mpr error code
 *  @ingroup MaPacket
 */
extern int maJoinPacket(MaPacket *packet, MaPacket *other);

/**
 *  Split a data packet
 *  @description Split a data packet at the specified offset. Packets may need to be split so that downstream
 *      stages can digest their contents. If a packet is too large for the queue maximum size, it should be split.
 *      When the packet is split, a new packet is created containing the data after the offset. Any suffix headers
 *      are moved to the new packet.
 *  @param conn MaConn connection object
 *  @param packet Packet to split
 *  @param offset Location in the original packet at which to split
 *  @return New MaPacket object containing the data after the offset. No need to free, unless you have a very long
 *      running request. Otherwise the packet memory will be released automatically when the request completes.
 *  @ingroup MaPacket
 */
extern MaPacket *maSplitPacket(struct MaConn *conn, MaPacket *packet, int offset);

/**
 *  Get the length of the packet data contents
 *  @description Get the content length of a packet. This does not include the prefix or suffix data length -- just
 *      the pure data contents.
 *  @param packet Packet to examine.
 *  @return Count of bytes contained by the packet.
 *  @ingroup MaPacket
 */
extern int maGetPacketLength(MaPacket *packet);


/*
 *  Queue directions
 */
#define MA_QUEUE_SEND           0           /**< Send (transmit to client) queue */
#define MA_QUEUE_RECEIVE        1           /**< Receive (read from client) queue */
#define MA_MAX_QUEUE            2           /**< Number of queue types */

/*
 *  Queue flags
 */
#define MA_QUEUE_OPEN           0x1         /**< Queue's open routine has been called */
#define MA_QUEUE_DISABLED       0x2         /**< Queue's service routine is disabled */
#define MA_QUEUE_FULL           0x4         /**< Queue is full */
#define MA_QUEUE_ALL            0x8         /**< Queue has all the data there is and will be */
#define MA_QUEUE_SERVICED       0x10        /**< Queue has been serviced at least once */
#define MA_QUEUE_EOF            0x20        /**< Queue at end of data */

/*
 *  Queue callback prototypes
 */
typedef void (*MaQueueOpen)(struct MaQueue *q);
typedef void (*MaQueueClose)(struct MaQueue *q);
typedef void (*MaQueueData)(struct MaQueue *q, MaPacket *packet);
typedef void (*MaQueueService)(struct MaQueue *q);

/**
 *  Queue Head
 *  @description The request pipeline consists of a full-duplex pipeline of stages. Each stage has two queues,
 *      one for outgoing data and one for incoming. A MaQueue object manages the data flow for a request stage
 *      and has the ability to queue and process data, manage flow control and schedule packets for service.
 *      \n\n
 *      Queue's provide open, close, put and service methods. These methods manage and respond to incoming packets.
 *      A queue can respond immediately to an incoming packet by processing or dispatching a packet in its put() method.
 *      Alternatively, the queue can defer processing by queueing the packet on it's service queue and then waiting for
 *      it's service() method to be invoked. 
 *      \n\n
 *      If a queue does not define a put() method, the default put method will 
 *      be used which queues data onto the service queue. The default incoming put() method joins incoming packets
 *      into a single packet on the service queue.
 *  @stability Evolving
 *  @defgroup MaQueue MaQueue
 *  @see MaQueue MaPacket MaConn maDiscardData maGet maJoinForService maPutForService maDefaultPut maDisableQueue
 *      maEnableQueue maGetQueueRoom maIsQueueEmpty maPacketTooBig maPut maPutBack maPutForService maPutNext
 *      maRemoveQueue maResizePacket maScheduleQueue maSendPacket maSendPackets maSendEndPacket maServiceQueue
 *      maWillNextQueueAccept maWrite maWriteBlock maWriteBody maWriteString
 */
typedef struct MaQueue {
    cchar           *owner;                 /**< Name of owning stage */
    struct MaStage  *stage;                 /**< Stage owning this queue */
    struct MaConn   *conn;                  /**< Connection ownning this queue */
    MaQueueOpen     open;                   /**< Open the queue */
    MaQueueClose    close;                  /**< Close the queue */
    MaQueueData     put;                    /**< Put a message on the queue */
    MaQueueService  service;                /**< Service the queue */
    struct MaQueue  *nextQ;                 /**< Downstream queue for next stage */
    struct MaQueue  *prevQ;                 /**< Upstream queue for prior stage */
    struct MaQueue  *scheduleNext;          /**< Next linkage when queue is on the service queue */
    struct MaQueue  *schedulePrev;          /**< Previous linkage when queue is on the service queue */
    struct MaQueue  *pair;                  /**< Queue for the same stage in the opposite direction */
    MaPacket        *first;                 /**< First packet in queue (singly linked) */
    MaPacket        *last;                  /**< Last packet in queue (tail pointer) */
    MaPacket        *pending;               /**< Packets pending more dynamic data output */
    int             count;                  /**< Bytes in queue */
    int             max;                    /**< Maxiumum queue size */
    int             low;                    /**< Low water mark for flow control */
    int             flags;                  /**< Queue flags */
    int             packetSize;             /**< Maximum acceptable packet size */
    int             direction;              /**< Flow direction */
    void            *queueData;             /**< Stage instance data */

    /*
     *  Connector instance data. Put here to save a memory allocation.
     */
    MprIOVec        iovec[MA_MAX_IOVEC];
    int             ioIndex;                /**< Next index into iovec */
    int             ioCount;                /**< Count of bytes in iovec */
    int             ioFileEntry;            /**< Has file entry in iovec */
    int             ioFileOffset;           /**< The next file position to use */
} MaQueue;


/**
 *  Discard all data from the queue
 *  @description Discard data from the queue. If removePackets (not yet implemented) is true, then remove the packets
 *      otherwise, just discard the data and preserve the packets.
 *  @param q Queue reference
 *  @param removePackets If true, the data packets will be removed from the queue
 *  @ingroup MaQueue
 */
extern void maDiscardData(MaQueue *q, bool removePackets);

/**
 *  Get the next packet from a queue
 *  @description Get the next packet. This will remove the packet from the queue and adjust the queue counts
 *      accordingly. If the queue was full and upstream queues were blocked, they will be enabled.
 *  @param q Queue reference
 *  @return The packet removed from the queue.
 *  @ingroup MaQueue
 */
extern MaPacket *maGet(MaQueue *q);

/**
 *  Join a packet onto the service queue
 *  @description Add a packet to the service queue. If the queue already has data, then this packet
 *      will be joined (aggregated) into the existing packet. If serviceQ is true, the queue will be scheduled
 *      for service.
 *  @param q Queue reference
 *  @param packet Packet to join to the queue
 *  @param serviceQ If true, schedule the queue for service
 *  @ingroup MaQueue
 */
extern void maJoinForService(MaQueue *q, MaPacket *packet, bool serviceQ);

/**
 *  Put a packet onto the service queue
 *  @description Add a packet to the service queue. If serviceQ is true, the queue will be scheduled for service.
 *  @param q Queue reference
 *  @param packet Packet to join to the queue
 *  @param serviceQ If true, schedule the queue for service
 *  @ingroup MaQueue
 */
extern void maPutForService(MaQueue *q, MaPacket *packet, bool serviceQ);

/**
 *  Disable a queue
 *  @description Mark a queue as disabled so that it will not be scheduled for service.
 *  @param q Queue reference
 *  @ingroup MaQueue
 */
extern void maDisableQueue(MaQueue *q);

/**
 *  Enable a queue
 *  @description Enable a queue for service and schedule it to run. This will cause the service routine
 *      to run as soon as possible.
 *  @param q Queue reference
 *  @ingroup MaQueue
 */
extern void maEnableQueue(MaQueue *q);

/**
 *  Get the room in the queue
 *  @description Get the amount of data the queue can accept before being full.
 *  @param q Queue reference
 *  @return A count of bytes that can be written to the queue
 *  @ingroup MaQueue
 */
extern int maGetQueueRoom(MaQueue *q);

/**
 *  Determine if the queue is empty
 *  @description Determine if the queue has no packets queued. This does not test if the queue has no data content.
 *  @param q Queue reference
 *  @return True if there are no packets queued.
 *  @ingroup MaQueue
 */
extern bool maIsQueueEmpty(MaQueue *q);

/**
 *  Test if a packet is too big 
 *  @description Test if a packet is too big to fit downstream. If the packet content exceeds the downstream queue's 
 *      maximum or exceeds the downstream queue's requested packet size -- then this routine will return true.
 *  @param q Queue reference
 *  @param packet Packet to test
 *  @return True if the packet is too big for the downstream queue
 *  @ingroup MaQueue
 */
extern bool maPacketTooBig(MaQueue *q, MaPacket *packet);

/**
 *  Put a packet onto a queue
 *  @description Put the packet onto the end of queue by calling the queue's put() method. 
 *  @param q Queue reference
 *  @param packet Packet to put
 *  @ingroup MaQueue
 */
extern void maPut(MaQueue *q, MaPacket *packet);

/**
 *  Put a packet back onto a queue
 *  @description Put the packet back onto the front of the queue. The queue's put() method is not called.
 *      This is typically used by the queue's service routine when a packet cannot complete processing.
 *  @param q Queue reference
 *  @param packet Packet to put back
 *  @ingroup MaQueue
 */
extern void maPutBack(MaQueue *q, MaPacket *packet);

/**
 *  Put a packet onto a service queue
 *  @description Put the packet onto the service queue and optionally schedule the queue for service.
 *  @param q Queue reference
 *  @param packet Packet to put
 *  @ingroup MaQueue
 */
extern void maPutForService(MaQueue *q, MaPacket *packet, bool serviceQ);

/**
 *  Put a packet onto the next queue
 *  @description Put a packet onto the next downstream queue by calling the downstreams queue's put() method. 
 *  @param q Queue reference. The packet will not be queued on this queue, but rather on the queue downstream.
 *  @param packet Packet to put
 *  @ingroup MaQueue
 */
extern void maPutNext(MaQueue *q, MaPacket *packet);

/**
 *  Remove a queue
 *  @description Remove a queue from the request/response pipeline. This will remove a queue so that it does
 *      not participate in the pipeline, effectively removing the processing stage from the pipeline. This is
 *      useful to remove unwanted filters and to speed up pipeline processing
 *  @param q Queue reference
 *  @ingroup MaQueue
 */
extern void maRemoveQueue(MaQueue *q);

/**
 *  Resize a packet
 *  @description Resize a packet if required so that it fits in the downstream queue. This may split the packet
 *      if it is too big to fit in the downstream queue. If it is split, the tail portion is put back on the queue.
 *  @param q Queue reference
 *  @param packet Packet to put
 *  @param size If size is > 0, then also ensure the packet is not larger than this size.
 *  @return Zero if successful, otherwise a negative Mpr error code
 *  @ingroup MaQueue
 */
extern int  maResizePacket(MaQueue *q, MaPacket *packet, int size);

/**
 *  Schedule a queue
 *  @description Schedule a queue by adding it to the schedule queue. Queues are serviced FIFO.
 *  @param q Queue reference
 *  @ingroup MaQueue
 */
extern void maScheduleQueue(MaQueue *q);

/**
 *  Send all queued packets
 *  @description Send all queued packets downstream
 *  @param q Queue reference
 *  @ingroup MaQueue
 */
extern void maSendPackets(MaQueue *q);

/**
 *  Send an end packet
 *  @description Create and send an end-of-stream packet downstream
 *  @param q Queue reference
 *  @ingroup MaQueue
 */
extern void maSendEndPacket(MaQueue *q);

/**
 *  Service a queue
 *  @description Service a queue by invoking its service() routine. 
 *  @param q Queue reference
 *  @ingroup MaQueue
 */
extern void maServiceQueue(MaQueue *q);

/**
 *  Determine if the downstream queue will accept this packet.
 *  @description Test if the downstream queue will accept a packet. The packet will be resized if required in an
 *      attempt to get the downstream queue to accept it. If the downstream queue is full, disable this queue
 *      and mark the downstream queue as full and service it immediately to try to relieve the congestion.
 *  @param q Queue reference
 *  @param packet Packet to put
 *  @return True if the downstream queue will accept the packet. Use #maPutNext to send the packet downstream
 *  @ingroup MaQueue
 */
extern bool maWillNextQueueAccept(MaQueue *q, MaPacket *packet);

/**
 *  Write a formatted string
 *  @description Write a formatted string of data into packets onto the end of the queue. Data packets will be created
 *      as required to store the write data. This call may block waiting for the downstream queue to drain if it is 
 *      or becomes full.
 *  @param q Queue reference
 *  @param fmt Printf style formatted string
 *  @param ... Arguments for fmt
 *  @return A count of the bytes actually written
 *  @ingroup MaQueue
 */
extern int maWrite(MaQueue *q, cchar *fmt, ...);

/**
 *  Write a block of data to the queue
 *  @description Write a block of data into packets onto the end of the queue. Data packets will be created
 *      as required to store the write data.
 *  @param q Queue reference
 *  @param buf Buffer containing the write data
 *  @param size of the data in buf
 *  @param block Set to true to block and wait for data to drain if the downstream queue is full. If false, the
 *      call may return not having written all the data. The return value indicates how many bytes were actually written.
 *  @return A count of the bytes actually written
 *  @ingroup MaQueue
 */
extern int maWriteBlock(MaQueue *q, cchar *buf, int size, bool block);

/**
 *  Write a string of data to the queue
 *  @description Write a string of data into packets onto the end of the queue. Data packets will be created
 *      as required to store the write data. This call may block waiting for the downstream queue to drain if it is 
 *      or becomes full.
 *  @param q Queue reference
 *  @param s String containing the data to write
 *  @return A count of the bytes actually written
 *  @ingroup MaQueue
 */
extern int maWriteString(MaQueue *q, cchar *s);

/*
 *  Internal
 */
//TODO - merge maCleanQueue with maDiscardData
extern void maCleanQueue(MaQueue *q);
extern MaQueue  *maCreateQueue(struct MaConn *conn, struct MaStage *stage, int direction, MaQueue *prev);
extern MaQueue *maGetNextQueueForService(MaQueue *q);
extern void maInitQueue(MaHttp *http, MaQueue *q, cchar *name);
extern void maInitSchedulerQueue(MaQueue *q);
extern void maInsertQueue(MaQueue *prev, MaQueue *q);

/******************************** Pipeline Stages *****************************/
/*
 *  Stage Flags. Use request method flags as-is so we can quickly validate methods for stages.
 */
#define MA_STAGE_DELETE     MA_REQ_DELETE   /**< Support DELETE requests */
#define MA_STAGE_GET        MA_REQ_GET      /**< Support GET requests */
#define MA_STAGE_HEAD       MA_REQ_HEAD     /**< Support HEAD requests */
#define MA_STAGE_OPTIONS    MA_REQ_OPTIONS  /**< Support OPTIONS requests */
#define MA_STAGE_POST       MA_REQ_POST     /**< Support POST requests */
#define MA_STAGE_PUT        MA_REQ_PUT      /**< Support PUT requests */
#define MA_STAGE_TRACE      MA_REQ_TRACE    /**< Support TRACE requests */
#define MA_STAGE_ALL        MA_REQ_MASK     /**< Mask for all methods */

#define MA_STAGE_CONNECTOR  0x1000          /**< Stage is a connector  */
#define MA_STAGE_HANDLER    0x2000          /**< Stage is a handler  */
#define MA_STAGE_FILTER     0x4000          /**< Stage is a filter  */
#define MA_STAGE_MODULE     0x8000          /**< Stage is a filter  */
#define MA_STAGE_FORM_VARS  0x10000         /**< Create query and form variables table */
#define MA_STAGE_ENV_VARS   0x20000         /**< Create CGI style environment variables table */
#define MA_STAGE_VIRTUAL    0x40000         /**< Handler serves virtual resources not the physical file system */
#define MA_STAGE_PATH_INFO  0x80000         /**< Always do path info processing */
#define MA_STAGE_AUTO_DIR   0x100000        /**< Want auto directory redirection */

/**
 *  Pipeline Stages
 *  @description The request pipeline consists of a full-duplex pipeline of stages. 
 *      Stages are used to process client HTTP requests in a modular fashion. Each stage either creates, filters or
 *      consumes data packets. The MaStage structure describes the stage capabilities and callbacks.
 *      Each stage has two queues, one for outgoing data and one for incoming data.
 *      \n\n
 *      Stages provide callback methods for parsing configuration, matching requests, open/close, run and the
 *      acceptance and service of incoming and outgoing data.
 *  @stability Evolving
 *  @defgroup MaStage MaStage 
 *  @see MaStage MaQueue MaConn maCreateConnector maCreateFilter maCreateHandler maDefaultOutgoingServiceStage
 *      maLookupStageData
 */
typedef struct MaStage {
    char            *name;                  /**< Stage name */
    int             flags;                  /**< Stage flags */

    /**
     *  Parse configuration data.
     *  @description This is invoked when parsing appweb configuration files
     *  @param http MaHttp object
     *  @param key Configuration directive name
     *  @param value Configuration directive value
     *  @param state Current configuration parsing state
     *  @return Zero if the key was not relevant to this stage. Return 1 if the directive applies to this stage and
     *      was accepted.
     *  @ingroup MaStage
     */
    int             (*parse)(MaHttp *http, cchar *key, char *value, MaConfigState *state);

    /**
     *  Match a request
     *  @description This method is invoked to see if the stage wishes to handle the request. If a stage denies to
     *      handle a request, it will be removed from the pipeline.
     *  @param conn MaConn connection object
     *  @param stage Stage object
     *  @param uri Current request URI
     *  @return True if the stage wishes to process this request.
     *  @ingroup MaStage
     */
    bool            (*match)(struct MaConn *conn, struct MaStage *stage, cchar *uri);

    /**
     *  Open the queue
     *  @description Open the queue instance and initialize for this request.
     *  @param q Queue instance object
     *  @ingroup MaStage
     */
    void            (*open)(MaQueue *q);

    /**
     *  Close the queue
     *  @description Close the queue instance
     *  @param q Queue instance object
     *  @ingroup MaStage
     */
    void            (*close)(MaQueue *q);

    /**
     *  Run the queue
     *  @description The queue is run after all incoming data has been received.
     *  @param q Queue instance object
     *  @ingroup MaStage
     */
    void            (*run)(MaQueue *q);

    /**
     *  Process outgoing data.
     *  @description Accept a packet as outgoing data
     *  @param q Queue instance object
     *  @param packet Packet of data
     *  @ingroup MaStage
     */
    void            (*outgoingData)(MaQueue *q, MaPacket *packet);

    /**
     *  Service the outgoing data queue
     *  @param q Queue instance object
     *  @ingroup MaStage
     */
    void            (*outgoingService)(MaQueue *q);

    /**
     *  Process incoming data.
     *  @description Accept an incoming packet of data
     *  @param q Queue instance object
     *  @param packet Packet of data
     *  @ingroup MaStage
     */
    void            (*incomingData)(MaQueue *q, MaPacket *packet);

    /**
     *  Service the incoming data queue
     *  @param q Queue instance object
     *  @ingroup MaStage
     */
    void            (*incomingService)(MaQueue *q);

    void            *stageData;             /**< Per-stage data */
} MaStage;


/**
 *  Create a connector stage
 *  @description Create a new connector. Connectors are the final stage for outgoing data. Their job is to transmit
 *      outgoing data to the client.
 *  @param http MaHttp object returned from #maCreateHttp
 *  @param name Name of connector stage
 *  @param flags Stage flags mask. These specify what Http request methods will be supported by this stage. Or together
 *      any of the following flags:
 *      @li MA_STAGE_DELETE     - Support DELETE requests
 *      @li MA_STAGE_GET        - Support GET requests
 *      @li MA_STAGE_HEAD       - Support HEAD requests
 *      @li MA_STAGE_OPTIONS    - Support OPTIONS requests
 *      @li MA_STAGE_POST       - Support POST requests
 *      @li MA_STAGE_PUT        - Support PUT requests
 *      @li MA_STAGE_TRACE      - Support TRACE requests
 *      @li MA_STAGE_ALL        - Mask to support all methods
 *  @return A new stage object
 *  @ingroup MaStage
 */
extern MaStage *maCreateConnector(MaHttp *http, cchar *name, int flags);

/**
 *  Create a filter stage
 *  @description Create a new filter. Filters transform data generated by handlers and before connectors transmit to
 *      the client. Filters can apply transformations to incoming, outgoing or bi-directional data.
 *  @param http MaHttp object returned from #maCreateHttp
 *  @param name Name of connector stage
 *  @param flags Stage flags mask. These specify what Http request methods will be supported by this stage. Or together
 *      any of the following flags:
 *      @li MA_STAGE_DELETE     - Support DELETE requests
 *      @li MA_STAGE_GET        - Support GET requests
 *      @li MA_STAGE_HEAD       - Support HEAD requests
 *      @li MA_STAGE_OPTIONS    - Support OPTIONS requests
 *      @li MA_STAGE_POST       - Support POST requests
 *      @li MA_STAGE_PUT        - Support PUT requests
 *      @li MA_STAGE_TRACE      - Support TRACE requests
 *      @li MA_STAGE_ALL        - Mask to support all methods
 *  @return A new stage object
 *  @ingroup MaStage
 */
extern MaStage *maCreateFilter(MaHttp *http, cchar *name, int flags);

/**
 *  Create a request handler stage
 *  @description Create a new handler. Handlers generate outgoing data and are the final stage for incoming data. 
 *      Their job is to process requests and send outgoing data downstream toward the client consumer.
 *      There is ever only one handler for a request.
 *  @param http MaHttp object returned from #maCreateHttp
 *  @param name Name of connector stage
 *  @param flags Stage flags mask. These specify what Http request methods will be supported by this stage. Or together
 *      any of the following flags:
 *      @li MA_STAGE_DELETE     - Support DELETE requests
 *      @li MA_STAGE_GET        - Support GET requests
 *      @li MA_STAGE_HEAD       - Support HEAD requests
 *      @li MA_STAGE_OPTIONS    - Support OPTIONS requests
 *      @li MA_STAGE_POST       - Support POST requests
 *      @li MA_STAGE_PUT        - Support PUT requests
 *      @li MA_STAGE_TRACE      - Support TRACE requests
 *      @li MA_STAGE_ALL        - Mask to support all methods
 *  @return A new stage object
 *  @ingroup MaStage
 */
extern MaStage *maCreateHandler(MaHttp *http, cchar *name, int flags);

/**
 *  Default outgoing data handling
 *  @description This routine provides default handling of outgoing data for stages. It simply sends all packets
 *      downstream.
 *  @param q Queue object
 *  @ingroup MaStage
 */
extern void maDefaultOutgoingServiceStage(MaQueue *q);

/**
 *  Lookup stage data
 *  @description This looks up the stage by name and returns the private stage data.
 *  @param http MaHttp object returned from #maCreateHttp
 *  @param name Name of the stage concerned
 *  @return Reference to the stage data block.
 *  @ingroup MaStage
 */
extern void *maLookupStageData(MaHttp *http, cchar *name);

/*
 *  Internal API
 */
extern int maAddHandler(MaHttp *http, MaLocation *location, cchar *name, cchar *extensions);
extern MaStage *maCreateStage(MaHttp *http, cchar *name, int flags);
extern struct MaStage *maLookupStage(MaHttp *http, cchar *name);
extern int maOpenPassHandler(MaHttp *http);
extern int maOpenNetConnector(MaHttp *http);
extern int maOpenSendConnector(MaHttp *http);
extern void maRegisterStage(MaHttp *http, MaStage *stage);
extern int maSetConnector(MaHttp *http, MaLocation *location, cchar *name);
extern int maSetHandler(MaHttp *http, MaHost *host, MaLocation *location, cchar *name);

/**
 *  Filter Stages
 *  @stability Evolving
 *  @defgroup MaFilter MaFilter
 *  @see MaFilter
 */
typedef struct MaFilter {
    MprHashTable    *extensions;
    MaStage         *stage;
} MaFilter;

/*
 *  Direction flags for AddFilter
 */
#define MA_FILTER_INCOMING          0x1
#define MA_FILTER_OUTGOING          0x2

extern int maAddFilter(MaHttp *http, MaLocation *location, cchar *name, cchar *extensions, int dir);
extern bool maMatchFilterByExtension(MaFilter *filter, cchar *ext);

/********************************** MaResponse *********************************/
/*
 *  Connection flags
 */
#define MA_CONN_CLOSE               0x1     /**< Connection needs to be closed */
#define MA_CONN_CLEAN_MASK          0x1     /**< Mask to clear flags after a request completes */
#define MA_CONN_CASE_INSENSITIVE    0x2     /**< System case-insensitive for file matches */

/**
 *  Http Connections
 *  @description The MaConn object represents a TCP/IP connection to the client. A connection object is created for
 *      each socket connection initiated by the client. One MaConn object may service many Http requests due to 
 *      HTTP/1.1 keep-alive.
 *  @stability Evolving
 *  @defgroup MaConn MaConn
 *  @see MaConn MaRequest MaResponse MaQueue MaStage
 */
typedef struct MaConn {

    MprHeap          *arena;                /**< Connection memory arena */

    struct MaRequest *request;              /**< Request object */
    struct MaResponse *response;            /**< Response object */
    struct MaQueue  serviceq;               /**< List of queues that require service for request pipeline */

    MaHostAddress   *address;               /**< Host address structure for this connection */
    MaHttp          *http;                  /**< Http handle */
    MaHost          *host;                  /**< Owning host for this request */
    MprSocket       *sock;                  /**< Underlying socket handle */
    MaHost          *originalHost;          /**< Owning host for this connection. May be changed by Host header  */
    MaPacket        *input;                 /**< Header packet */
    char            *remoteIpAddr;          /**< Remote client IP address (REMOTE_ADDR) */
    MprTime         started;                /**< When the connection started */
    MprTime         expire;                 /**< When the connection should expire */
    MprTime         time;                   /**< Cached current time */

    int             requestFailed;          /**< Request failed. Abbreviate request processing */
    int             abandonConnection;      /**< Abandon all processing on the current connection. Emit no data. */
    int             canProceed;             /**< State machine should continue to process the request */
    int             flags;                  /**< Connection flags */
    int             keepAliveCount;         /**< Count of remaining keep alive requests for this connection */
    int             protocol;               /**< HTTP protocol version 0 == HTTP/1.0, 1 == HTTP/1.1*/
    int             remotePort;             /**< Remote client IP port number */
    int             socketEventMask;        /**< Mask of events to receive */
    int             state;                  /**< Connection state */
    int             timeout;                /**< Timeout period in msec */
} MaConn;


extern void maAcceptConn(MaServer *server, MprSocket *sock, cchar *ip, int port);
extern void maAwakenConn(MaConn *conn);
extern void maCloseConn(MaConn *conn);
extern void maCreateEnvVars(MaConn *conn);
extern void maCreatePipeline(MaConn *conn);
extern void maDiscardPipeData(MaConn *conn);
extern void *maGetHandlerQueueData(struct MaConn *conn);
extern void maMatchHandler(MaConn *conn);
extern void maResetConn(MaConn *conn);
extern bool maRunPipeline(MaConn *conn);
extern void maStartPipeline(MaConn *conn);
extern bool maServiceQueues(MaConn *conn);
extern void maSetRangeDimensions(MaConn *conn, int length);
extern void maSetRequestGroup(MaConn *conn, cchar *group);
extern void maSetRequestUser(MaConn *conn, cchar *user);

/********************************** MaRequest *********************************/
/*
 *  Request methods
 */
#define MA_REQ_DELETE       0x1             /**< DELETE method  */
#define MA_REQ_GET          0x2             /**< GET method  */
#define MA_REQ_HEAD         0x4             /**< HEAD method  */
#define MA_REQ_OPTIONS      0x8             /**< OPTIONS method  */
#define MA_REQ_POST         0x10            /**< Post method */
#define MA_REQ_PUT          0x20            /**< PUT method  */
#define MA_REQ_TRACE        0x40            /**< TRACE method  */
#define MA_REQ_MASK         0x7F            /**< Method mask */

/*
 *  Request flags
 */
#define MA_REQ_CREATE_ENV   0x1             /**< Must create env for this request */
#define MA_REQ_IF_MODIFIED  0x2             /**< If-[un]modified-since supplied */
#define MA_REQ_CHUNKED      0x4             /**< Content is chunk encoded */

/**
 *  Http Requests
 *  @description Most of the APIs in the Request group still take a MaConn object as their first parameter. This is
 *      to make the API easier to remember - APIs take a connection object rather than a request or response object.
 *  @stability Evolving
 *  @defgroup MaRequest MaRequest
 *  @see MaRequest MaConn MaResponse maMapUriToStorage maRequestWriteBlocked maSetNoKeepAlive
 *      maAddFormVars maCompareFormVar maGetCookies maGetFormVar maGetIntFormVar maGetNumEnvProperties
 *      maGetQueryString maSetIntFormVar maSetFormVar maUnsetFormVar maTestFormVar 
 */
typedef struct MaRequest {

    MprHeap         *arena;                 /**< Request memory arena */
    struct MaConn   *conn;                  /**< Connection object */
    MaPacket        *headerPacket;          /**< HTTP headers */
    MaPacket        *packet;                /**< Current input packet */

    char            *methodName;            /**< Protocol method GET|PUT... (ENV: REQUEST_METHOD) */
    char            *httpProtocol;          /**< HTTP/1.0 or HTTP/1.1 */

    /*
     *  Request line. When a handler supports PATH_INFO, URLs are broken into the following: 
     *      {SCHEMA}://{HOST}:{PORT}{URL}{PATH_INFO}
     *
     *  url == scriptName
     */
    char            *url;                   /**< Decoded URL. Doesn't include scheme, host, extra path, query or fragments */
    char            *pathInfo;              /**< Extra path information (ENV: PATH_INFO) */
    char            *pathTranslated;        /**< Mapped extraPath to storage (ENV: PATH_TRANSLATED) */


    /*
     *  Header values
     */
    char            *accept;                /**< Accept header */
    char            *acceptCharset;         /**< Accept-Charset header */
    char            *acceptEncoding;        /**< Accept-Encoding header */
    char            *cookie;                /**< Request cookie header */
    char            *connection;            /**< Connection header */
    char            *contentLengthStr;      /**< Content length string value */
    char            *forwarded;             /**< Forwarded header */
    char            *hostName;              /**< Client supplied host name */
    char            *pragma;                /**< Pragma header */
    char            *mimeType;              /**< Mime type of the request payload (ENV: CONTENT_TYPE) */
    char            *referer;               /**< Refering URL */
    char            *userAgent;             /**< User-Agent header */

    MaRange         *ranges;                /**< Requested ranges for response data */
    MaRange         *inputRange;            /**< Specified range for input (post) data */
    MprUri          *parsedUri;             /**< Parsed query string. parsedUri->query is the query string */

    int             length;                 /**< Declared content length (ENV: CONTENT_LENGTH) */
    int             remainingChunk;         /**< Remaining chunk data to read */
    int             remainingContent;       /**< Remaining content data to read */
    int             receivedContent;        /**< Length of content actually received */
    int             method;                 /**< Request method */
    int             flags;                  /**< Request modifiers */

    /*
     *  Auth details
     */
    char            *authDetails;
    char            *authType;              /**< Authorization type (basic|digest) (ENV: AUTH_TYPE) */
    char            *group;                 /**< Supplied via basic / digest auth  */
    char            *password;
    char            *user;                  /**< Remote user (ENV: REMOTE_USER) */


    MprHashTable    *formVars;              /**< Query and post data variables */
    MaHost          *host;                  /**< Owning host for this request */
    MprList         *inputPipeline;         /**< Input processing */
    MprHashTable    *headers;               /**< Header variables */
    MaPacket        *headerpacket;          /**< Packet containing all headers ( == conn->input) */
    MaAlias         *alias;                 /**< Matching alias */
    MaAuth          *auth;                  /**< Set to either dir or location auth information */
    MaDir           *dir;                   /**< Best matching dir (PTR only) */
    MaLocation      *location;              /**< Location block */
    MprList         *etags;                 /**< Document etag to uniquely identify the document version */
    MprTime         since;                  /**< If-Modified date */
    bool            ifModified;             /**< If-Modified processing requested */
    bool            ifMatch;                /**< If-Match processing requested */

#if UNUSED
    //  TODO 
    MaRequestMatch  requestMatch;
    MaRequestModified requestModified;
#endif
} MaRequest;

/**
 *  Add form variables
 *  @description Add new form variables encoded in the supplied buffer
 *  @param conn MaConn connection object
 *  @param buf Buffer containing www-urlencoded data
 *  @param len Length of buf
 *  @ingroup MaRequest
 */
extern void maAddFormVars(MaConn *conn, cchar *buf, int len);

/**
 *  Compare a form variable
 *  @description Compare a form variable and return true if it exists and its value matches.
 *  @param conn MaConn connection object
 *  @param var Name of the form variable 
 *  @param value Value to compare
 *  @return True if the value matches
 *  @ingroup MaRequest
 */
extern int maCompareFormVar(MaConn *conn, cchar *var, cchar *value);

/**
 *  Get the cookies
 *  @description Get the cookies defined in the current requeset
 *  @param conn MaConn connection object
 *  @return Return a string containing the cookies sent in the Http header of the last request
 *  @ingroup MaRequest
 */
extern cchar *maGetCookies(MaConn *conn);

/**
 *  Get a form variable
 *  @description Get the value of a named form variable. Form variables are define via www-urlencoded query or post
 *      data contained in the request.
 *  @param conn MaConn connection object
 *  @param var Name of the form variable to retrieve
 *  @param defaultValue Default value to return if the variable is not defined. Can be null.
 *  @return String containing the form variable's value. Caller should not free.
 *  @ingroup MaRequest
 */
extern cchar *maGetFormVar(MaConn *conn, cchar *var, cchar *defaultValue);

/**
 *  Get a form variable as an integer
 *  @description Get the value of a named form variable as an integer. Form variables are define via 
 *      www-urlencoded query or post data contained in the request.
 *  @param conn MaConn connection object
 *  @param var Name of the form variable to retrieve
 *  @param defaultValue Default value to return if the variable is not defined. Can be null.
 *  @return Integer containing the form variable's value
 *  @ingroup MaRequest
 */
extern int maGetIntFormVar(MaConn *conn, cchar *var, int defaultValue);

/**
 *  Get the request query string
 *  @description Get query string sent with the current request.
 *  @param conn MaConn connection object
 *  @return String containing the request query string. Caller should not free.
 *  @ingroup MaRequest
 */
extern cchar *maGetQueryString(MaConn *conn);

/**
 *  Set a form variable value
 *  @description Set the value of a named form variable to an integer value. Form variables are define via 
 *      www-urlencoded query or post data contained in the request.
 *  @param conn MaConn connection object
 *  @param var Name of the form variable to retrieve
 *  @param value Default value to return if the variable is not defined. Can be null.
 *  @ingroup MaRequest
 */
extern void maSetIntFormVar(MaConn *conn, cchar *var, int value);

/**
 *  Set a form variable value
 *  @description Set the value of a named form variable to a string value. Form variables are define via 
 *      www-urlencoded query or post data contained in the request.
 *  @param conn MaConn connection object
 *  @param var Name of the form variable to retrieve
 *  @param value Default value to return if the variable is not defined. Can be null.
 *  @ingroup MaRequest
 */
extern void maSetFormVar(MaConn *conn, cchar *var, cchar *value);

/**
 *  Test if a form variable is defined
 *  @param conn MaConn connection object
 *  @param var Name of the form variable to retrieve
 *  @return True if the form variable is defined
 *  @ingroup MaRequest
 */
extern int maTestFormVar(MaConn *conn, cchar *var);

extern char *maMapUriToStorage(MaConn *conn, cchar *uri);
extern void maRequestWriteBlocked(MaConn *conn);
extern void maSetNoKeepAlive(MaConn *conn);


extern void         maCompleteRequest(MaConn *conn);
extern bool         maContentNotModified(MaConn *conn);
extern MaRange      *maCreateRange(MaConn *conn, int start, int end);
extern MaRequest    *maCreateRequest(MaConn *conn);
extern MaRequest    *maCreateRequest(struct MaConn *conn);
extern void         maFailConnection(struct MaConn *conn, int code, cchar *fmt, ...);
extern MaAuth       *maGetAuth(MaConn *conn);
extern void         maProcessReadEvent(MaConn *conn, MaPacket *packet);
extern void         maProcessWriteEvent(MaConn *conn);
extern void         maSetRequestFlags(MaConn *conn, int orFlags, int andFlags);
extern int          maSetRequestUri(MaConn *conn, cchar *newUri);
extern void         maSetEtag(MaConn *conn, MprFileInfo *info);

/********************************** MaResponse *********************************/
/*
 *  Response flags
 */
#define MA_RESP_DONT_CACHE          0x1     /**< Add no-cache to the response */
#define MA_RESP_DONT_FINISH         0x2     /**< Don't auto finish the request */
#define MA_RESP_NO_BODY             0x4     /**< No respose body, only return headers to client */
#define MA_RESP_HEADERS_CREATED     0x8     /**< Response headers have been created */

/**
 *  Http Response
 *  @description Most of the APIs in the Response group still take a MaConn object as their first parameter. This is
 *      to make the API easier to remember - APIs take a connection object rather than a request or response object.
 *  @stability Evolving
 *  @defgroup MaResponse MaResponse
 *  @see MaResponse MaRequest MaConn maSetCookie maFailRequest maFormatBody
 */
typedef struct MaResponse {

    struct MaConn   *conn;                  /**< Current connection object */
    MaStage         *handler;               /**< Response handler */
    MaStage         *connector;             /**< Response connector */
    MprList         *outputPipeline;        /**< Output processing */
    MaPacket        *freePackets;           /**< List of free packets */
    void            *handlerData;           /**< Data reserved for the handler */

    int             flags;                  /**< Response flags */
    int             code;                   /**< HTTP response code */
    int             length;                 /**< Response content length */
    int             chunkSize;              /**< Chunk size to use when using transfer encoding */
    char            *etag;                  /**< Unique identifier tag */
    char            *header;                /**< HTTP response header */
    cchar           *mimeType;              /**< Mime type of the response document */
    char            *altBody;               /**< Alternate response for errors */

    MprHashTable    *headers;               /**< Custom response headers */
    MaQueue         queue[2];               /**< Dummy head for the response queues */
    int             pos;                    /**< Current I/O position */

    /*
     *  File information for file based handlers
     */
    MprFile         *file;                  /**< File to be served */
    MprFileInfo     fileInfo;               /**< File information if there is a real file to serve */
    char            *filename;              /**< Name of a real file being served */
    cchar           *extension;             /**< Filename extension */
    int             entityLength;           /**< Original content length before range subsetting */

    int             bytesWritten;           /**< Bytes written including headers */
    int             headerSize;             /**< Size of the header written */

    MaRange         *currentRange;          /**< Current range being fullfilled */
    char            *rangeBoundary;         /**< Inter-range boundary */

} MaResponse;

//DDD
/**
 *  Set a response cookie
 *  @description Define a cookie to send in the response Http header
 *  @param conn MaConn connection object
 *  @param name Cookie name
 *  @param value Cookie value
 *  @param lifetime Duration for the cookie to persist in seconds
 *  @param path URI path to which the cookie applies
 *  @param secure Set to true if the cookie only applies for SSL based connections
 *  @ingroup MaResponse
 */
extern void maSetCookie(MaConn *conn, cchar *name, cchar *value, int lifetime, cchar *path, bool secure);

/**
 *  Fail a request
 *  @description Fail a request with the specified http status code. The formatted message is logged to the appweb
 *      error log.
 *  @param conn MaConn connection object
 *  @param code Http status code
 *  @param fmt Printf style formatted string
 *  @param ... Arguments for fmt
 *  @ingroup MaResponse
 */
extern void maFailRequest(MaConn *conn, int code, cchar *fmt, ...);

/**
 *  Format an alternate response body
 *  @description Format a response body to use instead of data generated by the request processing pipeline.
 *      This is typically used to send error responses and redirections.
 *  @param conn MaConn connection object
 *  @param title Title string to format into the HTML response body.
 *  @param fmt Printf style formatted string. This string may contain HTML tags and is not HTML encoded before
 *      sending to the user. NOTE: Do not send user input back to the client using this method. Otherwise you open
 *      large security holes.
 *  @param ... Arguments for fmt
 *  @return A count of the number of bytes in the response body.
 *  @ingroup MaResponse
 */
extern int maFormatBody(MaConn *conn, cchar *title, cchar *fmt, ...);

/**
 *  Dont cache the response 
 *  @description Instruct the client not to cache the response body. This is done by setting the Cache-control Http
 *      header.
 *  @param conn MaConn connection object
 *  @ingroup MaResponse
 */
extern void maDontCacheResponse(MaConn *conn);

/**
 *  Create the response headers
 *  @description Fill the given empty packet with the Http response headers. This should only be called by connectors
 *      just prior to sending output to the client. It should be delayed as long as possible if the content length is
 *      not yet known to give the pipeline a chance to determine the response length. This way, a non-chunked response
 *      can be sent with a content-length header. This is the fastest HTTP response.
 *  @param conn MaConn connection object
 *  @param packet Packet into which to place the headers
 *  @ingroup MaResponse
 */
extern void maFillHeaders(MaConn *conn, MaPacket *packet);

/**
 *  Redirect the client
 *  @description Redirect the client to a new uri.
 *  @param conn MaConn connection object
 *  @param code Http status code to send with the response
 *  @param uri New uri for the client
 *  @ingroup MaResponse
 */
extern void maRedirect(MaConn *conn, int code, cchar *uri);

/**
 *  Set a Http response code.
 *  @description Set the Http response code for the request. This defaults to 200 (OK).
 *  @param conn MaConn connection object
 *  @param code Http status code.
 *  @ingroup MaResponse
 */
extern void maSetResponseCode(MaConn *conn, int code);

/**
 *  Set the response mime type
 *  @description Set the response mime type Http header.
 *  @param conn MaConn connection object
 *  @param mimeType Mime type string
 *  @ingroup MaResponse
 */
extern void maSetResponseMimeType(MaConn *conn, cchar *mimeType);

/**
 *  Set a response header
 *  @description Set the response header. If allowMultiple is true, then duplicate keys can be defined. Otherwise, 
 *      prior key headers are overwritten.
 *  @param conn MaConn connection object
 *  @param allowMultiple If true, allow duplicate keys of the same value
 *  @param key Http response header key
 *  @param fmt Printf style formatted string to use as the header key value
 *  @param ... Arguments for fmt
 *  @ingroup MaResponse
 */
extern void maSetHeader(MaConn *conn, bool allowMultiple, cchar *key, cchar *fmt, ...);


extern void         maCloseStage(MaConn *conn);
extern MaResponse   *maCreateResponse(MaConn *conn);
extern int          maDateParse(cchar *cmd);
extern char         *maGetDateString(MprCtx ctx, MprFileInfo *sbuf);
extern void         maLogRequest(MaConn *conn);
extern char         *maMakePath(MaHost *host, char *buf, int buflen, cchar *file);
extern void         maOmitResponseBody(MaConn *conn);
extern char         *maReplaceReferences(MaHost *host, char *buf, int buflen, cchar *str);
extern int          maRewriteUri(MaConn *conn);
extern void         maRotateAccessLog(MaHost *host);
extern void         maSetEntityLength(MaConn *conn, int len);
extern void         maSetNoResponseData(MaConn *conn);
extern int          maStopLogging(MprCtx ctx);
extern int          maStartLogging(MprCtx ctx, cchar *logSpec);
extern int          maStartAccessLogging(MaHost *host);
extern int          maStopAccessLogging(MaHost *host);
extern void         maSetAccessLog(MaHost *host, cchar *path, cchar *format);
extern void         maSetLogHost(MaHost *host, MaHost *logHost);
extern void         maTraceOptions(MaConn *conn);
extern void         maWriteAccessLogEntry(MaHost *host, cchar *buf, int len);

/************************************ CGI *************************************/

#define MA_CGI_SEEN_HEADER          0x1

/************************************ EGI *************************************/

#if BLD_FEATURE_EGI

typedef struct MaEgi {
    MprHashTable        *forms;
} MaEgi;


typedef void (MaEgiForm)(MaQueue *q);

extern int maDefineEgiForm(MaHttp *http, cchar *name, MaEgiForm *form);

#endif


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _h_HTTP_SERVER */
