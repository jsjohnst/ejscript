/**
 *  listing.c - Assembler listing generator.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejsMod.h"

/********************************** Local Data ********************************/
/*
 *  Stack effect special values
 */
#define UNIMP           100             /* Not implemented */
#define POP1            101             /* Operand 1 specifies the stack change (pop) */

/*
 *  Operands
 */
#undef NONE
#define NONE            0x0             /* No operands */
#define BYTE            0x1             /* 8 bit integer */
#define SHORT           0x2             /* 16 bit integer */
#define WORD            0x4             /* 32 bits */
#define LONG            0x8             /* 64 bit integer */
#define DOUBLE          0x10            /* 64 bit floating */
#define NUM             0x40            /* Encoded integer */
#define STRING          0x80            /* Interned string as an encoded integer*/
#define GLOBAL          0x100           /* Encode global */
#define SLOT            0x200           /* Slot number as an encoded integer */
#define JMP             0x1000          /* 32 bit jump offset */
#define JMP8            0x2000          /* 8 bit jump offset */
#define VARCOUNT        0x4000          /* <count> <offset32> ... */
#define INIT_DEFAULT    0x8000          /* Computed goto table, 32 bit jumps  */
#define INIT_DEFAULT8   0x10000         /* Computed goto table, 8 bit jumps */
#define ARGC            0x20000         /* Argument count */
#define ARGC2           0x40000         /* Argument count  * 2*/

//  TODO - convert STRING, STRING to QNAME

typedef struct Optable {
    char    *name;
    int     stackEffect;
    int     args[8];
} Optable;

/*  Opcode string                           Stack Effect    Operands, ...                                   */
Optable optable[] = {
    {   "Add",                                      -1,         { NONE,                                         },},
    {   "AddNamespace",                             0,          { STRING,                                       },},
    {   "AddNamespaceRef",                          -1,         { NONE,                                         },},
    {   "And",                                      -1,         { NONE,                                         },},
    {   "BranchEQ",                                 -1,         { JMP,                                          },},
    {   "BranchStrictlyEQ",                         -1,         { JMP,                                          },},
    {   "BranchFalse",                              -1,         { JMP,                                          },},
    {   "BranchGE",                                 -1,         { JMP,                                          },},
    {   "BranchGT",                                 -1,         { JMP,                                          },},
    {   "BranchLE",                                 -1,         { JMP,                                          },},
    {   "BranchLT",                                 -1,         { JMP,                                          },},
    {   "BranchNE",                                 -1,         { JMP,                                          },},
    {   "BranchStrictlyNE",                         -1,         { JMP,                                          },},
    {   "BranchNull",                               -1,         { JMP,                                          },},
    {   "BranchNotZero",                            -1,         { JMP,                                          },},
    {   "BranchTrue",                               -1,         { JMP,                                          },},
    {   "BranchUndefined",                          -1,         { JMP,                                          },},
    {   "BranchZero",                               -1,         { JMP,                                          },},
    {   "BranchFalse.8",                            -1,         { JMP8,                                         },},
    {   "BranchTrue.8",                             -1,         { JMP8,                                         },},
    {   "Breakpoint",                               0,          { NUM, STRING,                                  },},
    {   "Call",                                     -2,         { ARGC,                                         },},
    {   "CallGlobalSlot",                           0,          { SLOT, ARGC,                                   },},
    {   "CallObjSlot",                              -1,         { SLOT, ARGC,                                   },},
    {   "CallThisSlot",                             0,          { SLOT, ARGC,                                   },},
    {   "CallBlockSlot",                            0,          { SLOT, NUM, ARGC,                              },},
    {   "CallObjInstanceSlot",                      -1,         { SLOT, ARGC,                                   },},
    {   "CallObjStaticSlot",                        -1,         { SLOT, NUM, ARGC,                              },},
    {   "CallThisStaticSlot",                       0,          { SLOT, NUM, ARGC,                              },},
    {   "CallObjName",                              -1,         { STRING, STRING, ARGC,                         },},
    {   "CallScopedName",                           0,          { STRING, STRING, ARGC,                         },},
    {   "CallConstructor",                          0,          { ARGC,                                         },},
    {   "CallNextConstructor",                      0,          { ARGC,                                         },},
    {   "Cast",                                     -1,         { NONE,                                         },},
    {   "CastBoolean",                              0,          { NONE,                                         },},
    {   "CloseBlock",                               0,          { NONE,                                         },},
    {   "CloseWith",                                0,          { NONE,                                         },},
    {   "CompareEQ",                                -1,         { NONE,                                         },},
    {   "CompareStrictlyEQ",                        -1,         { NONE,                                         },},
    {   "CompareFalse",                             -1,         { NONE,                                         },},
    {   "CompareGE",                                -1,         { NONE,                                         },},
    {   "CompareGT",                                -1,         { NONE,                                         },},
    {   "CompareLE",                                -1,         { NONE,                                         },},
    {   "CompareLT",                                -1,         { NONE,                                         },},
    {   "CompareNE",                                -1,         { NONE,                                         },},
    {   "CompareStrictlyNE",                        -1,         { NONE,                                         },},
    {   "CompareNull",                              -1,         { NONE,                                         },},
    {   "CompareNotZero",                           -1,         { NONE,                                         },},
    {   "CompareTrue",                              -1,         { NONE,                                         },},
    {   "CompareUndefined",                         -1,         { NONE,                                         },},
    {   "CompareZero",                              -1,         { NONE,                                         },},
    {   "Debug",                                    0,          { NUM, STRING,                                  },},
    {   "DefineClass",                              0,          { GLOBAL,                                       },},
    {   "DefineFunction",                           0,          { SLOT, NUM,                                    },},
    {   "DefineGlobalFunction",                     0,          { GLOBAL,                                       },},
    {   "DeleteNameExp",                            -2,         { NONE,                                         },},
    {   "Delete",                                   -1,         { STRING, STRING,                               },},
    {   "DeleteName",                               0,          { STRING, STRING,                               },},
    {   "Div",                                      -1,         { NONE,                                         },},
    {   "Dup",                                      1,          { NONE,                                         },},
    {   "Dup2",                                     2,          { NONE,                                         },},
    {   "EndCode",                                  0,          { NONE,                                         },},
    {   "EndException",                             0,          { NONE,                                         },},
    {   "Goto",                                     0,          { JMP,                                          },},
    {   "Goto.8",                                   0,          { JMP8,                                         },},
    {   "Inc",                                      0,          { BYTE,                                         },},
    {   "InitDefaultArgs",                          0,          { INIT_DEFAULT,                                 },},
    {   "InitDefaultArgs.8",                        0,          { INIT_DEFAULT8,                                },},
    {   "InstOf",                                   -1,         { NONE,                                         },},
    {   "IsA",                                      -1,         { NONE,                                         },},
    {   "Load0",                                    1,          { NONE,                                         },},
    {   "Load1",                                    1,          { NONE,                                         },},
    {   "Load2",                                    1,          { NONE,                                         },},
    {   "Load3",                                    1,          { NONE,                                         },},
    {   "Load4",                                    1,          { NONE,                                         },},
    {   "Load5",                                    1,          { NONE,                                         },},
    {   "Load6",                                    1,          { NONE,                                         },},
    {   "Load7",                                    1,          { NONE,                                         },},
    {   "Load8",                                    1,          { NONE,                                         },},
    {   "Load9",                                    1,          { NONE,                                         },},
    {   "LoadDouble",                               1,          { DOUBLE,                                       },},
    {   "LoadFalse",                                1,          { NONE,                                         },},
    {   "LoadGlobal",                               1,          { NONE,                                         },},
    {   "LoadInt.16",                               1,          { SHORT,                                        },},
    {   "LoadInt.32",                               1,          { WORD,                                         },},
    {   "LoadInt.64",                               1,          { LONG,                                         },},
    {   "LoadInt.8",                                1,          { BYTE,                                         },},
    {   "LoadM1",                                   1,          { NONE,                                         },},
    {   "LoadName",                                 1,          { STRING, STRING,                               },},
    {   "LoadNamespace",                            1,          { STRING,                                       },},
    {   "LoadNull",                                 1,          { NONE,                                         },},
    {   "LoadRegexp",                               1,          { STRING,                                       },},
    {   "LoadString",                               1,          { STRING,                                       },},
    {   "LoadThis",                                 1,          { NONE,                                         },},
    {   "LoadTrue",                                 1,          { NONE,                                         },},
    {   "LoadUndefined",                            1,          { NONE,                                         },},
    {   "LoadXML",                                  1,          { STRING,                                       },},
    {   "GetLocalSlot_0",                           1,          { NONE,                                         },},
    {   "GetLocalSlot_1",                           1,          { NONE,                                         },},
    {   "GetLocalSlot_2",                           1,          { NONE,                                         },},
    {   "GetLocalSlot_3",                           1,          { NONE,                                         },},
    {   "GetLocalSlot_4",                           1,          { NONE,                                         },},
    {   "GetLocalSlot_5",                           1,          { NONE,                                         },},
    {   "GetLocalSlot_6",                           1,          { NONE,                                         },},
    {   "GetLocalSlot_7",                           1,          { NONE,                                         },},
    {   "GetLocalSlot_8",                           1,          { NONE,                                         },},
    {   "GetLocalSlot_9",                           1,          { NONE,                                         },},
    {   "GetObjSlot_0",                             1,          { NONE,                                         },},
    {   "GetObjSlot_1",                             1,          { NONE,                                         },},
    {   "GetObjSlot_2",                             1,          { NONE,                                         },},
    {   "GetObjSlot_3",                             1,          { NONE,                                         },},
    {   "GetObjSlot_4",                             1,          { NONE,                                         },},
    {   "GetObjSlot_5",                             1,          { NONE,                                         },},
    {   "GetObjSlot_6",                             1,          { NONE,                                         },},
    {   "GetObjSlot_7",                             1,          { NONE,                                         },},
    {   "GetObjSlot_8",                             1,          { NONE,                                         },},
    {   "GetObjSlot_9",                             1,          { NONE,                                         },},
    {   "GetThisSlot_0",                            1,          { NONE,                                         },},
    {   "GetThisSlot_1",                            1,          { NONE,                                         },},
    {   "GetThisSlot_2",                            1,          { NONE,                                         },},
    {   "GetThisSlot_3",                            1,          { NONE,                                         },},
    {   "GetThisSlot_4",                            1,          { NONE,                                         },},
    {   "GetThisSlot_5",                            1,          { NONE,                                         },},
    {   "GetThisSlot_6",                            1,          { NONE,                                         },},
    {   "GetThisSlot_7",                            1,          { NONE,                                         },},
    {   "GetThisSlot_8",                            1,          { NONE,                                         },},
    {   "GetThisSlot_9",                            1,          { NONE,                                         },},
    {   "GetScopedName",                            1,          { STRING, STRING,                               },},
    {   "GetObjName",                               0,          { STRING, STRING,                               },},
    {   "GetObjNameExpr",                           -1,         { NONE,                                         },},
    {   "GetBlockSlot",                             1,          { SLOT, NUM,                                    },},
    {   "GetGlobalSlot",                            1,          { BYTE,                                         },},
    {   "GetLocalSlot",                             1,          { SLOT,                                         },},
    {   "GetObjSlot",                               0,          { SLOT,                                         },},
    {   "GetThisSlot",                              1,          { SLOT,                                         },},
    {   "GetTypeSlot",                              0,          { SLOT, NUM,                                    },},
    {   "GetThisTypeSlot",                          1,          { SLOT, NUM,                                    },},
    {   "In",                                       -1,         { NONE,                                         },},
    {   "Like",                                     -1,         { NONE,                                         },},
    {   "LogicalNot",                               0,          { NONE,                                         },},
    {   "Mul",                                      -1,         { NONE,                                         },},
    {   "Neg",                                      0,          { NONE,                                         },},
    {   "New",                                      0,          { NONE,                                         },},
    {   "NewArray",                                 1,          { GLOBAL, ARGC2,                                },},
    {   "NewObject",                                1,          { GLOBAL, ARGC2,                                },},
    {   "Nop",                                      0,          { NONE,                                         },},
    {   "Not",                                      0,          { NONE,                                         },},
    {   "OpenBlock",                                0,          { SLOT, NUM,                                    },},
    {   "OpenWith",                                 1,          { NONE,                                         },},
    {   "Or",                                       -1,         { NONE,                                         },},
    {   "Pop",                                      -1,         { NONE,                                         },},
    {   "PopItems",                                 POP1,       { BYTE,                                         },},
    {   "PushCatchArg",                             1,          { NONE,                                         },},
    {   "PushResult",                               1,          { NONE,                                         },},
    {   "PutLocalSlot_0",                           -1,         { NONE,                                         },},
    {   "PutLocalSlot_1",                           -1,         { NONE,                                         },},
    {   "PutLocalSlot_2",                           -1,         { NONE,                                         },},
    {   "PutLocalSlot_3",                           -1,         { NONE,                                         },},
    {   "PutLocalSlot_4",                           -1,         { NONE,                                         },},
    {   "PutLocalSlot_5",                           -1,         { NONE,                                         },},
    {   "PutLocalSlot_6",                           -1,         { NONE,                                         },},
    {   "PutLocalSlot_7",                           -1,         { NONE,                                         },},
    {   "PutLocalSlot_8",                           -1,         { NONE,                                         },},
    {   "PutLocalSlot_9",                           -1,         { NONE,                                         },},
    {   "PutObjSlot_0",                             -2,         { NONE,                                         },},
    {   "PutObjSlot_1",                             -2,         { NONE,                                         },},
    {   "PutObjSlot_2",                             -2,         { NONE,                                         },},
    {   "PutObjSlot_3",                             -2,         { NONE,                                         },},
    {   "PutObjSlot_4",                             -2,         { NONE,                                         },},
    {   "PutObjSlot_5",                             -2,         { NONE,                                         },},
    {   "PutObjSlot_6",                             -2,         { NONE,                                         },},
    {   "PutObjSlot_7",                             -2,         { NONE,                                         },},
    {   "PutObjSlot_8",                             -2,         { NONE,                                         },},
    {   "PutObjSlot_9",                             -2,         { NONE,                                         },},
    {   "PutThisSlot_0",                            -1,         { NONE,                                         },},
    {   "PutThisSlot_1",                            -1,         { NONE,                                         },},
    {   "PutThisSlot_2",                            -1,         { NONE,                                         },},
    {   "PutThisSlot_3",                            -1,         { NONE,                                         },},
    {   "PutThisSlot_4",                            -1,         { NONE,                                         },},
    {   "PutThisSlot_5",                            -1,         { NONE,                                         },},
    {   "PutThisSlot_6",                            -1,         { NONE,                                         },},
    {   "PutThisSlot_7",                            -1,         { NONE,                                         },},
    {   "PutThisSlot_8",                            -1,         { NONE,                                         },},
    {   "PutThisSlot_9",                            -1,         { NONE,                                         },},
    {   "PutObjNameExpr",                           -3,         { NONE,                                         },},
    {   "PutScopedNameExpr",                        -2,         { NONE,                                         },},
    {   "PutObjName",                               -2,         { STRING, STRING,                               },},
    {   "PutScopedName",                            -1,         { STRING, STRING,                               },},
    {   "PutBlockSlot",                             -1,         { SLOT, NUM,                                    },},
    {   "PutGlobalSlot",                            -1,         { BYTE,                                         },},
    {   "PutLocalSlot",                             -1,         { SLOT,                                         },},
    {   "PutObjSlot",                               -2,         { SLOT,                                         },},
    {   "PutThisSlot",                              -1,         { SLOT,                                         },},
    {   "PutTypeSlot",                              -2,         { SLOT, NUM,                                    },},
    {   "PutThisTypeSlot",                          -1,         { SLOT, NUM,                                    },},
    {   "Rem",                                      -1,         { NONE,                                         },},
    {   "Return",                                   0,          { NONE,                                         },},
    {   "ReturnValue",                              -1,         { NONE,                                         },},
    {   "SaveResult",                               -1,         { NONE,                                         },},
    {   "Shl",                                      -1,         { NONE,                                         },},
    {   "Shr",                                      -1,         { NONE,                                         },},
    {   "Sub",                                      -1,         { NONE,                                         },},
    {   "Super",                                    0,          { NONE,                                         },},
    {   "Swap",                                     0,          { NONE,                                         },},
    {   "Throw",                                    0,          { NONE,                                         },},
    {   "Ushr",                                     -1,         { NONE,                                         },},
    {   "Xor",                                      -1,         { NONE,                                         },},
    { 0,                                            0,          { NONE,                                         },},
};

/***************************** Forward Declarations ****************************/

static cchar *getBlockName(EjsMod *mp, EjsVar *block);
static uchar getByte(EjsMod *mp);
static ushort getShort(EjsMod *mp);
static uint getWord(EjsMod *mp);
static int64 getLong(EjsMod *mp);
#if BLD_FEATURE_FLOATING_POINT
static double getDouble(EjsMod *mp);
#endif
static int  getNum(EjsMod *dp);
static char *getString(EjsMod *dp);
static void getGlobal(EjsMod *mp, char *buf, int buflen);
static void lstSlotAssignments(EjsMod *mp, EjsModule *module, EjsVar *obj);
static char *getAttributeString(EjsMod *mp, int attributes);
static void interp(EjsMod *mp, EjsModule *module, EjsFunction *fun);
static void leadin(EjsMod *mp, EjsModule *module, int classDec, int inFunction);
static void lstBlock(EjsMod *mp, EjsModule *module, EjsVar *block, int slotNum, cchar *name, int numSlots);
static void lstClass(EjsMod *mp, EjsModule *module, int slotNum, EjsType *klass, int attributes);
static void lstClose(EjsMod *mp, MprList *modules);
static void lstDependency(EjsMod *mp, EjsModule *module, char *name, char *url);
static void lstEndModule(EjsMod *mp, EjsModule *module);
static void lstException(EjsMod *mp, EjsModule *module, EjsFunction *fun);
static void lstFunction(EjsMod *mp, EjsModule *module, EjsVar *block, int slotNum, EjsName qname, EjsFunction *fun, int attributes);
static int  lstOpen(EjsMod *mp, char *moduleFilename, EjsModuleHdr *hdr);
static void lstProperty(EjsMod *mp, EjsModule *module, EjsVar *block, int slotNum, EjsName qname, int attributes, EjsName typeName);
static void lstModule(EjsMod *mp, EjsModule *module);

/*********************************** Code *************************************/
/*
 *  Listing loader callback. This is invoked at key points when loading a module file.
 */
void emListingLoadCallback(Ejs *ejs, int kind, ...)
{
    va_list         args;
    EjsModuleHdr    *hdr;
    EjsMod          *mp;
    Lst             *lst;
    MprList         *modules;
    char            *name;

    va_start(args, kind);

    mp = ejs->userData;

    lst = mprAllocObjZeroed(mp, Lst);

    /*
     *  Decode the record type and create a list for later processing. We need to process
     *  after the loader has done fixup for forward type references.
     */
    switch (kind) {

    case EJS_SECT_BLOCK:
        lst->module = va_arg(args, EjsModule*);
        lst->owner = va_arg(args, EjsVar*);
        lst->slotNum = va_arg(args, int);
        lst->name = va_arg(args, char*);
        lst->numSlots = va_arg(args, int);
        break;

    case EJS_SECT_BLOCK_END:
        break;

    case EJS_SECT_CLASS:
        lst->module = va_arg(args, EjsModule*);
        lst->slotNum = va_arg(args, int);
        lst->qname = va_arg(args, EjsName);
        lst->type = va_arg(args, EjsType*);
        lst->attributes = va_arg(args, int);
        break;

    case EJS_SECT_CLASS_END:
        break;

    case EJS_SECT_DEPENDENCY:
        lst->module = va_arg(args, EjsModule*);
        lst->name = mprStrdup(lst, va_arg(args, char*));
        lst->url = mprStrdup(lst, va_arg(args, char*));
        break;

    case EJS_SECT_END:
        modules = va_arg(args, MprList*);
        lstClose(mp, modules);
        mprFree(lst);
        return;

    case EJS_SECT_EXCEPTION:
        lst->module = va_arg(args, EjsModule*);
        lst->fun = va_arg(args, EjsFunction*);
        break;

    case EJS_SECT_FUNCTION:
        lst->module = va_arg(args, EjsModule*);
        lst->owner = va_arg(args, EjsVar*);
        lst->slotNum = va_arg(args, int);
        lst->qname = va_arg(args, EjsName);
        lst->fun = va_arg(args, EjsFunction*);
        lst->attributes = va_arg(args, int);
        break;

    case EJS_SECT_FUNCTION_END:
        break;

    case EJS_SECT_START:
        name = va_arg(args, char*);
        hdr = va_arg(args, EjsModuleHdr*);
        lstOpen(mp, name, hdr);
        mprFree(lst);
        return;

    case EJS_SECT_PROPERTY:
        lst->module = va_arg(args, EjsModule*);
        lst->owner = va_arg(args, EjsVar*);
        lst->slotNum = va_arg(args, int);
        lst->qname = va_arg(args, EjsName);
        lst->attributes = va_arg(args, int);
        lst->typeName = va_arg(args, EjsName);
        break;

    case EJS_SECT_MODULE:
        break;

    case EJS_SECT_MODULE_END:
        break;

    default:
        mprAssert(0);
    }

    lst->kind = kind;
    mprAddItem(mp->lstRecords, lst);
}


/*
 *  Loader completion routine. Process listing records and emit the listing file.
 */
static void lstClose(EjsMod *mp, MprList *modules)
{
    Ejs         *ejs;
    EjsModule   *module;
    Lst         *lst;
    bool        headerOutput;
    int         next, nextModule, count;

    ejs = mp->ejs;

    for (nextModule = 0; (module = (EjsModule*) mprGetNextItem(modules, &nextModule)) != 0; ) {

        headerOutput = 0;
        count = 0;

        for (next = 0; (lst = (Lst*) mprGetNextItem(mp->lstRecords, &next)) != 0; ) {

            if (lst->module != module) {
                continue;
            }

            if (!headerOutput) {
                lstModule(mp, lst->module);
                headerOutput = 1;
            }

            switch (lst->kind) {
            case EJS_SECT_BLOCK:
                lstBlock(mp, lst->module, lst->owner, lst->slotNum, lst->name, lst->numSlots);
                count++;
                break;

            case EJS_SECT_CLASS:
                lstClass(mp, lst->module, lst->slotNum, lst->type, lst->attributes);
                count++;
                break;

            case EJS_SECT_DEPENDENCY:
                lstDependency(mp, lst->module, lst->name, lst->url);
                break;

            case EJS_SECT_EXCEPTION:
                lstException(mp, lst->module, lst->fun);
                break;

            case EJS_SECT_FUNCTION:
                lstFunction(mp, lst->module, lst->owner, lst->slotNum, lst->qname, lst->fun, lst->attributes);
                count++;
                break;

            case EJS_SECT_PROPERTY:
                lstProperty(mp, lst->module, lst->owner, lst->slotNum, lst->qname, lst->attributes, lst->typeName);
                count++;
                break;

            default:
            case EJS_SECT_START:
            case EJS_SECT_END:
                mprAssert(0);
                break;
            }
        }

        if (count > 0) {
            lstEndModule(mp, module);
        }
    }

    mprFree(mp->file);
    mp->file = 0;
}



static int lstOpen(EjsMod *mp, char *moduleFilename, EjsModuleHdr *hdr)
{
    char    *path, *name, *ext;

    mprAssert(mp);

    name = mprStrdup(mp, mprGetBaseName(moduleFilename));
    if ((ext = strstr(name, EJS_MODULE_EXT)) != 0) {
        *ext = '\0';
    }

    mprAllocSprintf(mp, &path, 0, "%s%s", name, EJS_LISTING_EXT);
    if ((mp->file = mprOpen(mp, path,  O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, 0664)) == 0) {
        mprError(mp, "Can't create %s", path);
        mprFree(path);
        return EJS_ERR;
    }
    mprEnableFileBuffering(mp->file, 0, 0);

    /*  TODO - add date/time of generation */
    mprFprintf(mp->file,
        "#\n"
        "#  %s -- Module Listing for %s\n"
        "#\n"
        "#  Header version %d.%d, Head Flags %x\n"
        "#\n",
        path, moduleFilename, hdr->major, hdr->minor, hdr->flags);

    mprFree(path);
    return 0;
}


static void lstBlock(EjsMod *mp, EjsModule *module, EjsVar *owner, int slotNum, cchar *name, int numSlots)
{
    Ejs         *ejs;
    cchar       *blockName;

    ejs = mp->ejs;

    mprFprintf(mp->file, "\n");
    blockName = getBlockName(mp, owner);
    mprFprintf(mp->file, "BLOCK:      [%s-%02d]  %s (Slots %d)\n", blockName, slotNum, name, numSlots);
}


/*
 *  List a class (type)
 */
static void lstClass(EjsMod *mp, EjsModule *module, int slotNum, EjsType *klass, int attributes)
{
    Ejs         *ejs;

    ejs = mp->ejs;

    mprFprintf(mp->file, "\n");

    //  TODO - base type may (won't) be set until fixup.

    if (klass->baseType) {
        mprFprintf(mp->file, "CLASS:      %sclass %s extends %s\n", getAttributeString(mp, attributes), klass->qname.name, klass->baseType->qname.name);
    } else {
        mprFprintf(mp->file, "CLASS:      %sclass %s\n", getAttributeString(mp, attributes), klass->qname.name);
    }

    leadin(mp, module, 1, 0);
    mprFprintf(mp->file, "        #  Class Details: %d class traits, %d instance traits, requested slot %d\n",
        ejsGetPropertyCount(ejs, (EjsVar*) klass),
        klass->instanceBlock ? ejsGetPropertyCount(ejs, (EjsVar*) klass->instanceBlock) : 0, 
        slotNum);
}


static void lstDependency(EjsMod *mp, EjsModule *module, char *name, char *url)
{
    leadin(mp, module, 0, 0);
    mprFprintf(mp->file, "DEPENDENCY: use module %s \"%s\"\n\n", name, url);
}


static void lstEndModule(EjsMod *mp, EjsModule *module)
{
    char        *pp;
    int         i, size;

    mprAssert(mp);

    mprFprintf(mp->file,
        "\n----------------------------------------------------------------------------------------------\n");

    lstSlotAssignments(mp, module, mp->ejs->global);

    /*
     *  Dump the constant pool
     */
    size = module->constants->len;
    mprFprintf(mp->file,
        "\n----------------------------------------------------------------------------------------------\n"
        "#\n"
        "#  Constant Pool (size %d bytes)\n"
        "#\n", size);

    pp = module->constants->pool;
    for (i = 0; pp < &module->constants->pool[size]; i++) {
        mprFprintf(mp->file, "%04d   \"%s\"\n", i, pp);
        pp = strchr(pp, '\0') + 1;
    }
}


//  TODO - not consistent with lstProp which looks up the property name using a slotNum

static void lstFunction(EjsMod *mp, EjsModule *module, EjsVar *block, int slotNum, EjsName qname, EjsFunction *fun, 
        int attributes)
{
    Ejs         *ejs;
    EjsTrait    *trait;
    EjsName     lname;
    EjsType     *resultType;
    cchar       *blockName;
    int         i, numLocals;

    ejs = mp->ejs;

    mprFprintf(mp->file,  "\n");
    mprFprintf(mp->file,  "FUNCTION:   ");

    /*
     *  Do the function declaration
     */
    blockName = getBlockName(mp, block);
    if (attributes) {
        if (slotNum < 0) {
            /* Special just for global initializers */
            mprFprintf(mp->file,  "[initializer]  %s %sfunction %s(", qname.space, getAttributeString(mp, attributes), 
                qname.name);
        } else {
            mprFprintf(mp->file,  "[%s-%02d]  %s %sfunction %s(", blockName, slotNum, qname.space,
                getAttributeString(mp, attributes), qname.name);
        }
    } else {
        mprFprintf(mp->file,  "[%s-%02d]  %s function %s(", blockName, slotNum, qname.space, qname.name);
    }

    for (i = 0; i < (int) fun->numArgs; ) {
        lname = ejsGetPropertyName(ejs, (EjsVar*) fun, i);
        trait = ejsGetPropertyTrait(ejs, (EjsVar*) fun, i);
        if (trait->type) {
            mprFprintf(mp->file,  "%s: %s", lname.name, trait->type->qname.name);
        } else {
            mprFprintf(mp->file,  "%s", lname.name);
        }
        if (++i < (int) fun->numArgs) {
            mprFprintf(mp->file,  ", ");
        }
    }

    resultType = fun->resultType;
    mprFprintf(mp->file,  ") : %s\n", resultType ? resultType->qname.name : "void");
    mprFprintf(mp->file,  "\n");

    /*
     *  Repeat the args
     */
    for (i = 0; i < (int) fun->numArgs; i++) {
        lname = ejsGetPropertyName(ejs, (EjsVar*) fun, i);
        trait = ejsGetPropertyTrait(ejs, (EjsVar*) fun, i);
        mprFprintf(mp->file,  "     ARG:   [arg-%02d]   %s %s", i, lname.space, lname.name);
        if (trait->type) {
            mprFprintf(mp->file,  " : %s", trait->type->qname.name);
        }
        mprFprintf(mp->file,  "\n");
    }

    numLocals = fun->block.obj.numProp - fun->numArgs;
    for (i = 0; i < numLocals; i++) {
        lname = ejsGetPropertyName(ejs, (EjsVar*) fun, i + fun->numArgs);
        trait = ejsGetPropertyTrait(ejs, (EjsVar*) fun, i + fun->numArgs);
        mprFprintf(mp->file,  "   LOCAL:   [local-%02d] var %s", i + fun->numArgs, lname.name);
        if (trait->type) {
            mprFprintf(mp->file,  " : %s", trait->type->qname.name);
        }
        mprFprintf(mp->file,  "\n");
    }
    mprFprintf(mp->file,  "\n");
    interp(mp, module, fun);

    leadin(mp, module, 0, 0);
    mprFprintf(mp->file,  "\n");
}


void lstException(EjsMod *mp, EjsModule *module, EjsFunction *fun)
{
    Ejs             *ejs;
    EjsEx           *ex;
    EjsCode         *code;
    cchar           *exKind;
    int             i;

    ejs = mp->ejs;
    code = &fun->body.code;

    if (code->numHandlers <= 0) {
        return;
    }

    mprFprintf(mp->file,
        "\n"
        "#\n"
        "#  Exception Section\n"
        "#    Kind     TryStart TryEnd  HandlerStart  HandlerEnd   CatchType\n"
        "#\n");

    for (i = 0; i < code->numHandlers; i++) {
        ex = code->handlers[i];

        if (ex->flags & EJS_EX_FINALLY) {
            exKind  = "finally";
        } else if (ex->flags & EJS_EX_ITERATION) {
            exKind  = "iteration";
        } else if (ex->flags & EJS_EX_CATCH) {
            exKind = "catch";
        } else {
            exKind = "unknown";
        }
        mprFprintf(mp->file,
            "%-3d %-10s %5d   %5d      %5d        %5d       %s\n",
            i, exKind, ex->tryStart, ex->tryEnd, ex->handlerStart, ex->handlerEnd,
            ex->catchType ? (ex->catchType->qname.name) : "");
    }
}


static void lstProperty(EjsMod *mp, EjsModule *module, EjsVar *block, int slotNum, EjsName qname, int attributes, 
        EjsName typeName)
{
    Ejs         *ejs;
    EjsType     *propType;
    cchar       *blockName;

    ejs = mp->ejs;
    mprFprintf(mp->file, "\nVARIABLE:   ");

    blockName = getBlockName(mp, block);
    mprFprintf(mp->file, "[%s-%02d]  %s %svar %s", blockName, slotNum, qname.space,
        getAttributeString(mp, attributes), qname.name);

    if (typeName.name && typeName.name[0]) {
        mprFprintf(mp->file, " : %s", typeName.name);
    }
    mprFprintf(mp->file, "\n");

    if (block == 0) {
        /*
         *  Nested block. TODO Why define the block here?
         */
        if (typeName.name) {
            propType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &typeName);
        } else {
            propType = 0;
        }
        mprAssert(mp->currentBlock && ejsIsBlock(mp->currentBlock));
        slotNum = ejsDefineProperty(ejs, (EjsVar*) mp->currentBlock, -1, &qname, propType, attributes, 0);
    }
}


static void lstModule(EjsMod *mp, EjsModule *module)
{
    //  TODO - doing noting with module->url

    mprFprintf(mp->file,
        "\n==============================================================================================\n\n"
        "MODULE:   %s", module->name);

    if (module->hasInitializer || module->boundGlobals) {
        mprFprintf(mp->file, " <%s%s>\n",
            module->hasInitializer ? "hasInitializer, " : "",
            module->boundGlobals ? "boundGlobals" : "");
    }
    mprFprintf(mp->file, "\n");
}


static int decodeOperands(EjsMod *mp, Optable *opt, char *argbuf, int argbufLen, int address, int *stackEffect)
{
    int         *argp;
    char        *sval, *bufp;
    uchar       *start;
    int         i, argc, ival, len, buflen, j, numEntries;
    int64       lval;
#if BLD_FEATURE_FLOATING_POINT
    double      dval;
#endif

    *stackEffect = opt->stackEffect;

    /*
     *  Keep a local progressive pointer into the argbuf and a length of the remaining room in the buffer.
     */
    *argbuf = '\0';
    bufp = argbuf;
    buflen = argbufLen;

    for (argc = 0, argp = opt->args; *argp; argc++, argp++) ;

    start = mp->pc;
    ival = 0;

    for (i = 0, argp = opt->args; i < argc; i++) {
        switch (opt->args[i]) {
        case NONE:
            break;

        case BYTE:
            ival = getByte(mp);
            mprSprintf(bufp, buflen,  "<%d> ", ival);
            break;

        case SHORT:
            ival = getShort(mp);
            mprSprintf(bufp, buflen,  "<%d> ", ival);
            break;

        case WORD:
            ival = getWord(mp);
            mprSprintf(bufp, buflen,  "<%d> ", ival);
            break;

        case LONG:
            lval = getLong(mp);
            mprSprintf(bufp, buflen,  "<%Ld> ", lval);
            break;

#if BLD_FEATURE_FLOATING_POINT
        case DOUBLE:
            dval = getDouble(mp);
            mprSprintf(bufp, buflen,  "<%f> ", dval);
            break;
#endif

        case ARGC:
        case ARGC2:
            ival = getNum(mp);
            mprSprintf(bufp, buflen,  "<argc: %d> ", ival);
            break;

        case SLOT:
            ival = getNum(mp);
            mprSprintf(bufp, buflen,  "<slot: %d> ", ival);
            break;

        case NUM:
            ival = getNum(mp);
            mprSprintf(bufp, buflen,  "<%d> ", ival);
            break;

        case JMP8:
            ival = getByte(mp);
            mprSprintf(bufp, buflen,  "<addr: %d> ", ((char) ival) + address + 1);
            break;

        case JMP:
            ival = getWord(mp);
            mprSprintf(bufp, buflen,  "<addr: %d> ", ival + address + 4);
            break;

        case INIT_DEFAULT8:
            numEntries = getByte(mp);
            len = mprSprintf(bufp, buflen,  "<%d> ", numEntries);
            bufp += len;
            buflen -= len;
            for (j = 0; j < numEntries; j++) {
                ival = getByte(mp);
                len = mprSprintf(bufp, buflen,  "<%d> ", ival + 2);
                bufp += len;
                buflen -= len;
            }
            break;

        case INIT_DEFAULT:
            numEntries = getByte(mp);
            len = mprSprintf(bufp, buflen,  "<%d> ", numEntries);
            bufp += len;
            buflen -= len;
            for (j = 0; j < numEntries; j++) {
                ival = getWord(mp);
                len = mprSprintf(bufp, buflen,  "<%d> ", ival + 2);
                bufp += len;
                buflen -= len;
            }
            break;

        case STRING:
            sval = getString(mp);
            mprSprintf(bufp, buflen,  "<%s> ", sval);
            break;

        case VARCOUNT:
            break;

        case GLOBAL:
            getGlobal(mp, bufp, buflen);
            break;

        default:
            mprError(mp, "Bad arg type in opcode table");
            break;
        }
        len = strlen(bufp);
        bufp += len;
        buflen -= len;

        if (opt->args[i] == ARGC) {
            *stackEffect -= ival;
        } else if (opt->args[i] == ARGC2) {
            *stackEffect -= (ival * 2);
        }
        if (i == 0 && opt->stackEffect == POP1) {
            *stackEffect = -ival;
        }
    }
    return mp->pc - start;
}


/*
 *  Interpret the code for a function
 */
static void interp(EjsMod *mp, EjsModule *module, EjsFunction *fun)
{
    Optable     *opt;
    EjsCode     *code;
    uchar       *start;
    char        *currentLine, *currentFile;
    char        argbuf[MPR_MAX_STRING], lineInfo[MPR_MAX_STRING];
    int         opcode, lineNumber, stack, codeLen, address, stackEffect, nbytes, i, lastDebug;

    mprAssert(mp);
    mprAssert(module);
    mprAssert(fun);

    /*
     *  Store so that getNum and getString can easily read instructions
     */
    code = &fun->body.code;
    mp->fun = fun;
    mp->module = module;
    mp->pc = code->byteCode;
    codeLen = code->codeLen;
    start = mp->pc;
    stack = 0;
    lastDebug = 0;
    currentLine = 0;
    lineNumber = 0;
    currentFile = 0;

    while ((mp->pc - start) < codeLen) {
        address = mp->pc - start;
        opcode = *mp->pc++;
        argbuf[0] = '\0';
        stackEffect = 0;

        if (opcode < 0 || opcode > (sizeof(optable) / sizeof(Optable))) {
            mprError(mp, "Bad opcode %x at address %d.\n", opcode, address);
            return;
        }
        opt = &optable[opcode];

        if (opcode != EJS_OP_DEBUG || mp->showDebug) {
            /*
             *  Output address [stack] opcode
             *  Format:  "address: [stackDepth] opcode <args> ..."
             */
            if (lastDebug) {
                mprFprintf(mp->file, "\n");
                lastDebug = 0;
            }
            mprFprintf(mp->file,  "    %04d: [%d] %02x ", address, stack, opcode);
            mp->showAsm = 1;

        } else {
            mp->showAsm = 0;
        }

        if (opcode == EJS_OP_DEBUG) {
            if ((currentFile = getString(mp)) == 0) {
                goto badToken;
            }
            lineNumber = getNum(mp);
            if ((currentLine = getString(mp)) == 0) {
                goto badToken;
            }
            nbytes = (mp->pc - start) - address - 1;

        } else {
            nbytes = decodeOperands(mp, opt, argbuf, sizeof(argbuf), mp->pc - start, &stackEffect);
        }

        if (mp->showAsm) {
            for (i = 24 - (nbytes * 3); i >= 0; i--) {
                mprFprintf(mp->file, ".");
            }
            mprFprintf(mp->file,  " %s %s\n", opt->name, argbuf);

        } else if (opcode != EJS_OP_DEBUG) {
            for (i = 24 - (nbytes * 3); i >= 0; i--) {
                mprFprintf(mp->file, " ");
            }
            mprFprintf(mp->file,  " %s\n", argbuf);
        }

        stack += stackEffect;

        if (opcode == EJS_OP_RETURN_VALUE || opcode == EJS_OP_RETURN) {
            stack = 0;
        }

        if (stack < 0) {
            if (mp->warnOnError) {
                mprErrorPrintf(mp, "Instruction stack is negative %d\n", stack);
            }
            if (mp->exitOnError) {
                exit(255);
            }
        }
        if (opcode == EJS_OP_DEBUG) {
            if (!lastDebug) {
                mprFprintf(mp->file, "\n");
            }
            //  TODO - should convert tabs to spaces.
            //  TODO - should do extra new line when the file name changes
            mprSprintf(lineInfo, sizeof(lineInfo), "%s:%d", currentFile, lineNumber);
            mprFprintf(mp->file, "    # %-25s %s\n", lineInfo, currentLine);
            lastDebug = 1;
        }
    }
    return;

badToken:
//  TODO - fix reporting of token
    mprError(mp, "Bad input stream token 0x%x at %d.\n", 0, address);
    mp->error = 1;
    //  TODO fatal or keep going?
}


static void lstVarSlot(EjsMod *mp, EjsModule *module, EjsName *qname, EjsTrait *trait, int slotNum)
{
    mprAssert(slotNum >= 0);
    mprAssert(trait);
    mprAssert(qname);

    if (qname->name == 0 || qname->name[0] == '\0') {
        mprFprintf(mp->file, "%04d    reserved slot for static super property\n", slotNum);

    } else if (trait->type) {
        if (trait->type == mp->ejs->functionType) {
            mprFprintf(mp->file, "%04d    %s function %s\n", slotNum, qname->space, qname->name);

        } else if (trait->type == mp->ejs->functionType) {
            mprFprintf(mp->file, "%04d    %s class %s\n", slotNum, qname->space, qname->name);

        } else {
            mprFprintf(mp->file, "%04d    %s var %s: %s\n", slotNum, qname->space, qname->name, trait->type->qname.name);
        }

    } else {
        mprFprintf(mp->file, "%04d    %s var %s\n", slotNum, qname->space, qname->name);
    }
}


/*
 *  List the various property slot assignments
 */
static void lstSlotAssignments(EjsMod *mp, EjsModule *module, EjsVar *obj)
{
    Ejs             *ejs;
    EjsTrait        *trait;
    EjsType         *type;
    EjsVar          *vp;
    EjsFunction     *fun;
    EjsBlock        *block, *instanceBlock;
    EjsName         qname;
    int             slotNum, numProp, numInherited, count;

    mprAssert(obj);
    mprAssert(module);

    ejs = mp->ejs;
    numProp = 0;

    if (obj->visited) {
        return;
    }
    obj->visited = 1;

    if (obj == ejs->global) {
        type = (EjsType*) obj;
        numInherited = (type->baseType) ? ejsGetPropertyCount(ejs, (EjsVar*) type->baseType) : 0;
        mprFprintf(mp->file,  "\n#\n"
            "#  Global slot assignments (Num prop %d, num inherited %d)\n"
            "#\n", ejsGetPropertyCount(ejs, (EjsVar*) type), numInherited);

        /*
         *  List slots for global
         */
        count = ejsGetNumTraits(ejs->globalBlock);
        for (slotNum = module->firstGlobalSlot; slotNum < count; slotNum++) {
            trait = ejsGetPropertyTrait(ejs, ejs->global, slotNum);
            qname = ejsGetPropertyName(ejs, ejs->global, slotNum);
            if (qname.name == 0) {
                continue;
            }
            if (trait->attributes & EJS_ATTR_BUILTIN && !mp->showBuiltin) {
                continue;
            }
            mprAssert(trait);
            lstVarSlot(mp, module, &qname, trait, slotNum);
        }


        /*
         *  List slots for the initializer
         */
        fun = (EjsFunction*) module->initializer;
        if (fun) {
            mprFprintf(mp->file,  "\n#\n"
                "#  Initializer slot assignments (Num prop %d)\n"
                "#\n", ejsGetPropertyCount(ejs, (EjsVar*) fun));

            count = ejsGetNumTraits((EjsBlock*) fun);
            for (slotNum = 0; slotNum < count; slotNum++) {
                trait = ejsGetPropertyTrait(ejs, (EjsVar*) fun, slotNum);
                qname = ejsGetPropertyName(ejs, (EjsVar*) fun, slotNum);
                if (qname.name == 0) {
                    continue;
                }
                if (trait->attributes & EJS_ATTR_BUILTIN && !mp->showBuiltin) {
                    continue;
                }
                mprAssert(trait);
                lstVarSlot(mp, module, &qname, trait, slotNum);
            }
        }

    } else if (ejsIsFunction(obj)) {

        fun = (EjsFunction*) obj;
        qname = ejsGetPropertyName(ejs, fun->owner, fun->slotNum);

        count = ejsGetNumTraits((EjsBlock*) obj);
        if (count > 0) {
            mprFprintf(mp->file,  "\n#\n"
                "#  Local slot assignments for the \"%s\" function (Num slots %d)\n"
                "#\n", qname.name, count);

            for (slotNum = 0; slotNum < count; slotNum++) {
                trait = ejsGetPropertyTrait(ejs, obj, slotNum);
                mprAssert(trait);
                qname = ejsGetPropertyName(ejs, obj, slotNum);
                lstVarSlot(mp, module, &qname, trait, slotNum);
            }
        }

    } else if (ejsIsType(obj)) {
        /*
         *  Types
         */
        type = (EjsType*) obj;
        numInherited = type->baseType ? ejsGetPropertyCount(ejs, (EjsVar*) type->baseType) : 0;
        mprFprintf(mp->file,  "\n#\n"
            "#  Class slot assignments for the \"%s\" class (Num slots %d, num inherited %d)\n"
            "#\n", type->qname.name,
            ejsGetPropertyCount(ejs, (EjsVar*) type), numInherited);

        count = ejsGetNumTraits((EjsBlock*) type);
        for (slotNum = 0; slotNum < count; slotNum++) {
            trait = ejsGetPropertyTrait(ejs, (EjsVar*) type, slotNum);
            mprAssert(trait);
            qname = ejsGetPropertyName(ejs, obj, slotNum);
            lstVarSlot(mp, module, &qname, trait, slotNum);
        }

        instanceBlock = type->instanceBlock;
        if (type->baseType && type->baseType->instanceBlock) {
            numInherited = ejsGetPropertyCount(ejs, (EjsVar*) type->baseType->instanceBlock);
        } else {
            numInherited = 0;
        }
        mprFprintf(mp->file,  "\n#\n"
            "#  Instance slot assignments for the \"%s\" class (Num prop %d, num inherited %d)\n"
            "#\n", type->qname.name,
            instanceBlock ? ejsGetPropertyCount(ejs, (EjsVar*) instanceBlock): 0 ,
            numInherited);

        if (instanceBlock) {
            count = ejsGetNumTraits(instanceBlock);
            for (slotNum = 0; slotNum < count; slotNum++) {
                trait = ejsGetPropertyTrait(ejs, (EjsVar*) instanceBlock, slotNum);
                mprAssert(trait);
                qname = ejsGetPropertyName(ejs, (EjsVar*) instanceBlock, slotNum);
                if (qname.name) {
                    lstVarSlot(mp, module, &qname, trait, slotNum);
                }
            }
        }

    } else if (ejsIsBlock(obj)) {

        block = (EjsBlock*) obj;
        count = ejsGetNumTraits(block);
        if (count > 0) {
            mprFprintf(mp->file,  
                "\n#\n"
                "#  Block slot assignments for the \"%s\" (Num slots %d)\n"
                "#\n", block->name, ejsGetPropertyCount(ejs, obj));
            
            count = ejsGetNumTraits((EjsBlock*) obj);
            for (slotNum = 0; slotNum < count; slotNum++) {
                trait = ejsGetPropertyTrait(ejs, obj, slotNum);
                mprAssert(trait);
                qname = ejsGetPropertyName(ejs, obj, slotNum);
                lstVarSlot(mp, module, &qname, trait, slotNum);
            }
        }
    }

    /*
     *  Now recurse on types, functions and blocks
     */
    count = ejsGetNumTraits((EjsBlock*) obj);

    slotNum = (obj == ejs->global) ? module->firstGlobalSlot : 0;
    for (; slotNum < count; slotNum++) {
        trait = ejsGetPropertyTrait(ejs, obj, slotNum);
        qname = ejsGetPropertyName(ejs, obj, slotNum);
        vp = ejsGetProperty(ejs, obj, slotNum);
        if (vp == 0 || (trait->attributes & EJS_ATTR_BUILTIN && !mp->showBuiltin)) {
            continue;
        }
        if (ejsIsType(vp) || ejsIsFunction(vp) || ejsIsBlock(vp)) {
            lstSlotAssignments(mp, module, vp);
        }
    }

    obj->visited = 0;
}


static cchar *getBlockName(EjsMod *mp, EjsVar *block)
{
    EjsFunction     *fun;
    EjsName         qname;

    if (block) {
        if (ejsIsType(block)) {
            return ((EjsType*) block)->qname.name;

        } else if (ejsIsFunction(block)) {
            fun = (EjsFunction*) block;
            if (fun->owner) {
                qname = ejsGetPropertyName(mp->ejs, fun->owner, fun->slotNum);
#if DEBUG
            } else {
                ejsName(&qname, 0, fun->obj.var.debugName);
#endif
            } else {
                /*
                 *  Only the initializers don't have an owner
                 */
                ejsName(&qname, 0, EJS_INITIALIZER_NAME);
            }
            return qname.name;
        }
    }
    mprAssert(ejsIsBlock(block));
    return ((EjsBlock*)block)->name;
}


static char *getAttributeString(EjsMod *mp, int attributes)
{
    static char attributeBuf[MPR_MAX_STRING];

    attributeBuf[0] = '\0';

    /*
     *  Order to look best
     */
    if (attributes & EJS_ATTR_NATIVE) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "native ", 0);
    }

    if (attributes & EJS_ATTR_PROTOTYPE) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "prototype ", 0);
    }

    if (attributes & EJS_ATTR_CONST) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "const ", 0);
    }

    if (attributes & EJS_ATTR_READONLY) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "readonly ", 0);
    }

    if (attributes & EJS_ATTR_STATIC) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "static ", 0);
    }

    if (attributes & EJS_ATTR_FINAL) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "final ", 0);
    }

    if (attributes & EJS_ATTR_OVERRIDE) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "override ", 0);

    }

    if (attributes & EJS_ATTR_DYNAMIC_INSTANCE) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "dynamic ", 0);
    }

    if (attributes & EJS_ATTR_ENUMERABLE) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "enumerable ", 0);

    }

    if (attributes & EJS_ATTR_GETTER) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "get ", 0);
    }

    if (attributes & EJS_ATTR_SETTER) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "set ", 0);
    }

#if UNUSED
    if (attributes & EJS_ATTR_OPERATOR) {
        mprStrcat(attributeBuf, sizeof(attributeBuf), "", "operator ", 0);
    }
#endif

    return attributeBuf;
}


/*
 *  TODO - ENDIAN.
 */
static uchar getByte(EjsMod *mp)
{
    if (mp->showAsm) {
        mprFprintf(mp->file, "%02x ",  mp->pc[0] & 0xFF);
    }
    return *mp->pc++ & 0xFF;
}


static ushort getShort(EjsMod *mp)
{
    ushort  value, *sp;
    int     i;

    if (mp->showAsm) {
        for (i = 0; i < 2; i++) {
            mprFprintf(mp->file, "%02x ", mp->pc[i] & 0xFF);
        }
    }
    sp = (ushort*) mp->pc;
    value = *sp++ & 0xFFFF;
    mp->pc = (uchar*) sp;
    return value;
}


static uint getWord(EjsMod *mp)
{
    uint    value, *sp;
    int     i;

    if (mp->showAsm) {
        for (i = 0; i < 4; i++) {
            mprFprintf(mp->file, "%02x ", mp->pc[i] & 0xFF);
        }
    }
    sp = (uint*) mp->pc;
    value = *sp++ & 0xFFFFFFFF;
    mp->pc = (uchar*) sp;
    return value;
}


static int64 getLong(EjsMod *mp)
{
    uint64  value, *sp;
    int     i;

    if (mp->showAsm) {
        for (i = 0; i < 8; i++) {
            mprFprintf(mp->file, "%02x ", mp->pc[i] & 0xff);
        }
    }
    sp = (uint64*) mp->pc;
    value = *sp++;
    mp->pc = (uchar*) sp;
    return value;
}


#if BLD_FEATURE_FLOATING_POINT
static double getDouble(EjsMod *mp)
{
    double  value, *sp;
    int     i;

    if (mp->showAsm) {
        for (i = 0; i < 8; i++) {
            mprFprintf(mp->file, "%02x ", mp->pc[i] & 0xff);
        }
    }
    sp = (double*) mp->pc;
    value = *sp++;
    mp->pc = (uchar*) sp;
    return value;
}
#endif


/*
 *  Get an encoded number
 */
static int getNum(EjsMod *mp)
{
    uchar   *start;
    uint    t, c;

    start = mp->pc;

    c = (uint) *mp->pc++;
    t = c & 0x7f;

    if (c & 0x80) {
        c = (uint) *mp->pc++;
        t |= ((c & 0x7f) << 7);

        if (c & 0x80) {
            c = (uint) *mp->pc++;
            t |= ((c & 0x7f) << 14);

            if (c & 0x80) {
                c = (uint) *mp->pc++;
                t |= ((c & 0x7f) << 21);

                if (c & 0x80) {
                    c = (uint) *mp->pc++;
                    t |= ((c & 0x7f) << 28);
                }
            }
        }
    }

    if (mp->showAsm) {
        for (; start < mp->pc; start++) {
            mprFprintf(mp->file, "%02x ", *start & 0xff);
        }
    }
    return (int) t;
}


/*
 *  Read an interned string constants are stored as token offsets into the constant pool. The pool contains null
 *  terminated UTF-8 strings.
 */
static char *getString(EjsMod *mp)
{
    int     number;

    number = getNum(mp);
    if (number < 0) {
        return 0;
    }

    return &mp->module->constants->pool[number];
}


/*
 *  Return the length of bytes added to buf
 */
static void getGlobal(EjsMod *mp, char *buf, int buflen)
{
    Ejs             *ejs;
    EjsName         qname;
    EjsVar          *vp;
    int             t, slotNum;

    ejs = mp->ejs;
    vp = 0;

    if ((t = getNum(mp)) < 0) {
        mprSprintf(buf, buflen,  "<can't read code>");
        return;
    }

    switch (t & EJS_ENCODE_GLOBAL_MASK) {
    default:
        mprAssert(0);
        return;

    case EJS_ENCODE_GLOBAL_NOREF:
        return;

    case EJS_ENCODE_GLOBAL_SLOT:
        /*
         *  Type is a builtin primitive type or we are binding globals.
         */
        slotNum = t >> 2;
        if (0 <= slotNum && slotNum < ejsGetPropertyCount(ejs, ejs->global)) {
            vp = ejsGetProperty(ejs, ejs->global, slotNum);
        }
        if (vp && ejsIsType(vp)) {
            mprSprintf(buf, buflen, "<type: 0x%x,  %s::%s> ", t, ((EjsType*) vp)->qname.space, ((EjsType*) vp)->qname.name);
        }
        break;

    case EJS_ENCODE_GLOBAL_NAME:
        /*
         *  Type was unknown at compile time
         */
        //  TODO - should have namespace
        qname.name = &mp->module->constants->pool[t >> 2];
        if (qname.name == 0) {
            mprAssert(0);
            mprSprintf(buf, buflen,  "<var: 0x%x,  missing name> ", t);
            return;
        }
        if ((qname.space = getString(mp)) == 0) {
            mprSprintf(buf, buflen,  "<var: 0x%x,  missing namespace> ", t);
            return;
        }
        if (qname.name) {
            vp = ejsGetPropertyByName(ejs, ejs->global, &qname);
        }
        mprSprintf(buf, buflen, "<var: 0x%x,  %s::%s> ", t, qname.space, qname.name);
        break;
    }

    if (vp == 0) {
        mprSprintf(buf, buflen, "<var: %d,  cannot resolve var/typ at slot e> ", t);
    }
}


static void leadin(EjsMod *mp, EjsModule *module, int classDec, int inFunction)
{
    mprFprintf(mp->file, "    ");
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
