/*
 *  mprSsl.h - Header for SSL services. Currently supporting OpenSSL and MatrixSSL.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#ifndef _h_MPR_SSL
#define _h_MPR_SSL 1
/***********************************Includes **********************************/

#include "mpr.h"

#if BLD_FEATURE_OPENSSL
    /* Clashes with WinCrypt.h */
    #undef OCSP_RESPONSE
    #include    <openssl/ssl.h>
    #include    <openssl/evp.h>
    #include    <openssl/rand.h>
    #include    <openssl/err.h>
#endif

#if BLD_FEATURE_MATRIXSSL
    #include    "matrixSsl.h"
#endif 

#ifdef __cplusplus
extern "C" {
#endif

/* *********************************** Defines ********************************/
/*
 *  SSL protocols
 */
#define MPR_HTTP_PROTO_SSLV2    0x1
#define MPR_HTTP_PROTO_SSLV3    0x2
#define MPR_HTTP_PROTO_TLSV1    0x4
#define MPR_HTTP_PROTO_ALL      0x7

/*
 *  Default SSL configuration
 */
#define MPR_DEFAULT_CIPHER_SUITE        "ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:+LOW:+SSLv2:+EXP:+eNULL"
#define MPR_DEFAULT_SERVER_CERT_FILE    "server.crt"
#define MPR_DEFAULT_SERVER_KEY_FILE     "server.key.pem"
#define MPR_DEFAULT_CLIENT_CERT_FILE    "client.crt"
#define MPR_DEFAULT_CLIENT_CERT_PATH    "certs"

typedef struct MprSsl {
    /*
     *  Server key and certificate configuration
     */
    char            *keyFile;
    char            *certFile;
    char            *caFile;
    char            *caPath;
    char            *ciphers;

    /*
     *  Client configuration
     */
    int             verifyClient;
    int             verifyDepth;

    int             protocols;
    bool            initialized;
    bool            connTraced;

    /*
     *  Per-SSL provider context information
     */
#if BLD_FEATURE_MATRIXSSL
    sslKeys_t       *keys;
#endif

#if BLD_FEATURE_OPENSSL
    SSL_CTX         *context;
    RSA             *rsaKey512;
    RSA             *rsaKey1024;
    DH              *dhKey512;
    DH              *dhKey1024;
#endif
} MprSsl;


typedef struct MprSslSocket
{
    MprSocket       *sock;
    MprSsl          *ssl;
#if BLD_FEATURE_OPENSSL
    SSL             *osslStruct;
    BIO             *bio;
#endif
#if BLD_FEATURE_MATRIXSSL
    ssl_t           *mssl;
    sslBuf_t        insock;             /* Cached ciphertext from socket */
    sslBuf_t        inbuf;              /* Cached (decoded) plaintext */
    sslBuf_t        outsock;            /* Cached ciphertext to socket */
    int             outBufferCount;     /* Count of outgoing data we've buffered */
#endif
} MprSslSocket;


extern MprModule *mprSslInit(MprCtx ctx, cchar *path);
extern MprSsl *mprCreateSsl(MprCtx ctx);
extern void mprSetSslCiphers(MprSsl *ssl, cchar *ciphers);
extern void mprSetSslKeyFile(MprSsl *ssl, cchar *keyFile);
extern void mprSetSslCertFile(MprSsl *ssl, cchar *certFile);
extern void mprSetSslCaFile(MprSsl *ssl, cchar *caFile);
extern void mprSetSslCaPath(MprSsl *ssl, cchar *caPath);
extern void mprSetSslProtocols(MprSsl *ssl, int protocols);
extern void mprVerifySslClients(MprSsl *ssl, bool on);

#if BLD_FEATURE_OPENSSL
extern int mprCreateOpenSslModule(MprCtx ctx, bool lazy);
#endif

#if BLD_FEATURE_MATRIXSSL
extern int mprCreateMatrixSslModule(MprCtx ctx, bool lazy);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _h_MPR_SSL */

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
