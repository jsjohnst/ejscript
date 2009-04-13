/*
 *  ecCompiler.h - Internal compiler header.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_EC_COMPILER
#define _h_EC_COMPILER 1

#include    "ejs.h"
#include    "ejsTune.h"
#include    "ejsCore.h"

/*********************************** Defines **********************************/
/*
 *  Compiler validation modes. From "use standard|strict"
 */
#define PRAGMA_MODE_STANDARD    1               /* Standard unstrict mode */
#define PRAGMA_MODE_STRICT      2               /* Strict mode */

#define STANDARD_MODE(cp)       (cp->fileState->mode == PRAGMA_MODE_STANDARD)
#define STRICT_MODE(cp)         (cp->fileState->mode == PRAGMA_MODE_STRICT)

/*
 *  Variable Kind bits
 */
#define KIND_CONST              0x1
#define KIND_VAR                0x2
#define KIND_LET                0x4

/*
 *  Phases for AST processing
 */
#define EC_PHASE_DEFINE         0           /* Define types, functions and properties in types */
#define EC_PHASE_CONDITIONAL    1           /* Do conditional processing, hoisting and then type fixups */
#define EC_PHASE_FIXUP          2           /* Fixup type references */
#define EC_PHASE_BIND           3           /* Bind var references to slots and property types */
#define EC_AST_PHASES           4


/*
 *  Ast node define
 */
typedef struct EcNode   *Node;

typedef struct EcNode {
    //  TODO - need consistency in naming. Nodes should have a node suffix. References EjsVar should have a Ref suffix.
    /*
     *  Ordered for debugging
     */
    char                *kindName;          /* Node kind string */

    /*
     *  TODO - optimize name+qualifier fields
     */
    EjsName             qname;
    int                 literalNamespace;   /* Namespace is a literal */

    cchar               *filename;          /* File containing source code line */
    char                *currentLine;       /* Current source code line */
    int                 lineNumber;

    Node                left;               /* children[0] */
    Node                right;              /* children[1] */

    EjsBlock            *blockRef;          /* Block scope */
    bool                createBlockObject;  /* Create the block object to contain let scope variables */
    bool                blockCreated;       /* Block object has been created */
    EjsLookup           lookup;

    int                 attributes;         /* Attributes applying to this node */
    Node                qualifierNode;      /* Namespace qualifier node */

    //  TODO - is this being used?
    bool                specialNamespace;   /* Using a public|private|protected|internal namespace */

    Node                typeNode;           /* Type of name */
    int                 nullable;           /* Type can be nullable */

    Node                parent;             /* Parent node */

    EjsNamespace        *namespaceRef;      /* Namespace reference */
    MprList             *namespaces;        /* Namespaces for hoisted variables */

    /*
     *  TODO is this used?
     */
    #define N_VALUE     1

#if BLD_DEBUG
    char                *tokenName;
#endif

    int                 tokenId;            /* Lex token */
    int                 groupMask;          /* Token group */
    int                 subId;              /* Sub token */

    /*
        TODO - order in most useful order: name, type, function, binary
        TODO - rename all with a Node suffix. ie. nameNode, importNode
     */

    //  TODO - disable for now. Crashes on Linux if enabled
#if !BLD_DEBUG && 0
    union {
#endif

        //  GROUPING ONLY
        #define N_ATTRIBUTES 50

        /*
         *  Name nodes hold a fully qualified name.
         */
        #define N_QNAME  3
        struct {
            //  TODO - bit field
            int         isAttribute;        /* Attribute identifier "@" */
            int         isType;             /* Name is a type */
            int         isNamespace;        /* Name is a namespace */
            int         letScope;           /* Variable is defined in let block scope */
            int         instanceVar;        /* Instance or static var (if defined in class) */
            int         isRest;             /* ... rest style args */
            EjsVar      *value;             /* Initialization value */
        } name;

        /*
         *  cast, is, to
         */
        #define N_BINARY_TYPE_OP  5

        /*
         * + - * / % << >> >>> & | ^ && || instanceof in == != ===
         *      !== < <= > >= , ]
         */
        #define N_BINARY_OP  6
        #define N_ASSIGN_OP  7

        struct {
            struct EcCodeGen    *rightCode;
        } binary;


        #define N_FOR_IN 33
        struct {
            int         each;                       /* For each used */
            Node        iterVar;
            Node        iterGet;
            Node        iterNext;
            Node        body;
            struct EcCodeGen    *initCode;
            struct EcCodeGen    *bodyCode;
        } forInLoop;


        /*
         *  Used by for and while
         */
        #define N_FOR   32
        struct {
            Node        initializer;
            Node        cond;
            Node        perLoop;
            Node        body;
            struct EcCodeGen    *condCode;
            struct EcCodeGen    *bodyCode;
            struct EcCodeGen    *perLoopCode;
        } forLoop;


        #define N_DO    51
        struct {
            Node        cond;
            Node        body;
            struct EcCodeGen    *condCode;
            struct EcCodeGen    *bodyCode;
        } doWhile;


        /*  TODO - convert to grouping  */
        #define N_UNARY_OP  9
        struct {
            int         dummy;
        } unary;


        #define N_IF  10
        struct {
            Node                cond;
            Node                thenBlock;
            Node                elseBlock;
            struct EcCodeGen    *thenCode;
            struct EcCodeGen    *elseCode;
        } tenary;


        #define N_HASH  55
        struct {
            Node        expr;
            Node        body;
            bool        disabled;
        } hash;


        #define N_VAR_DEFINITION  11
        /*
         *  Var definitions have one child per variable. Child nodes can
         *  be either N_NAME or N_ASSIGN_OP
         */
        struct {
            int         varKind;            /* Variable definition kind */
        } def;


        /*
         * Pragmas: use strict, use standard, use enhanced
         *  TODO - rename this to be MODE and mode. Or expand this to encompass
         *  use namespace etc.
         */
        //  TODO - split apart into separate AST notdes.
        #define N_PRAGMA  12
        struct {
            Node        decimalContext;     /* use decimal expr */
            uint        mode;               /* Pragmas. See PRAGMA_MODE above */
            uint        lang;               /* EJS_SPEC_XXX */
            char        *moduleName;        /* Module name value */
        } pragma;


        /*
         *  Module defintions
         */
        #define N_MODULE 52
        struct {
            EjsModule   *ref;               /* Module object */
            char        *fileName;          /* Module file name */
            char        *name;              /* Module name */
        } module;


        /*
         *  Use module
         */
        #define N_USE_MODULE 53
        struct {
            char        *url;               /* Module pragma URL */
        } useModule;


        /*
         *  use namespace, use default namespace
         */
        #define N_USE_NAMESPACE  49
        struct {
            int         isDefault;          /* "use default" */
            int         isLiteral;          /* use namespace "literal" */
        } useNamespace;


        #define N_FUNCTION 14
        struct {
            uint        operatorFn : 1;     /* operator function */
            uint        getter : 1;         /* getter function */
            uint        setter : 1;         /* setter function */
            uint        call : 1;           /* TODO ?? */
            uint        has : 1;            /* TODO ?? */
            uint        hasRest : 1;        /* Has rest parameter */
            uint        hasReturn : 1;      /* Has a return statement */
            uint        isMethod : 1;       /* Is a class method */
            uint        isConstructor : 1;  /* Is constructor method */
            uint        isDefaultConstructor : 1;/* Is default constructor */
#if UNUSED
            uint        noBlock: 1;         /* Single expression function without a block */
#endif
            Node        resultType;         /* Function return type */
            Node        body;               /* Function body */
            Node        parameters;         /* Function formal parameters */
            Node        constructorSettings;/* Constructor settings */

            /*
             *  Bound definition
             */
            EjsFunction *functionVar;       /* Function variable */
            Node        expressionRef;      /* Reference to the function expression name */

        } function;

        #define N_END_FUNCTION 40
#if UNUSED
        struct {
            Node        function;          /* Owning function */
        } endfunction;
#endif

        #define N_PARAMETER 15
        struct {
            char        *type;              /* Parameter type */
            char        *value;             /* Default value */
            int         isRest : 1;         /* Is rest parameter */
        } parameter;


        /*
         *  Body stored in the child node
         */
        #define N_CLASS  16
        struct {
            char        *extends;           /* Class base class */
            Node        implements;         /* Implemented interfaces */
            MprList     *staticProperties;  /* List of static properties */
            MprList     *instanceProperties;/* Implemented interfaces */
            MprList     *classMethods;      /* Static methods */
            MprList     *methods;           /* Instance methods */
            Node        constructor;        /* Class constructor */
            int         isInterface;        /* This is an interface */

            EjsType     *ref;               /* Type instance ref */
            EjsFunction *initializer;       /* Initializer function */

            EjsNamespace *publicSpace;
            EjsNamespace *internalSpace;

        } klass;


        #define N_DIRECTIVES 18

        #define N_SUPER 35
        struct {
            int         dummy;
        } super;


        #define N_NEW 31
        struct {
            int         callConstructors;   /* Bound type has a constructor */
        } newExpr;


        #define N_TRY 36
        struct {
            /* Children are the catch clauses */
            Node        tryBlock;           /* Try code */
            Node        catchClauses;       /* Catch clauses */
            Node        finallyBlock;       /* Finally code */
        } exception;


        #define N_CATCH 37
        struct {
            Node        arg;                /* Catch block argument */
        } catchBlock;


        #define N_CALL 28
        struct {
            int         dummmy;
        } call;


        #define N_PROGRAM 20
        struct {
            MprList     *dependencies;      /* Accumulated list of dependent modules */
        } program;


        /*
         *  Block kinds
         */
        #define EC_CLASS_BLOCK      1
        #define EC_FUNCTION_BLOCK   2
        #define EC_INTERFACE_BLOCK  3
        #define EC_NESTED_BLOCK     4
        #define EC_GLOBAL_BLOCK     5
        #define EC_MODULE_BLOCK     6

        #define N_BLOCK 25

        #define N_REF 42
        struct {
            Node        node;               /* Actual node reference */
        } ref;


        #define N_SWITCH 43
        struct {
            int         dummy;
        } switchNode;


        #define EC_SWITCH_KIND_CASE     1   /* Case block */
        #define EC_SWITCH_KIND_DEFAULT  2   /* Default block */

        #define N_CASE_LABEL 44
        struct {
            Node                expression;
            int                 kind;
            struct EcCodeGen    *expressionCode;    /* Code buffer for the case expression */
            int                 nextCaseCode;       /* Goto length to the next case statements */
        } caseLabel;

        #define N_THIS 30

        #define N_THIS_GENERATOR    1
        #define N_THIS_CALLEE       2
        #define N_THIS_TYPE         3
        #define N_THIS_FUNCTION     4

        struct {
            int                 thisKind;   /* Kind of this. See N_THIS_ flags */
        } thisNode;

        #define N_CASE_ELEMENTS 45
        #define N_BREAK         46
        #define N_CONTINUE      47
        #define N_GOTO          48

        #define N_LITERAL       2
        struct {
            EjsVar              *var;       /* Special value */
            MprBuf              *data;      /* XML data */
        } literal;

        #define N_OBJECT_LITERAL    56
        struct {
            Node                typeNode;   /* Type of object */
        } objectLiteral;

        /*
         *  Object literal field
         */
        #define FIELD_KIND_VALUE        0x1
        #define FIELD_KIND_FUNCTION     0x2

        #define N_FIELD 57
        struct {
            int                 fieldKind;  /* value or function */
            int                 varKind;    /* Variable definition kind (const) */
            Node                fieldName;  /* Field element name */
            Node                expr;       /* Field expression */
        } field;

        #define N_WITH 60
        struct {
            Node                object;
            Node                statement;
        } with;

#if !BLD_DEBUG && 0
    };
#endif

    int                 kind;               /* Kind of node */
    MprList             *children;

    struct EcCompiler   *cp;                /* Compiler instance reference */

    int                 column;             /* Column of token in currentLine */

    /*
     *  Bound definition. Applies to names and expression values.
     */

    int                 slotFixed;          /* Slot fixup has been done */
    int                 needThis;           /* Need to push this object */
    int                 needDupObj;         /* Need to dup the object on stack (before) */
    int                 needDup;            /* Need to dup the result (after) */
    int                 slotNum;            /* Allocated slot for variable */
    int                 enabled;            /* Node is enabled via conditional definitions */

    uchar               *patchAddress;      /* For code patching */
    struct EcCodeGen    *code;              /* Code buffer */
    int                 jumpLength;         /* Goto length for exceptions */

    int                 seqno;              /* Unique sequence number */
    EcModuleProp        *globalProp;        /* Set if this is a global property */

#if BLD_FEATURE_EJS_DOC
    char                *doc;               /* Documentation string */
#endif

} EcNode;


/*
 *  Grouping node types
 */
/* 21 - unused */
#define N_EXPRESSIONS 22
#define N_PRAGMAS 23
#define N_TYPE_IDENTIFIERS 24
#define N_DOT 26
#define N_RETURN 27
#define N_ARGS 29
/* N_new 31 */
/* N_for 32 */
/* N_forIn 33 */
#define N_POSTFIX_OP 34
/* N_try 36 */
/* N_catch 37 */
#define N_CATCH_CLAUSES 38
#define N_THROW 39
#define N_NOP 41
#define N_VOID 54
/* N_HASH 55 */
/* N_OBJECT_LITERAL 56 */
/* N_FIELD 57 */
#define N_ARRAY_LITERAL 58
#define N_CATCH_ARG 59
#define N_WITH 60

#define EC_NUM_NODES                    8
#define EC_TAB_WIDTH                    4

/*
 *  Fix clash with arpa/nameser.h
 */
#undef T_NULL

/*
 *  Flags for ecCompile()
 */
#define EC_FLAGS_BIND_GLOBALS    0x1                    /* Bind global references */
#define EC_FLAGS_DEBUG           0x2
#define EC_FLAGS_EMPTY           0x4                    /* Start with an empty interpreter */
//  TODO - gap
#define EC_FLAGS_MERGE           0x10                    /* Merge all output onto one output file */
#define EC_FLAGS_NO_BIND         0x20                    /* Don't bind any references */
#define EC_FLAGS_NO_OUT          0x40
#define EC_FLAGS_PARSE_ONLY      0x80
#define EC_FLAGS_RUN             0x100

/*
 *  Lexical tokens (must start at 1)
 *  ASSIGN tokens must be +1 compared to their non-assignment counterparts.
 */
#define T_AS 1
#define T_ASSIGN 2
#define T_AT 3
#define T_ATTRIBUTE 4
#define T_BIT_AND 5
#define T_BIT_AND_ASSIGN 6
#define T_BIT_OR 7
#define T_BIT_OR_ASSIGN 8
#define T_BIT_XOR 9
#define T_BIT_XOR_ASSIGN 10
#define T_BREAK 11
#define T_CALL 12
#define T_CASE 13
#define T_CAST 14
#define T_CATCH 15
#define T_CLASS 16
/* TODO - remove and resequence #define T_CLOSE_TAG 17 */
#define T_COLON 18
#define T_COLON_COLON 19
#define T_COMMA 20
#define T_CONST 21
#define T_CONTEXT_RESERVED_ID 22
#define T_CONTINUE 23
#define T_DEBUGGER 24
#define T_DECIMAL 25                    //  TODO unused
#define T_DECREMENT 26
#define T_DEFAULT 27
#define T_DELETE 28
#define T_DIV 29
#define T_DIV_ASSIGN 30
#define T_DO 31
#define T_DOT 32
#define T_DOT_DOT 33
#define T_DOT_LESS 34
#define T_DOUBLE 35
#define T_DYNAMIC 36
#define T_EACH 37
#define T_ELIPSIS 38
#define T_ELSE 39
#define T_ENUMERABLE 40
#define T_EOF 41
#define T_EQ 42
#define T_EXTENDS 43
#define T_FALSE 44
#define T_FINAL 45
#define T_FINALLY 46
#define T_FLOAT 47
#define T_FOR 48
#define T_FUNCTION 49
#define T_GE 50
#define T_GET 51
#define T_GOTO 52
#define T_GT 53
#define T_ID 54
#define T_IF 55
#define T_IMPLEMENTS 56
#define T_IMPORT 57
#define T_IN 58
#define T_INCLUDE 59
#define T_INCREMENT 60
#define T_INSTANCEOF 61
#define T_INT 62
#if UNUSED
#define T_INT64 63
#endif
#define T_INTERFACE 64
#define T_INTERNAL 65
#define T_INTRINSIC 66
#define T_IS 67
#define T_LBRACE 68
#define T_LBRACKET 69
#define T_LE 70
#define T_LET 71
#define T_LOGICAL_AND 72
#define T_LOGICAL_AND_ASSIGN 73
#define T_LOGICAL_NOT 74
#define T_LOGICAL_OR 75
#define T_LOGICAL_OR_ASSIGN 76
#define T_LOGICAL_XOR 77
#define T_LOGICAL_XOR_ASSIGN 78
#define T_LPAREN 79
#define T_LSH 80
#define T_LSH_ASSIGN 81
#define T_LT 82
#define T_MINUS 83
#define T_MINUS_ASSIGN 84
#define T_MINUS_MINUS 85
#define T_MOD 86
#define T_MOD_ASSIGN 87
#define T_MODULE 88
#define T_MUL 89
#define T_MUL_ASSIGN 90
#define T_NAMESPACE 91
#define T_NATIVE 92
#define T_NE 93
#define T_NEW 94
#define T_NULL 95
#define T_NUMBER 96
/* TODO - remove and resequence #define T_OPEN_TAG 97 */
#define T_OVERRIDE 98
//  TODO #define    T_PACKAGE 99
#define T_PLUS 100
#define T_PLUS_ASSIGN 101
#define T_PLUS_PLUS 102
#define T_PRIVATE 103
#define T_PROTECTED 104
#define T_PROTOTYPE 105
#define T_PUBLIC 106
#define T_QUERY 107
#define T_RBRACE 108
#define T_RBRACKET 109
#define T_READONLY 110
#define T_RETURN 111
#define T_ROUNDING 112
#define T_RPAREN 113
#define T_RSH 114
#define T_RSH_ASSIGN 115
#define T_RSH_ZERO 116
#define T_RSH_ZERO_ASSIGN 117
#define T_SEMICOLON 118
#define T_SET 119
#define T_RESERVED_NAMESPACE 120
#define T_STANDARD 121
#define T_STATIC 122
#define T_STRICT 123
#define T_STRICT_EQ 124
#define T_STRICT_NE 125
#define T_STRING 126
#define T_SUPER 127
#define T_SWITCH 128
#define T_SYNCHRONIZED 129
#define T_THIS 130
#define T_THROW 131
#define T_TILDE 132
#define T_TO 133
#define T_TRUE 134
#define T_TRY 135
#define T_TYPE 136
#define T_TYPEOF 137
#define T_UINT 138
#define T_USE 139
#define T_VAR 140
#define T_VOID 141
#define T_WHILE 142
#define T_WITH 143
#define T_XML 144
#define T_YIELD 145
#define T_EARLY_TODO_REMOVED 146
#define T_ENUM 147
#define T_HAS 148
#define T_PRECISION 149
#define T_UNDEFINED 150
#define T_BOOLEAN 151
#define T_LONG 152
#define T_VOLATILE 153
#define T_ULONG 154
#define T_HASH 155
#define T_ABSTRACT 156
#define T_CALLEE 157
#define T_GENERATOR 158
#define T_NUMBER_WORD 159
//  UNUSED 160
#define T_XML_COMMENT_START 161
#define T_XML_COMMENT_END 162
#define T_CDATA_START 163
#define T_CDATA_END 164
#define T_XML_PI_START 165
#define T_XML_PI_END 166
#define T_LT_SLASH 167
#define T_SLASH_GT 168
#define T_LIKE 169
#define T_REGEXP 170
#define T_LANG 171

#define T_NOP 172
#define T_ERR 173

/*
 *  Group masks
 */
#define G_RESERVED          0x1
#define G_CONREV            0x2
#define G_COMPOUND_ASSIGN   0x4                 /* Eg. <<= */
#define G_OPERATOR          0x8                 /* Operator overload*/

/*
 *  Attributes (including reserved namespaces)
 */
#define A_FINAL         0x1
#define A_OVERRIDE      0x2
#define A_EARLY         0x4                     /* Early binding */
#define A_DYNAMIC       0x8
#define A_NATIVE        0x10
#define A_PROTOTYPE     0x20
#define A_STATIC        0x40
#define A_ENUMERABLE    0x40

#define EC_INPUT_STREAM "__stdin__"

struct EcStream;
typedef int (*EcStreamGet)(struct EcStream *stream);

/*
 *  Stream flags
 */
#define EC_STREAM_EOL       0x1                 /* End of line */


typedef struct EcStream {
    char        *name;                          /* Stream name / filename */

    char        *currentLine;                   /* Current input source line */
    int         lineNumber;                     /* Line number in source of current token */
    int         column;                         /* Current reading position */

    char        *lastLine;                      /* Save last currentLine */
    int         lastColumn;                     /* Save last column length for putBack */

    /*
     *  In-memory copy if input source
     */
    char        *buf;                           /* Buffer holding source file */
    char        *nextChar;                      /* Ptr to next input char */
    char        *end;                           /* Ptr to one past end of buf */

    bool        eof;                            /* At end of file */

    int         flags;                          /* Input flags */
    EcStreamGet gets;                           /* Stream get another characters */

    struct EcCompiler *compiler;                /* Compiler back reference */

} EcStream;


/*
 *  Parse source code from a file
 */
typedef struct EcFileStream {
    EcStream    stream;
    MprFile     *file;
} EcFileStream;



/*
 *  Parse source code from a memory block
 */
typedef struct EcMemStream {
    EcStream    stream;
} EcMemStream;



/*
 *  Parse input from the console (or file if using ejsh)
 */
typedef struct EcConsoleStream {
    EcStream    stream;
    MprBuf      *inputBuffer;                   /* Stream input buffer */
} EcConsoleStream;



/*
 *  Program input tokens
 */
typedef struct EcToken {
    int         tokenId;
    int         subId;
    int         groupMask;

    uchar       *text;                          /* Token text */
    int         textLen;                        /* Length of text */
    int         textBufSize;                    /* Size of text buffer */

    EjsVar      *number;                        /* Any numeric literals */

    char        *filename;
    char        *currentLine;
    int         lineNumber;
    int         column;
    int         eol;                            /* At the end of the line */

    EcStream    *stream;                        /* Current input stream */
    struct EcToken *next;                       /* Putback linkage */
} EcToken;



/*
 *  Input token parsing state. Includes putback stack for N lookahead.
 */
typedef struct EcInput {
    EcStream    *stream;

    int         state;                  /* Lexer state */

    EcToken     *putBack;               /* List of putback tokens */
    EcToken     *token;                 /* Current token */
    EcToken     *freeTokens;            /* Free list of tokens */

    char        *doc;                   /* Last doc token */

    struct EcLexer *lexer;              /* Owning lexer */
    struct EcInput *next;               /* List of input streams */
    struct EcCompiler *compiler;        /* Reference to compiler */
} EcInput;



typedef struct EcLexer {
    MprHashTable        *keywords;
    EcInput             *input;         /* List of input streams */
    struct EcCompiler   *compiler;      /* Owning compiler */
} EcLexer;



/*
 *  Jump types
 */
#define EC_JUMP_BREAK       0x1
#define EC_JUMP_CONTINUE    0x2
#define EC_JUMP_GOTO        0x4

typedef struct EcJump {
    int             kind;               /* Break, continue */
    int             offset;             /* Code offset to patch */
    EcNode          *node;              /* Owning node */
} EcJump;


/*
 *  Structure for code generation buffers
 */
typedef struct EcCodeGen {
    MprBuf      *buf;                   /* Code generation buffer */
    MprList     *jumps;                 /* Break/continues to patch for this code block */
    MprList     *exceptions;            /* Exception handlers for this code block */
    int         jumpKinds;              /* Kinds of jumps allowed */
    int         stackCount;             /* Stack item counter */
} EcCodeGen;


/*
 *  Current parse state. Each non-terminal production has its own state.
 *  Some state fields are inherited. We keep a linked list from EcCompiler.
 */
typedef struct EcState {
    /*
     *  TODO - group into EcInheritableState and EcPrivateState. Then can use
     *  structur assignment.
     */
    /*
     *  Inherited fields. These are inherited by new states.
     */
    //  TODO - OPT - compress into bit mask
    int             inModule;                   /* Inside a module declaration */
    int             inClass;                /* Inside a class declaration */
    int             inFunction;             /* Inside a function declaration */
    int             inMethod;               /* Inside a method declaration */
    int             blockIsMethod;          /* Current function is a method */
    int             inHashExpression;       /* Inside a # expression */
    int             inSettings;             /* Inside constructor settings */

    /*
     *  These are used when parsing
     */
    EjsModule       *currentModule;         /* Current open module definition */
    EjsName         currentClassName;       /* Current open class name - Used only in ecParse */
    EcNode          *currentClassNode;      /* Current open class */
    EcNode          *currentFunctionNode;   /* Current open method */
    int             noin;                   /* Don't allow "in" */

    //  TODO - rename
    EcNode          *topVarBlockNode;       /* Top var block node */

    /*
     *  These are used when doing AST processing and code generation
     */
    EjsType         *currentClass;          /* Current open class */

    //  TODO - should be using frame->currentMethod
    EjsFunction     *currentFunction;       /* Current open method */

    //  TODO - can this be derrived from currentMethod?
    cchar           *currentFunctionName;   /* Current method name */

    EjsVar          *letBlock;              /* Block for local block scope declarations */
    EjsVar          *varBlock;              /* Block for var declarations */
    EjsVar          *optimizedLetBlock;     /* Optimized let block declarations - may equal ejs->global */
    EcNode          *letBlockNode;

    /*
     *  The defaultNamespace comes from "use default namespace NAME" and is used for new declarations in the top level block
     *  where the pragma was defined. ie. it is not passed into classes or functions (1 level deep).
     *  If the defaultNamespace is not defined, then the namespace field is used.
     */
    cchar           *namespace;             /* Namespace for declarations */
    cchar           *defaultNamespace;      /* Default namespace for new top level declarations. Does not propagate */
    int             namespaceCount;         /* Count of namespaces originally in block. Used to pop namespaces */

    //  TODO - should change this to include functions also
    EcNode          *currentObjectNode;     /* Left object in "." or "[" */

    int             preserveStackCount;     /* If reset needed, preserve this count of elements */
    int             needsStackReset;        /* Stack must be reset before jumping */
    int             needsValue;             /* Express must yield a value */
    int             onLeft;                 /* On the left of an assignment */
    int             saveOnLeft;             /* Saved left of an assignment */
    int             conditional;            /* In branching conditional */
    int             mode;                   /* Compiler checking mode: Strict, standard*/
    int             lang;                   /* Language specification level: ecma, plus, fixed */
    int             inheritedTraits;        /* Inherited traits from current block */
    bool            disabled;               /* Disable nodes below this scope */

    struct EcCodeGen    *code;              /* Global and function code buffer */
    struct EcCodeGen    *staticCodeBuf;     /* Class static level code generation buffer */
    struct EcCodeGen    *instanceCodeBuf;   /* Class instance level code generation buffer */

    MprList         *namespaces;            /* List of open namespaces */

    /*
     *  TODO refactor or source via some other way
     */
    int             inInterface;            /* Inside an interface */
    int             instanceCode;           /* Generating instance class code */

    struct EcState  *prev;                  /* State stack */
    struct EcState  *prevBlockState;        /* Block state stack */
    struct EcState  *breakState;            /* State for breakable blocks */
    int             blockNestCount;         /* Count of blocks encountered. Used by ejs shell */

    int             stateLevel;             /* State level counter */

} EcState;


extern int      ecEnterState(struct EcCompiler *cp);
extern void     ecLeaveState(struct EcCompiler *cp);
extern EcNode   *ecLeaveStateWithResult(struct EcCompiler *cp,  struct EcNode *np);
extern int      ecResetModule(struct EcCompiler *cp, struct EcNode *np);
extern void     ecStartBreakableStatement(struct EcCompiler *cp, int kinds);


/*
 *  Primary compiler control structure
 */
typedef struct EcCompiler {
    /*
     *  Properties ordered to make debugging easier
     */
    int         phase;                      /* Ast processing phase */
    EcState     *state;                     /* Current state */

#if BLD_DEBUG
    char        *currentLine;               /* Current input source code line */
#endif

    EcToken     *peekToken;                 /* Alias for peek ahead */
    EcToken     *token;                     /* Alias for lexer->input->token */

#if BLD_DEBUG
    char        *tokenName;                 /* Name of last consumed token */
    char        *peekTokenName;             /* Name of lookahead token */
#endif

    EcState     *fileState;                 /* Top level state for the file */
    EcState     *classState;                /* State for the current class - used in parse */
    EcState     *directiveState;            /* State for the current directive - used in parse and CodeGen */
    EcState     *blockState;                /* State for the current block */

    EjsLookup   lookup;                     /* Lookup residuals */

    int         currentLineNumber;          /* Current input source line number */
    cchar       *currentFilename;           /* Current input file name */

    EcLexer     *lexer;                     /* Lexical analyser */
    EcInput     *input;                     /* Alias for lexer->input */
    EjsService  *vmService;                 /* VM runtime */
    Ejs         *ejs;                       /* Interpreter instance */

    /*
     *  Compiler command line options
     */
    char        *certFile;                  /* Certificate to sign the module file */
    bool        debug;                      /* Run in debug mode */
    bool        doc;                        /* Include documentation strings in output */
    bool        empty;                      /* Create interpreter empty of system types */
    char        *extraFiles;                /* Extra source files to compile */
    MprList     *useModules;                /* List of modules to pre-load */
    bool        interactive;                /* Interactive use (ejsh) */
    bool        merge;                      /* Merge all dependent modules */
    bool        nobind;                     /* Don't bind properties to slots */
    bool        noout;                      /* Don't generate any module output files */
    int         optimizeLevel;              /* Optimization factor (0-9) */
    bool        shbang;                     /* Observe #!/path as the first line of a script */
    int         warnLevel;                  /* Warning level factor (0-9) */

    int         buildEndian;                /* Build system endian */
    int         hostEndian;                 /* Host system endian */

    int         defaultMode;                /* Compiler checking mode: strict, standard */
    int         lang;                       /* Language compliance level: ecma|plus|fixed */
    char        *outputFile;                /* Output module file name override */
    MprFile     *file;                      /* Current output file handle */

    int         bindGlobals;                /* Bind global references */
    int         parseOnly;                  /* Run the compiled code */
    int         run;                        /* Run the compiled code */
    int         strip;                      /* Strip debug symbols */
    int         tabWidth;                   /* For error reporting "^" */

    MprList     *modules;                   /* List of modules to process */
    MprList     *fixups;                    /* Type reference fixups */

    char        *ejsPath;                   /* Module file search path */

    int         error;                      /* Unresolved parse error */
    int         fatalError;                 /* Any a fatal error - Can't continue */
    int         memError;                   /* Memory error */

    int         errorCount;                 /* Count of all errors */
    int         warningCount;               /* Count of all warnings */

    int         nextSeqno;                  /* Node sequence numbers */
    int         blockLevel;                 /* Level of nest in blocks */

    /*
     *  TODO - aggregate these into flags
     */
    int         peeking;                    /* Parser is doing peek() */
    int         lastOpcode;                 /* Last opcode encoded */
    int         uid;                        /* Unique identifier generator */

} EcCompiler;


/********************************** Prototypes *******************************/

extern int          ecAddModule(EcCompiler *cp, EjsModule *mp);
extern EcNode       *ecAppendNode(EcNode *np, EcNode *child);
extern int          ecAstFixup(EcCompiler *cp, struct EcNode *np);
extern EcNode       *ecChangeNode(EcNode *np, EcNode *oldNode, EcNode *newNode);
extern void         ecGenConditionalCode(EcCompiler *cp, EcNode *np, EjsModule *up);
extern int          ecCodeGen(EcCompiler *cp, int argc, struct EcNode **nodes);
extern int          ecCompile(EcCompiler *cp, int argc, char **path, int flags);
extern EcLexer      *ecCreateLexer(EcCompiler *cp);
extern void         ecDestroyLexer(EcCompiler *cp);
EcCompiler          *ecCreateCompiler(struct Ejs *ejs, int flags, int langLevel);
extern EcNode       *ecCreateNode(EcCompiler *cp, int kind);
extern void         ecFreeToken(EcInput *input, EcToken *token);
extern char         *ecGetErrorMessage(EcCompiler *cp);
extern char         *ecGetInputStreamName(EcLexer *lp);
extern int          ecGetToken(EcInput *input);
extern int          ecGetRegExpToken(EcInput *input);
extern EcNode       *ecLinkNode(EcNode *np, EcNode *child);
extern EjsModule    *ecLookupModule(EcCompiler *cp, cchar *name);
extern int          ecLookupScope(EcCompiler *cp, EjsName *name, bool anySpace);
extern int          ecLookupVar(EcCompiler *cp, EjsVar *vp, EjsName *name, bool anySpace);
extern EcNode       *ecParseWarning(EcCompiler *cp, char *fmt, ...);
extern int          ecPeekToken(EcCompiler *cp);
extern int          ecPutSpecificToken(EcInput *input, EcToken *token);
extern int          ecPutToken(EcInput *input);
extern void         ecReportError(EcCompiler *cp, cchar *severity, cchar *filename, int lineNumber,
                        char *currentLine, int column, char *msg);
extern void         ecResetInput(EcCompiler *cp);
extern EcNode       *ecResetError(EcCompiler *cp, EcNode *np, bool eatInput);
extern int          ecRemoveModule(EcCompiler *cp, EjsModule *mp);
extern void         ecResetParser(EcCompiler *cp);
extern int          ecResetModuleList(EcCompiler *cp);
extern int          ecOpenConsoleStream(EcLexer *lp, EcStreamGet gets);
extern int          ecOpenFileStream(EcLexer *input, cchar *path);
extern int          ecOpenMemoryStream(EcLexer *input, const uchar *buf, int len);
extern void         ecCloseStream(EcLexer *input);
extern EcToken      *ecTakeToken(EcInput *input);

extern void         ecSetOptimizeLevel(EcCompiler *cp, int level);
extern void         ecSetWarnLevel(EcCompiler *cp, int level);
extern void         ecSetDefaultMode(EcCompiler *cp, int mode);
extern void         ecSetTabWidth(EcCompiler *cp, int width);
extern void         ecSetOutputFile(EcCompiler *cp, cchar *outputFile);
extern void         ecSetCertFile(EcCompiler *cp, cchar *certFile);
extern void         ecSetHostEndian(EcCompiler *cp, int endian);

extern int          ecAstProcess(struct EcCompiler *cp, int argc,  struct EcNode **nodes);

#endif /* _h_EC_COMPILER */

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
