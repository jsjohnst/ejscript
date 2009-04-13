/*
 *  dumpext.c - Dump external symbols from a COFF object file.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/************************************ Includes ********************************/
/*
 *  Suppress MS VS warnings
 */
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <string.h>

#define e_magic_number IMAGE_FILE_MACHINE_I386

/*
 * Break up a line into argc argv
 */
int GetArgcArgv(char *s, char **argv)
{
    int quote = 0;
    int argc = 0;
    char *bp;

    bp = s;
    while (1) {
        while (isspace(*bp)) {
            bp++;
        }
        if (*bp == '\n' || *bp == '\0') {
            *bp = '\0';
            return argc;
        }
        if (*bp == '\"') {
            quote = 1;
            bp++;
        }
        argv[argc++] = bp;

        while (*bp != '\0') {
            if (quote) {
                if (*bp == '\"') {
                    quote = 0;
                    *bp = '\0';
                    bp++;
                    break;
                }
                bp++;
                continue;
            }
            if (isspace(*bp)) {
                *bp = '\0';
                bp++;
                break;
            }
            bp++;
        }
    }
}


/*
 *  The names of the first group of possible symbol table storage classes
 */
char * SzStorageClass1[] = {
    "NULL","AUTOMATIC","EXTERNAL","STATIC","REGISTER","EXTERNAL_DEF","LABEL",
    "UNDEFINED_LABEL","MEMBER_OF_STRUCT","ARGUMENT","STRUCT_TAG",
    "MEMBER_OF_UNION","UNION_TAG","TYPE_DEFINITION","UNDEFINED_STATIC",
    "ENUM_TAG","MEMBER_OF_ENUM","REGISTER_PARAM","BIT_FIELD"
};


/*
 * The names of the second group of possible symbol table storage classes
 */
char * SzStorageClass2[] = {
    "BLOCK","FUNCTION","END_OF_STRUCT","FILE","SECTION","WEAK_EXTERNAL"
};


/*
 *  Given a symbol storage class value, return a descriptive ASCII string
 */
PSTR GetSZStorageClass(BYTE storageClass)
{
    if (storageClass <= IMAGE_SYM_CLASS_BIT_FIELD)
        return SzStorageClass1[storageClass];
    else if ((storageClass >= IMAGE_SYM_CLASS_BLOCK) && (storageClass <= IMAGE_SYM_CLASS_WEAK_EXTERNAL))
        return SzStorageClass2[storageClass-IMAGE_SYM_CLASS_BLOCK];
    else
        return "???";
}


/*
 *  Used by DumpSymbolTable, it gives meaningful names to the non-normal section number.
 *  Results: A name is returned in buffer
 */
void GetSectionName(WORD section, PSTR buffer, unsigned cbBuffer)
{
    char tempbuffer[10];

    switch ((SHORT)section) {
        case IMAGE_SYM_UNDEFINED: strcpy(tempbuffer, "UNDEF"); break;
        case IMAGE_SYM_ABSOLUTE:  strcpy(tempbuffer, "ABS  "); break;
        case IMAGE_SYM_DEBUG:   strcpy(tempbuffer, "DEBUG"); break;
        default: sprintf(tempbuffer, "%-5X", section);
    }
    strncpy(buffer, tempbuffer, cbBuffer-1);
}


/*
 *  Dumps a COFF symbol table from an EXE or OBJ.  We only use it to dump tables from OBJs.
 */
void DumpSymbolTable(PIMAGE_SYMBOL pSymbolTable, FILE *fout, unsigned cSymbols)
{
    unsigned i;
    PSTR stringTable;
    char sectionName[10];

    fprintf(fout, "Symbol Table - %X entries  (* = auxillary symbol)\n", cSymbols);

    fprintf(fout, 
     "Indx Name                 Value    Section    cAux  Type    Storage\n"
     "---- -------------------- -------- ---------- ----- ------- --------\n");

    /*
     * The string table apparently starts right after the symbol table
     */
    stringTable = (PSTR)&pSymbolTable[cSymbols]; 

    for (i=0; i < cSymbols; i++) {
        fprintf(fout, "%04X ", i);
        if (pSymbolTable->N.Name.Short != 0)
            fprintf(fout, "%-20.8s", pSymbolTable->N.ShortName);
        else
            fprintf(fout, "%-20s", stringTable + pSymbolTable->N.Name.Long);

        fprintf(fout, " %08X", pSymbolTable->Value);

        GetSectionName(pSymbolTable->SectionNumber, sectionName,
                       sizeof(sectionName));
        fprintf(fout, " sect:%s aux:%X type:%02X st:%s\n",
               sectionName, pSymbolTable->NumberOfAuxSymbols,
               pSymbolTable->Type, GetSZStorageClass(pSymbolTable->StorageClass) );

        /*
         * Take into account any aux symbols
         */
        i += pSymbolTable->NumberOfAuxSymbols;
        pSymbolTable += pSymbolTable->NumberOfAuxSymbols;
        pSymbolTable++;
    }
}


/*
 *  Dumps a COFF symbol table from an EXE or OBJ.  We only use it to dump tables from OBJs.
 */
void DumpExternals(PIMAGE_SYMBOL pSymbolTable, FILE *fout, unsigned cSymbols)
{
    unsigned i;
    PSTR stringTable;
    char *s;
    char symbol[1024];
    int len;

    /*
     * The string table apparently starts right after the symbol table
     */
    stringTable = (PSTR)&pSymbolTable[cSymbols]; 

    for (i=0; i < cSymbols; i++) {
        /* fprintf(fout, "%04X ", i); */
        /*
         *  Require pSymbolTable->Type == 0x20  for only functions
         */
        if ((pSymbolTable->SectionNumber > 0 && pSymbolTable->Type == 0x20) &&
                pSymbolTable->StorageClass == IMAGE_SYM_CLASS_EXTERNAL) {

            if (pSymbolTable->N.Name.Short != 0) {
                strncpy(symbol, (char*) pSymbolTable->N.ShortName, 8);
                symbol[8] = 0;
            } else {
                strcpy(symbol, stringTable + pSymbolTable->N.Name.Long);
            }
            s = symbol;
            if (*s == '_') {
                s++;
            }
            if (strncmp(s, "??_G", 4) == 0) {
                goto next;
            }
            if (strncmp(s, "??_E", 4) == 0) {
                goto next;
            }
            len = strlen(s);
            if (s[len - 1] == '2' && s[len - 2] == '1' && s[len - 3] == '@') {
                s[len - 3] = '\0';
            }
            fprintf(fout, "\t%s\n", s);
        }
next:

        /*
         * Take into account any aux symbols
         */
        i += pSymbolTable->NumberOfAuxSymbols;
        pSymbolTable += pSymbolTable->NumberOfAuxSymbols;
        pSymbolTable++;
    }
}


/*
 *  Dump an object file--either a full listing or just the exported symbols.
 */
void DumpObjFile(PIMAGE_FILE_HEADER pImageFileHeader, FILE *fout, int full)
{
    PIMAGE_SYMBOL PCOFFSymbolTable;
    DWORD COFFSymbolCount;

    PCOFFSymbolTable = (PIMAGE_SYMBOL)
        ((DWORD)pImageFileHeader + pImageFileHeader->PointerToSymbolTable);
    COFFSymbolCount = pImageFileHeader->NumberOfSymbols;

    if (full) {
        DumpSymbolTable(PCOFFSymbolTable, fout, COFFSymbolCount);
    } else {
        DumpExternals(PCOFFSymbolTable, fout, COFFSymbolCount);
    }
}


/*
 *  Skip over the current ROMF record and return the type of the next record.
 */

BYTE SkipToNextRecord(BYTE **ppBuffer)
{
    WORD *wp;
    int length;

    (*ppBuffer)++;      /* Skip over the type.*/

    /*
     *  Fix compiler warnings
     */
    wp = (WORD*) *ppBuffer;
    length = *wp++;

    *ppBuffer += length;    /* Skip over the rest. */
    return **ppBuffer;      /* Return the type. */
}


/*
 *  Dump a Relocatable Object Module Format file, displaying only the exported symbols.
 */
void DumpROMFObjFile(LPVOID pBuffer, FILE *fout)
{
    BYTE type, length, *bp;
    char symbol[1024];

    while (1) {
        bp = (BYTE*) pBuffer;
        type = SkipToNextRecord(&bp);
        if (type == 0x90) { /* PUBDEF */
            if (((BYTE*)pBuffer)[4] != 0) {
                length = ((BYTE*)pBuffer)[5];
                strncpy(symbol, ((char*)pBuffer) + 6, length);
                symbol[length] = '\0';
                fprintf(fout, "\t%s\n", symbol);
            }
        } else if (type == 0x8B || type == 0x8A) { /* MODEND */
            break;
        }
    }
}


/*
 *  Open up a file, memory map it, and call the appropriate dumping routine
 */
void DumpFile(LPSTR filename, FILE *fout, int full)
{
    HANDLE hFile;
    HANDLE hFileMapping;
    LPVOID lpFileBase;
    PIMAGE_DOS_HEADER dosHeader;

    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Couldn't open file with CreateFile(%s)\n", filename);
        return;
    }

    hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hFileMapping == 0) {
        CloseHandle(hFile);
        fprintf(stderr, "Couldn't open file mapping with CreateFileMapping()\n");
        return;
    }

    lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
    if (lpFileBase == 0) {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        fprintf(stderr, "Couldn't map view of file with MapViewOfFile()\n");
        return;
    }

    dosHeader = (PIMAGE_DOS_HEADER)lpFileBase;
    if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
        fprintf(stderr, "File is an executable.  I don't dump those.\n");
        return;
    }
    /* Does it look like a i386 COFF OBJ file??? */
    else if ((dosHeader->e_magic == e_magic_number)
            && (dosHeader->e_sp == 0)) {
        /*
         * The two tests above aren't what they look like.  They're
         * really checking for IMAGE_FILE_HEADER.Machine == i386 (0x14C)
         * and IMAGE_FILE_HEADER.SizeOfOptionalHeader == 0;
         */
        DumpObjFile((PIMAGE_FILE_HEADER) lpFileBase, fout, full);
    } else if (*((BYTE *)lpFileBase) == 0x80) {
        /*
         * This file looks like it might be a ROMF file.
         */
        DumpROMFObjFile(lpFileBase, fout);
    } else {
        printf("unrecognized file format\n");
    }
    UnmapViewOfFile(lpFileBase);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);
}


void main(int argc, char **argv)
{
    char *fargv[1000];
    char cmdline[10000];
    int i, arg;
    FILE *fout;
    int pos, len;
    int full = 0;
    char *outfile = NULL;

    if (argc < 3) {
Usage:
        fprintf(stderr, "Usage: %s ?-o outfile? ?-f(ull)? <dllname> <object filenames> ..\n", argv[0]);
        exit(1);
    }

    arg = 1;
    while (argv[arg][0] == '-') {
        if (strcmp(argv[arg], "--") == 0) {
            arg++;
            break;
        } else if (strcmp(argv[arg], "-f") == 0) {
            full = 1;
        } else if (strcmp(argv[arg], "-o") == 0) {
            arg++;
            if (arg == argc) {
                goto Usage;
            }
            outfile = argv[arg];
        }
        arg++;
    }
    if (arg == argc) {
        goto Usage;
    }

    if (outfile) {
        fout = fopen(outfile, "w+");
        if (fout == NULL) {
            fprintf(stderr, "Unable to open \'%s\' for writing:\n",
                    argv[arg]);
            perror("");
            exit(1);
        }
    } else {
        fout = stdout;
    }

    if (! full) {
        char *dllname = argv[arg];
        arg++;
        if (arg == argc) {
            goto Usage;
        }
        fprintf(fout, "LIBRARY    %s\n", dllname);
        fprintf(fout, "EXPORTS\n");
    }

    for (; arg < argc; arg++) {
        if (argv[arg][0] == '@') {
            FILE *fargs = fopen(&argv[arg][1], "r");
            if (fargs == NULL) {
                fprintf(stderr, "Unable to open \'%s\' for reading:\n",
                        argv[arg]);
                perror("");
                exit(1);
            }
            pos = 0;
            for (i = 0; i < arg; i++) {
                strcpy(&cmdline[pos], argv[i]);
                pos += strlen(&cmdline[pos]) + 1;
                fargv[i] = argv[i];
            }
            while (fgets(&cmdline[pos], sizeof(cmdline) - pos, fargs) != 0) {
                len = strlen(&cmdline[pos]);
                cmdline[pos + len - 1] = ' ';
                i += GetArgcArgv(&cmdline[pos], &fargv[i]);
                pos += len;
            }
            fclose(fargs);
            argc = i;
            argv = fargv;
        }
        DumpFile(argv[arg], fout, full);
    }
    exit(0);
}

/*
 *  @copy   custom
 *  
 *  Original Author: Matt Pietrek, Gordon Chaffee, Scott Stanton
 *
 *  The latest version of this code is available at http://www.embedthis.com
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1994-2009. All Rights Reserved.
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
