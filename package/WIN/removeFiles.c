/** 
 *  remove.c - Remove files safely on Windows
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/
/*
 *  Suppress MS VS warnings
 */
#define _CRT_SECURE_NO_WARNINGS

#include    "buildConfig.h"

#if WIN
#include    <stdio.h>
#include    <direct.h>
#include    <windows.h>

/*********************************** Locals ***********************************/

#define PROGRAM         BLD_NAME " Removal Program"
#define MPR_MAX_FNAME   1024

static char *fileList[] = {
    "appWeb.conf",
    "*.obj",
    "*.lib",
    "*.dll",
    "*.pdb",
    "*.exe",
    "*.def",
    "*.exp",
    "*.idb",
    "*.plg",
    "*.res",
    "*.ncb",
    "*.opt",
    "*.bak",
    "make.dep",
    ".port.log",
    0
};

/***************************** Forward Declarations ***************************/

static int      initWindow();
static void     cleanup();
static void     recursiveRemove(char *dir, char *pattern);
static int      match(char *file, char *pat);
static char     *mprGetDirName(char *buf, int bufsize, const char *path);
static int      mprStrcpy(char *dest, int destMax, const char *src);

/*********************************** Code *************************************/

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE junk, char *args, int junk2)
{
    char    dir[MPR_MAX_FNAME], moduleBuf[MPR_MAX_FNAME];
    char    *cp;
    int     errflg, sleepMsecs, removeOk;

    errflg = 0;
    sleepMsecs = 0;
    removeOk = 0;

    GetModuleFileName(0, moduleBuf, sizeof(moduleBuf) - 1);
    mprGetDirName(dir, sizeof(dir), moduleBuf);
    _chdir(dir);

    if (args && *args) {
        if (strstr(args, "-r") != 0) {
            removeOk++;
        }
        if ((cp = strstr(args, "-s")) != 0) {
            do {
                cp++;
            } while (isspace(*cp));
            sleepMsecs = atoi(cp) * 1000;
        }
    }

    /*
     *  We use removeOk to ensure that someone just running the program won't 
     *  do anything bad.
     */
    if (errflg || !removeOk) {
        fprintf(stderr, "Bad Usage");
        return FALSE;
    }   

    cleanup();

    /*
     *  Some products (services) take a while to exit. This is a convenient
     *  way to pause before removing
     */
    if (sleepMsecs) {
        printf("sleeping for %d msec\n", sleepMsecs);
        Sleep(sleepMsecs);
    }

    return 0;
}


/*
 *  Cleanup temporary files
 */
static void cleanup()
{
    char    *file;
    char    home[MPR_MAX_FNAME];
    int     i;

    _getcwd(home, sizeof(home) - 1);

    for (i = 0; fileList[i]; i++) {
        file = fileList[i];
        recursiveRemove(home, file);
    }
}


/*
 *  Remove a file
 */
static void recursiveRemove(char *dir, char *pattern)
{
    HANDLE          handle;
    WIN32_FIND_DATA data;
    char            saveDir[MPR_MAX_FNAME];

    saveDir[sizeof(saveDir) - 1] = '\0';
    _getcwd(saveDir, sizeof(saveDir) - 1);

    _chdir(dir);
    handle = FindFirstFile("*.*", &data);

    while (FindNextFile(handle, &data)) {
        if (strcmp(data.cFileName, "..") == 0 || 
            strcmp(data.cFileName, ".") == 0) {
            continue;
        }
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            recursiveRemove(data.cFileName, pattern);
            /*
             *  This will fail if there are files remaining in the directory.
             */
            printf("Removing directory %s\n", data.cFileName);
            RemoveDirectory(data.cFileName);
            continue;
        }
        if (match(data.cFileName, pattern)) {
            printf("Delete: %s\n", data.cFileName);
            DeleteFile(data.cFileName);
        }
    }
    FindClose(handle);
    _chdir(saveDir);
}


/*
 *  Simple wild-card matching
 */
static int match(char *file, char *pat)
{
    char    fileBuf[MPR_MAX_FNAME], patBuf[MPR_MAX_FNAME];
    char    *patExt;
    char    *fileExt;

    mprStrcpy(fileBuf, sizeof(fileBuf), file);
    file = fileBuf;
    mprStrcpy(patBuf, sizeof(patBuf), pat);
    pat = patBuf;

    if (strcmp(file, pat) == 0) {
        return 1;
    }
    if ((fileExt = strrchr(file, '.')) != 0) {
        *fileExt++ = '\0';
    }
    if ((patExt = strrchr(pat, '.')) != 0) {
        *patExt++ = '\0';
    }
    if (*pat == '*' || strcmp(pat, file) == 0) {
        if (patExt && *patExt == '*') {
            return 1;
        } else {
            if (fileExt && strcmp(fileExt, patExt) == 0) {
                return 1;
            }
        }
    }
    return 0;
}


/*
 *  Return the directory portion of a pathname into the users buffer.
 */
static char *mprGetDirName(char *buf, int bufsize, const char *path)
{
    char    *cp;
    int     dlen;

    cp = strrchr(path, '/');
    if (cp == 0) {
#if WIN
        cp = strrchr(path, '\\');
        if (cp == 0)
#endif
        {
            buf[0] = '\0';
            return buf;
        }
    }

    if (cp == path && cp[1] == '\0') {
        strcpy(buf, ".");
        return buf;
    }

    dlen = cp - path;
    if (dlen < bufsize) {
        if (dlen == 0) {
            dlen++;
        }
        memcpy(buf, path, dlen);
        buf[dlen] = '\0';
        return buf;
    }
    return 0;
}


static int mprStrcpy(char *dest, int destMax, const char *src)
{
    int     len;

    len = strlen(src);
    if (destMax > 0 && len >= destMax && len > 0) {
        return -1;
    }
    if (len > 0) {
        memcpy(dest, src, len);
        dest[len] = '\0';
    } else {
        *dest = '\0';
        len = 0;
    } 
    return len;
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
