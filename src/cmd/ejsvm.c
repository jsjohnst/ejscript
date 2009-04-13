/**
 *  ejsvm.c - C Language VM to interpret Ejscript programs.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"
 
/****************************** Forward Declarations **************************/

static int  loadModuleFile(Ejs *ejs, char *fileName);

/************************************ Main ************************************/

int main(int argc, char *argv[]) 
{
    Mpr             *mpr;
    EjsService      *ejsService;
    Ejs             *ejs; 
    cchar           *programName;
    char            *argp, *className, *methodName, **modules, *extraModules, *module;
    int             i, flags, nextArg, err, count;

    err = 0;
    className = 0;
    methodName = 0;
    extraModules = 0;
    flags = 0;

    /*
     *  Create the Embedthis Portable Runtime (MPR) and setup a memory failure handler
     */
    mpr = mprCreate(argc, argv, ejsMemoryFailure);
    mprSetAppName(mpr, argv[0], 0, 0);

    if (mprStart(mpr, 0) < 0) {
        mprError(mpr, "Can't start mpr services");
        return EJS_ERR;
    }
    
    programName = mprGetBaseName(argv[0]);

    ejsService = ejsCreateService(mpr);
    if (ejsService == 0) {
        mprError(mpr, "Can't initialize the Ejscript service.");
        return -1;
    }

    for (nextArg = 1; nextArg < argc; nextArg++) {
        argp = argv[nextArg];
        if (*argp != '-') {
            break;
        }
        if (strcmp(argp, "--class") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                className = argv[++nextArg];
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

        } else if (strcmp(argp, "--modules") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                extraModules = argv[++nextArg];
            }

        } else if (strcmp(argp, "--searchpath") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                ejsSetSearchPath(ejsService, argv[++nextArg]);
            }

        } else if (strcmp(argp, "--version") == 0 || strcmp(argp, "-V") == 0) {
            mprErrorPrintf(mpr, "%s %s\n"
                "Copyright (C) Embedthis Software 2003-2009\n"
                "Copyright (C) Michael O'Brien 2003-2009\n",
               BLD_NAME, BLD_VERSION);  
            exit(0);

        } else {
            err++;
            break;
        }
    }

    if (nextArg == argc) {
        err++;
    }

    if (err) {
        mprErrorPrintf(mpr, 
            "Usage: %s [options] [module args] ...\n"
            "  Ejscript virtual machine options:\n"
            "  --class className         # Name of class containing method to run\n"
            "                            # Default: first class with main method\n"
            "  --modules \"modules...\"    # Extra modules to load\n"
            "  --log logSpec             # Internal VM diagnostics logging\n"
            "  --method methodName       # Name of method to run. Default: main\n"
            "  --searchpath ejsPath      # Module search path\n"
            "  --version                 # Emit the compiler version information\n\n", programName);
        return -1;
    }

    ejs = ejsCreate(ejsService, NULL, flags);
    if (ejs == 0) {
        mprError(mpr, "Can't create the EJSCript interpreter");
        mprFree(mpr);
        exit(-1);
    }
    
    if (extraModules) {
        mprMakeArgv(ejs, 0, extraModules, &count, &modules);
        for (i = 0; i < count && !err; i++) {
            if (loadModuleFile(ejs, modules[i]) < 0) {
                err++;
            }
        }
    }

    /*
     *  Load the module supplied on the command line. 
     */
    module = argv[nextArg];

    //  TODO - should have an API for this
    ejs->argc = argc - nextArg;
    ejs->argv = &argv[nextArg];

    if (loadModuleFile(ejs, module) < 0) {
        err++;
    }

    if (!err) {
        if (ejsRunProgram(ejs, className, methodName) < 0) {
            err++;
        }
    }

    mprFree(mpr);
    return err;
}



/*
 *  Load a module file
 */
static int loadModuleFile(Ejs *ejs, char *fileName)
{
    int         len;

    /*
     *  Strip trailing ".mod" if present
     */
    fileName = mprStrdup(ejs, fileName);
    len = strlen(fileName);
    if (len > 4 && strcmp(&fileName[len - 4], EJS_MODULE_EXT) == 0) {
        fileName[len - 4] = '\0';
    }

    if (ejsLoadModule(ejs, fileName, NULL, NULL, 0) == 0) {
        ejsReportError(ejs, "Can't load module \"%s\"", fileName);
        mprFree(fileName);
        return EJS_ERR;
    }

    mprFree(fileName);
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
