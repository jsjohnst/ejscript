#include "appweb.h"

/******************************************************************************/
/* 
 *  This file is an amalgamation of all the individual source code files for
 *  Embedthis Appweb 3.0B.0.
 *
 *  Catenating all the source into a single file makes embedding simpler and
 *  the resulting application faster, as many compilers can do whole file
 *  optimization.
 *
 *  If you want to modify appweb, you can still get the whole source
 *  as individual files if you need.
 */


/************************************************************************/
/*
 *  Start of file "../server/appweb.c"
 */
/************************************************************************/

/**
 *  appweb.c  -- AppWeb main programs.
 *
 *  usage: %s [options] [IpAddr[:port]] [documentRoot]
 *          --config configFile     # Use config file instead of IP address (default: appweb.conf)
 *          --debug                 # Run in debug mode
 *          --ejs name:path         # Create an ejs application at the path
 *          --log logFile:level     # Log to file file at verbosity level
 *          --name uniqueName       # Name for this instance
 *          --threads maxThreads    # Set maximum pool threads
 *          --version               # Output version information
 *          -v                      # Same as --log stdout:2
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



/*
 *  Singleton server object
 */
MaServer    *server;


extern void appwebOsTerm();
static void memoryFailure(MprCtx ctx, uint askSize, uint totalHeapMem, bool granted);
extern int  osInit(Mpr *mpr);
static void printUsage(Mpr *mpr);
extern int  securityChecks(Mpr *mpr, cchar *program);

#if BLD_FEATURE_EJS
static void createEjsAlias(Mpr *mpr, MaHttp *http, MaServer *server, cchar *ejsAlias);
#endif
#if BLD_UNIX_LIKE
static void catchSignal(int signo, siginfo_t *info, void *arg);
static int  unixSecurityChecks(Mpr *mpr, cchar *program);
static int  setupUnixSignals(Mpr *mpr);
#endif

#if BLD_WIN_LIKE
static int writePort(MaHost *host);
static long msgProc(HWND hwnd, uint msg, uint wp, long lp);
#if UNUSED
static int  setupWindow(Mpr *mpr);
static int  findInstance(Mpr *mpr);
static int  initWindow(Mpr *mpr);
#endif
#endif


int main(int argc, char **argv)
{
    Mpr         *mpr;
    MaHttp      *http;
    cchar       *ipAddrPort, *documentRoot, *homeDir, *argp, *logSpec, *ejsAlias;
    char        *configFile, *ipAddr;
    int         err, poolThreads, outputVersion, argind, port;
    
    mpr = mprCreate(argc, argv, memoryFailure);
    mprSetAppName(mpr, argv[0], BLD_NAME, BLD_VERSION);

    mprSetLogLevel(mpr, 2);

    if (securityChecks(mpr, argv[0]) < 0) {
        exit(3);
    }

    if (osInit(mpr) < 0) {
        exit(2);
    }

    if (mprStart(mpr, 0) < 0) {
        mprUserError(mpr, "Can't start MPR for %s", mprGetAppName(mpr));
        mprFree(mpr);
        return MPR_ERR_CANT_INITIALIZE;
    }

    mprAllocSprintf(mpr, &configFile, -1, "./%s.conf", mprGetAppName(mpr));

    documentRoot = 0;
    err = 0;
    ejsAlias = 0;
    ipAddrPort = 0;
    ipAddr = 0;
    port = -1;
    logSpec = 0;
    homeDir = 0;
    outputVersion = 0;
    poolThreads = -1;

    for (argind = 1; !err && argind < argc; argind++) {
        argp = argv[argind];
        if (*argp != '-') {
            break;
        }

        if (strcmp(argp, "--config") == 0) {
            if (argind >= argc) {
                err++;
            } else {
                configFile = argv[++argind];
            }

#if BLD_UNIX_LIKE
        } else if (strcmp(argp, "--chroot") == 0) {
            if (argind >= argc) {
                err++;

            } else {
                char    *path;
                path = mprGetAbsFilename(mpr, argv[++argind]);
                chdir(path);
                if (chroot(path) < 0) {
                    if (errno == EPERM) {
                        mprErrorPrintf(mpr, "%s: Must be super user to use the --chroot option", mprGetAppName(mpr));
                    } else {
                        mprErrorPrintf(mpr, "%s: Can't change change root directory to %s, errno %d",
                            mprGetAppName(mpr), path, errno);
                    }
                    exit(4);
                }
                mprFree(path);
            }
#endif

        } else if (strcmp(argp, "--debug") == 0 || strcmp(argp, "-d") == 0) {
            mprSetDebugMode(mpr, 1);

        } else if (strcmp(argp, "--ejs") == 0) {
            if (argind >= argc) {
                err++;
            } else {
                ejsAlias = argv[++argind];
            }

        } else if (strcmp(argp, "--home") == 0) {
            if (argind >= argc) {
                err++;

            } else {
                homeDir = argv[++argind];
                if (chdir((char*) homeDir) < 0) {
                    mprErrorPrintf(mpr, "%s: Can't change directory to %s\n", mprGetAppName(mpr), homeDir);
                    err++;
                }
            }

        } else if (strcmp(argp, "--log") == 0 || strcmp(argp, "-l") == 0) {
            if (argind >= argc) {
                err++;

            } else {
                logSpec = argv[++argind];
                maStartLogging(mpr, logSpec);
            }

        } else if (strcmp(argp, "--name") == 0 || strcmp(argp, "-n") == 0) {
            if (argind >= argc) {
                err++;

            } else {
                mprSetAppName(mpr, argv[++argind], 0, 0);
            }

        } else if (strcmp(argp, "--threads") == 0) {
            if (argind >= argc) {
                err++;

            } else {
                poolThreads = atoi(argv[++argind]);
            }

#if UNUSED
        } else if (strcmp(argp, "--title") == 0) {
            if (argind >= argc) {
                err++;

            } else {
                mprSetAppName(mpr, 0, argv[++argind], 0);
            }
#endif

        } else if (strcmp(argp, "--verbose") == 0 || strcmp(argp, "-v") == 0) {
            maStartLogging(mpr, "stdout:4");

        } else if (strcmp(argp, "--version") == 0 || strcmp(argp, "-V") == 0) {
            outputVersion++;

        } else {
            mprErrorPrintf(mpr, "Unknown switch \"%s\"", argp);
            err++;
            break;
        }
    }

    if (argc > argind) {
        if (argc > (argind + 2)) {
            err++;
        } else {
            ipAddrPort = argv[argind++];
            if (argc > argind) {
                documentRoot = argv[argind++];
            } else {
                documentRoot = ".";
            }
        }
    }

    if (outputVersion) {
        mprPrintf(mpr, "%s: Version: %s\n", mprGetAppName(mpr), mprGetAppVersion(mpr));
        exit(0);
    }

    if (ipAddrPort) {
        mprParseIp(mpr, ipAddrPort, &ipAddr, &port, MA_SERVER_DEFAULT_PORT_NUM);
    } else {
#if BLD_FEATURE_CONFIG_PARSE
        if (!mprAccess(mpr, configFile, R_OK)) {
            mprErrorPrintf(mpr, "Can't open config file %s", configFile);
            err++;
        }
#else
        err++;
#endif
    }

    if (err) {
        printUsage(mpr);
        return MPR_ERR_BAD_SYNTAX;
    }

    /*
     *  Create the top level HTTP service and default HTTP server. Set the initial server root to "."
     */
    http = maCreateHttp(mpr);
    if (http == 0) {
        mprUserError(mpr, "Can't create HTTP service for %s", mprGetAppName(mpr));
        return MPR_ERR_CANT_INITIALIZE;
    }

    server = maCreateServer(http, "default", ".", 0, -1);
    if (server == 0) {
        mprUserError(mpr, "Can't create HTTP server for %s", mprGetAppName(mpr));
        return MPR_ERR_CANT_INITIALIZE;
    }

    if (maConfigureServer(mpr, http, server, configFile, ipAddr, port, documentRoot) < 0) {
        /* mprUserError(mpr, "Can't configure the server, exiting."); */
        exit(6);
    }

#if BLD_FEATURE_EJS
    if (ejsAlias) {
        createEjsAlias(mpr, http, server, ejsAlias);
    }
#endif

#if BLD_FEATURE_MULTITHREAD
    if (poolThreads >= 0) {
        mprSetMaxPoolThreads(http, poolThreads);
    }
#endif
#if BLD_WIN_LIKE
	if (!ejsAlias) {
		writePort(server->defaultHost);
	}
#endif

    if (maStartHttp(http) < 0) {
        mprUserError(mpr, "Can't start HTTP service, exiting.");
        exit(7);
    }

#if BLD_FEATURE_MULTITHREAD
    mprLog(mpr, 1, "HTTP services are ready with %d pool threads", mprGetMaxPoolThreads(mpr));
#else
    mprLog(mpr, 1, "HTTP services are ready (single-threaded)");
#endif

    /*
     *  Service HTTP events until instructed to exit
     */
//  mprPrintAllocReport(mprGetMpr(0), "After initialization");
    mprServiceEvents(mpr, -1, 0);

    /*
     *  Signal a graceful shutdown
     */
    maStopHttp(http);

#if TODO
    mprFree(http);
    mprFree(mpr);
#endif

    return 0;
}


#if BLD_FEATURE_EJS
/*
 *  Create an ejs application location block and alias
 */
static void createEjsAlias(Mpr *mpr, MaHttp *http, MaServer *server, cchar *ejsAlias)
{
    MaAlias     *alias;
    MaHost      *host;
    MaDir       *dir, *parent;
    MaLocation  *location;
    char        *cp, *path, target[MPR_MAX_FNAME];
    int         flags, len;

    host = server->defaultHost;
    flags = host->location->flags & (MA_LOC_BROWSER | MA_LOC_AUTO_SESSION /* | MA_LOC_PATH_INFO */);

    if ((path = strchr(ejsAlias, ':')) != 0) {
        *path++ = '\0';
        mprStrcpy(target, sizeof(target) - 2, path);
        path = target;
    } else {
        if ((path = getcwd(target, sizeof(target) - 2)) == 0) {
            mprError(http, "Can't get cwd");
            return;
        }
    }
    len = strlen(target);
    if (len > 0 && target[len - 1] != '/') {
        cp = &target[len];
        *cp++ = '/';
        *cp = '\0';
    }
    
    if (ejsAlias[0] != '/' || ejsAlias[strlen(ejsAlias) - 1] != '/') {
        mprError(http, "Ejs aliases should begin and end with \"/\"");
    }
    alias = maCreateAlias(host, ejsAlias, path, 0);
    maInsertAlias(host, alias);
    mprLog(http, 4, "Alias \"%s\" for \"%s\"", ejsAlias, path);

    if (maLookupLocation(host, ejsAlias)) {
        mprError(http, "Location block already exists for \"%s\"", ejsAlias);
        return;
    }
    location = maCreateLocation(host, host->location);
    //  TODO - what should the auth be set to?
    maSetLocationAuth(location, host->location->auth);
    maSetLocationPrefix(location, ejsAlias);
    maAddLocation(host, location);
    maSetLocationFlags(location, MA_LOC_APP | flags);
    maSetHandler(http, host, location, "ejsHandler");

    /*
     *  Make sure there is a directory for the alias target
     */
    dir = maLookupBestDir(host, path);
    if (dir == 0) {
        parent = mprGetFirstItem(host->dirs);
        dir = maCreateDir(host, alias->filename, parent);
        maInsertDir(host, dir);
    }
}
#endif


/*
 *  Display the program command line usage
 */
static void printUsage(Mpr *mpr)
{
    cchar   *name;

    name = mprGetAppName(mpr);

    mprErrorPrintf(mpr, "\n\n%s Usage:\n\n"
    "  %s [options]\n"
    "  %s [options] [IPaddress][:port] [documentRoot] \n\n"
    "  Options:\n"
    "    --config configFile    # Use named config file instead appweb.conf\n"
    "    --chroot directory     # Change root directory to run more securely (Unix)\n"
    "    --debug                # Run in debug mode\n"
    "    --ejs appSpec          # Create an ejs application at the path\n"
    "    --home directory       # Change to directory to run\n"
    "    --name uniqueName      # Unique name for this instance\n"
    "    --log logFile:level    # Log to file file at verbosity level\n"
    "    --threads maxThreads   # Set maximum pool threads\n"
    "    --version              # Output version information\n\n"
    "  Without IPaddress, %s will read the appweb.conf configuration file.\n\n",
    name, name, name, name, name);
}


/*
 *  Global memory failure hook
 *  TODO -- do more here on a redline. Should recycle.
 */
static void memoryFailure(MprCtx ctx, uint size, uint total, bool granted)
{
    if (!granted) {
        mprPrintf(ctx, "Can't allocate memory block of size %d\n", size);
        mprPrintf(ctx, "Total memory used %d\n", total);
        exit(255);
    }

    mprPrintf(ctx, "Memory request for %d bytes exceeds memory red-line\n", size);
    mprPrintf(ctx, "Total memory used %d\n", total);
}


int osInit(Mpr *mpr)
{
#if BLD_WIN_LIKE && UNUSED
    if (setupWindow(mpr) < 0) {
        return -1;
    }
#endif
#if BLD_UNIX_LIKE
    setupUnixSignals(mpr);
#endif
    return 0;
}


/*
 *  Security checks. Make sure we are staring with a safe environment
 */
int securityChecks(Mpr *mpr, cchar *program)
{
#if BLD_UNIX_LIKE
    if (unixSecurityChecks(mpr, program) < 0) {
        return -1;
    }
#endif
    return 0;
}


#if BLD_UNIX_LIKE
/*
 *  Security checks. Make sure we are staring with a safe environment
 */
static int unixSecurityChecks(Mpr *mpr, cchar *program)
{
    char            dir[MPR_MAX_FNAME];
    struct stat     sbuf;

#if UNUSED
    /*
     *  This pulls in a whole lot of code
     */
    uid_t           uid;
    uid = getuid();
    if (getpwuid(uid) == 0) {
        mprUserError(mpr, "Bad user id: %d", uid);
        return MPR_ERR_BAD_STATE;
    }
#endif

    dir[sizeof(dir) - 1] = '\0';
    if (getcwd(dir, sizeof(dir) - 1) == NULL) {
        mprUserError(mpr, "Can't get the current working directory");
        return MPR_ERR_BAD_STATE;
    }

    if (((stat(dir, &sbuf)) != 0) || !(S_ISDIR(sbuf.st_mode))) {
        mprUserError(mpr, "Can't access directory: %s", dir);
        return MPR_ERR_BAD_STATE;
    }
    if ((sbuf.st_mode & S_IWOTH) || (sbuf.st_mode & S_IWGRP)) {
        mprUserError(mpr, "Security risk, directory %s is writable by others", dir);
    }

    /*
     *  Should always convert the program name into a fully qualified path
     *  Otherwise this fails
     */
    if (*program == '/') {
        if (((lstat(program, &sbuf)) != 0) || (S_ISLNK(sbuf.st_mode))) {
            mprUserError(mpr, "Can't access program: %s", program);
            return MPR_ERR_BAD_STATE;
        }
        if ((sbuf.st_mode & S_IWOTH) || (sbuf.st_mode & S_IWGRP)) {
            mprUserError(mpr, "Security risk, Program %s is writable by others", program);
        }
        if (sbuf.st_mode & S_ISUID) {
            mprUserError(mpr, "Security risk, %s is setuid", program);
        }
        if (sbuf.st_mode & S_ISGID) {
            mprUserError(mpr, "Security risk, %s is setgid", program);
        }
    }
    return 0;
}


/*
 *  Signals need a global reference to the mpr
 */
static Mpr *_signalMpr;

static int setupUnixSignals(Mpr *mpr)
{
    struct sigaction    act;

    _signalMpr = mpr;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = catchSignal;
    act.sa_flags = 0;
   
    /*
     *  Mask these signals when processing signals
     */
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGCHLD);
    sigaddset(&act.sa_mask, SIGALRM);
    sigaddset(&act.sa_mask, SIGPIPE);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGUSR1);
    sigaddset(&act.sa_mask, SIGUSR2);
    sigaddset(&act.sa_mask, SIGTERM);

    if (!mprGetDebugMode(mpr)) {
        sigaddset(&act.sa_mask, SIGINT);
    }

    /*
     *  Catch thse signals
     */
    sigaction(SIGINT, &act, 0);
    sigaction(SIGQUIT, &act, 0);
    sigaction(SIGTERM, &act, 0);

    /*
     *  Ignore pipe signals
     */
    signal(SIGPIPE, SIG_IGN);

#if LINUX
    /*
     *  Ignore signals from write requests to large files
     */
    signal(SIGXFSZ, SIG_IGN);
#endif
    return 0;
}


/*
 *  Catch signals. Do a graceful shutdown.
 */
static void catchSignal(int signo, siginfo_t *info, void *arg)
{
    Mpr     *mpr;

    mpr = _signalMpr;

    mprLog(mpr, 1, "\n%s: Received signal %d\nExiting ...\n", mprGetAppName(mpr), signo);
    if (mpr) {
#if DEBUG_IDE
        if (signo != 2) {
            mprTerminate(mpr, 1);
        }
#else
        mprTerminate(mpr, 1);
#endif
    }
}

#endif /* BLD_HOST_UNIX */

#if BLD_WIN_LIKE

#if XX_UNUSED
static int setupWindow(Mpr *mpr)
{
    if (findInstance(mpr)) {
        mprUserError(mpr, "Application %s is already active.", mprGetAppTitle(mpr));
        return MPR_ERR_BUSY;
    }

    /*
     *  Create the window
     */
    if (initWindow(mpr) < 0) {
        mprUserError(mpr, "Can't initialize application Window");
        return MPR_ERR_CANT_INITIALIZE;
    }
    mprSetWinMsgCallback(mpr->waitService, msgProc);
    return 0;
}


/*
 *  See if an instance of this product is already running
 */
static int findInstance(Mpr *mpr)
{
    HWND    hwnd;

    hwnd = FindWindow(mprGetAppName(mpr), mprGetAppTitle(mpr));
    if (hwnd) {
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        }
        SetForegroundWindow(hwnd);
        return 1;
    }
    return 0;
}


/*
 *  Initialize the applications's window
 */
static int initWindow(Mpr *mpr)
{
    WNDCLASS    wc;
    HWND        appHwnd;
    int         rc;

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW+1);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = 0;
    wc.hIcon            = NULL;
    wc.lpfnWndProc      = (WNDPROC) msgProc;
    wc.lpszMenuName     = wc.lpszClassName = mprGetAppName(mpr);

    rc = RegisterClass(&wc);
    if (rc == 0) {
        mprError(mpr, "Can't register windows class");
        return -1;
    }

    appHwnd = CreateWindow(mprGetAppName(mpr), mprGetAppTitle(mpr), WS_OVERLAPPED, CW_USEDEFAULT, 
        0, 0, 0, NULL, NULL, 0, NULL);

    if (! appHwnd) {
        mprError(mpr, "Can't create window");
        return -1;
    }
    mprSetHwnd(mpr, appHwnd);
    mprSetSocketMessage(mpr, APPWEB_SOCKET_MESSAGE);
mprLog(mpr, 0, "HWND is %x", appHwnd);

    return 0;
}


/*
 *  Windows message processing loop
 */
static long msgProc(HWND hwnd, uint msg, uint wp, long lp)
{
    int     sock, winMask;

    if (msg == WM_DESTROY || msg == WM_QUIT) {
        mprTerminate(_globalMpr, 1);

    } else if (msg == APPWEB_SOCKET_MESSAGE) {
        sock = (int) wp;
        winMask = LOWORD(lp);
        mprServiceWinIO(_globalMpr->waitService, sock, winMask);

#if UNUSED
    } else if (msg == APPWEB_QUERY_PORT_MESSAGE) {
        port = -1;
        ipSpec = server->defaultHost->ipAddrPort;
        if (ipSpec) {
            cp = strchr(ipSpec, ':');
            if (cp) {
                port = atoi(&cp[1]);
            }
        }
        if (InSendMessage()) {
            /* Message came from another thread/process */
            ReplyMessage(port);
        }
#endif

    } else {
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}
#endif


/*
 *  Write the port so the monitor can manage
 */ 
static int writePort(MaHost *host)
{
    char    *cp, numBuf[16], path[MPR_MAX_FNAME], dir[MPR_MAX_FNAME];
    int     fd, port, len;

    mprGetAppDir(host, dir, sizeof(dir));
    mprSprintf(path, sizeof(path), "%s/.port.log", dir);

    if ((fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666)) < 0) {
        mprError(host, "Could not create port file %s\n", path);
        return MPR_ERR_CANT_CREATE;
    }

    //  TODO - need an API for this.
    cp = host->ipAddrPort;
    if ((cp = strchr(host->ipAddrPort, ':')) != 0) {
        port = atoi(++cp);
    } else {
        port = 80;
    }

    mprItoa(numBuf, sizeof(numBuf), port, 10);

    len = (int) strlen(numBuf);
    numBuf[len++] = '\n';
    if (write(fd, numBuf, len) != len) {
        mprError(host, "Write to file %s failed\n", path);
        return MPR_ERR_CANT_WRITE;
    }
    close(fd);
    return 0;
}

#endif /* WIN */

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
 *  End of file "../server/appweb.c"
 */
/************************************************************************/

