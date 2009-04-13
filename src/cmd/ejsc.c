/**
 *  ejsc.c - Ejscript Compiler main program
 *
 *  This compiler will compile the files given on the command line.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"
#include    "ecCompiler.h"

/********************************** Forwards **********************************/

static int preloadModules(EcCompiler *cp, MprList *modules);

/************************************ Code ************************************/

int main(int argc, char **argv)
{
    Mpr             *mpr;
    Ejs             *ejs;
    EcCompiler      *cp;
    EjsService      *vmService;
    MprList         *useModules;
    char            *argp, *searchPath, *outputFile, *certFile, *name, *tok, *modules, *spec, *order;
    int             nextArg, err, ejsFlags, ecFlags, bind, debug, doc, empty, merge, nobind, endian;
    int             warnLevel, noout, parseOnly, tabWidth, optimizeLevel, compilerMode, strip, lang;

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
    searchPath = 0;
    ejsFlags = EJS_FLAG_COMPILER | EJS_FLAG_NO_EXE;
    compilerMode = PRAGMA_MODE_STANDARD;
    certFile = 0;
    ecFlags = 0;

    bind = 0;
    debug = 0;
    doc = 0;
    empty = 0;
    merge = 0;
    nobind = 0;
    noout = 0;
    parseOnly = 0;
    tabWidth = 4;
    warnLevel = 1;
    outputFile = 0;
    optimizeLevel = 9;
    lang = BLD_FEATURE_EJS_LANG;

    endian = mprGetEndian(mpr);
    useModules = mprCreateList(mpr);

    for (nextArg = 1; nextArg < argc; nextArg++) {
        argp = argv[nextArg];
        if (*argp != '-') {
            break;
        }
        if (strcmp(argp, "--bind") == 0) {
            bind = 1;

        } else if (strcmp(argp, "--cross") == 0) {
            endian = BLD_HOST_ENDIAN;

        } else if (strcmp(argp, "--debug") == 0) {
            debug = 1;

        } else if (strcmp(argp, "--doc") == 0) {
            doc = 1;

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

        } else if (strcmp(argp, "--empty") == 0) {
            empty = 1;

        } else if (strcmp(argp, "--endian") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                order = argv[++nextArg];
                if (mprStrcmpAnyCase(order, "big") == 0) {
                    endian = MPR_BIG_ENDIAN;
                } else if (mprStrcmpAnyCase(order, "little") == 0) {
                    endian = MPR_LITTLE_ENDIAN;
                } else {
                    err++;
                }
            }

        } else if (strcmp(argp, "--log") == 0) {
            /*
             *  Undocumented logging switch
             */
            if (nextArg >= argc) {
                err++;
            } else {
                ejsStartLogging(mpr, argv[++nextArg]);
            }

        } else if (strcmp(argp, "--merge") == 0) {
            merge = 1;

        } else if (strcmp(argp, "--nobind") == 0) {
            /*
             *  This is a hidden switch just for the compiler developers
             */
            nobind = 1;

        } else if (strcmp(argp, "--noout") == 0) {
            noout = 1;

        } else if (strcmp(argp, "--standard") == 0) {
            compilerMode = PRAGMA_MODE_STANDARD;

        } else if (strcmp(argp, "--strict") == 0) {
            compilerMode = PRAGMA_MODE_STRICT;

        } else if (strcmp(argp, "--optimize") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                optimizeLevel = atoi(argv[++nextArg]);
            }

        } else if (strcmp(argp, "--out") == 0) {
            /*
             *  Create a single output module file containing all modules
             */
            if (nextArg >= argc) {
                err++;
            } else {
                outputFile = argv[++nextArg];
            }

        } else if (strcmp(argp, "--parse") == 0) {
            parseOnly = 1;

        } else if (strcmp(argp, "--searchpath") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                searchPath = argv[++nextArg];
            }

        } else if (strcmp(argp, "--sign") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                certFile = argv[++nextArg];
            }

        } else if (strcmp(argp, "--strip") == 0) {
            strip = 1;

        } else if (strcmp(argp, "--tabWidth") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                tabWidth = atoi(argv[++nextArg]);
            }

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

#if BLD_FEATURE_EJS_WEB
        } else if (strcmp(argp, "--web") == 0) {
#if BLD_FEATURE_EJS_DB
            mprAddItem(useModules, "ejs.db");
#endif
            mprAddItem(useModules, "ejs.web");
#endif

        } else {
            err++;
            break;
        }
    }

    if (noout || merge) {
        bind = 1;
    }
    if (nobind) {
        bind = 0;
    }

    if (outputFile && noout) {
        mprErrorPrintf(mpr, "Can't use --out and --noout\n");
        err++;
    }

    if (argc == nextArg) {
        err++;
    }

    if (err) {
        /*
         *  Usage Examples:
         *      ejsc Person.es User.es Customer.es
         *      ejsc --out group.mod Person.es User.es Customer.es
         *      ejsc --out group.mod Person.es User.es Customer.es
         */
        mprErrorPrintf(mpr,
            "Usage: %s [options] files...\n"
            "  Ejscript compiler options:\n"
            "  --bind               # Bind global properties to slots. Requires --out.\n"
            "  --cross              # Cross compilation. Will use host system byte ordering.\n"
            "  --debug              # Include symbolic debugging information in output\n"
            "  --doc                # Include documentation strings in output\n"
            "  --lang               # Language compliance (ecma|plus|fixed)\n"
            "  --empty              # Create empty interpreter\n"
            "  --endian big|little  # Set the target byte order\n"
            "  --merge              # Merge dependent input modules into the output\n"
            "  --noout              # Do not generate any output\n"
            "  --optimize level     # Set optimization level (0-9)\n"
            "  --out filename       # Name a single output module (default: \"default.mod\")\n"
            "  --parse              # Just parse source. No output\n"
            "  --searchpath ejsPath # Module search path\n"
            "  --standard           # Default compilation mode to standard (default)\n"
            "  --strict             # Default compilation mode to strict\n"
#if FUTURE
            "  --sign certFile      # Sign the module file (not implemented) \n"
            "  --strip              # Strip all symbolic names (Can't import)\n"
            "  --tabwidth           # Tab width for '^' error reporting\n"
#endif
            "  --use 'module, ...'  # List of modules to pre-load\n"
            "  --version            # Emit the compiler version information\n"
            "  --warn level         # Set the warning message level (0-9)\n\n",
            mpr->name);
        return -1;
    }

    /*
     *  Need an interpreter when compiling
     */
    vmService = ejsCreateService(mpr);
    if (vmService == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    if (searchPath) {
        ejsSetSearchPath(vmService, searchPath);
    }

    if (empty) {
        ejsFlags |= EJS_FLAG_EMPTY;
    }
    if (doc) {
        ejsFlags |= EJS_FLAG_DOC;
    }

    ejs = ejsCreate(vmService, NULL, ejsFlags);
    if (ejs == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    ecFlags = 0;
    ecFlags |= (bind) ? EC_FLAGS_BIND_GLOBALS: 0;
    ecFlags |= (debug) ? EC_FLAGS_DEBUG: 0;
    ecFlags |= (empty) ? EC_FLAGS_EMPTY: 0;
    ecFlags |= (merge) ? EC_FLAGS_MERGE: 0;
    ecFlags |= (nobind) ? EC_FLAGS_NO_BIND: 0;
    ecFlags |= (noout) ? EC_FLAGS_NO_OUT: 0;
    ecFlags |= (parseOnly) ? EC_FLAGS_PARSE_ONLY: 0;

    cp = ecCreateCompiler(ejs, ecFlags, lang);
    if (cp == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    ecSetHostEndian(cp, endian);
    ecSetOptimizeLevel(cp, optimizeLevel);
    ecSetWarnLevel(cp, warnLevel);
    ecSetDefaultMode(cp, compilerMode);
    ecSetTabWidth(cp, tabWidth);
    ecSetOutputFile(cp, outputFile);
    ecSetCertFile(cp, certFile);

    if (preloadModules(cp, useModules) < 0) {
        return EJS_ERR;
    }

    if (nextArg < argc) {
        /*
         *  Compile the source files supplied on the command line. This will compile in-memory and
         *  optionally also save to module files.
         */
        if (ecCompile(cp, argc - nextArg, &argv[nextArg], 0) < 0) {
            err++;
        }
    }

    if (cp->errorCount > 0) {
        err++;
    }

    mprFree(mpr);
    return err;
}


/*
 *  This can't be put into ejsCommon because all-in-one builds don't include ec.h which is required by this function.
 */
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
