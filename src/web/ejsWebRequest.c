/**
 *  ejsWebRequest.c - Native code for the Request class.
 *
 *  The Request properties are "virtual" in that they are created lazily and don't really exist in this class. Rather, 
 *  they are accessed from the web server as required.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if BLD_FEATURE_EJS_WEB

/************************************* Code ***********************************/
/*
 *  Lookup a property. These properties are virtualized.
 */
static EjsVar *getRequestProperty(Ejs *ejs, EjsWebRequest *rq, int slotNum)
{
    return ejsGetWebVar(ejs, EJS_WEB_REQUEST_VAR, slotNum);
}


static int getRequestPropertyCount(Ejs *ejs, EjsWebRequest *rq)
{
    return ES_ejs_web_Request_NUM_INSTANCE_PROP;
}


static EjsName getRequestPropertyName(Ejs *ejs, EjsWebRequest *rq, int slotNum)
{
    return ejsGetPropertyName(ejs, (EjsVar*) rq->var.type->instanceBlock, slotNum);
}


static int lookupRequestProperty(Ejs *ejs, EjsWebRequest *rq, EjsName *qname)
{
    return ejsLookupProperty(ejs, (EjsVar*) rq->var.type->instanceBlock, qname);
}


static int setRequestProperty(Ejs *ejs, EjsWebRequest *rq, int slotNum,  EjsVar *value)
{
    ejsThrowReferenceError(ejs, "Property is readonly");
    return 0;
}


/*********************************** Factory **********************************/

EjsWebRequest *ejsCreateWebRequestObject(Ejs *ejs, void *handle)
{
    EjsWebRequest   *vp;
    EjsType         *requestType;
    EjsName         qname;

    requestType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Request"));
    vp = (EjsWebRequest*) ejsCreateVar(ejs, requestType, 0);
    ejsSetDebugName(vp, "EjsWeb Request Instance");

    return vp;
}


void ejsConfigureWebRequestType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Request"));
    if (type == 0) {
        if (!(ejs->flags & EJS_FLAG_EMPTY)) {
            mprError(ejs, "Can't find web Request class");
            ejs->hasError = 1;
        }
        return;
    }
    type->instanceSize = sizeof(EjsWebRequest);
    type->hasObject = 0;

    /*
     *  Re-define the helper functions.
     */
    *type->helpers = *ejs->defaultHelpers;
    type->helpers->getProperty = (EjsGetPropertyHelper) getRequestProperty;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getRequestPropertyCount;
    type->helpers->getPropertyName = (EjsGetPropertyNameHelper) getRequestPropertyName;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupRequestProperty;
    type->helpers->setProperty = (EjsSetPropertyHelper) setRequestProperty;
}

#endif /* BLD_FEATURE_EJS_WEB */

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
