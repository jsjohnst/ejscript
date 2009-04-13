/**
 *  ejsFile.c - File class.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/********************************** Defines ***********************************/

#if BLD_WIN_LIKE
#define isDelim(fp, c)  (c == '/' || c == fp->delimiter)
#else
#define isDelim(fp, c)  (c == fp->delimiter)
#endif

/**************************** Forward Declarations ****************************/

static int  readData(Ejs *ejs, EjsFile *fp, EjsByteArray *ap, int offset, int count);

#if BLD_FEATURE_MMU && FUTURE
static void *mapFile(EjsFile *fp, uint size, int mode);
static void unmapFile(EjsFile *fp);
#endif

/************************************ Methods *********************************/
/*
 *  Index into a file and extract a byte. This is random access reading.
 */
static EjsVar *getFileProperty(Ejs *ejs, EjsFile *fp, int slotNum)
{
    int     c, offset;

    if (!(fp->mode & EJS_FILE_OPEN)) {
        ejsThrowIOError(ejs, "File is not open");
        return 0;
    }
#if UNUSED
    if (fp->mode & EJS_FILE_READ) {
        if (slotNum >= fp->info.size) {
            ejsThrowOutOfBoundsError(ejs, "Bad file index");
            return 0;
        }
    }
    if (slotNum < 0) {
        ejsThrowOutOfBoundsError(ejs, "Bad file index");
        return 0;
    }
#endif

#if BLD_FEATURE_MMU && FUTURE
    //  TODO - must check against mapped size here.
    c = fp->mapped[slotNum];
#else
    offset = mprSeek(fp->file, SEEK_CUR, 0);
    if (offset != slotNum) {
        if (mprSeek(fp->file, SEEK_SET, slotNum) != slotNum) {
            ejsThrowIOError(ejs, "Can't seek to file offset");
            return 0;
        }
    }
    c = mprPeekc(fp->file);
    if (c < 0) {
        ejsThrowIOError(ejs, "Can't read file");
        return 0;
    }
#endif
    return (EjsVar*) ejsCreateNumber(ejs, c);
}


/*
 *  Set a byte in the file at the offset designated by slotNum.
 */
static int setFileProperty(Ejs *ejs, EjsFile *fp, int slotNum, EjsVar *value)
{
    int     c, offset;

    if (!(fp->mode & EJS_FILE_OPEN)) {
        ejsThrowIOError(ejs, "File is not open");
        return 0;
    }
    if (!(fp->mode & EJS_FILE_WRITE)) {
        ejsThrowIOError(ejs, "File is not opened for writing");
        return 0;
    }
    c = ejsIsNumber(value) ? ejsGetInt(value) : ejsGetInt(ejsToNumber(ejs, value));

    offset = mprSeek(fp->file, SEEK_CUR, 0);
    if (slotNum < 0) {
        //  TODO OPT - could have an mprGetPosition(file) API
        slotNum = offset;
    }

#if BLD_FEATURE_MMU && FUTURE
    fp->mapped[slotNum] = c;
#else
    if (offset != slotNum && mprSeek(fp->file, SEEK_SET, slotNum) != slotNum) {
        ejsThrowIOError(ejs, "Can't seek to file offset");
        return 0;
    }

    if (mprPutc(fp->file, c) < 0) {
        ejsThrowIOError(ejs, "Can't write file");
        return 0;
    }
#endif
    return slotNum;
}


/************************************ Methods *********************************/
/*
 *  Constructor
 *
 *  function File(path: String)
 *
 */
static EjsVar *fileConstructor(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char    *path;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    path = ejsGetString(argv[0]);
    fp->path = mprCleanFilename(fp, path);

    return (EjsVar*) fp;
}


/*
 *  Return an absolute path name for the file
 *
 *  function get absolutePath()
 */
static EjsVar *absolutePath(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsVar  *vp;
    char    *path;

    path = mprGetAbsFilename(fp, fp->path);
    vp = (EjsVar*) ejsCreateString(ejs, path);
    mprFree(path);
    return vp;
}


/*
 *  Get the base name of a file
 *
 *  function basename(): String
 */
static EjsVar *basenameProc(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, mprGetBaseName(fp->path));
}


/*
 *  Close the file and free up all associated resources.
 *
 *  function close(graceful: Boolean): void
 */
static EjsVar *closeFile(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    if (fp->mode & EJS_FILE_OPEN && fp->mode & EJS_FILE_WRITE) {
        if (mprFlush(fp->file) < 0) {
            ejsThrowIOError(ejs, "Can't flush file data");
            return 0;
        }
    }

    if (fp->file) {
        mprFree(fp->file);
        fp->file = 0;
    }
#if BLD_FEATURE_MMU && FUTURE
    if (fp->mapped) {
        unmapFile(fp);
        fp->mapped = 0;
    }
#endif
    return 0;
}


/*
 *  Copy a file
 *
 *  function copy(to: String): Void
 */
static EjsVar *copyFile(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFile     *from, *to;
    char        *buf, *toPath;
    uint        bytes;
    int         rc;

    mprAssert(argc == 1);
    toPath = ejsGetString(argv[0]);

    from = mprOpen(ejs, fp->path, O_RDONLY | O_BINARY, 0);
    if (from == 0) {
        ejsThrowIOError(ejs, "Cant open %s", fp->path);
        return 0;
    }

    to = mprOpen(ejs, toPath, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, 0664);
    if (to == 0) {
        ejsThrowIOError(ejs, "Cant create %s", toPath);
        mprFree(from);
        return 0;
    }

    buf = mprAlloc(ejs, MPR_BUFSIZE);
    if (buf == 0) {
        ejsThrowMemoryError(ejs);
        mprFree(to);
        mprFree(from);
        return 0;
    }

    rc = 0;
    while ((bytes = mprRead(from, buf, MPR_BUFSIZE)) > 0) {
        if (mprWrite(to, buf, bytes) != bytes) {
            ejsThrowIOError(ejs, "Write error to %s", toPath);
            rc = 0;
            break;
        }
    }

    mprFree(from);
    mprFree(to);
    mprFree(buf);

    return 0;
}


/*
 *  Create a temporary file. Creates a new, uniquely named temporary file.
 *
 *  static function createTempFile(directory: String = null): File
 */
static EjsVar *createTempFile(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    char    *directory, path[MPR_MAX_FNAME];

    mprAssert(argc == 0 || argc == 1);

    directory = (argc == 1) ? ejsGetString(argv[0]): NULL;

    if (mprMakeTempFileName(ejs, path, sizeof(path), directory) < 0) {
        ejsThrowIOError(ejs, "Can't make temp file");
        return 0;
    }

    return (EjsVar*) ejsCreateFile(ejs, path);
}


/*
 *  Return when the file was created.
 *
 *  function get created(): Date
 */
static EjsVar *created(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateDate(ejs, (MprTime) info.ctime);
}


/**
 *  Get the directory name portion of a file.
 *
 *  function dirname(): String
 */
static EjsVar *dirnameProc(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char    *dir;
    int     len;

    len = (int) strlen(fp->path) + 1;
    dir = mprAlloc(fp, len);
    if (dir == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    return (EjsVar*) ejsCreateString(ejs, mprGetDirName(dir, len, fp->path));
}


/*
 *  Test to see if this file exists.
 *
 *  function get exists(): Boolean
 */
static EjsVar *exists(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    return (EjsVar*) ejsCreateBoolean(ejs, mprGetFileInfo(ejs, fp->path, &info) == 0);
}


/*
 *  Get the file extension portion of the file name.
 *
 *  function get extension(): String
 */
static EjsVar *extension(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char    *cp;

    if ((cp = strrchr(fp->path, '.')) == 0) {
        return (EjsVar*) ejs->emptyStringValue;
    }
    return (EjsVar*) ejsCreateString(ejs, cp);
}


/*
 *  Flush the stream and the underlying file data
 *
 *  function flush(): void
 */
static EjsVar *flushFile(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    if (!(fp->mode & EJS_FILE_OPEN)) {
        ejsThrowIOError(ejs, "File is not open");
        return 0;
    }
    if (!(fp->mode & EJS_FILE_WRITE)) {
        ejsThrowIOError(ejs, "File is not opened for writing");
        return 0;
    }
    if (mprFlush(fp->file) < 0) {
        ejsThrowIOError(ejs, "Can't flush file data");
        return 0;
    }
    return 0;
}


/*
 *  Return the amount of free space in the file system that would contain the given path.
 *  function freeSpace(path: String = null): Number
 */
static EjsVar *freeSpace(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    //  TODO

#if BREW
    Mpr     *mpr;
    uint    space;

    mpr = mprGetMpr(ejs);
    space = IFILEMGR_GetFreeSpace(mpr->fileMgr, 0);
    ejsSetReturnValueToInteger(ejs, space);
#endif

    return 0;
}


/*
 *  Function to iterate and return the next element index.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsFile     *fp;

    fp = (EjsFile*) ip->target;
    if (!ejsIsFile(fp)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < fp->info.size) {
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }

    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator for use with "for ... in". This returns byte offsets in the file.
 *
 *  iterator native function get(): Iterator
 */
static EjsVar *getFileIterator(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    mprGetFileInfo(ejs, fp->path, &fp->info);
    return (EjsVar*) ejsCreateIterator(ejs, (EjsVar*) fp, (EjsNativeFunction) nextKey, 0, NULL);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsFile     *fp;

    fp = (EjsFile*) ip->target;
    if (!ejsIsFile(fp)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < fp->info.size) {
#if !BLD_FEATURE_MMU || 1
        if (mprSeek(fp->file, SEEK_CUR, 0) != ip->index) {
            if (mprSeek(fp->file, SEEK_SET, ip->index) != ip->index) {
                ejsThrowIOError(ejs, "Can't seek to %d", ip->index);
                return 0;
            }
        }
        ip->index++;
        return (EjsVar*) ejsCreateNumber(ejs, mprGetc(fp->file));
#else
        return (EjsVar*) ejsCreateNumber(ejs, fp->mapped[ip->index++]);
#endif
    }

#if BLD_FEATURE_MMU && FUTURE
    unmapFile(fp);
    fp->mapped = 0;
#endif

    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to enumerate the bytes in the file. For use with "for each ..."
 *
 *  iterator native function getValues(): Iterator
 */
static EjsVar *getValues(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    mprGetFileInfo(ejs, fp->path, &fp->info);

    return (EjsVar*) ejsCreateIterator(ejs, (EjsVar*) fp, (EjsNativeFunction) nextValue, 0, NULL);
}


/*
 *  Get the files in a directory.
 *  function getFiles(enumDirs: Boolean = false): Array
 *
 *  TODO -- need pattern to match (what about "." and ".." and ".*")
 *  TODO - move this functionality into mprFile (see appweb dirHandler.c)
 */
static EjsVar *getFiles(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char            path[MPR_MAX_FNAME];
    EjsArray        *array;
    MprList         *list;
    MprDirEntry     *dp;
    bool            enumDirs, noPath;
    int             next;

    mprAssert(argc == 0 || argc == 1);

    enumDirs = (argc == 1) ? ejsGetBoolean(argv[0]): 0;

    array = ejsCreateArray(ejs, 0);
    if (array == 0) {
        //  TODO - great to push this down into ejsAllocVar
        ejsThrowMemoryError(ejs);
        return 0;
    }

    list = mprGetDirList(array, fp->path, enumDirs);
    if (list == 0) {
        ejsThrowIOError(ejs, "Can't read directory");
        return 0;
    }

    noPath = (fp->path[0] == '.' && fp->path[1] == '\0') || (fp->path[0] == '.' && fp->path[1] == '/' && fp->path[2] == '\0');

    for (next = 0; (dp = mprGetNextItem(list, &next)) != 0; ) {
        if (strcmp(dp->name, ".") == 0 || strcmp(dp->name, "..") == 0) {
            continue;
        }
        if (enumDirs || !(dp->isDir)) {
            if (noPath) {
                ejsSetProperty(ejs, (EjsVar*) array, -1, (EjsVar*) ejsCreateString(ejs, dp->name));
            } else {
                /*
                 *  Prepend the directory name
                 */
                mprSprintf(path, sizeof(path), "%s/%s", fp->path,  dp->name);
                ejsSetProperty(ejs, (EjsVar*) array, -1, (EjsVar*) ejsCreateString(ejs, path));
            }
        }
    }
    mprFree(list);

    return (EjsVar*) array;
}


/*
 *  Get the file contents as a byte array
 *
 *  static function getBytes(path: String): ByteArray
 */
static EjsVar *getBytes(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFile         *file;
    EjsByteArray    *result;
    cchar           *path;
    char            buffer[MPR_BUFSIZE];
    int             bytes, offset, rc;

    mprAssert(argc == 1 && ejsIsString(argv[0]));
    path = ejsGetString(argv[0]);

    file = mprOpen(ejs, path, O_RDONLY | O_BINARY, 0);
    if (file == 0) {
        ejsThrowIOError(ejs, "Can't open %s", path);
        return 0;
    }

    /*
     *  TODO - need to be smarter about running out of memory here if the file is very large.
     */
    result = ejsCreateByteArray(ejs, (int) mprGetFileSize(file));
    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    rc = 0;
    offset = 0;
    while ((bytes = mprRead(file, buffer, MPR_BUFSIZE)) > 0) {
        if (ejsCopyToByteArray(ejs, result, offset, buffer, bytes) < 0) {
            ejsThrowMemoryError(ejs);
            rc = -1;
            break;
        }
        offset += bytes;
    }
    ejsSetByteArrayPositions(ejs, result, 0, offset);

    mprFree(file);

    return (EjsVar*) result;
}


/**
 *  Get the file contents as an array of lines.
 *
 *  static function getLines(path: String): Array
 */
static EjsVar *getLines(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFile     *file;
    MprBuf      *data;
    EjsArray    *result;
    cchar       *path;
    char        *start, *end, *cp, buffer[MPR_BUFSIZE];
    int         bytes, rc, lineno;

    mprAssert(argc == 1 && ejsIsString(argv[0]));
    path = ejsGetString(argv[0]);

    result = ejsCreateArray(ejs, 0);
    if (result == NULL) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    file = mprOpen(ejs, path, O_RDONLY | O_BINARY, 0);
    if (file == 0) {
        ejsThrowIOError(ejs, "Can't open %s", path);
        return 0;
    }

    /*
     *  TODO - need to be smarter about running out of memory here if the file is very large.
     */
    data = mprCreateBuf(ejs, 0, (int) mprGetFileSize(file) + 1);
    result = ejsCreateArray(ejs, 0);
    if (result == NULL || data == NULL) {
        ejsThrowMemoryError(ejs);
        mprFree(file);
        return 0;
    }

    rc = 0;
    while ((bytes = mprRead(file, buffer, MPR_BUFSIZE)) > 0) {
        if (mprPutBlockToBuf(data, buffer, bytes) != bytes) {
            ejsThrowMemoryError(ejs);
            rc = -1;
            break;
        }
    }

    start = mprGetBufStart(data);
    end = mprGetBufEnd(data);
    for (lineno = 0, cp = start; cp < end; cp++) {
        if (*cp == '\n') {
            if (ejsSetProperty(ejs, (EjsVar*) result, lineno++, 
                    (EjsVar*) ejsCreateStringWithLength(ejs, start, (int) (cp - start))) < 0) {
                break;
            }
            start = cp + 1;
        } else if (*cp == '\r') {
            start = cp + 1;
        }
    }
    if (cp > start) {
        ejsSetProperty(ejs, (EjsVar*) result, lineno++, (EjsVar*) ejsCreateStringWithLength(ejs, start, (int) (cp - start)));
    }

    mprFree(file);
    mprFree(data);

    return (EjsVar*) result;
}


/**
 *  Get the file contents as a string
 *
 *  static function getString(path: String): String
 */
static EjsVar *getFileAsString(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFile     *file;
    MprBuf      *data;
    EjsVar      *result;
    cchar       *path;
    char        buffer[MPR_BUFSIZE];
    int         bytes, rc;

    mprAssert(argc == 1 && ejsIsString(argv[0]));
    path = ejsGetString(argv[0]);

    file = mprOpen(ejs, path, O_RDONLY | O_BINARY, 0);
    if (file == 0) {
        ejsThrowIOError(ejs, "Can't open %s", path);
        return 0;
    }

    /*
     *  TODO - need to be smarter about running out of memory here if the file is very large.
     */
    data = mprCreateBuf(ejs, 0, (int) mprGetFileSize(file) + 1);
    if (data == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    rc = 0;
    while ((bytes = mprRead(file, buffer, MPR_BUFSIZE)) > 0) {
        if (mprPutBlockToBuf(data, buffer, bytes) != bytes) {
            ejsThrowMemoryError(ejs);
            rc = -1;
            break;
        }
    }

    result = (EjsVar*) ejsCreateStringWithLength(ejs, mprGetBufStart(data),  mprGetBufLength(data));

    mprFree(file);
    mprFree(data);

    return result;
}


/*
 *  Get the file contents as an XML object
 *
 *  static function getXML(path: String): XML
 */
static EjsVar *getXML(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return 0;
}


/*
 *  Determine if the file path has a drive spec (C:) in the file name
 *
 *  static function hasDriveSpec(): Boolean
 */
static EjsVar *hasDriveSpec(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, 
        (isalpha((int) fp->path[0]) && fp->path[1] == ':' && (fp->path[2] == '/' || fp->path[2] == '\\')));
}


/*
 *  Determine if the file name is a directory
 *
 *  function get isDir(): Boolean
 */
static EjsVar *isDir(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;
    int             rc;

    rc = mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateBoolean(ejs, rc == 0 && info.isDir);
}


/*
 *  Determine if the file is currently open for reading or writing
 *
 *  function get isOpen(): Boolean
 */
static EjsVar *isOpen(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, fp->file != NULL);
}


/*
 *  Determine if the file name is a regular file
 *
 *  function get isRegular(): Boolean
 */
static EjsVar *isRegular(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateBoolean(ejs, info.isReg);
}


/*
 *  Get when the file was last accessed.
 *
 *  function get lastAccess(): Date
 */
static EjsVar *lastAccess(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateDate(ejs, (MprTime) info.atime);
}


/*
 *  Get the length of the file associated with this File object.
 *
 *  override intrinsic function get length(): Number
 */
static EjsVar *fileLength(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    if (fp->mode & EJS_FILE_OPEN) {
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) mprGetFileSize(fp->file));

    } else {
        if (mprGetFileInfo(ejs, fp->path, &info) < 0) {
            return (EjsVar*) ejs->minusOneValue;
        }
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) info.size);
    }
}


/*
 *  function makeDir(permissions: Number = 0755): Void
 */
static EjsVar *makeDir(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;
    int             perms;

    mprAssert(argc == 0 || argc == 1);

    perms = (argc == 1) ? ejsGetInt(argv[0]) : 0755;

    if (mprGetFileInfo(ejs, fp->path, &info) == 0 && info.isDir) {
        return 0;
    }
    if (mprMakeDirPath(ejs, fp->path, perms) < 0) {
        ejsThrowIOError(ejs, "Cant create directory %s", fp->path);
        return 0;
    }
    return 0;
}


/**
 *  Get the file access mode.
 *
 *  function get mode(): Number
 */
static EjsVar *mode(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, (fp->file) ? fp->mode: 0);
}


/*
 *  Get when the file was created or last modified.
 *
 *  function get modified(): Date
 */
static EjsVar *modified(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateDate(ejs, (MprTime) info.mtime);
}


/*
 *  Get the name of the file associated with this File object.
 *
 *  function get name(): String
 */
static EjsVar *name(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, fp->path);
}


/*
 *  Get the newline characters
 *
 *  function get newline(): String
 */
static EjsVar *newline(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, mprGetFileNewline(ejs, "/"));
}


/*
 *  set the newline characters
 *
 *  function set newline(terminator: String): Void
 */
static EjsVar *setNewline(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    mprAssert(ejsIsString(argv[0]));
    mprSetFileNewline(ejs, "/", ((EjsString*) argv[0])->value);
    return 0;
}


/*
 *  Open a file using the current file name. This method requires a file instance.
 *
 *  function open(mode: Number = Read, permissions: Number = 0644): void
 */
static EjsVar *openProc(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    int     flags, mode, perms;

    mode = (argc >= 1) ? ejsGetInt(argv[0]) : EJS_FILE_READ;
    perms = (argc == 2) ? ejsGetInt(argv[1]) : 0644;

    if (fp->file) {
        ejsThrowIOError(ejs, "File already open");
        return 0;
    }

    flags = 0;

    if (mode & EJS_FILE_WRITE) {
        //  TODO - read/write needs more thought
        flags = O_BINARY | ((mode & EJS_FILE_READ) ? O_RDWR : O_WRONLY);
        if (mode & EJS_FILE_TRUNCATE) {
            flags |= O_TRUNC;
        }
        if (mode & EJS_FILE_APPEND) {
            flags |= O_APPEND;
        }
        if (mode & EJS_FILE_CREATE) {
            flags |= O_CREAT;
        }
        fp->file = mprOpen(ejs, fp->path, flags, perms);

    } else {
        /*
         *  Read is the default
         */
        flags = O_RDONLY;
        fp->file = mprOpen(ejs, fp->path, O_RDONLY | O_BINARY, 0);
    }

    if (fp->file == 0) {
        fp->mode = 0;
        ejsThrowIOError(ejs, "Can't open %s, error %d", fp->path, errno);
    } else {
        fp->mode = mode | EJS_FILE_OPEN;
    }
    //  TODO - should push this into File and it should maintain position and length
    mprGetFileInfo(ejs, fp->path, &fp->info);

#if BLD_FEATURE_MMU && FUTURE
    //  TODO - should always do info at the start.
    //  TODO - could push this into mprOpen via:  O_MAPPED. Need destructor to unmap.
    fp->mapped = mapFile(fp, fp->info.size, MPR_MAP_READ | MPR_MAP_WRITE);
#endif

    return 0;
}


/*
 *  Get the parent directory of the absolute path of the file.
 *
 *  function get parent(): String
 */
static EjsVar *parent(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsVar  *vp;
    char    *path;

    path = mprGetParentDir(fp, fp->path);
    vp = (EjsVar*) ejsCreateString(ejs, path);
    mprFree(path);
    return vp;
}


//  TODO - needs a path arg.
/*
 *  Return the path segment delimiter
 *
 *  static function get pathDelimiter(): String
 */
static EjsVar *pathDelimiter(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char    buf[2];

    buf[0] = mprGetFileDelimiter(ejs, "/");
    buf[1] = '\0';
    return (EjsVar*) ejsCreateString(ejs, buf);
}


//  TODO - Remove.  This is defined by the file system and not really modifiable.
/*
 *  Set the path segment delimiter
 *
 *  static function set pathDelimiter(value: String): void
 */
static EjsVar *setPathDelimiter(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsString(argv[0]));
    mprSetFileDelimiter(ejs, "/", ((EjsString*) argv[0])->value[0]);
    return 0;
}


/*
 *  Get the file security permissions.
 *
 *  function get permissions(): Number
 */
static EjsVar *getPermissions(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    mprGetFileInfo(ejs, fp->path, &info);

    return (EjsVar*) ejsCreateNumber(ejs, info.perms);
}


/*
 *  Set the file security permissions.
 *
 *  function set permissions(mask: Number): void
 */
static EjsVar *setPermissions(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
#if !VXWORKS
    int     perms;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    perms = ejsGetInt(argv[0]);
//  TODO - windows requires _chmod
    if (chmod(fp->path, perms) < 0) {
        ejsThrowIOError(ejs, "Can't set permissions on %s", fp->path);
    }
#endif
    return 0;
}


/*
 *  Get the current I/O position in the file.
 *
 *  function get position(): Number
 */
static EjsVar *getPosition(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    if (fp->file == 0) {
        ejsThrowStateError(ejs, "File not opened");
        return 0;
    }
    return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) mprGetFilePosition(fp->file));
}


/*
 *  Seek to a new location in the file and set the File marker to a new read/write position.
 *
 *  function set position(value: Number): void
 */
static EjsVar *setPosition(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    long        pos;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    pos = ejsGetInt(argv[0]);

    if (fp->file == 0) {
        ejsThrowStateError(ejs, "File not opened");
        return 0;
    }
    pos = ejsGetInt(argv[0]);
    if (mprSeek(fp->file, SEEK_SET, pos) != pos) {
        ejsThrowIOError(ejs, "Can't seek to %ld", pos);
    }
    return 0;
}


/*
 *  Put the file contents
 *
 *  static function put(path: String, permissions: Number, ...args): void
 */
static EjsVar *putToFile(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFile     *file;
    EjsArray    *args;
    char        *path, *data;
    int         i, bytes, length, permissions;

    mprAssert(argc == 3);

    path = ejsGetString(argv[0]);
    permissions = ejsGetInt(argv[1]);
    args = (EjsArray*) argv[2];

    /*
     *  Create fails if already present
     */
    mprDelete(ejs, path);
    file = mprOpen(ejs, path, O_CREAT | O_WRONLY | O_BINARY, permissions);
    if (file == 0) {
        ejsThrowIOError(ejs, "Cant create %s", path);
        return 0;
    }

    for (i = 0; i < args->length; i++) {
        data = ejsGetString(ejsToString(ejs, ejsGetProperty(ejs, (EjsVar*) args, i)));
        length = (int) strlen(data);
        bytes = mprWrite(file, data, length);
        if (bytes != length) {
            ejsThrowIOError(ejs, "Write error to %s", path);
            break;
        }
    }
    mprFree(file);

    return 0;
}


/*
 *  Return a relative path name for the file.
 *
 *  function get relativePath(): String
 */
static EjsVar *relativePath(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, mprGetRelFilename(fp, fp->path));
}


/*
 *  Read data bytes from a file
 *
 *  function readBytes(count: Number): ByteArray
 */
static EjsVar *readBytes(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsByteArray    *result;
    MprFileInfo     info;
    int             count, arraySize, totalRead;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    count = ejsGetInt(argv[0]);

    if (fp->file == 0) {
        ejsThrowStateError(ejs, "File not open");
        return 0;
    }

    if (!(fp->mode & EJS_FILE_READ)) {
        ejsThrowStateError(ejs, "File not opened for reading");
        return 0;
    }

    arraySize = mprGetFileInfo(fp, fp->path, &info) == 0 ? (int) info.size : MPR_BUFSIZE;
    result = ejsCreateByteArray(ejs, arraySize);
    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    /*
     *  TODO - what if the file is opened with a stream. Should still work
     */
    totalRead = readData(ejs, fp, result, 0, count);
    if (totalRead < 0) {
        return 0;
    }
    ejsSetByteArrayPositions(ejs, result, 0, totalRead);

    return (EjsVar*) result;
}


/*
 *  Read data bytes from a file
 *
 *  function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number
 */
static EjsVar *readProc(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsByteArray    *buffer;
    int             count, offset, totalRead;

    mprAssert(1 <= argc && argc <= 3);

    buffer = (EjsByteArray*) argv[0];
    offset = (argc >= 2) ? ejsGetInt(argv[1]): 0;
    count = (argc >= 3) ? ejsGetInt(argv[2]): buffer->length;

    if (fp->file == 0) {
        ejsThrowStateError(ejs, "File not open");
        return 0;
    }

    if (!(fp->mode & EJS_FILE_READ)) {
        ejsThrowStateError(ejs, "File not opened for reading");
        return 0;
    }

    /*
     *  TODO - what if the file is opened with a stream. Should still work
     */
    totalRead = readData(ejs, fp, buffer, offset, count);
    if (totalRead < 0) {
        return 0;
    }
    ejsSetByteArrayPositions(ejs, buffer, offset, totalRead);

    return (EjsVar*) ejsCreateNumber(ejs, totalRead);
}


/*
 *  function removeDir(): Void
 */
static EjsVar *removeDir(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    if (mprGetFileInfo(ejs, fp->path, &info) == 0 && mprDeleteDir(ejs, fp->path) < 0) {
        ejsThrowIOError(ejs, "Cant remove directory %s", fp->path);
        return 0;
    }
    return 0;
}


/*
 *  Remove the file associated with the File object.
 *
 *  function remove(quiet: Boolean = true): void
 */
static EjsVar *removeFile(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    MprFileInfo     info;

    if (fp->file) {
        ejsThrowStateError(ejs, "Can't remove an open file");
        return 0;
    }
    if (mprGetFileInfo(ejs, fp->path, &info) == 0 && unlink(fp->path) < 0) {
        ejsThrowIOError(ejs, "Cant remove file %s", fp->path);
    }
    return 0;
}


/*
 *  Rename the file
 *
 *  function rename(to: String): Void
 */
static int renameProc(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    char    *to;

    mprAssert(argc == 1 && ejsIsString(argv[0]));
    to = ejsGetString(argv[0]);

    unlink(to);
    if (rename(fp->path, to) < 0) {
        ejsThrowIOError(ejs, "Cant rename file %s to %s", fp->path, to);
        return 0;
    }
    return 0;
}


/*
 *  Put the file stream into async mode and define a completion callback
 *
 *  function setCallback(callback: Function): void
 */
static EjsVar *setFileCallback(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    mprAssert(0);
    return 0;
}


/*
 *  Return an absolute unix style path name for the file (not drive spec)
 *
 *  function get unixPath(): String
 */
static EjsVar *unixPath(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsVar  *vp;
    char    *path;

    path = mprGetUnixFilename(fp, fp->path);
    vp = (EjsVar*) ejsCreateString(ejs, path);
    mprFree(path);

    return (EjsVar*) vp;
}


/*
 *  Write data to the file
 *
 *  function write(data: Object): Number
 */
EjsVar *fileWrite(Ejs *ejs, EjsFile *fp, int argc, EjsVar **argv)
{
    EjsArray        *args;
    EjsByteArray    *ap;
    EjsVar          *vp;
    EjsString       *str;
    char            *buf;
    int             i, len, written;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = (EjsArray*) argv[0];

    if (!(fp->mode & EJS_FILE_WRITE)) {
        ejsThrowStateError(ejs, "File not opened for writing");
        return 0;
    }

    written = 0;

    for (i = 0; i < args->length; i++) {
        vp = ejsGetProperty(ejs, (EjsVar*) args, i);
        mprAssert(vp);
        switch (vp->type->id) {
        case ES_ByteArray:
            //  TODO - should have function for this
            ap = (EjsByteArray*) vp;
            buf = (char*) &ap->value[ap->readPosition];
            len = ap->writePosition - ap->readPosition;
            break;

        case ES_String:
            buf = ((EjsString*) vp)->value;
            len = ((EjsString*) vp)->length;
            break;

        default:
            str = ejsToString(ejs, vp);
            buf = ejsGetString(str);
            len = str->length;
            break;
        }
        if (mprWrite(fp->file, buf, len) != len) {
            ejsThrowIOError(ejs, "Can't write to %s", fp->path);
            return 0;
        }
        written += len;
    }

    return (EjsVar*) ejsCreateNumber(ejs, written);
}


/*********************************** Support **********************************/

static int readData(Ejs *ejs, EjsFile *fp, EjsByteArray *ap, int offset, int count)
{
    char    buf[MPR_BUFSIZE];
    int     bufsize, totalRead, len, bytes;

    bufsize = ap->length - offset;

    for (totalRead = 0; count > 0; ) {
        len = min(count, bufsize);
        bytes = mprRead(fp->file, buf, bufsize);
        if (bytes < 0) {
            ejsThrowIOError(ejs, "Error reading from %s", fp->path);
        } else if (bytes == 0) {
            break;
        }
        mprMemcpy(&ap->value[offset], ap->length - offset, buf, bytes);
        count -= bytes;
        offset += bytes;
        totalRead += bytes;
    }
    return totalRead;
}


#if BLD_FEATURE_MMU && FUTURE
static void *mapFile(EjsFile *fp, uint size, int mode)
{
    Mpr         *mpr;
    void        *ptr;
    int x;

    mpr = mprGetMpr(fp);
    x = ~(mpr->alloc.pageSize - 1);
    size = (size + mpr->alloc.pageSize - 1) & ~(mpr->alloc.pageSize - 1);
#if MACOSX || LINUX || FREEBSD
    //  USE MAP_SHARED instead of MAP_PRIVATE if opened for write
    ptr = mmap(0, size, mode, MAP_FILE | MAP_PRIVATE, fp->file->fd, 0);
    x = errno;
#else
    ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, mapProt(mode));
#endif

    if (ptr == 0) {
        mprSetAllocError(mpr);
        return 0;
    }
    return ptr;
}


static void unmapFile(EjsFile *fp)
{
#if MACOSX || LINUX || FREEBSD
    munmap(fp->mapped, fp->info.size);
#else
    VirtualFree(file->mapped, 0, MEM_RELEASE);
#endif
}

#endif

/*********************************** Factory **********************************/

EjsFile *ejsCreateFile(Ejs *ejs, cchar *path)
{
    EjsFile     *fp;
    EjsVar      *arg;

    fp = (EjsFile*) ejsCreateVar(ejs, ejsGetType(ejs, ES_ejs_io_File), 0);
    if (fp == 0) {
        return 0;
    }

    arg = (EjsVar*) ejsCreateString(ejs, path);
    fileConstructor(ejs, fp, 1, (EjsVar**) &arg);

    return fp;
}


int ejsCreateFileType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, "ejs.io", "File"), ejs->objectType, sizeof(EjsFile), ES_ejs_io_File,
        ES_ejs_io_File_NUM_CLASS_PROP, ES_ejs_io_File_NUM_INSTANCE_PROP, 
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_HAS_CONSTRUCTOR | EJS_ATTR_OBJECT_HELPERS);
    if (type == 0) {
        return EJS_ERR;
    }

    /*
     *  Define the helper functions.
     */
    type->helpers->getProperty = (EjsGetPropertyHelper) getFileProperty;
    type->helpers->setProperty = (EjsSetPropertyHelper) setFileProperty;

    //  TODO - Need attribute for this */
    type->numericIndicies = 1;
    return 0;
}


int ejsConfigureFileType(Ejs *ejs)
{
    EjsType     *type;
    int         rc;

    type = ejsGetType(ejs, ES_ejs_io_File);

    rc = 0;
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_File, (EjsNativeFunction) fileConstructor);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_absolutePath, (EjsNativeFunction) absolutePath);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_basename, (EjsNativeFunction) basenameProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_close, (EjsNativeFunction) closeFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_copy, (EjsNativeFunction) copyFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_createTempFile, (EjsNativeFunction) createTempFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_created, (EjsNativeFunction) created);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_pathDelimiter, (EjsNativeFunction) pathDelimiter);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_set_pathDelimiter, (EjsNativeFunction) setPathDelimiter);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_dirname, (EjsNativeFunction) dirnameProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_exists, (EjsNativeFunction) exists);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_extension, (EjsNativeFunction) extension);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_freeSpace, (EjsNativeFunction) freeSpace);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_flush, (EjsNativeFunction) flushFile);
    rc += ejsBindMethod(ejs, type, ES_Object_get, (EjsNativeFunction) getFileIterator);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_getBytes, (EjsNativeFunction) getBytes);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_getLines, (EjsNativeFunction) getLines);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_getString, (EjsNativeFunction) getFileAsString);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_getXML, (EjsNativeFunction) getXML);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_getFiles, (EjsNativeFunction) getFiles);
    rc += ejsBindMethod(ejs, type, ES_Object_getValues, (EjsNativeFunction) getValues);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_hasDriveSpec, (EjsNativeFunction) hasDriveSpec);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_isDir, (EjsNativeFunction) isDir);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_isOpen, (EjsNativeFunction) isOpen);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_isRegular, (EjsNativeFunction) isRegular);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_lastAccess, (EjsNativeFunction) lastAccess);
    rc += ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) fileLength);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_makeDir, (EjsNativeFunction) makeDir);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_mode, (EjsNativeFunction) mode);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_modified, (EjsNativeFunction) modified);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_name, (EjsNativeFunction) name);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_newline, (EjsNativeFunction) newline);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_set_newline, (EjsNativeFunction) setNewline);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_open, (EjsNativeFunction) openProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_parent, (EjsNativeFunction) parent);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_permissions, (EjsNativeFunction) getPermissions);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_set_permissions, (EjsNativeFunction) setPermissions);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_position, (EjsNativeFunction) getPosition);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_set_position, (EjsNativeFunction) setPosition);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_put, (EjsNativeFunction) putToFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_relativePath, (EjsNativeFunction) relativePath);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_readBytes, (EjsNativeFunction) readBytes);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_read, (EjsNativeFunction) readProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_remove, (EjsNativeFunction) removeFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_removeDir, (EjsNativeFunction) removeDir);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_rename, (EjsNativeFunction) renameProc);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_setCallback, (EjsNativeFunction) setFileCallback);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_unixPath, (EjsNativeFunction) unixPath);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_File_write, (EjsNativeFunction) fileWrite);

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
