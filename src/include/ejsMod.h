/**
 *  ejsMod.h - Header for the ejsmod: module file list and slot generation program.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

//  TODO - move this back to ejsmod
/********************************** Includes **********************************/

#ifndef _h_EJS_MOD
#define _h_EJS_MOD 1

#include    "ejs.h"

/*********************************** Defines **********************************/
/*
 *  Listing record structure
 */
typedef struct Lst {
    int         kind;                       /* Record kind */
    EjsModule   *module;                    /* Module holding class, function or property */
    EjsType     *type;                      /* Class reference */
    EjsVar      *owner;                     /* Owner (type, function, block) for the element */
    EjsFunction *fun;                       /* Relevant function */
    int         numSlots;                   /* Number of slots*/
    int         slotNum;                    /* Slot number */
    int         attributes;                 /* Property attributes */
    char        *name;                      /* General name */
    EjsName     typeName;                   /* Property type name */
    EjsName     qname;                      /* Qualified Property name */
    char        *url;                       /* Relevant url */
} Lst;


/*
 *  Mod manager control structure
 */
typedef struct EjsMod
{
    char        *currentLine;               /* Current input source code line */
    int         currentLineNumber;          /* Current input source line number */
    char        *currentFileName;           /* Current input file name */

    EjsService  *vmService;                 /* VM runtime */
    Ejs         *ejs;                       /* Interpreter handle */
    
    MprList     *lstRecords;                /* Listing records */
    MprList     *packages;                  /* List of packages */
    
    MprList     *blocks;                    /* List of blocks */
    EjsBlock    *currentBlock;              /* Current lexical block being read */

    int         cslots;                     /* Create C slot definitions */
    int         empty;                      /* Create empty interpreter */
    int         exitOnError;                /* Exit if module file errors are detected */
    int         genSlots;                   /* Set if either cslots || jsslots */
    int         jslots;                     /* Create Java slot definitions */
    int         listing;                    /* Generate listing file */
    int         showBuiltin;                /* Show native properties */
    int         showDebug;                  /* Show debug instructions */
    int         verbosity;                  /* Verbosity level */
    int         warnOnError;                /* Warn if module file errors are detected */

    char        *docDir;                    /* Directory to generate HTML doc */
    bool        html;                       /* Generate HTML doc */
    bool        xml;                        /* Generate XML doc */
    
    int         error;                      /* Unresolved error */
    int         fatalError;                 /* Any a fatal error - Can't continue */
    int         memError;                   /* Memory error */
    
    int         errorCount;                 /* Count of all errors */
    int         warningCount;               /* Count of all warnings */
    int         showAsm;                    /* Display assember bytes */
    
    MprFile     *file;                      /* Current output file handle */
    EjsModule   *module;                    /* Current unit */
    EjsFunction *fun;                       /* Current function to disassemble */
    
    uchar       *pc;

#if UNUSED
    union {
        uchar       *pc;
        ushort      *spc;
        uint        *ipc;
#if BLD_FEATURE_FLOATING_POINT
        double      *dpc;
#endif
        uint64      *idpc;
    };
#endif

} EjsMod;


#if BLD_FEATURE_EJS_DOC
typedef struct DocFile {
    cchar           *path;              /* File path */
    uchar           *data;              /* Pointer to file data */
    int             size;               /* Size of file */
    int             inode;              /* Not used */
} DocFile;

extern DocFile docFiles[];
#endif

/********************************** Prototypes *******************************/

extern void emListingLoadCallback(Ejs *ejs, int kind, ...);
extern void emnDocLoadCallback(Ejs *ejs, int kind, ...);
extern int  emCreateSlotFiles(EjsMod *mp, EjsModule *up);
extern int  emCreateDoc(EjsMod *mp);

#endif /* _h_EJS_MOD */

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
