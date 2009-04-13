/**
 *  ejsSocket.c - Socket class. This implements TCP/IP v4 and v6 connectivity.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/** THIS CODE IS NOT IMPLEMENTED - just cut and pasted from HTTP */
#if ES_ejs_io_Socket && 0
/**************************** Forward Declarations ****************************/

static EjsVar   *getDateHeader(Ejs *ejs, EjsSocket *sp, cchar *key);
static EjsVar   *getStringHeader(Ejs *ejs, EjsSocket *sp, cchar *key);
static void     prepFormData(Ejs *ejs, EjsSocket *sp, EjsVar *data);
static char     *prepUrl(MprCtx ctx, char *url);
static void     responseCallback(MprSocket *socket, int nbytes);
static int      startRequest(Ejs *ejs, EjsSocket *sp, char *method, int argc, EjsVar **argv);

/************************************ Methods *********************************/
/*
 *  Constructor
 *
 *  function Socket(url: String = null)
 */
static EjsVar *socketConstructor(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    sp->ejs = ejs;
    sp->socket = mprCreateSocket(sp);
    if (sp->socket == 0) {
        ejsThrowMemoryError(ejs);
    }

    if (argc == 1 && argv[0] != ejs->nullValue) {
        sp->url = prepUrl(sp, ejsGetString(argv[0]));
    }
    sp->method = mprStrdup(sp, "GET");
    //  TODO - should have limit here
    sp->content = mprCreateBuf(sp, MPR_SOCKET_BUFSIZE, -1);
    mprSetSocketCallback(sp->socket, responseCallback, sp);

    return (EjsVar*) sp;
}


/*
 *  function get address(): String
 */
EjsVar *address(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function get available(): Number
 */
EjsVar *socketAvailable(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetBufLength(sp->content));
}


/*
 *  function set callback(cb: Function): Void
 */
EjsVar *setSocketCallback(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1);

    sp->callback = (EjsFunction*) argv[0];
    return 0;
}


/*
 *  function close(graceful: Boolean = true): Void
 */
static EjsVar *closeSocket(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    if (sp->socket) {
        mprDisconnectSocket(sp->socket);
    }
    return 0;
}


/*
 *  function connect(): Void
 */
static EjsVar *connectSocket(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    startRequest(ejs, sp, NULL, argc, argv);
    return 0;
}


/**
 *  function get eof(): Boolean
 */
static EjsVar *eof(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function flush(): Void
 */
static EjsVar *flushProc(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/**
 *  function listen(address: String = "", port: Number = 0): Socket
 */
static EjsVar *listenProc(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function get mode(): Number
 */
static EjsVar *getMode(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function set mode(value: Number): Void
 */
static EjsVar *setMode(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function get port(): Number
 */
static EjsVar *port(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number
 */
static EjsVar *readProc(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    EjsByteArray    *ba;
    int             offset, count, len, contentLength;

    count = -1;
    offset = -1;

    ba = (EjsByteArray*) argv[0];
    if (argc > 1) {
        offset = ejsGetInt(argv[1]);
    }
    if (argc > 2) {
        count = ejsGetInt(argv[2]);
    }
    if (count < 0) {
        count = MAXINT;
    }
    
    if (!sp->requestStarted && startRequest(ejs, sp, NULL, 0, NULL) < 0) {
        return 0;
    }

    contentLength = mprGetBufLength(sp->content);
    len = min(contentLength - sp->readOffset, count);
    if (len > 0) {
        if (offset < 0) {
            ejsCopyToByteArray(ejs, ba, ba->writePosition, (char*) mprGetBufStart(sp->content), len);
            ejsSetByteArrayPositions(ba, -1, ba->writePosition + len);

        } else {
            ejsCopyToByteArray(ejs, ba, offset, (char*) mprGetBufEnd(sp->content), len);
        }
        mprAdjustBufStart(sp->content, len);
    }
    return (EjsVar*) ejsCreateNumber(ejs, len);
}


/*
 *  function get remoteAddress(): String
 */
static EjsVar *remoteAddress(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function get room(): Number
 */
static EjsVar *room(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}



/*
 *  function set timeout(value: Number): Void
 */
static EjsVar *timeout(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}



/*
 *  function inputStream(): Stream
 */
static EjsVar *requestStream(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function outputStream(): Stream
 */
static EjsVar *requestStream(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  Write post data to the request stream. Connection must be in async mode by defining a callback.
 *
 *  function write(data: ByteArray): Void
 */
static EjsVar *socketWrite(Ejs *ejs, EjsSocket *sp, int argc, EjsVar **argv)
{
    EjsByteArray    *data;
    EjsNumber       *written;

    if (sp->callback == 0) {
        ejsThrowIOError(ejs, "Callback must be defined to use write");
        return 0;
    }

    if (!sp->requestStarted && startRequest(ejs, sp, NULL, 0, NULL) < 0) {
        return 0;
    }
    mprAssert(sp->socket->request);
    mprAssert(sp->socket->sock);

    data = ejsCreateByteArray(ejs, -1);
    written = ejsWriteToByteArray(ejs, data, 1, &argv[0]);

    if (mprWriteSocketPostData(sp->socket, (char*) data->value, written->value, 1) != written->value) {
        ejsThrowIOError(ejs, "Can't write post data");
    }
    return 0;
}


/*********************************** Support **********************************/
/*
 *  Issue a request
 */
static int startRequest(Ejs *ejs, EjsSocket *sp, char *method, int argc, EjsVar **argv)
{
    int     flags;

    if (argc >= 1 && argv[0] != ejs->nullValue) {
        mprFree(sp->url);
        sp->url = prepUrl(sp, ejsGetString(argv[0]));
    }
                                  
#if BLD_FEATURE_SSL
    if (strncmp(sp->url, "sockets", 5) == 0) {
        if (mprInitSSL(ejs, NULL, NULL, NULL, NULL, NULL, 0) < 0) {
            ejsThrowIOError(ejs, "Can't load SSL provider");
            return 0;
        }
    }
#endif
                                  
    if (method && strcmp(sp->method, method) != 0) {
        mprFree(sp->method);
        sp->method = mprStrdup(sp, method);
    }
    if (sp->method[0] != 'P' || sp->method[1] != 'O') {
        sp->contentLength = 0;
        sp->postData = 0;
    }

    mprFlushBuf(sp->content);

    sp->requestStarted = 1;
    sp->gotResponse = 0;

    /*
     *  Block if a callback has been defined
     */
    flags = (sp->callback) ? MPR_SOCKET_DONT_BLOCK : 0;
    if (mprSocketRequest(sp->socket, sp->method, sp->url, sp->postData, sp->contentLength, flags) < 0) {
        ejsThrowIOError(ejs, "Can't issue request for \"%s\"", sp->url);
        return EJS_ERR;
    }
    return 0;
}



/*
 *  Called by MprSocket on response data and request failure or completion.
 */
static void responseCallback(MprSocket *socket, int nbytes)
{
    Ejs         *ejs;
    EjsSocket     *hp;
    EjsObject   *event;
    EjsType     *eventType;
    EjsName     qname;
    EjsVar      *arg;

    hp = socket->callbackArg;
    sp->gotResponse = 1;
    
    if (socket->response && nbytes > 0) {
        mprResetBufIfEmpty(sp->content);        
        mprPutBlockToBuf(sp->content, mprGetBufStart(socket->response->content), nbytes);
    }
    
    if (sp->callback) {
        ejs = sp->ejs;
        if (socket->error) {
            /*
             *  Some kind of error
             */
            eventType = ejsGetType(ejs, ES_ejs_io_SocketError);
            arg = (EjsVar*) ejsCreateString(ejs, socket->error);
        } else {
            eventType = ejsGetType(ejs, ES_ejs_io_SocketDataEvent);
            arg = (EjsVar*) ejs->nullValue;
        }
        event = (EjsObject*) ejsCreateInstance(ejs, eventType, 1, (EjsVar**) &arg);
        if (event) {
            /*
             *  Invoked as:  callback(e: Event)  where e.data == socket
             */
            ejsSetPropertyByName(ejs, (EjsVar*) event, ejsName(&qname, EJS_PUBLIC_NAMESPACE, "data"), (EjsVar*) sp);
            arg = (EjsVar*) event;
            ejsRunFunction(sp->ejs, sp->callback, 0, 1, &arg);
        }
    }
}


/*********************************** Factory **********************************/

EjsSocket *ejsCreateSocket(Ejs *ejs)
{
    EjsSocket     *hp;

    hp = (EjsSocket*) ejsCreateVar(ejs, ejsGetType(ejs, ES_ejs_io_Socket), 0);
    if (hp == 0) {
        return 0;
    }
    return hp;
}


int ejsCreateSocketType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    //  TODO - attributes
    type = ejsCreateCoreType(ejs, ejsName(&qname, "ejs.io", "Socket"), ejs->objectType, sizeof(EjsSocket), 
        ES_ejs_io_Socket, ES_ejs_io_Socket_NUM_CLASS_PROP, ES_ejs_io_Socket_NUM_INSTANCE_PROP);
        EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR | EJS_ATTR_OBJECT_HELPERS);
    if (type == 0) {
        return EJS_ERR;
    }
    return 0;
}


int ejsConfigureSocketType(Ejs *ejs)
{
    EjsType     *type;
    int         rc;

    type = ejsGetType(ejs, ES_ejs_io_Socket);

    rc = 0;
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_Socket, (EjsNativeFunction) socketConstructor);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_address, (EjsNativeFunction) address);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_available, (EjsNativeFunction) socketAvailable, -1, 0);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_set_callback, (EjsNativeFunction) setSocketCallback);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_close, (EjsNativeFunction) closeSocket);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_connect, (EjsNativeFunction) connectSocket);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_eof, (EjsNativeFunction) eof);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_flush, (EjsNativeFunction) flush);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_listen, (EjsNativeFunction) listen);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_mode, (EjsNativeFunction) getMode);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_set_mode, (EjsNativeFunction) getMode);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_port, (EjsNativeFunction) port);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_read, (EjsNativeFunction) readProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_inputStream, (EjsNativeFunction) inputStream, -1, 0);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_outputStream, (EjsNativeFunction) outputStream, -1, 0);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_remoteAddress, (EjsNativeFunction) remoteAddress);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_room, (EjsNativeFunction) room);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_timeout, (EjsNativeFunction) getTimeout);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_set_timeout, (EjsNativeFunction) setTimeout);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Socket_write, (EjsNativeFunction) socketWrite);

    return rc;
}


#else /* ES_Socket */
void __dummySocket() {}
#endif /* ES_Socket */


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
