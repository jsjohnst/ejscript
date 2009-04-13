/**
 *  ejsWebResponse.c - Web framework Response class.
 *
 *  The Response properties are "virtual" in that they are created lazily and don't really exist in this class. Rather, 
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
static EjsVar *getResponseProperty(Ejs *ejs, EjsWebResponse *rq, int slotNum)
{
    return ejsGetWebVar(ejs, EJS_WEB_RESPONSE_VAR, slotNum);
}


static int getResponsePropertyCount(Ejs *ejs, EjsWebResponse *rq)
{
    return ES_ejs_web_Response_NUM_INSTANCE_PROP;
}


static EjsName getResponsePropertyName(Ejs *ejs, EjsWebResponse *rq, int slotNum)
{
    return ejsGetPropertyName(ejs, (EjsVar*) rq->var.type->instanceBlock, slotNum);
}

/*
 *  Lookup a property by name.
 */
static int lookupResponseProperty(struct Ejs *ejs, EjsWebResponse *rq, EjsName *qname)
{
    return ejsLookupProperty(ejs, (EjsVar*) rq->var.type->instanceBlock, qname);
}


/*
 *  Update a property's value.
 */
static int setResponseProperty(struct Ejs *ejs, EjsWebResponse *ap, int slotNum,  EjsVar *value)
{
    return ejsSetWebVar(ejs, EJS_WEB_RESPONSE_VAR, slotNum, value);
}


/*********************************** Factory **********************************/

EjsWebResponse *ejsCreateWebResponseObject(Ejs *ejs, void *handle)
{
    EjsWebResponse  *vp;
    EjsType         *requestType;
    EjsName         qname;

    requestType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Response"));
    vp = (EjsWebResponse*) ejsCreateVar(ejs, requestType, 0);

    ejsSetDebugName(vp, "EjsWeb Response Instance");

    return vp;
}


void ejsConfigureWebResponseType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Response"));
    if (type == 0) {
        if (!(ejs->flags & EJS_FLAG_EMPTY)) {
            mprError(ejs, "Can't find web Response class");
            ejs->hasError = 1;
        }
        return;
    }
    type->instanceSize = sizeof(EjsWebResponse);
    type->hasObject = 0;

    /*
     *  Re-define the helper functions.
     */
    *type->helpers = *ejs->defaultHelpers;
    type->helpers->getProperty = (EjsGetPropertyHelper) getResponseProperty;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getResponsePropertyCount;
    type->helpers->getPropertyName = (EjsGetPropertyNameHelper) getResponsePropertyName;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupResponseProperty;
    type->helpers->setProperty = (EjsSetPropertyHelper) setResponseProperty;
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
