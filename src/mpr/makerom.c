/**
 *  makerom.c - Compile source files into C code suitable for embedding in ROM.
 *
 *  Usage: makerom -p prefix -r romName filelist >rom.c
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "mpr.h"

/**************************** Forward Declarations ****************************/

static int  makeFileList(Mpr *mpr, MprList *files, char *fileList);
static void printUsage(Mpr *mpr);
static int  binToC(Mpr *mpr, MprList *files, char *romName, char *prefix);

/*********************************** Code *************************************/
/*
 *  Main program
 */ 

int main(int argc, char **argv)
{
    Mpr         *mpr;
    MprList     *files;
    char        *argp, *prefix, *romName;
    int         nextArg, err;

    mpr = mprCreate(argc, argv, 0);

    err = 0;
    prefix = "";
    romName = "defaultRomFiles";
    files = mprCreateList(mpr);

    for (nextArg = 1; nextArg < argc; nextArg++) {
        argp = argv[nextArg];
        if (*argp != '-') {
            break;
        }

        if (strcmp(argp, "--prefix") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                prefix = argv[++nextArg];
            }

        } else if (strcmp(argp, "--name") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                romName = argv[++nextArg];
            }

        } else if (strcmp(argp, "--files") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                if (makeFileList(mpr, files, argv[++nextArg]) < 0) {
                    exit(2);
                }
            }
        }
    }
    if (nextArg >= argc) {
        err++;
    }

    if (err) {
        printUsage(mpr);
        exit(2);
    }   

    while (nextArg < argc) {
        mprAddItem(files, argv[nextArg++]);
    }

    if (binToC(mpr, files, romName, prefix) < 0) {
        return MPR_ERR;
    }
    return 0;
}

 

static void printUsage(Mpr *mpr)
{
    mprErrorPrintf(mpr, "usage: makerom [options] files... >output.c\n");
    mprErrorPrintf(mpr, "  Makerom options:\n");
    mprErrorPrintf(mpr, "  --files fileList      # List of files to convert\n");
    mprErrorPrintf(mpr, "  --prefix prefix       # File prefix to remove\n");
    mprErrorPrintf(mpr, "  --name structName     # Name of top level C struct\n");
}



/*
 *  Read the "fileList" which should be a file containing a list of filenames, and create an MprList of those filenames
 */
static int makeFileList(Mpr *mpr, MprList *files, char *fileList)
{
    MprFile     *file;
    char        path[MPR_MAX_FNAME];
    char        *p;

    if ((file = mprOpen(mpr, fileList, O_RDONLY | O_TEXT, 0)) == NULL) {
        mprError(mpr, "Can't open file list %s", fileList);
        return MPR_ERR_CANT_OPEN;
    }

    while (mprGets(file, path, sizeof(path)) != NULL) {
        if ((p = strchr(path, '\n')) || (p = strchr(path, '\r'))) {
            *p = '\0';
        }
        if (*path == '\0') {
            continue;
        }
        mprAddItem(files, mprStrdup(files, path));
    }
    return 0;
}


/* 
 *  Encode the files as C code
 */
static int binToC(Mpr *mpr, MprList *files, char *romName, char *prefix)
{
    MprFileInfo     info;
    MprFile         *file;
    char            buf[512];
    char            *filename, *cp, *sl, *p;
    int             next, j, i, len;

    mprPrintf(mpr, "/*\n *  %s -- Compiled Files\n */\n", romName);

    mprPrintf(mpr, "#include \"mpr.h\"\n\n");
    mprPrintf(mpr, "#if BLD_FEATURE_ROMFS\n");

    /*
     *  Open each input file and compile
     */
    for (next = 0; (filename = mprGetNextItem(files, &next)) != 0; ) {
        if (mprGetFileInfo(mpr, filename, &info) == 0 && info.isDir) {
            mprError(mpr, "Skipping directory %s", filename);
            continue;
        } 
        if ((file = mprOpen(mpr, filename, O_RDONLY | O_BINARY, 0666)) < 0) {
            mprError(mpr, "Can't open file %s\n", filename);
            return -1;
        }
        mprPrintf(mpr, "static uchar _file_%d[] = {\n", next);

        while ((len = mprRead(file, buf, sizeof(buf))) > 0) {
            p = buf;
            for (i = 0; i < len; ) {
                mprPrintf(mpr, "    ");
                for (j = 0; p < &buf[len] && j < 16; j++, p++) {
                    mprPrintf(mpr, "%3d,", (unsigned char) *p);
                }
                i += j;
                mprPrintf(mpr, "\n");
            }
        }
        mprPrintf(mpr, "    0 };\n\n");

        mprFree(file);
    }

    /*
     *  Now output the page index
     */ 
    mprPrintf(mpr, "MprRomInode %s[] = {\n", romName);

    for (next = 0; (filename = mprGetNextItem(files, &next)) != 0; ) {
        /*
         *  Replace the prefix with a leading "/"
         */ 
        if (strncmp(filename, prefix, strlen(prefix)) == 0) {
            cp = &filename[strlen(prefix)];
        } else {
            cp = filename;
        }
        while((sl = strchr(filename, '\\')) != NULL) {
            *sl = '/';
        }
        if (*cp == '/') {
            cp++;
        }

        if (*cp == '.' && cp[1] == '\0') {
            cp++;
        }
        if (mprGetFileInfo(mpr, filename, &info) == 0 && info.isDir) {
            mprPrintf(mpr, "    { \"%s\", 0, 0, 0 },\n", cp);
            continue;
        }
        mprPrintf(mpr, "    { \"%s\", _file_%d, %d, %d },\n", cp, next, (int) info.size, next);
    }
    
    mprPrintf(mpr, "    { 0, 0, 0, 0 },\n");
    mprPrintf(mpr, "};\n");

    mprPrintf(mpr, "#endif /* BLD_FEATURE_ROMFS */\n");

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
