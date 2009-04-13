#include "appweb.h"

/******************************************************************************/
/* 
 *  This file is an amalgamation of all the individual source code files for
 *  Embedthis Appweb 3.0B.0.
 *
 *  Catenating all the source into a single file makes embedding simpler and
 *  the resulting application faster, as many compilers can do whole file
 *  optimization.
 *
 *  If you want to modify appweb, you can still get the whole source
 *  as individual files if you need.
 */


/************************************************************************/
/*
 *  Start of file "../http/alias.c"
 */
/************************************************************************/

/*
 *  alias.c -- Alias service for aliasing URLs to file storage.
 *
 *  This module supports the alias directives and mapping URLs to physical locations. 
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



/*
 *  Create an alias for a URI prefix. During processing, a request URI prefix is substituted to the target which
 *  may be either a physical path or a URI if a non-zero redirect code is supplied.
 */

MaAlias *maCreateAlias(MprCtx ctx, cchar *prefix, cchar *target, int code)
{
    MaAlias     *ap;
    int         len;

    mprAssert(ctx);
    mprAssert(prefix);
    mprAssert(target && *target);

    ap = mprAllocObjZeroed(ctx, MaAlias);
    if (ap == 0) {
        return 0;
    }

    ap->prefix = mprStrdup(ctx, prefix);
    ap->prefixLen = (int) strlen(prefix);

    /*
     *  Always strip trailing "/"
     */
    if (ap->prefixLen > 0 && ap->prefix[ap->prefixLen - 1] == '/') {
        ap->prefix[--ap->prefixLen] = '\0';
    }

    if (code) {
        ap->redirectCode = code;
        ap->uri = mprStrdup(ctx, target);

    } else {
        /*
         *  Trim trailing "/" from filename always
         */
        ap->filename = mprStrdup(ctx, target);
        len = strlen(ap->filename) - 1;
        if (len >= 0 && ap->filename[len] == '/') {
            ap->filename[len] = '\0';
        }
    }

    return ap;
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
/************************************************************************/
/*
 *  End of file "../http/alias.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/authFile.c"
 */
/************************************************************************/

/*
 *  authFile.c - File based authorization using httpPassword files.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_AUTH_FILE

static bool isUserValid(MaAuth *auth, cchar *realm, cchar *user);
static char *trimWhiteSpace(char *str);


cchar *maGetNativePassword(MaConn *conn, cchar *realm, cchar *user)
{
    MaUser      *up;
    MaAuth      *auth;
    char        key[MA_MAX_USER * 2];

    auth = conn->request->auth;

    up = 0;
    mprStrcat(key, sizeof(key), ":", realm, user, 0);
    if (auth->users) {
        up = (MaUser*) mprLookupHash(auth->users, key);
    }
    if (up == 0) {
        return 0;
    }
    return up->password;
}


bool maValidateNativeCredentials(MaConn *conn, cchar *realm, cchar *user, cchar *password, cchar *requiredPassword, 
        char **msg)
{
    MaAuth  *auth;
    char    passbuf[MA_MAX_PASS * 2];
    int     len;

    auth = conn->request->auth;
    
    if (auth->type == MA_AUTH_BASIC) {
        len = mprSprintf(passbuf, sizeof(passbuf), "%s:%s:%s", user, realm, password);
        password = mprGetMD5Hash(conn, (uchar*) passbuf, len, NULL);
    }
    if (!isUserValid(auth, realm, user)) {
        *msg = "Access Denied, Unknown User.";
        return 0;
    }
    if (strcmp(password, requiredPassword)) {
        *msg = "Access Denied, Wrong Password.";
        return 0;
    }
    return 1;
}


/*
 *  Determine if this user is specified as being eligible for this realm. We examine the requiredUsers and requiredGroups.
 */
static bool isUserValid(MaAuth *auth, cchar *realm, cchar *user)
{
    MaGroup         *gp;
    MaUser          *up;
    cchar           *tok, *gtok;
    char            ubuf[80], gbuf[80], key[MA_MAX_USER * 2], *requiredUser, *group, *name;
    int             next;

    if (auth->anyValidUser) {
        mprStrcat(key, sizeof(key), ":", realm, user, 0);
        if (auth->users == 0) {
            return 0;
        }
        return mprLookupHash(auth->users, key) != 0;
    }

    if (auth->requiredUsers) {
        tok = NULL;
        requiredUser = mprGetWordTok(ubuf, sizeof(ubuf), auth->requiredUsers, " \t", &tok);
        while (requiredUser) {
            if (strcmp(user, requiredUser) == 0) {
                return 1;
            }
            requiredUser = mprGetWordTok(ubuf, sizeof(ubuf), 0, " \t", &tok);
        }
    }

    if (auth->requiredGroups) {
        gtok = NULL;
        group = mprGetWordTok(gbuf, sizeof(gbuf), auth->requiredGroups, " \t", &gtok);
        /*
         *  For each group, check all the users in the group.
         */
        while (group) {
            if (auth->groups == 0) {
                gp = 0;
            } else {
                gp = (MaGroup*) mprLookupHash(auth->groups, group);
            }
            if (gp == 0) {
                mprError(auth, "Can't find group %s", group);
                group = mprGetWordTok(gbuf, sizeof(gbuf), 0, " \t", &gtok);
                continue;
            }

            for (next = 0; (name = mprGetNextItem(gp->users, &next)) != 0; ) {
                if (strcmp(user, name) == 0) {
                    return 1;
                }
            }
            group = mprGetWordTok(gbuf, sizeof(gbuf), 0, " \t", &gtok);
        }
    }

    if (auth->requiredAcl != 0) {
        mprStrcat(key, sizeof(key), ":", realm, user, 0);
        up = (MaUser*) mprLookupHash(auth->users, key);
        if (up) {
            mprLog(auth, 6, "UserRealm \"%s\" has ACL %lx, Required ACL %lx", key, up->acl, auth->requiredAcl);
            if (up->acl & auth->requiredAcl) {
                return 1;
            }
        }
    }
    return 0;
}


MaGroup *maCreateGroup(MaAuth *auth, cchar *name, MaAcl acl, bool enabled)
{
    MaGroup     *gp;

    gp = mprAllocObjZeroed(auth, MaGroup);
    if (gp == 0) {
        return 0;
    }

    gp->acl = acl;
    gp->name = mprStrdup(gp, name);
    gp->enabled = enabled;
    gp->users = mprCreateList(gp);
    return gp;
}


//  TODO - should be cchar
int maAddGroup(MaAuth *auth, char *group, MaAcl acl, bool enabled)
{
    MaGroup     *gp;

    mprAssert(auth);
    mprAssert(group && *group);

    gp = maCreateGroup(auth, group, acl, enabled);
    if (gp == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    /*
     *  Create the index on demand
     */
    if (auth->groups == 0) {
        auth->groups = mprCreateHash(auth, -1);
    }

    if (mprLookupHash(auth->groups, group)) {
        return MPR_ERR_ALREADY_EXISTS;
    }

    if (mprAddHash(auth->groups, group, gp) == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    return 0;
}


MaUser *maCreateUser(MaAuth *auth, cchar *realm, cchar *user, cchar *password, bool enabled)
{
    MaUser      *up;

    up = mprAllocObjZeroed(auth, MaUser);
    if (up == 0) {
        return 0;
    }

    up->name = mprStrdup(up, user);
    up->realm = mprStrdup(up, realm);
    up->password = mprStrdup(up, password);
    up->enabled = enabled;
    return up;
}


int maAddUser(MaAuth *auth, cchar *realm, cchar *user, cchar *password, bool enabled)
{
    MaUser  *up;

    char    key[MA_MAX_USER * 2];

    up = maCreateUser(auth, realm, user, password, enabled);
    if (up == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    if (auth->users == 0) {
        auth->users = mprCreateHash(auth, -1);
    }
    mprStrcat(key, sizeof(key), ":", realm, user, 0);
    if (mprLookupHash(auth->users, key)) {
        return MPR_ERR_ALREADY_EXISTS;
    }

    if (mprAddHash(auth->users, key, up) == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    return 0;
}


int maAddUserToGroup(MaAuth *auth, MaGroup *gp, cchar *user)
{
    char        *name;
    int         next;

    for (next = 0; (name = mprGetNextItem(gp->users, &next)) != 0; ) {
        if (strcmp(name, user) == 0) {
            return MPR_ERR_ALREADY_EXISTS;
        }
    }
    mprAddItem(gp->users, mprStrdup(gp, user));
    return 0;
}


int maAddUsersToGroup(MaAuth *auth, cchar *group, cchar *users)
{
    MaGroup     *gp;
    cchar       *tok;
    char        ubuf[80], *user;

    gp = 0;

    if (auth->groups == 0 || (gp = (MaGroup*) mprLookupHash(auth->groups, group)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }

    tok = NULL;
    user = mprGetWordTok(ubuf, sizeof(ubuf), users, " \t", &tok);
    while (user) {
        /* Ignore already exists errors */
        maAddUserToGroup(auth, gp, user);
        user = mprGetWordTok(ubuf, sizeof(ubuf), 0, " \t", &tok);
    }
    return 0;
}


int maDisableGroup(MaAuth *auth, cchar *group)
{
    MaGroup     *gp;

    gp = 0;

    if (auth->groups == 0 || (gp = (MaGroup*) mprLookupHash(auth->groups, group)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    gp->enabled = 0;
    return 0;
}


int maDisableUser(MaAuth *auth, cchar *realm, cchar *user)
{
    MaUser      *up;
    char        key[MA_MAX_USER * 2];

    up = 0;
    mprStrcat(key, sizeof(key), ":", realm, user, 0);
    if (auth->users == 0 || (up = (MaUser*) mprLookupHash(auth->users, key)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    up->enabled = 0;
    return 0;
}


int maEnableGroup(MaAuth *auth, cchar *group)
{
    MaGroup     *gp;

    gp = 0;

    if (auth->groups == 0 || (gp = (MaGroup*) mprLookupHash(auth->groups, group)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    gp->enabled = 1;
    return 0;
}


int maEnableUser(MaAuth *auth, cchar *realm, cchar *user)
{
    MaUser      *up;
    char        key[MA_MAX_USER * 2];

    up = 0;
    mprStrcat(key, sizeof(key), ":", realm, user, 0);    
    if (auth->users == 0 || (up = (MaUser*) mprLookupHash(auth->users, key)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    up->enabled = 1;
    return 0;
}


MaAcl maGetGroupAcl(MaAuth *auth, char *group)
{
    MaGroup     *gp;

    gp = 0;

    if (auth->groups == 0 || (gp = (MaGroup*) mprLookupHash(auth->groups, group)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    return gp->acl;
}


bool maIsGroupEnabled(MaAuth *auth, cchar *group)
{
    MaGroup     *gp;

    gp = 0;
    if (auth->groups == 0 || (gp = (MaGroup*) mprLookupHash(auth->groups, group)) == 0) {
        return 0;
    }
    return gp->enabled;
}


bool maIsUserEnabled(MaAuth *auth, cchar *realm, cchar *user)
{
    MaUser  *up;
    char    key[MA_MAX_USER * 2];

    up = 0;
    mprStrcat(key, sizeof(key), ":", realm, user, 0);
    if (auth->users == 0 || (up = (MaUser*) mprLookupHash(auth->users, key)) == 0) {
        return 0;
    }
    return up->enabled;
}


/*
 *  ACLs are simple hex numbers
 */
//  TODO - better to convert to user, role, capability
MaAcl maParseAcl(MaAuth *auth, cchar *aclStr)
{
    MaAcl   acl = 0;
    int     c;

    if (aclStr) {
        if (aclStr[0] == '0' && aclStr[1] == 'x') {
            aclStr += 2;
        }
        for (; isxdigit((int) *aclStr); aclStr++) {
            c = (int) tolower((int) *aclStr);
            if ('0' <= c && c <= '9') {
                acl = (acl * 16) + c - '0';
            } else {
                acl = (acl * 16) + c - 'a' + 10;
            }
        }
    }
    return acl;
}


/*
 *  Update the total ACL for each user. We do this by oring all the ACLs for each group the user is a member of. 
 *  For fast access, this union ACL is stored in the MaUser object
 */
void maUpdateUserAcls(MaAuth *auth)
{
    MprHash     *groupHash, *userHash;
    MaUser      *user;
    MaGroup     *gp;
    
    /*
     *  Reset the ACL for each user
     */
    if (auth->users != 0) {
        for (userHash = 0; (userHash = mprGetNextHash(auth->users, userHash)) != 0; ) {
            ((MaUser*) userHash->data)->acl = 0;
        }
    }

    /*
     *  Get the union of all ACLs defined for a user over all groups that the user is a member of.
     */
    if (auth->groups != 0 && auth->users != 0) {
        for (groupHash = 0; (groupHash = mprGetNextHash(auth->groups, groupHash)) != 0; ) {
            gp = ((MaGroup*) groupHash->data);
            for (userHash = 0; (userHash = mprGetNextHash(auth->users, userHash)) != 0; ) {
                user = ((MaUser*) userHash->data);
                if (strcmp(user->name, user->name) == 0) {
                    user->acl = user->acl | gp->acl;
                    break;
                }
            }
        }
    }
}


int maRemoveGroup(MaAuth *auth, cchar *group)
{
    if (auth->groups == 0 || !mprLookupHash(auth->groups, group)) {
        return MPR_ERR_CANT_ACCESS;
    }
    mprRemoveHash(auth->groups, group);
    return 0;
}


int maRemoveUser(MaAuth *auth, cchar *realm, cchar *user)
{
    char    key[MA_MAX_USER * 2];

    mprStrcat(key, sizeof(key), ":", realm, user, 0);
    if (auth->users == 0 || !mprLookupHash(auth->users, key)) {
        return MPR_ERR_CANT_ACCESS;
    }
    mprRemoveHash(auth->users, key);
    return 0;
}


int maRemoveUsersFromGroup(MaAuth *auth, cchar *group, cchar *users)
{
    MaGroup     *gp;
    cchar       *tok;
    char        ubuf[80], *user;

    gp = 0;
    if (auth->groups == 0 || (gp = (MaGroup*) mprLookupHash(auth->groups, group)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }

    tok = NULL;
    user = mprGetWordTok(ubuf, sizeof(ubuf), users, " \t", &tok);
    while (user) {
        maRemoveUserFromGroup(gp, user);
        user = mprGetWordTok(ubuf, sizeof(ubuf), 0, " \t", &tok);
    }
    return 0;
}


int maSetGroupAcl(MaAuth *auth, cchar *group, MaAcl acl)
{
    MaGroup     *gp;

    gp = 0;
    if (auth->groups == 0 || (gp = (MaGroup*) mprLookupHash(auth->groups, group)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    gp->acl = acl;
    return 0;
}


void maSetRequiredAcl(MaAuth *auth, MaAcl acl)
{
    auth->requiredAcl = acl;
}


int maRemoveUserFromGroup(MaGroup *gp, cchar *user)
{
    char    *name;
    int     next;

    for (next = 0; (name = mprGetNextItem(gp->users, &next)) != 0; ) {
        if (strcmp(name, user) == 0) {
            mprRemoveItem(gp->users, name);
            return 0;
        }
    }
    return MPR_ERR_CANT_ACCESS;
}


int maReadGroupFile(MaServer *server, MaAuth *auth, char *path)
{
    MprFile     *file;
    MaAcl       acl;
    char        buf[MPR_MAX_STRING];
    char        *users, *group, *enabled, *aclSpec, *tok, *cp;

    mprFree(auth->groupFile);
    auth->groupFile = mprStrdup(server, path);

    if ((file = mprOpen(auth, path, O_RDONLY | O_TEXT, 0444)) == 0) {
        return MPR_ERR_CANT_OPEN;
    }

    while (mprGets(file, buf, sizeof(buf))) {
        enabled = mprStrTok(buf, " :\t", &tok);

        for (cp = enabled; isspace((int) *cp); cp++) {
            ;
        }
        if (*cp == '\0' || *cp == '#') {
            continue;
        }

        aclSpec = mprStrTok(0, " :\t", &tok);
        group = mprStrTok(0, " :\t", &tok);
        users = mprStrTok(0, "\r\n", &tok);

        acl = maParseAcl(auth, aclSpec);
        maAddGroup(auth, group, acl, (*enabled == '0') ? 0 : 1);
        maAddUsersToGroup(auth, group, users);
    }
    mprFree(file);

    maUpdateUserAcls(auth);
    return 0;
}


int maReadUserFile(MaServer *server, MaAuth *auth, char *path)
{
    MprFile     *file;
    char        buf[MPR_MAX_STRING];
    char        *enabled, *user, *password, *realm, *tok, *cp;

    mprFree(auth->userFile);
    auth->userFile = mprStrdup(auth, path);

    if ((file = mprOpen(auth, path, O_RDONLY | O_TEXT, 0444)) == 0) {
        return MPR_ERR_CANT_OPEN;
    }

    while (mprGets(file, buf, sizeof(buf))) {
        enabled = mprStrTok(buf, " :\t", &tok);

        for (cp = enabled; isspace((int) *cp); cp++) {
            ;
        }
        if (*cp == '\0' || *cp == '#') {
            continue;
        }

        user = mprStrTok(0, ":", &tok);
        realm = mprStrTok(0, ":", &tok);
        password = mprStrTok(0, " \t\r\n", &tok);

        user = trimWhiteSpace(user);
        realm = trimWhiteSpace(realm);
        password = trimWhiteSpace(password);

        maAddUser(auth, realm, user, password, (*enabled == '0' ? 0 : 1));
    }
    mprFree(file);
    maUpdateUserAcls(auth);
    return 0;
}


int maWriteUserFile(MaServer *server, MaAuth *auth, char *path)
{
    MprFile         *file;
    MprHash         *hp;
    MaUser          *up;
    char            buf[MA_MAX_PASS * 2];
    char            tempFile[MPR_MAX_FNAME];

    mprMakeTempFileName(auth, tempFile, sizeof(tempFile), 0);

    if ((file = mprOpen(auth, tempFile, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0444)) == 0) {
        mprError(server, "Can't open %s", tempFile);
        return MPR_ERR_CANT_OPEN;
    }

    hp = mprGetNextHash(auth->users, 0);
    while (hp) {
        up = (MaUser*) hp->data;
        mprSprintf(buf, sizeof(buf), "%d: %s: %s: %s\n", up->enabled, up->realm, up->name, up->password);
        mprWrite(file, buf, (int) strlen(buf));
        hp = mprGetNextHash(auth->users, hp);
    }

    mprFree(file);

    unlink(path);
    if (rename(tempFile, path) < 0) {
        mprError(server, "Can't create new %s", path);
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}


int maWriteGroupFile(MaServer *server, MaAuth *auth, char *path)
{
    MprHash         *hp;
    MprFile         *file;
    MaGroup         *gp;
    char            buf[MPR_MAX_STRING], tempFile[MPR_MAX_FNAME], *name;
    int             next;

    mprMakeTempFileName(server, tempFile, sizeof(tempFile), 0);
    if ((file = mprOpen(auth, tempFile, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0444)) == 0) {
        mprError(server, "Can't open %s", tempFile);
        return MPR_ERR_CANT_OPEN;
    }

    hp = mprGetNextHash(auth->groups, 0);
    while (hp) {
        gp = (MaGroup*) hp->data;
        mprSprintf(buf, sizeof(buf), "%d: %x: %s: ", gp->enabled, gp->acl, gp->name);
        mprWrite(file, buf, (int) strlen(buf));
        for (next = 0; (name = mprGetNextItem(gp->users, &next)) != 0; ) {
            mprWrite(file, name, (int) strlen(name));
        }
        mprWrite(file, "\n", 1);
        hp = mprGetNextHash(auth->groups, hp);
    }
    mprFree(file);

    unlink(path);
    if (rename(tempFile, path) < 0) {
        mprError(server, "Can't create new %s", path);
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}


//  TODO - replace with mprStrTrim()
static char *trimWhiteSpace(char *str)
{
    int     len;

    if (str == 0) {
        return str;
    }
    while (isspace((int) *str)) {
        str++;
    }
    len = (int) strlen(str) - 1;
    while (isspace((int) str[len])) {
        str[len--] = '\0';
    }
    return str;
}


#else
void __nativeAuthFile() {}
#endif /* BLD_FEATURE_AUTH_FILE */

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
/************************************************************************/
/*
 *  End of file "../http/authFile.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/authPam.c"
 */
/************************************************************************/

/*
 *  authPam.c - Authorization using PAM (Pluggable Authorization Module)
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_AUTH_PAM && BLD_UNIX_LIKE

#if MACOSX
#include    <pam/pam_appl.h>
#else
#include    <security/pam_appl.h>
#endif


typedef struct {
    MprCtx  ctx;
    char    *name;
    char    *password;
} UserInfo;


static int pamChat(int msgCount, const struct pam_message **msg, struct pam_response **resp, void *data);


cchar *maGetPamPassword(MaConn *conn, cchar *realm, cchar *user)
{
    /*
     *  Can't return the password.
     */
    return "";
}


bool maValidatePamCredentials(MaConn *conn, cchar *realm, cchar *user, cchar *password, cchar *requiredPass, char **msg)
{
    pam_handle_t        *pamh;
    UserInfo            info;
    struct pam_conv     conv = { pamChat, &info };
    int                 res;
   
    info.ctx = conn->request;
    info.name = (char*) user;
    info.password = (char*) password;
    pamh = NULL;
        
    if ((res = pam_start("login", user, &conv, &pamh)) != PAM_SUCCESS) {
        return 0;
    }
    if ((res = pam_authenticate(pamh, 0)) != PAM_SUCCESS) {
        return 0;
    }
    pam_end(pamh, PAM_SUCCESS);
    return 1;
}


/*
 *  Callback invoked by the pam_authenticate function
 */
static int pamChat(int msgCount, const struct pam_message **msg, struct pam_response **resp, void *data) 
{
    UserInfo                *info;
    struct pam_response     *reply;
    int                     i;
    
    i = 0;
    reply = 0;
    info = (UserInfo*) data;

    if (resp == 0 || msg == 0 || info == 0) {
        return PAM_CONV_ERR;
    }
    
    if ((reply = mprAlloc(info->ctx, msgCount * sizeof(struct pam_response))) == 0) {
        return PAM_CONV_ERR;
    }
    
    for (i = 0; i < msgCount; i++) {
        reply[i].resp_retcode = 0;
        reply[i].resp = 0;
        
        switch (msg[i]->msg_style) {
        case PAM_PROMPT_ECHO_ON:
            reply[i].resp = mprStrdup(info->ctx, info->name);
            break;

        case PAM_PROMPT_ECHO_OFF:
            reply[i].resp = mprStrdup(info->ctx, info->password);
            break;

        default:
            return PAM_CONV_ERR;
        }
    }
    *resp = reply;
    return PAM_SUCCESS;
}


#else
void __pamAuth() {}
#endif /* BLD_FEATURE_AUTH_PAM */

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
/************************************************************************/
/*
 *  End of file "../http/authPam.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/config.c"
 */
/************************************************************************/

/**
 *  config.c - Parse the configuration file.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_CONFIG_PARSE


static bool featureSupported(MprCtx ctx, char *key);
static MaConfigState *pushState(MprCtx ctx, MaConfigState *state, int *top);
static int processSetting(MaServer *server, char *key, char *value, MaConfigState *state);

#if BLD_FEATURE_CONFIG_SAVE
static void tabs(int fd, int indent);
static void printAuth(int fd, MaHost *host, MaAuth *auth, int indent);
#endif

/*
 *  Configure the server
 */
int maConfigureServer(MprCtx ctx, MaHttp *http, MaServer *server, cchar *configFile, cchar *ipAddr, int port, cchar *docRoot)
{
    MaHost      *host;
    MaLocation  *location, *loc;
    MaAlias     *ap;
    char        *path;

    if (ipAddr && docRoot) {

        mprLog(http, 2, "DocumentRoot %s", docRoot);
        if ((host = maCreateDefaultHost(server, docRoot, ipAddr, port)) == 0) {
            mprUserError(ctx, "Can't open server on %s", ipAddr);
            return MPR_ERR_CANT_OPEN;
        }

        location = host->location;

        maSetConnector(http, location, "netConnector");

// TODO - neeed other connectors, filters and handlers
// TODO - need to call module Init() entry points
        /*
         *  Auth must be added first to authorize all requests. File is last as a catch all.
         */
        if (maLookupStage(http, "authFilter")) {
            maAddHandler(http, location, "authFilter", "");
        }
        if (maLookupStage(http, "cgiHandler")) {
            maAddHandler(http, location, "cgiHandler", ".cgi .cgi-nph .bat .cmd .pl .py");
        }
        if (maLookupStage(http, "ejsHandler")) {
            maAddHandler(http, location, "ejsHandler", ".ejs");
        }
        if (maLookupStage(http, "fileHandler")) {
            maAddHandler(http, location, "fileHandler", "");
        }

        /*
         *  Add cgi-bin with a location block for the /cgi-bin URL prefix.
         */
        path = "cgi-bin";
        if (mprAccess(host, path, X_OK)) {
            ap = maCreateAlias(host, "/cgi-bin/", path, 0);
            mprLog(host, 4, "ScriptAlias \"/cgi-bin/\":\"%s\"", path);
            maInsertAlias(host, ap);
            loc = maCreateLocation(host, host->location);
            maSetLocationPrefix(loc, "/cgi-bin/");
            maSetHandler(http, host, loc, "cgiHandler");
            maAddLocation(host, loc);
        }

#if BLD_FEATURE_CONFIG_PARSE
    } else {
        /*
         *  Configure the http service and hosts specified in the config file.
         */
        path = mprGetAbsFilename(server, configFile);

        if (maParseConfig(server, path) < 0) {
            /* mprUserError(ctx, "Can't configure server using %s", path); */
            return MPR_ERR_CANT_INITIALIZE;
        }
        mprFree(path);
#endif
    }

    return 0;
}


int maParseConfig(MaServer *server, cchar *configFile)
{
    Mpr             *mpr;
    MprList         *includes;
    MaHttp          *http;
    MaHost          *defaultHost;
    MaConfigState   stack[MA_MAX_CONFIG_DEPTH], *state;
    MaHostAddress   *address;
    MaListen        *listen;
    MaDir           *dir, *bestDir;
    MaHost          *host, *hp;
    MaAlias         *alias;
    MprDirEntry     *dp;
    bool            needServerName, natServerName;
    char            buf[MPR_MAX_STRING], pathBuf[MPR_MAX_FNAME];
    char            ipAddrPort[MPR_MAX_IP_ADDR_PORT];
    char            cwd[MPR_MAX_FNAME];
    char            *cp, *tok, *key, *value, *ipAddr;
    int             i, rc, top, next, nextAlias, len;

    mpr = mprGetMpr(server);

    http = server->http;
    memset(stack, 0, sizeof(stack));

#if BLD_FEATURE_LOG
    server->alreadyLogging = mprGetLogHandler(server) ? 1 : 0;
#endif

    /*
     *  Create the default host and directory
     */
    defaultHost = host = maCreateHost(server, 0, 0);
    server->defaultHost = defaultHost;
    mprAddItem(server->hosts, host);

    top = 0;
    state = &stack[top];
    state->server = server;
    state->host = host;
    state->dir = maCreateBareDir(host, ".");
    state->location = defaultHost->location;
    state->location->connector = http->netConnector;
    state->enabled = 1;
    state->lineNumber = 0;

    state->filename = (char*) configFile;
    state->file = mprOpen(server, configFile, O_RDONLY | O_TEXT, 0444);
    if (state->file == 0) {
        mprError(server, "Can't open %s for config directives", configFile);
        return MPR_ERR_CANT_OPEN;
    }

    /*
     *  Set the default location authorization definition to match the default directory auth
     */
    state->location->auth = state->dir->auth;
    state->auth = state->dir->auth;

    maInsertDir(host, state->dir);
    maSetHostName(host, "Main Server");
    getcwd(cwd, sizeof(cwd) - 1);

    /*
     *  Parse each line in the config file
     */
    for (state->lineNumber = 1; top >= 0; state->lineNumber++) {

        state = &stack[top];
        mprAssert(state->file);
        if (mprGets(state->file, buf, sizeof(buf) - 1) == 0) {
            if (--top > 0 && strcmp(state->filename, stack[top].filename) == 0) {
                mprError(server, "Unclosed directives in %s", state->filename);
                while (top >= 0 && strcmp(state->filename, state[top].filename) == 0) {
                    top--;
                }
            }
            if (top >= 0 && state->file == stack[top].file) {
                stack[top].lineNumber = state->lineNumber + 1;
            } else {
                mprFree(state->file);
                state->file = 0;
                if (top >= 0) {
                    state = &stack[top];
                }
            }
            continue;
        }

        buf[sizeof(buf) - 1] = '\0';
        cp = buf;
        while (isspace((int) *cp)) {
            cp++;
        }
        if (*cp == '\0' || *cp == '#') {
            continue;
        }

        cp = mprStrTrim(cp, "\r\n\t ");
        key = mprStrTok(cp, " \t\n", &tok);
        value = mprStrTok(0, "\n", &tok);
        if (key == 0 || *key == '\0') {
            goto syntaxErr;
        }
        if (value) {
            while (isspace((int) *value)) {
                value++;
            }
            if (*value) {
                cp = &value[strlen(value) - 1];
                while (cp > value && isspace((int) *cp)) {
                    cp--;
                }
                *++cp = '\0';
            }

        } else {
            value = "";
        }


        if (mprStrcmpAnyCase(key, "Include") == 0) {
            state->lineNumber++;
            value = mprStrTrim(value, "\"");
            if ((cp = strchr(value, '*')) == 0) {
                state = pushState(server, state, &top);
                state->lineNumber = 0;
                state->filename = mprStrdup(server, value);
                state->file = mprOpen(server, value, O_RDONLY | O_TEXT, 0444);
                if (state->file == 0) {
                    mprError(server, "Can't open include file %s for config directives", state->filename);
                    return MPR_ERR_CANT_OPEN;
                }
                mprLog(server, 5, "Parsing config file: %s", state->filename);

            } else {
                /*
                 *  Process wild cards. This is very simple - only "*" is supported.
                 */
                *cp = '\0';
                len = (int) strlen(value);
                if (value[len - 1] == '/') {
                    value[len - 1] = '\0';
                }
                includes = mprGetDirList(server, value, 0);
                if (includes == 0) {
                    continue;
                }
                for (next = 0; (dp = mprGetNextItem(includes, &next)) != 0; ) {
                    state = pushState(server, state, &top);
                    mprSprintf(pathBuf, sizeof(pathBuf), "%s/%s", value, dp->name);
                    state->filename = mprStrdup(server, pathBuf);
                    state->file = mprOpen(server, pathBuf, O_RDONLY | O_TEXT, 0444);
                    if (state->file == 0) {
                        mprError(server, "Can't open include file %s for config directives", state->filename);
                        return MPR_ERR_CANT_OPEN;
                    }
                    mprLog(server, 5, "Parsing config file: %s", state->filename);
                }
                mprFree(includes);
            }
            continue;

        } else if (*key != '<') {

            if (!state->enabled) {
                mprLog(server, 6, "Skipping key %s at %s:%d", key, state->filename, state->lineNumber);
                continue;
            }

            /*
             *  Keywords outside of a virtual host or directory section
             */
            rc = processSetting(server, key, value, state);
            if (rc == 0) {
                char    *extraMsg;
                if (strcmp(key, "SSLEngine") == 0) {
                    extraMsg =
                        "\n\nFor SSL, you may have one SSL provider loaded.\n"
                        "Make sure that either OpenSSL or MatrixSSL is loaded.";
                } else {
                    extraMsg = "";
                }
                mprError(server,
                    "Ignoring unknown directive \"%s\"\nAt line %d in %s\n\n"
                    "Make sure the required modules are loaded. %s\n",
                    key, state->lineNumber, state->filename, extraMsg);
                continue;

            } else if (rc < 0) {
                mprError(server, "Ignoring bad directive \"%s\" at %s:%d in %s", key, state->filename, state->lineNumber, 
                    configFile);
            }
            continue;
        }

        mprLog(server, 9, "AT %d, key %s", state->lineNumber, key);

        /*
         *  Directory, Location and virtual host sections
         */
        key++;
        i = (int) strlen(key) - 1;
        if (key[i] == '>') {
            key[i] = '\0';
        }
        if (*key != '/') {
            if (!state->enabled) {
                state = pushState(server, state, &top);
                mprLog(server, 6, "Skipping key %s at %s:%d", key, state->filename, state->lineNumber);
                continue;
            }

            i = (int) strlen(value) - 1;
            if (value[i] == '>') {
                value[i] = '\0';
            }

            /*
             *  Opening tags
             */
            if (mprStrcmpAnyCase(key, "If") == 0) {
                value = mprStrTrim(value, "\"");

                /*
                 *  Want to be able to nest <if> directives.
                 */
                state = pushState(server, state, &top);
                state->enabled = featureSupported(server, value);
                if (!state->enabled) {
                    mprLog(server, 6, "If \"%s\" conditional is false at %s:%d", value, state->filename, state->lineNumber);
                }

            } else if (mprStrcmpAnyCase(key, "VirtualHost") == 0) {

                value = mprStrTrim(value, "\"");

                state = pushState(server, state, &top);
                state->host = host = maCreateVirtualHost(server, value, host);
                state->location = host->location;
                state->auth = host->location->auth;

                maAddHost(server, host);
                maSetVirtualHost(host);

                state->dir = maCreateDir(host, stack[top - 1].dir->path, stack[top - 1].dir);
                state->auth = state->dir->auth;
                maInsertDir(host, state->dir);

                mprLog(server, 2, "Virtual Host: %s ", value);
                if (maCreateHostAddresses(server, host, value) < 0) {
                    mprFree(host);
                    goto err;
                }

            } else if (mprStrcmpAnyCase(key, "Directory") == 0) {
                value = mprStrTrim(value, "\"");

                if (maMakePath(host, pathBuf, sizeof(pathBuf), value) == 0) {
                    mprError(server, "Directory path is too long");
                    goto err;
                }

                state = pushState(server, state, &top);

                if ((dir = maLookupDir(host, pathBuf)) != 0) {
                    /*
                     *  Allow multiple occurences of the same directory. Append directives.
                     */  
                    state->dir = dir;

                } else {
                    /*
                     *  Create a new directory inherit and parent directory settings. This means inherit authorization from
                     *  the enclosing host.
                     */
                    state->dir = maCreateDir(host, pathBuf, stack[top - 1].dir);
                    maInsertDir(host, state->dir);
                }
                state->auth = state->dir->auth;

            } else if (mprStrcmpAnyCase(key, "Location") == 0) {
                value = mprStrTrim(value, "\"");
                if (maLookupLocation(host, value)) {
                    mprError(http, "Location block already exists for \"%s\"", value);
                    goto err;
                }
                state = pushState(server, state, &top);
                state->location = maCreateLocation(host, state->location);
                state->auth = state->location->auth;

                maSetLocationPrefix(state->location, value);

                if (maAddLocation(host, state->location) < 0) {
                    mprError(server, "Can't add location %s", value);
                    goto err;
                }
                mprAssert(host->location->prefix);
            }

        } else {

            stack[top - 1].lineNumber = state->lineNumber + 1;
            key++;

            /*
             *  Closing tags
             */
            if (state->enabled && state->location != stack[top-1].location) {
                maFinalizeLocation(state->location);
            }
            if (mprStrcmpAnyCase(key, "If") == 0) {
                top--;
                host = stack[top].host;

            } else if (mprStrcmpAnyCase(key, "VirtualHost") == 0) {
                top--;
                host = stack[top].host;

            } else if (mprStrcmpAnyCase(key, "Directory") == 0) {
                top--;

            } else if (mprStrcmpAnyCase(key, "Location") == 0) {
                top--;
            }
            if (top < 0) {
                goto syntaxErr;
            }
        }
    }

    /*
     *  Validate configuration
     */
    if (mprGetListCount(server->listens) == 0) {
        mprError(server, "Must have a Listen directive");
        return MPR_ERR_BAD_SYNTAX;
    }
    if (defaultHost->documentRoot == 0) {
        maSetDocumentRoot(defaultHost, ".");
    }

    for (next = 0; (hp = mprGetNextItem(server->hosts, &next)) != 0; ) {

        if (hp->documentRoot == 0) {
            maSetDocumentRoot(hp, defaultHost->documentRoot);
        }

        /*
         *  Ensure all hosts have mime types.
         */
        if (hp->mimeTypes == 0 || mprGetHashCount(hp->mimeTypes) == 0) {
            if (hp == defaultHost && defaultHost->mimeTypes) {
                hp->mimeTypes = defaultHost->mimeTypes;
            } else if (maOpenMimeTypes(hp, "mime.types") < 0) {
                /*
                 *  Do minimal configure
                 */
                maAddStandardMimeTypes(hp);
                mprLog(server, 2, "No mime.types file, using minimal mime configuration");
            }
        }

        /*
         *  Check aliases have directory blocks. We must be careful to inherit authorization from the best 
         *  matching directory.
         */
        for (nextAlias = 0; (alias = mprGetNextItem(hp->aliases, &nextAlias)) != 0; ) {
            // mprLog(hp, 0, "Alias \"%s\" %s", alias->prefix, alias->filename);
            maMakePath(hp, pathBuf, sizeof(pathBuf), alias->filename);
            bestDir = maLookupBestDir(hp, pathBuf);
            if (bestDir == 0) {
                bestDir = maCreateDir(hp, alias->filename, stack[0].dir);
                maInsertDir(hp, bestDir);
            }
        }

        //  TODO - need to check all location blocks in all hosts?
    }

    /*
     *  TODO -- should test here that all location handlers are defined
     */
#if BLD_FEATURE_MULTITHREAD
{
    MaLimits *limits = &http->limits;
    if (limits->maxThreads > 0) {
        mprSetMaxPoolThreads(http, limits->maxThreads);
        mprSetMinPoolThreads(http, limits->minThreads);
    }
}
#endif

    /*
     *  Add default server listening addresses to the HostAddress hash. We pretend it is a vhost. Insert at the
     *  end of the vhost list so we become the default if no other vhost matches. Ie. vhosts take precedence
     *  At the same time, define a ServerName if one has not been defined. We take the first non loopback listening
     *  address.
     */
    for (next = 0; (listen = mprGetNextItem(server->listens, &next)) != 0; ) {
        address = (MaHostAddress*) maLookupHostAddress(server, listen->ipAddr, listen->port);
        if (address == 0) {
            address = maCreateHostAddress(server, listen->ipAddr, listen->port);
            mprAddItem(server->hostAddresses, address);
        }
        maInsertVirtualHost(address, defaultHost);
    }

    /*
     *  Define a ServerName if one has not been defined. We take the first non loopback listening address.
     */
    natServerName = 0;
    needServerName = strcmp(defaultHost->name, "Main Server") == 0;
    for (next = 0; (listen = mprGetNextItem(server->listens, &next)) != 0; ) {
        ipAddr = listen->ipAddr;
        if (needServerName && *ipAddr != '\0') {
            /*
             *  Try to get the most accessible server name possible.
             */
            if (strncmp(ipAddr, "127.", 4) == 0 || strncmp(ipAddr, "localhost:", 10) == 0) {
                if (! natServerName) {
                    maSetHostName(defaultHost, ipAddr);
                    needServerName = 0;
                }
            } else {
                if (strncmp(ipAddr, "10.", 3) == 0 || strncmp(ipAddr, "192.168.", 8) == 0 || 
                        strncmp(ipAddr, "172.16.", 7) == 0) {
                    natServerName = 1;
                } else {
                    maSetHostName(defaultHost, ipAddr);
                    needServerName = 0;
                }
            }
        }
    }

    /*
     *  Last try to setup the server name if we don't have a non-local name.
     */
    if (needServerName && !natServerName) {
        /*
         *  This code is undesirable as it makes us dependent on DNS -- bad
         */
        if (natServerName) {
            cchar *hostName = mprGetServerName(http);
            mprLog(server, 0, "WARNING: Missing ServerName directive, doing DNS lookup.");
            listen = mprGetFirstItem(server->listens);
            mprSprintf(ipAddrPort, sizeof(ipAddrPort), "%s:%d", hostName, listen->port);
            maSetHostName(defaultHost, hostName);

        } else {
            maSetHostName(defaultHost, defaultHost->ipAddrPort);
        }
        mprLog(server, 2, "Missing ServerName directive, ServerName set to: \"%s\"", defaultHost->name);
    }

    if (mprHasAllocError(mpr)) {
        mprError(server, "Memory allocation error when initializing");
        return MPR_ERR_NO_MEMORY;
    }
    return 0;

syntaxErr:
    mprError(server, "Syntax error in %s at %s:%d", configFile, state->filename, state->lineNumber);
    return MPR_ERR_BAD_SYNTAX;

err:
    mprError(server, "Error in %s at %s:%d", configFile, state->filename, state->lineNumber);
    return MPR_ERR_BAD_SYNTAX;
}


/*
 *  Process the configuration settings. Permissible to modify key and value.
 *  Return < 0 for errors, zero if directive not found, otherwise 1 is success.
 *
 *  TODO -- this function is quite big. Could be subject to a FEATURE.
 */
static int processSetting(MaServer *server, char *key, char *value, MaConfigState *state)
{
    MaHttp          *http;
    MaAuth          *auth;
    MaAlias         *alias;
    MaStage         *stage;
    MaLocation      *location;
    MaHost          *host;
    MaDir           *dir;
    MaLimits        *limits;
    MprHash         *hp;
    char            ipAddrPort[MPR_MAX_IP_ADDR_PORT], pathBuf[MPR_MAX_FNAME], prefixBuf[MPR_MAX_FNAME];
    char            *name, *path, *prefix, *cp, *tok, *ext, *mimeType, *url, *newUrl, *extensions, *codeStr, *hostName;
    int             port, rc, code, processed, num, flags, colonCount;

    mprAssert(state);
    mprAssert(key);

    http = server->http;
    host = state->host;
    dir = state->dir;
    location = state->location;
    mprAssert(state->location->prefix);

    mprAssert(host);
    mprAssert(dir);
    auth = state->auth;
    processed = 0;
    limits = host->limits;
    flags = 0;

    //  TODO BUG - crashes with missing value
    //  TODO - need a real parser
    switch (toupper((int) key[0])) {
    case 'A':
        if (mprStrcmpAnyCase(key, "Alias") == 0) {
            /* Scope: server, host */
            if (maSplitConfigValue(server, &prefix, &path, value, 1) < 0) {
                return MPR_ERR_BAD_SYNTAX;
            }
            if (strchr(prefix, '$') != 0) {
                /*
                 *  Allow $ in the prefix side also
                 */
                maReplaceReferences(host, prefixBuf, sizeof(prefixBuf), prefix);
                prefix = prefixBuf;
            }
            if (maMakePath(host, pathBuf, sizeof(pathBuf), path) == 0) {
                mprError(server, "Alias path is too long");
                return MPR_ERR_BAD_SYNTAX;
            }
            if (prefix[strlen(prefix) - 1] == '/' &&
                pathBuf[strlen(pathBuf) - 1] != '/') {
                mprError(server, "May be missing a trailing '/' on the Alias path \"%s\". "
                    "In configuration file at %s:%d", path, state->filename, state->lineNumber);
            }
            if (maLookupAlias(host, prefix)) {
                mprError(server, "Alias \"%s\" already exists", prefix);
                return MPR_ERR_BAD_SYNTAX;
            }
            alias = maCreateAlias(host, prefix, pathBuf, 0);
            mprLog(server, 4, "Alias: \"%s for \"%s\"", prefix, pathBuf);
            if (maInsertAlias(host, alias) < 0) {
                mprError(server, "Can't insert alias: %s", prefix);
                return MPR_ERR_BAD_SYNTAX;
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "AddFilter") == 0) {
            /* Scope: server, host, location */
            name = mprStrTok(value, " \t", &extensions);
            if (maAddFilter(http, location, name, extensions, MA_FILTER_INCOMING | MA_FILTER_OUTGOING) < 0) {
                mprError(server, "Can't add filter %s", name);
                return MPR_ERR_CANT_CREATE;
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "AddInputFilter") == 0) {
            /* Scope: server, host, location */
            name = mprStrTok(value, " \t", &extensions);
            if (maAddFilter(http, location, name, extensions, MA_FILTER_INCOMING) < 0) {
                mprError(server, "Can't add filter %s", name);
                return MPR_ERR_CANT_CREATE;
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "AddOutputFilter") == 0) {
            /* Scope: server, host, location */
            name = mprStrTok(value, " \t", &extensions);
            if (maAddFilter(http, location, name, extensions, MA_FILTER_OUTGOING) < 0) {
                mprError(server, "Can't add filter %s", name);
                return MPR_ERR_CANT_CREATE;
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "AddHandler") == 0) {
            /* Scope: server, host, location */
            name = mprStrTok(value, " \t", &extensions);
            if (maAddHandler(http, state->location, name, extensions) < 0) {
                mprError(server, "Can't add handler %s", name);
                return MPR_ERR_CANT_CREATE;
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "AddType") == 0) {
            if (maSplitConfigValue(server, &mimeType, &ext, value, 1) < 0) {
                return MPR_ERR_BAD_SYNTAX;
            }
            maAddMimeType(host, ext, mimeType);
            return 1;

#if BLD_FEATURE_AUTH
        } else if (mprStrcmpAnyCase(key, "Allow") == 0) {
            char *from, *spec;
            if (maSplitConfigValue(server, &from, &spec, value, 1) < 0) {
                return MPR_ERR_BAD_SYNTAX;
            }
            /* spec can be: all, host, ipAddr */
            maSetAuthAllow(auth, spec);
            return 1;
#endif
        }
        break;

    case 'B':
        if (mprStrcmpAnyCase(key, "BrowserMatch") == 0) {
            return 1;
        }
        break;

    case 'C':
        if (mprStrcmpAnyCase(key, "CustomLog") == 0) {
#if BLD_FEATURE_ACCESS_LOG && !BLD_FEATURE_ROMFS
            char *format, *end;
            if (*value == '\"') {
                end = strchr(++value, '\"');
                if (end == 0) {
                    mprError(server, "Missing closing quote");
                    return MPR_ERR_BAD_SYNTAX;
                }
                *end++ = '\0';
                path = value;
                format = end;
                while (isspace(*format)) {
                    format++;
                }

            } else {
                path = mprStrTok(value, " \t", &format);
            }
            if (path == 0 || format == 0) {
                return MPR_ERR_BAD_SYNTAX;
            }
            if (maMakePath(host, pathBuf, sizeof(pathBuf), path) == 0) {
                mprError(server, "CustomLog path is too long");
                return MPR_ERR_BAD_SYNTAX;
            }
            maSetAccessLog(host, pathBuf, mprStrTrim(format, "\""));
            maSetLogHost(host, host);
#endif
            return 1;
        }
        break;

    case 'D':
        if (0) {
#if BLD_FEATURE_AUTH
        } else if (mprStrcmpAnyCase(key, "Deny") == 0) {
            char *from, *spec;
            if (maSplitConfigValue(server, &from, &spec, value, 1) < 0) {
                return MPR_ERR_BAD_SYNTAX;
            }
            maSetAuthDeny(auth, spec);
            return 1;
#endif

        } else if (mprStrcmpAnyCase(key, "DirectoryIndex") == 0) {
            value = mprStrTrim(value, "\"");
            if (dir == 0) {
                return MPR_ERR_BAD_SYNTAX;
            }
            maSetDirIndex(dir, value);
            return 1;

        } else if (mprStrcmpAnyCase(key, "DocumentRoot") == 0) {
            value = mprStrTrim(value, "\"");
            if (maMakePath(host, pathBuf, sizeof(pathBuf), value) == 0) {
                mprError(server, "DocumentRoot is too long");
                return MPR_ERR_BAD_SYNTAX;
            }
#if !VXWORKS
            /*
             *  VxWorks stat() is broken if using a network FTP server.
             */
            if (!mprAccess(server, pathBuf, X_OK)) {
                mprError(server, "Can't access DocumentRoot directory");
                return MPR_ERR_BAD_SYNTAX;
            }
#endif
            maSetDocumentRoot(host, pathBuf);
            maSetDirPath(dir, pathBuf);
            mprLog(server, MPR_CONFIG, "Document Root for %s is \"%s\"", host->name, pathBuf);
            return 1;
        }
        break;

    case 'E':
        if (mprStrcmpAnyCase(key, "ErrorDocument") == 0) {
            codeStr = mprStrTok(value, " \t", &url);
            if (codeStr == 0 || url == 0) {
                mprError(server, "Bad ErrorDocument directive");
                return MPR_ERR_BAD_SYNTAX;
            }
            maAddErrorDocument(location, codeStr, url);
            return 1;

        } else if (mprStrcmpAnyCase(key, "ErrorLog") == 0) {
            path = mprStrTrim(value, "\"");
            if (path && *path) {
#if BLD_FEATURE_LOG
                if (server->alreadyLogging) {
                    mprLog(server, 4, "Already logging. Ignoring ErrorLog directive");
                } else {
                    maStopLogging(server);

                    if (strncmp(path, "stdout", 6) == 0) {
                        mprStrcpy(pathBuf, sizeof(pathBuf), path);

                    } else if (maMakePath(host, pathBuf, sizeof(pathBuf), path) == 0) {
                        mprError(server, "ErrorLog path is too long");
                        return MPR_ERR_BAD_SYNTAX;
                    }
                    if (strlen(pathBuf) < sizeof(pathBuf) - 3) {
                        strcat(pathBuf, ":2");
                    }
                    if (maStartLogging(server, pathBuf) < 0) {
                        mprError(server, "Can't log errors to %s", pathBuf);
                        return MPR_ERR_BAD_SYNTAX;
                    }
                }
#endif
            }
            return 1;
        }
        break;

    case 'G':
        if (mprStrcmpAnyCase(key, "Group") == 0) {
            value = mprStrTrim(value, "\"");
            maSetHttpGroup(http, value);
            return 1;
        }
        break;

    case 'H':
        break;

    case 'K':
        if (mprStrcmpAnyCase(key, "KeepAlive") == 0) {
            if (mprStrcmpAnyCase(value, "on") == 0) {
                maSetKeepAlive(host, 1);
            } else {
                maSetKeepAlive(host, 0);
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "KeepAliveTimeout") == 0) {
            if (! mprGetDebugMode(server)) {
                maSetKeepAliveTimeout(host, atoi(value) * 1000);
            }
            return 1;
        }
        break;

    case 'L':
        if (mprStrcmpAnyCase(key, "LimitChunkSize") == 0) {
            num = atoi(value);
            if (num < MA_BOT_CHUNK_SIZE || num > MA_TOP_CHUNK_SIZE) {
                return MPR_ERR_BAD_SYNTAX;
            }
            limits->maxChunkSize = num;
            return 1;

        } else if (mprStrcmpAnyCase(key, "LimitClients") == 0) {
            mprSetMaxSocketClients(server, atoi(value));
            return 1;

        } else if (mprStrcmpAnyCase(key, "LimitRequestBody") == 0) {
            num = atoi(value);
            if (num < MA_BOT_BODY || num > MA_TOP_BODY) {
                return MPR_ERR_BAD_SYNTAX;
            }
            limits->maxBody = num;
            return 1;

        } else if (mprStrcmpAnyCase(key, "LimitRequestFields") == 0) {
            num = atoi(value);
            if (num < MA_BOT_NUM_HEADERS || num > MA_TOP_NUM_HEADERS) {
                return MPR_ERR_BAD_SYNTAX;
            }
            limits->maxNumHeaders = num;
            return 1;

        } else if (mprStrcmpAnyCase(key, "LimitRequestFieldSize") == 0) {
            num = atoi(value);
            if (num < MA_BOT_HEADER || num > MA_TOP_HEADER){
                return MPR_ERR_BAD_SYNTAX;
            }
            limits->maxHeader = num;
            return 1;

        } else if (mprStrcmpAnyCase(key, "LimitResponseBody") == 0) {
            num = atoi(value);
            if (num < MA_BOT_RESPONSE_BODY || num > MA_TOP_RESPONSE_BODY) {
                return MPR_ERR_BAD_SYNTAX;
            }
            limits->maxResponseBody = num;
            return 1;

        } else if (mprStrcmpAnyCase(key, "LimitStageBuffer") == 0) {
            num = atoi(value);
            if (num < MA_BOT_STAGE_BUFFER || num > MA_TOP_STAGE_BUFFER) {
                return MPR_ERR_BAD_SYNTAX;
            }
            limits->maxStageBuffer = num;
            return 1;

        } else if (mprStrcmpAnyCase(key, "LimitUrl") == 0) {
            num = atoi(value);
            if (num < MA_BOT_URL || num > MA_TOP_URL){
                return MPR_ERR_BAD_SYNTAX;
            }
            limits->maxUrl = num;
            return 1;

        } else if (mprStrcmpAnyCase(key, "LimitUploadSize") == 0) {
            num = atoi(value);
            if (num != -1 && (num < MA_BOT_UPLOAD_SIZE || num > MA_TOP_URL)){
                return MPR_ERR_BAD_SYNTAX;
            }
            limits->maxUploadSize = num;
            return 1;

#if DEPRECATED
        } else if (mprStrcmpAnyCase(key, "ListenIF") == 0) {
            MprList         *ipList;
            MprInterface    *ip;

            /*
             *  Options:
             *      interface:port
             *      interface   (default port MA_SERVER_DEFAULT_PORT_NUM)
             */
            if ((cp = strchr(value, ':')) != 0) {           /* interface:port */
                do {                                        /* find last colon */
                    tok = cp;
                    cp = strchr(cp + 1, ':');
                } while (cp != 0);
                cp = tok;
                *cp++ ='\0';

                port = atoi(cp);
                if (port <= 0 || port > 65535) {
                    mprError(server, "Bad listen port number %d", port);
                    return MPR_ERR_BAD_SYNTAX;
                }

            } else {            /* interface */
                port = MA_SERVER_DEFAULT_PORT_NUM;
            }

            ipList = mprGetMpr()->socketService->getInterfaceList();
            ip = (MprInterface*) ipList->getFirst();
            if (ip == 0) {
                mprError(server, "Can't find interfaces, use Listen-directive with IP address.");
                return MPR_ERR_BAD_SYNTAX;
            }

            while (ip) {
                if (mprStrcmpAnyCase(ip->name, value) != 0) {
                    ip = (MprInterface*) ipList->getNext(ip);
                    continue;
                }

                listens.insert(new MaListen(ip->ipAddr, port));
                if (host->ipAddrPort == 0) {
                    mprSprintf(ipAddrPort, sizeof(ipAddrPort), "%s:%d", ip->ipAddr, port);
                    maSetIpAddrPort(host, ipAddrPort);
                }
                break;
            }
            return 1;
#endif

        } else if (mprStrcmpAnyCase(key, "Listen") == 0) {

            /*
             *  Options:
             *      ipAddr:port
             *      ipAddr          default port MA_SERVER_DEFAULT_PORT_NUM
             *      port            All ip interfaces on this port
             *
             *  Where ipAddr may be "::::::" for ipv6 addresses or may be enclosed in "[]" if appending a port.
             */

            value = mprStrTrim(value, "\"");

            if (isdigit((int) *value) && strchr(value, '.') == 0 && strchr(value, ':') == 0) {
                /*
                 *  Port only, listen on all interfaces (ipv4 + ipv6)
                 */
                port = atoi(value);
                if (port <= 0 || port > 65535) {
                    mprError(server, "Bad listen port number %d", port);
                    return MPR_ERR_BAD_SYNTAX;
                }
                hostName = "";
                flags = MA_LISTEN_WILD_IP;

            } else {
                colonCount = 0;
                for (cp = (char*) ipAddrPort; ((*cp != '\0') && (colonCount < 2)) ; cp++) {
                    if (*cp == ':') {
                        colonCount++;
                    }
                }
                /*
                 *  Hostname with possible port number. Port number parsed if address enclosed in "[]"
                 */
                if (colonCount > 1) {
                    /* ipv6 */
                    hostName = value;
                    if (*value == '[' && (cp = strrchr(cp, ':')) != 0) {
                        *cp++ = '\0';
                        port = atoi(cp);
                    } else {
                        port = MA_SERVER_DEFAULT_PORT_NUM;
                        flags = MA_LISTEN_DEFAULT_PORT;
                    }

                } else {
                    /* ipv4 */
                    hostName = value;
                    if ((cp = strrchr(value, ':')) != 0) {
                        *cp++ = '\0';
                        port = atoi(cp);

                    } else {
                        port = MA_SERVER_DEFAULT_PORT_NUM;
                        flags = MA_LISTEN_DEFAULT_PORT;
                    }
                }
            }

            //  TODO - need api insead of mprAddItem
            mprAddItem(server->listens, maCreateListen(server, hostName, port, flags));

            /*
             *  Set the host ip spec if not already set
             */
            if (host->ipAddrPort == 0) {
                mprSprintf(ipAddrPort, sizeof(ipAddrPort), "%s:%d", hostName, port);
                maSetHostIpAddrPort(host, ipAddrPort);
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "LogFormat") == 0) {
            return 1;

        } else if (mprStrcmpAnyCase(key, "LogLevel") == 0) {
#if BLD_FEATURE_LOG
            if (server->alreadyLogging) {
                mprLog(server, 4, "Already logging. Ignoring LogLevel directive");

            } else {
                int level;
                value = mprStrTrim(value, "\"");
                level = atoi(value);
                mprSetLogLevel(server, level);
            }
#endif
            return 1;

        } else if (mprStrcmpAnyCase(key, "LoadModulePath") == 0) {
            value = mprStrTrim(value, "\"");
            mprSetModuleSearchPath(server, value);
            return 1;

        } else if (mprStrcmpAnyCase(key, "LoadModule") == 0) {
            name = mprStrTok(value, " \t", &tok);
            if (name == 0) {
                return MPR_ERR_BAD_SYNTAX;
            }
            path = mprStrTok(0, "\n", &tok);
            if (path == 0) {
                return MPR_ERR_BAD_SYNTAX;
            }
            if (maLoadModule(http, name, path) < 0) {
                /*  Error messages already done */
                return MPR_ERR_CANT_CREATE;
            }
            return 1;
        }
        break;

    case 'M':
        if (mprStrcmpAnyCase(key, "MaxKeepAliveRequests") == 0) {
            maSetMaxKeepAlive(host, atoi(value));
            return 1;
        }
        break;

    case 'N':
        if (mprStrcmpAnyCase(key, "NameVirtualHost") == 0) {
            mprLog(server, 2, "NameVirtual Host: %s ", value);
            if (maCreateHostAddresses(server, 0, value) < 0) {
                return -1;
            }
            return 1;
        }
        break;

    case 'O':
        if (0) {
#if BLD_FEATURE_AUTH
        } else if (mprStrcmpAnyCase(key, "Order") == 0) {
            if (mprStrcmpAnyCase(mprStrTrim(value, "\""), "Allow,Deny") == 0) {
                maSetAuthOrder(auth, MA_ALLOW_DENY);
            } else {
                maSetAuthOrder(auth, MA_DENY_ALLOW);
            }
            return 1;
#endif
        }
        break;

    case 'P':
        if (mprStrcmpAnyCase(key, "Protocol") == 0) {
            if (strcmp(value, "HTTP/1.0") == 0) {
                maSetHttpVersion(host, MPR_HTTP_1_0);

            } else if (strcmp(value, "HTTP/1.1") == 0) {
                maSetHttpVersion(host, MPR_HTTP_1_1);
            }
            return 1;
        }
        break;

    case 'R':
        if (mprStrcmpAnyCase(key, "Redirect") == 0) {
            if (value[0] == '/' || value[0] == 'h') {
                code = 302;
                url = mprStrTok(value, " \t", &tok);

            } else if (isdigit((int) value[0])) {
                cp = mprStrTok(value, " \t", &tok);
                code = atoi(cp);
                url = mprStrTok(0, " \t\n", &tok);

            } else {
                cp = mprStrTok(value, " \t", &tok);
                if (strcmp(value, "permanent") == 0) {
                    code = 301;
                } else if (strcmp(value, "temp") == 0) {
                    code = 302;
                } else if (strcmp(value, "seeother") == 0) {
                    code = 303;
                } else if (strcmp(value, "gone") == 0) {
                    code = 410;
                } else {
                    return MPR_ERR_BAD_SYNTAX;
                }
                url = mprStrTok(0, " \t\n", &tok);
            }
            if (code >= 300 && code <= 399) {
                newUrl = mprStrTok(0, "\n", &tok);
            } else {
                newUrl = "";
            }
            if (code <= 0 || url == 0 || newUrl == 0) {
                return MPR_ERR_BAD_SYNTAX;
            }
            url = mprStrTrim(url, "\"");
            newUrl = mprStrTrim(newUrl, "\"");
            mprLog(server, 4, "insertAlias: Redirect %d from \"%s\" to \"%s\"", code, url, newUrl);
            alias = maCreateAlias(host, url, newUrl, code);
            maInsertAlias(host, alias);
            return 1;

        } else if (mprStrcmpAnyCase(key, "ResetPipeline") == 0) {
            maResetPipeline(location);
            return 1;
        }
        break;

    case 'S':
        if (mprStrcmpAnyCase(key, "ServerName") == 0) {
            value = mprStrTrim(value, "\"");
            if (strncmp(value, "http://", 7) == 0) {
                maSetHostName(host, &value[7]);
            } else {
                maSetHostName(host, value);
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "ServerRoot") == 0) {
            value = mprStrTrim(value, "\"");
            if (maMakePath(host, pathBuf, sizeof(pathBuf), value) == 0) {
                mprError(server, "ServerRoot is too long");
                return MPR_ERR_BAD_SYNTAX;
            }
#if !VXWORKS
            /*
             *  VxWorks stat() is broken if using a network FTP server.
             */
            if (! mprAccess(server, pathBuf, R_OK)) {
                mprError(server, "Can't access ServerRoot directory");
                return MPR_ERR_BAD_SYNTAX;
            }
#endif
            maSetServerRoot(server, pathBuf);
#if BLD_FEATURE_ROMFS
            mprLog(server, MPR_CONFIG, "Server Root \"%s\" in ROM", pathBuf);
#else
            mprLog(server, MPR_CONFIG, "Server Root \"%s\"", pathBuf);
#endif
            return 1;

        } else if (mprStrcmpAnyCase(key, "SetConnector") == 0) {
            /* Scope: server, host, location */
            value = mprStrTrim(value, "\"");
            if (maSetConnector(http, location, value) < 0) {
                mprError(server, "Can't add handler %s", value);
                return MPR_ERR_CANT_CREATE;
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "SetHandler") == 0) {
            /* Scope: server, host, location */
            name = mprStrTok(value, " \t", &extensions);
            if (maSetHandler(http, host, state->location, name) < 0) {
                mprError(server, "Can't add handler %s", name);
                return MPR_ERR_CANT_CREATE;
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "StartThreads") == 0) {
#if BLD_FEATURE_MULTITHREAD
            num = atoi(value);
            if (num < 0 || num > MA_TOP_THREADS) {
                return MPR_ERR_BAD_SYNTAX;
            }
            limits->minThreads = num;
#endif
            return 1;
        }
        break;

    case 'T':
        if (mprStrcmpAnyCase(key, "ThreadLimit") == 0) {
#if BLD_FEATURE_MULTITHREAD
            num = atoi(value);
            if (num < 0 || num > MA_TOP_THREADS) {
                return MPR_ERR_BAD_SYNTAX;
            }
            limits->maxThreads = num;
#endif
            return 1;

        } else if (mprStrcmpAnyCase(key, "ThreadStackSize") == 0) {
#if BLD_FEATURE_MULTITHREAD
            num = atoi(value);
            if (num < MA_BOT_STACK || num > MA_TOP_STACK) {
                return MPR_ERR_BAD_SYNTAX;
            }
            mprSetThreadStackSize(server, num);
            return 1;
#endif

        } else if (mprStrcmpAnyCase(key, "TimeOut") == 0) {
            if (! mprGetDebugMode(server)) {
                maSetTimeout(host, atoi(value) * 1000);
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "TraceMethod") == 0) {
            if (mprStrcmpAnyCase(value, "on") == 0) {
                maSetTraceMethod(host, 1);
            } else {
                maSetTraceMethod(host, 0);
            }
            return 1;

        } else if (mprStrcmpAnyCase(key, "TypesConfig") == 0) {
            path = mprStrTrim(value, "\"");
            if (maMakePath(host, pathBuf, sizeof(pathBuf), path) == 0) {
                mprError(server, "TypesConfig path is too long");
                maAddStandardMimeTypes(host);
                return MPR_ERR_BAD_SYNTAX;
            }
            if (maOpenMimeTypes(host, pathBuf) < 0) {
                mprError(server, "Can't open TypesConfig mime file %s", pathBuf);
                maAddStandardMimeTypes(host);
                return MPR_ERR_BAD_SYNTAX;
            }
            return 1;
        }
        break;

    case 'U':
        if (mprStrcmpAnyCase(key, "User") == 0) {
            maSetHttpUser(http, mprStrTrim(value, "\""));
            return 1;
        }
        break;
    }

    rc = 0;

    /*
     *  Allow all stages to parse the request
     */
    hp = mprGetFirstHash(http->stages);
    while (hp) {
        stage = (MaStage*) hp->data;
        if (stage->parse) {
            rc = stage->parse(http, key, value, state);
        }
        if (rc < 0) {
            return rc;
        } else if (rc > 0) {
            break;
        }
        hp = mprGetNextHash(http->stages, hp);
    }

    return rc;
}


/*
 *  Create a location block for a handler and an alias. Convenience routine for ScriptAlias, EjsAppAlias, EjsAppDirAlias.
 */
MaLocation *maCreateLocationAlias(MaHttp *http, MaConfigState *state, cchar *prefix, cchar *path, cchar *handlerName, 
        int flags)
{
    MaHost          *host;
    MaAlias         *alias;
    MaLocation      *location;
    char            pathBuf[MPR_MAX_FNAME], prefixBuf[MPR_MAX_FNAME];

    host = state->host;

    /*
     *  Allow $references on the prefix side
     */
    maReplaceReferences(host, prefixBuf, sizeof(prefixBuf), prefix);
    prefix = prefixBuf;

    if (maMakePath(host, pathBuf, sizeof(pathBuf), path) == 0) {
        mprError(http, "Alias path is too long");
        return 0;
    }

    if (prefix[strlen(prefix) - 1] == '/' && pathBuf[strlen(pathBuf) - 1] != '/') {
        mprError(http, "May be missing a trailing '/' on the Alias path \"%s\". "
            "In configuration file at %s:%d", path, state->filename, state->lineNumber);
    }

    /*
     *  Create an ejs application location block and alias
     */
    alias = maCreateAlias(host, prefix, pathBuf, 0);
    maInsertAlias(host, alias);
    mprLog(http, 4, "Alias \"%s\" for \"%s\"", prefix, pathBuf);

    if (maLookupLocation(host, prefix)) {
        mprError(http, "Location block already exists for \"%s\"", prefix);
        return 0;
    }
    location = maCreateLocation(host, state->location);
    maSetLocationAuth(location, state->dir->auth);
    maSetLocationPrefix(location, prefix);
    maAddLocation(host, location);
    maSetLocationFlags(location, flags);
    maSetHandler(http, host, location, handlerName);

    return location;
}


/*
 *  Make a path name. This replaces $references, converts to an absolute path name, cleans the path and maps delimiters.
 */
char *maMakePath(MaHost *host, char *path, int pathsize, cchar *file)
{
    MaServer    *server;
    char        buf[MPR_MAX_FNAME], *tmp;

    mprAssert(file);

    server = host->server;

    if (maReplaceReferences(host, path, pathsize, file) == 0) {
        /*  Overflow */
        return 0;
    }

    if (*path == '\0' || strcmp(path, ".") == 0) {
        mprStrcpy(buf, sizeof(buf), server->serverRoot);

#if WIN
    } else if (*path != '/' && path[1] != ':' && path[2] != '/') {
        mprSprintf(buf, sizeof(buf), "%s/%s", server->serverRoot, path);
#elif VXWORKS
    } else if (strchr((path), ':') == 0 && *path != '/') {
        mprSprintf(buf, sizeof(buf), "%s/%s", server->serverRoot, path);
#else
    } else if (*path != '/') {
        mprSprintf(buf, sizeof(buf), "%s/%s", server->serverRoot, path);
#endif

    } else {
        mprStrcpy(buf, sizeof(buf), path);
    }

    /*
     *  Convert to an fully qualified name (on windows).
     */
    tmp = mprGetAbsFilename(server, buf);
    tmp = mprCleanFilename(tmp, tmp);
    mprStrcpy(path, pathsize, tmp);
    mprMapDelimiters(host, path, '/');
    mprFree(tmp);

    return path;
}


/*
 *  Replace a limited set of $VAR references. Currently support DOCUMENT_ROOT, SERVER_ROOT and PRODUCT
 *  TODO - Expand and formalize this. Should support many more variables.
 */
char *maReplaceReferences(MaHost *host, char *buf, int buflen, cchar *str)
{
    cchar   *src;
    char    *dest, *key, *root;
    int     len;

    dest = buf;
    buflen--;
    for (src = str; *src && buflen > 0; ) {
        if (*src == '$') {
            *dest = '\0';
            len = 0;
            key = "DOCUMENT_ROOT";
            if (strncmp(++src, key, strlen(key)) == 0 ) {
                root = host->documentRoot;
                if (root) {
                    len = mprStrcpy(dest, buflen, root);
                }
            } else {
                key = "SERVER_ROOT";
                if (strncmp(src, key, strlen(key)) == 0) {
                    len = mprStrcpy(dest, buflen, host->server->serverRoot);
                } else {
                    key = "PRODUCT";
                    if (strncmp(src, key, strlen(key)) == 0) {
                        len = mprStrcpy(dest, buflen, BLD_PRODUCT);
                    }
                }
            }
            if (*dest) {
                if (len > 0 && dest[len - 1] == '/') {
                    dest[len - 1] = '\0';
                }
                len = (int) strlen(dest);
                dest += len;
                buflen -= len;
                src += strlen(key);
                continue;
            }
        }
        *dest++ = *src++;
        buflen--;
    }

    if (buflen <= 0) {
        return 0;
    }

    *dest = '\0';

    return buf;
}


/*
 *  Max stack depth is:
 *      Default Server          Level 1
 *          <VirtualHost>       Level 2
 *              <Directory>     Level 3
 *                  <Location>  Level 4
 *
 */
static MaConfigState *pushState(MprCtx ctx, MaConfigState *state, int *top)
{
    MaConfigState   *next;

    (*top)++;
    if (*top >= MA_MAX_CONFIG_DEPTH) {
        mprError(ctx, "Too many nested directives in config file at %s:%d", state->filename, state->lineNumber);
        return 0;
    }
    next = state + 1;
    next->server = state->server;
    next->host = state->host;
    next->location = state->location;
    next->dir = state->dir;
    next->auth = state->auth;
    next->lineNumber = state->lineNumber;
    next->enabled = state->enabled;
    next->filename = state->filename;
    next->file = state->file;

    return next;
}


static bool featureSupported(MprCtx ctx, char *key)
{
    //  TODO - should always create a conditional directive when a module is loaded. Even for user modules.
    if (mprStrcmpAnyCase(key, "BLD_COMMERCIAL") == 0) {
        return strcmp(BLD_COMMERCIAL, "0") == 0;

#ifdef BLD_DEBUG
    } else if (mprStrcmpAnyCase(key, "BLD_DEBUG") == 0) {
        return BLD_DEBUG;
#endif

#ifdef BLD_FEATURE_ACCESS_LOG
    } else if (mprStrcmpAnyCase(key, "ACCESS_LOG") == 0) {
        return BLD_FEATURE_ACCESS_LOG;
#endif

#ifdef BLD_FEATURE_AUTH
    } else if (mprStrcmpAnyCase(key, "AUTH_MODULE") == 0) {
        return BLD_FEATURE_AUTH;
#endif

#ifdef BLD_FEATURE_CGI
    } else if (mprStrcmpAnyCase(key, "CGI_MODULE") == 0) {
        return BLD_FEATURE_CGI;
#endif

#ifdef BLD_FEATURE_CHUNK
    } else if (mprStrcmpAnyCase(key, "CHUNK_MODULE") == 0) {
        return BLD_FEATURE_CHUNK;
#endif

#ifdef BLD_FEATURE_AUTH_DIGEST
    } else if (mprStrcmpAnyCase(key, "DIGEST") == 0) {
        return BLD_FEATURE_AUTH_DIGEST;
#endif

#ifdef BLD_FEATURE_DIR
    } else if (mprStrcmpAnyCase(key, "DIR_MODULE") == 0) {
        return BLD_FEATURE_DIR;
#endif

#ifdef BLD_FEATURE_DOC
    } else if (mprStrcmpAnyCase(key, "DOC") == 0) {
        return BLD_FEATURE_DOC;
#endif

#ifdef BLD_FEATURE_EGI
    } else if (mprStrcmpAnyCase(key, "EGI_MODULE") == 0) {
        return BLD_FEATURE_EGI;
#endif

#ifdef BLD_FEATURE_EJS
    } else if (mprStrcmpAnyCase(key, "EJS_MODULE") == 0) {
        return BLD_FEATURE_EJS;
#endif

#ifdef BLD_FEATURE_FILE
    } else if (mprStrcmpAnyCase(key, "FILE_MODULE") == 0) {
        return BLD_FEATURE_FILE;
#endif

#ifdef BLD_FEATURE_LOG
    } else if (mprStrcmpAnyCase(key, "LOG") == 0) {
        return BLD_FEATURE_LOG;
#endif

#ifdef BLD_FEATURE_MULTITHREAD
    } else if (mprStrcmpAnyCase(key, "MULTITHREAD") == 0) {
        return BLD_FEATURE_MULTITHREAD;
#endif

#ifdef BLD_FEATURE_NET
    } else if (mprStrcmpAnyCase(key, "NET") == 0) {
        return BLD_FEATURE_NET;
#endif

#ifdef BLD_FEATURE_PHP
    } else if (mprStrcmpAnyCase(key, "PHP_MODULE") == 0) {
        return BLD_FEATURE_PHP;
#endif

#ifdef BLD_FEATURE_RANGE
    } else if (mprStrcmpAnyCase(key, "RANGE_MODULE") == 0) {
        return BLD_FEATURE_RANGE;
#endif

#ifdef BLD_FEATURE_SAMPLES
    } else if (mprStrcmpAnyCase(key, "SAMPLES") == 0) {
        return BLD_FEATURE_SAMPLES;
#endif

#ifdef BLD_FEATURE_SEND
    } else if (mprStrcmpAnyCase(key, "SEND") == 0) {
        return BLD_FEATURE_SEND;
#endif

#ifdef BLD_FEATURE_SSL
    } else if (mprStrcmpAnyCase(key, "SSL_MODULE") == 0) {
        return BLD_FEATURE_SSL;
#endif

#ifdef BLD_FEATURE_TEST
    } else if (mprStrcmpAnyCase(key, "TEST") == 0) {
        return BLD_FEATURE_TEST;
#endif

#ifdef BLD_FEATURE_UPLOAD
    } else if (mprStrcmpAnyCase(key, "UPLOAD_MODULE") == 0) {
        return BLD_FEATURE_UPLOAD;
#endif
    }

    mprError(ctx, "Unknown conditional \"%s\"", key);
    return 0;
}
#endif  /* BLD_FEATURE_CONFIG_PARSE */


#if BLD_FEATURE_CONFIG_SAVE
/*
 *  Save the configuration to the named config file
 */
int MaServer::saveConfig(char *configFile)
{
    MaAlias         *alias;
    MaDir           *dp, *defaultDp;
    MprFile         out;
    MaStage     *hanp;
    MaHost          *host, *defaultHost;
    MaLimits        *limits;
    MaListen        *listen;
    MaLocation      *loc;
    MaMimeHashEntry *mt;
    MprHashTable    *mimeTypes;
    MprList         *aliases;
    char            *ext, *path, *cp, *mimeFile;
    char            *hostName, *actionProgram;
    int             fd, indent, flags, first, code;
#if BLD_FEATURE_LOG
    MprLogService   *logService;
    char            *logSpec;
#endif

    indent = 0;
    host = 0;
    defaultHost = (MaHost*) hosts.getFirst();

    fd = open(configFile, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0666);
    if (fd < 0) {
        mprLog(server, 0, "saveConfig: Can't open %s", configFile);
        return MPR_ERR_CANT_OPEN;
    }

    mprFprintf(fd, \
    "#\n"
    "#  Configuration for %s\n"
    "#\n"
    "#  This file is dynamically generated. If you edit this file, your\n"
    "#  changes may not be preserved by the manager. PLEASE keep a backup of\n"
    "#  the file before and after all manual changes.\n"
    "#\n"
    "#  The order of configuration directives matters as this file is parsed\n"
    "#  only once. You must put the server root and error log definitions\n"
    "#  first ensure configuration errors are logged.\n"
    "#\n\n", BLD_NAME);

    mprFprintf(fd, "ServerRoot \"%s\"\n", serverRoot);

#if BLD_FEATURE_LOG
    logService = mprGetMpr()->logService;
    logSpec = mprStrdup(logService->getLogSpec());
    if ((cp = strchr(logSpec, ':')) != 0) {
        *cp = '\0';
    }
    mprFprintf(fd, "ErrorLog \"%s\"\n", logSpec);
    mprFprintf(fd, "LogLevel %d\n", logService->getDefaultLevel());
#endif

    /*
     *  Listen directives
     */
    listen = (MaListen*) listens.getFirst();
    while (listen) {
        flags = listen->getFlags();
        if (flags & MA_LISTEN_DEFAULT_PORT) {
            mprFprintf(fd, "Listen %s # %d\n", listen->getIpAddr(), listen->getPort());
        } else if (flags & MA_LISTEN_WILD_IP) {
            mprFprintf(fd, "Listen %d\n", listen->getPort());
        } else if (flags & MA_LISTEN_WILD_IP2) {
            /*  Ignore */
        } else {
            if (strchr(listen->getIpAddr(), '.') != 0) {
                mprFprintf(fd, "Listen %s:%d\n", listen->getIpAddr(),
                    listen->getPort());
            } else {
                mprFprintf(fd, "Listen [%s]:%d\n", listen->getIpAddr(),
                    listen->getPort());
            }
        }
        listen = (MaListen*) listens.getNext(listen);
    }

    /*
     *  Global directives
     */
    mprFprintf(fd, "User %s\n", http->getUser());
    mprFprintf(fd, "Group %s\n", http->getGroup());

#if BLD_FEATURE_DLL
    MaModule    *mp;
    MprList     *modules;
    mprFprintf(fd, "\n#\n#  Loadable Modules\n#\n");
    mprFprintf(fd, "LoadModulePath %s\n", defaultHost->getModuleDirs());
    modules = http->getModules();
    mp = (MaModule*) modules->getFirst();
    while (mp) {
        mprFprintf(fd, "LoadModule %s lib%sModule\n", mp->name, mp->name);
        mp = (MaModule*) modules->getNext(mp);
    }
#endif

    /*
     *  For clarity, always print the ThreadLimit even if default.
     */
    limits = http->getLimits();
    mprFprintf(fd, "ThreadLimit %d\n", limits->maxThreads);
    if (limits->threadStackSize != 0) {
        mprFprintf(fd, "ThreadStackSize %d\n", limits->threadStackSize);
    }
    if (limits->minThreads != 0) {
        mprFprintf(fd, "\nStartThreads %d\n", limits->minThreads);
    }
    if (limits->maxBody != MA_MAX_BODY) {
        mprFprintf(fd, "LimitRequestBody %d\n", limits->maxBody);
    }
    if (limits->maxResponseBody != MA_MAX_RESPONSE_BODY) {
        mprFprintf(fd, "LimitResponseBody %d\n", limits->maxResponseBody);
    }
    if (limits->maxNumHeaders != MA_MAX_NUM_HEADERS) {
        mprFprintf(fd, "LimitRequestFields %d\n", limits->maxNumHeaders);
    }
    if (limits->maxHeader != MA_MAX_HEADERS) {
        mprFprintf(fd, "LimitRequestFieldSize %d\n", limits->maxHeader);
    }
    if (limits->maxUrl != MA_MAX_URL) {
        mprFprintf(fd, "LimitUrl %d\n", limits->maxUrl);
    }
    if (limits->maxUploadSize != MA_MAX_UPLOAD_SIZE) {
        mprFprintf(fd, "LimitUploadSize %d\n", limits->maxUploadSize);
    }

    /*
     *  Virtual hosts. The first host is the default server
     */
    host = (MaHost*) hosts.getFirst();
    while (host) {
        mprFprintf(fd, "\n");
        if (host->isVhost()) {
            if (host->isNamedVhost()) {
                mprFprintf(fd, "NameVirtualHost %s\n", host->getIpSpec());
            }
            mprFprintf(fd, "<VirtualHost %s>\n", host->getIpSpec());
            indent++;
        }

        hostName = host->getName();
        if (strcmp(hostName, "default") != 0) {
            tabs(fd, indent);
            mprFprintf(fd, "ServerName http://%s\n", hostName);
        }

        tabs(fd, indent);
        mprFprintf(fd, "DocumentRoot %s\n", host->getDocumentRoot());

        /*
         *  Handlers
         */
        flags = host->getFlags();
        if (flags & MA_ADD_HANDLER) {
            mprFprintf(fd, "\n");
            if (flags & MA_RESET_HANDLERS) {
                tabs(fd, indent);
                mprFprintf(fd, "ResetPipeline\n");
            }
            hanp = (MaStage*) host->getHandlers()->getFirst();
            while (hanp) {
                ext = (char*) (hanp->getExtensions() ?
                    hanp->getExtensions() : "");
                tabs(fd, indent);
                mprFprintf(fd, "AddHandler %s %s\n", hanp->getName(), ext);
                hanp = (MaStage*) host->getHandlers()->getNext(hanp);
            }
        }

#if BLD_FEATURE_SSL
        /*
         *  SSL configuration
         */
        if (host->isSecure()) {
            MaSslConfig *sslConfig;
            MaSslModule *sslModule;

            mprFprintf(fd, "\n");
            tabs(fd, indent);
            mprFprintf(fd, "SSLEngine on\n");
            sslModule = (MaSslModule*) http->findModule("ssl");
            if (sslModule != 0) {
                sslConfig = sslModule->getSslConfig(host->getName());
                if (sslConfig != 0) {

                    tabs(fd, indent);
                    mprFprintf(fd, "SSLCipherSuite %s\n",
                        sslConfig->getCipherSuite());

                    tabs(fd, indent);
                    mprFprintf(fd, "SSLProtocol ");
                    int protoMask = sslConfig->getSslProto();
                    if (protoMask == MA_PROTO_ALL) {
                        mprFprintf(fd, "ALL");
                    } else if (protoMask ==
                        (MA_PROTO_ALL & ~MA_PROTO_SSLV2)) {
                        mprFprintf(fd, "ALL -SSLV2");
                    } else {
                        if (protoMask & MA_PROTO_SSLV2) {
                            mprFprintf(fd, "SSLv2 ");
                        }
                        if (protoMask & MA_PROTO_SSLV3) {
                            mprFprintf(fd, "SSLv3 ");
                        }
                        if (protoMask & MA_PROTO_TLSV1) {
                            mprFprintf(fd, "TLSv1 ");
                        }
                    }
                    mprFprintf(fd, "\n");

                    if ((path = sslConfig->getCertFile()) != 0) {
                        tabs(fd, indent);
                        mprFprintf(fd, "SSLCertificateFile %s\n", path);
                    }
                    if ((path = sslConfig->getKeyFile()) != 0) {
                        tabs(fd, indent);
                        mprFprintf(fd, "SSLCertificateKeyFile %s\n", path);
                    }
                    if (sslConfig->getVerifyClient()) {
                        tabs(fd, indent);
                        mprFprintf(fd, "SSLVerifyClient require\n");
                        if ((path = sslConfig->getCaFile()) != 0) {
                            tabs(fd, indent);
                            mprFprintf(fd, "SSLCaCertificateFile %s\n", path);
                        }
                        if ((path = sslConfig->getCaPath()) != 0) {
                            tabs(fd, indent);
                            mprFprintf(fd, "SSLCertificatePath %s\n", path);
                        }
                    }
                }
            }
        }
#endif
        /*
         *  General per-host directives
         */
        if (! host->getKeepAlive()) {
            tabs(fd, indent);
            mprFprintf(fd, "KeepAlive off\n");
        } else {
            if (host->getMaxKeepAlive() != defaultHost->getMaxKeepAlive()) {
                tabs(fd, indent);
                mprFprintf(fd, "MaxKeepAliveRequests %d\n",
                    host->getMaxKeepAlive());
            }
            if (host->getKeepAliveTimeout() !=
                    defaultHost->getKeepAliveTimeout()) {
                tabs(fd, indent);
                mprFprintf(fd, "KeepAliveTimeout %d\n",
                    host->getKeepAliveTimeout() / 1000);
            }
        }
        mimeFile = host->getMimeFile();
        if (mimeFile && *mimeFile) {
            mprFprintf(fd, "TypesConfig %s\n", mimeFile);
        }
        if (host->getTimeout() != defaultHost->getTimeout()) {
            tabs(fd, indent);
            mprFprintf(fd, "Timeout %d\n", host->getTimeout() / 1000);
        }

        if (host->getSessionTimeout() != defaultHost->getSessionTimeout()) {
            tabs(fd, indent);
            mprFprintf(fd, "SessionTimeout %d\n", host->getSessionTimeout());
        }
#if BLD_FEATURE_ACCESS_LOG && !BLD_FEATURE_ROMFS
        if (host->getLogHost() == host) {
            char    format[MPR_MAX_FNAME * 2];
            char    *fp;
            fp = format;
            format[0] = '\0';
            for (cp = host->getLogFormat();
                    cp && *cp && fp < &format[sizeof(format) - 4]; cp++) {
                if (*cp == '\"') {
                    *fp++ = '\\';
                    *fp++ = *cp;
                } else {
                    *fp++ = *cp;
                }
            }
            *fp++ = '\0';
            tabs(fd, indent);
            mprFprintf(fd, "CustomLog %s \"%s\"\n", host->getLogPath(), format);
        }
#endif

        /*
         *  ActionPrograms. One mimeTypes table is shared among all hosts.
         */
        if (host == defaultHost) {
            mimeTypes = host->getMimeTypes();
            mt = (MaMimeHashEntry*) mimeTypes->getFirst();
            first = 1;
            while (mt) {
                actionProgram = mt->getActionProgram();
                if (actionProgram && *actionProgram) {
                    if (first) {
                        mprFprintf(fd, "\n");
                        first = 0;
                    }
                    tabs(fd, indent);
                    mprFprintf(fd, "Action %s %s\n", mt->getMimeType(),
                        mt->getActionProgram());
                }
                mt = (MaMimeHashEntry*) mimeTypes->getNext(mt);
            }
        }

        /*
         *  Aliases
         */
        aliases = host->getAliases();
        alias = (MaAlias*) aliases->getFirst();
        first = 1;
        while (alias) {
            /*
             *  Must skip the catchall alias which has an empty prefix
             */
            if (alias->getPrefix()[0] != '\0' && !alias->isInherited()) {
                if (first) {
                    mprFprintf(fd, "\n");
                    first = 0;
                }
                tabs(fd, indent);
                code = alias->getRedirectCode();
                if (code != 0) {
                    mprFprintf(fd, "Redirect %d %s \"%s\"\n", code, alias->getPrefix(), alias->getName());
                } else {
                    mprFprintf(fd, "Alias %s \"%s\"\n", alias->getPrefix(), alias->getName());
                }
            }
            alias = (MaAlias*) aliases->getNext(alias);
        }

        /*
         *  Directories -- Do in reverse order
         */
        defaultDp = dp = (MaDir*) host->getDirs()->getLast();
        first = 1;
        while (dp) {
            if (dp->isInherited()) {
                dp = (MaDir*) host->getDirs()->getPrev(dp);
                continue;
            }
            path = dp->getPath();
            if (*path) {
                if (!first) {
                    mprFprintf(fd, "\n");
                    tabs(fd, indent++);
                    mprFprintf(fd, "<Directory %s>\n", dp->getPath());
                }
            }
            if (strcmp(dp->getIndex(), defaultDp->getIndex()) != 0) {
                tabs(fd, indent);
                mprFprintf(fd, "DirectoryIndex %s\n", dp->getIndex());
            }

            printAuth(fd, host, dp, indent);

            if (*path && !first) {
                tabs(fd, --indent);
                mprFprintf(fd, "</Directory>\n");
            }
            first = 0;
            dp = (MaDir*) host->getDirs()->getPrev(dp);
        }

        /*
         *  Locations
         */
        loc = (MaLocation*) host->getLocations()->getLast();
        while (loc) {
            if (loc->isInherited()) {
                loc = (MaLocation*) host->getLocations()->getPrev(loc);
                continue;
            }
            mprFprintf(fd, "\n");
            tabs(fd, indent++);
            mprFprintf(fd, "<Location %s>\n", loc->getPrefix());

            if (loc->getHandlerName()) {
                tabs(fd, indent);
                //  TODO - not supported
                mprFprintf(fd, "SetHandler %s\n", loc->getHandlerName());
            }

            printAuth(fd, host, loc, indent);

            tabs(fd, --indent);
            mprFprintf(fd, "</Location>\n");

            loc = (MaLocation*) host->getLocations()->getPrev(loc);
        }

        /*
         *  Close out the VirtualHosts
         */
        if (host->isVhost()) {
            tabs(fd, --indent);
            mprFprintf(fd, "</VirtualHost>\n");
        }
        host = (MaHost*) hosts.getNext(host);
    }
    close(fd);
    return 0;
}


/*
 *  Print Authorization configuration
 */
static void printAuth(int fd, MaHost *host, MaAuth *auth, int indent)
{
    MaAuthType      authType;
    MaAcl           acl;
    char            *users, *groups, *realm;
#if BLD_FEATURE_AUTH
    MaAuthHandler   *handler;
#endif

    if (auth->isAuthInherited()) {
        return;
    }

#if BLD_FEATURE_AUTH
    handler = (MaAuthHandler*) host->lookupHandler("auth");
    if (handler) {
        char    *path;
        path = handler->getGroupFile();
        if (path) {
            tabs(fd, indent);
            mprFprintf(fd, "AuthGroupFile %s\n", path);
        }
        path = handler->getUserFile();
        if (path) {
            tabs(fd, indent);
            mprFprintf(fd, "AuthUserFile %s\n", path);
        }
    }
#endif

    authType = auth->getType();
    if (authType == MA_AUTH_BASIC) {
        tabs(fd, indent);
        mprFprintf(fd, "AuthType basic\n");
    } else if (authType == MA_AUTH_DIGEST) {
        char *qop = auth->getQop();

        tabs(fd, indent);
        mprFprintf(fd, "AuthType digest\n");
        tabs(fd, indent);
        if (qop && *qop) {
            mprFprintf(fd, "AuthDigestQop %s\n", qop);
        }
    }

    realm = auth->getRealm();
    groups = auth->getRequiredGroups();
    users = auth->getRequiredUsers();
    acl = auth->getRequiredAcl();

    if (realm && *realm) {
        tabs(fd, indent);
        mprFprintf(fd, "AuthName \"%s\"\n", realm);
    }
    if (auth->getAnyValidUser()) {
        tabs(fd, indent);
        mprFprintf(fd, "Require valid-user\n");
    } else if (groups && *groups) {
        tabs(fd, indent);
        mprFprintf(fd, "Require group %s\n", groups);
    } else if (users && *users) {
        tabs(fd, indent);
        mprFprintf(fd, "Require user %s\n", users);
    } else if (acl) {
        tabs(fd, indent);
        mprFprintf(fd, "Require acl 0x%x\n", acl);
    }
}


static void tabs(int fd, int indent)
{
    for (int i = 0; i < indent; i++) {
        write(fd, "\t", 1);
    }
}


#endif  /* BLD_FEATURE_CONFIG_SAVE */

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
/************************************************************************/
/*
 *  End of file "../http/config.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/conn.c"
 */
/************************************************************************/

/*
 *  conn.c -- Connection module to handle individual HTTP connections.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */




static int  connectionDestructor(MaConn *conn);
static inline MaPacket *getPacket(MaConn *conn);
static void readEvent(MaConn *conn);
static void ioEvent(MaConn *conn, MprSocket *sock, int mask, bool isPoolThread);
static void setupConnIO(MaConn *conn);
static void setupHandler(MaConn *conn);

/*
 *  Create a new connection object.
 */

static MaConn *createConn(MprCtx ctx, MaHost *host, MprSocket *sock, cchar *ipAddr, int port, MaHostAddress *address)
{
    MaConn      *conn;

    conn = mprAllocObjWithDestructorZeroed(ctx, MaConn, connectionDestructor);

    if (conn == 0) {
        return 0;
    }

    if (host->keepAlive) {
        conn->keepAliveCount = host->maxKeepAlive;
    }

    conn->http = host->server->http;
    conn->sock = sock;
    mprStealBlock(conn, sock);

    conn->state = MPR_HTTP_STATE_BEGIN;
    conn->timeout = host->timeout;
    conn->remotePort = port;
    conn->remoteIpAddr = mprStrdup(conn, ipAddr);
    conn->address = address;
    conn->host = host;
    conn->originalHost = host;
    conn->input = 0;
    conn->expire = 1;

    maInitSchedulerQueue(&conn->serviceq);

    return conn;
}


/*
 *  Cleanup a connection. Invoked automatically whenever the connection is freed.
 */
static int connectionDestructor(MaConn *conn)
{
    maRemoveConn(conn->host, conn);
    mprAssert(conn->sock);

    if (conn->sock) {
        mprLog(conn, 4, "Closing connection");
        mprCloseSocket(conn->sock, MPR_SOCKET_GRACEFUL);
        mprFree(conn->sock);
    }
    return 0;
}


/*
 *  Close a connection
 */
void maCloseConn(MaConn *conn)
{
    //  TODO - locking issues
    //  TODO - need a parameter to say force a close.
    mprCloseSocket(conn->sock, MPR_SOCKET_GRACEFUL);
}


/*
 *  Reset a connection after completing a request. Connection may be kept-alive
 */
void maResetConn(MaConn *conn)
{
    conn->requestFailed = 0;
    conn->request = 0;
    conn->response = 0;
    conn->state =  MPR_HTTP_STATE_BEGIN;
    conn->flags &= ~MA_CONN_CLEAN_MASK;
    conn->expire = conn->time + conn->host->keepAliveTimeout;
}


/*
 *  Accept a new client connection. If multithreaded, this will come in on a pool thread dedicated to this connection.
 *  This is called from the listen wait handler.
 */
void maAcceptConn(MaServer *server, MprSocket *sock, cchar *ip, int port)
{
    MaHostAddress   *address;
    MaHost          *host;
    MaConn          *conn;
    MprSocket       *listenSock;
    MprHeap         *arena;

    listenSock = sock->listenSock;

    //  TODO OPT - how to compile this away
    //  TODO - want to report the current thread here (must compile single threaded)
    mprLog(server, 4, "New connection from %s:%d for %s:%d %s",
        ip, port, listenSock->ipAddr, listenSock->port, listenSock->sslSocket ? "(secure)" : "");

    /*
     *  Map the address onto a suitable host to initially serve the request initially until we can parse the Host header.
     */
    address = (MaHostAddress*) maLookupHostAddress(server, listenSock->ipAddr, listenSock->port);

    if (address == 0 || (host = mprGetFirstItem(address->vhosts)) == 0) {
        mprError(server, "No host configured for request %s:%d", listenSock->ipAddr, listenSock->port);
        mprFree(sock);
        return;
    }

    /*
     *  Create a connection memory arena. This optimizes memory allocations for this entire connection.
     *  Arenas are scalable, thread-safe virtual memory blocks that are freed in one chunk.
     */
    arena = mprAllocArena(host, "conn", 1, 0, NULL);
    if (arena == 0) {
        mprError(server, "Can't create connect arena object. Insufficient memory");
        return;
    }

    conn = createConn(arena, host, sock, ip, port, address);
    if (conn == 0) {
        mprError(server, "Can't create connect object. Insufficient memory");
        return;
    }
    conn->arena = arena;
    maAddConn(host, conn);

    if (listenSock->sslSocket) {
        /*
         *  SSL always needs the handler for non-blocking I/O (deep down in the SSL stack). Normal requests often don't
         *  require the handler as they can read the entire request in one I/O
         */
        setupHandler(conn);
    }

    ioEvent(conn, sock, MPR_READABLE, 1);

    /* WARNING the connection object may be destroyed here */
    
#if BLD_FEATURE_MULTITHREAD
    //  TODO - should this not be enabling wiat events on sock?
    //  TODO - must only do this is the queue is enabled.
    mprEnableWaitEvents(listenSock->handler, 1);
#endif
}


/*
 *  IO event handler. Called in response to accept and when single-threaded, I/O events. If multithreaded, this will be 
 *  run by a pool thread. NOTE: a request is not permanently assigned to a pool thread. Each io event may be serviced by a
 *  different pool thread.
 */
static void ioEvent(MaConn *conn, MprSocket *sock, int mask, bool isPoolThread)
{
    conn->time = mprGetTime(conn);

    if (mask & MPR_WRITEABLE) {
        maProcessWriteEvent(conn);
    }
    if (mask & MPR_READABLE) {
        readEvent(conn);
    }
    setupConnIO(conn);
}


static void setupHandler(MaConn *conn) 
{
    if (conn->sock->handler == 0) {
        mprSetSocketCallback(conn->sock, (MprSocketProc) ioEvent, conn, NULL, conn->socketEventMask, MPR_NORMAL_PRIORITY);
    } else {
        mprSetSocketEventMask(conn->sock, conn->socketEventMask);
    }
}


/*
 *  Control the connection's I/O events
 */
static void setupConnIO(MaConn *conn)
{
    conn->socketEventMask = 0;
    
    if (conn->request) {
        if (conn->response->queue[MA_QUEUE_SEND].prevQ->count > 0) {
            /*
             *  Connector has more data to send in its queue. Request a write event to service it.
             */
            conn->socketEventMask |= MPR_WRITEABLE;
        }
        if (conn->state <= MPR_HTTP_STATE_CHUNK) {
            conn->socketEventMask |= MPR_READABLE;
            
        } else if (MPR_HTTP_STATE_COMPLETE == conn->state) {
            maProcessReadEvent(conn, 0);
        }

    } else {
        if (mprGetSocketEof(conn->sock) || conn->keepAliveCount < 0) {
            /*
             *  This will close the connection and free all connection resources
             */
            mprFree(conn->arena);
            /* mprPrintAllocReport(mprGetMpr(0), "After closing connection"); */
            return;

        } else {
            conn->socketEventMask |= MPR_READABLE;
        }
    }

    /*
     *  Not end of file so enable an I/O handler to listen for either more data or another request on this connection
     */
    setupHandler(conn);

    //  TODO Locking??
    conn->expire = conn->time + conn->host->timeout;
}


/*
 *  TODO MULTITHREAD - race. This is called from cgiCallback on another thread.
 */
void maAwakenConn(MaConn *conn)
{
    if (conn->keepAliveCount <= 0) {
        maCloseConn(conn);
    } else {
        conn->socketEventMask |= MPR_READABLE;
        setupHandler(conn);
    }
}


/*
 *  Process a socket readable event and keep reading while there is data.
 */
static void readEvent(MaConn *conn)
{
    MaPacket    *packet;
    MprBuf      *content;
    int         nbytes, len;

    while (1) {

        if ((packet = getPacket(conn)) == 0) {
            return;
        }
        content = packet->content;
        len = mprGetBufSpace(content);
        if (conn->request) {
            len = min(conn->request->remainingContent, len);
            if (len == 0) {
                len = mprGetBufSpace(content);
            }
        }
        mprAssert(len > 0);

        nbytes = mprReadSocket(conn->sock, mprGetBufEnd(content), len);
    
        mprLog(conn, 5, "readEvent: read nbytes %d, bufsize %d", nbytes, len);
            
        if (nbytes < 0) {
            if (conn->request) {
                maProcessReadEvent(conn, 0);
            } else {
                conn->keepAliveCount = 0;
            }
            break;
    
        } else if (nbytes > 0) {
            mprAdjustBufEnd(content, nbytes);
            packet->count += nbytes;
            maProcessReadEvent(conn, packet);
    
        } else {
            if (mprGetSocketEof(conn->sock) && 
                    (MPR_HTTP_STATE_CONTENT <= conn->state && conn->state <= MPR_HTTP_STATE_CHUNK)) {
                maProcessReadEvent(conn, 0);
            } 
            break;
        }
    }
}


static inline MaPacket *getPacket(MaConn *conn)
{
    MaPacket        *packet;
    MprBuf          *content;

    if ((packet = conn->input) == NULL) {
        conn->input = packet = maCreatePacket(conn, MA_BUFSIZE);
    }
    if (packet) {
        content = packet->content;
        mprResetBufIfEmpty(content);
        if (mprGetBufSpace(content) < MPR_BUFSIZE) {
            mprGrowBuf(content, MPR_BUFSIZE);
        }
    }
    return packet;
}


void *maGetHandlerQueueData(MaConn *conn)
{
    MaQueue     *q;

    q = &conn->response->queue[MA_QUEUE_SEND];
    return q->nextQ->queueData;
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
/************************************************************************/
/*
 *  End of file "../http/conn.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/connectors/netConnector.c"
 */
/************************************************************************/

/*
 *  netConnector.c -- General network connector. 
 *
 *  The Network connector handles output data (only) from upstream handlers and filters. It uses vectored writes to
 *  aggregate output packets into fewer actual I/O requests to the O/S. 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_NET

static void addPacketForNet(MaQueue *q, MaPacket *packet);
static void adjustNetVec(MaQueue *q, int written);
static int  buildNetVec(MaQueue *q);


static void netOutgoingService(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;
    int         written, errCode;

    conn = q->conn;
    resp = conn->response;
    
    while (q->first || q->ioIndex) {

        if (q->ioIndex == 0 && buildNetVec(q) <= 0) {
            break;
        }

        /*
         *  Issue a single I/O request to write all the blocks in the I/O vector
         */
        mprAssert(q->ioIndex > 0);
        written = mprWriteSocketVector(conn->sock, q->iovec, q->ioIndex);
        mprLog(q, 5, "Net connector write %d", written);

        if (written < 0) {
            errCode = mprGetOsError(q);
            if (errCode == EAGAIN || errCode == EWOULDBLOCK) {
                break;
            }
#if UNUSED
            saveErrno = errno;
            written = mprWriteSocketVector(conn->sock, q->iovec, q->ioIndex);
            mprAssert(written == -1);
            if (written != -1) {
                mprLog(q, 5, "mprWriteSocketVector errno %d, then wrote %d", saveErrno, written);
            }
#endif
            if (errCode == EPIPE) {
                maFailConnection(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "Client closed connection");
            } else {
                maFailConnection(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "mprVectorWriteSocket failed, error %d", errCode);
            }
            break;

        } else if (written == 0) {
            /* 
             * Socket full. Wait for an I/O event. Conn.c will setup listening for write events if the queue is non-empty
             */
            maRequestWriteBlocked(conn);
            break;

        } else if (written == q->ioCount && q->flags & MA_QUEUE_EOF) {
#if BLD_DEBUG
            /*
             *  Just to see the contents
             */
            adjustNetVec(q, written);
#else
            resp->bytesWritten += written;
#endif
            maCompleteRequest(conn);
            break;

        } else {
            adjustNetVec(q, written);
        }
    }
}


/*
 *  Build the IO vector. Return the count of bytes to be written. Return -1 for EOF.
 */
static int buildNetVec(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;
    MaPacket    *packet;

    conn = q->conn;
    resp = conn->response;

    /*
     *  Examine each packet and accumulate as many packets into the I/O vector as possible. Leave the packets on the queue 
     *  for now, they are removed after the IO is complete for the entire packet.
     */
    for (packet = q->first; packet; packet = packet->next) {
        
        /* 
         *  Must be room for 3 fragments in IO vector (could be prefix, content and suffix) 
         */
        if (q->ioIndex >= (MA_MAX_IOVEC - 2)) {
            break;
        }
        if (packet->flags & MA_PACKET_HEADER) {
            if (resp->chunkSize <= 0 && q->count > 0 && resp->length < 0) {
                /* Incase no chunking filter and we've not seen all the data yet */
                conn->keepAliveCount = 0;
            }
            maFillHeaders(conn, packet);
            q->count += packet->count;

        } else if (packet->count == 0) {
            q->flags |= MA_QUEUE_EOF;
            if (packet->prefix == NULL) {
                break;
            }
            
        } else if (resp->flags & MA_RESP_NO_BODY) {
            //  TODO - convert to maDiscardData and then remove maCleanQueue
            maCleanQueue(q);
            continue;
        }
        addPacketForNet(q, packet);
    }

    return q->ioCount;
}


/*
 *  Add one entry to the io vector
 */
static void addToNetVector(MaQueue *q, char *ptr, int bytes)
{
    mprAssert(bytes > 0);

    q->iovec[q->ioIndex].start = ptr;
    q->iovec[q->ioIndex].len = bytes;
    q->ioCount += bytes;
    q->ioIndex++;
}


/*
 *  Add a packet to the io vector. Return the number of bytes added to the vector.
 */
static void addPacketForNet(MaQueue *q, MaPacket *packet)
{
    MaResponse  *resp;
    MaConn      *conn;
    MprIOVec    *iovec;
    int         index;

    conn = q->conn;
    resp = conn->response;
    iovec = q->iovec;
    index = q->ioIndex;

    mprAssert(q->count >= 0);
    mprAssert(q->ioIndex < (MA_MAX_IOVEC - 2));

    if (packet->prefix) {
        addToNetVector(q, mprGetBufStart(packet->prefix), mprGetBufLength(packet->prefix));
    }

    if (packet->count > 0) {
        addToNetVector(q, mprGetBufStart(packet->content), mprGetBufLength(packet->content));
    }
    
#if FUTURE
    if (packet->suffix) {
        addToNetVector(q, mprGetBufStart(packet->suffix), mprGetBufLength(packet->suffix));
    }
#endif
}


/*
 *  Clear entries from the IO vector that have actually been transmitted. Support partial writes.
 */
static void adjustNetVec(MaQueue *q, int written)
{
    MprIOVec    *iovec;
    MaPacket    *packet, *last;
    MaResponse  *resp;
    int         i, j, bytes, len;

    mprAssert(q->first);

    resp = q->conn->response;
    resp->bytesWritten += written;

    /*
     *  Remove completed packets
     */
    bytes = written;
    while (bytes > 0 && (packet = q->first) != 0) {
        if (packet->prefix) {
            len = mprGetBufLength(packet->prefix);
            if (len > bytes) {
                mprAdjustBufStart(packet->prefix, bytes);
                bytes = 0;
                break;
            }
            bytes -= len;
            packet->prefix = 0;
        }

        if (packet->content) {
            len = mprGetBufLength(packet->content);
            if (len > bytes) {
                mprAdjustBufStart(packet->content, bytes);
                bytes = 0;
                break;
            }
            bytes -= len;
            packet->content = 0;
        }

#if FUTURE
        if (packet->suffix) {
            len = mprGetBufLength(packet->suffix);
            if (len > bytes) {
                mprAdjustBufStart(packet->suffix, bytes);
                bytes = 0;
                break;
            }
            bytes -= len;
            packet->suffix = 0;
        }
#endif
        last = packet;
        /*
         *  This will get the packet off the queue and will re-enable upstream disabled queues
         */
        maGet(q);
        mprFree(packet);
    }
    mprAssert(bytes == 0);

    /*
     *  Cleanup the IO vector
     */
    if (written == q->ioCount) {
        /*
         *  Entire vector written. Just reset.
         */
        q->ioIndex = 0;
        q->ioCount = 0;

    } else {
        /*
         *  Partial write of an vector entry. Need to copy down the unwritten vector entries.
         */
        q->ioCount -= written;
        mprAssert(q->ioCount >= 0);
        iovec = q->iovec;
        for (i = 0; i < q->ioIndex; i++) {
            len = (int) iovec[i].len;
            if (written < len) {
                iovec[i].start += written;
                iovec[i].len -= written;
                break;
            } else {
                written -= len;
            }
        }
        /*
         *  Compact
         */
        for (j = 0; i < q->ioIndex; j++) {
            iovec[j++] = iovec[i++];
        }
        q->ioIndex = j;
    }
}


/*
 *  Initialize the net connector
 */
int maOpenNetConnector(MaHttp *http)
{
    MaStage     *stage;

    stage = maCreateConnector(http, "netConnector", MA_STAGE_ALL);
    if (stage == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    stage->outgoingService = netOutgoingService;
    http->netConnector = stage;
    return 0;
}


#endif /* BLD_FEATURE_NET */

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
/************************************************************************/
/*
 *  End of file "../http/connectors/netConnector.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/connectors/sendConnector.c"
 */
/************************************************************************/

/*
 *  sendConnector.c -- Send file connector. 
 *
 *  The Sendfile connector supports the optimized transmission of whole static files. It uses operating system sendfile APIs to 
 *  eliminate reading the document into user space and multiple socket writes. The send connector is not a general purpose
 *  connector. It cannot handle dynamic data or ranged requests. It does support chunked requests.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_SEND && (MACOSX || LINUX)


static void addPacketForSend(MaQueue *q, MaPacket *packet);
static void adjustSendVec(MaQueue *q, int written);
static int  buildSendVec(MaQueue *q);

/*
 *  Invoked to initialize the send connector for a request
 */
static void sendOpen(MaQueue *q)
{
    MaConn          *conn;
    MaResponse      *resp;

    conn = q->conn;
    resp = conn->response;

    /*
     *  To write an entire file, reset the maximum and packet size to the maximum response body size (LimitResponseBody)
     */
    q->max = conn->http->limits.maxResponseBody;
    q->packetSize = conn->http->limits.maxResponseBody;

    if (!conn->requestFailed) {
        resp->file = mprOpen(q, resp->filename, O_RDONLY | O_BINARY, 0);
        if (resp->file == 0) {
            maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Can't open document: %s", resp->filename);
        }
    }
}


/*
 *  Outgoing data service routine. May be called multiple times.
 */
static void sendOutgoingService(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;
    int         written, ioCount, errCode;

    conn = q->conn;
    resp = conn->response;
    
    /*
     *  Loop doing non-blocking I/O until blocked or all the packets received are written.
     */
    while (1) {
        
        if (q->ioIndex == 0) {
            /*
             *  Rebuild the iovector only when the past vector has been completely written. Simplifies the logic quite a bit.
             */
            if (buildSendVec(q) <= 0) {
                break;
            }
        }

        /*
         *  Write the vector and file data. Exclude the file entry in the io vector.
         */
        ioCount = q->ioIndex - q->ioFileEntry;
        mprAssert(ioCount >= 0);
        written = mprSendFileToSocket(resp->file, conn->sock, resp->pos, q->ioCount, q->iovec, ioCount, NULL, 0);
        if (written < 0) {
            errCode = mprGetOsError(q);
            if (errCode == EAGAIN || errCode == EWOULDBLOCK) {
                break;
            }
            if (errCode == EPIPE) {
                maFailConnection(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "Client closed connection");
            } else {
                maFailConnection(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "mprSendFileToSocket failed, errCode %d", errCode);
            }
            break;

        } else if (written == 0) {
            /* Socket is full. Wait for an I/O event */
            maRequestWriteBlocked(conn);
            break;

        } else if (written == q->ioCount && q->flags & MA_QUEUE_EOF) {
            resp->bytesWritten += written;
            maCompleteRequest(conn);
            break;

        } else {
            adjustSendVec(q, written);
        }
    }
}


/*
 *  Build the IO vector. This connector uses the send file API which permits multiple IO blocks to be written with 
 *  file data. This is used to write transfer the headers and chunk encoding boundaries. Return the count of bytes to 
 *  be written. Return -1 for EOF.
 */
static int buildSendVec(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;
    MaPacket    *packet;

    conn = q->conn;
    resp = conn->response;

    mprAssert(q->ioIndex == 0);
    q->ioCount = 0;
    q->ioFileEntry = 0;

    /*
     *  Examine each packet and accumulate as many packets into the I/O vector as possible. Can only have one data packet at
     *  a time due to the limitations of the sendfile API. Leave the packets on the queue for now, they are removed after 
     *  the IO is complete for the entire packet.
     */
    for (packet = q->first; packet; packet = packet->next) {
        /* 
         *  Must be room for 2 fragments in IO vector (could be prefix and suffix) 
         */
        if (q->ioIndex >= (MA_MAX_IOVEC - 2)) {
            break;
        }
        
        if (packet->flags & MA_PACKET_HEADER) {
            maFillHeaders(conn, packet);
            q->count += packet->count;

        } else if (packet->count == 0) {
            /*
             *  This is the end of file packet. If chunking, we must still add this to the vector as we need to emit 
             *  a trailing chunk termination line.
             */
            q->flags |= MA_QUEUE_EOF;
            if (packet->prefix == NULL) {
                break;
            }

        } else if (q->ioFileEntry) {
            break;

        } else if (resp->flags & MA_RESP_NO_BODY) {
            //  TODO - convert to maDiscardData and then remove maCleanQueue
            maCleanQueue(q);
            continue;
        }
        addPacketForSend(q, packet);
    }

    return q->ioCount;
}


/*
 *  Add one entry to the io vector
 */
static void addToSendVector(MaQueue *q, char *ptr, int bytes)
{
    mprAssert(bytes > 0);

    q->iovec[q->ioIndex].start = ptr;
    q->iovec[q->ioIndex].len = bytes;
    q->ioCount += bytes;
    q->ioIndex++;
}


/*
 *  Add a packet to the io vector. Return the number of bytes added to the vector.
 */
static void addPacketForSend(MaQueue *q, MaPacket *packet)
{
    MaResponse  *resp;
    MaConn      *conn;
    MprIOVec    *iovec;

    conn = q->conn;
    resp = conn->response;
    iovec = q->iovec;
    
    mprAssert(q->count >= 0);
    mprAssert(q->ioIndex < (MA_MAX_IOVEC - 2));

    if (packet->prefix) {
        addToSendVector(q, mprGetBufStart(packet->prefix), mprGetBufLength(packet->prefix));
    }

    if (packet->count > 0) {
        /*
         *  Header packets have actual content. File data packets are virtual and only have a count.
         */
        if (packet->content) {
            addToSendVector(q, mprGetBufStart(packet->content), mprGetBufLength(packet->content));

        } else {
            addToSendVector(q, 0, packet->count);
            mprAssert(q->ioFileEntry == 0);
            q->ioFileEntry = 1;
            q->ioFileOffset += packet->count;
        }
    }

#if FUTURE
    /*
     *  Suffixes are currently not used
     */
    if (packet->suffix) {
        addToSendVector(q, mprGetBufStart(packet->suffix), mprGetBufLength(packet->suffix));
    }
#endif
}


/*
 *  Clear entries from the IO vector that have actually been transmitted. This supports partial writes due to the socket
 *  being full. Don't come here if we've seen all the packets and all the data has been completely written. ie. small files
 *  don't come here.
 */
static void adjustSendVec(MaQueue *q, int written)
{
    MprIOVec    *iovec;
    MaPacket    *packet, *last;
    MaResponse  *resp;
    int         i, j, bytes, len, count;

    mprAssert(q->first);

    resp = q->conn->response;
    resp->bytesWritten += written;

    /*
     *  Remove completed packets
     */
    bytes = written;
    while (bytes > 0 && (packet = q->first) != 0) {
        if (packet->prefix) {
            len = mprGetBufLength(packet->prefix);
            if (len > bytes) {
                mprAdjustBufStart(packet->prefix, bytes);
                bytes = 0;
                break;
            }
            bytes -= len;
            packet->prefix = 0;
        }

        if (packet->count) {
            if (packet->content) {
                len = mprGetBufLength(packet->content);
                if (len > bytes) {
                    mprAdjustBufStart(packet->content, bytes);
                    bytes = 0;
                    break;
                }

            } else {
                len = packet->count;
                count = min(len, bytes);
                /*
                 *  Packet has no content buffer, we adjust the actual packet count. Must adjust the queue count also.
                 */
                packet->count -= count;
                q->count -= count;
                if (len > bytes) {
                    break;
                }
            }
            bytes -= len;
        }
#if FUTURE
        /*
         *  Suffies are currently not used
         */
        if (packet->suffix) {
            len = mprGetBufLength(packet->suffix);
            if (len > bytes) {
                mprAdjustBufStart(packet->suffix, bytes);
                bytes = 0;
                break;
            }
            bytes -= len;
            packet->suffix = 0;
        }
#endif
        last = packet;
        maGet(q);
        mprFree(packet);
    }

    /*
     *  Cleanup the IO vector
     */
    if (written == q->ioCount) {
        /*
         *  Entire vector written. Just reset.
         */
        q->ioIndex = 0;
        q->ioCount = 0;
        resp->pos = q->ioFileOffset;

    } else {
        /*
         *  Partial write of an vector entry. Need to copy down the unwritten vector entries.
         */
        q->ioCount -= written;
        mprAssert(q->ioCount >= 0);
        iovec = q->iovec;
        for (i = 0; i < q->ioIndex; i++) {
            len = (int) iovec[i].len;
            if (iovec[i].start) {
                if (written < len) {
                    iovec[i].start += written;
                    iovec[i].len -= written;
                    break;
                } else {
                    written -= len;
                }
            } else {
                /*
                 *  File data has a null start ptr
                 */
                resp->pos += written;
                q->ioIndex = 0;
                q->ioCount = 0;
                return;
            }
        }

        /*
         *  Compact
         */
        for (j = 0; i < q->ioIndex; j++) {
            iovec[j++] = iovec[i++];
        }
        q->ioIndex = j;
    }
}


int maOpenSendConnector(MaHttp *http)
{
    MaStage     *stage;

    stage = maCreateConnector(http, "sendConnector", MA_STAGE_ALL);
    if (stage == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    stage->open = sendOpen;
    stage->outgoingService = sendOutgoingService; 
    http->sendConnector = stage;
    return 0;
}


#endif /* BLD_FEATURE_SEND */

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
/************************************************************************/
/*
 *  End of file "../http/connectors/sendConnector.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/dir.c"
 */
/************************************************************************/

/*
 *  dir.c -- Support authorization on a per-directory basis.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




MaDir *maCreateBareDir(MaHost *host, cchar *path)
{
    MaDir   *dir;

    mprAssert(host);
    mprAssert(path);

    dir = mprAllocObjZeroed(host, MaDir);
    if (dir == 0) {
        return 0;
    }
    dir->indexName = mprStrdup(dir, "index.html");
    dir->host = host;

#if BLD_FEATURE_AUTH
    dir->auth = maCreateAuth(dir, 0);
#endif

    if (path) {
        dir->path = mprStrdup(dir, path);
        dir->pathLen = mprStrlen(path, MPR_MAX_FNAME);
    }

    return dir;
}


MaDir *maCreateDir(MaHost *host, cchar *path, MaDir *parent)
{
    MaDir   *dir;

    mprAssert(host);
    mprAssert(path);

    dir = mprAllocObjZeroed(host, MaDir);
    if (dir == 0) {
        return 0;
    }
    
    dir->host = host;
    dir->indexName = mprStrdup(dir, parent->indexName);

    if (path == 0) {
        path = parent->path;
    }
    maSetDirPath(dir, path);

#if BLD_FEATURE_AUTH
    dir->auth = maCreateAuth(dir, parent->auth);
#endif

    return dir;
}


void maSetDirPath(MaDir *dir, cchar *fileName)
{
    char    buf[MPR_MAX_FNAME], *path;

    mprAssert(dir);
    mprAssert(fileName);

    mprFree(dir->path);
    path = mprGetAbsFilename(dir, fileName);
    mprStrcpy(buf, sizeof(buf), path);
    mprFree(path);

#if UNUSED
    /*
     *  Append a trailing "/"
     */
    len = (int) strlen(buf);
    if (buf[len - 1] != '/') {
        buf[len] = '/';
        buf[++len] = '\0';
    }
#endif

    dir->path = mprStrdup(dir, buf);
    dir->pathLen = (int) strlen(dir->path);

    /*
     *  Always strip trailing "/"
     */
    if (dir->pathLen > 0 && dir->path[dir->pathLen - 1] == '/') {
        dir->path[--dir->pathLen] = '\0';
    }
}


void maSetDirIndex(MaDir *dir, cchar *name) 
{ 
    mprAssert(dir);
    mprAssert(name && *name);

    mprFree(dir->indexName);
    dir->indexName = mprStrdup(dir, name); 
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
/************************************************************************/
/*
 *  End of file "../http/dir.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/filters/authFilter.c"
 */
/************************************************************************/

/*
 *  authFilter.c - Authorization filter for basic and digest authentication.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_AUTH
/*
 *  Per-request authorization data
 */
typedef struct AuthData 
{
    char            *password;          /* User password or digest */
    char            *userName;
#if BLD_FEATURE_AUTH_DIGEST
    char            *cnonce;
    char            *nc;
    char            *nonce;
    char            *opaque;
    char            *qop;
    char            *realm;
    char            *uri;
#endif
} AuthData;


static void decodeBasicAuth(MaQueue *q);
static int  decodeDigestDetails(MaQueue *q);
static void formatAuthResponse(MaConn *conn, MaAuth *auth, int code, char *msg, char *logMsg);
static cchar *getPassword(MaConn *conn, cchar *realm, cchar *user);
static bool validateUserCredentials(MaConn *conn, cchar *realm, char *user, cchar *password, cchar *requiredPass, 
        char **msg);

/*
 *  Open the authorization filter AND check if the request has the required authorization.
 *  This runs when the pipeline is created. 
 */
static void openAuth(MaQueue *q)
{
    MaConn      *conn;
    MaRequest   *req;
    MaAuth      *auth;
    AuthData    *ad;
    cchar       *requiredPassword;
    char        *url, *requiredDigest, *msg;
    int         actualAuthType;

    conn = q->conn;
    req = conn->request;
    url = req->url;
    auth = req->auth;

    if (auth == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_UNAUTHORIZED, "Access Denied, Authorization enabled.");
        return;
    }

    ad = q->queueData = mprAllocObjZeroed(q, AuthData);
    if (ad == 0) {
        return;
    }

    if (auth->type == 0) {
        formatAuthResponse(conn, auth, MPR_HTTP_CODE_UNAUTHORIZED, "Access Denied, Authorization required.", 0);
        return;
    }
    if (req->authDetails == 0) {
        formatAuthResponse(conn, auth, MPR_HTTP_CODE_UNAUTHORIZED, "Access Denied, Missing authorization details.", 0);
        return;
    }

    if (mprStrcmpAnyCase(req->authType, "basic") == 0) {
        decodeBasicAuth(q);
        actualAuthType = MA_AUTH_BASIC;

#if BLD_FEATURE_AUTH_DIGEST
    } else if (mprStrcmpAnyCase(req->authType, "digest") == 0) {
        if (decodeDigestDetails(q) < 0) {
            //  TODO - code
            maFailRequest(conn, 400, "Bad authorization header");
            return;
        }
        actualAuthType = MA_AUTH_DIGEST;
#endif
    } else {
        actualAuthType = MA_AUTH_UNKNOWN;
    }
    mprLog(q, 4, "run: type %d, url %s\nDetails %s\n", auth->type, req->url, req->authDetails);

    if (ad->userName == 0) {
        formatAuthResponse(conn, auth, MPR_HTTP_CODE_UNAUTHORIZED, "Access Denied, Missing user name.", 0);
        return;
    }

    if (auth->type != actualAuthType) {
        formatAuthResponse(conn, auth, MPR_HTTP_CODE_UNAUTHORIZED, "Access Denied, Wrong authentication protocol.", 0);
        return;
    }

    /*
     *  Some backend methods can't return the password and will simply do everything in validateUserCredentials. 
     *  In this case, they and will return "". That is okay.
     */
    if ((requiredPassword = getPassword(conn, auth->requiredRealm, ad->userName)) == 0) {
        formatAuthResponse(conn, auth, MPR_HTTP_CODE_UNAUTHORIZED, "Access Denied, authentication error.", 
            "User not defined");
        return;
    }

#if BLD_FEATURE_AUTH_DIGEST
    if (auth->type == MA_AUTH_DIGEST) {
        if (strcmp(ad->qop, auth->qop) != 0) {
            formatAuthResponse(conn, auth, MPR_HTTP_CODE_UNAUTHORIZED, 
                "Access Denied, Quality of protection does not match.", 0);
            return;
        }
        mprCalcDigest(req, &requiredDigest, 0, requiredPassword, ad->realm, req->url, ad->nonce, ad->qop, ad->nc, ad->cnonce,
            req->methodName);
        requiredPassword = requiredDigest;
    }
#endif
    if (!validateUserCredentials(conn, auth->requiredRealm, ad->userName, ad->password, requiredPassword, &msg)) {
        formatAuthResponse(conn, auth, MPR_HTTP_CODE_UNAUTHORIZED, 
            "Access denied, authentication error", msg);
    }
}


/*
 *  Validate the user credentials with the designated authorization backend method.
 */
static bool validateUserCredentials(MaConn *conn, cchar *realm, char *user, cchar *password, cchar *requiredPass, char **msg)
{
    MaAuth      *auth;

    auth = conn->request->auth;

    /*
     *  Use this funny code construct incase no backend method is configured. Still want the code to compile.
     */
    if (0) {
#if BLD_FEATURE_AUTH_FILE
    } else if (auth->method == MA_AUTH_METHOD_FILE) {
        return maValidateNativeCredentials(conn, realm, user, password, requiredPass, msg);
#endif
#if BLD_FEATURE_AUTH_PAM
    } else if (auth->method == MA_AUTH_METHOD_PAM) {
        return maValidatePamCredentials(conn, realm, user, password, NULL, msg);
#endif
    } else {
        *msg = "Required authorization backend method is not enabled or configured";
    }
    return 0;
}


/*
 *  Get the password (if the designated authorization backend method will give it to us)
 */
static cchar *getPassword(MaConn *conn, cchar *realm, cchar *user)
{
    MaAuth      *auth;

    auth = conn->request->auth;

    /*
     *  Use this funny code construct incase no backend method is configured. Still want the code to compile.
     */
    if (0) {
#if BLD_FEATURE_AUTH_FILE
    } else if (auth->method == MA_AUTH_METHOD_FILE) {
        return maGetNativePassword(conn, realm, user);
#endif
#if BLD_FEATURE_AUTH_PAM
    } else if (auth->method == MA_AUTH_METHOD_PAM) {
        return maGetPamPassword(conn, realm, user);
#endif
    }
    return 0;
}


/*
 *  Decode basic authorization details
 */
static void decodeBasicAuth(MaQueue *q)
{
    MaConn      *conn;
    MaRequest   *req;
    AuthData    *ad;
    char        decodedDetails[64], *cp;

    conn = q->conn;
    req = conn->request;
    ad = q->queueData;

    mprDecode64(decodedDetails, sizeof(decodedDetails), req->authDetails);
    if ((cp = strchr(decodedDetails, ':')) != 0) {
        *cp++ = '\0';
    }

    if (cp) {
        ad->userName = mprStrdup(req, decodedDetails);
        ad->password = mprStrdup(req, cp);

    } else {
        ad->userName = mprStrdup(req, "");
        ad->password = mprStrdup(req, "");
    }

    maSetRequestUser(conn, ad->userName);
}


#if BLD_FEATURE_AUTH_DIGEST
/*
 *  Decode the digest authentication details.
 */
static int decodeDigestDetails(MaQueue *q)
{
    MaConn      *conn;
    MaRequest   *req;
    AuthData    *ad;
    char        *authDetails, *value, *tok, *key, *dp, *sp;
    int         seenComma;

    ad = q->queueData;
    conn = q->conn;
    req = conn->request;

    key = authDetails = mprStrdup(q, req->authDetails);

    while (*key) {
        while (*key && isspace((int) *key)) {
            key++;
        }
        tok = key;
        while (*tok && !isspace((int) *tok) && *tok != ',' && *tok != '=') {
            tok++;
        }
        *tok++ = '\0';

        while (isspace((int) *tok)) {
            tok++;
        }
        seenComma = 0;
        if (*tok == '\"') {
            value = ++tok;
            while (*tok != '\"' && *tok != '\0') {
                tok++;
            }
        } else {
            value = tok;
            while (*tok != ',' && *tok != '\0') {
                tok++;
            }
            seenComma++;
        }
        *tok++ = '\0';

        /*
         *  Handle back-quoting
         */
        if (strchr(value, '\\')) {
            for (dp = sp = value; *sp; sp++) {
                if (*sp == '\\') {
                    sp++;
                }
                *dp++ = *sp++;
            }
            *dp = '\0';
        }

        /*
         *  username, response, oqaque, uri, realm, nonce, nc, cnonce, qop
         */
        switch (tolower((int) *key)) {
        case 'a':
            if (mprStrcmpAnyCase(key, "algorithm") == 0) {
                break;
            } else if (mprStrcmpAnyCase(key, "auth-param") == 0) {
                break;
            }
            break;

        case 'c':
            if (mprStrcmpAnyCase(key, "cnonce") == 0) {
                ad->cnonce = mprStrdup(q, value);
            }
            break;

        case 'd':
            if (mprStrcmpAnyCase(key, "domain") == 0) {
                break;
            }
            break;

        case 'n':
            if (mprStrcmpAnyCase(key, "nc") == 0) {
                ad->nc = mprStrdup(q, value);
            } else if (mprStrcmpAnyCase(key, "nonce") == 0) {
                ad->nonce = mprStrdup(q, value);
            }
            break;

        case 'o':
            if (mprStrcmpAnyCase(key, "opaque") == 0) {
                ad->opaque = mprStrdup(q, value);
            }
            break;

        case 'q':
            if (mprStrcmpAnyCase(key, "qop") == 0) {
                ad->qop = mprStrdup(q, value);
            }
            break;

        case 'r':
            if (mprStrcmpAnyCase(key, "realm") == 0) {
                ad->realm = mprStrdup(q, value);
            } else if (mprStrcmpAnyCase(key, "response") == 0) {
                /* Store the response digest in the password field */
                ad->password = mprStrdup(q, value);
            }
            break;

        case 's':
            if (mprStrcmpAnyCase(key, "stale") == 0) {
                break;
            }
        
        case 'u':
            if (mprStrcmpAnyCase(key, "uri") == 0) {
                ad->uri = mprStrdup(q, value);
            } else if (mprStrcmpAnyCase(key, "username") == 0) {
                ad->userName = mprStrdup(q, value);
            }
            break;

        default:
            /*  Just ignore keywords we don't understand */
            ;
        }
        key = tok;
        if (!seenComma) {
            while (*key && *key != ',') {
                key++;
            }
            if (*key) {
                key++;
            }
        }
    }
    mprFree(authDetails);
    if (ad->userName == 0 || ad->realm == 0 || ad->nonce == 0 || ad->uri == 0 || ad->password == 0) {
        return MPR_ERR_BAD_ARGS;
    }
    if (ad->qop && (ad->cnonce == 0 || ad->nc == 0)) {
        return MPR_ERR_BAD_ARGS;
    }
    if (ad->qop == 0) {
        ad->qop = mprStrdup(q, "");
    }

    maSetRequestUser(conn, ad->userName);
    return 0;
}
#endif


#if BLD_FEATURE_CONFIG_PARSE
/*
 *  Parse the appweb.conf directives for authorization
 */
static int parseAuth(MaHttp *http, cchar *key, char *value, MaConfigState *state)
{
    MaServer    *server;
    MaHost      *host;
    MaAuth      *auth;
    MaDir       *dir;
    MaAcl       acl;
    char        pathBuf[MPR_MAX_FNAME];
    char        *path, *names, *tok, *type, *aclSpec;

    server = state->server;
    host = state->host;
    auth = state->auth;
    dir = state->dir;

    if (mprStrcmpAnyCase(key, "AuthGroupFile") == 0) {
        //  TODO - this belongs elsewhere
        path = mprStrTrim(value, "\"");
        if (maMakePath(host, pathBuf, sizeof(pathBuf), path) == 0) {
            mprError(http, "AuthGroupFile is too long");
            return MPR_ERR_BAD_SYNTAX;
        }
        if (maReadGroupFile(server, auth, pathBuf) < 0) {
            mprError(http, "Can't open AuthGroupFile %s", pathBuf);
            return MPR_ERR_BAD_SYNTAX;
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "AuthMethod") == 0) {
        value = mprStrTrim(value, "\"");
        if (mprStrcmpAnyCase(value, "pam") == 0) {
            auth->method = MA_AUTH_METHOD_PAM;
            return 1;

        } else if (mprStrcmpAnyCase(value, "file") == 0) {
            auth->method = MA_AUTH_METHOD_FILE;
            return 1;

        } else {
            return MPR_ERR_BAD_SYNTAX;
        }

    } else if (mprStrcmpAnyCase(key, "AuthName") == 0) {
        //  TODO - should be maSetAuthRealm
        maSetAuthRealm(auth, mprStrTrim(value, "\""));
        return 1;
        
    } else if (mprStrcmpAnyCase(key, "AuthType") == 0) {
        value = mprStrTrim(value, "\"");
        if (mprStrcmpAnyCase(value, "Basic") == 0) {
            auth->type = MA_AUTH_BASIC;

#if BLD_FEATURE_AUTH_DIGEST
        } else if (mprStrcmpAnyCase(value, "Digest") == 0) {
            auth->type = MA_AUTH_DIGEST;
#endif

        } else {
            mprError(http, "Unsupported authorization protocol");
            return MPR_ERR_BAD_SYNTAX;
        }
        return 1;
        
    } else if (mprStrcmpAnyCase(key, "AuthUserFile") == 0) {
        //  TODO - this belons elsewhere
        path = mprStrTrim(value, "\"");
        if (maMakePath(host, pathBuf, sizeof(pathBuf), path) == 0) {
            mprError(http, "AuthUserFile is too long");
            return MPR_ERR_BAD_SYNTAX;
        }
        if (maReadUserFile(server, auth, pathBuf) < 0) {
            mprError(http, "Can't open AuthUserFile %s", pathBuf);
            return MPR_ERR_BAD_SYNTAX;
        }
        return 1;

#if BLD_FEATURE_AUTH_DIGEST
    } else if (mprStrcmpAnyCase(key, "AuthDigestQop") == 0) {
        value = mprStrTrim(value, "\"");
        mprStrLower(value);
        if (strcmp(value, "none") != 0 && strcmp(value, "auth") != 0 && strcmp(value, "auth-int") != 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
        maSetAuthQop(auth, value);
        return 1;

    } else if (mprStrcmpAnyCase(key, "AuthDigestAlgorithm") == 0) {
        return 1;

    } else if (mprStrcmpAnyCase(key, "AuthDigestDomain") == 0) {
        return 1;

    } else if (mprStrcmpAnyCase(key, "AuthDigestNonceLifetime") == 0) {
        return 1;

#endif
    } else if (mprStrcmpAnyCase(key, "Require") == 0) {
        if (maGetConfigValue(http, &type, value, &tok, 1) < 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
        if (mprStrcmpAnyCase(type, "acl") == 0) {
            aclSpec = mprStrTrim(tok, "\"");
            acl = maParseAcl(auth, aclSpec);
            maSetRequiredAcl(auth, acl);

        } else if (mprStrcmpAnyCase(type, "valid-user") == 0) {
            maSetAuthAnyValidUser(auth);

        } else {
            names = mprStrTrim(tok, "\"");
            if (mprStrcmpAnyCase(type, "user") == 0) {
                maSetAuthRequiredUsers(auth, names);

            } else if (mprStrcmpAnyCase(type, "group") == 0) {
                maSetAuthRequiredGroups(auth, names);

            } else {
                mprError(http, "Bad Require syntax: %s", type);
                return MPR_ERR_BAD_SYNTAX;
            }
        }
        return 1;
    }
    return 0;
}
#endif


/*
 *  Format an authentication response. This is typically a 401 response code.
 */
static void formatAuthResponse(MaConn *conn, MaAuth *auth, int code, char *msg, char *logMsg)
{
    MaRequest       *req;
#if BLD_FEATURE_AUTH_DIGEST
    char            *qopClass, *nonceStr, *etag;
#endif

    req = conn->request;
    if (logMsg == 0) {
        logMsg = msg;
    }

    mprLog(conn, 3, "formatAuthResponse: code %d, %s\n", code, logMsg);

    if (auth->type == MA_AUTH_BASIC) {
        maSetHeader(conn, 0, "WWW-Authenticate", "Basic realm=\"%s\"", auth->requiredRealm);

#if BLD_FEATURE_AUTH_DIGEST
    } else if (auth->type == MA_AUTH_DIGEST) {

        qopClass = auth->qop;

        /*
         *  Use the etag as our opaque string
         */
        etag = conn->response->etag;
        if (etag == 0) {
            etag = "";
        }
        mprCalcDigestNonce(req, &nonceStr, conn->host->secret, etag, auth->requiredRealm);

        if (strcmp(qopClass, "auth") == 0) {
            //TODO - use mprStrcat quicker
            maSetHeader(conn, 0, "WWW-Authenticate", "Digest realm=\"%s\", domain=\"%s\", "
                "qop=\"auth\", nonce=\"%s\", opaque=\"%s\", algorithm=\"MD5\", stale=\"FALSE\"", 
                auth->requiredRealm, conn->host->name, nonceStr, etag);

        } else if (strcmp(qopClass, "auth-int") == 0) {
            maSetHeader(conn, 0, "WWW-Authenticate", "Digest realm=\"%s\", domain=\"%s\", "
                "qop=\"auth\", nonce=\"%s\", opaque=\"%s\", algorithm=\"MD5\", stale=\"FALSE\"", 
                auth->requiredRealm, conn->host->name, nonceStr, etag);

        } else {
            maSetHeader(conn, 0, "WWW-Authenticate", "Digest realm=\"%s\", nonce=\"%s\"", auth->requiredRealm, nonceStr);
        }
        mprFree(nonceStr);
#endif
    }

    maFailRequest(conn, code, "Authentication Error: %s", msg);
}


MaAuth *maCreateAuth(MprCtx ctx, MaAuth *parent)
{
    MaAuth      *auth;

    auth = mprAllocObjZeroed(ctx, MaAuth);

    if (parent) {
        auth->allow = parent->allow;
        auth->anyValidUser = parent->anyValidUser;
        auth->type = parent->type;
        auth->deny = parent->deny;
        auth->method = parent->method;
        auth->flags = parent->flags;
        auth->order = parent->order;
        auth->qop = parent->qop;

#if BLD_FEATURE_AUTH_FILE
        auth->userFile = parent->userFile;
        auth->groupFile = parent->groupFile;
        auth->users = parent->users;
        auth->groups = parent->groups;
#endif

    } else{
#if BLD_FEATURE_AUTH_PAM
        auth->method = MA_AUTH_METHOD_PAM;
#elif BLD_FEATURE_AUTH_FILE
        auth->method = MA_AUTH_METHOD_FILE;
#endif
    }

    return auth;
}


void maSetAuthAllow(MaAuth *auth, cchar *allow)
{
    mprFree(auth->allow);
    auth->allow = mprStrdup(auth, allow);
}


void maSetAuthAnyValidUser(MaAuth *auth)
{
    auth->anyValidUser = 1;
    auth->flags |= MA_AUTH_REQUIRED;
}


void maSetAuthDeny(MaAuth *auth, cchar *deny)
{
    mprFree(auth->deny);
    auth->deny = mprStrdup(auth, deny);
}


void maSetAuthOrder(MaAuth *auth, int o)
{
    auth->order = o;
}


void maSetAuthQop(MaAuth *auth, cchar *qop)
{
    mprFree(auth->qop);
    if (strcmp(qop, "auth") == 0 || strcmp(qop, "auth-int") == 0) {
        auth->qop = mprStrdup(auth, qop);
    } else {
        auth->qop = mprStrdup(auth, "");
    }
}


void maSetAuthRealm(MaAuth *auth, cchar *realm)
{
    mprFree(auth->requiredRealm);
    auth->requiredRealm = mprStrdup(auth, realm);
}


void maSetAuthRequiredGroups(MaAuth *auth, cchar *groups)
{
    mprFree(auth->requiredGroups);
    auth->requiredGroups = mprStrdup(auth, groups);
    auth->flags |= MA_AUTH_REQUIRED;
}


void maSetAuthRequiredUsers(MaAuth *auth, cchar *users)
{
    mprFree(auth->requiredUsers);
    auth->requiredUsers = mprStrdup(auth, users);
    auth->flags |= MA_AUTH_REQUIRED;
}


/*
 *  Loadable module initialization
 */
MprModule *maAuthFilterInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *filter;

    module = mprCreateModule(http, "authFilter", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    filter = maCreateFilter(http, "authFilter", MA_STAGE_ALL);
    if (filter == 0) {
        mprFree(module);
        return 0;
    }
    http->authFilter = filter;

    filter->open = openAuth; 
    filter->parse = parseAuth; 

    return module;
}


#else
void __mprAuthFilterDummy() {}
#endif /* BLD_FEATURE_AUTH */

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
/************************************************************************/
/*
 *  End of file "../http/filters/authFilter.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/filters/chunkFilter.c"
 */
/************************************************************************/

/*
 *  chunkFilter.c - Transfer chunk endociding filter.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_CHUNK

static void setChunkPrefix(MaQueue *q, MaPacket *packet);


static void openChunk(MaQueue *q)
{
    MaConn          *conn;
    MaResponse      *resp;

    conn = q->conn;
    resp = conn->response;

    if (resp->entityLength >= 0 && resp->chunkSize < 0) {
        maRemoveQueue(q);
    } else {
        q->packetSize = min(conn->http->limits.maxChunkSize, q->max);
    }
}


/*
 *  Apply chunks to dynamic outgoing data. 
 */
static void outgoingChunkService(MaQueue *q)
{
    MaConn      *conn;
    MaPacket    *packet;
    MaResponse  *resp;

    conn = q->conn;
    resp = conn->response;

    if (!(q->flags & MA_QUEUE_SERVICED)) {
        /*
         *  If the last packet is the end packet, we have all the data. Thus we know the actual content length 
         *  and can bypass the chunk handler.
         */
        if (q->last->flags & MA_PACKET_END) {
            if (resp->chunkSize < 0 && resp->length <= 0) {
                /*  
                 *  Set the response content length and thus disable chunking -- not needed as we know the entity length.
                 */
                resp->length = q->count;
            }

        } else {
            resp->chunkSize = min(conn->http->limits.maxChunkSize, q->max);
        }
    }

    if (resp->chunkSize <= 0) {
        maDefaultOutgoingServiceStage(q);
        
    } else {
        for (packet = maGet(q); packet; packet = maGet(q)) {
            if (!(packet->flags & MA_PACKET_HEADER)) {
                if (packet->count > resp->chunkSize) {
                    maResizePacket(q, packet, resp->chunkSize);
                }
                setChunkPrefix(q, packet);
            }
            if (!maWillNextQueueAccept(q, packet)) {
                maPutBack(q, packet);
                return;
            }
            maPutNext(q, packet);
        }
    }
}


static void setChunkPrefix(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;

    conn = q->conn;

    if (packet->prefix) {
        return;
    }
    packet->prefix = mprCreateBuf(packet, 32, 32);
    if (packet->count) {
        mprPutFmtToBuf(packet->prefix, "\r\n%x\r\n", packet->count);
    } else {
        mprPutStringToBuf(packet->prefix, "\r\n0\r\n\r\n");
    }
}


/*
 *  Loadable module initialization
 */
MprModule *maChunkFilterInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *filter;

    module = mprCreateModule(http, "chunkFilter", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    filter = maCreateFilter(http, "chunkFilter", MA_STAGE_ALL);
    if (filter == 0) {
        mprFree(module);
        return 0;
    }
    http->chunkFilter = filter;

    filter->open = openChunk; 
    filter->outgoingService = outgoingChunkService; 

    return module;
}


#else
void __mprChunkFilterDummy() {}
#endif /* BLD_FEATURE_CHUNK */

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
/************************************************************************/
/*
 *  End of file "../http/filters/chunkFilter.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/filters/rangeFilter.c"
 */
/************************************************************************/

/*
 *  rangeFilter.c - Ranged request filter.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_RANGE

static MaPacket *createRangePacket(MaConn *conn, MaRange *range);
static MaPacket *createFinalRangePacket(MaConn *conn);

/*
 *  Apply ranges to dynamic outgoing data. 
 */
void maRangeService(MaQueue *q, MaRangeFillProc fill)
{
    MaPacket    *packet;
    MaRange     *range;
    MaConn      *conn;
    MaRequest   *req;
    MaResponse  *resp;
    int         bytes, count, endpos;

    conn = q->conn;
    req = conn->request;
    resp = conn->response;
    range = resp->currentRange;

    if (!(q->flags & MA_QUEUE_SERVICED)) {
        if (resp->entityLength < 0 && q->last->flags & MA_PACKET_END) {
           /*
            *   Have all the data, so compute an entity length. This allows negative ranges computed from the end of the data.
            */
           resp->entityLength = q->count;
        }
        if (resp->code != MPR_HTTP_CODE_OK || !maFixRangeLength(conn)) {
            maSendPackets(q);
            maRemoveQueue(q);
            return;
        }
        if (req->ranges->next) {
            maCreateRangeBoundary(conn);
        }
        resp->code = MPR_HTTP_CODE_PARTIAL;
    }

    for (packet = maGet(q); packet; packet = maGet(q)) {

        if (!(packet->flags & MA_PACKET_DATA)) {
            if (packet->flags & MA_PACKET_END && resp->rangeBoundary) {
                maPutNext(q, createFinalRangePacket(conn));
            }
            if (!maWillNextQueueAccept(q, packet)) {
                maPutBack(q, packet);
                return;
            }
            maPutNext(q, packet);
            continue;
        }

        /*
         *  Process the current packet over multiple ranges ranges until all the data is processed or discarded.
         */
        bytes = packet->content ? mprGetBufLength(packet->content) : packet->count;
        while (range && bytes > 0) {

            endpos = resp->pos + bytes;
            if (endpos < range->start) {
                /* Packet is before the next range, so discard the entire packet */
                resp->pos += bytes;
                mprFree(packet);
                break;

            } else if (resp->pos > range->end) {
                /* Missing some output - should not happen */
                mprAssert(0);

            } else if (resp->pos < range->start) {
                /*  Packets starts before range with some data in range so skip some data */
                count = range->start - resp->pos;
                bytes -= count;
                resp->pos += count;
                packet->count -= count;
                if (packet->content) {
                    mprAdjustBufStart(packet->content, count);
                }
                continue;

            } else {
                /* In range */
                mprAssert(range->start <= resp->pos && resp->pos < range->end);
                count = min(bytes, range->end - resp->pos);
                count = min(count, q->nextQ->packetSize);
    
                mprAssert(count > 0);
                
                if (count < bytes) {
                    //  TODO OPT> Only need to resize if this completes all the range data.
                    maResizePacket(q, packet, count);
                }
                if (!maWillNextQueueAccept(q, packet)) {
                    maPutBack(q, packet);
                    return;
                }
                if (fill) {
                    if ((*fill)(q, packet) < 0) {
                        return;
                    }
                }
                bytes -= count;
                resp->pos += count;
                if (resp->rangeBoundary) {
                    maPutNext(q, createRangePacket(conn, range));
                }
#if UNUSED
                //  TODO - is this being used anywhere?
                packet->pos = resp->pos;
#endif
                maPutNext(q, packet);
                if (resp->pos >= range->end) {
                    range = range->next;
                }
                break;
            }
        }
    }
    resp->currentRange = range;
}


static void outgoingRangeService(MaQueue *q)
{
    maRangeService(q, NULL);
}


/*
 *  Create a range boundary packet
 */
static MaPacket *createRangePacket(MaConn *conn, MaRange *range)
{
    MaPacket        *packet;
    MaResponse      *resp;
    char            lenBuf[16];

    resp = conn->response;

    if (resp->entityLength >= 0) {
        mprItoa(lenBuf, sizeof(lenBuf), resp->entityLength, 10);
    } else {
        lenBuf[0] = '*';
        lenBuf[1] = '\0';
    }
    packet = maCreatePacket(conn, MA_RANGE_BUFSIZE);
    packet->flags |= MA_PACKET_RANGE;
    packet->count = mprPutFmtToBuf(packet->content,
        "\r\n--%s\r\n"
        "Content-Type: %s\r\n"
        "Content-Range: bytes %d-%d/%s\r\n\r\n",
        resp->rangeBoundary, resp->mimeType, range->start, range->end - 1, lenBuf);

    return packet;
}


/*
 *  Create a final range packet that follows all the data
 */
static MaPacket *createFinalRangePacket(MaConn *conn)
{
    MaPacket        *packet;
    MaResponse      *resp;

    resp = conn->response;

    packet = maCreatePacket(conn, MA_RANGE_BUFSIZE);
    packet->flags |= MA_PACKET_RANGE;
    packet->count = mprPutFmtToBuf(packet->content, "\r\n--%s--\r\n", resp->rangeBoundary);
    return packet;
}


/*
 *  Create a range boundary. This is required if more than one range is requested.
 */
void maCreateRangeBoundary(MaConn *conn)
{
    MaResponse      *resp;

    resp = conn->response;

    mprAssert(resp->rangeBoundary == 0);
    mprAllocSprintf(resp, &resp->rangeBoundary, -1, "%08X%08X", PTOI(resp) + PTOI(conn) * (int) conn->time, (int) conn->time);
}


/*
 *  Ensure all the range limits are within the entity size limits. Fixup negative ranges.
 */
bool maFixRangeLength(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    MaRange     *range;
    int         length;

    req = conn->request;
    resp = conn->response;
    length = resp->entityLength;

    for (range = req->ranges; range; range = range->next) {
        /*
         *      Range: 0-49             first 50 bytes
         *      Range: 50-99,200-249    Two 50 byte ranges from 50 and 200
         *      Range: -50              Last 50 bytes
         *      Range: 1-               Skip first byte then emit the rest
         */
        if (length) {
            if (range->end > length) {
                range->end = length;
            }
            if (range->start > length) {
                range->start = length;
            }
        }
        if (range->start < 0) {
            if (length <= 0) {
                /*
                 *  Can't compute an offset from the end as we don't know the entity length
                 */
                return 0;
            }
            /* select last -range-end bytes */
            range->start = length - range->end + 1;
            range->end = length;
        }
        if (range->end < 0) {
            if (length <= 0) {
                return 0;
            }
            range->end = length - range->end - 1;
        }
        range->len = range->end - range->start;
    }
    return 1;
}


/*
 *  Loadable module initialization
 */
MprModule *maRangeFilterInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *filter;

    module = mprCreateModule(http, "rangeFilter", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    filter = maCreateFilter(http, "rangeFilter", MA_STAGE_ALL);
    if (filter == 0) {
        mprFree(module);
        return 0;
    }
    http->rangeFilter = filter;

    filter->outgoingService = outgoingRangeService; 

    return module;
}


#else
void __mprRangeFilterDummy() {}
#endif /* BLD_FEATURE_RANGE */

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
/************************************************************************/
/*
 *  End of file "../http/filters/rangeFilter.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/handlers/cgiHandler.c"
 */
/************************************************************************/

/* 
 *  cgiHandler.c -- Common Gateway Interface Handler
 *
 *  Support the CGI/1.1 standard for external gateway programs to respond to HTTP requests.
 *  This CGI handler uses async-pipes and non-blocking I/O for all communications.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



#if BLD_FEATURE_CGI

static void buildArgs(MaConn *conn, MprCmd *cmd, int *argcp, char ***argvp);
static void cgiCallback(MprCmd *cmd, int fd, int channel, void *data);
static char *getCgiToken(MprBuf *buf, cchar *delim);
static bool parseFirstCgiResponse(MaConn *conn, MprCmd *cmd);
static bool parseHeader(MaConn *conn, MprCmd *cmd);
static void pushDataToCgi(MaQueue *q);
static void startCmd(MaQueue *q);

#if BLD_DEBUG
static void traceCGIData(MprCmd *cmd, char *src, int size);
#define traceData(cmd, src, size) traceCGIData(cmd, src, size)
#else
#define traceData(cmd, src, size)
#endif

#if BLD_WIN_LIKE
static void findExecutable(MaConn *conn, char **program, char **script, char **bangScript, char *fileName);
static void checkCompletion(MprCmd *cmd, MprEvent *event);
#endif

/*
 *  Open this handler instance for a new request
 */
static void openCgi(MaQueue *q)
{
    MaRequest       *req;
    MaConn          *conn;

    conn = q->conn;
    req = conn->request;

    startCmd(q);
    maSetHeader(conn, 0, "Last-Modified", req->host->currentDate);
    maDontCacheResponse(conn);

    /*
     *  Create an empty header packet and add to the service queue
     */
    maPutForService(q, maCreateHeaderPacket(conn), 0);
}


/*
 *  Prepare and start the CGI command. Called from openCgi()
 */
static void startCmd(MaQueue *q)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaConn          *conn;
    MprCmd          *cmd;
    MprHashTable    *vars;
    MprHash         *hp;
    cchar           *baseName;
    char            **argv, **envv, *fileName, dir[MPR_MAX_FNAME];
    int             index, argc, varCount;

    argv = 0;
    vars = 0;
    argc = 0;

    conn = q->conn;
    req = conn->request;
    resp = conn->response;

    cmd = q->queueData = mprCreateCmd(req);

    /*
     *  Build the commmand line arguments
     */
    argc = 1;                                   /* argv[0] == programName */
    buildArgs(conn, cmd, &argc, &argv);
    fileName = argv[0];

    mprGetDirName(dir, sizeof(dir), fileName);
    baseName = mprGetBaseName(fileName);
    if (strncmp(baseName, "nph-", 4) == 0 || 
            (strlen(baseName) > 4 && strcmp(&baseName[strlen(baseName) - 4], "-nph") == 0)) {
        /*
         *  Pretend we've seen the header for Non-parsed Header CGI programs
         */
        cmd->userFlags |= MA_CGI_SEEN_HEADER;
    }

    //  TODO - refactor and move this into prepareArgs()
    /*
     *  Build environment variables. For unix, also export the PATH and LD_LIBRARY_PATH so add 2.
     */
    vars = req->headers;
    varCount = mprGetHashCount(vars);
#if BLD_HOST_UNIX
    varCount += 2;
#endif

    index = 0;
    envv = (char**) mprAlloc(cmd, (varCount + 1) * sizeof(char*));
    hp = mprGetFirstHash(req->headers);
    while (hp) {
        if (hp->data) {
            mprAllocSprintf(cmd, &envv[index], MPR_MAX_FNAME, "%s=%s", hp->key, (char*) hp->data);
            index++;
        }
        hp = mprGetNextHash(req->headers, hp);
    }

#if BLD_HOST_UNIX
    {
        char    *cp;
        if ((cp = getenv("PATH")) != 0) {
            mprAllocSprintf(cmd, &envv[index++], MPR_MAX_STRING, "PATH=%s", cp);
        }
        if ((cp = getenv("LD_LIBRARY_PATH")) != 0) {
            mprAllocSprintf(cmd, &envv[index++], MPR_MAX_STRING, "LD_LIBRARY_PATH=%s", cp);
        }
    }
#endif

    envv[index] = 0;
    mprAssert(index <= varCount);

#if BLD_DEBUG
{
    int     i;
    mprLog(q, 4, "CGI: running program: %s: ", fileName);
    for (i = 1; argv[i]; i++) {
        mprRawLog(q, 4, "%s ", argv[i]);
    }
    mprRawLog(q, 4, "\n");
    for (i = 0; i < index; i++) {
        mprLog(q, 4, "CGI ENV %s", envv[i]);
    }
}
#endif

    cmd->stdoutBuf = mprCreateBuf(cmd, MPR_BUFSIZE, -1);
    cmd->stderrBuf = mprCreateBuf(cmd, MPR_BUFSIZE, -1);

    mprSetCmdDir(cmd, dir);
    mprSetCmdCallback(cmd, cgiCallback, conn);

    if (mprStartCmd(cmd, argc, argv, envv, MPR_CMD_IN | MPR_CMD_OUT | MPR_CMD_ERR) < 0) {
        maFailRequest(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "Can't run CGI process: %s, URI %s", fileName, req->url);
    }
}


/*
 *  Accept a new packet of data destined for the browser. Initially, these packets are empty and need filling will data
 *  from the CGI program, so just queue for service (don't enable the service routine yet).
 */
static void outgoingCgiData(MaQueue *q, MaPacket *packet)
{
    maPutForService(q, packet, 0);
}


/*
 *  Service outgoing data destined for the browser.
 */ 
static void outgoingCgiService(MaQueue *q)
{
    MprCmd      *cmd;

    cmd = (MprCmd*) q->queueData;

    /*
     *  This will copy outgoing packets downstream toward the network connector and on to the browser. This may disable this 
     *  queue if the downstream net connector queue overflows because the socket is full. In that case, conn.c:setupConnIO() 
     *  will setup to listen for writable events. When the socket is writable again, the connector will drain its queue
     *  which will re-enable this queue and schedule it for service again.
     */ 
    maDefaultOutgoingServiceStage(q);

    //  TODO OPT - need a flag so we don't do this first time.
    if (q->count < q->low) {
        mprEnableCmdEvents(cmd, MPR_CMD_STDOUT);
    }
}


/*
 *  Accept incoming data from the browser destined for the CGI gateway (POST | PUT dat).
 */
static void incomingCgiData(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;
    MaResponse  *resp;
    MaRequest   *req;
    MprCmd      *cmd;

    conn = q->conn;
    resp = conn->response;
    req = conn->request;

    cmd = (MprCmd*) q->pair->queueData;
    mprAssert(cmd);

    if (packet->count == 0) {
        /*
         *  End of input
         */
        if (req->remainingContent > 0) {
            /*
             *  Short incoming body data. Just kill the CGI process.
             */
            mprFree(cmd);
            q->queueData = 0;
            maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Client supplied iinsufficient body data");
        }
        /*
         *  End of input. request.c should handle transitioning to call the run() routine for us automatically.
         */

    } else {
        /*
         *  No service routine, we just need it to be queued for pushDataToCgi
         */
        maPutForService(q, packet, 0);
    }
    pushDataToCgi(q);
}


/*
 *  Write data to the CGI program. (may block). This is called from incomingCgiData and from the cgiCallback when the pipe
 *  to the CGI program becomes writabl.
 */
static void pushDataToCgi(MaQueue *q)
{
    MaConn      *conn;
    MaPacket    *packet;
    MprCmd      *cmd;
    MprBuf      *buf;
    int         len, rc;

    cmd = (MprCmd*) q->pair->queueData;
    mprAssert(cmd);
    conn = q->conn;

    for (packet = maGet(q); packet && !conn->requestFailed; packet = maGet(q)) {
        buf = packet->content;
        len = mprGetBufLength(buf);
        mprAssert(len > 0);
        rc = mprWriteCmdPipe(cmd, MPR_CMD_STDIN, mprGetBufStart(buf), len);
        if (rc < 0) {
            mprLog(q, 2, "CGI: write to gateway failed for %d bytes, rc %d, errno %d\n", len, rc, errno);
            mprCloseCmdFd(cmd, MPR_CMD_STDIN);
            maFailRequest(conn, MPR_HTTP_CODE_BAD_GATEWAY, "Can't write body data to CGI gateway");
            break;

        } else {
            mprLog(q, 5, "CGI: write to gateway %d bytes asked to write %d\n", rc, len);
            mprAdjustBufStart(buf, rc);
            if (mprGetBufLength(buf) > 0) {
                maPutBack(q, packet);
            }
            if (rc < len) {
                /*
                 *  CGI gateway didn't accept all the data. Enable CGI write events to be notified when the gateway
                 *  can read more data.
                 */
                mprEnableCmdEvents(cmd, MPR_CMD_STDIN);
            }
        }
    }
}


/*
 *  Run after all incoming data has been received. Can now close the client's stdin.
 */
static void runCgi(MaQueue *q)
{
    MprCmd      *cmd;
    MaConn      *conn;

    conn = q->conn;
    cmd = (MprCmd*) q->queueData;

    /*
     *  Close the CGI program's stdin. This will allow it to exit if it was expecting input data.
     */
    if (q->queueData) {
        mprCloseCmdFd(cmd, MPR_CMD_STDIN);
    }
    if (conn->requestFailed) {
        maPutForService(q, maCreateEndPacket(conn), 1);
    }

#if BLD_WIN_LIKE
    /*
     *  Windows polls for completion
     */
    mprCreateTimerEvent(cmd, checkCompletion, MA_CGI_PERIOD, MPR_NORMAL_PRIORITY, cmd, 0);
#endif
}


#if BLD_WIN_LIKE
static void checkCompletion(MprCmd *cmd, MprEvent *event)
{
    mprPollCmd(cmd);
}
#endif


static int writeToBrowser(MaQueue *q, MprCmd *cmd, MprBuf *buf, int channel)
{
    MaConn  *conn;
    int     servicedQueues, rc, len;

    conn = q->conn;

    /*
     *  Write to the browser. We write as much as we can. Service queues to get the filters and connectors pumping.
     */
    for (servicedQueues = 0; (len = mprGetBufLength(buf)) > 0 ; ) {

        if (!conn->requestFailed) {
            rc = maWriteBlock(q, mprGetBufStart(buf), len, 0);
            mprLog(cmd, 5, "Write to browser ask %d, actual %d", len, rc);
        } else {
            rc = len;
        }
        if (rc > 0) {
            mprAdjustBufStart(buf, rc);
            mprResetBufIfEmpty(buf);

        } else if (rc == 0) {
            if (servicedQueues) {
                /*
                 *  Can't write anymore data. Block the CGI gateway. outgoingCgiService will enable.
                 */
                mprAssert(q->count >= q->max);
                mprAssert(q->flags & MA_QUEUE_DISABLED);
                mprDisableCmdEvents(cmd, channel);
                return MPR_ERR_CANT_WRITE;
                
            } else {
                maServiceQueues(conn);
                servicedQueues++;
            }
        }
    }
    return 0;
}


/*
 *  Read the output data from the CGI script and return it to the client
 */
static void cgiCallback(MprCmd *cmd, int fd, int channel, void *data)
{
    MaConn      *conn;
    MaResponse  *resp;
    MprBuf      *buf;
    MaQueue     *q;
    int         space, rc, bytesRead, closed;

    conn = data;
    resp = conn->response;
    q = resp->queue[MA_QUEUE_SEND].nextQ;
    buf = 0;
    closed = 0;

    switch (channel) {
    case MPR_CMD_STDIN:
        /*
         *  CGI's stdin is now accepting more data
         */
        mprDisableCmdEvents(cmd, MPR_CMD_STDIN);
        pushDataToCgi(q->pair);
        return;

    case MPR_CMD_STDOUT:
        buf = cmd->stdoutBuf;
        break;

    case MPR_CMD_STDERR:
        buf = cmd->stderrBuf;
        break;
    }

    mprAssert(buf);
    mprResetBufIfEmpty(buf);

    /*
     *  Come here for CGI stdout, stderr events. ie. we can read data from the CGI program.
     */
    while (!cmd->completed) {

        /*
         *  Read data from the CGI pipe and try to totally fill the buffer
         */
        while ((space = mprGetBufSpace(buf)) > 0) {
            bytesRead = mprReadCmdPipe(cmd, channel, mprGetBufEnd(buf), space);
            if (bytesRead <= 0) {
                if (bytesRead == 0 || !(errno == EAGAIN || EWOULDBLOCK || errno == EINTR)) {
                    /*
                     *  This will set cmd->completed when we've closed both stderr and stdout
                     */
                    rc = errno;
                    mprCloseCmdFd(cmd, channel);
                    mprLog(cmd, 5, "CGI Read from %s got %d, ask %d, errno %d",
                        (channel == MPR_CMD_STDOUT) ? "stdout" : "stderr", bytesRead, space, rc);
                    closed++;
                }
                break;

            } else {
                mprLog(cmd, 5, "CGI Read from %s got %d, ask %d",
                       (channel == MPR_CMD_STDOUT) ? "stdout" : "stderr", bytesRead, space);
                mprAdjustBufEnd(buf, bytesRead);
                traceData(cmd, mprGetBufStart(buf), bytesRead);
            }
        }
        if (mprGetBufLength(buf) == 0) {
            break;
        }
        if (channel == MPR_CMD_STDERR) {
            mprAddNullToBuf(buf);
            mprError(conn, mprGetBufStart(buf));
            mprFlushBuf(buf);
            
        } else {
            if (!(cmd->userFlags & MA_CGI_SEEN_HEADER)) {
                if (!parseHeader(conn, cmd)) {
                    return;
                }
            }
            if (writeToBrowser(q, cmd, buf, channel) < 0) {
                return;
            }
        }
        if (mprGetBufLength(buf) == 0 && closed) {
            break;
        }
    }

    if (cmd->completed) {
        if ((rc = mprReapCmd(cmd, MA_CGI_TIMEOUT) != 0) || cmd->status != 0) {
            maFailRequest(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE,
                "CGI process %s: exited abnormally with return code %d, exit status: %d", resp->filename, rc, cmd->status);
        } else if (!(cmd->userFlags & MA_CGI_SEEN_HEADER)) {
            maFailRequest(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "Header not seen");
        }
        /*
         *  Write an EOF packet downstream to the client and enable for service. This will Schedule outgoingCgiService.
         */            
        maPutForService(q, maCreateEndPacket(q->conn), 1);
        maServiceQueues(conn);
    }
    if (conn->state == MPR_HTTP_STATE_COMPLETE) {
        /*
         *  TODO - multithread race 
         *  Issue a dummy read event to cycle through the last stage of the request pipeline. This will complete
         *  the request and cleanup. WARNING - the request and cmd will be deleted after this.
         */
        maProcessReadEvent(conn, 0);
        maAwakenConn(conn);

    } else if (conn->requestFailed) {
        maServiceQueues(conn);
    }
}


/*
 *  Parse the CGI output first line
 */
static bool parseFirstCgiResponse(MaConn *conn, MprCmd *cmd)
{
    MaResponse      *resp;
    MprBuf          *buf;
    char            *protocol, *code, *message;
    
    resp = conn->response;
    buf = mprGetCmdBuf(cmd, MPR_CMD_STDOUT);
    
    protocol = getCgiToken(buf, " ");
    if (protocol == 0 || protocol[0] == '\0') {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_GATEWAY, "Bad CGI HTTP protocol response");
        return 0;
    }
    
    if (strncmp(protocol, "HTTP/1.", 7) != 0) {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_GATEWAY, "Unsupported CGI protocol");
        return 0;
    }
    
    code = getCgiToken(buf, " ");
    if (code == 0 || *code == '\0') {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_GATEWAY, "Bad CGI header response");
        return 0;
    }
    message = getCgiToken(buf, "\n");
    
    mprLog(conn, 4, "CGI status line: %s %s %s", protocol, code, message);
    return 1;
}


/*
 *  Parse the CGI output headers. 
 *  Sample CGI program:
 *
 *  Content-type: text/html
 * 
 *  <html.....
 */
static bool parseHeader(MaConn *conn, MprCmd *cmd)
{
    MaResponse      *resp;
    MprBuf          *buf;
    char            *endHeaders, *headers, *key, *value, *location;
    int             len;

    resp = conn->response;
    location = 0;
    value = 0;

    buf = mprGetCmdBuf(cmd, MPR_CMD_STDOUT);
    headers = mprGetBufStart(buf);

    /*
     *  Split the headers from the body.
     */
    len = 0;
    if ((endHeaders = strstr(headers, "\r\n\r\n")) == NULL) {
        if ((endHeaders = strstr(headers, "\n\n")) == NULL) {
            return 0;
        } 
        len = 2;
    } else {
        len = 4;
    }

    endHeaders[len - 1] = '\0';
    endHeaders += len;

    /*
     *  Want to be tolerant of CGI programs that omit the status line.
     */
    if (strncmp((char*) buf->start, "HTTP/1.", 7) == 0) {
        if (!parseFirstCgiResponse(conn, cmd)) {
            return 0;
        }
    }
    
    if (strchr(mprGetBufStart(buf), ':')) {
        mprLog(conn, 4, "CGI: parseHeader: header\n%s\n", headers);

        while (mprGetBufLength(buf) > 0 && buf->start[0] && (buf->start[0] != '\r' && buf->start[0] != '\n')) {

            if ((key = getCgiToken(buf, ":")) == 0) {
                maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad header format");
                return 0;
            }
            value = getCgiToken(buf, "\n");
            while (isspace((int) *value)) {
                value++;
            }
            len = (int) strlen(value);
            while (len > 0 && (value[len - 1] == '\r' || value[len - 1] == '\n')) {
                value[len - 1] = '\0';
                len--;
            }
            mprStrLower(key);

            if (strcmp(key, "location") == 0) {
                location = value;

            } else if (strcmp(key, "status") == 0) {
                maSetResponseCode(conn, atoi(value));

            } else if (strcmp(key, "content-type") == 0) {
                maSetResponseMimeType(conn, value);

            } else {
                /*
                 *  Now pass all other headers back to the client
                 */
                maSetHeader(conn, 0, key, "%s", value);
            }
        }
        buf->start = (uchar*) endHeaders;
    }
    if (location) {
        maRedirect(conn, resp->code, value);
    }
    cmd->userFlags |= MA_CGI_SEEN_HEADER;
    return 1;
}


/*
 *  Build the command arguments. NOTE: argv is untrusted input.
 */
static void buildArgs(MaConn *conn, MprCmd *cmd, int *argcp, char ***argvp)
{
    MaRequest   *req;
    MaResponse  *resp;
    char        *fileName, **argv, *program, *cmdScript, status[8], *indexQuery, escape[MPR_MAX_STRING], *cp, *tok;
    cchar       *actionProgram;
    int         argc, argind, len;

    req = conn->request;
    resp = conn->response;

    fileName = resp->filename;
    mprAssert(fileName);

    program = cmdScript = 0;
    actionProgram = 0;
    argind = 0;
    argc = *argcp;

    if (req->mimeType) {
        actionProgram = maGetMimeActionProgram(req->host, req->mimeType);
        if (actionProgram != 0) {
            argc++;
            /*
             *  This is an Apache compatible hack
             */
            mprItoa(status, sizeof(status), MPR_HTTP_CODE_MOVED_TEMPORARILY, 10);
            mprAddHash(req->headers, "REDIRECT_STATUS", status);
        }
    }

    /*
     *  Count the args for ISINDEX queries. Only valid if there is not a "=" in the query. 
     *  If this is so, then we must not have these args in the query env also?
     */
    indexQuery = req->parsedUri->query;
    if (indexQuery && !strchr(indexQuery, '=')) {
        argc++;
        for (cp = indexQuery; *cp; cp++) {
            if (*cp == '+') {
                argc++;
            }
        }
    } else {
        indexQuery = 0;
    }

#if BLD_WIN_LIKE
{
    char    *bangScript, *cmdBuf;

    /*
     *  On windows we attempt to find an executable matching the fileName.
     *  We look for *.exe, *.bat and also do unix style processing "#!/program"
     */
    findExecutable(conn, &program, &cmdScript, &bangScript, fileName);
    mprAssert(program);

    if (cmdScript) {
        /*
         *  Cmd/Batch script (.bat | .cmd)
         *  Convert the command to the form where there are 4 elements in argv
         *  that cmd.exe can interpret.
         *
         *      argv[0] = cmd.exe
         *      argv[1] = /Q
         *      argv[2] = /C
         *      argv[3] = ""script" args ..."
         */
        argc = 4;

        len = (argc + 1) * sizeof(char*);
        argv = (char**) mprAlloc(cmd, len);
        memset(argv, 0, len);

        argv[argind++] = program;               /* Duped in findExecutable */
        argv[argind++] = mprStrdup(cmd, "/Q");
        argv[argind++] = mprStrdup(cmd, "/C");

        len = (int) strlen(cmdScript) + 2 + 1;
        cmdBuf = (char*) mprAlloc(cmd, len);
        mprSprintf(cmdBuf, len, "\"%s\"", cmdScript);
        argv[argind++] = cmdBuf;

        mprSetCmdDir(cmd, cmdScript);
        mprFree(cmdScript);
        /*  program will get freed when argv[] gets freed */
        
    } else if (bangScript) {
        /*
         *  Script used "#!/program". NOTE: this may be overridden by a mime
         *  Action directive.
         */
        argc++;     /* Adding bangScript arg */

        len = (argc + 1) * sizeof(char*);
        argv = (char**) mprAlloc(cmd, len);
        memset(argv, 0, len);

        argv[argind++] = program;       /* Will get freed when argv[] is freed */
        argv[argind++] = bangScript;    /* Will get freed when argv[] is freed */
        mprSetCmdDir(cmd, bangScript);

    } else {
        /*
         *  Either unknown extension or .exe program.
         */
        len = (argc + 1) * sizeof(char*);
        argv = (char**) mprAlloc(cmd, len);
        memset(argv, 0, len);

        if (actionProgram) {
            argv[argind++] = mprStrdup(cmd, actionProgram);
        }
        argv[argind++] = program;
    }
}
#else
    len = (argc + 1) * sizeof(char*);
    argv = (char**) mprAlloc(cmd, len);
    memset(argv, 0, len);

    if (actionProgram) {
        argv[argind++] = mprStrdup(cmd, actionProgram);
    }
    argv[argind++] = mprStrdup(cmd, fileName);

#endif

    /*
     *  ISINDEX queries. Only valid if there is not a "=" in the query. If this is so, then we must not
     *  have these args in the query env also?
     *  TODO - should query vars be set in the env?
     */
    if (indexQuery) {
        indexQuery = mprStrdup(cmd, indexQuery);

        cp = mprStrTok(indexQuery, "+", &tok);
        while (cp) {
            mprUrlDecode(cp, (int) strlen(cp), cp);
            mprEscapeCmd(escape, sizeof(escape), cp, 0);
            argv[argind++] = mprStrdup(cmd, escape);
            cp = mprStrTok(NULL, "+", &tok);
        }
    }
    
    mprAssert(argind == argc);
    argv[argind] = 0;
    *argcp = argc;
    *argvp = argv;
}


#if BLD_WIN_LIKE
/*
 *  If the program has a UNIX style "#!/program" string at the start of the file that program will be selected 
 *  and the original program will be passed as the first arg to that program with argv[] appended after that. If 
 *  the program is not found, this routine supports a safe intelligent search for the command. If all else fails, 
 *  we just return in program the fileName we were passed in. script will be set if we are modifying the program 
 *  to run and we have extracted the name of the file to run as a script.
 */
static void findExecutable(MaConn *conn, char **program, char **script, char **bangScript, char *fileName)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaLocation      *location;
    MprHash         *hp;
    MprFile         *file;
    cchar           *actionProgram, *ext, *cmdShell;
    char            buf[MPR_MAX_FNAME + 1], pathBuf[MPR_MAX_FNAME];
    char            dirBuf[MPR_MAX_FNAME + 1], tmp[MPR_MAX_FNAME];
    char            *tok;

    req = conn->request;
    resp = conn->response;
    location = req->location;

    *bangScript = 0;
    *script = 0;
    *program = 0;

    actionProgram = maGetMimeActionProgram(conn->host, req->mimeType);
    ext = resp->extension;

    /*
     *  If not found, go looking for the fileName with the extensions defined in appweb.conf. 
     *  NOTE: we don't use PATH deliberately!!!
     */
    if (access(fileName, X_OK) < 0 && *ext == '\0') {
        for (hp = 0; (hp = mprGetNextHash(location->extensions, hp)) != 0; ) {
            mprSprintf(pathBuf, sizeof(pathBuf), "%s.%s", fileName, hp->key);
            if (access(pathBuf, X_OK) == 0) {
                break;
            }
        }
        if (hp) {
            ext = hp->key;
        } else {
            mprStrcpy(pathBuf, sizeof(pathBuf), fileName);
        }

    } else {
        mprStrcpy(pathBuf, sizeof(pathBuf), fileName);
    }

    if (ext && (strcmp(ext, ".bat") == 0 || strcmp(ext, ".cmd") == 0)) {
        /*
         *  Let a mime action override COMSPEC
         */
        if (actionProgram) {
            cmdShell = actionProgram;
        } else {
            cmdShell = getenv("COMSPEC");
        }
        if (cmdShell == 0) {
            cmdShell = "cmd.exe";
        }
        *script = mprStrdup(resp, pathBuf);
        *program = mprStrdup(resp, cmdShell);
        return;
    }

    if ((file = mprOpen(resp, pathBuf, O_RDONLY, 0)) != 0) {
        if (mprRead(file, buf, MPR_MAX_FNAME) > 0) {
            mprFree(file);
            buf[MPR_MAX_FNAME] = '\0';
            if (buf[0] == '#' && buf[1] == '!') {
                cmdShell = mprStrTok(&buf[2], " \t\r\n", &tok);
                if (cmdShell[0] != '/' && (cmdShell[0] != '\0' && cmdShell[1] != ':')) {
                    /*
                     *  If we can't access the command shell and the command 
                     *  is not an absolute path, look in the same directory 
                     *  as the script.
                     */
                    if (mprAccess(resp, cmdShell, X_OK)) {
                        mprGetDirName(dirBuf, sizeof(dirBuf), pathBuf);
                        mprSprintf(tmp, sizeof(tmp), "%s/%s", dirBuf, cmdShell);
                        cmdShell = tmp;
                    }
                }
                if (actionProgram) {
                    *program = mprStrdup(resp, actionProgram);
                } else {
                    *program = mprStrdup(resp, cmdShell);
                }
                *bangScript = mprStrdup(resp, pathBuf);
                return;
            }
        } else {
            mprFree(file);
        }
    }

    if (actionProgram) {
        *program = mprStrdup(resp, actionProgram);
        *bangScript = mprStrdup(resp, pathBuf);
    } else {
        *program = mprStrdup(resp, pathBuf);
    }
    return;
}
#endif
 

/*
 *  Get the next input token. The content buffer is advanced to the next token. This routine always returns a 
 *  non-zero token. The empty string means the delimiter was not found.
 */
static char *getCgiToken(MprBuf *buf, cchar *delim)
{
    char    *token, *nextToken;
    int     len;

    len = mprGetBufLength(buf);
    if (len == 0) {
        return "";
    }

    token = mprGetBufStart(buf);
    nextToken = mprStrnstr(mprGetBufStart(buf), delim, len);
    if (nextToken) {
        *nextToken = '\0';
        len = (int) strlen(delim);
        nextToken += len;
        buf->start = (uchar*) nextToken;

    } else {
        buf->start = (uchar*) mprGetBufEnd(buf);
    }
    return token;
}


#if BLD_DEBUG
/*
 *  Trace output received from the cgi process
 */
static void traceCGIData(MprCmd *cmd, char *src, int size)
{
    char    dest[512];
    int     index, i;

    mprRawLog(cmd, 5, "@@@ CGI process wrote => \n");

    for (index = 0; index < size; ) { 
        for (i = 0; i < (sizeof(dest) - 1) && index < size; i++) {
            dest[i] = src[index];
            index++;
        }
        dest[i] = '\0';
        mprRawLog(cmd, 5, "%s", dest);
    }
    mprRawLog(cmd, 5, "\n");
}
#endif


#if BLD_FEATURE_CONFIG_PARSE
static int parseCgi(MaHttp *http, cchar *key, char *value, MaConfigState *state)
{
    MaLocation  *location;
    MaServer    *server;
    MaHost      *host;
    MaAlias     *alias;
    MaDir       *dir, *parent;
    char        pathBuf[MPR_MAX_FNAME];
    char        *program, *mimeType, *prefix, *path;

    host = state->host;
    server = state->server;
    location = state->location;

    if (mprStrcmpAnyCase(key, "Action") == 0) {
        if (maSplitConfigValue(http, &mimeType, &program, value, 1) < 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
        maSetMimeActionProgram(host, mimeType, program);
        return 1;

    } else if (mprStrcmpAnyCase(key, "ScriptAlias") == 0) {
        if (maSplitConfigValue(server, &prefix, &path, value, 1) < 0 || path == 0 || prefix == 0) {
            return MPR_ERR_BAD_SYNTAX;
        }

        /*
         *  Create an alias and location with a cgiHandler and pathInfo processing
         */
        maMakePath(host, pathBuf, sizeof(pathBuf), path);

        dir = maLookupDir(host, pathBuf);
        if (maLookupDir(host, pathBuf) == 0) {
            parent = mprGetFirstItem(host->dirs);
            dir = maCreateDir(host, pathBuf, parent);
#if UNUSED
            mprError(http, "Missing directory block at %s for alias %s", pathBuf, prefix);
            return MPR_ERR_NOT_FOUND;
#endif
        }
        alias = maCreateAlias(host, prefix, pathBuf, 0);
        mprLog(server, 4, "ScriptAlias \"%s\" for \"%s\"", prefix, pathBuf);
        maInsertAlias(host, alias);

        if ((location = maLookupLocation(host, prefix)) == 0) {
            location = maCreateLocation(host, state->location);
            maSetLocationAuth(location, state->dir->auth);
            maSetLocationPrefix(location, prefix);
            maAddLocation(host, location);
        } else {
            maSetLocationPrefix(location, prefix);
        }
#if UNUSED
        maSetLocationFlags(location, MA_LOC_PATH_INFO);
#endif
        maSetHandler(http, host, location, "cgiHandler");
        return 1;
    }

    return 0;
}
#endif


/*
 *  Dynamic module initialization
 */
MprModule *maCgiHandlerInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *handler;

    module = mprCreateModule(http, "cgiHandler", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    handler = maCreateHandler(http, "cgiHandler", 
        MA_STAGE_ALL | MA_STAGE_FORM_VARS | MA_STAGE_ENV_VARS | MA_STAGE_PATH_INFO);
    if (handler == 0) {
        mprFree(module);
        return 0;
    }

    handler->open = openCgi; 
    handler->outgoingData = outgoingCgiData;
    handler->outgoingService = outgoingCgiService;
    handler->incomingData = incomingCgiData; 
    handler->run = runCgi; 
    handler->parse = parseCgi; 

    return module;
}


#else
void mprCgiHandlerDummy() {}

#endif /* BLD_FEATURE_CGI */

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
/************************************************************************/
/*
 *  End of file "../http/handlers/cgiHandler.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/handlers/dirHandler.c"
 */
/************************************************************************/

/*
 *  dirHandler.c - Directory listing handler.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



#if BLD_FEATURE_DIR

/*
 *  Handler configuration
 */
typedef struct Dir {
    cchar           *defaultIcon;
    MprList         *dirList;
    bool            enabled;
    MprList         *extList;
    int             fancyIndexing;
    bool            foldersFirst;
    MprList         *ignoreList;
    cchar           *pattern;
    char            *sortField;
    int             sortOrder;              /* 1 == ascending, -1 descending */
} Dir;



static void filterDirList(MaConn *conn, MprList *list);
static int  match(cchar *pattern, cchar *file);
static void outputFooter(MaQueue *q);
static void outputHeader(MaQueue *q, cchar *dir, int nameSize);
static void outputLine(MaQueue *q, MprDirEntry *ep, cchar *dir, int nameSize);
static void parseQuery(MaConn *conn);
static void parseWords(MprList *list, cchar *str);
static void sortList(MaConn *conn, MprList *list);

/*
 *  Match if the url maps to a directory.
 */
static bool matchDir(MaConn *conn, MaStage *handler, cchar *url)
{
    MaResponse      *resp;
    MprFileInfo     *info;
    Dir             *dir;

    resp = conn->response;
    info = &resp->fileInfo;
    dir = handler->stageData;
    
    if (!info->valid && mprGetFileInfo(conn, resp->filename, info) < 0) {
        return 0;
    }
    return dir->enabled && info->isDir;
}


static void runDir(MaQueue *q)
{
    MaConn          *conn;
    MaResponse      *resp;
    MaRequest       *req;
    MprList         *list;
    MprDirEntry     *dp;
    Dir             *dir;
    cchar           *filename;
    uint            nameSize;
    int             next;

    conn = q->conn;
    req = conn->request;
    resp = conn->response;
    dir = q->stage->stageData;

    filename = resp->filename;
    mprAssert(filename);

    maDontCacheResponse(conn);
    maSetHeader(conn, 0, "Last-Modified", req->host->currentDate);
    maPutForService(q, maCreateHeaderPacket(conn), 0);

    parseQuery(conn);

    list = mprGetDirList(conn, filename, 1);
    if (list == 0) {
        maWrite(q, "<h2>Can't get file list</h2>\r\n");
        outputFooter(q);
        return;
    }

    if (dir->pattern) {
        filterDirList(conn, list);
    }

    sortList(conn, list);

    /*
     *  Get max filename
     */
    nameSize = 0;
    for (next = 0; (dp = mprGetNextItem(list, &next)) != 0; ) {
        nameSize = max((int) strlen(dp->name), nameSize);
    }
    nameSize = max(nameSize, 22);

    outputHeader(q, req->url, nameSize);
    for (next = 0; (dp = mprGetNextItem(list, &next)) != 0; ) {
        outputLine(q, dp, filename, nameSize);
    }
    outputFooter(q);

    maPutForService(q, maCreateEndPacket(conn), 1);

    mprFree(list);
}
 

static void parseQuery(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    Dir         *dir;
    char        *value, *query, *next, *tok;

    req = conn->request;
    resp = conn->response;
    dir = resp->handler->stageData;
    
    query = mprStrdup(req, req->parsedUri->query);
    if (query == 0) {
        return;
    }

    tok = mprStrTok(query, ";&", &next);
    while (tok) {
        if ((value = strchr(tok, '=')) != 0) {
            *value++ = '\0';
            if (*tok == 'C') {                  /* Sort column */
                mprFree(dir->sortField);
                if (*value == 'N') {
                    dir->sortField = "Name";
                } else if (*value == 'M') {
                    dir->sortField = "Date";
                } else if (*value == 'S') {
                    dir->sortField = "Size";
                }
                dir->sortField = mprStrdup(dir, dir->sortField);

            } else if (*tok == 'O') {           /* Sort order */
                if (*value == 'A') {
                    dir->sortOrder = 1;
                } else if (*value == 'D') {
                    dir->sortOrder = -1;
                }

            } else if (*tok == 'F') {           /* Format */ 
                if (*value == '0') {
                    dir->fancyIndexing = 0;
                } else if (*value == '1') {
                    dir->fancyIndexing = 1;
                } else if (*value == '2') {
                    dir->fancyIndexing = 2;
                }

            } else if (*tok == 'P') {           /* Pattern */ 
                dir->pattern = mprStrdup(dir, value);
            }
        }
        tok = mprStrTok(next, ";&", &next);
    }
    
    mprFree(query);
}


static void sortList(MaConn *conn, MprList *list)
{
    MaRequest       *req;
    MaResponse      *resp;
    MprDirEntry     *tmp, **items;
    Dir             *dir;
    int             count, i, j, rc;

    req = conn->request;
    resp = conn->response;
    dir = resp->handler->stageData;
    
    if (dir->sortField == 0) {
        return;
    }

    count = mprGetListCount(list);
    items = (MprDirEntry**) list->items;
    if (mprStrcmpAnyCase(dir->sortField, "Name") == 0) {
        for (i = 1; i < count; i++) {
            for (j = 0; j < i; j++) {
                rc = strcmp(items[i]->name, items[j]->name);
                if (dir->foldersFirst) {
                    if (items[i]->isDir && !items[j]->isDir) {
                        rc = -dir->sortOrder;
                    } else if (items[j]->isDir && !items[i]->isDir) {
                        rc = dir->sortOrder;
                    } 
                }
                rc *= dir->sortOrder;
                if (rc < 0) {
                    tmp = items[i];
                    items[i] = items[j];
                    items[j] = tmp;
                }
            }
        }

    } else if (mprStrcmpAnyCase(dir->sortField, "Size") == 0) {
        for (i = 1; i < count; i++) {
            for (j = 0; j < i; j++) {
                rc = (items[i]->size < items[j]->size) ? -1 : 1;
                if (dir->foldersFirst) {
                    if (items[i]->isDir && !items[j]->isDir) {
                        rc = -dir->sortOrder;
                    } else if (items[j]->isDir && !items[i]->isDir) {
                        rc = dir->sortOrder;
                    }
                }
                rc *= dir->sortOrder;
                if (rc < 0) {
                    tmp = items[i];
                    items[i] = items[j];
                    items[j] = tmp;
                }
            }
        }

    } else if (mprStrcmpAnyCase(dir->sortField, "Date") == 0) {
        for (i = 1; i < count; i++) {
            for (j = 0; j < i; j++) {
                rc = (items[i]->lastModified < items[j]->lastModified) ? -1: 1;
                if (dir->foldersFirst) {
                    if (items[i]->isDir && !items[j]->isDir) {
                        rc = -dir->sortOrder;
                    } else if (items[j]->isDir && !items[i]->isDir) {
                        rc = dir->sortOrder;
                    }
                }
                rc *= dir->sortOrder;
                if (rc < 0) {
                    tmp = items[i];
                    items[i] = items[j];
                    items[j] = tmp;
                }
            }
        }
    }
}


static void outputHeader(MaQueue *q, cchar *path, int nameSize)
{
    Dir     *dir;
    char    parent[MPR_MAX_FNAME], *parentSuffix;
    int     order, reverseOrder, fancy, isRootDir;

    dir = q->stage->stageData;
    
    fancy = 1;

    maWrite(q, "<!DOCTYPE HTML PUBLIC \"-/*W3C//DTD HTML 3.2 Final//EN\">\r\n");
    maWrite(q, "<html>\r\n <head>\r\n  <title>Index of %s</title>\r\n", path);
    maWrite(q, " </head>\r\n");
    maWrite(q, "<body>\r\n");

    maWrite(q, "<h1>Index of %s</h1>\r\n", path);

    if (dir->sortOrder > 0) {
        order = 'A';
        reverseOrder = 'D';
    } else {
        order = 'D';
        reverseOrder = 'A';
    }

    if (dir->fancyIndexing == 0) {
        fancy = '0';
    } else if (dir->fancyIndexing == 1) {
        fancy = '1';
    } else if (dir->fancyIndexing == 2) {
        fancy = '2';
    }

    mprGetDirName(parent, sizeof(parent), (char*) path);

    if (parent[strlen(parent) - 1] != '/') {
        parentSuffix = "/";
    } else {
        parentSuffix = "";
    }

    isRootDir = (strcmp(path, "/") == 0);

    if (dir->fancyIndexing == 2) {
        maWrite(q, "<table><tr><th><img src=\"/icons/blank.gif\" alt=\"[ICO]\" /></th>");

        maWrite(q, "<th><a href=\"?C=N;O=%c;F=%c\">Name</a></th>", reverseOrder, fancy);
        maWrite(q, "<th><a href=\"?C=M;O=%c;F=%c\">Last modified</a></th>", reverseOrder, fancy);
        maWrite(q, "<th><a href=\"?C=S;O=%c;F=%c\">Size</a></th>", reverseOrder, fancy);
        maWrite(q, "<th><a href=\"?C=D;O=%c;F=%c\">Description</a></th>\r\n", reverseOrder, fancy);

        maWrite(q, "</tr><tr><th colspan=\"5\"><hr /></th></tr>\r\n");

        if (! isRootDir) {
            maWrite(q, "<tr><td valign=\"top\"><img src=\"/icons/back.gif\"");
            maWrite(q, "alt=\"[DIR]\" /></td><td><a href=\"%s%s\">", parent, parentSuffix);
            maWrite(q, "Parent Directory</a></td>");
            maWrite(q, "<td align=\"right\">  - </td></tr>\r\n");
        }

    } else if (dir->fancyIndexing == 1) {
        maWrite(q, "<pre><img src=\"/icons/space.gif\" alt=\"Icon\" /> ");

        maWrite(q, "<a href=\"?C=N;O=%c;F=%c\">Name</a>%*s", reverseOrder, fancy, nameSize - 3, " ");
        maWrite(q, "<a href=\"?C=M;O=%c;F=%c\">Last modified</a>       ", reverseOrder, fancy);
        maWrite(q, "<a href=\"?C=S;O=%c;F=%c\">Size</a>               ", reverseOrder, fancy);
        maWrite(q, "<a href=\"?C=D;O=%c;F=%c\">Description</a>\r\n", reverseOrder, fancy);

        maWrite(q, "<hr />");

        if (! isRootDir) {
            maWrite(q, "<img src=\"/icons/parent.gif\" alt=\"[DIR]\" />");
            maWrite(q, " <a href=\"%s%s\">Parent Directory</a>\r\n", parent, parentSuffix);
        }

    } else {
        maWrite(q, "<ul>\n");
        if (! isRootDir) {
            maWrite(q, "<li><a href=\"%s%s\"> Parent Directory</a></li>\r\n", parent, parentSuffix);
        }
    }
}


static void fmtNum(char *buf, int bufsize, int num, int divisor, char *suffix)
{
    int     whole, point;

    whole = num / divisor;
    point = (num % divisor) / (divisor / 10);

    if (point == 0) {
        mprSprintf(buf, bufsize, "%6d%s", whole, suffix);
    } else {
        mprSprintf(buf, bufsize, "%4d.%d%s", whole, point, suffix);
    }
}


static void outputLine(MaQueue *q, MprDirEntry *ep, cchar *path, int nameSize)
{
    MprFileInfo fileInfo;
    Dir         *dir;
    MprTime     when;
    MaHost      *host;
    char        newpath[MPR_MAX_FNAME], sizeBuf[16], timeBuf[48], *icon;
    struct tm   tm;
    bool        isDir;
    int         len;
    cchar       *ext, *mimeType;
    char        *dirSuffix;
    char        *months[] = { 
                    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" 
                };

    dir = q->stage->stageData;
    if (ep->size >= (1024*1024*1024)) {
        fmtNum(sizeBuf, sizeof(sizeBuf), (int) ep->size, 1024 * 1024 * 1024, "G");

    } else if (ep->size >= (1024*1024)) {
        fmtNum(sizeBuf, sizeof(sizeBuf), (int) ep->size, 1024 * 1024, "M");

    } else if (ep->size >= 1024) {
        fmtNum(sizeBuf, sizeof(sizeBuf), (int) ep->size, 1024, "K");

    } else {
        mprSprintf(sizeBuf, sizeof(sizeBuf), "%6d", (int) ep->size);
    }

    mprSprintf(newpath, sizeof(newpath), "%s/%s", path, ep->name);

    if (mprGetFileInfo(q, newpath, &fileInfo) < 0) {
        when = mprGetTime(q);
        isDir = 0;

    } else {
        isDir = fileInfo.isDir ? 1 : 0;
    }

    if (isDir) {
        icon = "folder";
        dirSuffix = "/";
    } else {
        host = q->conn->host;
        ext = mprGetExtension(ep->name);
        if ((mimeType = maLookupMimeType(host, ext)) != 0) {
            if (strcmp(ext, "es") == 0 || strcmp(ext, "ejs") == 0 || strcmp(ext, "php") == 0) {
                icon = "text";
            } else if (strstr(mimeType, "text") != 0) {
                icon = "text";
            } else {
                icon = "compressed";
            }
        } else {
            icon = "compressed";
        }
        dirSuffix = "";
    }

    when = (MprTime) fileInfo.mtime * MPR_TICKS_PER_SEC;
    mprLocaltime(q, &tm, when);

    mprSprintf(timeBuf, sizeof(timeBuf), "%02d-%3s-%4d %02d:%02d",
        tm.tm_mday, months[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour,  tm.tm_min);

    len = (int) strlen(ep->name) + (int) strlen(dirSuffix);

    if (dir->fancyIndexing == 2) {

        maWrite(q, "<tr><td valign=\"top\">");
        maWrite(q, "<img src=\"/icons/%s.gif\" alt=\"[   ]\", /></td>", icon);
        maWrite(q, "<td><a href=\"%s%s\">%s%s</a></td>", ep->name, dirSuffix, ep->name, dirSuffix);
        maWrite(q, "<td>%s</td><td>%s</td></tr>\r\n", timeBuf, sizeBuf);

    } else if (dir->fancyIndexing == 1) {

        maWrite(q, "<img src=\"/icons/%s.gif\" alt=\"[   ]\", /> ", icon);
        maWrite(q, "<a href=\"%s%s\">%s%s</a>%-*s %17s %4s\r\n", ep->name, dirSuffix, ep->name, dirSuffix, nameSize - len, "", 
            timeBuf, sizeBuf);

    } else {
        maWrite(q, "<li><a href=\"%s%s\"> %s%s</a></li>\r\n", ep->name, dirSuffix, ep->name, dirSuffix);
    }
}


static void outputFooter(MaQueue *q)
{
    MaRequest   *req;
    MaConn      *conn;
    MprSocket   *sock;
    Dir         *dir;
    
    conn = q->conn;
    req = conn->request;
    dir = q->stage->stageData;
    
    if (dir->fancyIndexing == 2) {
        maWrite(q, "<tr><th colspan=\"5\"><hr /></th></tr>\r\n</table>\r\n");
        
    } else if (dir->fancyIndexing == 1) {
        maWrite(q, "<hr /></pre>\r\n");
    } else {
        maWrite(q, "</ul>\r\n");
    }
    
    sock = conn->sock->listenSock;
    maWrite(q, "<address>%s %s at %s Port %d</address>\r\n", BLD_NAME, BLD_VERSION, sock->ipAddr, sock->port);
    maWrite(q, "</body></html>\r\n");
}


static void filterDirList(MaConn *conn, MprList *list)
{
    Dir             *dir;
    MprDirEntry     *dp;
    int             next;

    dir = conn->response->handler->stageData;
    
    /*
     *  Do pattern matching. Entries that don't match, free the name to mark
     */
    for (next = 0; (dp = mprGetNextItem(list, &next)) != 0; ) {
        if (! match(dir->pattern, dp->name)) {
            mprRemoveItem(list, dp);
            mprFree(dp);
            next--;
        }
    }
}


/*
 *  Return true if the file matches the pattern. Supports '?' and '*'
 */
static int match(cchar *pattern, cchar *file)
{
    cchar   *pp, *fp;

    if (pattern == 0 || *pattern == '\0') {
        return 1;
    }
    if (file == 0 || *file == '\0') {
        return 0;
    }

    for (pp = pattern, fp = file; *pp; ) {
        if (*fp == '\0') {
            if (*pp == '*' && pp[1] == '\0') {
                /* Trailing wild card */
                return 1;
            }
            return 0;
        }

        if (*pp == '*') {
            if (match(&pp[1], &fp[0])) {
                return 1;
            }
            fp++;
            continue;

        } else if (*pp == '?' || *pp == *fp) {
            fp++;

        } else {
            return 0;
        }
        pp++;
    }
    if (*fp == '\0') {
        /* Match */
        return 1;
    }
    return 0;
}


#if BLD_FEATURE_CONFIG_PARSE
static int parseDir(MaHttp *http, cchar *key, char *value, MaConfigState *state)
{
    MaStage     *handler;
    Dir         *dir;
    
    char    *name, *extensions, *option, *nextTok, *junk;

    handler = maLookupStage(http, "dirHandler");
    dir = handler->stageData;
    mprAssert(dir);
    
    if (mprStrcmpAnyCase(key, "AddIcon") == 0) {
        /*  AddIcon file ext ext ext */
        /*  Not yet supported */
        name = mprStrTok(value, " \t", &extensions);
        parseWords(dir->extList, extensions);
        return 1;

    } else if (mprStrcmpAnyCase(key, "DefaultIcon") == 0) {
        /*  DefaultIcon file */
        /*  Not yet supported */
        dir->defaultIcon = mprStrTok(value, " \t", &junk);
        return 1;

    } else if (mprStrcmpAnyCase(key, "IndexOrder") == 0) {
        /*  IndexOrder ascending|descending name|date|size */
        mprFree(dir->sortField);
        dir->sortField = 0;
        option = mprStrTok(value, " \t", &dir->sortField);
        if (mprStrcmpAnyCase(option, "ascending") == 0) {
            dir->sortOrder = 1;
        } else {
            dir->sortOrder = -1;
        }
        if (dir->sortField) {
            dir->sortField = mprStrdup(dir, dir->sortField);
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "IndexIgnore") == 0) {
        /*  IndexIgnore pat ... */
        /*  Not yet supported */
        parseWords(dir->ignoreList, value);
        return 1;

    } else if (mprStrcmpAnyCase(key, "IndexOptions") == 0) {
        /*  IndexOptions FancyIndexing|FoldersFirst ... (set of options) */
        option = mprStrTok(value, " \t", &nextTok);
        while (option) {
            if (mprStrcmpAnyCase(option, "FancyIndexing") == 0) {
                dir->fancyIndexing = 1;
            } else if (mprStrcmpAnyCase(option, "HTMLTable") == 0) {
                dir->fancyIndexing = 2;
            } else if (mprStrcmpAnyCase(option, "FoldersFirst") == 0) {
                dir->foldersFirst = 1;
            }
            option = mprStrTok(nextTok, " \t", &nextTok);
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "Options") == 0) {
        /*  Options Indexes */
        option = mprStrTok(value, " \t", &nextTok);
        while (option) {
            if (mprStrcmpAnyCase(option, "Indexes") == 0) {
                dir->enabled = 1;
            }
            option = mprStrTok(nextTok, " \t", &nextTok);
        }
        return 1;
    }
    return 0;
}
#endif


static void parseWords(MprList *list, cchar *str)
{
    char    *word, *tok, *strTok;

    mprAssert(str);
    if (str == 0 || *str == '\0') {
        return;
    }

    strTok = mprStrdup(list, str);
    word = mprStrTok(strTok, " \t\r\n", &tok);
    while (word) {
        mprAddItem(list, word);
        word = mprStrTok(0, " \t\r\n", &tok);
    }
}


/*
 *  Dynamic module initialization
 */
MprModule *maDirHandlerInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *handler;
    Dir         *dir;

    module = mprCreateModule(http, "dirHandler", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    handler = maCreateHandler(http, "dirHandler", MA_STAGE_GET | MA_STAGE_HEAD);
    if (handler == 0) {
        mprFree(module);
        return 0;
    }

    handler->match = matchDir; 
    handler->run = runDir; 
    handler->parse = parseDir; 

    handler->stageData = dir = mprAllocObjZeroed(handler, Dir);
    dir->sortOrder = 1;
    http->dirHandler = handler;

    return module;
}

#else
void __mprDirHandlerDummy() {}
#endif /* BLD_FEATURE_DIR */


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
/************************************************************************/
/*
 *  End of file "../http/handlers/dirHandler.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/handlers/egiHandler.c"
 */
/************************************************************************/

/*
 *  egiHandler.c -- Embedded Gateway Interface (EGI) handler. Fast in-process replacement for CGI.
 *
 *  The EGI handler implements a very fast in-process CGI scheme.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



#if BLD_FEATURE_EGI
#if BLD_DEBUG
/*
 *  Non-production in-line test code
 *  TODO - this should be moved to a loadable module that the test.conf loads
 */
#define EGI_TEST 1

static int  egiTestInit(MaHttp *http, cchar *path);
static int  getVars(MaQueue *q, char ***keys, char *buf, int len);
static void printRequestHeaders(MaQueue *q);
static void printQueryData(MaQueue *q);
static void printBodyData(MaQueue *q);
#endif

/*
 *  This runs when all input data has been received. The egi form must write all the data.
 *  It currently does not support forms that return before writing all the data.
 */
static void runEgi(MaQueue *q)
{
    MaConn          *conn;
    MaRequest       *req;
    MaEgiForm       *form;
    MaEgi           *egi;

    conn = q->conn;
    req = conn->request;
    egi = (MaEgi*) q->stage->stageData;
    
    maSetHeader(conn, 0, "Last-Modified", req->host->currentDate);
    maDontCacheResponse(conn);
    maPutForService(q, maCreateHeaderPacket(conn), 0);

    form = (MaEgiForm*) mprLookupHash(egi->forms, req->url);
    if (form == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Egi Form: \"%s\" is not defined", req->url);
        
    } else {
        (*form)(q);
    }
    maPutForService(q, maCreateEndPacket(conn), 1);
}


/*
 *  User API to define a form
 */
int maDefineEgiForm(MaHttp *http, cchar *name, MaEgiForm *form)
{
    MaEgi       *egi;
    MaStage     *handler;

    handler = http->egiHandler;
    if (handler) {
        egi = (MaEgi*) handler->stageData;
        mprAddHash(egi->forms, name, form);
    }
    return 0;
}


/*
 *  Dynamic module initialization
 */
MprModule *maEgiHandlerInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *handler;
    MaEgi       *egi;

    module = mprCreateModule(http, "egiHandler", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    handler = maCreateHandler(http, "egiHandler", 
        MA_STAGE_GET | MA_STAGE_HEAD | MA_STAGE_POST | MA_STAGE_PUT | MA_STAGE_FORM_VARS | MA_STAGE_ENV_VARS | \
        MA_STAGE_VIRTUAL);
    if (handler == 0) {
        mprFree(module);
        return 0;
    }
    http->egiHandler = handler;

    handler->run = runEgi; 

    handler->stageData = egi = mprAllocObjZeroed(handler, MaEgi);
    egi->forms = mprCreateHash(egi, MA_EGI_HASH_SIZE);

#if EGI_TEST
    egiTestInit(http, path);
#endif

    return module;
}


/*
 *  This code is never in release product.
 */

#if EGI_TEST
static void simpleTest(MaQueue *q)
{
    maWrite(q, "Hello %s\r\n", maGetFormVar(q->conn, "name", "unknown"));
}


static void bigTest(MaQueue *q)
{
    int     i;

    for (i = 0; i < 200; i++) {
        maWrite(q, "line %04d 012345678901234567890123456789012345678901234567890123456789\r\n", i);
    }
}


static void printVars(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;
    MaRequest   *req;
    char        *sw;
    char        *newLocation;
    int         responseStatus;

    conn = q->conn;
    resp = conn->response;
    req = conn->request;
    newLocation = 0;
    responseStatus = 0;
    sw = 0;

    /*
     *  Parse the switches
     */
    if (req->parsedUri->query) {
        sw = (char*) strstr(req->parsedUri->query, "SWITCHES=");
        if (sw) {
            sw = mprStrdup(resp, sw + 9);
            mprUrlDecode(sw, (int) strlen(sw) + 1, sw);
            if (*sw == '-') {
                if (sw[1] == 'l') {
                    newLocation = sw + 3;
                } else if (sw[1] == 's') {
                    responseStatus = atoi(sw + 3);
                }
            }
        }
    }

    maSetResponseCode(conn, 200);
    maSetResponseMimeType(conn, "text/html");
    maDontCacheResponse(conn);

    /*
     *  Test writing headers. The Server header overwrote the "Server" header
     *
     *  maSetHeader(conn, "MyCustomHeader", "true");
     *  maSetHeader(conn, "Server", "private");
     */

    if (maGetCookies(conn) == 0) {
        maSetCookie(conn, "appwebTest", "Testing can be fun", 43200, "/", 0);
    }

    if (newLocation) {
        maRedirect(conn, 302, newLocation);

    } else if (responseStatus) {
        maFailRequest(conn, responseStatus, "Custom Status");

    } else {
        maWrite(q, "<HTML><TITLE>egiProgram: EGI Output</TITLE><BODY>\r\n");

        printRequestHeaders(q);
        printQueryData(q);
        printBodyData(q);

        maWrite(q, "</BODY></HTML>\r\n");
    }
    if (sw) {
        mprFree(sw);
    }
}


static void printRequestHeaders(MaQueue *q)
{
    MprHashTable    *env;
    MprHash         *hp;

    maWrite(q, "<H2>Request Headers</H2>\r\n");

    env = q->conn->request->headers;

    for (hp = 0; (hp = mprGetNextHash(env, hp)) != 0; ) {
        maWrite(q, "<P>%s=%s</P>\r\n", hp->key, hp->data ? hp->data: "");
    }
    maWrite(q, "\r\n");
}


static void printQueryData(MaQueue *q)
{
    MaRequest   *req;
    char        buf[MPR_MAX_STRING], **keys, *value;
    int         i, numKeys;

    req = q->conn->request;
    if (req->parsedUri->query == 0) {
        return;
    }
    mprStrcpy(buf, sizeof(buf), req->parsedUri->query);
    numKeys = getVars(q, &keys, buf, (int) strlen(buf));

    if (numKeys == 0) {
        maWrite(q, "<H2>No Query Data Found</H2>\r\n");
    } else {
        maWrite(q, "<H2>Decoded Query Data Variables</H2>\r\n");
        for (i = 0; i < (numKeys * 2); i += 2) {
            value = keys[i+1];
            maWrite(q, "<p>QVAR %s=%s</p>\r\n", keys[i], value ? value: "");
        }
    }
    maWrite(q, "\r\n");
    mprFree(keys);
}


static void printBodyData(MaQueue *q)
{
    MprBuf  *buf;
    char    **keys, *value;
    int     i, numKeys;
    
    if (q->pair == 0 || q->pair->first == 0) {
        return;
    }
    
    buf = q->pair->first->content;
    mprAddNullToBuf(buf);
    
    numKeys = getVars(q, &keys, mprGetBufStart(buf), mprGetBufLength(buf));

    if (numKeys == 0) {
        maWrite(q, "<H2>No Body Data Found</H2>\r\n");
    } else {
        maWrite(q, "<H2>Decoded Body Data</H2>\r\n");
        for (i = 0; i < (numKeys * 2); i += 2) {
            value = keys[i+1];
            maWrite(q, "<p>PVAR %s=%s</p>\r\n", keys[i], value ? value: "");
        }
    }
    maWrite(q, "\r\n");
    mprFree(keys);
    mprFree(buf);
}


static int getVars(MaQueue *q, char ***keys, char *buf, int len)
{
    char**  keyList;
    char    *eq, *cp, *pp, *tok;
    int     i, keyCount;

    *keys = 0;

    /*
     *  Change all plus signs back to spaces
     */
    keyCount = (len > 0) ? 1 : 0;
    for (cp = buf; cp < &buf[len]; cp++) {
        if (*cp == '+') {
            *cp = ' ';
        } else if (*cp == '&' && (cp > buf && cp < &buf[len - 1])) {
            keyCount++;
        }
    }

    if (keyCount == 0) {
        return 0;
    }

    /*
     *  Crack the input into name/value pairs 
     */
    keyList = (char**) mprAlloc(q, (keyCount * 2) * sizeof(char**));

    i = 0;
    tok = 0;
    for (pp = mprStrTok(buf, "&", &tok); pp; pp = mprStrTok(0, "&", &tok)) {
        if ((eq = strchr(pp, '=')) != 0) {
            *eq++ = '\0';
            mprUrlDecode(pp, (int) strlen(pp) + 1, pp);
            mprUrlDecode(eq, (int) strlen(eq) + 1, eq);
        } else {
            mprUrlDecode(pp, (int) strlen(pp) + 1, pp);
        }
        if (i < (keyCount * 2)) {
            keyList[i++] = pp;
            keyList[i++] = eq;
        }
    }
    *keys = keyList;
    return keyCount;
}


static void upload(MaQueue *q)
{
    MaConn      *conn;
    char        *sw;
    char        *newLocation;
    int         responseStatus;

    conn = q->conn;
    newLocation = 0;
    responseStatus = 0;

    sw = (char*) strstr(maGetFormVar(conn, "QUERY_STRING", ""), "SWITCHES=");
    if (sw) {
        sw = mprStrdup(q, sw + 9);
        mprUrlDecode(sw, (int) strlen(sw) + 1, sw);
        if (*sw == '-') {
            if (sw[1] == 'l') {
                newLocation = sw + 3;
            } else if (sw[1] == 's') {
                responseStatus = atoi(sw + 3);
            }
        }
    }

    maSetResponseCode(conn, 200);
    maSetResponseMimeType(conn, "text/html");
    maDontCacheResponse(conn);

    /*
     *  Test writing headers. The Server header overwrote the "Server" header
     *
     *  maSetHeader(conn, "MyCustomHeader: true");
     *  maSetHeader(conn, "Server: private");
     */

    if (maGetCookies(conn) == 0) {
        maSetCookie(conn, "appwebTest", "Testing can be fun", 43200, "/", 0);
    }

    if (newLocation) {
        maRedirect(conn, 302, newLocation);

    } else if (responseStatus) {
        maFailRequest(conn, responseStatus, "Custom Status");

    } else {
        maWrite(q, "<HTML><TITLE>egiProgram: EGI Output</TITLE><BODY>\r\n");

        printRequestHeaders(q);
        printQueryData(q);
        printBodyData(q);

        maWrite(q, "</BODY></HTML>\r\n");
    }
    if (sw) {
        mprFree(sw);
    }
}


static int egiTestInit(MaHttp *http, cchar *path)
{
    /*
     *  Five instances of the same program. Location blocks must be defined in appweb.conf to test these.
     */
    maDefineEgiForm(http, "/egi/egiProgram", printVars);
    maDefineEgiForm(http, "/egi/egiProgram.egi", printVars);
    maDefineEgiForm(http, "/egi/egi Program.egi", printVars);
    maDefineEgiForm(http, "/egiProgram.egi", printVars);
    maDefineEgiForm(http, "/MyInProcScripts/egiProgram.egi", printVars);
    maDefineEgiForm(http, "/myEgi/egiProgram.egi", printVars);
    maDefineEgiForm(http, "/myEgi/egiProgram", printVars);
    maDefineEgiForm(http, "/upload/upload.egi", upload);
    maDefineEgiForm(http, "/egi/test", simpleTest);
    maDefineEgiForm(http, "/test.egi", simpleTest);
    maDefineEgiForm(http, "/big.egi", bigTest);

    return 0;
}
#endif /* EGI_TEST */


#else
void mprEgiHandlerDummy() {}
#endif /* BLD_FEATURE_EGI */


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
/************************************************************************/
/*
 *  End of file "../http/handlers/egiHandler.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/handlers/fileHandler.c"
 */
/************************************************************************/

/*
 *  fileHandler.c -- Static file content handler
 *
 *  This handler manages static file based content such as HTML, GIF /or JPEG pages. It supports all methods including:
 *  GET, PUT, DELETE, OPTIONS and TRACE.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



#if BLD_FEATURE_FILE


static void handleDeleteRequest(MaQueue *q);
static int  readFileData(MaQueue *q, MaPacket *packet);
static void handlePutRequest(MaQueue *q);

/*
 *  Initialize a handler instance for the file handler.
 */
static void openFile(MaQueue *q)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaConn          *conn;
    char            *date;

    conn = q->conn;
    resp = conn->response;
    req = conn->request;

    switch (req->method) {
    case MA_REQ_GET:
    case MA_REQ_HEAD:
        //  TODO - OPT could cache this
        date = maGetDateString(conn->arena, &resp->fileInfo);
        maSetHeader(conn, 0, "Last-Modified", date);
        mprFree(date);

        if (maContentNotModified(conn)) {
            maSetResponseCode(conn, MPR_HTTP_CODE_NOT_MODIFIED);
            maOmitResponseBody(conn);
        } else {
            maSetEntityLength(conn, (int) resp->fileInfo.size);
        }
        
        if (!resp->fileInfo.isReg) {
            maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Can't locate document: %s", req->url);
            
        } else if (!(resp->connector == conn->http->sendConnector)) {
            /*
             *  Open the file if a body must be sent with the response. The file will be automatically closed when 
             *  the response is freed. Cool eh?
             */
            resp->file = mprOpen(resp, resp->filename, O_RDONLY | O_BINARY, 0);
            if (resp->file == 0) {
                maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Can't open document: %s", resp->filename);
            }
        }
        break;
                
    case MA_REQ_PUT:
        maOmitResponseBody(conn);
        handlePutRequest(q);
        break;
        
    case MA_REQ_DELETE:
        maOmitResponseBody(conn);
        handleDeleteRequest(q);
        break;
            
    default:
        maFailRequest(q->conn, MPR_HTTP_CODE_BAD_METHOD, "Method not supported by file handler: %s", req->methodName);
        break;
    }
}


static void runFile(MaQueue *q)
{
    MaConn          *conn;
    MaRequest       *req;
    MaResponse      *resp;
    MaPacket        *packet;

    conn = q->conn;
    req = conn->request;
    resp = conn->response;
    
    maPutForService(q, maCreateHeaderPacket(conn), 0);
   
    if (!(resp->flags & MA_RESP_NO_BODY) || req->method & MA_REQ_HEAD) {
        /*
         *  Create a single data packet based on the entity length.
         */
        packet = maCreateDataPacket(conn, 0);
        packet->count = resp->entityLength;
        if (!req->ranges) {
            resp->length = resp->entityLength;
        }
        maPutForService(q, packet, 0);
    }

    /*
     *  Append end-of-data packet. Signifies end of stream.
     */
    maPutForService(q, maCreateEndPacket(q->conn), 1);
}


/*
 *  The service routine will be called when all incoming data has been received. This routine may flow control if the 
 *  downstream stage cannot accept all the file data. It will then be re-called as required to send more data.
 */
static void outgoingFileService(MaQueue *q)
{
    MaConn      *conn;
    MaRequest   *req;
    MaResponse  *resp;
    MaPacket    *packet;
    bool        usingSend;
    int         len;

    conn = q->conn;
    req = conn->request;
    resp = conn->response;

    usingSend = resp->connector == conn->http->sendConnector;

#if BLD_FEATURE_RANGE
    if (req->ranges) {
        maRangeService(q, (usingSend) ? NULL : readFileData);
    
    } else {
#endif
        for (packet = maGet(q); packet; packet = maGet(q)) {
            if (!usingSend && packet->flags & MA_PACKET_DATA) {
                if (!maWillNextQueueAccept(q, packet)) {
                    maPutBack(q, packet);
                    return;
                }
                if ((len = readFileData(q, packet)) < 0) {
                    return;
                }
                resp->pos += len;
            }
            maPutNext(q, packet);
        }
#if BLD_FEATURE_RANGE
    }
#endif
}


static void incomingFileData(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;
    MaResponse  *resp;
    MaRequest   *req;
    MaRange     *range;
    MprBuf      *buf;
    MprFile     *file;
    int         len;

    conn = q->conn;
    resp = conn->response;
    req = conn->request;

    file = (MprFile*) q->queueData;
  
    if (file == 0) {
        /* 
         *  Not a PUT so just ignore the incoming data.
         */
        mprFree(packet);
        return;
    }

    if (packet->count == 0) {
        /*
         *  End of input
         *  TODO - BUG does this free the file before it is served? Okay for PUT. Okay for PUT. Okay for PUT. Okay for PUT.
         */
        mprFree(file);
        q->queueData = 0;
        return;
    }

    buf = packet->content;
    len = mprGetBufLength(buf);
    mprAssert(len > 0);

    range = req->inputRange;
    if (range && mprSeek(file, SEEK_SET, range->start) != range->start) {
        maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't seek to range start to %d", range->start);

    } else {
        if (mprWrite(file, mprGetBufStart(buf), len) != len) {
            maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't PUT to %s", resp->filename);

        } else if (req->remainingContent > 0) {
            return;
        }
#if UNUSED && KEEP
        status = mprGetHttpCodeString(conn, resp->code);
        maFormatBody(conn, status, "<h2>%s</h2>", status);
#endif
    }
}


/*
 *  Populate a packet with file data
 */
static int readFileData(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;
    MaResponse  *resp;
    MaRequest   *req;
    int         len, rc;

    conn = q->conn;
    resp = conn->response;
    req = conn->request;
    len = packet->count;
    mprAssert(len > 0);

    if (packet->content == 0) {
        packet->content = mprCreateBuf(packet, len, len);
        if (packet->content == 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }
    mprAssert(len <= mprGetBufSpace(packet->content));    
    
    if (req->ranges) {
        /*
         *  maRangeService will have set resp->pos to the next read position already
         */
        mprSeek(resp->file, SEEK_SET, resp->pos);
    }

    if ((rc = mprRead(resp->file, mprGetBufStart(packet->content), len)) != len) {
        /*
         *  As we may have sent some data already to the client, the only thing we can do is abort and hope the client 
         *  notices the short data.
         */
        maFailRequest(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "Can't read file %s", resp->filename);
        return MPR_ERR_CANT_READ;
    }

    mprAdjustBufEnd(packet->content, len);

    return len;
}


/*
 *  This is called to setup for a HTTP PUT request. It is called before receiving the post data via incomingFileData
 */
static void handlePutRequest(MaQueue *q)
{
    MaConn          *conn;
    MaRequest       *req;
    MaResponse      *resp;
    MprFile         *file;
    char            *path;

    mprAssert(q->pair->queueData == 0);

    conn = q->conn;
    req = conn->request;
    resp = conn->response;

    path = maMapUriToStorage(q->conn, req->url);
    if (path == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Can't map URI to file storage");
        return;
    }

    if (req->ranges) {
        /*
         *  Open an existing file with fall-back to create
         */
        file = mprOpen(q, path, O_BINARY | O_WRONLY, 0644);
        if (file == 0) {
            file = mprOpen(q, path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, 0644);
            if (file == 0) {
                maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't create the put URI");
                return;
            }
        } else {
            mprSeek(file, SEEK_SET, 0);
        }

    } else {
        file = mprOpen(q, path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, 0644);
        if (file == 0) {
            maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't create the put URI");
            return;
        }
    }

    maSetResponseCode(conn, resp->fileInfo.isReg ? MPR_HTTP_CODE_NO_CONTENT : MPR_HTTP_CODE_CREATED);
    q->pair->queueData = (void*) file;
}


/*
 *  Immediate processing of delete requests
 */
static void handleDeleteRequest(MaQueue *q)
{
    MaConn          *conn;
    MaRequest       *req;
    char            *path;

    conn = q->conn;
    req = conn->request;

    path = maMapUriToStorage(q->conn, req->url);
    if (path == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Can't map URI to file storage");
        return;
    }

    if (!conn->response->fileInfo.isReg) {
        maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "URI not found");
        return;
    }

    if (mprDelete(q, path) < 0) {
        maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Can't remove URI");
        return;
    }
    maSetResponseCode(conn, MPR_HTTP_CODE_NO_CONTENT);
}


/*
 *  Dynamic module initialization
 */
MprModule *maFileHandlerInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *handler;

    module = mprCreateModule(http, "fileHandler", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    handler = maCreateHandler(http, "fileHandler", MA_STAGE_GET | MA_STAGE_HEAD | MA_STAGE_PUT | MA_STAGE_DELETE);
    if (handler == 0) {
        mprFree(module);
        return 0;
    }

    handler->open = openFile;
    handler->run = runFile;
    handler->outgoingService = outgoingFileService;
    handler->incomingData = incomingFileData;
    http->fileHandler = handler;

    return module;
}


#else
void __mprFileHandlerDummy() {}
#endif /* BLD_FEATURE_FILE */

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
/************************************************************************/
/*
 *  End of file "../http/handlers/fileHandler.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/handlers/passHandler.c"
 */
/************************************************************************/

/*
 *  passHandler.c -- Pass through handler
 *
 *  This handler simply relays all content onto a connector. It is used to convey errors.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */




static void runPass(MaQueue *q)
{
    maPutForService(q, maCreateHeaderPacket(q->conn), 0);
    maPutForService(q, maCreateEndPacket(q->conn), 1);
}


static void incomingPassData(MaQueue *q, MaPacket *packet)
{
    mprFree(packet);
}


int maOpenPassHandler(MaHttp *http)
{
    MaStage     *stage;

    stage = maCreateHandler(http, "passHandler", MA_STAGE_ALL | MA_STAGE_VIRTUAL);
    if (stage == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    stage->run = runPass;
    stage->incomingData = incomingPassData;
    http->passHandler = stage;

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
/************************************************************************/
/*
 *  End of file "../http/handlers/passHandler.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/handlers/phpHandler.c"
 */
/************************************************************************/

/*
 *  phpHandler.c - PHP handler
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_PHP

#if WIN
    /*
     *  Workaround for VS 2005 and PHP headers. Need to include before PHP headers include it.
     */
    #if _MSC_VER >= 1400
        #include    <sys/utime.h>
    #endif
    #undef  WIN32
    #define WIN32 1
    #define WINNT 1
    #define TIME_H
    #undef _WIN32_WINNT
    #undef chdir
    #undef popen
    #undef pclose
    #define PHP_WIN32 1
    #define ZEND_WIN32 1
    #define ZTS 1
#endif

    /*
     *  Indent headers to side-step make depend if PHP is not enabled
     */
    #include <main/php.h>
    #include <main/php_globals.h>
    #include <main/php_variables.h>
    #include <Zend/zend_modules.h>
    #include <main/SAPI.h>

#ifdef PHP_WIN32
    #include <win32/time.h>
    #include <win32/signal.h>
    #include <process.h>
#else
    #include <main/build-defs.h>
#endif

    #include <Zend/zend.h>
    #include <Zend/zend_extensions.h>
    #include <main/php_ini.h>
    #include <main/php_globals.h>
    #include <main/php_main.h>
#if ZTS
    #include <TSRM/TSRM.h>
#endif


typedef struct MaPhp {
    zval    *var_array;             /* Track var array */
} MaPhp;



static void flushOutput(void *context);
static void logMessage(char *message);
static char *readCookies(TSRMLS_D);
static int  readPostData(char *buffer, uint len TSRMLS_DC);
static void registerServerVars(zval *varArray TSRMLS_DC);
static int  runScript(MaConn *conn);
static int  startup(sapi_module_struct *sapiModule);
static int  sendHeaders(sapi_headers_struct *sapiHeaders TSRMLS_DC);
static int  writeBlock(cchar *str, uint len TSRMLS_DC);
static int  writeHeader(sapi_header_struct *sapiHeader, sapi_headers_struct *sapiHeaders TSRMLS_DC);

/*
 *  PHP Module Interface
 */
static sapi_module_struct phpSapiBlock = {
    BLD_PRODUCT,                    /* name */
    BLD_NAME,                       /* long name */
    startup,                        /* startup */
    php_module_shutdown_wrapper,    /* shutdown */
    0,                              /* activate */
    0,                              /* deactivate */
    writeBlock,                     /* unbuffered write */
    flushOutput,                    /* flush */
    0,                              /* get uid */
    0,                              /* getenv */
    php_error,                      /* error handler */
    writeHeader,                    /* header handler */
    sendHeaders,                    /* send headers */
    0,                              /* send header */
    readPostData,                   /* read POST data */
    readCookies,                    /* read Cookies */
    registerServerVars,             /* register server variables */
    logMessage,                     /* Log message */
    NULL,                           /* php_ini_path_override (set below) */
    0,                              /* Block interruptions */
    0,                              /* Unblock interruptions */
    STANDARD_SAPI_MODULE_PROPERTIES
};


static bool matchPhp(MaConn *conn, MaStage *handler, cchar *url)
{
    cchar       *ext;

    ext = conn->response->extension;

    /*
     *  Best to deny requests for non-php content inside location blocks where the php handler is enabled
     */
    if (ext == 0 || ext[0] == '\0' || strcmp(ext, "php") == 0) {
        return 1;
    }
    return 0;
}


/*
 *  Open the queue for a new request
 */
static void openPhp(MaQueue *q)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaConn          *conn;
    MaAlias         *alias;

    // TSRMLS_FETCH();

    conn = q->conn;
    resp = conn->response;
    req = conn->request;
    alias = req->alias;

    switch (req->method) {
    case MA_REQ_GET:
    case MA_REQ_HEAD:
    case MA_REQ_POST:
    case MA_REQ_PUT:
        q->queueData = mprAllocObjZeroed(resp, MaPhp);
        maDontCacheResponse(conn);
        maSetHeader(conn, 0, "Last-Modified", req->host->currentDate);
        break;
                
    case MA_REQ_DELETE:
    default:
        maFailRequest(q->conn, MPR_HTTP_CODE_BAD_METHOD, "Method not supported by file handler: %s", req->methodName);
        break;
    }
}


static void runPhp(MaQueue *q)
{
    MaConn          *conn;
    MaRequest       *req;
    MaResponse      *resp;
    MprHash         *hp;
    MaPhp           *php;

    TSRMLS_FETCH();

    conn = q->conn;
    req = conn->request;
    resp = conn->response;
    php = q->queueData;

    maPutForService(q, maCreateHeaderPacket(conn), 0);

    zend_first_try {
        php->var_array = 0;
        SG(server_context) = conn;
        SG(request_info).path_translated = resp->filename;
        SG(request_info).request_method = req->methodName;
        SG(request_info).request_uri = req->url;
        SG(request_info).query_string = req->parsedUri->query;
        SG(request_info).content_type = req->mimeType;
        SG(request_info).content_length = req->length;
        SG(sapi_headers).http_response_code = MPR_HTTP_CODE_OK;
        SG(request_info).auth_user = req->user;
        SG(request_info).auth_password = req->password;
        php_request_startup(TSRMLS_C);
        CG(zend_lineno) = 0;

    } zend_catch {
        mprError(q, "Can't start PHP request");
        zend_try {
            php_request_shutdown(0);
        } zend_end_try();
        maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "PHP initialization failed");
        return;
    } zend_end_try();

    /*
     *  Build environment variables.
     *  TODO - should this be inside a try/catch?
     */
    hp = mprGetFirstHash(req->headers);
    while (hp) {
        if (hp->data) {
            php_register_variable(hp->key, (char*) hp->data, php->var_array TSRMLS_CC);
        }
        hp = mprGetNextHash(req->headers, hp);
    }

    if (runScript(conn) < 0) {
        zend_try {
            php_request_shutdown(0);
        } zend_end_try();

        maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR,  "PHP script execution failed");
        return;
    }
    zend_try {
        php_request_shutdown(0);
        maPutForService(q, maCreateEndPacket(conn), 1);
    } zend_end_try();

#if UNUSED
    maSendPackets(q);
#endif
}


static int runScript(MaConn *conn)
{
    MaResponse          *resp;
    zend_file_handle    file_handle;

    TSRMLS_FETCH();

    resp = conn->response;

    file_handle.filename = resp->filename;
    file_handle.free_filename = 0;
    file_handle.type = ZEND_HANDLE_FILENAME;
    file_handle.opened_path = 0;

    zend_try {
        php_execute_script(&file_handle TSRMLS_CC);
        if (!SG(headers_sent)) {
            sapi_send_headers(TSRMLS_C);
        }
    } zend_catch {
        mprLog(conn, 1, "PHP exec failed");
        return -1;

    } zend_end_try();

    return 0;
}

 /*************************** PHP Support Functions ***************************/
/*
 *
 *  Flush write data back to the client
 */
static void flushOutput(void *server_context)
{
    MaConn      *conn;

    conn = (MaConn*) server_context;
    if (conn) {
        maServiceQueues(conn);
    }
}


static int writeBlock(cchar *str, uint len TSRMLS_DC)
{
    MaConn      *conn;
    int         written;

    conn = (MaConn*) SG(server_context);
    if (conn == 0) {
        return -1;
    }

    written = maWriteBlock(conn->response->queue[MA_QUEUE_SEND].nextQ, str, len, 1);
    if (written <= 0) {
        php_handle_aborted_connection();
    }
    return written;
}


static void registerServerVars(zval *track_vars_array TSRMLS_DC)
{
    MaConn      *conn;
    MaPhp       *php;

    conn = (MaConn*) SG(server_context);
    if (conn == 0) {
        return;
    }
    php_import_environment_variables(track_vars_array TSRMLS_CC);

    if (SG(request_info).request_uri) {
        php_register_variable("PHP_SELF", SG(request_info).request_uri,  track_vars_array TSRMLS_CC);
    }
    php = maGetHandlerQueueData(conn);
    mprAssert(php);
    php->var_array = track_vars_array;
}


static void logMessage(char *message)
{
    mprLog(mprGetMpr(0), 0, "phpModule: %s", message);
}


static char *readCookies(TSRMLS_D)
{
    MaConn      *conn;

    conn = (MaConn*) SG(server_context);
    return conn->request->cookie;
}


static int sendHeaders(sapi_headers_struct *phpHeaders TSRMLS_DC)
{
    MaConn      *conn;

    conn = (MaConn*) SG(server_context);
    maSetResponseCode(conn, phpHeaders->http_response_code);

#if UNUSED
    if (SG(sapi_headers).send_default_content_type) {
        maSetResponseMimeType(conn, "text/html");
    }
#else
    maSetResponseMimeType(conn, phpHeaders->mimetype);
#endif
    return SAPI_HEADER_SENT_SUCCESSFULLY;
}


static int writeHeader(sapi_header_struct *sapi_header, sapi_headers_struct *sapi_headers TSRMLS_DC)
{
    MaConn      *conn;
    MaResponse  *resp;
    char        *key, *value;

    conn = (MaConn*) SG(server_context);
    resp = conn->response;

    key = mprStrdup(resp, sapi_header->header);
    if ((value = strchr(key, ':')) == 0) {
        return -1;
    }
    *value++ = '\0';
    while (!isalnum(*value) && *value) {
        value++;
    }
    maSetHeader(conn, !sapi_header->replace, key, value);
    return SAPI_HEADER_ADD;
}


static int readPostData(char *buffer, uint bufsize TSRMLS_DC)
{
    MaConn      *conn;
    MaQueue     *q;
    MprBuf      *content;
    int         len;

    conn = (MaConn*) SG(server_context);
    q = conn->response->queue[MA_QUEUE_RECEIVE].prevQ;

    if (q->first == 0) {
        return 0;
    }

    content = q->first->content;
    len = min(mprGetBufLength(content), MPR_BUFSIZE);
    if (len > 0) {
        mprMemcpy(buffer, len, mprGetBufStart(content), len);
        mprAdjustBufStart(content, len);
    }
    return len;
}


static int startup(sapi_module_struct *sapi_module)
{
    return php_module_startup(sapi_module, 0, 0);
}


/*
 *  Initialze the php module
 */
static int initializePhp(MprModule *mp)
{
    MaHttp                  *http;
#if ZTS
    void                    ***tsrm_ls;
    php_core_globals        *core_globals;
    sapi_globals_struct     *sapi_globals;
    zend_llist              global_vars;
    zend_compiler_globals   *compiler_globals;
    zend_executor_globals   *executor_globals;

    tsrm_startup(128, 1, 0, 0);
    compiler_globals = (zend_compiler_globals*)  ts_resource(compiler_globals_id);
    executor_globals = (zend_executor_globals*)  ts_resource(executor_globals_id);
    core_globals = (php_core_globals*) ts_resource(core_globals_id);
    sapi_globals = (sapi_globals_struct*) ts_resource(sapi_globals_id);
    tsrm_ls = (void***) ts_resource(0);
#endif

    /*
     *  Define the php.ini location to be the ServerRoot
     */
    http = mp->moduleData;
    phpSapiBlock.php_ini_path_override = http->defaultServer->serverRoot;

    sapi_startup(&phpSapiBlock);

    if (php_module_startup(&phpSapiBlock, 0, 0) == FAILURE) {
        mprError(mp, "Can't startup PHP");
        return MPR_ERR_CANT_INITIALIZE;
    }

#if ZTS
    zend_llist_init(&global_vars, sizeof(char *), 0, 0);
#endif

    /*
     *  Set PHP defaults. As Appweb buffers output, we don't want PHP to call flush.
     */
    SG(options) |= SAPI_OPTION_NO_CHDIR;
    zend_alter_ini_entry("register_argc_argv", 19, "0", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);
    zend_alter_ini_entry("html_errors", 12, "0", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);
    zend_alter_ini_entry("implicit_flush", 15, "0", 1, PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);
    return 0;
}


static int finalizePhp(MprModule *mp)
{
    TSRMLS_FETCH();
    php_module_shutdown(TSRMLS_C);
    sapi_shutdown();
#if ZTS
    tsrm_shutdown();
#endif
    return 0;
}


/*
 *  Dynamic module initialization
 */
MprModule *maPhpHandlerInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *handler;

    module = mprCreateModule(http, "phpHandler", BLD_VERSION, http, NULL, finalizePhp);
    if (module == 0) {
        return 0;
    }

    //  TODO - what about PUT, DELETE
    handler = maCreateHandler(http, "phpHandler", 
        MA_STAGE_GET | MA_STAGE_HEAD | MA_STAGE_POST | MA_STAGE_ENV_VARS | MA_STAGE_PATH_INFO);
    if (handler == 0) {
        mprFree(module);
        return 0;
    }

    handler->match = matchPhp;
    handler->open = openPhp;
    handler->run = runPhp;

    initializePhp(module);

    return module;
}


#else
void __mprPhpHandlerDummy() {}

#endif /* BLD_FEATURE_PHP */

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
/************************************************************************/
/*
 *  End of file "../http/handlers/phpHandler.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/handlers/uploadHandler.c"
 */
/************************************************************************/

/*
 *  uploadHandler.c - Form-based file upload handler. 
 *
 *  The upload handler processes post data according to RFC-1867 ("multipart/form-data" post data). 
 *  It saves the uploaded files in a configured upload directory and creates files[] variables to 
 *  describe the uploaded files.  
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */


//  TODO - temp
#include    "ejs.h"

#if BLD_FEATURE_UPLOAD

#define UPLOAD_BUF_SIZE         4096        /* Post data buffer size */

/*
 *  Configuration for the upload handler
 */
typedef struct UploadHandler {
    MaUploadCallback callback;          /* User fn to process upload data */
    void            *callbackData;      /* User fn callback data */
    MprList         *handlerHeaders;    /* List of handler headers */
    MaLocation      *location;          /* Upload URL location prefix */
    char            *uploadDir;         /* Default upload directory */
#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;
#endif
} UploadHandler;

#define MA_UPLOAD_REQUEST_HEADER        1   /* Request header */
#define MA_UPLOAD_BOUNDARY              2   /* Boundary divider */
#define MA_UPLOAD_CONTENT_HEADER        3   /* Content part header */
#define MA_UPLOAD_CONTENT_DATA          4   /* Content encoded data */
#define MA_UPLOAD_CONTENT_END           5   /* End of multipart message */

/*
 *  State for an upload request
 */
typedef struct Upload {
    char            *boundary;          /* Boundary signature */
    int             boundaryLen;        /* Length of boundary */
    MaUploadCallback callback;          /* User fn to process upload data */
    void            *callbackData;      /* User fn callback data */
    int             contentState;       /* Input states */
    MaUploadFile    *file;              /* Current file */
    MprHashTable    *files;             /* List of uploaded files */
    char            *fileName;          /* Current file filename */
    char            *filePath;          /* Current file incoming filename */
    //  TODO - remove this fileSize as we can use file->size
    int             fileSize;           /* Current file size */
    MaLocation      *location;          /* Upload URL location prefix */
    char            *nameField;         /* Current name keyword value */
    MprFile         *upfile;            /* Incoming file object */
    char            *uploadDir;         /* Upload dir */
} Upload;


static char *findBoundary(void *buf, int bufLen, void *boundary, int boundaryLen);
static int  processContentBoundary(MaQueue *q, char *line);
static int  processContentHeader(MaQueue *q, char *line);
static int  processContentData(MaQueue *q);


static bool uploadMatch(MaConn *conn, MaStage *handler, cchar *uri)
{
    char    *pat;
    int     len;
    
    pat = "multipart/form-data";
    len = (int) strlen(pat);
    return mprStrcmpAnyCaseCount(conn->request->mimeType, pat, len) == 0;
}


static void uploadOpen(MaQueue *q)
{
    MaConn          *conn;
    MaResponse      *resp;
    Upload          *up;
    UploadHandler   *uph;

    conn = q->conn;
    resp = conn->response;

    uph = resp->handler->stageData;
    mprAssert(uph);

    up = mprAllocObjZeroed(resp, Upload);
    if (up == 0) {
        return;
    }
    q->queueData = up;

    up->contentState = MA_UPLOAD_REQUEST_HEADER;
    up->files = mprCreateHash(up, -1);
    
    //  TDOO - why replicate this
    up->uploadDir = mprStrdup(up, uph->uploadDir);
    up->callback = uph->callback;
    up->callbackData = uph->callbackData;
}


static void uploadClose(MaQueue *q)
{
    Upload      *up;

    up = q->queueData;
    if (up->filePath && *up->filePath) {
        mprDelete(q, up->filePath);
    }
}


static void uploadIncomingData(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;
    MaRequest   *req;
    MprBuf      *content;
    MprHash     *hp;
    Upload      *up;
    char        *line, *nextTok;
    
    mprAssert(packet);
    
    conn = q->conn;
    req = conn->request;
    up = q->queueData;
    
    if (packet->count == 0) {
        /* TODO END OF INPUT */
    }
    
    mprLog(conn, 5, "uploadIncomingData: %d bytes", packet->count);
    
    maJoinPacket(q->first, packet);

    content = q->first->content;

    line = 0;

    while (1) {
        /*
         *  Read the next line
         */
        switch (up->contentState) {
        case MA_UPLOAD_BOUNDARY:
        case MA_UPLOAD_CONTENT_HEADER:
            line = mprGetBufStart(content);
            mprStrTok(line, "\n", &nextTok);
            if (nextTok == 0) {
                return;                         /* Incomplete line */
            }
            mprAdjustBufStart(content, (int) (nextTok - line));
            mprStrTrim(line, "\r");
            break;

        case MA_UPLOAD_CONTENT_DATA:
        default:
            /* Data read below */
            ;
        }

        /*
         *  Do per state processing
         */
        switch (up->contentState) {
        case MA_UPLOAD_BOUNDARY:
            if (processContentBoundary(q, line) < 0) {
                return;
            }
            break;

        case MA_UPLOAD_CONTENT_HEADER:
            if (processContentHeader(q, line) < 0) {
                return;
            }
            break;

        case MA_UPLOAD_CONTENT_DATA:
            if (processContentData(q) < 0) {
                return;
            }
            if (mprGetBufLength(content) < up->boundaryLen) {
                /*  Incomplete boundary. Return and get more data */
                return;
            }
            break;

        case MA_UPLOAD_CONTENT_END:
            if (up->callback) {
                hp = mprGetFirstHash(up->files);
                while (hp) {
                    (*up->callback)(conn, up->callbackData, (MaUploadFile*) hp->data);
                    hp = mprGetNextHash(up->files, hp);
                }
            }
            return;
        }
    }

    if (req->remainingContent <= 0) {
        /*
         *  All done. TODO - see above.
         */
        up->contentState = MA_UPLOAD_CONTENT_END;
    }
}


/*
 *  Process the mime boundary division
 *
 *  Returns  < 0 on a request or state error
 *          == 0 if successful
 */
static int processContentBoundary(MaQueue *q, char *line)
{
    MaConn      *conn;
    Upload      *up;

    conn = q->conn;
    up = q->queueData;

    /*
     *  Expecting a multipart boundary string
     */
    if (strncmp(up->boundary, line, up->boundaryLen) != 0) {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad upload state. Incomplete boundary\n");
        return MPR_ERR_BAD_STATE;
    }
    if (line[up->boundaryLen] && strcmp(&line[up->boundaryLen], "--") == 0) {
        mprLog(conn, 7, "End of content.");
        up->contentState = MA_UPLOAD_CONTENT_END;

    } else {
        mprLog(conn, 7, "Starting new header.");
        up->contentState = MA_UPLOAD_CONTENT_HEADER;
    }
    return 0;
}


/*
 *  Expecting content headers. A blank line indicates the start of the data.
 *
 *  Returns  < 0  Request or state error
 *  Returns == 0  Successfully parsed the input line.
 */
static int processContentHeader(MaQueue *q, char *line)
{
    MaConn          *conn;
    MaUploadFile    *file;
    Upload          *up;
    char            tmpFile[MPR_MAX_FNAME];
    char            *key, *headerTok, *rest, *nextPair, *value;

    conn = q->conn;
    up = q->queueData;
    
    if (line[0] == '\0') {
        up->contentState = MA_UPLOAD_CONTENT_DATA;
        return 0;
    }
    mprLog(conn, 5, "Header line: %s", line);

    headerTok = line;
    mprStrTok(line, ": ", &rest);

    if (mprStrcmpAnyCase(headerTok, "Content-Disposition") == 0) {
        /*
         *  The content disposition header describes either a form
         *  variable or an uploaded file.
         *
         *      Content-Disposition: form-data; name="field1"
         *      >>blank line
         *      Field Data
         *      ---boundary
         *
         *      Content-Disposition: form-data; name="field1" ->
         *          filename="user.file"
         *      >>blank line
         *      File data
         *      ---boundary
         */
        key = rest;
        up->nameField = up->fileName = 0;
        while (key && mprStrTok(key, ";\r\n", &nextPair)) {

            key = mprStrTrim(key, " ");
            mprStrTok(key, "= ", &value);
            value = mprStrTrim(value, "\"");

            if (mprStrcmpAnyCase(key, "form-data") == 0) {
                /* Nothing to do */

            } else if (mprStrcmpAnyCase(key, "name") == 0) {
                mprFree(up->nameField);
                up->nameField = mprStrdup(up, value);

            } else if (mprStrcmpAnyCase(key, "filename") == 0) {
                if (up->nameField == 0) {
                    maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad upload state. Missing name field.");
                    return MPR_ERR_BAD_STATE;
                }
                mprFree(up->fileName);
                up->fileName = mprStrdup(up, value);

                /*
                 *  Create the file to hold the uploaded data
                 */
                mprMakeTempFileName(up, tmpFile, sizeof(tmpFile), up->uploadDir);
                up->filePath = mprStrdup(up, tmpFile);

                mprLog(conn, 5, "File upload of: %s stored as %s", up->fileName, up->filePath);

                up->upfile = mprOpen(up, up->filePath, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0600);
                if (up->upfile == 0) {
                    maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't open upload temp file %s", up->filePath);
                    return MPR_ERR_BAD_STATE;
                }

                /*
                 *  Create the files[entry]
                 */
                //  TODO - remove nameField, fileName, filePath, size
                file = up->file = mprAllocObjZeroed(up, MaUploadFile);
                file->name = mprStrdup(file, up->nameField);
                file->clientFilename = mprStrdup(file, up->fileName);
                file->filename = mprStrdup(file, up->filePath);
                mprAddHash(up->files, up->nameField, file);
            }
            key = nextPair;
        }

    } else if (mprStrcmpAnyCase(headerTok, "Content-Type") == 0) {
        if (up->fileName) {
            mprLog(conn, 5, "Set files[%s][CONTENT_TYPE] = %s", up->file->name, rest);
            up->file->contentType = mprStrdup(up->file, rest);
        }
    }

    return 1;
}


/*
 *  Process the content data.
 *
 *  Returns < 0 on error
 *          == 0 when more data is needed
 *          == 1 when data successfully written
 */
static int processContentData(MaQueue *q)
{
    MaLimits    *limits;
    MaConn      *conn;
    Upload      *up;
    MprBuf      *content;
    char        *fileData, *bp;
    int         size, fileDataLen, rc;

    conn = q->conn;
    up = q->queueData;
    content = q->first->content;
    limits = conn->host->limits;

    size = mprGetBufLength(content);
    if (size < up->boundaryLen) {
        /*  Incomplete boundary. Return and get more data */
        return 0;
    }

    bp = findBoundary(mprGetBufStart(content), size, up->boundary, up->boundaryLen);
    if (bp == 0) {

        if (up->fileName) {
            /*
             *  No signature found yet. probably more data to come. Since we did not match the boundary above we 
             *  know there is no boundary. But there could be most of the boundary with the boundary tail in the 
             *  next block of data. Must always preserve boundary-1 bytes.
             */
            fileData = mprGetBufStart(content);
            fileDataLen = ((int) (mprGetBufEnd(content) - fileData)) - (up->boundaryLen - 1);

            if ((up->fileSize + fileDataLen) > limits->maxUploadSize) {
                maFailRequest(conn, MPR_HTTP_CODE_REQUEST_TOO_LARGE, 
                    "Uploaded file %s exceeds maximum %d\n", up->filePath, limits->maxUploadSize);
                return MPR_ERR_CANT_WRITE;
            }
            if (fileDataLen > 0) {
                mprAdjustBufStart(content, fileDataLen);

                /*
                 *  File upload. Write the file data.
                 */
                rc = mprWrite(up->upfile, fileData, fileDataLen);
                if (rc != fileDataLen) {
                    maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, 
                        "Can't write to upload temp file %s, rc %d, errno %d\n", up->filePath, rc, mprGetOsError(up));
                    return MPR_ERR_CANT_WRITE;
                }
                up->fileSize += fileDataLen;
                up->file->size = up->fileSize;
            }
        }
        return 0;       /* Get more data */
    }

    mprLog(conn, 7, "Boundary found");
    fileData = mprGetBufStart(content);
    fileDataLen = (int) (bp - fileData);

    if (fileDataLen > 0) {

        mprAdjustBufStart(content, fileDataLen);

        /*
         *  This is the CRLF before the boundary
         */
        if (fileDataLen >= 2 && fileData[fileDataLen - 2] == '\r' && fileData[fileDataLen - 1] == '\n') {
            fileDataLen -= 2;
        }

        if (up->fileName) {
            /*
             *  File upload. Write the file data
             */
            rc = mprWrite(up->upfile, fileData, fileDataLen);
            if (rc != fileDataLen) {
                maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't write trailing data to upload temp file %s, "
                    "rc %d, errno %d\n", 
                    up->filePath, rc, mprGetOsError());
                return MPR_ERR_CANT_WRITE;
            }
            up->fileSize += fileDataLen;
            up->file->size = up->fileSize;

            /*
             *  Now have all the data (we've seen the boundary)
             */
            mprFree(up->upfile);
            up->upfile = 0;
            //  TODO - free?
            up->fileName = 0;

        } else {
            /*
             *  Normal string form data variables
             */
            fileData[fileDataLen] = '\0';
            mprLog(conn, 5, "Set form[%s] = %s", up->nameField, fileData);
            //  TODO - not right
            maSetFormVar(conn, up->nameField, fileData);
        }
    }
    up->contentState = MA_UPLOAD_BOUNDARY;
    return 1;
}


static void uploadOutgoingService(MaQueue *q)
{
    MaRequest   *req;
    MaConn      *conn;
    Upload      *up;
    char        *mimeType, *param;

    conn = q->conn;
    req = conn->request;
    up = q->queueData;


//  TODO BUG  - what should param be set to
param = "";
    /*
     *  The string to use as the multipart boundary divider is the next token
     */
    if (mprStrcmpAnyCaseCount(param, "boundary=", 9) == 0) {
        param += 9;
    }

    up->boundaryLen = mprAllocSprintf(up, &up->boundary, MPR_MAX_STRING, "--%s", param);

//  TODO BUG
//  mprFree(mimeType);
    mimeType = 0;

    if (up->boundaryLen == 0 || *up->boundary == '\0') {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad boundary");
        return;
    }

    maSetFormVar(conn, "UPLOAD_DIR", up->uploadDir);

    up->contentState = MA_UPLOAD_BOUNDARY;
}


/*
 *  Find the boundary signature in memory. Returns pointer to the first match.
 */ 
static char *findBoundary(void *buf, int bufLen, void *boundary, int boundaryLen)
{
    char    *cp, *endp;
    char    first;

    mprAssert(buf);
    mprAssert(boundary);
    mprAssert(boundaryLen > 0);

    first = *((char*) boundary);
    cp = (char*) buf;

    if (bufLen < boundaryLen) {
        return 0;
    }
    endp = cp + (bufLen - boundaryLen) + 1;
    while (cp < endp) {
        cp = (char *) memchr(cp, first, endp - cp);
        if (!cp) {
            return 0;
        }
        if (memcmp(cp, boundary, boundaryLen) == 0) {
            return cp;
        }
        cp++;
    }
    return 0;
}


extern void maSetUploadCallback(MaHttp *http, MaUploadCallback userCallback, void *data)
{
    MaStage             *handler;
    UploadHandler       *uph;
    
    handler = maLookupStage(http, "uploadHandler");
    if (handler) {
        uph = handler->stageData;
        uph->callback = userCallback;
        uph->callbackData = data;
    }
}


#if BLD_FEATURE_CONFIG_PARSE || 1
static int uploadParseConfig(MaHttp *http, cchar *key, char *value, MaConfigState *state)
{
    UploadHandler       *uph;
    MaHost              *host;
    char                pathBuf[MPR_MAX_FNAME], pathBuf2[MPR_MAX_FNAME];

    uph = maLookupStageData(http, "uploadHandler");

    if (mprStrcmpAnyCase(key, "FileUploadDir") == 0) {
        value = mprStrTrim(value, "\"");

        host = http->defaultServer->defaultHost;
        maReplaceReferences(http->defaultServer->defaultHost, pathBuf2, sizeof(pathBuf2), value);
        if (maMakePath(host, pathBuf, sizeof(pathBuf), pathBuf2) == 0) {
            mprError(http, "FormUploadDir path is too long");
            return -1;
        }
        mprAssert(pathBuf2[0]);

        mprFree(uph->uploadDir);
        uph->uploadDir = mprStrdup(uph, pathBuf);
        uph->location = state->location;

        mprLog(http, MPR_CONFIG, "Upload directory: %s", uph->uploadDir);
        return 1;
    }
    return 0;
}
#endif


/*
 *  Dynamic module initialization
 */
MprModule *maUploadHandlerInit(MaHttp *http, cchar *path)
{
    MprModule       *module;
    MaStage         *handler;
    UploadHandler   *uph;

    module = mprCreateModule(http, "uploadHandler", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    handler = maCreateHandler(http, "uploadHandler", MA_STAGE_POST | MA_STAGE_HEAD | MA_STAGE_FORM_VARS | MA_STAGE_VIRTUAL);
    if (handler == 0) {
        mprFree(module);
        return 0;
    }

    handler->match = uploadMatch; 
    handler->parse = uploadParseConfig; 
    handler->open = uploadOpen; 
    handler->close = uploadClose; 
    handler->incomingData = uploadIncomingData; 
    handler->outgoingService = uploadOutgoingService; 

    handler->stageData = uph = mprAllocObjZeroed(handler, UploadHandler);

#if WIN
{
    char *cp;
    uph->uploadDir = mprStrdup(uph, getenv("TEMP"));
    //  TODO - Need an MPR routine to do this
    for (cp = uph->uploadDir; *cp; cp++) {
        if (*cp == '\\') {
            *cp = '/';
        }
    }
}
#else
    uph->uploadDir = mprStrdup(uph, "/tmp");
#endif
    
#if BLD_FEATURE_MULTITHREAD && FUTURE && TODO
    upload->mutex = new MprMutex();
#endif

    return module;
}

#else
void __mprUploadHandlerDummy() { }

#endif /* BLD_FEATURE_UPLOAD */
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
/************************************************************************/
/*
 *  End of file "../http/handlers/uploadHandler.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/host.c"
 */
/************************************************************************/

/*
 *  host.c -- Host class for all HTTP hosts
 *
 *  The Host class is used for the default HTTP server and for all virtual hosts (including SSL hosts).
 *  Many objects are controlled at the host level. Eg. URL handlers.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */




static int  getRandomBytes(MaHost *host, char *buf, int bufsize);
static void lock(MaHost *host);
static void hostTimer(MaHost *host, MprEvent *event);
static void unlock(MaHost *host);
static void updateCurrentDate(MaHost *host);

/*
 *  Create a host from scratch
 */
MaHost *maCreateHost(MaServer *server, cchar *ipAddrPort, MaLocation *location)
{
    MaHost      *host;

    host = mprAllocObjZeroed(server, MaHost);
    if (host == 0) {
        return 0;
    }

    host->currentDate = maGetDateString(host, 0);
    host->aliases = mprCreateList(host);
    host->dirs = mprCreateList(host);
    host->connections = mprCreateList(host);
    host->locations = mprCreateList(host);

    if (ipAddrPort) {
        host->ipAddrPort = mprStrdup(server, ipAddrPort);
        host->name = mprStrdup(server, ipAddrPort);
    } else {
        host->ipAddrPort = 0;
        host->name = 0;
    }

    host->server = server;
    host->flags = MA_HOST_NO_TRACE;
    host->httpVersion = MPR_HTTP_1_1;
    host->timeout = MA_SERVER_TIMEOUT;
    host->limits = &server->http->limits;

    host->keepAliveTimeout = MA_KEEP_TIMEOUT;
    host->maxKeepAlive = MA_MAX_KEEP_ALIVE;
    host->keepAlive = 1;

    host->location = (location) ? location : maCreateBareLocation(host);
    maAddLocation(host, host->location);

#if BLD_FEATURE_AUTH
    host->location->auth = maCreateAuth(host->location, host->location->auth);
#endif

#if BLD_FEATURE_MULTITHREAD
    host->mutex = mprCreateLock(host);
#endif

    return host;
}


/*
 *  Create a new virtual host and inherit settings from another host
 */
MaHost *maCreateVirtualHost(MaServer *server, cchar *ipAddrPort, MaHost *parent)
{
    MaHost      *host;

    host = mprAllocObjZeroed(server, MaHost);
    if (host == 0) {
        return 0;
    }

    host->parent = parent;
    host->currentDate = maGetDateString(host, 0);
    host->connections = mprCreateList(host);

    if (ipAddrPort) {
        host->ipAddrPort = mprStrdup(server, ipAddrPort);
        host->name = mprStrdup(server, ipAddrPort);
    } else {
        host->ipAddrPort = 0;
        host->name = 0;
    }

    /*
     *  The aliases, dirs and locations are all copy-on-write
     */
    host->aliases = parent->aliases;
    host->dirs = parent->dirs;
    host->locations = parent->locations;
    host->server = parent->server;
    host->flags = parent->flags;
    host->httpVersion = parent->httpVersion;
    host->timeout = parent->timeout;
    host->limits = parent->limits;
    host->keepAliveTimeout = parent->keepAliveTimeout;
    host->maxKeepAlive = parent->maxKeepAlive;
    host->keepAlive = parent->keepAlive;
    host->accessLog = parent->accessLog;
    host->location = maCreateLocation(host, parent->location);

    maAddLocation(host, host->location);

#if BLD_FEATURE_MULTITHREAD
    host->mutex = mprCreateLock(host);
#endif

    return host;
}


int maStartHost(MaHost *host)
{
    char    ascii[MA_MAX_SECRET * 2 + 1];

    if (getRandomBytes(host, ascii, sizeof(ascii)) < 0) {
        //  Use the best we can get. getRandomBytes will report.
    }
    host->secret = mprStrdup(host, ascii);

#if BLD_FEATURE_ACCESS_LOG
    return maStartAccessLogging(host);
#else
    return 0;
#endif
}


int maStopHost(MaHost *host)
{
#if BLD_FEATURE_ACCESS_LOG
    return maStopAccessLogging(host);
#else
    return 0;
#endif
}


void maSetDocumentRoot(MaHost *host, cchar *dir)
{
    MaAlias     *alias;
    char        pathBuf[MPR_MAX_FNAME];
    int         len;

    maMakePath(host, pathBuf, sizeof(pathBuf) - 1, dir);

    host->documentRoot = mprStrdup(host, pathBuf);

    /*
     *  Create a catch-all alias. We want this directory to have a trailing "/" just like all aliases should.
     */
    len = (int) strlen(pathBuf);
    if (pathBuf[len - 1] != '/') {
        pathBuf[len] = '/';
        pathBuf[len + 1] = '\0';
    }
    alias = maCreateAlias(host, "", pathBuf, 0);
    //  TODO - fix
    maInsertAlias(host, alias);
}


/*
 *  Set the host name. Comes from the ServerName directive. Name should not contain "http://"
 */
void maSetHostName(MaHost *host, cchar *name)
{
    if (host->name) {
        mprFree(host->name);
    }
    host->name = mprStrdup(host, name);
}


void maSetHostIpAddrPort(MaHost *host, cchar *ipAddrPort)
{
    char    buf[MPR_MAX_IP_ADDR_PORT];

    mprAssert(ipAddrPort);

    if (*ipAddrPort == ':') {
        ipAddrPort++;
    }
    if (isdigit((int) *ipAddrPort) && strchr(ipAddrPort, '.') == 0) {
        buf[0] = '\0';
        mprStrcat(buf, sizeof(buf), ":", "127.0.0.1", ipAddrPort, 0);
        ipAddrPort = buf;
    }

    mprFree(host->ipAddrPort);
    host->ipAddrPort = mprStrdup(host, ipAddrPort);
}


void maSetHttpVersion(MaHost *host, int version)
{
    host->httpVersion = version;
}


void maSetKeepAlive(MaHost *host, bool on)
{
    host->keepAlive = on;
}


void maSetKeepAliveTimeout(MaHost *host, int timeout)
{
    host->keepAliveTimeout = timeout;
}


void maSetMaxKeepAlive(MaHost *host, int timeout)
{
    host->maxKeepAlive = timeout;
}


void maSetNamedVirtualHost(MaHost *host)
{
    host->flags |= MA_HOST_NAMED_VHOST;
}


void maSecureHost(MaHost *host, struct MprSsl *ssl)
{
    MaListen    *lp;
    cchar       *hostIp;
    char        *ipAddr;
    int         port, next;

    host->secure = 1;

    hostIp = host->ipAddrPort;
    if (mprStrcmpAnyCase(hostIp, "_default_") == 0) {
        hostIp = (char*) "*:*";
    }

    mprParseIp(host, hostIp, &ipAddr, &port, -1);
   
    for (next = 0; (lp = mprGetNextItem(host->server->listens, &next)) != 0; ) {
        if (port > 0 && port != lp->port) {
            continue;
        }
        if (*lp->ipAddr && ipAddr && ipAddr[0] != '*' && strcmp(ipAddr, lp->ipAddr) != 0) {
            continue;
        }
#if BLD_FEATURE_SSL
        if (host->secure) {
            if (host->flags & MA_HOST_NAMED_VHOST) {
                mprError(host, "SSL does not support named virtual hosts");
                return;
            }
            lp->ssl = ssl;
        }
#endif
    }
}


void maSetTraceMethod(MaHost *host, bool on)
{
    if (on) {
        host->flags &= ~MA_HOST_NO_TRACE;
    } else {
        host->flags |= MA_HOST_NO_TRACE;
    }
}


void maSetVirtualHost(MaHost *host)
{
    host->flags |= MA_HOST_VHOST;
}


/*
 *  Set the default request timeout. This is the maximum time a request can run.
 *  No to be confused with the session timeout or the keep alive timeout.
 */
void maSetTimeout(MaHost *host, int timeout)
{
    host->timeout = timeout;
}


int maInsertAlias(MaHost *host, MaAlias *newAlias)
{
    MaAlias     *alias, *old;
    int         rc, next, index;

    if (mprGetParent(host->aliases) == host->parent) {
        host->aliases = mprDupList(host, host->parent->aliases);
    }

    /*
     *  Sort in reverse collating sequence. Must make sure that /abc/def sorts before /abc. But we sort redirects with
     *  status codes first.
     */
    for (next = 0; (alias = mprGetNextItem(host->aliases, &next)) != 0; ) {
        rc = strcmp(newAlias->prefix, alias->prefix);
        if (rc == 0) {
            index = mprLookupItem(host->aliases, alias);
            old = (MaAlias*) mprGetItem(host->aliases, index);
            mprRemoveItem(host->aliases, alias);
            mprInsertItemAtPos(host->aliases, next - 1, newAlias);
            return 0;
            
        } else if (rc > 0) {
            if (newAlias->redirectCode >= alias->redirectCode) {
                mprInsertItemAtPos(host->aliases, next - 1, newAlias);
                return 0;
            }
        }
    }
    mprAddItem(host->aliases, newAlias);
    return 0;
}


int maInsertDir(MaHost *host, MaDir *newDir)
{
    MaDir       *dir;
    int         rc, next;

    mprAssert(newDir);
    mprAssert(newDir->path);
    
    if (mprGetParent(host->dirs) == host->parent) {
        host->dirs = mprDupList(host, host->parent->dirs);
    }

    /*
     *  Sort in reverse collating sequence. Must make sure that /abc/def sorts before /abc
     */
    for (next = 0; (dir = mprGetNextItem(host->dirs, &next)) != 0; ) {
        mprAssert(dir->path);
        rc = strcmp(newDir->path, dir->path);
        if (rc == 0) {
            mprRemoveItem(host->dirs, dir);
            mprInsertItemAtPos(host->dirs, next, newDir);
            return 0;

        } else if (rc > 0) {
            mprInsertItemAtPos(host->dirs, next - 1, newDir);
            return 0;
        }
    }

    mprAddItem(host->dirs, newDir);
    return 0;
}


int maAddLocation(MaHost *host, MaLocation *newLocation)
{
    MaLocation  *location;
    int         next, rc;

    mprAssert(newLocation);
    mprAssert(newLocation->prefix);
    
    if (mprGetParent(host->locations) == host->parent) {
        host->locations = mprDupList(host, host->parent->locations);
    }

    /*
     *  Sort in reverse collating sequence. Must make sure that /abc/def sorts before /abc
     */
    for (next = 0; (location = mprGetNextItem(host->locations, &next)) != 0; ) {
        rc = strcmp(newLocation->prefix, location->prefix);
        if (rc == 0) {
            mprRemoveItem(host->locations, location);
            mprInsertItemAtPos(host->locations, next - 1, newLocation);
            return 0;
        }
        if (strcmp(newLocation->prefix, location->prefix) > 0) {
            mprInsertItemAtPos(host->locations, next - 1, newLocation);
            return 0;
        }
    }
    mprAddItem(host->locations, newLocation);

    return 0;
}


MaAlias *maGetAlias(MaHost *host, cchar *uri)
{
    MaAlias     *alias;
    int         next;

    for (next = 0; (alias = mprGetNextItem(host->aliases, &next)) != 0; ) {
        if (strncmp(alias->prefix, uri, alias->prefixLen) == 0) {
            if (uri[alias->prefixLen] == '\0' || uri[alias->prefixLen] == '/') {
                return alias;
            }
        }
    }
    return 0;
}


MaAlias *maLookupAlias(MaHost *host, cchar *prefix)
{
    MaAlias     *alias;
    int         next;

    for (next = 0; (alias = mprGetNextItem(host->aliases, &next)) != 0; ) {
        if (strcmp(alias->prefix, prefix) == 0) {
            return alias;
        }
    }
    return 0;
}


/*
 *  Find an exact dir match. May be called with raw file names. ie. D:\myDir
 */
MaDir *maLookupDir(MaHost *host, cchar *path)
{
    MaDir       *dir;
    char        *absPath, buf[MPR_MAX_FNAME];
    int         next, len;

    /*
     *  Cannonicalize and ensure path has a trailing "/"
     */
    absPath = mprGetAbsFilename(host, path);
    mprStrcpy(buf, sizeof(buf), absPath);
    len = (int) strlen(buf);
    if (buf[len - 1] != '/') {
        buf[len] = '/';
        buf[++len] = '\0';
    }

    for (next = 0; (dir = mprGetNextItem(host->dirs, &next)) != 0; ) {
        if (dir->path != 0) {
#if WIN
            if (mprStrcmpAnyCase(dir->path, buf) == 0)
#else
            if (strcmp(dir->path, buf) == 0)
#endif
                return dir;
        }
    }
    return 0;
}


/*
 *  Find the directory entry that this file (path) resides in. path is a physical file path. We find the most specific
 *  (longest) directory that matches. The directory must match or be a parent of path. Not called with raw files names.
 *  They will be lower case and only have forward slashes. For windows, the will be in cannonical format with drive
 *  specifiers.
 */
MaDir *maLookupBestDir(MaHost *host, cchar *path)
{
    MaDir   *dir;
    int     next, len, dlen;

    len = (int) strlen(path);

    for (next = 0; (dir = mprGetNextItem(host->dirs, &next)) != 0; ) {
        dlen = dir->pathLen;
        /*
         *  Special case where the Directory has a trailing "/" and we match all the letters before that.
         */
        if (len == (dlen - 1) && dir->path[len] == '/') {
            dlen--;
        }
#if WIN
        //  TODO - must make this generic and use something like:  host->caseMatters
        //  TODO - but this must be per-file system.
        if (mprCompareFilename(host, dir->path, path, dlen) == 0)
#else
        if (strncmp(dir->path, path, dlen) == 0)
#endif
        {
            if (dlen >= 0) {
                return dir;
            }
        }
    }
    return 0;
}


MaLocation *maLookupLocation(MaHost *host, cchar *prefix)
{
    MaLocation      *location;
    int             next;

    for (next = 0; (location = mprGetNextItem(host->locations, &next)) != 0; ) {
        if (strcmp(prefix, location->prefix) == 0) {
            return location;
        }
    }
    return 0;
}


MaLocation *maLookupBestLocation(MaHost *host, cchar *uri)
{
    MaLocation  *location;
    int         next, rc;

    for (next = 0; (location = mprGetNextItem(host->locations, &next)) != 0; ) {
        rc = strncmp(location->prefix, uri, location->prefixLen);
        if (rc == 0) {
            return location;
        }
    }
    return 0;
}


static int getRandomBytes(MaHost *host, char *buf, int bufsize)
{
    MprTime     now;
    char        *hex = "0123456789abcdef";
    char        *ap, *cp;
    uchar       bytes[MA_MAX_SECRET], *bp;
    int         i, pid;

    mprLog(host, 7, "Get random bytes");

    memset(bytes, 0, sizeof(bytes));

    /*
     *  Create a random secret for use in authentication. Don't block. TODO -- conditional on Auth
     */
    if (mprGetRandomBytes(host, bytes, sizeof(bytes), 0) < 0) {

        mprError(host, "Can't get sufficient random data for secure SSL operation. If SSL is used, it will not be secure.");

        now = mprGetTime(host); 
        pid = (int) getpid();
        cp = (char*) &now;
        bp = bytes;
        for (i = 0; i < sizeof(now) && bp < &bytes[MA_MAX_SECRET]; i++) {
            *bp++= *cp++;
        }
        cp = (char*) &now;
        for (i = 0; i < sizeof(pid) && bp < &bytes[MA_MAX_SECRET]; i++) {
            *bp++ = *cp++;
        }
    }

    for (i = 0, ap = buf; ap < &buf[bufsize - 1] && i < sizeof(bytes); i++) {
        *ap++ = hex[(bytes[i] >> 4) & 0xf];
        *ap++ = hex[bytes[i] & 0xf];
    }
    *ap = '\0';

    mprLog(host, 7, "Got %d random bytes", (int) sizeof(bytes));

    return 0;
}


/*
 *  Start the host timer. This may create multiple timers -- no worry. maAddConn does its best to only schedule one.
 */
static void startTimer(MaHost *host)
{
    if (mprGetDebugMode(host)) {
        return;
    }
    mprCreateTimerEvent(host, (MprEventProc) hostTimer, MA_TIMER_PERIOD, MPR_NORMAL_PRIORITY, host, MPR_EVENT_CONTINUOUS);
}


/*
 *  The host timer does maintenance activities and will fire per second while there is active requests.
 *  When multi-threaded, the host timer runs as an event off the service thread. Because we lock the host here,
 *  connections cannot be deleted while we are modifying the list.
 */
static void hostTimer(MaHost *host, MprEvent *event)
{
    MaConn      *conn;
    int         next, connCount;

    lock(host);

    updateCurrentDate(host);

    /*
     *  Check for any expired connections
     */
    for (connCount = 0, next = 0; (conn = mprGetNextItem(host->connections, &next)) != 0; connCount++) {
        /*
         *  Workaround for a GCC bug when comparing two 64bit numerics directly. Need a temporary.
         */
        int64 diff = conn->expire - host->whenCurrentDate;
        if (diff < 0) {
            //  thread will have the connection lock and may be trying to get the host lock.
            // mprFree(conn);
            // mprCloseSocket(
            // if (conn->sock && conn->sock->handler) {
            //     mprRecallWaitHandler(conn->sock->handler);
            // }
            // closesocket(conn->sock->fd);
            conn->keepAliveCount = 0;
            mprSetSocketEventMask(conn->sock, MPR_WRITEABLE);
        }
    }

    if (event) {
        if (connCount == 0) {
            mprStopContinuousEvent(event);
        }
        mprFree(event);
    }
    unlock(host);
}


void maAddConn(MaHost *host, MaConn *conn)
{
    lock(host);
    mprAddItem(host->connections, conn);
    conn->started = mprGetTime(conn);

    if ((host->whenCurrentDate + MPR_TICKS_PER_SEC) < conn->started) {
        updateCurrentDate(host);
    }
    if (mprGetListCount(host->connections) == 1) {
        startTimer(host);
    }
    unlock(host);
}


static void updateCurrentDate(MaHost *host)
{
    char        *oldDate;

    oldDate = host->currentDate;
    host->whenCurrentDate = mprGetTime(host);
    host->currentDate = maGetDateString(host, 0);
    mprFree(oldDate);
}


/*
 *  See locking note for maAddConn
 */
void maRemoveConn(MaHost *host, MaConn *conn)
{
    lock(host);
    mprRemoveItem(host->connections, conn);
    unlock(host);
}


MaHostAddress *maCreateHostAddress(MprCtx ctx, cchar *ipAddr, int port)
{
    MaHostAddress   *hostAddress;

    mprAssert(ipAddr && ipAddr);
    mprAssert(port >= 0);

    hostAddress = mprAllocObjZeroed(ctx, MaHostAddress);
    if (hostAddress == 0) {
        return 0;
    }

    hostAddress->flags = 0;
    hostAddress->ipAddr = mprStrdup(hostAddress, ipAddr);
    hostAddress->port = port;
    hostAddress->vhosts = mprCreateList(hostAddress);

    return hostAddress;
}


void maInsertVirtualHost(MaHostAddress *hostAddress, MaHost *vhost)
{
    mprAddItem(hostAddress->vhosts, vhost);
}


bool maIsNamedVirtualHostAddress(MaHostAddress *hostAddress)
{
    return hostAddress->flags & MA_IPADDR_VHOST;
}


void maSetNamedVirtualHostAddress(MaHostAddress *hostAddress)
{
    hostAddress->flags |= MA_IPADDR_VHOST;
}


/*
 *  Look for a host with the right host name (ServerName)
 */
MaHost *maLookupVirtualHost(MaHostAddress *hostAddress, cchar *hostStr)
{
    MaHost      *host;
    int         next;

    for (next = 0; (host = mprGetNextItem(hostAddress->vhosts, &next)) != 0; ) {
        /*  TODO  -- need to support aliases */
        if (hostStr == 0 || strcmp(hostStr, host->name) == 0) {
            return host;
        }
    }
    return 0;
}


// TODO - opt. Should be macro.
static void lock(MaHost *host)
{
#if BLD_FEATURE_MULTITHREAD
    mprLock(host->mutex);
#endif
}


static void unlock(MaHost *host)
{
#if BLD_FEATURE_MULTITHREAD
    mprUnlock(host->mutex);
#endif
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
/************************************************************************/
/*
 *  End of file "../http/host.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/link.c"
 */
/************************************************************************/

/*
 *  link.c -- Link in static modules and initialize.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_STATIC || (!BLD_APPWEB_PRODUCT && BLD_APPWEB_BUILTIN)

static MprModule *staticModules[64];        /* List of static modules */
static int maxStaticModules;                /* Max static modules */


void maLoadStaticModules(MaHttp *http)
{
    int     index = 0;

    staticModules[index] = 0;

#if BLD_FEATURE_AUTH
    staticModules[index++] = maAuthFilterInit(http, NULL);
#endif
#if BLD_FEATURE_CGI
    staticModules[index++] = maCgiHandlerInit(http, NULL);
#endif
#if BLD_FEATURE_CHUNK
    staticModules[index++] = maChunkFilterInit(http, NULL);
#endif
#if BLD_FEATURE_DIR
    staticModules[index++] = maDirHandlerInit(http, NULL);
#endif
#if BLD_FEATURE_EJS
    staticModules[index++] = maEjsHandlerInit(http, NULL);
#endif
#if BLD_FEATURE_FILE
    staticModules[index++] = maFileHandlerInit(http, NULL);
#endif
#if BLD_FEATURE_EGI
    staticModules[index++] = maEgiHandlerInit(http, NULL);
#endif
#if BLD_FEATURE_SSL
    staticModules[index++] = maSslModuleInit(http, NULL);
#endif
#if BLD_FEATURE_PHP
    staticModules[index++] = maPhpHandlerInit(http, NULL);
#endif
#if BLD_FEATURE_RANGE
    staticModules[index++] = maRangeFilterInit(http, NULL);
#endif
#if BLD_FEATURE_UPLOAD
    staticModules[index++] = maUploadHandlerInit(http, NULL);
#endif
    maxStaticModules = index;
}


void maUnloadStaticModules(MaHttp *http)
{
    int     i;

    for (i = 0; i < maxStaticModules; i++) {
        mprUnloadModule(staticModules[i]);
    }
}

#else
void maLoadStaticModules(MaHttp *http) {}
void maUnloadStaticModules(MaHttp *http) {}

#endif /* BLD_FEATURE_STATIC */

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
/************************************************************************/
/*
 *  End of file "../http/link.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/listen.c"
 */
/************************************************************************/

/*
 *  listen.c -- Listen for client connections.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



/*
 *  Listen on an ipAddress:port. NOTE: ipAddr may be empty which means bind to all addresses.
 */
MaListen *maCreateListen(MaServer *server, cchar *ipAddr, int port, int flags)
{
    MaListen    *listen;

    mprAssert(ipAddr);
    mprAssert(port > 0);

    listen = mprAllocObjZeroed(server, MaListen);
    if (listen == 0) {
        return 0;
    }

    listen->server = server;
    listen->flags = flags;
    listen->port = port;
    listen->ipAddr = mprStrdup(listen, ipAddr);
    listen->flags = flags;

    return listen;
}


int maStartListening(MaListen *listen)
{
    cchar       *proto;
    char        *ipAddr;

#if BLD_FEATURE_SSL
    listen->sock = mprCreateSocket(listen, listen->ssl);
#else
    listen->sock = mprCreateSocket(listen, NULL);
#endif

    if (mprOpenServerSocket(listen->sock, listen->ipAddr, listen->port, (MprSocketAcceptProc) maAcceptConn, listen->server,
            MPR_SOCKET_NODELAY | MPR_SOCKET_THREAD) < 0) {
        mprError(listen, "Can't open a socket on %s, port %d", listen->ipAddr, listen->port);
        return MPR_ERR_CANT_OPEN;
    }

    proto = "HTTP";
#if BLD_FEATURE_SSL
    if (mprSocketIsSecure(listen->sock)) {
        proto = "HTTPS";
    }
#endif
    ipAddr = listen->ipAddr;
    if (ipAddr == 0 || *ipAddr == '\0') {
        ipAddr = "*";
    }
    mprLog(listen, MPR_CONFIG, "Listening for %s on %s:%d", proto, ipAddr, listen->port);

    return 0;
}


int maStopListening(MaListen *listen)
{
    if (listen->sock) {
        mprFree(listen->sock);
        listen->sock = 0;
    }
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
/************************************************************************/
/*
 *  End of file "../http/listen.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/location.c"
 */
/************************************************************************/

/*
 *  location.c -- Implement Location directives.
 *
 *  Location directives provide authorization and handler matching based on URL prefixes.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */




MaLocation *maCreateBareLocation(MprCtx ctx)
{
    MaLocation  *location;

    location = mprAllocObjZeroed(ctx, MaLocation);
    if (location == 0) {
        return 0;
    }

    location->errorDocuments = mprCreateHash(location, MA_ERROR_HASH_SIZE);
    location->handlers = mprCreateList(location);
    location->extensions = mprCreateHash(location, MA_HANDLER_HASH_SIZE);
    location->inputStages = mprCreateList(location);
    location->outputStages = mprCreateList(location);

    //  TODO - should be no need to dup
    location->prefix = mprStrdup(location, "");
    location->prefixLen = (int) strlen(location->prefix);

#if BLD_FEATURE_AUTH
    location->auth = maCreateAuth(location, 0);
#endif
    
    return location;
}


/*
 *  Create a new location block. Inherit from the parent. We use a copy-on-write scheme if these are modified later.
 */
MaLocation *maCreateLocation(MprCtx ctx, MaLocation *parent)
{
    MaLocation  *location;

    if (parent == 0) {
        return maCreateBareLocation(ctx);
    }

    location = mprAllocObjZeroed(ctx, MaLocation);
    if (location == 0) {
        return 0;
    }
    
    location->prefix = mprStrdup(location, parent->prefix);
    location->parent = parent;
    location->prefixLen = parent->prefixLen;
    location->flags = parent->flags;
    location->inputStages = parent->inputStages;
    location->outputStages = parent->outputStages;
    location->handlers = parent->handlers;
    location->extensions = parent->extensions;
    location->connector = parent->connector;
    location->errorDocuments = parent->errorDocuments;
    location->sessionTimeout = parent->sessionTimeout;

#if BLD_FEATURE_SSL
    location->ssl = parent->ssl;
#endif
#if BLD_FEATURE_AUTH
    location->auth = maCreateAuth(location, parent->auth);
#endif
    return location;
}


void maFinalizeLocation(MaLocation *location)
{
    MaFilter    *filter;

    /*
     *  Only update if this location owns (not-inheriting) the output stages pipeline
     */
    if (mprGetParent(location->outputStages) == location) {
        filter = mprAllocObjZeroed(location, MaFilter);
        filter->stage = location->connector;
        mprAddItem(location->outputStages, filter);
    }
#if BLD_FEATURE_SSL
    if (location->ssl) {
        mprConfigureSsl(location->ssl);
    }
#endif
}


void maSetLocationAuth(MaLocation *location, MaAuth *auth)
{
    location->auth = auth;
}


/*
 *  Add a handler. This adds a handler to the set of possible handlers for a set of file extensions.
 */
int maAddHandler(MaHttp *http, MaLocation *location, cchar *name, cchar *extensions)
{
    MaStage     *handler;
    char        *extlist, *word, *tok;

    mprAssert(location);
    
    if (mprGetParent(location->handlers) == location->parent) {
        location->extensions = mprCopyHash(location, location->parent->extensions);
        location->handlers = mprDupList(location, location->parent->handlers);
    }
    
    handler = maLookupStage(http, name);
    if (handler == 0) {
        mprError(http, "Can't find stage %s", name); 
        return MPR_ERR_NOT_FOUND;
    }

    if (extensions && *extensions) {
        /*
         *  Add to the handler extension hash
         */ 
        extlist = mprStrdup(location, extensions);
        word = mprStrTok(extlist, " \t\r\n", &tok);
        while (word) {
            if (*word == '*' && word[1] == '.') {
                word += 2;
            } else if (*word == '.') {
                word++;
            } else if (*word == '\"' && word[1] == '\"') {
                word = "";
            }
            mprAddHash(location->extensions, word, handler);
            word = mprStrTok(0, " \t\r\n", &tok);
        }
        mprFree(extlist);

    } else {
        if (handler->match == 0) {
            /*
             *  If a handler provides a custom match() routine, then don't match by extension.
             */
            mprAddHash(location->extensions, "", handler);
        }
        mprAddItem(location->handlers, handler);
    }

    if (extensions && *extensions) {
        mprLog(location, MPR_CONFIG, "Add handler \"%s\" for extensions \"%s\"", name, extensions);
    } else {
        mprLog(location, MPR_CONFIG, "Add handler \"%s\" to location \"%s\" for all extensions", name, location->prefix);
    }

    return 0;
}


/*
 *  Set a handler to universally apply to requests in this location block.
 */
int maSetHandler(MaHttp *http, MaHost *host, MaLocation *location, cchar *name)
{
    MaStage     *handler;

    mprAssert(location);
    
    if (mprGetParent(location->handlers) == location->parent) {
        location->extensions = mprCopyHash(location, location->parent->extensions);
        location->handlers = mprDupList(location, location->parent->handlers);
    }
    
    handler = maLookupStage(http, name);
    if (handler == 0) {
        mprError(http, "Can't find handler %s", name); 
        return MPR_ERR_NOT_FOUND;
    }
    location->handler = handler;

    mprLog(location, MPR_CONFIG, "Set handler \"%s\" in location \"%s\" for host \"%s\"", name, location->prefix,
        (host) ? host->name: "unknown");

    return 0;
}


/*
 *  Add a filter. Direction defines what direction the stage filter be defined.
 */
int maAddFilter(MaHttp *http, MaLocation *location, cchar *name, cchar *extensions, int direction)
{
    MaStage     *stage;
    MaFilter    *filter;
    char        *extlist, *word, *tok;

    mprAssert(location);
    
    stage = maLookupStage(http, name);
    if (stage == 0) {
        mprError(http, "Can't find filter %s", name); 
        return MPR_ERR_NOT_FOUND;
    }

    filter = mprAllocObjZeroed(location, MaFilter);
    filter->stage = stage;

    if (extensions && *extensions) {
        filter->extensions = mprCreateHash(filter, 0);
        extlist = mprStrdup(location, extensions);
        word = mprStrTok(extlist, " \t\r\n", &tok);
        while (word) {
            if (*word == '*' && word[1] == '.') {
                word += 2;
            } else if (*word == '.') {
                word++;
            } else if (*word == '\"' && word[1] == '\"') {
                word = "";
            }
            mprAddHash(filter->extensions, word, filter);
            word = mprStrTok(0, " \t\r\n", &tok);
        }
        mprFree(extlist);
    }

    if (direction & MA_FILTER_INCOMING) {
        if (mprGetParent(location->inputStages) == location->parent) {
            location->inputStages = mprDupList(location, location->parent->inputStages);
        }
        mprAddItem(location->inputStages, filter);
    }
    if (direction & MA_FILTER_OUTGOING) {
        if (mprGetParent(location->outputStages) == location->parent) {
            location->outputStages = mprDupList(location, location->parent->outputStages);
        }
        mprAddItem(location->outputStages, filter);
    }

    if (extensions && *extensions) {
        mprLog(location, MPR_CONFIG, "Add filter \"%s\" to location \"%s\" for extensions \"%s\"", name, 
            location->prefix, extensions);
    } else {
        mprLog(location, MPR_CONFIG, "Add filter \"%s\" to location \"%s\" for all extensions", name, location->prefix);
    }

    return 0;
}


/*
 *  Set the network connector.
 */
int maSetConnector(MaHttp *http, MaLocation *location, cchar *name)
{
    MaStage     *stage;

    mprAssert(location);
    
    stage = maLookupStage(http, name);
    if (stage == 0) {
        mprError(http, "Can't find connector %s", name); 
        return MPR_ERR_NOT_FOUND;
    }

    location->connector = stage;
    mprLog(location, MPR_CONFIG, "Set connector \"%s\"", name);

    return 0;
}


void maResetPipeline(MaLocation *location)
{
    if (mprGetParent(location->extensions) == location) {
        mprFree(location->extensions);
    }
    location->extensions = mprCreateHash(location, 0);
    
    if (mprGetParent(location->handlers) == location) {
        mprFree(location->handlers);
    }
    location->handlers = mprCreateList(location);
    
    if (mprGetParent(location->inputStages) == location) {
        mprFree(location->inputStages);
    }
    location->inputStages = mprCreateList(location);
    
    if (mprGetParent(location->outputStages) == location) {
        mprFree(location->outputStages);
    }
    location->outputStages = mprCreateList(location);
}


MaStage *maGetHandlerByExtension(MaLocation *location, cchar *ext)
{
    return (MaStage*) mprLookupHash(location->extensions, ext);
}


void maSetLocationPrefix(MaLocation *location, cchar *uri)
{
    mprAssert(location);

    mprFree(location->prefix);
    location->prefix = mprStrdup(location, uri);
    location->prefixLen = (int) strlen(location->prefix);

    /*
     *  Always strip trailing "/"
     */
    if (location->prefixLen > 0 && location->prefix[location->prefixLen - 1] == '/') {
        location->prefix[--location->prefixLen] = '\0';
    }
}


void maSetLocationFlags(MaLocation *location, int flags)
{
    location->flags = flags;
}


void maAddErrorDocument(MaLocation *location, cchar *code, cchar *url)
{
    if (mprGetParent(location->errorDocuments) == location->parent) {
        location->errorDocuments = mprCopyHash(location, location->parent->errorDocuments);
    }
    mprAddHash(location->errorDocuments, code, url);
}


cchar *maLookupErrorDocument(MaLocation *location, int code)
{
    char        numBuf[16];

    if (location->errorDocuments == 0) {
        return 0;
    }
    mprItoa(numBuf, sizeof(numBuf), code, 10);
    return (cchar*) mprLookupHash(location->errorDocuments, numBuf);
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
/************************************************************************/
/*
 *  End of file "../http/location.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/log.c"
 */
/************************************************************************/

/*
 *  log.c -- Logging
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



/*
 *  Turn on logging. If no logSpec is specified, default to stdout:2. If the user specifies --log "none" then 
 *  the log is disabled. This is useful when specifying the log via the appweb.conf.
 */
//  TODO - would be nice to have logging per host

static void logHandler(MprCtx ctx, int flags, int level, const char *msg)
{
    Mpr         *mpr;
    MprFile     *file;
    char        *prefix;

    mpr = mprGetMpr(ctx);
    file = (MprFile*) mpr->logHandlerData;
    prefix = mpr->name;

    while (*msg == '\n') {
        mprFprintf(file, "\n");
        msg++;
    }

    if (flags & MPR_LOG_SRC) {
        mprFprintf(file, "%s: %d: %s\n", prefix, level, msg);

    } else if (flags & MPR_ERROR_SRC) {
        /*
         *  Use static printing to avoid malloc when the messages are small.
         *  This is important for memory allocation errors.
         */
        if (strlen(msg) < (MPR_MAX_STRING - 32)) {
            mprStaticErrorPrintf(file, "%s: Error: %s\n", prefix, msg);
        } else {
            mprFprintf(file, "%s: Error: %s\n", prefix, msg);
        }

    } else if (flags & MPR_FATAL_SRC) {
        mprFprintf(file, "%s: Fatal: %s\n", prefix, msg);
        
    } else if (flags & MPR_ASSERT_SRC) {
        mprFprintf(file, "%s: Assertion %s, failed\n", prefix, msg);

    } else if (flags & MPR_RAW) {
        mprFprintf(file, "%s", msg);
    }
}



/*
 *  Start error and information logging. Note: this is not per-request access logging
 */
int maStartLogging(MprCtx ctx, cchar *logSpec)
{
    Mpr         *mpr;
    MprFile     *file;
    MprTime     now;
    char        *levelSpec, *spec, timeText[80];
    int         level;

    level = 0;
    mpr = mprGetMpr(ctx);

    if (logSpec == 0) {
        logSpec = "stdout:2";
    }
    if (*logSpec && strcmp(logSpec, "none") != 0) {
        spec = mprStrdup(mpr, logSpec);
        if ((levelSpec = strrchr(spec, ':')) != 0 && isdigit((int) levelSpec[1])) {
            *levelSpec++ = '\0';
            level = atoi(levelSpec);
        }

        if (strcmp(spec, "stdout") == 0) {
            file = mpr->fileService->console;

        } else {
            if ((file = mprOpen(mpr, spec, O_CREAT | O_WRONLY | O_TRUNC | O_TEXT, 0664)) == 0) {
                mprErrorPrintf(mpr, "Can't open log file %s\n", spec);
                return -1;
            }
        }
        mprSetLogLevel(mpr, level);
        mprSetLogHandler(mpr, logHandler, (void*) file);

        now = mprGetTime(mpr);
        mprCtime(mpr, timeText, sizeof(timeText), now);

        mprLog(mpr, MPR_CONFIG, "Configuration for %s", mprGetAppTitle(mpr));
        mprLog(mpr, MPR_CONFIG, "--------------------------------------------");
        mprLog(mpr, MPR_CONFIG, "Host:               %s", mprGetHostName(mpr));
        mprLog(mpr, MPR_CONFIG, "CPU:                %s", BLD_HOST_CPU);
        mprLog(mpr, MPR_CONFIG, "OS:                 %s", BLD_HOST_OS);
        mprLog(mpr, MPR_CONFIG, "Distribution:       %s %s", BLD_HOST_DIST, BLD_HOST_DIST_VER);
        mprLog(mpr, MPR_CONFIG, "OS:                 %s", BLD_HOST_OS);
        mprLog(mpr, MPR_CONFIG, "Version:            %s.%d", BLD_VERSION, BLD_NUMBER);
        mprLog(mpr, MPR_CONFIG, "BuildType:          %s", BLD_TYPE);
        mprLog(mpr, MPR_CONFIG, "Started at:         %s", timeText);
        mprLog(mpr, MPR_CONFIG, "--------------------------------------------");
    }

    return 0;
}


/*
 *  Stop the error and information logging. Note: this is not per-request access logging
 */
int maStopLogging(MprCtx ctx)
{
    MprFile     *file;
    Mpr         *mpr;

    mpr = mprGetMpr(ctx);

    file = mpr->logHandlerData;
    if (file) {
        mprFree(file);
        mpr->logHandlerData = 0;
        mprSetLogHandler(mpr, 0, 0);
    }
    return 0;
}

#if BLD_FEATURE_ACCESS_LOG

int maStartAccessLogging(MaHost *host)
{
#if !BLD_FEATURE_ROMFS
    if (host->logPath) {
        host->accessLog = mprOpen(host, host->logPath, O_CREAT | O_APPEND | O_WRONLY | O_TEXT, 0664);
        if (host->accessLog ==0) {
            mprError(host, "Can't open log file %s", host->logPath);
        }
    }
#endif
    return 0;
}


int maStopAccessLogging(MaHost *host)
{
    if (host->accessLog) {
        mprFree(host->accessLog);
        host->accessLog = 0;
    }
    return 0;
}


void maSetAccessLog(MaHost *host, cchar *path, cchar *format)
{
    char    *src, *dest;

    mprAssert(host);
    mprAssert(path && *path);
    mprAssert(format);

    mprFree(host->logPath);
    host->logPath = mprStrdup(host, path);

    mprFree(host->logFormat);
    host->logFormat = mprStrdup(host, format);

    for (src = dest = host->logFormat; *src; src++) {
        if (*src == '\\' && src[1] != '\\') {
            continue;
        }
        *dest++ = *src;
    }
    *dest = '\0';
}


void maSetLogHost(MaHost *host, MaHost *logHost)
{
    host->logHost = logHost;
}


void maWriteAccessLogEntry(MaHost *host, cchar *buf, int len)
{
    static int once = 0;

    if (mprWrite(host->accessLog, (char*) buf, len) != len && once++ == 0) {
        mprError(host, "Can't write to access log %s", host->logPath);
    }
}


/*
 *  Called to rotate the access log
 */
void maRotateAccessLog(MaHost *host)
{
    MprFileInfo     info;
    struct tm       tm;
    MprTime         when;
    char            bak[MPR_MAX_FNAME];

    /*
     *  Rotate logs when full
     */
    if (mprGetFileInfo(host, host->logPath, &info) == 0 && info.size > MA_MAX_ACCESS_LOG) {

        when = mprGetTime(host);
        mprGmtime(host, &tm, when);

        mprSprintf(bak, sizeof(bak), "%s-%02d-%02d-%02d-%02d:%02d:%02d", host->logPath, 
            tm.tm_mon, tm.tm_mday, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

        mprFree(host->accessLog);
        rename(host->logPath, bak);
        unlink(host->logPath);

        host->accessLog = mprOpen(host, host->logPath, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0664);
    }
}


void maLogRequest(MaConn *conn)
{
    MaHost      *logHost, *host;
    MaResponse  *resp;
    MaRequest   *req;
    MprBuf      *buf;
    MprTime     now;
    char        keyBuf[80], timeBuf[64], *fmt, *cp, *qualifier, *value, c;
    int         len;

    resp = conn->response;
    req = conn->request;
    host = req->host;

    logHost = host->logHost;
    if (logHost == 0) {
        return;
    }
    fmt = logHost->logFormat;
    if (fmt == 0) {
        return;
    }

    len = MA_MAX_URL + 256;
    buf = mprCreateBuf(resp, len, len);

    while ((c = *fmt++) != '\0') {
        if (c != '%' || (c = *fmt++) == '%') {
            mprPutCharToBuf(buf, c);
            continue;
        }

        switch (c) {
        case 'a':                           /* Remote IP */
            mprPutStringToBuf(buf, conn->remoteIpAddr);
            break;

        case 'A':                           /* Local IP */
            mprPutStringToBuf(buf, conn->sock->listenSock->ipAddr);
            break;

        case 'b':
            if (resp->bytesWritten == 0) {
                mprPutCharToBuf(buf, '-');
            } else {
                mprPutIntToBuf(buf, resp->bytesWritten);
            } 
            break;

        case 'B':                           /* Bytes written (minus headers) */
            mprPutIntToBuf(buf, resp->bytesWritten - resp->headerSize);
            break;

        case 'h':                           /* Remote host */
            //  TODO - Should this trigger a reverse DNS?
            mprPutStringToBuf(buf, conn->remoteIpAddr);
            break;

        case 'n':                           /* Local host */
            mprPutStringToBuf(buf, req->parsedUri->host);
            break;

        case 'l':                           /* Supplied in authorization */
            mprPutStringToBuf(buf, req->user ? req->user : "-");
            break;

        case 'O':                           /* Bytes written (including headers) */
            mprPutIntToBuf(buf, resp->bytesWritten);
            break;

        case 'r':                           /* First line of request */
            mprPutFmtToBuf(buf, "%s %s %s", req->methodName, req->parsedUri->originalUri, req->httpProtocol);
            break;

        case 's':                           /* Response code */
            mprPutIntToBuf(buf, resp->code);
            break;

        case 't':                           /* Time */
            now = mprGetTime(conn);
            mprCtime(conn, timeBuf, sizeof(timeBuf), now);
            if ((cp = strchr(timeBuf, '\n')) != 0) {
                *cp = '\0';
            }
            mprPutCharToBuf(buf, '[');
            mprPutStringToBuf(buf, timeBuf);
            mprPutCharToBuf(buf, ']');
            break;

        case 'u':                           /* Remote username */
            mprPutStringToBuf(buf, req->user ? req->user : "-");
            break;

        case '{':                           /* Header line */
            qualifier = fmt;
            if ((cp = strchr(qualifier, '}')) != 0) {
                fmt = &cp[1];
                *cp = '\0';
                c = *fmt++;
                mprStrcpy(keyBuf, sizeof(keyBuf), "HTTP_");
                mprStrcpy(&keyBuf[5], sizeof(keyBuf) - 5, qualifier);
                mprStrUpper(keyBuf);
                switch (c) {
                case 'i':
                    value = (char*) mprLookupHash(req->headers, keyBuf);
                    mprPutStringToBuf(buf, value ? value : "-");
                    break;
                default:
                    mprPutStringToBuf(buf, qualifier);
                }
                *cp = '}';

            } else {
                mprPutCharToBuf(buf, c);
            }
            break;

        case '>':
            if (*fmt == 's') {
                fmt++;
                mprPutIntToBuf(buf, resp->code);
            }
            break;

        default:
            mprPutCharToBuf(buf, c);
            break;
        }
    }
    mprPutCharToBuf(buf, '\n');
    mprAddNullToBuf(buf);

    mprWrite(logHost->accessLog, mprGetBufStart(buf), mprGetBufLength(buf));
}

#else
void maLogRequest(MaConn *conn) {}
#endif /* BLD_FEATURE_ACCESS_LOG */



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
/************************************************************************/
/*
 *  End of file "../http/log.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/mime.c"
 */
/************************************************************************/

/* 
 *  mime.c
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */




int maOpenMimeTypes(MaHost *host, cchar *path)
{
    MprFile     *file;
    char        buf[80], *tok, *ext, *type;
    int         line;

    host->mimeFile = mprStrdup(host, path);

    if (host->mimeTypes == 0) {
        host->mimeTypes = mprCreateHash(host, MA_MIME_HASH_SIZE);
    }
    file = mprOpen(host, path, O_RDONLY | O_TEXT, 0);
    if (file == 0) {
        return MPR_ERR_CANT_OPEN;
    }

    line = 0;
    while (mprGets(file, buf, sizeof(buf)) != 0) {
        line++;
        if (buf[0] == '#' || isspace((int) buf[0])) {
            continue;
        }
        type = mprStrTok(buf, " \t\n\r", &tok);
        ext = mprStrTok(0, " \t\n\r", &tok);
        if (type == 0 || ext == 0) {
            mprError(host, "Bad mime spec in %s at line %d", path, line);
            continue;
        }
        while (ext) {
            maAddMimeType(host, ext, type);
            ext = mprStrTok(0, " \t\n\r", &tok);
        }
    }

    mprFree(file);

    return 0;
}


/*
 *  Add a mime type to the mime lookup table. Action Programs are added separately.
 */
MaMimeType *maAddMimeType(MaHost *host, cchar *ext, cchar *mimeType)
{
    MaMimeType  *mime;

    mime = mprAllocObjZeroed(host->mimeTypes, MaMimeType);
    if (mime == 0) {
        return 0;
    }
    mime->type = mprStrdup(host, mimeType);

    if (host->mimeTypes == 0) {
        host->mimeTypes = mprCreateHash(host, MA_MIME_HASH_SIZE);
    }
    if (*ext == '.') {
        ext++;
    }
    mprAddHash(host->mimeTypes, ext, mime);
    return mime;
}


int maSetMimeActionProgram(MaHost *host, cchar *mimeType, cchar *actionProgram)
{
    MaMimeType      *mime;
    MprHash         *hp;
    
    hp = 0;
    mime = 0;
    while ((hp = mprGetNextHash(host->mimeTypes, hp)) != 0) {
        mime = (MaMimeType*) hp->data;
        if (mime->type[0] == mimeType[0] && strcmp(mime->type, mimeType) == 0) {
            break;
        }
    }
    if (mime == 0) {
        mprError(host, "Can't find mime type %s for action program %s", mimeType, actionProgram);
        return MPR_ERR_NOT_FOUND;
    }

    mprFree(mime->actionProgram);
    mime->actionProgram = mprStrdup(host, actionProgram);

    return 0;
}


cchar *maGetMimeActionProgram(MaHost *host, cchar *mimeType)
{
    MaMimeType      *mime;

    if (mimeType == 0 || *mimeType == '\0') {
        return 0;
    }
    mime = (MaMimeType*) mprLookupHash(host->mimeTypes, mimeType);
    if (mime == 0) {
        return 0;
    }
    return mime->actionProgram;
}


cchar *maLookupMimeType(MaHost *host, cchar *ext)
{
    MaMimeType      *mime;

    if (ext == 0 || *ext == '\0') {
        return 0;
    }
    mime = (MaMimeType*) mprLookupHash(host->mimeTypes, ext);
    if (mime == 0) {
        return 0;
    }

    return mime->type;
}


void maAddStandardMimeTypes(MaHost *host)
{
    maAddMimeType(host, "ai", "application/postscript");
    maAddMimeType(host, "asc", "text/plain");
    maAddMimeType(host, "au", "audio/basic");
    maAddMimeType(host, "avi", "video/x-msvideo");
    maAddMimeType(host, "bin", "application/octet-stream");
    maAddMimeType(host, "bmp", "image/bmp");
    maAddMimeType(host, "class", "application/octet-stream");
    maAddMimeType(host, "css", "text/css");
    maAddMimeType(host, "dll", "application/octet-stream");
    maAddMimeType(host, "doc", "application/msword");
    maAddMimeType(host, "ejs", "text/html");
    maAddMimeType(host, "eps", "application/postscript");
    maAddMimeType(host, "es", "application/x-javascript");
    maAddMimeType(host, "exe", "application/octet-stream");
    maAddMimeType(host, "gif", "image/gif");
    maAddMimeType(host, "gz", "application/x-gzip");
    maAddMimeType(host, "htm", "text/html");
    maAddMimeType(host, "html", "text/html");
    maAddMimeType(host, "ico", "image/x-icon");
    maAddMimeType(host, "jar", "application/octet-stream");
    maAddMimeType(host, "jpeg", "image/jpeg");
    maAddMimeType(host, "jpg", "image/jpeg");
    maAddMimeType(host, "js", "application/x-javascript");
    maAddMimeType(host, "mp3", "audio/mpeg");
    maAddMimeType(host, "pdf", "application/pdf");
    maAddMimeType(host, "php", "text/html");
    maAddMimeType(host, "png", "image/png");
    maAddMimeType(host, "ppt", "application/vnd.ms-powerpoint");
    maAddMimeType(host, "ps", "application/postscript");
    maAddMimeType(host, "ra", "audio/x-realaudio");
    maAddMimeType(host, "ram", "audio/x-pn-realaudio");
    maAddMimeType(host, "rmm", "audio/x-pn-realaudio");
    maAddMimeType(host, "rtf", "text/rtf");
    maAddMimeType(host, "rv", "video/vnd.rn-realvideo");
    maAddMimeType(host, "so", "application/octet-stream");
    maAddMimeType(host, "swf", "application/x-shockwave-flash");
    maAddMimeType(host, "tar", "application/x-tar");
    maAddMimeType(host, "tgz", "application/x-gzip");
    maAddMimeType(host, "tiff", "image/tiff");
    maAddMimeType(host, "txt", "text/plain");
    maAddMimeType(host, "wav", "audio/x-wav");
    maAddMimeType(host, "xls", "application/vnd.ms-excel");
    maAddMimeType(host, "zip", "application/zip");
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
/************************************************************************/
/*
 *  End of file "../http/mime.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/misc.c"
 */
/************************************************************************/

/*
 *  misc.c -- Bits and pieces
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Build an ASCII time string.  If sbuf is NULL we use the current time, else we use the last modified time of sbuf
 */
char *maGetDateString(MprCtx ctx, MprFileInfo *sbuf)
{
    char        *dateStr;
    struct tm   tm;
    MprTime     when;

    if (sbuf == 0) {
        when = mprGetTime(ctx);
    } else {
        when = (MprTime) sbuf->mtime * MPR_TICKS_PER_SEC;
    }
    dateStr = (char*) mprAlloc(ctx, 64);
    mprGmtime(ctx, &tm, when);
    mprStrftime(ctx, dateStr, 64, "%a, %d %b %Y %T %Z", &tm);
    return dateStr;
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
/************************************************************************/
/*
 *  End of file "../http/misc.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/modules/sslModule.c"
 */
/************************************************************************/

/*
 *  sslModule.c - Module for SSL support
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */


#include    "mprSsl.h"

#if BLD_FEATURE_SSL

static int parseSsl(MaHttp *http, cchar *key, char *value, MaConfigState *state)
{
    MaLocation  *location;
    MaServer    *server;
    MaHost      *host;
    char        pathBuf[MPR_MAX_FNAME], prefix[MPR_MAX_FNAME];
    char        *tok, *word, *enable, *provider;
    int         protoMask, mask;

    host = state->host;
    server = state->server;
    location = state->location;

    mprStrcpy(prefix, sizeof(prefix), key);
    prefix[3] = '\0';
    if (mprStrcmpAnyCase(prefix, "SSL") != 0) {
        return 0;
    }

    if (!mprHasSecureSockets(http)) {
        mprError(http, "Missing an SSL Provider");
        return MPR_ERR_BAD_SYNTAX;
    }

    if (location->ssl == 0) {
        location->ssl = mprCreateSsl(location);
    }

    if (mprStrcmpAnyCase(key, "SSLEngine") == 0) {
        enable = mprStrTok(value, " \t", &tok);
        provider = mprStrTok(0, " \t", &tok);
        if (mprStrcmpAnyCase(value, "on") == 0) {
            maSecureHost(host, location->ssl);
        }
        return 1;
    }

    if (maMakePath(host, pathBuf, sizeof(pathBuf), mprStrTrim(value, "\"")) == 0) {
        mprError(http, "SSL path is too long");
        return MPR_ERR_BAD_SYNTAX;
    }

    if (mprStrcmpAnyCase(key, "SSLCACertificatePath") == 0) {
        mprSetSslCaPath(location->ssl, pathBuf);
        return 1;

    } else if (mprStrcmpAnyCase(key, "SSLCACertificateFile") == 0) {
        mprSetSslCaFile(location->ssl, pathBuf);
        return 1;

    } else if (mprStrcmpAnyCase(key, "SSLCertificateFile") == 0) {
        mprSetSslCertFile(location->ssl, pathBuf);
        return 1;

    } else if (mprStrcmpAnyCase(key, "SSLCertificateKeyFile") == 0) {
        mprSetSslKeyFile(location->ssl, pathBuf);
        return 1;

    } else if (mprStrcmpAnyCase(key, "SSLCipherSuite") == 0) {
        mprSetSslCiphers(location->ssl, value);
        return 1;

    } else if (mprStrcmpAnyCase(key, "SSLVerifyClient") == 0) {
        if (mprStrcmpAnyCase(value, "require") == 0) {
            mprVerifySslClients(location->ssl, 1);

        } else if (mprStrcmpAnyCase(value, "none") == 0) {
            mprVerifySslClients(location->ssl, 0);

        } else {
            return -1;
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "SSLProtocol") == 0) {
        protoMask = 0;
        word = mprStrTok(value, " \t", &tok);
        while (word) {
            mask = -1;
            if (*word == '-') {
                word++;
                mask = 0;
            } else if (*word == '+') {
                word++;
            }
            if (mprStrcmpAnyCase(word, "SSLv2") == 0) {
                protoMask &= ~(MPR_HTTP_PROTO_SSLV2 & ~mask);
                protoMask |= (MPR_HTTP_PROTO_SSLV2 & mask);

            } else if (mprStrcmpAnyCase(word, "SSLv3") == 0) {
                protoMask &= ~(MPR_HTTP_PROTO_SSLV3 & ~mask);
                protoMask |= (MPR_HTTP_PROTO_SSLV3 & mask);

            } else if (mprStrcmpAnyCase(word, "TLSv1") == 0) {
                protoMask &= ~(MPR_HTTP_PROTO_TLSV1 & ~mask);
                protoMask |= (MPR_HTTP_PROTO_TLSV1 & mask);

            } else if (mprStrcmpAnyCase(word, "ALL") == 0) {
                protoMask &= ~(MPR_HTTP_PROTO_ALL & ~mask);
                protoMask |= (MPR_HTTP_PROTO_ALL & mask);
            }
            word = mprStrTok(0, " \t", &tok);
        }
        mprSetSslProtocols(location->ssl, protoMask);
        return 1;
    }
    return 0;
}


/*
 *  Loadable module initialization. 
 */
MprModule *maSslModuleInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *stage;

    if ((module = mprLoadSsl(http, 1)) == 0) {
        return 0;
    }
    module->name = "sslModule";
    if ((stage = maCreateStage(http, "sslModule", MA_STAGE_MODULE)) == 0) {
        mprFree(module);
        return 0;
    }
    stage->parse = parseSsl; 

    return module;
}


#else
void __maSslModuleDummy() {}
#endif /* BLD_FEATURE_SSL */

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
/************************************************************************/
/*
 *  End of file "../http/modules/sslModule.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/pipeline.c"
 */
/************************************************************************/

/*
 *  pipeline.c -- HTTP pipeline processing.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static char *addIndex(MaConn *conn, char *path, int pathsize, cchar *index);
static char *getExtension(MaConn *conn);
static MaStage *findHandlerByExtension(MaConn *conn);
static MaStage *findLocationHandler(MaConn *conn);
static char *makeFilename(MaConn *conn, MaAlias *alias, cchar *url, bool skipAliasPrefix);
static bool mapToFile(MaConn *conn, bool *rescan);
static bool matchFilter(MaConn *conn, MaFilter *filter);
static void openQ(MaQueue *q);
static void processDirectory(MaConn *conn, bool *rescan);
static void setEnv(MaConn *conn);
static void setPathInfo(MaConn *conn);


/*
 *  Find the matching handler for a request. If any errors occur, the pass handler is used to pass errors onto the 
 *  net/sendfile connectors to send to the client. This routine may rewrite the request URI and may redirect the request.
 */
void maMatchHandler(MaConn *conn)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaHost          *host;
    MaAlias         *alias;
    MaStage         *handler;
    bool            rescan;
    int             loopCount;

    req = conn->request;
    resp = conn->response;
    host = req->host;

    /*
     *  Find the alias that applies for this url. There is always a catch-all alias for the document root.
     */
    alias = req->alias = maGetAlias(host, req->url);
    mprAssert(alias);
    if (alias->redirectCode) {
        // TODO - what about internal redirects?
        maRedirect(conn, alias->redirectCode, alias->uri);
        return;
    }

    if (conn->requestFailed || conn->request->method & (MA_REQ_OPTIONS | MA_REQ_TRACE)) {
        handler = conn->http->passHandler;
        return;
    }

    /*
     *  Get the best (innermost) location block and see if a handler is explicitly set for that location block.
     *  Possibly rewrite the url and retry.
     */
    loopCount = MA_MAX_REWRITE;
    do {
        rescan = 0;
        if ((handler = findLocationHandler(conn)) == 0) {
            /*
             *  Didn't find a location block handler, so try to match by extension and by handler match() routines.
             *  This may invoke processDirectory which may redirect and thus require reprocessing -- hence the loop.
             */
            handler = findHandlerByExtension(conn);
        }

        if (handler && !(handler->flags & MA_STAGE_VIRTUAL)) {
            if (!mapToFile(conn, &rescan)) {
                return;
            }
        }
    } while (handler && rescan && loopCount-- > 0);

    if (handler == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_METHOD, "Requested method %s not supported for URL: %s", 
            req->methodName, req->url);
        handler = conn->http->passHandler;
    }
    resp->handler = handler;

    mprLog(resp, 4, "Select handler: \"%s\" for \"%s\"", handler->name, req->url);

    setEnv(conn);
}


/*
 *  Create stages for the request pipeline.
 */
void maCreatePipeline(MaConn *conn)
{
    MaHttp          *http;
    MaHost          *host;
    MaResponse      *resp;
    MaRequest       *req;
    MaStage         *handler;
    MaLocation      *location;
    MaStage         *stage, *connector;
    MaFilter        *filter;
    MaQueue         *q, *qhead, *rq, *rqhead;
    int             next;

    req = conn->request;
    resp = conn->response;
    host = req->host;
    location = req->location;
    handler = resp->handler;
    http = conn->http;

    mprAssert(req);
    mprAssert(location->outputStages);

    /*
     *  Create the output pipeline for this request. Handler first, then filters, connector last.
     */
    resp->outputPipeline = mprCreateList(resp);

    /*
     *  Add the handler and filters. Switch to the pass handler if any errors have occurred so far. Only add the 
     *  filters if the request has not failed.
     */
    if (conn->requestFailed) {
        resp->handler = http->passHandler;
        mprAddItem(resp->outputPipeline, resp->handler);

    } else {
        mprAddItem(resp->outputPipeline, resp->handler);
        for (next = 0; (filter = mprGetNextItem(location->outputStages, &next)) != 0; ) {
            if (filter->stage == http->authFilter) {
                if (req->auth->type == 0 && req->auth->type == 0) {
                    continue;
                }
            }
            if (filter->stage == http->rangeFilter && (req->ranges == 0 || handler == http->fileHandler)) {
                continue;
            }
            if ((filter->stage->flags & MA_STAGE_ALL & req->method) == 0) {
                continue;
            }
            /*
             *  Remove the chunk filter chunking if it is explicitly turned off vi a the X_APPWEB_CHUNK_SIZE header setting
             *  the chunk size to zero. Also remove if using the fileHandler which always knows the entity length and an
             *  explicit chunk size has not been requested.
             */
            if (filter->stage == http->chunkFilter) {
                if ((handler == http->fileHandler && resp->chunkSize < 0) || resp->chunkSize == 0) {
                    continue;
                }
            }
            if (matchFilter(conn, filter)) {
                mprAddItem(resp->outputPipeline, filter->stage);
            }
        }
    }
    
    connector = location->connector;
#if BLD_FEATURE_SEND
    if (resp->handler == http->fileHandler && connector == http->netConnector && 
        http->sendConnector && !req->ranges && !host->secure) {
        /*
         *  Switch (transparently) to the send connector if serving whole static file content via the net connector
         */
        connector = http->sendConnector;
    }
#endif
    resp->connector = connector;
    if ((connector->flags & MA_STAGE_ALL & req->method) == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Connector \"%s\" does not support the \"%s\" method \"%s\"", 
            connector->name, req->methodName);
        return;
    }
    mprAddItem(resp->outputPipeline, connector);

    /*
     *  Create the outgoing queue heads and open the queues
     */
    q = &resp->queue[MA_QUEUE_SEND];
    for (next = 0; (stage = mprGetNextItem(resp->outputPipeline, &next)) != 0; ) {
        q = maCreateQueue(conn, stage, MA_QUEUE_SEND, q);
    }

    /*
     *  Create the receive pipeline for this request. Connector first, handler last
     */
    if (req->remainingContent > 0) {
        req->inputPipeline = mprCreateList(resp);

        mprAddItem(req->inputPipeline, connector);
        if (!conn->requestFailed) {
            for (next = 0; (filter = mprGetNextItem(location->inputStages, &next)) != 0; ) {
                if (filter->stage == http->authFilter || !matchFilter(conn, filter)) {
                    continue;
                }
                if ((filter->stage->flags & MA_STAGE_ALL & req->method) == 0) {
                    continue;
                }
                mprAddItem(req->inputPipeline, filter->stage);
            }
        }
        mprAddItem(req->inputPipeline, handler);

        /*
         *  Create the incoming queue heads and open the queues.
         */
        q = &resp->queue[MA_QUEUE_RECEIVE];
        for (next = 0; (stage = mprGetNextItem(req->inputPipeline, &next)) != 0; ) {
            q = maCreateQueue(conn, stage, MA_QUEUE_RECEIVE, q);
        }
    }

    /*
     *  Pair up the send and receive queues. NOTE: can't use a stage multiple times.
     */
    qhead = &resp->queue[MA_QUEUE_SEND];
    rqhead = &resp->queue[MA_QUEUE_RECEIVE];
    for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
        for (rq = rqhead->nextQ; rq != rqhead; rq = rq->nextQ) {
            if (q->stage == rq->stage) {
                q->pair = rq;
                rq->pair = q;
            }
        }
    }

    /*
     *  Open the queues (keep going on errors)
     */
    qhead = &resp->queue[MA_QUEUE_SEND];
    for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
        if (q->open && !(q->flags & MA_QUEUE_OPEN)) {
            q->flags |= MA_QUEUE_OPEN;
            openQ(q);
        }
    }

    if (req->remainingContent > 0) {
        qhead = &resp->queue[MA_QUEUE_RECEIVE];
        for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
            if (q->open && !(q->flags & MA_QUEUE_OPEN)) {
                if (q->pair == 0 || !(q->pair->flags & MA_QUEUE_OPEN)) {
                    q->flags |= MA_QUEUE_OPEN;
                    openQ(q);
                }
            }
        }
    }
}


/*
 *  Invoke the run routine for the handler and then pump the pipeline by servicing all scheduled queues.
 */
bool maRunPipeline(MaConn *conn)
{
    MaQueue     *q;
    
    q = conn->response->queue[MA_QUEUE_SEND].nextQ;
    
    if (q->stage->run) {
        q->stage->run(q);
    }
    return maServiceQueues(conn);
}


/*
 *  Run the queue service routines until there is no more work to be done. NOTE: all I/O is non-blocking.
 */
bool maServiceQueues(MaConn *conn)
{
    MaQueue     *q;
    bool        workDone;

    workDone = 0;
    while (!conn->abandonConnection && (q = maGetNextQueueForService(&conn->serviceq)) != NULL) {
        maServiceQueue(q);
        workDone = 1;
    }
    return workDone;
}


void maDiscardPipeData(MaConn *conn)
{
    MaResponse      *resp;
    MaQueue         *q, *qhead;

    resp = conn->response;

    qhead = &resp->queue[MA_QUEUE_SEND];
    for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
        maDiscardData(q, 0);
    }

    qhead = &resp->queue[MA_QUEUE_RECEIVE];
    for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
        maDiscardData(q, 0);
    }
}


static char *addIndex(MaConn *conn, char *urlbuf, int bufsize, cchar *index)
{
    MaRequest       *req;
    char            *indexDelim;

    req = conn->request;

    indexDelim = (req->url[strlen(req->url) - 1] = '/') ? "" : "/";
    
    if (req->parsedUri->query && req->parsedUri->query[0]) {
        mprSprintf(urlbuf, bufsize, "%s%s%s?%s", req->url, indexDelim, index, req->parsedUri->query);
    } else {
        mprSprintf(urlbuf, bufsize, "%s%s%s", req->url, indexDelim, index);
    }
    return urlbuf;
}


static MaStage *checkStage(MaConn *conn, MaStage *stage)
{
    MaRequest   *req;

    req = conn->request;

    if (stage == 0) {
        return 0;
    }
    if ((stage->flags & MA_STAGE_ALL & req->method) == 0) {
        return 0;
    }
    if (stage->match && !stage->match(conn, stage, req->url)) {
        return 0;
    }
    return stage;
}


int maRewriteUri(MaConn *conn) { return 0; }

static MaStage *findLocationHandler(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    MaLocation  *location;
    int         loopCount;

    req = conn->request;
    resp = conn->response;
    loopCount = MA_MAX_REWRITE;

    do {
        location = req->location = maLookupBestLocation(req->host, req->url);
        mprAssert(location);
        req->auth = location->auth;
        resp->handler = checkStage(conn, location->handler);
    } while (maRewriteUri(conn) && --loopCount > 0);

    return resp->handler;
}


static char *getExtension(MaConn *conn)
{
    MaRequest   *req;
    char        *cp;
    char        *ep, ext[16];

    req = conn->request;

    /*
     *  This is not perfect, but manageable. If a directory after the location prefix has a "." in it, then it may
     *  be mis-interpreted as the extension.
     */
    if ((cp = strchr(&req->url[req->alias->prefixLen], '.')) != 0) {
        cp++;
        for (ep = ext; isalpha((int) *cp) && ep < &ext[sizeof(ext)]; ) {
            *ep++ = *cp++;
        }
        *ep = '\0';
        return mprStrdup(req, ext);
    }
    return "";
}


/*
 *  Search for a handler by request extension. If that fails, use handler custom matching.
 *  If all that fails, return the catch-all handler (fileHandler)
 */
static MaStage *findHandlerByExtension(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    MaStage     *handler;
    MaLocation  *location;
    int         next;

    req = conn->request;
    resp = conn->response;
    location = req->location;
    
    resp->extension = getExtension(conn);

    if (*resp->extension) {
        handler = maGetHandlerByExtension(location, resp->extension);
        if (checkStage(conn, handler)) {
            return handler;
        }
    }

    /*
     *  Failed to match by extension, so perform custom handler matching. May need a filename (dir handler)
     */
    resp->filename = makeFilename(conn, req->alias, req->url, 1);
    for (next = 0; (handler = mprGetNextItem(location->handlers, &next)) != 0; ) {
        if (handler->match && handler->match(conn, handler, req->url)) {
            if (checkStage(conn, handler)) {
                resp->handler = handler;
                return handler;
            }
        }
    }

    /*
     *  Failed to match. Return any catch-all handler.
     */
    handler = maGetHandlerByExtension(location, "");
    if (handler == 0) {
        /*
         *  Could be missing a catch-all in the config file, so invoke the file handler.
         */
        handler = maLookupStage(conn->http, "fileHandler");
    }

    mprAssert(handler);
    //  TODO - should we be setting this here?
    resp->handler = handler;
    
    return checkStage(conn, handler);
}


//  TODO - remove last parameter
static char *makeFilename(MaConn *conn, MaAlias *alias, cchar *url, bool skipAliasPrefix)
{
    char        *cleanPath, *path;
    int         len;

    mprAssert(alias);
    mprAssert(url && *url);

    if (skipAliasPrefix) {
        url += alias->prefixLen;
    }
    while (*url == '/') {
        url++;
    }

    len = (int) strlen(alias->filename);
    if ((path = mprAlloc(conn->request, len + (int) strlen(url) + 2)) == 0) {
        return 0;
    }
    strcpy(path, alias->filename);
    if (*url) {
        path[len++] = '/';
        strcpy(&path[len], url);
    }

    cleanPath = mprCleanFilename(conn, path);
    mprMapDelimiters(conn, cleanPath, '/');
    mprFree(path);

    return cleanPath;
}


static bool mapToFile(MaConn *conn, bool *rescan)
{
    MaRequest   *req;
    MaResponse  *resp;

    req = conn->request;
    resp = conn->response;

    if (resp->filename == 0) {
        resp->filename = makeFilename(conn, req->alias, req->url, 1);
    }
    req->dir = maLookupBestDir(req->host, resp->filename);

    if (req->dir == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Missing directory block for %s", resp->filename);
        return 0;
    }

    mprAssert(req->dir);

    req->auth = req->dir->auth;

    if (!resp->fileInfo.valid && mprGetFileInfo(conn, resp->filename, &resp->fileInfo) < 0) {
#if UNUSED
        if (req->method & (MA_REQ_GET | MA_REQ_POST)) {
            maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Can't open document: %s", resp->filename);
            return 0;
        }
#endif
    }
    if (resp->fileInfo.isDir) {
        processDirectory(conn, rescan);
    }
    return 1;
}


/*
 *  Match a filter by extension
 */
static bool matchFilter(MaConn *conn, MaFilter *filter)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaLocation      *location;
    MaStage         *stage;

    req = conn->request;
    resp = conn->response;
    location = req->location;
    stage = filter->stage;

    if (stage->match) {
        return stage->match(conn, stage, req->url);
    }

    if (filter->extensions && *resp->extension) {
        return maMatchFilterByExtension(filter, resp->extension);
    }
    return 1;
}


static void openQ(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;

    conn = q->conn;
    resp = conn->response;

    if (resp->chunkSize > 0) {
        q->packetSize = min(q->packetSize, resp->chunkSize);
    }
    q->flags |= MA_QUEUE_OPEN;
    if (q->open) {
        q->open(q);
    }
}


/*
 *  Manage requests to directories. This will either do an external redirect back to the browser or do an internal 
 *  (transparent) redirection and serve different content back to the browser. This routine may modify the requested 
 *  URI and/or the request handler.
 */
static void processDirectory(MaConn *conn, bool *rescan)
{
    MaRequest       *req;
    MaResponse      *resp;
    MprFileInfo     *info;
    char            path[MPR_MAX_FNAME], urlBuf[MPR_MAX_FNAME], *index;
    int             len;

    req = conn->request;
    resp = conn->response;
    info = &resp->fileInfo;

    mprAssert(info->isDir);
    index = req->dir->indexName;
    if (req->url[strlen(req->url) - 1] == '/') {
        /*
         *  Internal directory redirections
         */
        len = (int) strlen(resp->filename);
        if (resp->filename[len - 1] == '/') {
            resp->filename[len - 1] = '\0';
        }
        path[0] = '\0';
        mprAssert(resp->filename && *resp->filename);
        mprAssert(index && *index);
        mprStrcat(path, sizeof(path), NULL, resp->filename, "/", index, NULL);
        if (mprAccess(resp, path, R_OK)) {
            /*
             *  Index file exists, so do an internal redirect to it. Client will not be aware of this happening.
             *  Must rematch the handler on return.
             */
            maSetRequestUri(conn, addIndex(conn, urlBuf, sizeof(urlBuf), index));

            resp->filename = mprStrdup(resp, path);
            mprGetFileInfo(conn, resp->filename, &resp->fileInfo);

            resp->extension = getExtension(conn);
            if ((resp->mimeType = (char*) maLookupMimeType(conn->host, resp->extension)) == 0) {
                resp->mimeType = (char*) "text/html";
            }
            *rescan = 1;
        }
        return;
    }

    /*
     *  External redirect. Ask the client to re-issue a request for a new location. See if an index exists and if so, 
     *  construct a new location for the index. If the index can't be accessed, just append a "/" to the URI and redirect.
     */
    if (req->parsedUri->query && req->parsedUri->query[0]) {
        mprSprintf(path, sizeof(path), "%s/%s?%s", req->url, index, req->parsedUri->query);
    } else {
        mprSprintf(path, sizeof(path), "%s/%s", req->url, index);
    }
    if (!mprAccess(resp, path, R_OK)) {
        mprSprintf(path, sizeof(path), "%s/", req->url);
    }
    maRedirect(conn, MPR_HTTP_CODE_MOVED_PERMANENTLY, path);
    resp->handler = conn->http->passHandler;
}


static bool fileExists(MprCtx ctx, cchar *path) {
    if (mprAccess(ctx, path, R_OK)) {
        return 1;
    }
#if BLD_WIN_LIKE
    if (strchr(path, '.') == 0) {
        char    filename[MPR_MAX_FNAME];
        mprSprintf(filename, sizeof(filename), "%s.exe", path);
        if (mprAccess(ctx, filename, R_OK)) {
            return 1;
        }
        mprSprintf(filename, sizeof(filename), "%s.bat", path);
        if (mprAccess(ctx, filename, R_OK)) {
            return 1;
        }
    }
#endif
    return 0;
}


/*
 *  Set the pathInfo (PATH_INFO) and update the request uri. This may set the response filename if convenient.
 */
static void setPathInfo(MaConn *conn)
{
    MaStage     *handler;
    MaAlias     *alias;
    MaRequest   *req;
    MaResponse  *resp;
    char        *last, *start, *cp, *pathInfo;
    int         found;

    req = conn->request;
    resp = conn->response;
    alias = req->alias;
    handler = resp->handler;

    mprAssert(handler);

    if (/* (req->location->flags & MA_LOC_PATH_INFO) || */ (handler && handler->flags & MA_STAGE_PATH_INFO)) {
        if (!(handler->flags & MA_STAGE_VIRTUAL)) {
            /*
             *  Find the longest subset of the filename that matches a real file. Test each segment to see if 
             *  it corresponds to a real physical file. This also defines a new response filename without the 
             *  extra path info.
             */
            last = 0;
            resp->filename = makeFilename(conn, alias, req->url, 1);
            for (cp = start = &resp->filename[strlen(alias->filename)]; cp; ) {
                
                if ((cp = strchr(cp, '/')) != 0) {
                    *cp = '\0';
                }
                found = fileExists(conn, resp->filename);
                if (cp) {
                    *cp = '/';
                }
                if (found) {
                    if (cp) {
                        last = cp++;
                    } else {
                        last = &resp->filename[strlen(resp->filename)];
                        break;
                    }
                } else {
                    break;
                }
            }
            if (last) {
                pathInfo = &req->url[alias->prefixLen + last - start];
                req->pathInfo = mprStrdup(req, pathInfo);
                *last = '\0';
                pathInfo[0] = '\0';
                if (req->pathInfo[0]) {
                    req->pathTranslated = makeFilename(conn, alias, req->pathInfo, 0);
                }
            }
        }
        if (req->pathInfo == 0) {
            req->pathInfo = req->url;
            req->url = "";

            if ((cp = strchr(req->pathInfo, '.')) != 0) {
                resp->extension = mprStrdup(req, ++cp);
            } else {
                resp->extension = "";
            }
            req->pathTranslated = makeFilename(conn, alias, req->pathInfo, 0); 
            resp->filename = alias->filename;
        }
    }
}


static void setEnv(MaConn *conn)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaStage         *handler;
    MprFileInfo     *info;

    req = conn->request;
    resp = conn->response;
    handler = resp->handler;

    setPathInfo(conn);

    if (resp->extension == 0) {
        resp->extension = getExtension(conn);
    }
    if (resp->filename == 0) {
        resp->filename = makeFilename(conn, req->alias, req->url, 1);
    }

    if ((resp->mimeType = (char*) maLookupMimeType(conn->host, resp->extension)) == 0) {
        resp->mimeType = (char*) "text/html";
    }

    if (!(resp->handler->flags & MA_STAGE_VIRTUAL)) {
        /*
         *  Define an Etag for physical entities. Redo the file info if not valid now that extra path has been removed.
         */
        info = &resp->fileInfo;
        if (!info->valid) {
            mprGetFileInfo(conn, resp->filename, info);
        }
        if (info->valid) {
            mprAllocSprintf(resp, &resp->etag, -1, "%x-%Lx-%Lx", info->inode, info->size, info->mtime);
        }
    }

    if (handler->flags & MA_STAGE_FORM_VARS) {
        req->formVars = mprCreateHash(req, MA_VAR_HASH_SIZE);
        if (req->parsedUri->query) {
            maAddFormVars(conn, req->parsedUri->query, (int) strlen(req->parsedUri->query));
        }
    }
    if (handler->flags & MA_STAGE_ENV_VARS) {
        maCreateEnvVars(conn);
    }
}


//  TODO - should be MapUrl
char *maMapUriToStorage(MaConn *conn, cchar *url)
{
    MaAlias     *alias;

    alias = maGetAlias(conn->request->host, url);
    if (alias == 0) {
        return 0;
    }
    return makeFilename(conn, alias, url, 1);
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
/************************************************************************/
/*
 *  End of file "../http/pipeline.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/queue.c"
 */
/************************************************************************/

/*
 *  queue.c -- Queue support routines
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */



/*
 *  Createa a new queue for the given stage. If prev is given, then link the new queue after the previous queue.
 */

MaQueue *maCreateQueue(MaConn *conn, MaStage *stage, int direction, MaQueue *prev)
{
    MaQueue     *q;
    MaResponse  *resp;
    MaLimits    *limits;

    resp = conn->response;
    limits = &conn->http->limits;

    q = mprAllocObjZeroed(resp, MaQueue);
    if (q == 0) {
        return 0;
    }
    
    maInitQueue(conn->http, q, stage->name);
    maInitSchedulerQueue(q);

    q->conn = conn;
    q->stage = stage;
    q->close = stage->close;
    q->open = stage->open;
    q->direction = direction;

    q->max = limits->maxStageBuffer;
    q->packetSize = limits->maxStageBuffer;

    if (direction == MA_QUEUE_SEND) {
        q->put = stage->outgoingData;
        q->service = stage->outgoingService;
        
    } else {
        q->put = stage->incomingData;
        q->service = stage->incomingService;
    }
    
    if (prev) {
        maInsertQueue(prev, q);
    }
    return q;
}


/*
 *  Initialize a bare queue. Used for dummy heads.
 */
void maInitQueue(MaHttp *http, MaQueue *q, cchar *name)
{
    q->nextQ = q;
    q->prevQ = q;
    q->owner = name;

    q->max = http->limits.maxStageBuffer;
    q->low = q->max / 100 * 5;
}


/*
 *  Insert a queue after the previous element
 */
void maAppendQueue(MaQueue *head, MaQueue *q)
{
    q->nextQ = head;
    q->prevQ = head->prevQ;
    head->prevQ->nextQ = q;
    head->prevQ = q;
}


/*
 *  Insert a queue after the previous element
 */
void maInsertQueue(MaQueue *prev, MaQueue *q)
{
    q->nextQ = prev->nextQ;
    q->prevQ = prev;
    
    prev->nextQ->prevQ = q;
    prev->nextQ = q;
}


void maRemoveQueue(MaQueue *q)
{
    q->prevQ->nextQ = q->nextQ;
    q->nextQ->prevQ = q->prevQ;
    q->prevQ = q->nextQ = q;
}


MaQueue *findPreviousQueue(MaQueue *q)
{
    while (q->prevQ) {
        q = q->prevQ;
        if (q->service) {
            return q;
        }
    }
    return 0;
}


bool maIsQueueEmpty(MaQueue *q)
{
    return q->first == 0;
}


/*
 *  Get the next packet from the queue
 */
MaPacket *maGet(MaQueue *q)
{
    MaConn      *conn;
    MaQueue     *prev;
    MaPacket    *packet;

    conn = q->conn;
    while (q->first) {
        if ((packet = q->first) != 0) {
            if (packet->flags & MA_PACKET_DATA && conn->requestFailed) {
                q->first = packet->next;
                mprFree(packet);
                continue;
            }
            q->first = packet->next;
            packet->next = 0;
            q->count -= packet->count;
            mprAssert(q->count >= 0);
            if (packet == q->last) {
                q->last = 0;
                mprAssert(q->first == 0);
            }
        }
        if (q->flags & MA_QUEUE_FULL && q->count < q->low) {
            /*
             *  This queue was full and now is below the low water mark. Back-enable the previous queue.
             */
            q->flags &= ~MA_QUEUE_FULL;
            prev = findPreviousQueue(q);
            if (prev && prev->flags & MA_QUEUE_DISABLED) {
                maEnableQueue(prev);
            }
        }
        return packet;
    }
    return 0;
}


//  TODO - Make a macro
int maGetPacketLength(MaPacket *packet)
{
    return (packet->content) ? mprGetBufLength(packet->content) : 0;
}


/*
 *  Create a new packet. If size is -1, then also create a default growable buffer -- used for incoming body content. If 
 *  size > 0, then create a non-growable buffer of the requested size.
 */
MaPacket *maCreatePacket(MaConn *conn, int size)
{
    MaPacket    *packet;
    MprCtx      ctx;

#if UNUSED
    ctx = (conn->response) ? (MprCtx) conn->response : (MprCtx) conn;
#else
    ctx = conn;
#endif
    
    packet = mprAllocObjZeroed(ctx, MaPacket);
    if (packet == 0) {
        return 0;
    }
    packet->conn = conn;

    if (size != 0) {
        packet->content = mprCreateBuf(packet, size < 0 ? MA_BUFSIZE: size, -1);
        if (packet->content == 0) {
            mprFree(packet);
            return 0;
        }
    }
    return packet;
}


/*
 *  Create the response header packet
 */
MaPacket *maCreateHeaderPacket(MaConn *conn)
{
    MaPacket    *packet;

    packet = maCreatePacket(conn, MA_BUFSIZE);
    if (packet == 0) {
        return 0;
    }
    packet->flags = MA_PACKET_HEADER;
    return packet;
}


MaPacket *maCreateDataPacket(MaConn *conn, int size)
{
    MaPacket    *packet;

    packet = maCreatePacket(conn, size);
    if (packet == 0) {
        return 0;
    }
    packet->flags = MA_PACKET_DATA;
    return packet;
}


MaPacket *maCreateEndPacket(MaConn *conn)
{
    MaPacket    *packet;

    packet = maCreatePacket(conn, 0);
    if (packet == 0) {
        return 0;
    }
    packet->flags = MA_PACKET_END;
    return packet;
}


/*
 *  Put a packet on the service queue.
 */
void maPutForService(MaQueue *q, MaPacket *packet, bool serviceQ)
{
    mprAssert(packet);
    
    q->count += packet->count;
    packet->next = 0;
    
    if (q->first) {
        q->last->next = packet;
        q->last = packet;
        
    } else {
        q->first = packet;
        q->last = packet;
    }
    if (serviceQ && !(q->flags & MA_QUEUE_DISABLED))  {
        maScheduleQueue(q);
    }
}


/*
 *  Join a packet onto the service queue.
 */
void maJoinForService(MaQueue *q, MaPacket *packet, bool serviceQ)
{
    if (q->first == 0) {
        /*
         *  Just use the service queue as a holding queue while we aggregate the post data.
         */
        maPutForService(q, packet, 0);

    } else {
        /*
         *  Aggregate all data into one packet
         */
        maJoinPacket(q->first, packet);
    }
    if (serviceQ && !(q->flags & MA_QUEUE_DISABLED))  {
        maScheduleQueue(q);
    }
}


/*
 *  Pass to a queue
 */
void maPut(MaQueue *q, MaPacket *packet)
{
    mprAssert(packet);
    
    mprAssert(q->put);
    q->put(q, packet);
}


/*
 *  Pass to the next queue
 */
void maPutNext(MaQueue *q, MaPacket *packet)
{
    mprAssert(packet);
    
    mprAssert(q->nextQ->put);
    q->nextQ->put(q->nextQ, packet);
}


/*
 *  Put the packet back at the front of the queue
 */
void maPutBack(MaQueue *q, MaPacket *packet)
{
    mprAssert(packet);
    mprAssert(packet->next == 0);
    
    packet->next = q->first;

    if (q->first == 0) {
        q->last = packet;
    }
    q->first = packet;

    mprAssert(packet->count >= 0);
    q->count += packet->count;
    mprAssert(q->count >= 0);
}


/*
 *  Return true if the next queue will accept this packet. If not, then disable the queue's service procedure.
 *  This may split the packet if it exceeds the downstreams maximum packet size.
 */
bool maWillNextQueueAccept(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;
    MaQueue     *next;
    int         size;

    conn = q->conn;
    next = q->nextQ;

    size = maGetPacketLength(packet);
    if (size <= next->packetSize && (size + next->count) <= next->max) {
        return 1;
    }

    if (maResizePacket(q, packet, 0) < 0) {
        return 0;
    }
    size = maGetPacketLength(packet);
    if (size <= next->packetSize && (size + next->count) <= next->max) {
        return 1;
    }

    /*
     *  The downstream queue is full, so disable the queue and mark the downstream queue as full and service immediately. 
     */
    maDisableQueue(q);
    next->flags |= MA_QUEUE_FULL;
    return 0;
}


void maSendEndPacket(MaQueue *q)
{
    maPutNext(q, maCreateEndPacket(q->conn));
    q->flags |= MA_QUEUE_EOF;
}


void maSendPackets(MaQueue *q)
{
    MaPacket    *packet;

    for (packet = maGet(q); packet; packet = maGet(q)) {
        maPutNext(q, packet);
    }
}


void maDisableQueue(MaQueue *q)
{
    q->flags |= MA_QUEUE_DISABLED;
}


//  TODO - should this check if the queue is enabled?
void maScheduleQueue(MaQueue *q)
{
    MaQueue     *head;
    
    mprAssert(q->conn);
    head = &q->conn->serviceq;
    
    if (q->scheduleNext == q) {
        q->scheduleNext = head;
        q->schedulePrev = head->schedulePrev;
        head->schedulePrev->scheduleNext = q;
        head->schedulePrev = q;
    }
}


MaQueue *maGetNextQueueForService(MaQueue *q)
{
    MaQueue     *next;
    
    if (q->scheduleNext != q) {
        next = q->scheduleNext;
        next->schedulePrev->scheduleNext = next->scheduleNext;
        next->scheduleNext->schedulePrev = next->schedulePrev;
        next->schedulePrev = next->scheduleNext = next;
        return next;
    }
    return 0;
}


void maInitSchedulerQueue(MaQueue *q)
{
    q->scheduleNext = q;
    q->schedulePrev = q;
}


void maServiceQueue(MaQueue *q)
{
    /*
     *  Since we are servicing the queue, remove it from the service queue if it is at the front of the queue.
     */
    if (q->conn->serviceq.scheduleNext == q) {
        maGetNextQueueForService(&q->conn->serviceq);
    }
    q->service(q);
    q->flags |= MA_QUEUE_SERVICED;
}


void maEnableQueue(MaQueue *q)
{
    q->flags &= ~MA_QUEUE_DISABLED;
    maScheduleQueue(q);
}


/*
 *  Return the number of bytes the queue will accept. Always positive.
 */
int maGetQueueRoom(MaQueue *q)
{
    mprAssert(q->max > 0);
    mprAssert(q->count >= 0);
    
    if (q->count >= q->max) {
        return 0;
    }
    return q->max - q->count;
}


/*
 *  Return true if the packet is too large to be accepted by the downstream queue.
 */
bool maPacketTooBig(MaQueue *q, MaPacket *packet)
{
    int     size;
    
    size = mprGetBufLength(packet->content);
    return size > q->max || size > q->packetSize;
}


/*
 *  Split a packet if required so it fits in the downstream queue. Put back the 2nd portion of the split packet on the queue.
 *  Ensure that the packet is not larger than "size" if it is greater than zero.
 */
int maResizePacket(MaQueue *q, MaPacket *packet, int size)
{
    MaPacket    *tail;
    int         len;
    
    if (size <= 0) {
        size = MAXINT;
    }

    /*
     *  Calculate the size that will fit
     */
    len = packet->content ? mprGetBufLength(packet->content) : packet->count;
    size = min(size, len);
    size = min(size, q->nextQ->max);
    size = min(size, q->nextQ->packetSize);

    if (size == 0) {
        /* Can't fit anything downstream, no point splitting yet */
        return 0;
    }
    if (size == len) {
        return 0;
    }
    
    tail = maSplitPacket(q->conn, packet, size);
    if (tail == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    maPutBack(q, tail);
    return 0;
}


/*
 *  Drain a service queue
 */
static bool drain(MaQueue *q, bool block)
{
    MaConn      *conn;

    conn = q->conn;
    
    q->pending = 0;

    /*
     *  Queue is full. Need to drain the service queue if possible.
     */
    if (block) {
        mprSetSocketBlockingMode(conn->sock, 1);
    }
    maScheduleQueue(q);
    maServiceQueues(conn);
    
    if (block) {
        mprSetSocketBlockingMode(conn->sock, 0);
    
    } else if (q->count >= q->max) {
        /*
         *  Still no room.
         */
        return 0;
    }
    return 1;
}


/*
 *  Write a block of data. This is the lowest level write routine for dynamic data. If block is true, this routine will 
 *  block until all the block is written. If block is false, then it may return without having written all the data.
 *  WARNING: This routine will block if the downstream queue is full. 
 */
int maWriteBlock(MaQueue *q, cchar *buf, int size, bool block)
{
    MaPacket    *packet;
    MaConn      *conn;
    MaResponse  *resp;
    int         bytes, written, packetSize;

    mprAssert(q->stage->flags & MA_STAGE_HANDLER);
               
    if (q->flags & MA_QUEUE_DISABLED) {
        return 0;
    }
    
    conn = q->conn;
    resp = conn->response;
    packetSize = (resp->chunkSize > 0) ? resp->chunkSize : q->max;

    for (written = 0; size > 0; ) {

        if (q->count >= q->max && !drain(q, block)) {
            break;
        }
        if (conn->abandonConnection) {
            return MPR_ERR_CANT_WRITE;
        }

        packet = q->pending;
        if (packet == 0 || mprGetBufSpace(packet->content) == 0) {
            if ((packet = maCreateDataPacket(q->conn, packetSize)) != 0) {
                q->pending = packet;
                maPutForService(q, packet, 1);
            }
        }

        bytes = mprPutBlockToBuf(packet->content, buf, size);

        buf += bytes;
        size -= bytes;
        q->count += bytes;
        packet->count += bytes;
        written += bytes;
    }
    return written;
}


int maWriteString(MaQueue *q, cchar *s)
{
    return maWriteBlock(q, s, (int) strlen(s), 1);
}


int maWrite(MaQueue *q, cchar *fmt, ...)
{
    va_list     vargs;
    char        *buf;
    int         len, rc;
    
    va_start(vargs, fmt);
    len = mprAllocVsprintf(q, &buf, -1, fmt, vargs);
    va_end(vargs);

    rc = maWriteBlock(q, buf, len, 1);
    mprFree(buf);

    return rc;
}


/*
 *  Join two packets by pulling the content from the second into the first.
 */
int maJoinPacket(MaPacket *packet, MaPacket *p)
{
    packet->count += maGetPacketLength(p);
    if (mprPutBlockToBuf(packet->content, mprGetBufStart(p->content), maGetPacketLength(p)) < 0) {
        return MPR_ERR_NO_MEMORY;
    }
    return 0;
}


/*
 *  Split a packet at a given offset and return a new packet containing the data after the offset.
 *  The suffix data migrates to the new packet. 
 */
MaPacket *maSplitPacket(MaConn *conn, MaPacket *orig, int offset)
{
    MaPacket    *packet;

    if (offset >= orig->count) {
        mprAssert(0);
        return 0;
    }

    packet = maCreatePacket(conn, 0);
    if (packet == 0) {
        return 0;
    }
    
    packet->flags = orig->flags;
    packet->count = orig->count - offset;
    orig->count = offset;

    /*
     *  Suffix migrates to the new packet
     */
    if (packet->suffix) {
        packet->suffix = orig->suffix;
        mprStealBlock(packet, packet->suffix);
        orig->suffix = 0;
    }

    if (orig->content && maGetPacketLength(orig) > 0) {
        packet->content = mprCreateBuf(packet, packet->count, -1);
        if (packet->content == 0) {
            mprFree(packet);
            return 0;
        }
        mprAdjustBufEnd(orig->content, -packet->count);
        mprPutBlockToBuf(packet->content, mprGetBufEnd(orig->content), packet->count);
    }
    
    return packet;
}


/*
 *  Remove packets from a queue which do not need to be processed.
 *  Remove data packets if no body is required (HEAD|TRACE|OPTIONS|PUT|DELETE method, not modifed content, or error)
 *  This actually removes and frees the data packets whereas maDiscardData will just flush the data packets.
 */
void maCleanQueue(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;
    MaPacket    *packet, *next, *prev;

    conn = q->conn;
    resp = conn->response;

    if (!(resp->flags & MA_RESP_NO_BODY)) {
        return;
    }

    for (prev = 0, packet = q->first; packet; packet = next) {
        next = packet->next;
        if (packet->flags & (MA_PACKET_RANGE | MA_PACKET_DATA)) {
            if (prev) {
                prev->next = next;
            } else {
                q->first = next;
            }
            q->count -= packet->count;
            mprFree(packet);
            continue;
        }
        prev = packet;
    }
}


/*
 *  Remove all data from non-header packets in the queue. Don't worry about freeing. Will happen automatically at 
 *  the request end. See also maCleanQueue above.
 *  TODO - implement removePackets
 */
void maDiscardData(MaQueue *q, bool removePackets)
{
    MaPacket    *packet;

    if (q->first) {
        /*
         *  Skip the header packet
         */
        if (q->first->flags & MA_PACKET_HEADER) {
            packet = q->first->next;
        } else {
            packet = q->first;
        }

        /*
         *  Just flush each packet. Don't remove so the EOF packet is preserved
         */
        for (; packet; packet = packet->next) {
            q->conn->response->length -= packet->count;
            if (packet->content) {
                mprFlushBuf(packet->content);
            }
            packet->count = 0;
        }
    }
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
/************************************************************************/
/*
 *  End of file "../http/queue.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/request.c"
 */
/************************************************************************/

/*
 *  request.c -- Request class to handle individual HTTP requests.
 *
 *  The Request class is the real work-horse in managing HTTP requests. An instance is created per HTTP request. During
 *  keep-alive it is preserved to process further requests. Requests run in a single thread and do not need multi-thread
 *  locking except for the timeout code which may run on another thread.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */




static void addMatchEtag(MaConn *conn, char *etag);
static int  destroyRequest(MaRequest *req);
static bool getChunkSize(MaConn *conn, MprBuf *buf, int *boundaryLen, int *size);
static char *getToken(MaConn *conn, cchar *delim);
static bool matchEtag(MaConn *conn, char *requestedEtag);
static bool matchModified(MaConn *conn, MprTime time);
static bool parseFirstLine(MaConn *conn, MaPacket *packet);
static bool parseHeaders(MaConn *conn, MaPacket *packet);
static bool parseRange(MaConn *conn, char *value);
static bool parseRequest(MaConn *conn, MaPacket *packet);
static void processChunk(MaConn *conn, MaPacket *packet);
static void processContent(MaConn *conn, MaPacket *packet);
static bool processCompletion(MaConn *conn);
static void setIfModifiedDate(MaConn *conn, MprTime when, bool ifMod);

#if BLD_DEBUG
static void traceContent(MaConn *conn, MaPacket *packet);
#endif


MaRequest *maCreateRequest(MaConn *conn)
{
    MaRequest   *req;
    MprHeap     *arena;

    /*
     *  Create a request memory arena. From this arena, are all allocations made for this entire request.
     *  Arenas are scalable, thread-safe virtual memory blocks that are freed in one chunk.
     */
    arena  = mprAllocArena(conn->arena, "request", MA_REQ_MEM, 0, NULL);
    if (arena == 0) {
        return 0;
    }

    req = mprAllocObjWithDestructorZeroed(arena, MaRequest, destroyRequest);
    if (req == 0) {
        return 0;
    }
    req->conn = conn;
    req->arena = arena;
    req->length = -1;
    req->ifMatch = 1;
    req->ifModified = 1;
    req->host = conn->host;
    req->remainingContent = 0;
    req->method = MA_REQ_GET;

    req->headers = mprCreateHash(req, MA_VAR_HASH_SIZE);

    return req;
}


int destroyRequest(MaRequest *req)
{
    maResetConn(req->conn);
    return 0;
}


/*
 *  Process a write event. These occur when a request could not be completed when it was first received.
 */
void maProcessWriteEvent(MaConn *conn)
{
    mprLog(conn, 6, "maProcessWriteEvent, state %d", conn->state);

    if (unlikely(conn->expire <= conn->time)) {
        /*
         *  Ignore the event if we have expired. TODO - who cleans up?
         */
        return;
    }
    if (conn->response) {
        /*
         *  Enable the queue upstream from the connector
         */
        maEnableQueue(conn->response->queue[MA_QUEUE_SEND].prevQ);
        maServiceQueues(conn);
    }
}


/*
 *  Process incoming requests. This will process as much of the request as possible before returning. All socket I/O is 
 *  non-blocking, and this routine must not block. 
 */
void maProcessReadEvent(MaConn *conn, MaPacket *packet)
{
    mprLog(conn, 6, "maProcessReadEvent, state %d", conn->state);
    conn->canProceed = 1;
    
    while (conn->canProceed) {

        switch (conn->state) {
        case MPR_HTTP_STATE_BEGIN:
            conn->canProceed = parseRequest(conn, packet);
            break;

        case MPR_HTTP_STATE_CONTENT:
            processContent(conn, packet);
            break;

        case MPR_HTTP_STATE_CHUNK:
            processChunk(conn, packet);
            break;

        case MPR_HTTP_STATE_PROCESSING:
            conn->canProceed = maServiceQueues(conn);
            break;

        case MPR_HTTP_STATE_COMPLETE:
            conn->canProceed = processCompletion(conn);
            break;

        default:
            conn->keepAliveCount = 0;
            mprAssert(0);
            return;
        }
    }
}


/*
 *  Parse a new request. Return true to keep going with this or subsequent request, zero means insufficient data to proceed.
 */
static bool parseRequest(MaConn *conn, MaPacket *packet) 
{
    MaRequest   *req;
    char        *start, *end;
    int         len;

    /*
     *  Must wait until we have the complete set of headers.
     */
    if ((len = mprGetBufLength(packet->content)) == 0) {
        return 0;
    }
    if (len >= conn->host->limits->maxHeader) {
        maFailRequest(conn, MPR_HTTP_CODE_REQUEST_TOO_LARGE, "Header too big");
        return 0;
    }
    start = mprGetBufStart(packet->content);
    if ((end = mprStrnstr(start, "\r\n\r\n", len)) == 0) {
        return 0;
    }

    *end = '\0';
    mprLog(conn, 3, "\n@@@ Request =>\n%s\n", start);
    *end = '\r';
        
    if (!parseFirstLine(conn, packet) || !parseHeaders(conn, packet)) {
        conn->keepAliveCount = 0;
        return 0;
    }  
    
    /*
     *  This request now owns the input packet. Must preserve the headers.
     */    
    req = conn->request;
    conn->input = 0;
    mprStealBlock(req, packet);

    /*
     *  Have read the headers. Create the request pipeline.
     */
    maCreatePipeline(conn);

    if (req->remainingContent > 0) {
        conn->state = (req->flags & MA_REQ_CHUNKED) ? MPR_HTTP_STATE_CHUNK : MPR_HTTP_STATE_CONTENT;
    } else {
        /*
         *  Can run the request now if there is no incoming data.
         */
        conn->state = MPR_HTTP_STATE_PROCESSING;
        maRunPipeline(conn);
    }
    return !conn->abandonConnection;
}


/*
 *  Parse the first line of a http request. Return true if the first line parsed. This is only called once all the headers
 *  have been read and buffered.
 */
static bool parseFirstLine(MaConn *conn, MaPacket *packet)
{
    MaRequest   *req;
    MaResponse  *resp;
    char        *uri;
    int         method;

    req = conn->request = maCreateRequest(conn);
    resp = conn->response = maCreateResponse(conn);

    mprLog(req, 4, "New request from %s:%d to %s:%d", conn->remoteIpAddr, conn->remotePort, conn->sock->ipAddr, 
        conn->sock->port);

    req->methodName = getToken(conn, " ");
    if (*req->methodName == '\0') {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad request method name");
        return 0;
    }

    method = 0;
    switch (req->methodName[0]) {
    case 'D':
        if (strcmp(req->methodName, "DELETE") == 0) {
            method = MA_REQ_DELETE;
        }
        break;

    case 'G':
        if (strcmp(req->methodName, "GET") == 0) {
            method = MA_REQ_GET;
        }
        break;

    case 'P':
        if (strcmp(req->methodName, "POST") == 0) {
            method = MA_REQ_POST;

        } else if (strcmp(req->methodName, "PUT") == 0) {
            method = MA_REQ_PUT;
        }
        break;

    case 'H':
        if (strcmp(req->methodName, "HEAD") == 0) {
            method = MA_REQ_HEAD;
            resp->flags |= MA_RESP_NO_BODY;
        }
        break;

    case 'O':
        if (strcmp(req->methodName, "OPTIONS") == 0) {
            method = MA_REQ_OPTIONS;
            resp->flags |= MA_RESP_NO_BODY;
        }
        break;

    case 'T':
        if (strcmp(req->methodName, "TRACE") == 0) {
            method = MA_REQ_TRACE;
            resp->flags |= MA_RESP_NO_BODY;
        }
        break;
    }

    if (method == 0) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_METHOD, "Bad method");
        return 0;
    }

    req->method = method;

    uri = getToken(conn, " ");
    if (*uri == '\0') {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad HTTP request. Bad URI.");
        return 0;
    }

    if ((int) strlen(uri) >= conn->http->limits.maxUrl) {
        maFailConnection(conn, MPR_HTTP_CODE_REQUEST_URL_TOO_LARGE, "Bad request. URI too long.");
        return 0;
    }

    req->httpProtocol = getToken(conn, "\r\n");

    if (strcmp(req->httpProtocol, "HTTP/1.1") == 0) {
        conn->protocol = 1;

    } else if (strcmp(req->httpProtocol, "HTTP/1.0") == 0) {
        conn->keepAliveCount = 0;
        conn->protocol = 0;
        if (method == MA_REQ_POST || method == MA_REQ_PUT) {
            req->remainingContent = MAXINT;
        }

    } else {
        maFailConnection(conn, MPR_HTTP_CODE_NOT_ACCEPTABLE, "Unsupported HTTP protocol");
        return 0;
    }

    mprLog(conn, 2, "%s %s %s", req->methodName, uri, req->httpProtocol);

    if (maSetRequestUri(conn, uri) < 0) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad URL format");
        return 0;
    }
    return 1;
}


/*
 *  Parse the request headers. Return true if the header parsed.
 */
static bool parseHeaders(MaConn *conn, MaPacket *packet)
{
    MaHostAddress   *address;
    MaRequest       *req;
    MaResponse      *resp;
    MaHost          *host, *hp;
    MaLimits        *limits;
    MprBuf          *content;
    char            keyBuf[MPR_MAX_STRING];
    char            *key, *value, *cp, *tok;
    int             count;

    req = conn->request;
    resp = conn->response;
    host = req->host;
    content = packet->content;
    conn->request->headerPacket = packet;
    limits = &conn->http->limits;

    strcpy(keyBuf, "HTTP_");

    //  TODO - BUG can fall off the end. What if start[0] == '\0'
    for (count = 0; content->start[0] != '\r'; count++) {

        if (count >= limits->maxNumHeaders) {
            maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Too many headers");
            return 0;
        }

        if ((key = getToken(conn, ":")) == 0) {
            maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad header format");
            return 0;
        }

        value = getToken(conn, "\r\n");
        while (isspace((int) *value)) {
            value++;
        }
        if (conn->requestFailed) {
            continue;
        }

        //  TODO - should support header continuations

        mprStrUpper(key);
        //  TODO - why do this??? No needed for Ejs anymore
        for (cp = key; *cp; cp++) {
            if (*cp == '-') {
                *cp = '_';
            }
        }

        mprLog(req, 8, "Key %s, value %s", key, value);

        if (strspn(key, "%<>/\\") > 0) {
            maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad header key value");
            continue;
        }

        /*
         *  Define the header with a "HTTP_" prefix
         */
        mprStrcpy(&keyBuf[5], sizeof(keyBuf) - 5, key);
        mprAddDuplicateHash(req->headers, keyBuf, value);

        switch (key[0]) {
        case 'A':
            if (strcmp(key, "AUTHORIZATION") == 0) {
                req->authType = mprStrTok(value, " \t", &tok);
                req->authDetails = tok;

            } else if (strcmp(key, "ACCEPT_CHARSET") == 0) {
                req->acceptCharset = value;

            } else if (strcmp(key, "ACCEPT") == 0) {
                req->accept = value;

            } else if (strcmp(key, "ACCEPT_ENCODING") == 0) {
                req->acceptEncoding = value;
            }
            break;

        case 'C':
            if (strcmp(key, "CONTENT_LENGTH") == 0) {
                if (req->length >= 0) {
                    maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Mulitple content length headers");
                    continue;
                }
                req->length = atoi(value);
                if (req->length < 0) {
                    maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad content length");
                    continue;
                }
                if (req->length >= host->limits->maxBody) {
                    maFailConnection(conn, MPR_HTTP_CODE_REQUEST_TOO_LARGE, 
                        "Request content length %d is too big. Limit %d", req->length, host->limits->maxBody);
                    continue;
                }
                mprAssert(req->length >= 0);
                req->remainingContent = req->length;
                req->contentLengthStr = value;

            } else if (strcmp(key, "CONTENT_RANGE") == 0) {
                /*
                 *  This headers specifies the range of any posted body data
                 *  Format is:  Content-Range: bytes n1-n2/length
                 *  Where n1 is first byte pos and n2 is last byte pos
                 */
                char    *sp;
                int     start, end, size;

                start = end = size = -1;

                sp = value;
                while (*sp && !isdigit((int) *sp)) {
                    sp++;
                }
                if (*sp) {
                    start = mprAtoi(sp, 10);

                    if ((sp = strchr(sp, '-')) != 0) {
                        end = mprAtoi(++sp, 10);
                    }
                    if ((sp = strchr(sp, '/')) != 0) {
                        /*
                         *  Note this is not the content length transmitted, but the original size of the input of which 
                         *  the client is transmitting only a portion.
                         */
                        size = mprAtoi(++sp, 10);
                    }
                }
                if (start < 0 || end < 0 || size < 0 || end <= start) {
                    maFailRequest(conn, MPR_HTTP_CODE_RANGE_NOT_SATISFIABLE, "Bad content range");
                    continue;
                }
                req->inputRange = maCreateRange(conn, start, end);

            } else if (strcmp(key, "CONTENT_TYPE") == 0) {
                req->mimeType = value;

            } else if (strcmp(key, "COOKIE") == 0) {
                if (req->cookie && *req->cookie) {
                    mprAllocStrcat(req, &req->cookie, -1, 0, req->cookie, "; ", value, 0);
                } else {
                    req->cookie = value;
                }

            } else if (strcmp(key, "CONNECTION") == 0) {
                req->connection = value;
                if (mprStrcmpAnyCase(value, "KEEP_ALIVE") == 0) {
                    /* Nothing to do */
                    ;

                } else if (mprStrcmpAnyCase(value, "CLOSE") == 0) {
                    conn->keepAliveCount = 0;
                }
                if (!host->keepAlive) {
                    conn->keepAliveCount = 0;
                }
            }
            break;

        case 'F':
            req->forwarded = value;
            break;

        case 'H':
            if (strcmp(key, "HOST") == 0) {
                req->hostName = value;
                address = conn->address;
                if (maIsNamedVirtualHostAddress(address)) {
                    hp = maLookupVirtualHost(address, value);
                    if (hp == 0) {
                        maFailRequest(conn, 404, "No host to serve request. Searching for %s", value);
                        continue;
                    }
                    req->host = hp;
#if UNUSED && TODO
                    /*
                     *  Reassign this request to a new host
                     */
                    host->removeRequest(this);
                    host = hp;
                    host->insertRequest(this);
#endif
                }
            }
            break;

        case 'I':
            if ((strcmp(key, "IF_MODIFIED_SINCE") == 0) || (strcmp(key, "IF_UNMODIFIED_SINCE") == 0)) {
                MprTime     newDate = 0;
                char        *cp;
                bool        ifModified = (key[3] == 'M');

                if ((cp = strchr(value, ';')) != 0) {
                    *cp = '\0';
                }
                if (mprParseTime(conn, &newDate, value) < 0) {
                    mprAssert(0);
                    break;
                }
                if (newDate) {
                    setIfModifiedDate(conn, newDate, ifModified);
                    req->flags |= MA_REQ_IF_MODIFIED;
                }

            } else if ((strcmp(key, "IF_MATCH") == 0) || (strcmp(key, "IF_NONE_MATCH") == 0)) {
                char    *word, *tok;
                bool    ifMatch = key[3] == 'M';

                if ((tok = strchr(value, ';')) != 0) {
                    *tok = '\0';
                }

                req->ifMatch = ifMatch;
                req->flags |= MA_REQ_IF_MODIFIED;

                value = mprStrdup(conn, value);
                word = mprStrTok(value, " ,", &tok);
                while (word) {
                    addMatchEtag(conn, word);
                    word = mprStrTok(0, " ,", &tok);
                }
                mprFree(value);

            } else if (strcmp(key, "IF_RANGE") == 0) {
                char    *word, *tok;

                if ((tok = strchr(value, ';')) != 0) {
                    *tok = '\0';
                }

                req->ifMatch = 1;
                req->flags |= MA_REQ_IF_MODIFIED;

                value = mprStrdup(conn, value);
                word = mprStrTok(value, " ,", &tok);
                while (word) {
                    addMatchEtag(conn, word);
                    word = mprStrTok(0, " ,", &tok);
                }
                mprFree(value);
            }
            break;

        case 'P':
            if (strcmp(key, "PRAGMA") == 0) {
                req->pragma = value;
            }
            break;

        case 'R':
            if (strcmp(key, "RANGE") == 0) {
                if (!parseRange(conn, value)) {
                    maFailRequest(conn, MPR_HTTP_CODE_RANGE_NOT_SATISFIABLE, "Bad range");
                }
            } else if (strcmp(key, "REFERER") == 0) {
                req->referer = value;
            }
            break;

        case 'T':
            if (strcmp(key, "TRANSFER_ENCODING") == 0) {
                mprStrLower(value);
                if (strcmp(value, "chunked") == 0) {
                    req->flags |= MA_REQ_CHUNKED;
                }
            }
            break;
        
#if BLD_DEBUG
        case 'X':
            if (strcmp(key, "X_APPWEB_CHUNK_SIZE") == 0) {
                mprStrUpper(value);
                resp->chunkSize = atoi(value);
                if (resp->chunkSize <= 0) {
                    resp->chunkSize = 0;
                } else if (resp->chunkSize > conn->http->limits.maxChunkSize) {
                    resp->chunkSize = conn->http->limits.maxChunkSize;
                }
            }
            break;
#endif

        case 'U':
            if (strcmp(key, "USER_AGENT") == 0) {
                req->userAgent = value;
            }
            break;
        }
    }
    mprAdjustBufStart(content, 2);

    maMatchHandler(conn);
    
    return 1;
}


/*
 *  Process post or put content data. Packet will be null if the client closed the connection to signify end of data.
 */
static void processContent(MaConn *conn, MaPacket *packet)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaQueue         *q;
    MprBuf          *content;
    int             nbytes;


    req = conn->request;
    resp = conn->response;
    q = &resp->queue[MA_QUEUE_RECEIVE];

    if (conn->requestFailed) {
        conn->canProceed = 0;
        mprFree(packet);
        return;
    }

    /*
     *  Transfer ownership of the packet. If it contains header data for the next pipelined request, it will be split below.
     */
    mprStealBlock(resp, packet);
    conn->input = 0;

    /*
     *  Packet will be null if the client closed the connection to signify end of data
     */
    if (packet) {
        content = packet->content;
        nbytes = min(req->remainingContent, mprGetBufLength(content));
#if BLD_DEBUG
        if (mprGetLogLevel(conn) >= 5) {
            traceContent(conn, packet);
        }
#endif
        if (nbytes > 0) {
            mprAssert(packet->count > 0);
            req->remainingContent -= nbytes;
            req->receivedContent += nbytes;

            if (req->receivedContent >= conn->host->limits->maxBody) {
                conn->keepAliveCount = 0;
                maFailConnection(conn, MPR_HTTP_CODE_REQUEST_TOO_LARGE, 
                    "Request content body is too big %d vs limit %d", 
                    req->receivedContent, conn->host->limits->maxBody);
                return;
            } 

            if (req->remainingContent == 0 && mprGetBufLength(packet->content) > nbytes) {
                /*
                 *  Looks like this packet contains the header of the next request. Split the packet and put back
                 *  the next request header onto the connection input queue.
                 */
                conn->input = maSplitPacket(conn, packet, mprGetBufLength(content) - nbytes);
            }
            if ((q->count + packet->count) > q->max) {
                conn->keepAliveCount = 0;
                maFailConnection(q->conn, MPR_HTTP_CODE_REQUEST_TOO_LARGE, "Too much body data");
                return;
            }
            if (!conn->requestFailed) {
                packet->count = mprGetBufLength(packet->content);
                maPutNext(q, packet);
            }
        }
    }

    if (packet == 0 || req->remainingContent == 0) {
        /*
         *  End of input. Send a zero packet EOF signal and enable the handler send queue.
         */
        if (req->remainingContent > 0 && conn->protocol > 0) {
            maFailConnection(conn, MPR_HTTP_CODE_COMMS_ERROR, "Insufficient content data sent with request");

        } else {
            maPutNext(q, maCreateEndPacket(conn));
            conn->state = MPR_HTTP_STATE_PROCESSING;
            maRunPipeline(conn);
        }

    } else {
        conn->canProceed = 0;
        maServiceQueues(conn);
    }
}


/*
 *  Complete the request and return true if there is a pipelined request following
 */
static bool processCompletion(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    MaPacket    *packet;
    bool        more;

    req = conn->request;
    resp = conn->response;

    maLogRequest(conn);
    maCloseStage(conn);

#if BLD_DEBUG
    mprAssert((conn->arena->allocBytes / 1024) < 20000);
    mprLog(req, 7, "Request complete used %,d K, conn usage %,d K, mpr usage %,d K, page usage %,d K", 
        req->arena->allocBytes / 1024, conn->arena->allocBytes / 1024, mprGetMpr(conn)->heap.allocBytes / 1024, 
        mprGetMpr(conn)->pageHeap.allocBytes / 1024);
//  mprPrintAllocReport(mprGetMpr(conn), "Before completing request");
#endif

    packet = conn->input;
    more = packet && (mprGetBufLength(packet->content) > 0);
    mprAssert(!more || mprGetParent(packet) == conn);

    /*
     *  This will free the request and response and cause maResetConn to run which will reset the state and cleanse the conn
     *  The connection will not be closed unless keepAliveCount is zero when returning to ioEvent in conn.c 
     */
    mprFree(req->arena);

    return more;
}


static void processChunk(MaConn *conn, MaPacket *packet)
{
    MaRequest       *req;
    MprBuf          *content;
    int             nbytes, boundaryLen;

    req = conn->request;

    if (packet == 0) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Connection closed before request fully read");
        return;
    }
    content = packet->content;
    nbytes = min(req->remainingContent, mprGetBufLength(content));

    if (!getChunkSize(conn, content, &boundaryLen, &req->remainingChunk)) {
        return;
    }

    mprAdjustBufStart(content, boundaryLen);

    if (req->remainingChunk == 0) {
        /*
         *  Received all the data. Discard last "\r\n" 
         */
        mprAdjustBufEnd(content, 2);
        conn->state = MPR_HTTP_STATE_PROCESSING;
        maRunPipeline(conn);
        
    } else {
        conn->state = MPR_HTTP_STATE_CONTENT;
    }
}


#if BLD_DEBUG
static void traceContent(MaConn *conn, MaPacket *packet)
{
    MprBuf      *content;
    char        *data, *buf;
    int         len;

    content = packet->content;

    len = mprGetBufLength(content);
    buf = mprGetBufStart(content);

    data = mprAlloc(conn, len + 1);
    memcpy(data, buf, len);
    data[len] = '\0';
    mprRawLog(conn, 5, "@@@ Content =>\n%s\n", data);
    mprFree(data);
}
#endif


static bool getChunkSize(MaConn *conn, MprBuf *buf, int *boundaryLen, int *size)
{
    char    *start, *nextContent;
    int     available;

    /*
     *  Must at least have "\r\nDIGIT\r\n"
     */
    start = mprGetBufStart(buf), 
    available = (int) (mprGetBufEnd(buf) - start);
    mprAssert(available >= 0);

    if (available < 5) {
        conn->canProceed = 0;
        return 0;
    }

    /*
     *  Chunk delimiter is: "\r\nHEX_COUNT; chunk length DECIMAL_COUNT\r\n". The "; chunk length DECIMAL_COUNT is optional.
     *  Step over the existing content and get the next chunk count.
     */
    if (start[0] != '\r' || start[1] != '\n') {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad chunk specification");
        return 0;
    }
    nextContent = mprStrnstr(start + 2, "\r\n", available);
    if (nextContent == 0) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad chunk specification");
        return 0;
    }
    nextContent += 2;

    *boundaryLen = (int) (nextContent - start);
    if (*boundaryLen < 0 || *boundaryLen > 256) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad boundary length");
        return 0;
    }

    *size = mprAtoi(start + 2, 16);
    if (*size < 0) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad chunk size");
        return 0;
    }
    return 1;
}


void maFailRequest(MaConn *conn, int code, cchar *fmt, ...)
{
    MaResponse  *resp;
    MaRequest   *req;
    va_list     args;
    cchar       *url, *status;
    char        *msg, *filename;

    mprAssert(fmt);
    
    if (conn->requestFailed) {
        return;
    }

    req = conn->request;
    resp = conn->response;
    resp->code = code;

    msg = 0;
    va_start(args, fmt);
    mprAllocVsprintf(resp, &msg, MA_BUFSIZE, fmt, args);
    va_end(args);

    //  TODO - should we only do one error per request?
  
    if (resp == 0 || req == 0) {
        mprLog(resp, 2, "\"%s\", code %d: %s.", mprGetHttpCodeString(conn, code), code, msg);

    } else {
        filename = resp->filename ? resp->filename : 0;

        if (code != 711) {
            mprLog(resp, 2, "Error: \"%s\", code %d for URI \"%s\", file \"%s\": %s.", 
                mprGetHttpCodeString(conn, code), code, req->url ? req->url : "", filename ? filename : "", msg);
        }

        /*
         *  Use an error document rather than standard error boilerplate.
         */
        if (req->location) {
            url = maLookupErrorDocument(req->location, code);
            if (url && *url) {
                maRedirect(conn, 302, url);
                return;
            }
        }

        /*
         *  If the headers have already been filled, this alternate response body will be ignored.
         */
        if (resp->altBody == 0) {
            status = mprGetHttpCodeString(conn, code);
            /*
             *  For security, do not emit the "msg" value. 
             */
            mprAllocSprintf(conn->response, &resp->altBody, -1, 
                "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
                "<html><head><title>Document Error: %s</title></head>\r\n"
                "<body><h2>Access Error: %d -- %s</h2></body>\r\n</html>\r\n",
                status, code, status);
        }
    }
    mprFree(msg);

    resp->flags |= MA_RESP_NO_BODY;
    conn->requestFailed = 1;
}


/*
 *  Stop all requests on the current connection. Fail the current request and the processing pipeline. Force a connection 
 *  closure as in most cases, the client is seriously messed up.
 */
void maFailConnection(MaConn *conn, int code, cchar *fmt, ...)
{
    va_list     args;
    char        *msg;

    mprAssert(fmt);

    conn->state = MPR_HTTP_STATE_COMPLETE;
    conn->canProceed = 0;

    if (conn->abandonConnection) {
        return;
    }
    conn->abandonConnection = 1;

    va_start(args, fmt);
    msg = 0;
    mprAllocVsprintf(conn->response, &msg, MA_BUFSIZE, fmt, args);
    va_end(args);

    maFailRequest(conn, code, "%s", msg);
    mprFree(msg);

    conn->keepAliveCount = 0;
    maDiscardPipeData(conn);
}


//  TODO - reorder the routines to the end of the file
int maSetRequestUri(MaConn *conn, cchar *uri)
{
    MaRequest   *req;
    int         len;

    req = conn->request;

    /*
     *  We parse (tokenize) the request uri first. Then we decode and lastly we validate the URI path portion.
     *  This allows URLs to have '?' in the URL. We descape and validate insitu.
     */
    req->parsedUri = mprParseUri(req, uri);
    if (req->parsedUri == 0) {
        return MPR_ERR_BAD_ARGS;
    }
    conn->response->extension = req->parsedUri->ext;

    /*
     *  Decode the URI path portion
     *  TODO - is this right? What about decoding the other parts of the URI
     */
    req->url = mprStrdup(req, req->parsedUri->url);
    len = (int) strlen(req->url);
    if (mprUrlDecode(req->url, len + 1, req->url) == 0) {
        return MPR_ERR_BAD_ARGS;
    }

    //  TODO - need to do case insensitive testing on ext and maybe other places See MprFileInfo.caseMatters

    if (mprValidateUrl(req->url) == 0) {
        return MPR_ERR_BAD_ARGS;
    }
    return 0;
}


/*
 *  Format is:  Range: bytes=n1-n2,n3-n4,...
 *  Where n1 is first byte pos and n2 is last byte pos
 *
 *  Examples:
 *      Range: 0-49             first 50 bytes
 *      Range: 50-99,200-249    Two 50 byte ranges from 50 and 200
 *      Range: -50              Last 50 bytes
 *      Range: 1-               Skip first byte then emit the rest
 *
 *  Return 1 if more ranges, 0 if end of ranges, -1 if bad range.
 */
static bool parseRange(MaConn *conn, char *value)
{
    MaRequest   *req;
    MaResponse  *resp;
    MaRange     *range, *last, *next;
    char        *tok, *ep;

    req = conn->request;
    resp = conn->response;

    value = mprStrdup(conn, value);
    if (value == 0) {
        return 0;
    }

    /*
     *  Step over the "bytes="
     */
    tok = mprStrTok(value, "=", &value);

    for (last = 0; value && *value; ) {
        range = mprAllocObjZeroed(req, MaRange);
        if (range == 0) {
            return 0;
        }

        /*
         *  A range "-7" will set the start to -1 and end to 8
         */
        tok = mprStrTok(value, ",", &value);
        if (*tok != '-') {
            range->start = mprAtoi(tok, 10);
        } else {
            range->start = -1;
        }
        range->end = -1;

        if ((ep = strchr(tok, '-')) != 0) {
            if (*++ep != '\0') {
                /*
                 *  End is one beyond the range. Makes the math easier.
                 */
                range->end = mprAtoi(ep, 10) + 1;
            }
        }
        if (range->start >= 0 && range->end >= 0) {
            range->len = range->end - range->start;
        }
        if (last == 0) {
            req->ranges = range;
        } else {
            last->next = range;
        }
        last = range;
    }

    /*
     *  Validate ranges
     */
    for (range = req->ranges; range; range = range->next) {
        if (range->end != -1 && range->start >= range->end) {
            return 0;
        }
        if (range->start < 0 && range->end < 0) {
            return 0;
        }
        next = range->next;
        if (range->start < 0 && next) {
            /* This range goes to the end, so can't have another range afterwards */
            return 0;
        }
        if (next) {
            if (next->start >= 0 && range->end > next->start) {
                return 0;
            }
        }
    }
    resp->currentRange = req->ranges;

    return (last) ? 1: 0;
}


/*
 *  Called by connectors to complete a request
 */
void maCompleteRequest(MaConn *conn)
{
    conn->state = MPR_HTTP_STATE_COMPLETE;
}


/*
 *  Connector is write blocked and can't proceed
 */
void maRequestWriteBlocked(MaConn *conn)
{
    conn->canProceed = 0;
}


void maSetNoKeepAlive(MaConn *conn)
{
    conn->keepAliveCount = 0;
}


//  TODO - move and 
bool maContentNotModified(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    bool        same;

    req = conn->request;
    resp = conn->response;

    if (req->flags & MA_REQ_IF_MODIFIED) {
        /*
         *  If both checks, the last modification time and etag, claim that the request doesn't need to be
         *  performed, skip the transfer.
         */
        //  TODO - need to check if fileInfo is actually set.
        same = matchModified(conn, resp->fileInfo.mtime) && matchEtag(conn, resp->etag);

        if (req->ranges && !same) {
            /*
             *  Need to transfer the entire resource
             */
            mprFree(req->ranges);
            req->ranges = 0;
        }
        return same;
    }
    return 0;
}


MaRange *maCreateRange(MaConn *conn, int start, int end)
{
    MaRange     *range;

    range = mprAllocObjZeroed(conn->request, MaRange);
    if (range == 0) {
        return 0;
    }
    range->start = start;
    range->end = end;
    range->len = end - start;

    return range;
}


static void addMatchEtag(MaConn *conn, char *etag)
{
    MaRequest   *req;

    req = conn->request;

    if (req->etags == 0) {
        req->etags = mprCreateList(req);
    }
    mprAddItem(req->etags, etag);
}


/*
 *  Return TRUE if the client's cached copy matches an entity's etag.
 */
//  TODO was RequestMatch::matches

static bool matchEtag(MaConn *conn, char *requestedEtag)
{
    MaRequest   *req;
    char        *tag;
    int         next;

    req = conn->request;

    if (req->etags == 0) {
        return 1;
    }
    if (requestedEtag == 0) {
        return 0;
    }

    for (next = 0; (tag = mprGetNextItem(req->etags, &next)) != 0; ) {
        if (strcmp(tag, requestedEtag) == 0) {
            return (req->ifMatch) ? 0 : 1;
        }
    }
    return (req->ifMatch) ? 1 : 0;
}


/*
 *  If an IF-MODIFIED-SINCE was specified, then return true if the resource has not been modified. If using
 *  IF-UNMODIFIED, then return true if the resource was modified.
 */
static bool matchModified(MaConn *conn, MprTime time)
{
    MaRequest   *req;

    req = conn->request;

    if (req->since == 0) {
        /*  If-Modified or UnModified not supplied. */
        return 1;
    }

    if (req->ifModified) {
        /*
         *  Return true if the file has not been modified.
         */
        return !(time > req->since);

    } else {
        /*
         *  Return true if the file has been modified.
         */
        return (time > req->since);
    }
}


static void setIfModifiedDate(MaConn *conn, MprTime when, bool ifMod)
{
    MaRequest   *req;

    req = conn->request;
    req->since = when;
    req->ifModified = ifMod;
}


/*
 *  Get the next input token. The content buffer is advanced to the next token. This routine always returns a non-zero token. 
 *  The empty string means the delimiter was not found.
 */
static char *getToken(MaConn *conn, cchar *delim)
{
    MprBuf  *buf;
    char    *token, *nextToken;
    int     len;

    buf = conn->input->content;
    len = mprGetBufLength(buf);
    if (len == 0) {
        return "";
    }

    token = mprGetBufStart(buf);
    nextToken = mprStrnstr(mprGetBufStart(buf), delim, len);
    if (nextToken) {
        *nextToken = '\0';
        len = (int) strlen(delim);
        nextToken += len;
        buf->start = (uchar*) nextToken;

    } else {
        buf->start = (uchar*) mprGetBufEnd(buf);
    }
    return token;
}


cchar *maGetCookies(MaConn *conn)
{
    return conn->request->cookie;
}


//  TODO - move
void maSetRequestUser(MaConn *conn, cchar *user)
{
    MaRequest   *req;

    req = conn->request;

    mprFree(req->user);
    req->user = mprStrdup(conn->request, user);
}


//  TODO - move
void maSetRequestGroup(MaConn *conn, cchar *group)
{
    MaRequest   *req;

    req = conn->request;

    mprFree(req->group);
    req->group = mprStrdup(conn->request, group);
}



cchar *maGetQueryString(MaConn *conn)
{
    MaRequest   *req;

    req = conn->request;

    return conn->request->parsedUri->query;
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
/************************************************************************/
/*
 *  End of file "../http/request.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/response.c"
 */
/************************************************************************/

/*
 *  response.c
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */




static int destroyResponse(MaResponse *resp);
static void putFormattedHeader(MaPacket *packet, cchar *key, cchar *fmt, ...);
static void putHeader(MaPacket *packet, cchar *key, cchar *value);


MaResponse *maCreateResponse(MaConn *conn)
{
    MaResponse  *resp;
    MaHttp      *http;

    http = conn->http;

    resp = mprAllocObjWithDestructorZeroed(conn->request->arena, MaResponse, destroyResponse);
    if (resp == 0) {
        return 0;
    }

    resp->conn = conn;
    resp->code = MPR_HTTP_CODE_OK;
    resp->mimeType = "text/html";
    resp->handler = http->passHandler;
    resp->length = -1;
    resp->entityLength = -1;
    resp->chunkSize = -1;

    resp->headers = mprCreateHash(resp, MA_HEADER_HASH_SIZE);

    maInitQueue(http, &resp->queue[MA_QUEUE_SEND], "responseSendHead");
    maInitQueue(http, &resp->queue[MA_QUEUE_RECEIVE], "responseReceiveHead");

    return resp;
}


static int destroyResponse(MaResponse *resp)
{
    MaConn      *conn;

    conn = resp->conn;
    mprLog(conn, 5, "destroyResponse");
    maCloseStage(conn);

    return 0;
}


/*
 *  TODO OPT - how to speed this whole routine up. Can some of this be pre-computed?
 */
void maFillHeaders(MaConn *conn, MaPacket *packet)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaHost          *host;
    MaRange         *range;
    MprHash         *hp;
    MprBuf          *buf;

    mprAssert(packet->flags == MA_PACKET_HEADER);

    req = conn->request;
    resp = conn->response;
    host = req->host;
    buf = packet->content;

    if (resp->flags & MA_RESP_HEADERS_CREATED) {
        return;
    }    
    if (req->method ==  MA_REQ_TRACE || req->method == MA_REQ_OPTIONS) {
        maTraceOptions(conn);
    }

    mprPutStringToBuf(buf, req->httpProtocol);
    mprPutCharToBuf(buf, ' ');
    mprPutIntToBuf(buf, resp->code);
    mprPutCharToBuf(buf, ' ');
    mprPutStringToBuf(buf, mprGetHttpCodeString(resp, resp->code));
    mprPutStringToBuf(buf, "\r\n");

    mprLog(conn, 2, "    => %s %d %s", req->httpProtocol, resp->code, mprGetHttpCodeString(resp, resp->code));

    putHeader(packet, "Date", req->host->currentDate);
    putHeader(packet, "Server", MA_SERVER_NAME);

    if (resp->flags & MA_RESP_DONT_CACHE) {
        putHeader(packet, "Cache-Control", "no-cache");
    }

    if (resp->etag) {
        putFormattedHeader(packet, "ETag", "%s", resp->etag);
    }

    if (resp->altBody) {
        resp->length = (int) strlen(resp->altBody);
    }

    if (resp->chunkSize > 0) {
        if (!(req->method & MA_REQ_HEAD)) {
            maSetHeader(conn, 0, "Transfer-Encoding", "chunked");
        }

    } else if (resp->length > 0) {
        putFormattedHeader(packet, "Content-Length", "%d", resp->length);
    }

    if (req->ranges) {
        if (req->ranges->next == 0) {
            range = req->ranges;
            if (resp->entityLength > 0) {
                putFormattedHeader(packet, "Content-Range", "bytes %d-%d/%d", range->start, range->end, resp->entityLength);
            } else {
                putFormattedHeader(packet, "Content-Range", "bytes %d-%d/*", range->start, range->end);
            }
        } else {
            putFormattedHeader(packet, "Content-Type", "multipart/byteranges; boundary=%s", resp->rangeBoundary);
        }
        putHeader(packet, "Accept-Ranges", "bytes");

        //  TODO - does not look right
    } else if (resp->code != MPR_HTTP_CODE_MOVED_TEMPORARILY) {
        putHeader(packet, "Content-Type", (resp->mimeType) ? resp->mimeType : "text/html");
    }

    if (conn->keepAliveCount-- > 0) {
        putHeader(packet, "Connection", "keep-alive");
        putFormattedHeader(packet, "Keep-Alive", "timeout=%d, max=%d", host->keepAliveTimeout / 1000, conn->keepAliveCount);
    } else {
        putHeader(packet, "Connection", "close");
    }

    /*
     *  Output any remaining custom headers
     */
    hp = mprGetFirstHash(resp->headers);
    while (hp) {
        putHeader(packet, hp->key, hp->data);
        hp = mprGetNextHash(resp->headers, hp);
    }

    if (resp->chunkSize <= 0 || resp->altBody) {
        mprPutStringToBuf(buf, "\r\n");
    }
    if (resp->altBody) {
        mprPutStringToBuf(buf, resp->altBody);
        maDiscardData(resp->queue[MA_QUEUE_SEND].nextQ, 0);
    }
    packet->count = mprGetBufLength(buf);
    resp->headerSize = packet->count;

    resp->flags |= MA_RESP_HEADERS_CREATED;

    mprLog(conn, 3, "\n@@@ Response => \n%s", mprGetBufStart(buf));
}


void maTraceOptions(MaConn *conn)
{
    MaResponse  *resp;
    MaRequest   *req;
    int         flags;

    if (conn->requestFailed) {
        return;
    }
    resp = conn->response;
    req = conn->request;

    if (req->method & MA_REQ_TRACE) {
        if (req->host->flags & MA_HOST_NO_TRACE) {
            resp->code = MPR_HTTP_CODE_NOT_ACCEPTABLE;
            maFormatBody(conn, "Trace Request Denied", "<p>The TRACE method is disabled on this server.</p>");
        } else {
            mprAllocSprintf(resp, &resp->altBody, -1, "%s %s %s\r\n", req->methodName, req->parsedUri->originalUri, 
                req->httpProtocol);
        }

    } else if (req->method & MA_REQ_OPTIONS) {

        if (resp->handler == 0) {
            maSetHeader(conn, 0, "Allow", "OPTIONS,TRACE");

        } else {
            flags = resp->handler->flags;
            maSetHeader(conn, 0, "Allow", "OPTIONS,TRACE%s%s%s",
                (flags & MA_STAGE_GET) ? ",GET" : "",
                (flags & MA_STAGE_HEAD) ? ",HEAD" : "",
                (flags & MA_STAGE_POST) ? ",POST" : "",
                (flags & MA_STAGE_PUT) ? ",PUT" : "",
                (flags & MA_STAGE_DELETE) ? ",DELETE" : "");
        }
        resp->length = 0;
    }
}


static void putFormattedHeader(MaPacket *packet, cchar *key, cchar *fmt, ...)
{
    va_list     args;
    char        *value;

    va_start(args, fmt);
    mprAllocVsprintf(packet, &value, MA_MAX_HEADERS, fmt, args);
    va_end(args);

    putHeader(packet, key, value);
    mprFree(value);
}


static void putHeader(MaPacket *packet, cchar *key, cchar *value)
{
    MaResponse  *resp;
    MprBuf      *buf;

    buf = packet->content;
    resp = packet->conn->response;

    mprPutStringToBuf(buf, key);
    mprPutStringToBuf(buf, ": ");
    if (value) {
        mprPutStringToBuf(buf, value);
    }
    mprPutStringToBuf(buf, "\r\n");
}


int maFormatBody(MaConn *conn, cchar *title, cchar *fmt, ...)
{
    MaResponse  *resp;
    va_list     args;
    char        *body;
    int         len;

    resp = conn->response;
    mprAssert(resp->altBody == 0);

    va_start(args, fmt);

    //  TODO - should this string be HTML encoded?
    mprAllocVsprintf(resp, &body, MA_MAX_HEADERS, fmt, args);

    len = mprAllocSprintf(resp, &resp->altBody, -1,
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
        "<html><head><title>%s</title></head>\r\n"
        "<body>\r\n%s\r\n</body>\r\n</html>\r\n",
        title, body);
    mprFree(body);
    va_end(args);
    return len;
}


/*
 *  Redirect the user to another web page. The targetUri may or may not have a scheme.
 */
void maRedirect(MaConn *conn, int code, cchar *targetUri)
{
    MaResponse  *resp;
    MaRequest   *req;
    MaHost      *host;
    MprUri      *target, *prev;
    char        path[MA_MAX_URL];
    char        *uri, *dir, *cp;

    mprAssert(targetUri);

    req = conn->request;
    resp = conn->response;
    host = req->host;

    mprLog(conn, 3, "redirect %d %s", code, targetUri);

    uri = 0;
    resp->code = code;

    prev = req->parsedUri;
    target = mprParseUri(resp, targetUri);

    if (strstr(targetUri, "://") == 0) {
        /*
         *  Use host->name to get the real ServerName from the config file
         */
        if (target->url[0] == '/') {
            uri = mprFormatUri(resp, prev->scheme, host->name, prev->port, target->url, target->query);

        } else {
            /*
             *  Redirection to a file in the same directory as the previous request.
             */
            dir = mprStrdup(resp, req->url);
            if ((cp = strrchr(dir, '/')) != 0) {
                *cp = '\0';
            }
            mprSprintf(path, sizeof(path), "%s/%s", dir, target->url);
            uri = mprFormatUri(resp, prev->scheme, host->name, prev->port, path, target->query);
        }
        targetUri = uri;
    }

    maSetHeader(conn, 0, "Location", "%s", targetUri);
    mprAssert(resp->altBody == 0);
    mprAllocSprintf(resp, &resp->altBody, -1,
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
        "<html><head><title>%s</title></head>\r\n"
        "<body><h1>%s</h1>\r\n</H1>\r\n<p>The document has moved <a href=\"%s\">here</a>.</p>\r\n"
        "<address>%s at %s Port %d</address></body>\r\n</html>\r\n",
        mprGetHttpCodeString(conn, code), mprGetHttpCodeString(conn, code), targetUri,
        MA_SERVER_NAME, host->name, prev->port);

    mprFree(uri);

    /*
     *  Pretent the request failed. This will prevent further processing of the pipeline.
     */
    conn->requestFailed = 1;
}


void maDontCacheResponse(MaConn *conn)
{
    conn->response->flags |= MA_RESP_DONT_CACHE;
}


void maSetHeader(MaConn *conn, bool allowMultiple, cchar *key, cchar *fmt, ...)
{
    MaResponse      *resp;
    char            *value;
    va_list         vargs;

    resp = conn->response;

    va_start(vargs, fmt);
    mprAllocVsprintf(resp, &value, MA_MAX_HEADERS, fmt, vargs);

    if (allowMultiple) {
        mprAddDuplicateHash(resp->headers, key, value);
    } else {
        //  TODO - this should be doing case mapping on the key
        mprAddHash(resp->headers, key, value);
    }
}


void maSetCookie(MaConn *conn, cchar *name, cchar *value, int lifetime, cchar *path, bool secure)
{
    MaResponse  *resp;
    struct tm   tm;
    time_t      when;
    char        dateStr[64];

    resp = conn->response;

    if (path == 0) {
        path = "/";
    }

    if (lifetime > 0) {
        when = conn->time + lifetime * MPR_TICKS_PER_SEC;
        mprGmtime(resp, &tm, when);
        mprRfctime(resp, dateStr, sizeof(dateStr), &tm);

        /*
         *  Other keywords:
         *      Domain=%s
         */
        maSetHeader(conn, 1, "Set-Cookie", "%s=%s; path=%s; Expires=%s;%s", name, value, path, dateStr, 
            secure ? " secure" : "");

    } else {
        maSetHeader(conn, 1, "Set-Cookie", "%s=%s; path=%s;%s", name, value, path, secure ? " secure" : "");
    }
    maSetHeader(conn, 0, "Cache-control", "no-cache=\"set-cookie\"");
}


void maSetEntityLength(MaConn *conn, int len)
{
    MaRequest       *req;
    MaResponse      *resp;

    resp = conn->response;
    req = conn->request;

    resp->entityLength = len;
    if (req->ranges == 0) {
        resp->length = len;
    }
}


void maSetResponseCode(MaConn *conn, int code)
{
    conn->response->code = code;
}


void maSetResponseMimeType(MaConn *conn, cchar *mimeType)
{
    MaResponse      *resp;

    resp = conn->response;

    /*  Can't free old mime type, as it is sometimes a literal constant */
    resp->mimeType = mprStrdup(resp, mimeType);
}


void maOmitResponseBody(MaConn *conn)
{
    if (conn->response) {
        conn->response->flags |= MA_RESP_NO_BODY;
    }
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
/************************************************************************/
/*
 *  End of file "../http/response.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/server.c"
 */
/************************************************************************/

/*
 *  server.c -- Server Class to manage a single server
 *
 *  An instance of the MaServer Class may be created for each http.conf file. Each server can manage multiple hosts
 *  (standard, virtual or SSL). This file parses the http.conf file and creates all the necessary MaHost, MaDir and
 *  MaLocation objects to manage the server's operation.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */




#if BLD_UNIX_LIKE
static int allDigits(cchar *s);
#endif
static void initLimits(MaHttp *http);
static int  httpDestructor(MaHttp *http);

/*
 *  Create a web server described by a config file. 
 */
MaHttp *maCreateWebServer(cchar *configFile)
{
    Mpr         *mpr;
    MaHttp      *http;
    MaServer    *server;

    /*
     *  Initialize and start the portable runtime services.
     */
    if ((mpr = mprCreate(0, NULL, NULL)) == 0) {
        mprError(mpr, "Can't create the web server runtime");
        return 0;
    }

    if (mprStart(mpr, 0) < 0) {
        mprError(mpr, "Can't start the web server runtime");
        return 0;
    }

    http = maCreateHttp(mpr);

    if ((server = maCreateServer(http, configFile, NULL, NULL, -1)) == 0) {
        mprError(mpr, "Can't create the web server");
        return 0;
    }
    
    if (maParseConfig(server, configFile) < 0) {
        mprError(mpr, "Can't parse the config file %s", configFile);
        return 0;
    }
    return http;
}


/*
 *  Service requests for a web server.
 */
int maServiceWebServer(MaHttp *http)
{
    if (maStartHttp(http) < 0) {
        mprError(http, "Can't start the web server");
        return MPR_ERR_CANT_CREATE;
    }
    mprServiceEvents(http, -1, 0);

    maStopHttp(http);
    return 0;
}


/*
 *  Run a web server using a config file. 
 */
int maRunWebServer(cchar *configFile)
{
    MaHttp      *http;

    if ((http = maCreateWebServer(configFile)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    return maServiceWebServer(http);
}


int maRunSimpleWebServer(cchar *ipAddr, int port, cchar *docRoot)
{
    Mpr         *mpr;
    MaHttp      *http;
    MaServer    *server;

    /*
     *  Initialize and start the portable runtime services.
     */
    if ((mpr = mprCreate(0, NULL, NULL)) == 0) {
        mprError(mpr, "Can't create the web server runtime");
        return MPR_ERR_CANT_CREATE;
    }

    if (mprStart(mpr, 0) < 0) {
        mprError(mpr, "Can't start the web server runtime");
        return MPR_ERR_CANT_INITIALIZE;
    }

    /*
     *  Create the HTTP object.
     */
    if ((http = maCreateHttp(mpr)) == 0) {
        mprError(mpr, "Can't create the web server http services");
        return MPR_ERR_CANT_INITIALIZE;
    }

    /*
     *  Create and start the HTTP server. Give the server a name of "default" and define "." as 
     *  the default serverRoot, ie. the directory with the server configuration files.
     */
    server = maCreateServer(http, ipAddr, ".", ipAddr, port);
    if (server == 0) {
        mprError(mpr, "Can't create the web server");
        return MPR_ERR_CANT_CREATE;
    }
    maSetDocumentRoot(server->defaultHost, docRoot);
    
    if (maStartHttp(http) < 0) {
        mprError(mpr, "Can't start the web server");
        return MPR_ERR_CANT_CREATE;
    }

    mprServiceEvents(mpr, -1, 0);

    maStopHttp(http);
    mprFree(mpr);

    return 0;
}


MaHttp *maCreateHttp(MprCtx ctx)
{
    MaHttp      *http;

    http = mprAllocObjWithDestructorZeroed(ctx, MaHttp, httpDestructor);
    if (http == 0) {
        return 0;
    }
    http->servers = mprCreateList(http);
    http->stages = mprCreateHash(http, 0);

#if BLD_FEATURE_MULTITHREAD
    http->mutex = mprCreateLock(http);
#endif

    initLimits(http);

#if BLD_UNIX_LIKE
{
    struct passwd   *pp;
    struct group    *gp;

    http->uid = getuid();
    if ((pp = getpwuid(http->uid)) == 0) {
        mprError(http, "Can't read user credentials: %d. Check your /etc/passwd file.", http->uid);
    } else {
        http->username = mprStrdup(http, pp->pw_name);
    }

    http->gid = getgid();
    if ((gp = getgrgid(http->gid)) == 0) {
        mprError(http, "Can't read group credentials: %d. Check your /etc/group file", http->gid);
    } else {
        http->groupname = mprStrdup(http, gp->gr_name);
    }
}
#else
    http->uid = http->gid = -1;
#endif

#if BLD_FEATURE_SEND && !WIN
    maOpenSendConnector(http);
#endif
#if BLD_FEATURE_NET
    maOpenNetConnector(http);
#endif
    maOpenPassHandler(http);

    return http;
}


static int httpDestructor(MaHttp *http)
{
    /* TODO - should this call stop? */
    maUnloadStaticModules(http);
    return 0;
}


void maAddServer(MaHttp *http, MaServer *server)
{
    mprAddItem(http->servers, server);
}



void maSetDefaultServer(MaHttp *http, MaServer *server)
{
    http->defaultServer = server;
}


//  TODO - standardize on Find, Lookup, Get
MaServer *maLookupServer(MaHttp *http, cchar *name)
{
    MaServer    *server;
    int         next;

    for (next = 0; (server = mprGetNextItem(http->servers, &next)) != 0; ) {
        if (strcmp(server->name, name) == 0) {
            return server;
        }
    }
    return 0;
}


void maRegisterStage(MaHttp *http, MaStage *stage)
{
    mprAddHash(http->stages, stage->name, stage);
}


MaStage *maLookupStage(MaHttp *http, cchar *name)
{
    return (MaStage*) mprLookupHash(http->stages, name);
}


void *maLookupStageData(MaHttp *http, cchar *name)
{
    MaStage     *stage;

    stage = (MaStage*) mprLookupHash(http->stages, name);
    return stage->stageData;
}


int maStartHttp(MaHttp *http)
{
    MaServer    *server;
    int         next;

    /*
     *  Start servers (and hosts)
     */
    for (next = 0; (server = mprGetNextItem(http->servers, &next)) != 0; ) {
        if (maStartServer(server) < 0) {
            return MPR_ERR_CANT_INITIALIZE;
        }
    }

    return 0;
}


int maStopHttp(MaHttp *http)
{
    MaServer    *server;
    int         next;

    for (next = 0; (server = mprGetNextItem(http->servers, &next)) != 0; ) {
        maStopServer(server);
    }
    return 0;
}


int maSetHttpUser(MaHttp *http, cchar *newUser)
{
#if BLD_UNIX_LIKE
    struct passwd   *pp;

    if (allDigits(newUser)) {
        http->uid = atoi(newUser);
        if ((pp = getpwuid(http->uid)) == 0) {
            mprError(http, "Bad user id: %d", http->uid);
            return MPR_ERR_CANT_ACCESS;
        }
        newUser = pp->pw_name;

    } else {
        if ((pp = getpwnam(newUser)) == 0) {
            mprError(http, "Bad user name: %s", newUser);
            return MPR_ERR_CANT_ACCESS;
        }
        http->uid = pp->pw_uid;
    }
#endif

    mprFree(http->username);
    http->username = mprStrdup(http, newUser);

    return 0;
}


int maSetHttpGroup(MaHttp *http, cchar *newGroup)
{
#if BLD_UNIX_LIKE
    struct group    *gp;

    if (allDigits(newGroup)) {
        http->gid = atoi(newGroup);
        if ((gp = getgrgid(http->gid)) == 0) {
            mprError(http, "Bad group id: %d", http->gid);
            return MPR_ERR_CANT_ACCESS;
        }
        newGroup = gp->gr_name;

    } else {
        if ((gp = getgrnam(newGroup)) == 0) {
            mprError(http, "Bad group name: %s", newGroup);
            return MPR_ERR_CANT_ACCESS;
        }
        http->gid = gp->gr_gid;
    }
#endif

    mprFree(http->groupname);
    http->groupname = mprStrdup(http, newGroup);

    return 0;
}


int maApplyChangedUser(MaHttp *http)
{
#if BLD_UNIX_LIKE
    if (http->uid >= 0) {
        if ((setuid(http->uid)) != 0) {
            mprError(http, "Can't change user to: %s: %d\n"
                "WARNING: This is a major security exposure", http->username, http->uid);
#if LINUX && PR_SET_DUMPABLE
        } else {
            prctl(PR_SET_DUMPABLE, 1);
#endif
        }
    }
#endif
    return 0;
}


int maApplyChangedGroup(MaHttp *http)
{
#if BLD_UNIX_LIKE
    if (http->gid >= 0) {
        if (setgid(http->gid) != 0) {
            mprError(http, "Can't change group to %s: %d\n"
                "WARNING: This is a major security exposure", http->groupname, http->gid);
#if LINUX && PR_SET_DUMPABLE
        } else {
            prctl(PR_SET_DUMPABLE, 1);
#endif
        }
    }
#endif
    return 0;
}


/*
 *  Load a module. Returns 0 if the modules is successfully loaded either statically or dynamically.
 */
int maLoadModule(MaHttp *http, cchar *name, cchar *libname)
{
    MprModule   *module;
    char        entryPoint[MPR_MAX_FNAME];
    char        pathBuf[MPR_MAX_FNAME];

    module = mprLookupModule(http, name);
    if (module) {
        mprLog(http, MPR_CONFIG, "Activating module (Builtin) %s", name);
        return 0;
    }

    mprSprintf(entryPoint, sizeof(entryPoint), "ma%sInit", name);
    entryPoint[2] = toupper((int) entryPoint[2]);

    if (libname) {
        mprStrcpy(pathBuf, sizeof(pathBuf), libname);
    } else {
        mprSprintf(pathBuf, sizeof(pathBuf), "mod_%s%s", name, BLD_SHOBJ);
    }

    if (mprLoadModule(http, pathBuf, entryPoint) == 0) {
        return MPR_ERR_CANT_CREATE;
    }

    mprLog(http, MPR_CONFIG, "Activating module (Loadable) %s", name);

    return 0;
}


static void initLimits(MaHttp *http)
{
    MaLimits    *limits;

    limits = &http->limits;

    limits->maxBody = MA_MAX_BODY;
    limits->maxChunkSize = MA_MAX_CHUNK_SIZE;
    limits->maxResponseBody = MA_MAX_RESPONSE_BODY;
    limits->maxStageBuffer = MA_MAX_STAGE_BUFFER;
    limits->maxNumHeaders = MA_MAX_NUM_HEADERS;
    limits->maxHeader = MA_MAX_HEADERS;
    limits->maxUrl = MA_MAX_URL;
    limits->maxUploadSize = MA_MAX_UPLOAD_SIZE;
    limits->maxThreads = MA_DEFAULT_MAX_THREADS;
    limits->minThreads = 0;

    /*
     *  Zero means use O/S defaults
     */
    //  TODO - we should modify this to be smaller!
    limits->threadStackSize = 0;
}


/*
 *  Create a new server. There is typically only one server with one or more (virtual) hosts.
 */
MaServer *maCreateServer(MaHttp *http, cchar *name, cchar *root, cchar *ipAddr, int port)
{
    MaServer        *server;
    MaHostAddress   *hostAddress;
    MaListen        *listen;

    mprAssert(http);
    mprAssert(name && *name);

    server = mprAllocObjZeroed(http, MaServer);
    if (server == 0) {
        return 0;
    }

    server->hosts = mprCreateList(server);
    server->listens = mprCreateList(server);
    server->hostAddresses = mprCreateList(server);
    server->name = mprStrdup(server, name);
    server->http = http;

    maAddServer(http, server);
    maSetServerRoot(server, root);

    if (ipAddr && port > 0) {
        listen = maCreateListen(server, ipAddr, port, 0);
        maAddListen(server, listen);

        hostAddress = maCreateHostAddress(server, ipAddr, port);
        mprAddItem(server->hostAddresses, hostAddress);
    }
    maSetDefaultServer(http, server);

    maLoadStaticModules(http);

    return server;
}


int maStartServer(MaServer *server)
{
    MaHost      *host;
    MaListen    *listen;
    int         next, count, warned;

    /*
     *  Start the hosts
     */
    for (next = 0; (host = mprGetNextItem(server->hosts, &next)) != 0; ) {
        mprLog(server, 1, "Starting host named: \"%s\"", host->name);
        if (maStartHost(host) < 0) {
            return MPR_ERR_CANT_INITIALIZE;
        }
    }

    /*
     *  Listen to all required ipAddr:ports
     */
    warned = 0;
    count = 0;
    for (next = 0; (listen = mprGetNextItem(server->listens, &next)) != 0; ) {
        if (maStartListening(listen) < 0) {
            //  TODO push error message down
            mprError(server, "Can't listen for HTTP on %s:%d", listen->ipAddr, listen->port);
            warned++;
            break;

        } else {
            count++;
        }
    }

    if (count == 0) {
        if (! warned) {
            mprError(server, "Server is not listening on any addresses");
        }
        return MPR_ERR_CANT_OPEN;
    }

    /*
     *  Now change user and group to the desired identities (user must be last)
     */
    if (maApplyChangedGroup(server->http) < 0 || maApplyChangedUser(server->http) < 0) {
        return MPR_ERR_CANT_COMPLETE;
    }

    return 0;
}


/*
 *  Stop the server and stop listening on all ports
 */
int maStopServer(MaServer *server)
{
    MaHost      *host;
    MaListen    *listen;
    int         next;

    for (next = 0; (listen = mprGetNextItem(server->listens, &next)) != 0; ) {
        maStopListening(listen);
    }

    for (next = 0; (host = mprGetNextItem(server->hosts, &next)) != 0; ) {
        maStopHost(host);
    }
    return 0;
}


void maAddHost(MaServer *server, MaHost *host)
{
    mprAddItem(server->hosts, host);
}


void maAddListen(MaServer *server, MaListen *listen)
{
    mprAddItem(server->listens, listen);
}


MaHost *maLookupHost(MaServer *server, cchar *name)
{
    MaHost  *host;
    int     next;

    for (next = 0; (host = mprGetNextItem(server->hosts, &next)) != 0; ) {
        if (strcmp(host->name, name) == 0) {
            return host;
        }
    }
    return 0;
}


/*
 *  Lookup a host address. If ipAddr is null or port is -1, then those elements are wild.
 */
MaHostAddress *maLookupHostAddress(MaServer *server, cchar *ipAddr, int port)
{
    MaHostAddress   *address;
    int             next;

    for (next = 0; (address = mprGetNextItem(server->hostAddresses, &next)) != 0; ) {
        if (address->port < 0 || port < 0 || address->port == port) {
            if (ipAddr == 0 || address->ipAddr == 0 || strcmp(address->ipAddr, ipAddr) == 0) {
                return address;
            }
        }
    }
    return 0;
}


/*
 *  Create the host addresses for a host. Called for hosts or for NameVirtualHost directives (host == 0).
 */
int maCreateHostAddresses(MaServer *server, MaHost *host, cchar *configValue)
{
    MaListen        *listen;
    MaHostAddress   *address;
    char            *ipAddrPort, *ipAddr, *value, *tok;
    char            addrBuf[MPR_MAX_IP_ADDR_PORT];
    int             next, port;

    address = 0;
    value = mprStrdup(server, configValue);
    ipAddrPort = mprStrTok(value, " \t", &tok);

    while (ipAddrPort) {
        if (mprStrcmpAnyCase(ipAddrPort, "_default_") == 0) {
            //  TODO is this used?
            mprAssert(0);
            ipAddrPort = "*:*";
        }

        if (mprParseIp(server, ipAddrPort, &ipAddr, &port, -1) < 0) {
            mprError(server, "Can't parse ipAddr %s", ipAddrPort);
            continue;
        }
        mprAssert(ipAddr && *ipAddr);
        if (ipAddr[0] == '*') {
            ipAddr = mprStrdup(server, "");
        }

        /*
         *  For each listening endpiont,
         */
        for (next = 0; (listen = mprGetNextItem(server->listens, &next)) != 0; ) {
            if (port > 0 && port != listen->port) {
                continue;
            }
            if (listen->ipAddr[0] != '\0' && ipAddr[0] != '\0' && strcmp(ipAddr, listen->ipAddr) != 0) {
                continue;
            }

            /*
             *  Find the matching host address or create a new one
             */
            if ((address = maLookupHostAddress(server, listen->ipAddr, listen->port)) == 0) {
                address = maCreateHostAddress(server, listen->ipAddr, listen->port);
                mprAddItem(server->hostAddresses, address);
            }

            /*
             *  If a host is specified
             */
            if (host == 0) {
                maSetNamedVirtualHostAddress(address);

            } else {
                maInsertVirtualHost(address, host);
                if (listen->ipAddr[0] != '\0') {
                    mprSprintf(addrBuf, sizeof(addrBuf), "%s:%d", listen->ipAddr, listen->port);
                } else {
                    mprSprintf(addrBuf, sizeof(addrBuf), "%s:%d", ipAddr, listen->port);
                }
                maSetHostName(host, addrBuf);
            }
        }
        mprFree(ipAddr);
        ipAddrPort = mprStrTok(0, " \t", &tok);
    }

    if (host) {
        if (address == 0) {
            mprError(server, "No valid IP address for host %s", host->name);
            mprFree(value);
            return MPR_ERR_CANT_INITIALIZE;
        }
        if (maIsNamedVirtualHostAddress(address)) {
            maSetNamedVirtualHost(host);
        }
    }

    mprFree(value);

    return 0;
}


/*
 *  Set the Server Root directory. We convert path into an absolute path.
 */
void maSetServerRoot(MaServer *server, cchar *path)
{
    if (path == 0) {
        path = ".";
    }
#if BLD_FEATURE_ROMFS
    path = ".";
#endif

    mprFree(server->serverRoot);
    server->serverRoot = mprGetAbsFilename(server, path);
}


void maSetDefaultHost(MaServer *server, MaHost *host)
{
    server->defaultHost = host;
}


void maSetDefaultIndex(MaServer *server, cchar *path, cchar *fileName)
{
    MaHost  *host;
    MaDir   *dir;
    char    pathBuf[MPR_MAX_FNAME];
    int     next;

    for (next = 0; (host = mprGetNextItem(server->hosts, &next)) != 0; ) {
        mprStrcat(pathBuf, sizeof(pathBuf), "/", host->documentRoot, path, 0);
        dir = maLookupBestDir(host, pathBuf);
        mprAssert(dir);
        maSetDirIndex(dir, fileName);
    }
}


int maSplitConfigValue(MprCtx ctx, char **s1, char **s2, char *buf, int quotes)
{
    char    *next;

    if (maGetConfigValue(ctx, s1, buf, &next, quotes) < 0 || maGetConfigValue(ctx, s2, next, &next, quotes) < 0) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (*s1 == 0 || *s2 == 0) {
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}


int maGetConfigValue(MprCtx ctx, char **arg, char *buf, char **nextToken, int quotes)
{
    char    *endp;

    if (buf == 0) {
        return -1;
    }
    while (isspace((int) *buf)) {
        buf++;
    }

    if (quotes && *buf == '\"') {
        *arg = ++buf;
        if ((endp = strchr(buf, '\"')) != 0) {
            *endp++ = '\0';
        } else {
            return MPR_ERR_BAD_SYNTAX;
        }
        while ((int) isspace((int) *endp)) {
            endp++;
        }
        *nextToken = endp;
    } else {
        *arg = mprStrTok(buf, " \t\n", nextToken);
    }
    return 0;
}


/*
 *  Convenience function to create a new default host
 */
MaHost *maCreateDefaultHost(MaServer *server, cchar *docRoot, cchar *ipAddr, int port)
{
    MaHost          *host;
    MaListen        *listen;
    MaHostAddress   *address;

    if (ipAddr == 0) {
        /*
         *  If no IP:PORT specified, find the first listening endpoint. In this case, we expect the caller to
         *  have setup the lisenting endponts and to have added them to the host address hash.
         */
        listen = mprGetFirstItem(server->listens);
        if (listen) {
            ipAddr = listen->ipAddr;
            port = listen->port;

        } else {
            listen = maCreateListen(server, "localhost", MA_SERVER_DEFAULT_PORT_NUM, 0);
            maAddListen(server, listen);
            ipAddr = "localhost";
            port = MA_SERVER_DEFAULT_PORT_NUM;
        }
        host = maCreateHost(server, ipAddr, 0);

    } else {
        /*
         *  Create a new listening endpoint
         */
        listen = maCreateListen(server, ipAddr, port, 0);
        maAddListen(server, listen);
        host = maCreateHost(server, ipAddr, 0);
    }

    if (maOpenMimeTypes(host, "mime.types") < 0) {
        maAddStandardMimeTypes(host);
    }

    /*
     *  Insert the host and create a directory object for the docRoot
     */
    maAddHost(server, host);
    maInsertDir(host, maCreateBareDir(host, docRoot));
    maSetDocumentRoot(host, docRoot);

    /*
     *  Ensure we are in the hash lookup of all the addresses to listen to acceptWrapper uses this hash to find
     *  the host to serve the request.
     */
    address = maLookupHostAddress(server, ipAddr, port);
    if (address == 0) {
        address = maCreateHostAddress(server, ipAddr, port);
        mprAddItem(server->hostAddresses, address);
    }
    maInsertVirtualHost(address, host);

    if (server->defaultHost == 0) {
        server->defaultHost = host;
    }
    return host;
}



#if BLD_UNIX_LIKE
static int allDigits(cchar *s)
{
    return strspn(s, "1234567890") == strlen(s);
}
#endif

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
/************************************************************************/
/*
 *  End of file "../http/server.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/stage.c"
 */
/************************************************************************/

/*
 *  stages.c -- Stage module to processes HTTP requests.
 *
 *  Stages support the extensible and modular processing of HTTP requests. Handlers are a kind of stage that are the first line 
 *  processing of a request. Connectors are the last stage in a chain to send/receive data over a network.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */




void maCloseStage(MaConn *conn)
{
    MaResponse      *resp;
    MaQueue         *q, *qhead;
    int             i;

    resp = conn->response;
    if (resp) {
        for (i = 0; i < MA_MAX_QUEUE; i++) {
            qhead = &resp->queue[i];
            for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
                if (q->close && q->flags & MA_QUEUE_OPEN) {
                    q->flags &= ~MA_QUEUE_OPEN;
                    q->close(q);
                }
            }
        }
    }
}


static void defaultOpen(MaQueue *q)
{
    MaResponse      *resp;

    resp = q->conn->response;
    q->packetSize = (resp->chunkSize > 0) ? min(q->max, resp->chunkSize): q->max;
}


static void defaultClose(MaQueue *q)
{
}


static int defaultParse(MaHttp *http, cchar *key, char *value, MaConfigState *state)
{
    mprAssert(http);
    mprAssert(key && *key);
    mprAssert(state);
    return 0;
}


/*
 *  The default put will put the packet on the service queue.
 */
static void outgoingData(MaQueue *q, MaPacket *packet)
{
    int     enableService;

    /*
     *  Handlers service routines must only be enabled if in the processing state.
     */
    enableService = !(q->stage->flags & MA_STAGE_HANDLER) || (q->conn->state & MPR_HTTP_STATE_PROCESSING) ? 1 : 0;
    maPutForService(q, packet, enableService);
}


static void incomingData(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;
    MaRequest   *req;
    MaResponse  *resp;
    MprBuf      *content;
    cchar       *pat;
    
    mprAssert(packet);
    
    if (!(q->stage->flags & MA_STAGE_HANDLER)) {
        maPutNext(q, packet);
        return;
    }

    conn = q->conn;
    req = conn->request;
    resp = conn->response;
    
    if (packet->count == 0) {
        if (resp->handler->flags & MA_STAGE_FORM_VARS) {
            pat = "application/x-www-form-urlencoded";
            if (mprStrcmpAnyCaseCount(req->mimeType, pat, (int) strlen(pat)) == 0) {
                content = q->first->content;
                if (content) {
                    mprAddNullToBuf(content);
                    mprLog(q, 3, "post data: length %d, \"%s\"", mprGetBufLength(content), mprGetBufStart(content));
                    maAddFormVars(conn, mprGetBufStart(content), mprGetBufLength(content));
                }
            }
        }
        return;
    }
    maJoinForService(q, packet, 0);
}


/*
 *  The service routine runs when all input data has been received.
 */
void maDefaultOutgoingServiceStage(MaQueue *q)
{
    MaConn      *conn;
    MaPacket    *packet;

    conn = q->conn;

    for (packet = maGet(q); packet; packet = maGet(q)) {
        if (!maWillNextQueueAccept(q, packet)) {
            maPutBack(q, packet);
            return;
        }
        maPutNext(q, packet);
    }
}


static void incomingService(MaQueue *q)
{
}


MaStage *maCreateStage(MaHttp *http, cchar *name, int flags)
{
    MaStage     *stage;

    mprAssert(http);
    mprAssert(name && *name);

    stage = mprAllocObjZeroed(http, MaStage);
    if (stage == 0) {
        return 0;
    }

    stage->flags = flags;
    stage->name = mprStrdup(stage, name);

    /*
     *  Caller will selectively override
     */
    stage->open = defaultOpen;
    stage->close = defaultClose;
    stage->parse = defaultParse;
    
    stage->incomingData = incomingData;
    stage->incomingService = incomingService;
    
    stage->outgoingData = outgoingData;
    stage->outgoingService = maDefaultOutgoingServiceStage;

    maRegisterStage(http, stage);
    
    return stage;
}


MaStage *maCreateHandler(MaHttp *http, cchar *name, int flags)
{
    MaStage     *stage;
    
    stage = maCreateStage(http, name, flags);
    stage->flags |= MA_STAGE_HANDLER;
    return stage;
}


MaStage *maCreateFilter(MaHttp *http, cchar *name, int flags)
{
    MaStage     *stage;
    
    stage = maCreateStage(http, name, flags);
    stage->flags |= MA_STAGE_FILTER;
    return stage;
}


MaStage *maCreateConnector(MaHttp *http, cchar *name, int flags)
{
    MaStage     *stage;
    
    stage = maCreateStage(http, name, flags);
    stage->flags |= MA_STAGE_CONNECTOR;
    return stage;
}


bool maMatchFilterByExtension(MaFilter *filter, cchar *ext)
{
    return mprLookupHash(filter->extensions, ext) != 0;
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
/************************************************************************/
/*
 *  End of file "../http/stage.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../http/var.c"
 */
/************************************************************************/

/*
 *  var.c -- Create header and query variables.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 *
 *  TODO - rethink this while approach
 */



/*
 *  Define standard CGI environment variables
 */
void maCreateEnvVars(MaConn *conn)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaHost          *host;
    MprSocket       *listenSock;
    MprHashTable    *vars;
    char            port[16];

    req = conn->request;
    resp = conn->response;
    host = conn->host;
    
    vars = req->headers;

    //  TODO - BUG COOKIE
    
    /*
     *  Alias for REMOTE_USER. Define both for broader compatibility with CGI.
     */
    mprAddHash(vars, "AUTH_TYPE", req->authType);
    mprAddHash(vars, "AUTH_USER", (req->user && *req->user) ? req->user : 0);
    mprAddHash(vars, "AUTH_GROUP", req->group);
    mprAddHash(vars, "AUTH_ACL", "");
    mprAddHash(vars, "CONTENT_LENGTH", req->contentLengthStr);
    mprAddHash(vars, "CONTENT_TYPE", req->mimeType);
    mprAddHash(vars, "DOCUMENT_ROOT", host->documentRoot);
    mprAddHash(vars, "GATEWAY_INTERFACE", "CGI/1.1");
    mprAddHash(vars, "QUERY_STRING", req->parsedUri->query);

    mprAddHash(vars, "REMOTE_ADDR", conn->remoteIpAddr);
    mprItoa(port, sizeof(port) - 1, conn->remotePort, 10);
    mprAddHash(vars, "REMOTE_PORT", mprStrdup(vars, port));


#if BLD_FEATURE_REVERSE_DNS && !WIN
    /*
     *  This feature has denial of service risks. Doing a reverse DNS will be slower,
     *  and can potentially hang the web server. Use at your own risk!!  Not supported for windows.
     */
    {
        struct addrinfo *result;
        char            name[MPR_MAX_STRING];
        int             rc;

        if (getaddrinfo(remoteIpAddr, NULL, NULL, &result) == 0) {
            rc = getnameinfo(result->ai_addr, sizeof(struct sockaddr), name, sizeof(name), NULL, 0, NI_NAMEREQD);
            freeaddrinfo(result);
            if (rc == 0) {
                mprAddHash(vars, "REMOTE_HOST", remoteIpAddr);
            }
        }
        mprAddHash(vars, "REMOTE_HOST", (rc == 0) ? name : remoteIpAddr);
    }
#else
    mprAddHash(vars, "REMOTE_HOST", conn->remoteIpAddr);
#endif

    /*
     *  Same as AUTH_USER (yes this is right)
     */
    mprAddHash(vars, "REMOTE_USER", (req->user && *req->user) ? req->user : 0);
    mprAddHash(vars, "REQUEST_METHOD", req->methodName);

#if BLD_FEATURE_SSL
    mprAddHash(vars, "REQUEST_TRANSPORT", (char*) ((host->secure) ? "https" : "http"));
#else
    mprAddHash(vars, "REQUEST_TRANSPORT", "http");
#endif
    
    listenSock = conn->sock->listenSock;
    mprAddHash(vars, "SERVER_ADDR", listenSock->ipAddr);
    mprItoa(port, sizeof(port) - 1, listenSock->port, 10);
    mprAddHash(vars, "SERVER_PORT", mprStrdup(req, port));
    mprAddHash(vars, "SERVER_NAME", host->name);
    mprAddHash(vars, "SERVER_PROTOCOL", req->parsedUri->scheme);
    mprAddHash(vars, "SERVER_SOFTWARE", MA_SERVER_NAME);

    /*
     *  This is the complete URI before decoding
     */ 
    mprAddHash(vars, "REQUEST_URI", req->parsedUri->originalUri);

    /*
     *  URLs are broken into the following: http://{SERVER_NAME}:{SERVER_PORT}{SCRIPT_NAME}{PATH_INFO}
     */
    mprAddHash(vars, "SCRIPT_NAME", req->url);
    mprAddHash(vars, "SCRIPT_FILENAME", resp->filename);
    mprAddHash(vars, "PATH_INFO", req->pathInfo);

    if (req->pathTranslated) {
        /*
         *  Only set PATH_TRANSLATED if PATH_INFO is set (CGI spec)
         */
        mprAddHash(vars, "PATH_TRANSLATED", req->pathTranslated);
    }
}


/*
 *  Add variables to the form params[] data. This comes from the query string and urlencoded post data.
 *  Make variables for each keyword in a query string. The buffer must be url encoded (ie. key=value&key2=value2..., 
 *  spaces converted to '+' and all else should be %HEX encoded).
 */
void maAddFormVars(MaConn *conn, cchar *buf, int len)
{
    MaResponse      *resp;
    MaRequest       *req;
    MprHashTable    *vars;
    cchar           *oldValue;
    char            *newValue, *decoded, *keyword, *value, *tok;

    resp = conn->response;
    req = conn->request;
    vars = req->formVars;
    
    mprAssert(vars);
    
    decoded = (char*) mprAlloc(resp, len + 1);
    decoded[len] = '\0';
    memcpy(decoded, buf, len);

    keyword = mprStrTok(decoded, "&", &tok);
    while (keyword != 0) {
        if ((value = strchr(keyword, '=')) != 0) {
            *value++ = '\0';
            mprUrlDecode(value, (int) strlen(value) + 1, value);
        } else {
            value = "";
        }
        mprUrlDecode(keyword, (int) strlen(keyword) + 1, keyword);

        if (*keyword) {
            /*
             *  Append to existing keywords.
             */
            oldValue = mprLookupHash(vars, keyword);
            if (oldValue != 0) {
                mprAllocSprintf(vars, &newValue, MA_MAX_HEADERS, "%s %s", oldValue, value);
                mprAddHash(vars, keyword, newValue);
                mprFree(newValue);
            } else {
                mprAddHash(vars, keyword, value);
            }
        }
        keyword = mprStrTok(0, "&", &tok);
    }
    /*
     *  Must not free "decoded". This will be freed when the response completes.
     */
}


int maTestFormVar(MaConn *conn, cchar *var)
{
    MprHashTable    *vars;
    
    vars = conn->request->formVars;
    return vars && mprLookupHash(vars, var) != 0;
}


cchar *maGetFormVar(MaConn *conn, cchar *var, cchar *defaultValue)
{
    MprHashTable    *vars;
    cchar           *value;
    
    vars = conn->request->formVars;

    if (vars) {
        value = mprLookupHash(vars, var);
        return (value) ? value : defaultValue;
    }
    return defaultValue;
}


int maGetIntFormVar(MaConn *conn, cchar *var, int defaultValue)
{
    MprHashTable    *vars;
    cchar           *value;
    
    vars = conn->request->formVars;

    if (vars) {
        value = mprLookupHash(vars, var);
        return (value) ? mprAtoi(value, 10) : defaultValue;
    }
    return defaultValue;
}


void maSetFormVar(MaConn *conn, cchar *var, cchar *value) 
{
    MprHashTable    *vars;
    
    vars = conn->request->formVars;
    mprAssert(vars);
    
    if (vars == 0) {
        return;
    }
    
    mprAddHash(vars, var, (void*) value);
}


void maSetIntFormVar(MaConn *conn, cchar *var, int value) 
{
    MprHashTable    *vars;
    char            buf[16];
    
    vars = conn->request->formVars;
    mprAssert(vars);
    
    if (vars == 0) {
        return;
    }
    
    mprItoa(buf, sizeof(buf) - 1, value, 10);
    mprAddHash(vars, var, buf);
}


int maCompareFormVar(MaConn *conn, cchar *var, cchar *value)
{
    MprHashTable    *vars;
    
    vars = conn->request->formVars;
    mprAssert(vars);
    
    if (vars == 0) {
        return 0;
    }
 
    if (strcmp(value, maGetFormVar(conn, var, " __UNDEF__ ")) == 0) {
        return 1;
    }
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
/************************************************************************/
/*
 *  End of file "../http/var.c"
 */
/************************************************************************/

