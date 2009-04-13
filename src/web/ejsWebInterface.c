/*
 *  ejsWebInterface.c - Wrappers for the web framework control block functions.
 *
 *  These functions provide the API for web servers to invoke.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */
/********************************** Includes **********************************/

#include    "ejs.h"

/************************************ Code ************************************/

void ejsDefineParams(Ejs *ejs)
{
    EjsWeb         *web;

    mprAssert(ejs);
    web = ejsGetHandle(ejs);

    if (web->control->defineParams) {
        web->control->defineParams(web->handle);
    }
}


/*
 *  Discard all generated output
 */
void ejsDiscardOutput(Ejs *ejs)
{
    EjsWeb         *web;

    mprAssert(ejs);
    web = ejsGetHandle(ejs);

    if (web->control->discardOutput) {
        web->control->discardOutput(web->handle);
    }
}


/*
 *  Return an error to the client
 */
void ejsWebError(Ejs *ejs, int code, cchar *fmt, ...)
{
    EjsWeb      *web;
    va_list     args;
    char        *buf;

    mprAssert(ejs);
    mprAssert(fmt);

    va_start(args, fmt);
    mprAllocVsprintf(ejs, &buf, -1, fmt, args);
    web = ejsGetHandle(ejs);

    if (web->control->error) {
        web->control->error(web->handle, code, buf);
    }
    mprFree(buf);
    va_end(args);
}


/*
 *  Get a HTTP request header
 */
cchar *ejsGetHeader(Ejs *ejs, cchar *key)
{
    EjsWeb     *web;

    mprAssert(ejs);
    web = ejsGetHandle(ejs);
    mprAssert(web->control->getHeader);
    return web->control->getHeader(web->handle, key);
}


/*
 *  Get a variable from the web server. This is used to implement virtual properties
 */
EjsVar *ejsGetWebVar(Ejs *ejs, int collection, int field)
{
    EjsWeb     *web;

    mprAssert(ejs);
    web = ejsGetHandle(ejs);
    mprAssert(web->control->getVar);
    return web->control->getVar(web->handle, collection, field);
}


#if UNUSED
int ejsMapToStorage(Ejs *ejs, char *path, int pathsize, cchar *url)
{
    EjsWeb     *web;

    //  TODO - include files are not required anymore
    mprAssert(0);
    mprAssert(ejs);
    web = ejsGetHandle(ejs);
    mprAssert(web->control->mapToStorage);
    return web->control->mapToStorage(web->handle, path, pathsize, url);
}


int ejsReadFile(Ejs *ejs, char **buf, int *len, cchar *path)
{
    EjsWeb     *web;

    //  TODO - what is this for?
    mprAssert(0);
    mprAssert(ejs);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->readFile);
    return web->control->readFile(web->handle, buf, len, path);
}
#endif


void ejsRedirect(Ejs *ejs, int code, cchar *url)
{
    EjsWeb     *web;

    mprAssert(ejs);
    mprAssert(url);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->redirect);
    web->control->redirect(web->handle, code, url);
}


void ejsSetCookie(Ejs *ejs, cchar *name, cchar *value, int lifetime, cchar *path, bool secure)
{
    EjsWeb     *web;

    mprAssert(ejs);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->setCookie);
    web->control->setCookie(web->handle, name, value, lifetime, path, secure);
}


void ejsSetWebHeader(Ejs *ejs, bool allowMultiple, cchar *key, cchar *fmt, ...)
{
    EjsWeb     *web;
    char            *value;
    va_list         vargs;

    mprAssert(ejs);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->setHeader);

    va_start(vargs, fmt);
    mprAllocVsprintf(web, &value, -1, fmt, vargs);

    web->control->setHeader(web->handle, allowMultiple, key, value);
}


void ejsSetHttpCode(Ejs *ejs, int code)
{
    EjsWeb     *web;

    mprAssert(ejs);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->setHttpCode);
    web->control->setHttpCode(web->handle, code);
}


/*
 *  Set a variable in the web server. This is used to implement virtual properties
 */
int ejsSetWebVar(Ejs *ejs, int collection, int field, EjsVar *value)
{
    EjsWeb     *web;

    mprAssert(ejs);

    web = ejsGetHandle(ejs);
    if (web->control->setVar == 0) {
        ejsThrowReferenceError(ejs, "Object is read-only");
        return EJS_ERR;
    }
    mprAssert(web->control->setVar);
    return web->control->setVar(web->handle, collection, field, value);
}


int ejsWriteBlock(Ejs *ejs, cchar *buf, int size)
{
    EjsWeb     *web;

    mprAssert(ejs);
    mprAssert(buf);
    mprAssert(size >= 0);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->write);
    return web->control->write(web->handle, buf, size);
}


int ejsWriteString(Ejs *ejs, cchar *buf)
{
    EjsWeb     *web;

    mprAssert(ejs);
    mprAssert(buf);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->write);
    return web->control->write(web->handle, buf, (int) strlen(buf));
}


int ejsWrite(Ejs *ejs, cchar *fmt, ...)
{
    EjsWeb     *web;
    va_list     args;
    char        *buf;
    int         rc, len;

    mprAssert(ejs);
    mprAssert(fmt);

    web = ejsGetHandle(ejs);
    mprAssert(web->control->write);

    va_start(args, fmt);
    len = mprAllocVsprintf(web, &buf, -1, fmt, args);
    rc = web->control->write(web->handle, buf, len);
    mprFree(buf);
    va_end(args);

    return rc;
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
