/**
 *  ejs.c - Ejscript shell
 *
 *  Interactive shell that interprets interactive sessions and command files.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"
#include    "ecCompiler.h"

/*********************************** Locals ***********************************/

static int  consoleGets(EcStream *stream);
static int  commandGets(EcStream *stream);
static int  interpretCommands(EcCompiler*cp, cchar *cmd);
static int  interpretFiles(EcCompiler *cp, MprList *files, int argc, char **argv, cchar *className, cchar *methodName);
static int  preloadModules(EcCompiler *cp, MprList *modules);

/************************************ Code ************************************/

int main(int argc, char **argv)
{
    Mpr             *mpr;
    EcCompiler      *cp;
    EjsService      *vmService;
    Ejs             *ejs;
    MprList         *useModules, *files;
    cchar           *cmd, *className, *methodName;
    char            *argp, *searchPath, *modules, *name, *tok, *extraFiles, *spec;
    int             nextArg, err, ejsFlags, ecFlags, stats, run, merge, bind, noout, debug, optimizeLevel, nobind, warnLevel;
    int             compilerMode, lang;

    /*
     *  Create the Embedthis Portable Runtime (MPR) and setup a memory failure handler
     */
    mpr = mprCreate(argc, argv, ejsMemoryFailure);
    mprSetAppName(mpr, argv[0], 0, 0);

    if (mprStart(mpr, 0) < 0) {
        mprError(mpr, "Can't start mpr services");
        return EJS_ERR;
    }

    err = 0;
    className = 0;
    cmd = 0;
    methodName = 0;
    searchPath = 0;
    stats = 0;
    run = 1;
    merge = 0;
    bind = 1;
    nobind = 0;
    noout = 1;
    debug = 1;
    warnLevel = 1;
    optimizeLevel = 9;
    compilerMode = PRAGMA_MODE_STANDARD;
    lang = BLD_FEATURE_EJS_LANG;

    useModules = mprCreateList(mpr);
    files = mprCreateList(mpr);

    ejsFlags = EJS_FLAG_COMPILER;

    for (nextArg = 1; nextArg < argc; nextArg++) {
        argp = argv[nextArg];
        if (*argp != '-') {
            break;
        }

        if (strcmp(argp, "--bind") == 0) {
            /* Ignore. Not required and only here for compatibility with ec */

        } else if (strcmp(argp, "--class") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                className = argv[++nextArg];
            }

        } else if (strcmp(argp, "--cmd") == 0 || strcmp(argp, "-c") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                cmd = argv[++nextArg];
            }

        } else if (strcmp(argp, "--debug") == 0) {
            debug = 1;

        } else if (strcmp(argp, "--lang") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                spec = argv[++nextArg];
                if (mprStrcmpAnyCase(spec, "ecma") == 0) {
                    lang = EJS_SPEC_ECMA;
                } else if (mprStrcmpAnyCase(spec, "plus") == 0) {
                    lang = EJS_SPEC_PLUS;
                } else if (mprStrcmpAnyCase(spec, "fixed") == 0) {
                    lang = EJS_SPEC_FIXED;
                }
            }

        } else if (strcmp(argp, "--files") == 0 || strcmp(argp, "-f") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                extraFiles = mprStrdup(mpr, argv[++nextArg]);
                name = mprStrTok(extraFiles, " \t", &tok);
                while (name != NULL) {
                    mprAddItem(files, name);
                    name = mprStrTok(NULL, " \t", &tok);
                }
            }

        } else if (strcmp(argp, "--log") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                ejsStartLogging(mpr, argv[++nextArg]);
            }

        } else if (strcmp(argp, "--method") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                methodName = argv[++nextArg];
            }

        } else if (strcmp(argp, "--nobind") == 0) {
            /*
             *  This is a hidden switch just for the compiler developers
             */
            nobind = 1;
            bind = 0;

        } else if (strcmp(argp, "--nodebug") == 0) {
            debug = 0;

        } else if (strcmp(argp, "--optimize") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                optimizeLevel = atoi(argv[++nextArg]);
            }

        } else if (strcmp(argp, "--searchpath") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                searchPath = argv[++nextArg];
            }

        } else if (strcmp(argp, "--standard") == 0) {
            compilerMode = PRAGMA_MODE_STANDARD;

        } else if (strcmp(argp, "--stats") == 0) {
            stats = 1;

        } else if (strcmp(argp, "--strict") == 0) {
            compilerMode = PRAGMA_MODE_STRICT;

        } else if (strcmp(argp, "--use") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                modules = mprStrdup(mpr, argv[++nextArg]);
                name = mprStrTok(modules, " \t", &tok);
                while (name != NULL) {
                    mprAddItem(useModules, name);
                    name = mprStrTok(NULL, " \t", &tok);
                }
            }

        } else if (strcmp(argp, "--version") == 0 || strcmp(argp, "-V") == 0) {
            mprErrorPrintf(mpr, "%s %s\n"
                "Copyright (C) Embedthis Software 2003-2009\n"
                "Copyright (C) Michael O'Brien 2003-2009\n",
               BLD_NAME, BLD_VERSION);
            exit(0);

        } else if (strcmp(argp, "--warn") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                warnLevel = atoi(argv[++nextArg]);
            }

        } else {
            err++;
            break;
        }
    }

    if (err) {
        /*
         *  If --method or --class is specified, then the named class.method will be run (method defaults to "main", class
         *  defaults to first class with a "main").
         *
         *  Examples:
         *      ejs
         *      ejs script.es arg1 arg2 arg3
         *      ejs --class "Customer" --method "start" --files "script1.es script2.es" main.es
         */
        mprErrorPrintf(mpr,
            "Usage: %s [options] script.es [arguments] ...\n"
            "  Ejscript shell program options:\n"
            "  --class className        # Name of class containing method to run\n"
            "  --cmd ejscriptCode       # Literal ejscript statements to execute\n"
            "  --debug                  # Include symbolic debugging information in output\n"
            "  --lang                   # Language compliance (ecma|plus|fixed)\n"
            "  --files \"files..\"        # Extra source to compile\n"
            "  --log logSpec            # Internal compiler diagnostics logging\n"
            "  --method methodName      # Name of method to run. Defaults to main\n"
            "  --nodebug                # Omit symbolic debugging information in output\n"
            "  --optimize level         # Set the optimization level (0-9 default is 9)\n"
            "  --searchpath ejsPath     # Module search path\n"
            "  --standard               # Default compilation mode to standard (default)\n"
            "  --stats                  # Print stats on exit\n"
            "  --strict                 # Default compilation mode to strict\n"
            "  --use 'module, ...'      # List of modules to pre-load\n"
            "  --version                # Emit the compiler version information\n"
            "  --warn level             # Set the warning message level (0-9 default is 0)\n\n",
            mpr->name);
        return -1;
    }

    vmService = ejsCreateService(mpr);
    if (vmService == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    if (searchPath) {
        ejsSetSearchPath(vmService, searchPath);
    }

    ejs = ejsCreate(vmService, 0, ejsFlags);
    if (ejs == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    ecFlags = 0;
    ecFlags |= (run) ? EC_FLAGS_RUN: 0;
    ecFlags |= (merge) ? EC_FLAGS_MERGE: 0;
    ecFlags |= (bind) ? EC_FLAGS_BIND_GLOBALS: 0;
    ecFlags |= (nobind) ? EC_FLAGS_NO_BIND: 0;
    ecFlags |= (noout) ? EC_FLAGS_NO_OUT: 0;
    ecFlags |= (debug) ? EC_FLAGS_DEBUG: 0;

    cp = ecCreateCompiler(ejs, ecFlags, lang);
    if (cp == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    ecSetOptimizeLevel(cp, optimizeLevel);
    ecSetWarnLevel(cp, warnLevel);
    ecSetDefaultMode(cp, compilerMode);

    if (preloadModules(cp, useModules) < 0) {
        return EJS_ERR;
    }

    if (cmd) {
        if (interpretCommands(cp, cmd) < 0) {
            err++;
        }

    } else if (nextArg < argc) {
        name = argv[nextArg];
        mprAddItem(files, argv[nextArg]);
        if (interpretFiles(cp, files, argc - nextArg, &argv[nextArg], className, methodName) < 0) {
            err++;
        }

    } else {
        /*
         *  No args - run as an interactive shell
         */
        if (interpretCommands(cp, NULL) < 0) {
            err++;
        }
    }

#if BLD_DEBUG
    if (stats) {
        mprSetLogLevel(ejs, 1);
        ejsPrintAllocReport(ejs);
    }
#endif

    mprFree(mpr);
    return err;
}


/*
 *  Compile the source files supplied on the command line. This will compile in-memory and optionally also save to 
 *  module files.
 */
static int interpretFiles(EcCompiler *cp, MprList *files, int argc, char **argv, cchar *className, cchar *methodName)
{
    Ejs     *ejs;

    mprAssert(files);

    ejs = cp->ejs;
    ejs->argc = argc;
    ejs->argv = argv;

    if (ecCompile(cp, files->length, (char**) files->items, 0) < 0) {
        return EJS_ERR;
    }

    if (cp->run && cp->errorCount == 0) {
        ejs->flags &= ~EJS_FLAG_COMPILER;
        ejsEnableGC(ejs, 1);
        ejsCollectGarbage(ejs, EJS_GC_ALL);
        if (ejsRunProgram(ejs, className, methodName) < 0) {
            ejsReportError(ejs, "Error in program");
            return EJS_ERR;
        }
    }
    return 0;
}


/*
 *  Interpret from the console or from a literal command
 */
static int interpretCommands(EcCompiler *cp, cchar *cmd)
{
    Ejs         *ejs;
    EjsString   *result;
    char        *tmpArgv[1];
    int         err;

    ejs = cp->ejs;

    if (ecOpenConsoleStream(cp->lexer, (cmd) ? commandGets: consoleGets) < 0) {
        mprError(cp, "Can't open input");
        return EJS_ERR;
    }
    if (cmd) {
        cp->lexer->input->stream->buf = mprStrdup(cp, cmd);
    } else {
        cp->interactive = 1;
    }
    cp->input = cp->lexer->input;
    cp->token = cp->lexer->input->token;

    ecResetInput(cp);
    tmpArgv[0] = EC_INPUT_STREAM;

    while (!cp->lexer->input->stream->eof) {
        ejs->flags |= EJS_FLAG_COMPILER;
        err = 0;
        cp->uid = 0;
        if (ecCompile(cp, 1, tmpArgv, 0) < 0) {
            ejs->result = ejs->undefinedValue;
            err++;
        }
        if (cp->run && !err && cp->errorCount == 0) {
            ejs->flags &= ~EJS_FLAG_COMPILER;
            ejsCollectGarbage(ejs, 1);
            ejsRun(ejs);
        }

        if (ejs->result != ejs->undefinedValue) {
            result = (EjsString*) ejsCastVar(ejs, ejs->result, ejs->stringType);
            mprPrintf(cp, "%s\n", result->value);
        }
        ecResetInput(cp);
        err = 0;
        cp->errorCount = 0;
    }
    ecCloseStream(cp->lexer);

    return 0;
}


/*
 *  Read input from the console (stdin)
 */
static int consoleGets(EcStream *stream)
{
    char        buf[MPR_MAX_STRING];
    int         len;

    if (stream->flags & EC_STREAM_EOL) {
        return 0;
    }

    buf[sizeof(buf) - 1] = '\0';

    /*
     *  Prompt for input with the level of current nest (block nest depth)
     */
    mprPrintf(stream, "%s-%d> ", EJS_NAME, stream->compiler->state->blockNestCount);
    if (fgets(buf, sizeof(buf) - 1, stdin) == 0) {
        stream->eof = 1;
        mprPrintf(stream, "\n");
        return -1;
    }

    len = (int) strlen(buf);
    while (buf[len-1] == '\r') {
        len--;
    }
    while (buf[len-1] == '\n') {
        len--;
    }
    buf[len] = '\0';

    stream->buf = mprStrdup(stream, buf);
    stream->nextChar = stream->buf;
    stream->end = &stream->buf[len];
    stream->currentLine = stream->buf;
    stream->lineNumber = 1;

    stream->flags |= EC_STREAM_EOL;

    return len;
}


/*
 *  Read input from a literal command
 */
static int commandGets(EcStream *stream)
{
    /*
     *  We only get to execute one string of commands. So we only come here once. Second time round, nextChar will be set.
     */
    if (stream->nextChar) {
        stream->eof = 1;
        return -1;
    }

    stream->nextChar = stream->buf;
    stream->end = &stream->buf[strlen(stream->buf)];
    stream->currentLine = stream->buf;
    stream->lineNumber = 1;

    return (int) strlen(stream->buf);
}



static int preloadModules(EcCompiler *cp, MprList *modules)
{
    cchar   *name;
    int     next;

    for (next = 0; modules && (name = (cchar*) mprGetNextItem(modules, &next)) != 0; ) {
        if (ejsLoadModule(cp->ejs, name, NULL, NULL, EJS_MODULE_DONT_INIT) < 0) {
            return EJS_ERR;
        }
    }
    cp->useModules = modules;
    return 0;
}


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
