/**
 *  ecParser. Parse ejscript source files.
 *
 *  Parse source and create an internal abstract syntax tree of nodes representing the program.
 *
 *  The Abstract Syntax Tree (AST) is comprised of a linked set of EcNodes. EjsNodes have a left and right pointer.
 *  Node with a list of children are represented by right hand links.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"
#include    "ecCompiler.h"

/********************************** Defines ***********************************/

#define peekToken(cp)   peekAheadToken(cp, 1)

/*
 *  State level macros. Enter/Leave manage state and inheritance of state.
 */
#undef ENTER
#define ENTER(a)        if (ecEnterState(a) < 0) { return 0; } else

#undef LEAVE
#define LEAVE(cp, np)   ecLeaveStateWithResult(cp, np)

/***************************** Forward Declarations ***************************/

static void     addTokenToBuf(EcCompiler *cp, EcNode *np);
static void     appendDocString(EcCompiler *cp, EcNode *np, EcNode *parameter, EcNode *value);
static EcNode   *appendNode(EcNode *top, EcNode *np);
static void     applyAttributes(EcCompiler *cp, EcNode *np, EcNode *attributes, cchar *namespaceName);
static void     copyDocString(EcCompiler *cp, EcNode *np, EcNode *from);
static EcNode   *createAssignNodeInner(EcCompiler *cp, EcNode *lhs, EcNode *rhs, EcNode *parent);
static EcNode   *createBinaryNodeInner(EcCompiler *cp, EcNode *lhs, EcNode *rhs, EcNode *parent);
static EcNode   *createNameNode(EcCompiler *cp, cchar *name, cchar *space);
static EcNode   *createNamespaceNode(EcCompiler *cp, cchar *name, bool isDefault, bool isLiteral);
static EcNode   *createNode(EcCompiler *cp, int kind);
static void     dummy(int junk);
static EcNode   *expected(EcCompiler *cp, const char *str);
static int      getToken(EcCompiler *cp);
static EcNode   *insertNode(EcNode *top, EcNode *np, int pos);
static EcNode   *linkNode(EcNode *np, EcNode *node);
static EjsVar   *loadScript(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv);
static const char *getExt(const char *path);
static EcNode   *parseAnnotatableDirective(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseArgumentList(EcCompiler *cp);
static EcNode   *parseArguments(EcCompiler *cp);
static EcNode   *parseArrayType(EcCompiler *cp);
static EcNode   *parseAssignmentExpression(EcCompiler *cp);
static EcNode   *parseAttribute(EcCompiler *cp);
static EcNode   *parseAttributeName(EcCompiler *cp);
static EcNode   *parseBlock(EcCompiler *cp);
static EcNode   *parseBlockStatement(EcCompiler *cp);
static EcNode   *parseBrackets(EcCompiler *cp);
static EcNode   *parseBreakStatement(EcCompiler *cp);
static EcNode   *parseCaseElements(EcCompiler *cp);
static EcNode   *parseCaseLabel(EcCompiler *cp);
static EcNode   *parseCatchClause(EcCompiler *cp);
static EcNode   *parseCatchClauses(EcCompiler *cp);
static EcNode   *parseClassBody(EcCompiler *cp);
static EcNode   *parseClassDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseClassInheritance(EcCompiler *cp);
static EcNode   *parseClassName(EcCompiler *cp);
static EcNode   *parseConstructorSignature(EcCompiler *cp, EcNode *np);
static EcNode   *parseConstructorInitializer(EcCompiler *cp);
static EcNode   *parseContinueStatement(EcCompiler *cp);
static EcNode   *parseDirective(EcCompiler *cp);
static EcNode   *parseDirectives(EcCompiler *cp);
static EcNode   *parseDoStatement(EcCompiler *cp);
static EcNode   *parseDirectivesPrefix(EcCompiler *cp);
static EcNode   *parseElementList(EcCompiler *cp, EcNode *newNode);
static EcNode   *parseElements(EcCompiler *cp, EcNode *newNode);
static EcNode   *parseElementTypeList(EcCompiler *cp);
static EcNode   *parseFieldList(EcCompiler *cp, EcNode *np);
static EcNode   *parseEmptyStatement(EcCompiler *cp);
static EcNode   *parseError(EcCompiler *cp, char *fmt, ...);
static EcNode   *parseExpressionStatement(EcCompiler *cp);
static EcNode   *parseFieldName(EcCompiler *cp);
static EcNode   *parseForStatement(EcCompiler *cp);
static EcNode   *parseFunctionDeclaration(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseFunctionDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseFunctionBody(EcCompiler *cp, EcNode *fun);
static EcNode   *parseFunctionExpression(EcCompiler *cp);
static EcNode   *parseFunctionExpressionBody(EcCompiler *cp);
static EcNode   *parseFunctionName(EcCompiler *cp);
static EcNode   *parseFunctionSignature(EcCompiler *cp, EcNode *np);
static EcNode   *parseHashStatement(EcCompiler *cp);
static EcNode   *parseIdentifier(EcCompiler *cp);
static EcNode   *parseIfStatement(EcCompiler *cp);
static EcNode   *parseInterfaceBody(EcCompiler *cp);
static EcNode   *parseInterfaceInheritance(EcCompiler *cp);
static EcNode   *parseInitializerList(EcCompiler *cp, EcNode *np);
static EcNode   *parseInitializer(EcCompiler *cp);
static EcNode   *parseParameter(EcCompiler *cp, bool rest);
static EcNode   *parseParameterInit(EcCompiler *cp, EcNode *args);
static EcNode   *parseInterfaceDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseLabeledStatement(EcCompiler *cp);
static EcNode   *parseLeftHandSideExpression(EcCompiler *cp);
static EcNode   *parseLetBindingList(EcCompiler *cp);
static EcNode   *parseLetExpression(EcCompiler *cp);
static EcNode   *parseLetStatement(EcCompiler *cp);
static EcNode   *parseLiteralElement(EcCompiler *cp);
static EcNode   *parseLiteralField(EcCompiler *cp);
static EcNode   *parseListExpression(EcCompiler *cp);
static EcNode   *parseNamespaceAttribute(EcCompiler *cp);
static EcNode   *parseNamespaceDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseNamespaceInitialisation(EcCompiler *cp, EcNode *nameNode);
static EcNode   *parseNonemptyParameters(EcCompiler *cp, EcNode *list);
static EcNode   *parseNullableTypeExpression(EcCompiler *cp);
static EcNode   *parseOptionalExpression(EcCompiler *cp);
static EcNode   *parseOverloadedOperator(EcCompiler *cp);
static EcNode   *parseParenListExpression(EcCompiler *cp);
static EcNode   *parseParameterisedTypeName(EcCompiler *cp);
static EcNode   *parseParameterKind(EcCompiler *cp);
static EcNode   *parseParameters(EcCompiler *cp, EcNode *args);
static EcNode   *parsePath(EcCompiler *cp, EcNode *lhs);
static EcNode   *parsePattern(EcCompiler *cp);
static EcNode   *parsePragmaItems(EcCompiler *cp, EcNode *np);
static EcNode   *parsePragmaItem(EcCompiler *cp);
static EcNode   *parsePragmas(EcCompiler *cp, EcNode *np);
static EcNode   *parsePrimaryExpression(EcCompiler *cp);
static EcNode   *parsePrimaryName(EcCompiler *cp);
static EcNode   *parseProgram(EcCompiler *cp, cchar *path);
static EcNode   *parsePropertyName(EcCompiler *cp);
static EcNode   *parsePropertyOperator(EcCompiler *cp);
static EcNode   *parseQualifiedNameIdentifier(EcCompiler *cp);
static EcNode   *parseRegularExpression(EcCompiler *cp);
static EcNode   *parseReservedNamespace(EcCompiler *cp);
static EcNode   *parseRestParameter(EcCompiler *cp);
static EcNode   *parseResultType(EcCompiler *cp);
static EcNode   *parseReturnStatement(EcCompiler *cp);
static EcNode   *parseSimplePattern(EcCompiler *cp);
static EcNode   *parseSimpleQualifiedName(EcCompiler *cp);
static EcNode   *parseStatement(EcCompiler *cp);
static EcNode   *parseSubstatement(EcCompiler *cp);
static EcNode   *parseSuperInitializer(EcCompiler *cp);
static EcNode   *parseSwitchStatement(EcCompiler *cp);
static EcNode   *parseThrowStatement(EcCompiler *cp);
static EcNode   *parseTryStatement(EcCompiler *cp);
static EcNode   *parseTypeDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseTypeExpression(EcCompiler *cp);
static EcNode   *parseTypeIdentifierList(EcCompiler *cp);
static EcNode   *parseTypeInitialisation(EcCompiler *cp);
static EcNode   *parseModuleBody(EcCompiler *cp);
static EcNode   *parseModuleName(EcCompiler *cp);
static EcNode   *parseModuleDefinition(EcCompiler *cp);
static EcNode   *parseUsePragma(EcCompiler *cp, EcNode *np);
static EcNode   *parseVariableBinding(EcCompiler *cp, EcNode *varList, EcNode *attributes);
static EcNode   *parseVariableBindingList(EcCompiler *cp, EcNode *list, EcNode *attributes);
static EcNode   *parseVariableDefinition(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseVariableDefinitionKind(EcCompiler *cp, EcNode *attributes);
static EcNode   *parseVariableInitialisation(EcCompiler *cp);
static EcNode   *parseWhileStatement(EcCompiler *cp);
static EcNode   *parseWithStatement(EcCompiler *cp);
struct EcNode   *parseXMLAttribute(EcCompiler *cp, EcNode *np);
struct EcNode   *parseXMLAttributes(EcCompiler *cp, EcNode *np);
struct EcNode   *parseXMLElement(EcCompiler *cp, EcNode *np);
struct EcNode   *parseXMLElementContent(EcCompiler *cp, EcNode *np);
struct EcNode   *parseXMLTagContent(EcCompiler *cp, EcNode *np);
struct EcNode   *parseXMLTagName(EcCompiler *cp, EcNode *np);
static EcNode   *parseYieldExpression(EcCompiler *cp);
static int      peekAheadToken(EcCompiler *cp, int ahead);
static EcToken  *peekAheadTokenStruct(EcCompiler *cp, int ahead);
static void     putSpecificToken(EcCompiler *cp, EcToken *token);
static void     putToken(EcCompiler *cp);
static EcNode   *removeNode(EcNode *np, EcNode *child);
static void     setNodeDoc(EcCompiler *cp, EcNode *np);
static void     setId(EcNode *np, char *name);
static EcNode   *unexpected(EcCompiler *cp);
static void     updateTokenInfo(EcCompiler *cp);

#if BLD_DEBUG
/*
 *  Just for debugging. Generated via tokens.ksh
 */
char *tokenNames[] = {
    "",
    "as",
    "assign",
    "at",
    "attribute",
    "bit_and",
    "bit_and_assign",
    "bit_or",
    "bit_or_assign",
    "bit_xor",
    "bit_xor_assign",
    "break",
    "call",
    "case",
    "cast",
    "catch",
    "class",
    "close_tag",
    "colon",
    "colon_colon",
    "comma",
    "const",
    "context_reserved_id",
    "continue",
    "debugger",
    "decimal",
    "decrement",
    "default",
    "delete",
    "div",
    "div_assign",
    "do",
    "dot",
    "dot_dot",
    "dot_less",
    "double",
    "dynamic",
    "each",
    "elipsis",
    "else",
    "enumerable",
    "eof",
    "eq",
    "extends",
    "false",
    "final",
    "finally",
    "float",
    "for",
    "function",
    "ge",
    "get",
    "goto",
    "gt",
    "id",
    "if",
    "implements",
    "import",                   //  UNUSED
    "in",
    "include",
    "increment",
    "instanceof",
    "int",
    "int64",
    "interface",
    "internal",
    "intrinsic",
    "is",
    "lbrace",
    "lbracket",
    "le",
    "let",
    "logical_and",
    "logical_and_assign",
    "logical_not",
    "logical_or",
    "logical_or_assign",
    "logical_xor",
    "logical_xor_assign",
    "lparen",
    "lsh",
    "lsh_assign",
    "lt",
    "minus",
    "minus_assign",
    "minus_minus",
    "mod",
    "module",
    "mod_assign",
    "mul",
    "mul_assign",
    "namespace",
    "native",
    "ne",
    "new",
    "null",
    "number",
    "open_tag",
    "override",
    "package",              //  TODO UNUSED
    "plus",
    "plus_assign",
    "plus_plus",
    "private",
    "protected",
    "prototype",
    "public",
    "query",
    "rbrace",
    "rbracket",
    "readonly",
    "return",
    "rounding",             //  TODO UNUSED
    "rparen",
    "rsh",
    "rsh_assign",
    "rsh_zero",
    "rsh_zero_assign",
    "semicolon",
    "set",
    "nspace",
    "standard",
    "static",
    "strict",
    "strict_eq",
    "strict_ne",
    "string",
    "super",
    "switch",
    "sync",
    "this",
    "throw",
    "tilde",
    "to",
    "true",
    "try",
    "type",
    "typeof",
    "uint",
    "use",
    "var",
    "void",
    "while",
    "with",
    "xml",                  //  UNUSED
    "yield",
    "early",
    "enum",
    "has",
    "precision",            //  UNUSED
    "undefined",
    "boolean",
    "long",
    "volatile",
    "ulong",
    "hash",
    "abstract",
    "callee",
    "generator",
    "number",
    "UNUSED-unit",
    "xml_comment_start",
    "xml_comment_end",
    "cdata_start",
    "cdata_end",
    "xml_pi_start",
    "xml_pi_end",
    "lt_slash",
    "slash_gt",
    "like",
    "regexp",
    "language",
    "nop",
    "err",
    0,
};


/*
 *  Just for debugging. Generated via tokens.ksh
 */
char *nodes[] = {
    "",
    "n_value",
    "n_literal",
    "N_qname",
    "n_import",             //  UNUSED
    "n_binary_type_op",
    "n_binary_op",
    "n_assign_op",
    "n_number_type",
    "n_unary_op",
    "n_if",
    "n_var_definition",
    "n_pragma",
    "N_namespace_definition",
    "n_function",
    "n_parameter",
    "n_class",
    "n_package",                    //  UNUSED
    "n_directives",
    "n_type",                       //  UNUSED
    "n_program",
    "n_packages",                   //  UNUSED
    "n_expressions",
    "n_pragmas",
    "n_type_identifiers",
    "n_block",
    "n_dot",
    "n_return",
    "n_call",
    "n_args",
    "n_this",
    "n_new",
    "n_for",
    "n_for_in",
    "n_postfix_op",
    "n_super",
    "n_try",
    "n_catch",
    "n_catch_clauses",
    "n_throw",
    "n_end_function",
    "n_nop",
    "n_ref",
    "n_switch",
    "n_case_label",
    "n_case_elements",
    "n_break",
    "n_continue",
    "n_goto",
    "n_use_namespace",
    "n_attributes",
    "n_do",
    "n_module",
    "n_use_module",
    "n_void",
    "n_hash",
    "n_object_literal",
    "n_field",
    "n_array_literal",
    "n_catch_arg",
    "n_with",
    0,
};


#endif  /* BLD_DEBUG */

#define createBinaryNode(cp, lhs, rhs) createBinaryNodeInner(cp, lhs, rhs, createNode(cp, N_BINARY_OP))
#define createAssignNode(cp, lhs, rhs) createAssignNodeInner(cp, lhs, rhs, createNode(cp, N_ASSIGN_OP))

/************************************ Code ************************************/
/*
 *  Create a compiler instance
 */

EcCompiler *ecCreateCompiler(Ejs *ejs, int flags, int langLevel)
{
    EcCompiler      *cp;

    cp = mprAllocObjWithDestructorZeroed(ejs, EcCompiler, NULL);
    if (cp == 0) {
        return 0;
    }

    cp->ejs = ejs;
    cp->defaultMode = PRAGMA_MODE_STANDARD;
    cp->lang = langLevel;
    cp->tabWidth = EC_TAB_WIDTH;
    cp->warnLevel = 1;
    cp->shbang = 1;
    cp->optimizeLevel = 9;
    cp->warnLevel = 1;

    /*
     *  Default to non-cross compilation
     */
    cp->buildEndian = mprGetEndian(ejs);
    cp->hostEndian = cp->buildEndian;

    if (flags & EC_FLAGS_EMPTY) {
        cp->empty = 1;
    }
    if (flags & EC_FLAGS_RUN) {
        cp->run = 1;
    }
    if (flags & EC_FLAGS_MERGE) {
        cp->merge = 1;
    }
    if (flags & EC_FLAGS_BIND_GLOBALS) {
        cp->bindGlobals = 1;
    }
    if (flags & EC_FLAGS_NO_BIND) {
        cp->nobind = 1;
    }
    if (flags & EC_FLAGS_NO_OUT) {
        cp->noout = 1;
    }
    if (flags & EC_FLAGS_DEBUG) {
        cp->debug = 1;
    }

    if (ecResetModuleList(cp) < 0) {
        mprFree(cp);
        return 0;
    }

    cp->lexer = ecCreateLexer(cp);
    if (cp->lexer == 0) {
        mprFree(cp);
        return 0;
    }
    ecResetParser(cp);

    if (!cp->empty) {
        ejsBindFunction(ejs, ejs->globalBlock, ES_load, loadScript);
    }

    return cp;
}


/**
 *  Load a script or module. Name should have an extension. Name will be located according to the EJSPATH search strategy.
 *
 *  public static function load(name: String): void
 */
static EjsVar *loadScript(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    cchar       *name, *cp;
    char        *path;

    name = ejsGetString(argv[0]);

    if ((cp = strrchr(name, '.')) != NULL && strcmp(cp, ".es") == 0) {
        if (ejsSearch(ejs, &path, name) < 0) {
            ejsThrowIOError(ejs, "Can't find %s to load", name);
        } else {
            ejsLoadScript(ejs, path, EC_FLAGS_RUN | EC_FLAGS_NO_OUT | EC_FLAGS_DEBUG | EC_FLAGS_BIND_GLOBALS);
        }

    } else {
        if (ejsSearch(ejs, &path, name) < 0) {
            ejsThrowIOError(ejs, "Can't find %s to load", name);
        } else {
            ejsLoadModule(ejs, path, NULL, NULL, 0);
        }
    }
    return 0;
}


/*
 *  Compile the input stream and parse all directives into the given nodes reference.
 *  path is optional.
 */
static int compileInput(EcCompiler *cp, EcNode **nodes, cchar *path)
{
    EcNode      *np;

    mprAssert(cp);
    mprAssert(nodes);

    *nodes = 0;

    if (ecEnterState(cp) < 0) {
        return EJS_ERR;
    }

    /*
     *  Alias for convenient access. TODO - who maintains these. Should have an API for this.
     */
    cp->input = cp->lexer->input;
    cp->token = cp->lexer->input->token;

    cp->fileState = cp->state;
    cp->fileState->mode = cp->defaultMode;
    cp->blockState = cp->state;

    if (cp->shbang) {
        if (getToken(cp) == T_HASH && peekToken(cp) == T_LOGICAL_NOT) {
            while (cp->token->lineNumber <= 1 && cp->token->tokenId != T_EOF && cp->token->tokenId != T_NOP) {
                getToken(cp);
            }
        }
        putToken(cp);
    }

    np = parseProgram(cp, path);
    mprAssert(np || cp->error);

    np = ecLeaveStateWithResult(cp, np);
    *nodes = np;

    cp->fileState = 0;

    if (np == 0 || cp->errorCount > 0) {
        return EJS_ERR;
    }
    return 0;
}


/*
 *  Compile a source file and parse all directives into the given nodes reference.
 *  This may be called with the input stream already setup to parse a script.
 */
static int parseFile(EcCompiler *cp, char *path, EcNode **nodes)
{
    int         rc, opened;

    mprAssert(path);
    mprAssert(nodes);

    opened = 0;

    if (cp->lexer->input->stream == 0) {
        if (ecOpenFileStream(cp->lexer, path) < 0) {
            parseError(cp, "Can't open %s", path);
            return EJS_ERR;
        }
        opened = 1;
    }

    rc = compileInput(cp, nodes, path);

    if (opened) {
        ecCloseStream(cp->lexer);
    }

    return rc;
}


/*
 *  Compile a file.
 */
int ecCompile(EcCompiler *cp, int argc, char **argv, int flags)
{
    Ejs         *ejs;
    EjsModule   *mp;
    EcNode      **nodes;
    EjsBlock    *block;
    MprList     *modules;
    cchar       *ext;
    int         saveFlags, i, j, next;

    ejs = cp->ejs;
    saveFlags = ejs->flags;
    ejs->flags |= EJS_FLAG_COMPILER;

    nodes = (EcNode**) mprAllocZeroed(cp, sizeof(EcNode*) * argc);
    if (nodes == 0) {
        return EJS_ERR;
    }

    /*
     *  Warn about source files mentioned multiple times.
     */
    for (i = 0; i < argc; i++) {
        for (j = 0; j < argc; j++) {
            if (i == j) {
                continue;
            }
            if (strcmp(argv[i], argv[j]) == 0) {
                parseError(cp, "Loading source %s multiple times. Ignoring extra copies.", argv[i]);
                return EJS_ERR;
            }
        }
        if (cp->outputFile && strcmp(cp->outputFile, argv[i]) == 0) {
            parseError(cp, "Output file is the same as input file: %s", argv[i]);
            return EJS_ERR;
        }
    }

    /*
     *  Compile source files and load any module files
     */
    for (i = 0; i < argc && !cp->fatalError; i++) {
        ext = getExt(argv[i]);

        if (mprStrcmpAnyCase(ext, EJS_MODULE_EXT) == 0 || mprStrcmpAnyCase(ext, BLD_SHOBJ) == 0) {
            if ((modules = ejsLoadModule(cp->ejs, argv[i], NULL, NULL, EJS_MODULE_DONT_INIT)) == 0) {
                parseError(cp, "Can't load module file %s\n%s", argv[i], ejsGetErrorMsg(cp->ejs, 0));
                return EJS_ERR;
            }
            if (cp->merge) {
                /*
                 *  If merging, we must emit the loaded module into the output. So add to the compiled modules list.
                 */
                for (next = 0; (mp = mprGetNextItem(modules, &next)) != 0; ) {
                    if (mprLookupItem(cp->modules, mp) < 0 && mprAddItem(cp->modules, mp) < 0) {
                        parseError(cp, "Can't add module %s", mp->name);
                    }
                }
            }
            mprFree(modules);
            nodes[i] = 0;

        } else  {
            parseFile(cp, argv[i], &nodes[i]);
        }
    }

    /*
     *  Allocate the eval frame stack. This is used for property lookups. We have one dummy block at the top always.
     */
    block = ejsCreateBlock(ejs, "top", 0);
    ejs->frame = ejsPushFrame(ejs, block);
    
    /*
     *  Process the internal representation and generate code
     */
    if (!cp->parseOnly && cp->errorCount == 0) {

        ecResetParser(cp);
        if (ecAstProcess(cp, argc, nodes) < 0) {
            ejsPopFrame(ejs);
            mprFree(nodes);
            return EJS_ERR;
        }

        /*
         *  TODO createNode will set cp->token which causes "^" in error messages which are only
         *  appropriate during the parse phase. When listings are removed and done by the disassembler,
         *  we can call ecDestroyLexer() earlier and remove calls to ResetParser.
         */
        if (cp->errorCount == 0) {
            ecResetParser(cp);
            if (ecCodeGen(cp, argc, nodes) < 0) {
                ejsPopFrame(ejs);
                mprFree(nodes);
                return EJS_ERR;
            }
        }
    }
    ejsPopFrame(ejs);

    mprFree(nodes);

    if (cp->errorCount > 0) {
        return EJS_ERR;
    }

    ejs->flags = saveFlags;

    /*
     *  Add compiled modules to the interpreter
     */
    for (next = 0; ((mp = (EjsModule*) mprGetNextItem(cp->modules, &next)) != 0); ) {
        ejsAddModule(cp->ejs, mp);
    }
    return 0;
}


//  TODO - the names of these routines needs work
int ejsLoadScript(Ejs *ejs, cchar *path, int flags)
{
    EcCompiler      *ec;

    //  TDOO - need to be able to free compiler memory. 
    if ((ec = ecCreateCompiler(ejs, flags, BLD_FEATURE_EJS_LANG)) == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    if (ecCompile(ec, 1, (char**) &path, 0) < 0) {
        // mprFree(ec);
        return EJS_ERR;
    }
    // mprFree(ec);
    if (ejsRun(ejs) < 0) {
        return EJS_ERR;
    }
    return 0;
}


/*
 *  One-line embedding. Evaluate a file. This will compile and interpret the given Ejscript source file.
 */
int ejsEvalFile(cchar *path)
{
    EjsService      *vm;   
    Ejs             *ejs;
    Mpr             *mpr;

    mpr = mprCreate(0, NULL, NULL);
    if ((vm = ejsCreateService(mpr)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    if ((ejs = ejsCreate(vm, NULL, EJS_FLAG_COMPILER)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    if (ejsLoadScript(ejs, path, EC_FLAGS_RUN | EC_FLAGS_NO_OUT | EC_FLAGS_DEBUG | EC_FLAGS_BIND_GLOBALS) == 0) {
        mprFree(mpr);
        return MPR_ERR;
    }
    mprFree(mpr);
    return 0;
}


/*
 *  One-line embedding. Evaluate a script. This will compile and interpret the given script.
 */
int ejsEvalScript(cchar *script)
{
    EcCompiler      *ec;
    EjsService      *vm;   
    Ejs             *ejs;
    cchar           *path;
    Mpr             *mpr;

    mpr = mprCreate(0, NULL, NULL);
    if ((vm = ejsCreateService(mpr)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    if ((ejs = ejsCreate(vm, NULL, EJS_FLAG_COMPILER)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    if ((ec = ecCreateCompiler(ejs, EC_FLAGS_RUN | EC_FLAGS_NO_OUT | EC_FLAGS_DEBUG | EC_FLAGS_BIND_GLOBALS,
            BLD_FEATURE_EJS_LANG)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    if (ecOpenMemoryStream(ec->lexer, (uchar*) script, (int) strlen(script)) < 0) {
        parseError(ec, "Can't open memory stream");
        return EJS_ERR;
    }
    path = "__script__";
    if (ecCompile(ec, 1, (char**) &path, 0) < 0) {
        mprFree(mpr);
        return EJS_ERR;
    }
    ecCloseStream(ec->lexer);
    if (ejsRun(ejs) < 0) {
        mprFree(mpr);
        return EJS_ERR;
    }
    mprFree(mpr);
    return 0;
}


EjsModule *ecLookupModule(EcCompiler *cp, cchar *name)
{
    EjsModule   *mp;
    int         next;

    next = 0;
    while ((mp = (EjsModule*) mprGetNextItem(cp->modules, &next)) != 0) {
        if (strcmp(mp->name, name) == 0) {
            return mp;
        }
    }
    return 0;
}


int ecAddModule(EcCompiler *cp, EjsModule *mp)
{
    mprAssert(cp->modules);
    return mprAddItem(cp->modules, mp);
}


int ecRemoveModule(EcCompiler *cp, EjsModule *mp)
{
    mprAssert(cp->modules);
    return mprRemoveItem(cp->modules, mp);
}


int ecResetModuleList(EcCompiler *cp)
{
    mprFree(cp->modules);
    cp->modules = mprCreateList(cp);
    if (cp->modules == 0) {
        return EJS_ERR;
    }
    return 0;
}


void ecResetParser(EcCompiler *cp)
{
    cp->token = 0;
}


/************************************* Parser Productions *************************/
/*
 *  XMLComment (ECMA-357)
 *
 *  Input Sequences
 *      <!-- XMLCommentCharacters -->
 *
 *  AST
 */
static EcNode *parseXMLComment(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);

    np = 0;

    if (getToken(cp) == T_XML_COMMENT_START) {

    }
    if (getToken(cp) != T_XML_COMMENT_END) {
        return expected(cp, "-->");
    }
    return LEAVE(cp, np);
}


/*
 *  XMLCdata (ECMA-357)
 *
 *  Input Sequences
 *      <![CDATA[ XMLCDataCharacters ]]>
 *
 *  AST
 */
static EcNode *parseXMLCdata(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = 0;

    if (getToken(cp) == T_CDATA_START) {

    }
    if (getToken(cp) != T_CDATA_END) {
        return expected(cp, "-->");
    }
    return LEAVE(cp, np);
}


/*
 *  XMLPI (ECMA-357)
 *
 *  Input Sequences
 *      <? .... ?>
 *
 *  AST
 */
static EcNode *parseXMLPI(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = 0;

    if (getToken(cp) == T_XML_PI_START) {

    }
    if (getToken(cp) != T_XML_PI_END) {
        return expected(cp, "?>");
    }
    return LEAVE(cp, np);
}


/*
 *  XMLMarkup (ECMA-357)
 *      XMLComment
 *      XMLCDATA
 *      XMLPI
 *
 *  Input Sequences
 *      <!--
 *      [CDATA
 *      <?
 *
 *  AST
 */
static EcNode *parseXMLMarkup(EcCompiler *cp, EcNode *np)
{
    switch (peekToken(cp)) {
    case T_XML_COMMENT_START:
        return parseXMLComment(cp);

    case T_CDATA_START:
        return parseXMLCdata(cp);

    case T_XML_PI_START:
        return parseXMLPI(cp);

    default:
        //  TODO - should really ensure the token is eaten before returning to prevent a swamp of errors
        return LEAVE(cp, unexpected(cp));
    }
}


/*
 *  XMLText (ECMA-357)
 *
 *  Input Sequences
 *      SourceCharacters but no { or <
 *
 *  AST
 */
static EcNode *parseXMLText(EcCompiler *cp, EcNode *np)
{
    uchar   *p;
    int     count;

    //  TODO This is discarding text white space. Need a low level getXmlToken routine
    //  or need lexer to preserved inter-token white space somewhere.
    peekToken(cp);
    for (count = 0; np; count++) {
        for (p = cp->peekToken->text; p && *p; p++) {
            if (*p == '{' || *p == '<') {
                return np;
            }
        }
        if (getToken(cp) == T_EOF || cp->token->tokenId == T_ERR) {
            return 0;
        }
        if (isalnum(cp->token->text[0]) && count > 0) {
            mprPutCharToBuf(np->literal.data, ' ');
        }
        addTokenToBuf(cp, np);
        peekToken(cp);
    }
    return np;
}


/*
 *  XMLName (ECMA-357)
 *      XMLNameStart
 *      XMLName XMLNamePart
 *
 *  Input Sequences
 *      UnicodeLetter
 *      _       underscore
 *      :       colon
 *
 *  AST
 */
static EcNode *parseXMLName(EcCompiler *cp, EcNode *np)
{
    int         c;

    ENTER(cp);

    getToken(cp);
    if (cp->token == 0 || cp->token->text == 0) {
        return LEAVE(cp, unexpected(cp));
    }
    
    c = cp->token->text[0];

    //  TODO - convert to unicode
    if (isalpha(c) || c == '_' || c == ':') {
        addTokenToBuf(cp, np);

    } else {
        np = parseError(cp, "Not an XML Name \"%s\"", cp->token->text);
    }
    return LEAVE(cp, np);
}


/*
 *  XMLAttributeValue (ECMA-357)
 *      XMLDoubleStringCharacters
 *      XMLSingleStringCharacters
 *
 *  Input Sequences
 *      "
 *      '
 *
 *  AST
 *      Add data to literal.data buffer
 */
static EcNode *parseXMLAttributeValue(EcCompiler *cp, EcNode *np)
{
    if (getToken(cp) != T_STRING) {
        return expected(cp, "quoted string");
    }

    mprPutCharToBuf(np->literal.data, '\"');
    addTokenToBuf(cp, np);
    mprPutCharToBuf(np->literal.data, '\"');

    return np;
}


/*
 *  Identifier (1)
 *      ID |
 *      ContextuallyReservedIdentifier
 *
 *  Input Sequences
 *      ID
 *      ContextuallyReservedIdentifier
 *
 *  AST
 *      N_QNAME
 *          name
 *              id
 */

static EcNode *parseIdentifier(EcCompiler *cp)
{
    EcNode      *np;
    int         tid;

    ENTER(cp);

    tid = getToken(cp);
    if (cp->token->groupMask & G_CONREV) {
        tid = T_ID;
    }

    switch (tid) {
    case T_ID:
    case T_MUL:     //  TODO TODO57 - temp to allow property.*
        np = createNode(cp, N_QNAME);
        setId(np, (char*) cp->token->text);
        break;

    default:
        np = parseError(cp, "Not an identifier \"%s\"", cp->token->text);
    }
    return LEAVE(cp, np);
}


/*
 *  Qualifier (3)
 *      *
 *      Identifier
 *      ReservedNamespace
 *      "StringLiteral"                     //  TODO MOB1
 *
 *  Input Sequences:
 *      *
 *      ID
 *
 *  AST
 *      N_ATTRIBUTES
 *          namespace
 */

static EcNode *parseQualifier(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_ID:
        np = parseIdentifier(cp);
        break;

    //  TODO MOB1
    case T_STRING:
        getToken(cp);
        np = createNode(cp, N_QNAME);
        np->qname.space = mprStrdup(np, (char*) cp->token->text);
        np->literalNamespace = 1;
        break;

    case T_MUL:
        getToken(cp);
        np = createNode(cp, N_ATTRIBUTES);
        np->qname.space = mprStrdup(np, (char*) cp->token->text);
        break;

    case T_RESERVED_NAMESPACE:
        np = parseReservedNamespace(cp);
        break;

    default:
        //  TODO - should really ensure the token is eaten before returning to prevent a swamp of errors
        np = unexpected(cp);
    }
    return LEAVE(cp, np);
}


/*
 *  ReservedNamespace (6)
 *      internal
 *      intrinsic
 *      private
 *      protected
 *      public
 *
 *  Input
 *      internal
 *      intrinsic
 *      private
 *      protected
 *      public
 *
 *  AST
 *      N_ATTRIBUTES
 *          namespace
 *          attributes
 */
static EcNode *parseReservedNamespace(EcCompiler *cp)
{
    EcNode      *np;
    int         attributes;

    ENTER(cp);

    if (getToken(cp) != T_RESERVED_NAMESPACE) {
        return LEAVE(cp, expected(cp, "reserved namespace"));
    }

    np = createNode(cp, N_ATTRIBUTES);

    attributes = 0;

    switch (cp->token->subId) {
    case T_INTRINSIC:
        break;

    case T_INTERNAL:
    case T_PRIVATE:
    case T_PROTECTED:
    case T_PUBLIC:
        np->specialNamespace = 1;
        break;

    default:
        return LEAVE(cp, parseError(cp, "Unknown reserved namespace %s", cp->token->text));
    }

    np->attributes = attributes;
    np->qname.space = mprStrdup(np, (char*) cp->token->text);

    return LEAVE(cp, np);
}


/*
 *  QualifiedNameIdentifier (11)
 *      Identifier
 *      ReservedIdentifier
 *      StringLiteral
 *      NumberLiteral
 *      Brackets
 *      OverloadedOperator
 *
 *  Notes:
 *      Can be used to the right of a namespace qualifier. Eg. public::QualfiedNameIdentifier
 *
 *  Input
 *      Identifier
 *      ReservedIdentifier
 *      Number
 *      "String"
 *      [
 *      Overloaded Operator
 *
 *  AST
 *      N_QNAME
 *          name:
 *              namespace
 *              id
 *          left: N_EXPRESSIONS
 */
/*
 *  TODO MOB - what does  namespace::47, or namespace::"String" or namespace::+  mean?
 *
 */
static EcNode *parseQualifiedNameIdentifier(EcCompiler *cp)
{
    EcNode      *np;
    EjsVar      *vp;
    int         tid, reservedWord;

    ENTER(cp);

    tid = peekToken(cp);
    reservedWord = (cp->peekToken->groupMask & G_RESERVED);

    if (reservedWord) {
        np = createNode(cp, N_QNAME);
        setId(np, (char*) cp->token->text);

    } else switch (tid) {

        case T_ID:
        case T_TYPE:
            np = parseIdentifier(cp);
            break;

        case T_NUMBER:
            getToken(cp);
            np = createNode(cp, N_QNAME);
            setId(np, (char*) cp->token->text);
            vp = ejsParseVar(cp->ejs, (char*) cp->token->text, -1);
            np->literal.var = vp;
            break;

        case T_STRING:
            getToken(cp);
            np = createNode(cp, N_QNAME);
            setId(np, (char*) cp->token->text);
            vp = (EjsVar*) ejsCreateString(cp->ejs, (char*) cp->token->text);
            np->literal.var = vp;
            break;

        case T_LBRACKET:
            np = parseBrackets(cp);
            break;

        default:
            if (cp->token->groupMask == G_OPERATOR) {
                np = parseOverloadedOperator(cp);
            } else {
                //  TODO - should really ensure the token is eaten before returning to prevent a swamp of errors
                np = unexpected(cp);
            }
            break;
    }

    return LEAVE(cp, np);
}


/*
 *  SimpleQualifiedName (17)
 *      Identifier
 *      Qualifier :: QualifiedNameIdentifier
 *
 *  Notes:
 *      Optionally namespace qualified name
 *
 *  Input
 *      Identifier
 *      *
 *
 *  AST
 *      N_QNAME
 *          name
 *              id
 *          qualifier: N_ATTRIBUTES
 */
static EcNode *parseSimpleQualifiedName(EcCompiler *cp)
{
    EcNode      *np, *qualifier;

    ENTER(cp);

    //  TODO MOB1
    if (peekToken(cp) == T_MUL || cp->peekToken->tokenId == T_STRING) {

        if (peekAheadToken(cp, 2) == T_COLON_COLON) {
            np = parseQualifier(cp);

            if (getToken(cp) != T_COLON_COLON) {
                return LEAVE(cp, expected(cp, "::"));
            }
            qualifier = np;
            np = parseQualifiedNameIdentifier(cp);
            if (qualifier->kind == N_QNAME) {
                np->qname.space = mprStrdup(np, qualifier->qname.space);
                np->literalNamespace = qualifier->literalNamespace;
            } else {
                //  TODO - not supported yet
                np->qualifierNode = linkNode(np, qualifier);
            }

        } else {
            //  TODO TODO57 - temp workaround to allow propertyName.*
            np = parseIdentifier(cp);
        }

    } else {
        np = parseIdentifier(cp);
        if (peekToken(cp) == T_COLON_COLON) {
            getToken(cp);
            qualifier = np;
            np = parseQualifiedNameIdentifier(cp);
            if (np) {
                if (np && qualifier->kind == N_QNAME) {
                    np->qname.space = mprStrdup(np, qualifier->qname.name);

                } else {
                    //  TODO - not supported yet
                    np->qualifierNode = linkNode(np, qualifier);
                }
            }
        }
    }

    return LEAVE(cp, np);
}


/*
 *  ExpressionQualifiedName (15)
 *      ParenListExpression :: QualifiedNameIdentifier
 *
 *  Input
 *      ( ListExpression ) :: *
 *      ( ListExpression ) :: Identifier
 *      ( ListExpression ) :: ReservedIdentifier
 *      ( ListExpression ) :: Number
 *      ( ListExpression ) :: String
 *      ( ListExpression ) :: [ ... ]
 *      ( ListExpression ) :: OverloadedOperator
 *
 *  AST
 *      N_QNAME
 *          left: N_EXPRESSIONS
 *          qualifier: N_ATTRIBUTES
 */
static EcNode *parseExpressionQualifiedName(EcCompiler *cp)
{
    EcNode      *np, *qualifier;

    ENTER(cp);

    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, unexpected(cp));
    }

    //  TODO - not debugged yet
    qualifier = parseListExpression(cp);

    if (getToken(cp) == T_COLON_COLON) {
        np = parseQualifiedNameIdentifier(cp);
        np->qualifierNode = linkNode(np, qualifier);

    } else {
        np = expected(cp, "\"::\"");
    }

    return LEAVE(cp, np);
}


/*
 *  PropertyName (20)
 *      SimpleQualifiedName         |
 *      ExpressionQualifiedName
 *
 *  Input
 *      Identifier
 *      *
 *      internal, intrinsic, private, protected, public
 *      (
 *
 *  AST
 *      N_QNAME
 *          name
 *              namespace
 *              id
 *          left: N_EXPRESSIONS
 *          right: N_EXPRESSIONS
 */
static EcNode *parsePropertyName(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) == T_LPAREN) {
        np = parseExpressionQualifiedName(cp);
    } else {
        np = parseSimpleQualifiedName(cp);
    }

    return LEAVE(cp, np);
}


/*
 *  AttributeName (22)
 *      @ Brackets
 *      @ PropertyName
 *
 *  Input
 *      @ [ ... ]
 *      @ PropertyName
 *          @ *
 *          @ ID
 *          @ Qualifier :: *
 *          @ Qualifier :: ID
 *          @ Qualifier :: ReservedIdentifier
 *          @ Qualifier :: Brackets
 *          @ ( ListExpression ) :: *
 *          @ ( ListExpression ) :: ID
 *          @ ( ListExpression ) :: ReservedIdentifier
 *          @ ( ListExpression ) :: [ ... ]
 *
 *  AST
 *      N_QNAME
 *          name
 *              id
 *              namespace
 *              isAttribute
 *          left: N_EXPRESSIONS
 */
static EcNode *parseAttributeName(EcCompiler *cp)
{
    EcNode      *np;
    char        *attribute;

    ENTER(cp);

    if (getToken(cp) != T_AT) {
        return LEAVE(cp, expected(cp, "@ prefix"));
    }

    if (peekToken(cp) == T_LBRACKET) {
        np = createNode(cp, N_QNAME);
        np = appendNode(np, parseBrackets(cp));

    } else {
        np = parsePropertyName(cp);
    }

    if (np && np->kind == N_QNAME) {
        //  TODO - OPT. Better to allow lexer to keep @ in the id name and return T_AT with the entire attribute name.
        np->name.isAttribute = 1;
        mprAllocStrcat(np, &attribute, -1, NULL, "@", np->qname.name, NULL);
        mprFree((char*) np->qname.name);
        np->qname.name = attribute;
    }

    return LEAVE(cp, np);
}


/*
 *  QualifiedName (24)
 *      AttributeName
 *      PropertyName
 *
 *  Input
 *      @ ...
 *      Identifier
 *      *
 *      internal, intrinsic, private, protected, public
 *      (
 *
 *  AST
 *      N_QNAME
 *          name
 *              id
 *              namespace
 *              isAttribute
 *          left: listExpression
 *          right: bracketExpression
 */
static EcNode *parseQualifiedName(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) == T_AT) {
        np = parseAttributeName(cp);
    } else {
        np = parsePropertyName(cp);
    }

    return LEAVE(cp, np);
}


/*
 *  PrimaryName (26)
 *      Path . PropertyName
 *      PropertyName
 *
 *  Input
 *      *
 *      internal, intrinsic, private, protected, public
 *      Identifier
 *      (
 *
 *  AST
 *      N_QNAME
 *      N_DOT
 */

static EcNode *parsePrimaryName(EcCompiler *cp)
{
    EcNode      *np;
    int         tid, id;

    ENTER(cp);

    np = 0;
    tid = peekToken(cp);
    if (cp->peekToken->groupMask & G_CONREV) {
        tid = T_ID;
    }

    if (tid == T_ID && peekAheadToken(cp, 2) == T_DOT) {
        EcToken     *tok;
        tok = peekAheadTokenStruct(cp, 3);
        id = tok->tokenId;
        if (tok->groupMask & G_CONREV) {
            id = T_ID;
        }
        if (id == T_ID || id == T_MUL || id == T_RESERVED_NAMESPACE || id == T_LPAREN) {
            np = appendNode(createNode(cp, N_DOT), parsePath(cp, 0));
            getToken(cp);
        }
    }

    if (np) {
        np = appendNode(np, parsePropertyName(cp));
    } else {
        np = parsePropertyName(cp);
    }

    return LEAVE(cp, np);
}


/*
 *  Path (28)
 *      Identifier |
 *      Path . Identifier
 *
 *  Input
 *      ID
 *      ID. ... .ID
 *
 *  AST
 *      N_QNAME
 *      N_DOT
 *
 *  "dontConsumeLast" will be set if parsePath should not consume the last Identifier.
 */
static EcNode *parsePath(EcCompiler *cp, EcNode *lhs)
{
    EcNode      *np;
    EcToken     *tok;
    int         tid;

    ENTER(cp);

    if (lhs) {
        np = appendNode(createNode(cp, N_DOT), lhs);
        np = appendNode(np,  parseIdentifier(cp));
    } else {
        np = parseIdentifier(cp);
    }

    /*
     *  parsePath is called only from parsePrimaryName which requires that a ".PropertyName" be preserved.
     *  TODO - OPT. Perhaps hoist back into parsePrimaryName.
     */
    if (peekToken(cp) == T_DOT && peekAheadToken(cp, 2) == T_ID) {
        if (peekAheadToken(cp, 3) == T_DOT) {
            tok = peekAheadTokenStruct(cp, 4);
            tid = tok->tokenId;
            if (tok->groupMask & G_CONREV) {
                tid = T_ID;
            }
            if (tid == T_ID || tid == T_MUL || tid == T_RESERVED_NAMESPACE || tid == T_LPAREN) {
                getToken(cp);
                np = parsePath(cp, np);
            }
        }
    }

    return LEAVE(cp, np);
}


#if UNUSED
/*
 *  ParenExpression (30)
 *      ( AssignmentExpression )
 *      ( )                                 # EJS FIX
 *
 *  Input
 *      (
 *
 *  AST
 *      N_EXPR
 *      N_NOP
 */
static EcNode *parseParenExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, unexpected(cp));
    }

    if (peekToken(cp) != T_RPAREN) {
        np = parseAssignmentExpression(cp);
    } else {
        np = createNode(cp, N_NOP);
    }

    if (getToken(cp) != T_RPAREN) {
        np = expected(cp, ")");
    }

    return LEAVE(cp, np);
}
#endif


/*
 *  ParenListExpression (31)
 *      ( ListExpression )
 *
 *  Input
 *      (
 *
 *  AST
 *      N_EXPRESSIONS
 */
static EcNode *parseParenListExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, expected(cp, "("));
    }

    np = parseListExpression(cp);

    if (getToken(cp) != T_RPAREN) {
        np = expected(cp, ")");
    }

    return LEAVE(cp, np);
}


/*
 *  FunctionExpression (32)
 *      function Identifier FunctionSignature FunctionBody
 *      function FunctionSignature FunctionBody
 *
 *  Input
 *      function id ( args ) { body }
 *      function ( args ) { body }
 *
 *  AST
 *      N_FUNCTION
 */
static EcNode *parseFunctionExpression(EcCompiler *cp)
{
    EcState     *state;
    EcNode      *np, *funRef;

    ENTER(cp);

    state = cp->state;

    if (getToken(cp) != T_FUNCTION) {
        return LEAVE(cp, unexpected(cp));
    }

    np = createNode(cp, N_FUNCTION);

    if (peekToken(cp) == T_ID) {
        getToken(cp);
        //  TODO - what should happen with this name. Seems to be ignored in all cases.
        np->qname.name = mprStrdup(np, (char*) cp->token->text);
    }
    if (np->qname.name == 0) {
        /*
         *  Create a property name for the function expression.
         */
        mprAllocSprintf(np, (char**) &np->qname.name, -1, "--fun_%d--", np->seqno);
    }

    //  TODO this doesn't seem right using the default internal namespace
    np->qname.space = mprStrdup(np, state->inFunction ? EJS_PRIVATE_NAMESPACE: cp->fileState->namespace);

    np = parseFunctionSignature(cp, np);
    if (np) {
        if (STRICT_MODE(cp)) {
            if (np->function.resultType == 0) {
                return LEAVE(cp, parseError(cp,
                    "Function has not defined a return type. Fuctions must be typed when declared in strict mode"));
            }
        }
    }

    cp->state->currentFunctionNode = np;
    np->function.body = linkNode(np, parseFunctionExpressionBody(cp));
    mprStealBlock(np, np->function.body);

    if (np->function.body == 0) {
        return LEAVE(cp, 0);
    }

    /*
     *  The function must get linked into the top var block. It must not get processed inline at this point in the AST tree.
     */
    mprAssert(cp->state->topVarBlockNode);
    appendNode(cp->state->topVarBlockNode, np);

    /*
     *  Create a name node to reference the function. This is the value of this function expression.
     *  The funRef->name will be filled in by the AST processing for the function node.
     */
    funRef = createNode(cp, N_QNAME);
    funRef->qname = ejsCopyName(funRef, &np->qname);

    return LEAVE(cp, funRef);
}


/*
 *  FunctionExpressionBody (34)
 *      Block
 *      AssignmentExpression
 *
 *  Input
 *      {
 *      TODO TBD
 *
 *  AST
 *      TODO TBD
 */
static EcNode *parseFunctionExpressionBody(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) == T_LBRACE) {
        /* TODO - is this the right kind */
        np = parseBlock(cp);
        if (np) {
            np = np->left;
        }

    } else {
        np = parseAssignmentExpression(cp);
    }
    if (np) {
        mprAssert(np->kind == N_DIRECTIVES);
    }

    np = appendNode(np, createNode(cp, N_END_FUNCTION));

    return LEAVE(cp, np);
}


/*
 *  ObjectLiteral (36)
 *      { FieldList }
 *      { FieldList } : NullableTypeExpression
 *
 *  Input
 *      { LiteralField , ... }
 *
 *  AST
 *      N_EXPRESSIONS
 */
static EcNode *parseObjectLiteral(EcCompiler *cp)
{
    EcNode  *typeNode, *np;

    ENTER(cp);

    np = createNode(cp, N_OBJECT_LITERAL);

    if (getToken(cp) != T_LBRACE) {
        return LEAVE(cp, unexpected(cp));
    }

    np = parseFieldList(cp, np);
    if (np == 0) {
        return LEAVE(cp, 0);
    }

    if (peekToken(cp) == T_COLON) {
        getToken(cp);
        typeNode = parseNullableTypeExpression(cp);

    } else {
        /*
         *  Defaults to Object type
         */
        typeNode = createNode(cp, N_QNAME);
        setId(typeNode, (char*) cp->ejs->objectType->qname.name);
    }
    np->objectLiteral.typeNode = linkNode(np, typeNode);

    if (getToken(cp) != T_RBRACE) {
        return LEAVE(cp, unexpected(cp));
    }

    return LEAVE(cp, np);
}


/*
 *  FieldList (41)
 *      EMPTY
 *      LiteralField
 *      LiteralField , LiteralField
 *
 *  Input
 *      LiteralField , ...
 *
 *  AST
 */
static EcNode *parseFieldList(EcCompiler *cp, EcNode *np)
{
    EcNode      *elt;

    ENTER(cp);

    while (peekToken(cp) != T_RBRACE) {
        elt = parseLiteralField(cp);
        if (elt) {
            np = appendNode(np, elt);
        }
        if (peekToken(cp) != T_COMMA) {
            break;
        }
        getToken(cp);
    }
    return LEAVE(cp, np);
}


/*
 *  LiteralField (42)
 *      FieldKind FieldName : AssignmentExpression
 *      get Identifier FunctionSignature FunctionBody
 *      set Identifier FunctionSignature FunctionBody
 *
 *  Input
 *      TODO
 *
 *  AST
 */
static EcNode *parseLiteralField(EcCompiler *cp)
{
    EcNode  *fp, *np, *id, *funRef;

    ENTER(cp);

    np = 0;

    if (peekToken(cp) == T_GET || cp->peekToken->tokenId == T_SET) {
        fp = createNode(cp, N_FUNCTION);
        if (getToken(cp) == T_GET) {
            fp->function.getter = 1;
            fp->attributes |= EJS_ATTR_GETTER | EJS_ATTR_LITERAL_GETTER;
        } else {
            fp->function.setter = 1;
            fp->attributes |= EJS_ATTR_SETTER;
        }
        id = parseIdentifier(cp);
        if (id == 0) {
            //  TODO - try to recover and eat rest of this production somehow
            return LEAVE(cp, 0);
        }
        if (fp->function.getter) {
            fp->qname.name = mprStrdup(fp, id->qname.name);
        } else {
            mprAllocSprintf(fp, (char**) &fp->qname.name, -1, "set-%s", id->qname.name);
        }
        fp->qname.space = mprStrdup(fp, "");

        cp->state->currentFunctionNode = fp;

        fp = parseFunctionSignature(cp, fp);
        fp->function.body = linkNode(fp, parseFunctionBody(cp, fp));
        if (fp->function.body == 0) {
            return LEAVE(cp, 0);
        }

        /*
         *  The function must get linked into the current var block. It must not get processed inline at
         *  this point in the AST tree.
         */
        mprAssert(cp->state->topVarBlockNode);
        appendNode(cp->state->topVarBlockNode, fp);

        np = createNode(cp, N_FIELD);
        np->field.fieldKind = FIELD_KIND_FUNCTION;

        funRef = createNode(cp, N_QNAME);
        funRef->qname = ejsCopyName(funRef, &fp->qname);
        np->field.fieldName = linkNode(np, funRef);

    } else {
        if (getToken(cp) == T_CONST) {
            np = createNode(cp, N_FIELD);
            np->field.varKind = KIND_CONST;
        } else {
            putToken(cp);
            np = createNode(cp, N_FIELD);
        }
        np->field.fieldKind = FIELD_KIND_VALUE;
        np->field.fieldName = linkNode(np, parseFieldName(cp));
        if (getToken(cp) != T_COLON) {
            return LEAVE(cp, expected(cp, ":"));
        }
        np->field.expr = linkNode(np, parseAssignmentExpression(cp));
    }

    return LEAVE(cp, np);
}


#if ROLLED_UP
/*
 *  FieldKind (45)
 *      EMPTY
 *      const
 *
 *  Input
 *      TODO
 *
 *  AST
 */
static EcNode *parseFieldKind(EcCompiler *cp, EcNode *np)
{
    EcNode  *np;

    ENTER(cp);

    if (peekToken(cp) == T_CONST) {
        getToken(cp);
        np->def.varKind = KIND_CONST;
    }
    return LEAVE(cp, np);
}
#endif


/*
 *  FieldName (47)
 *      PropertyName
 *      StringLiteral
 *      NumberLiteral
 *      ReservedIdentifier
 *
 *  Input
 *      TODO
 *
 *  AST
 */
static EcNode *parseFieldName(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_ID:
    case T_NUMBER:
    case T_STRING:
        np = parsePrimaryExpression(cp);
        break;

    default:
        np = parsePropertyName(cp);
        break;
    }
    return LEAVE(cp, np);
}


/*
 *  ArrayLiteral (51)
 *      [ Elements ]
 *
 *  ArrayLiteral (52)
 *      [ Elements ] : NullableTypeExpression
 *
 *  Input sequence
 *      [
 *
 *  AST
 *      N_EXPRESSIONS
 *          N_NEW
 *              N_QNAME
 *          N_EXPRESSIONS
 *              N_ASSIGN_OP
 *                  N_DOT
 *                      N_PARENT
 *                      N_LITERAL
 *                  ANY
 */
static EcNode *parseArrayLiteral(EcCompiler *cp)
{
    EjsType     *type;
    EcNode      *np, *typeNode, *elementsNode, *newNode, *lit;

    ENTER(cp);

    typeNode = 0;
    newNode = createNode(cp, N_NEW);

    if (getToken(cp) != T_LBRACKET) {
        np = parseError(cp, "Expecting \"[\"");

    } else {
        np = parseElements(cp, newNode);
        if (getToken(cp) != T_RBRACKET) {
            np = parseError(cp, "Expecting \"[\"");

        } else {
            if (peekToken(cp) == T_COLON) {
                typeNode = parseArrayType(cp);
                if (typeNode == 0) {
                    return LEAVE(cp, 0);
                }
            }
        }

        if (np) {
            elementsNode = np;

            if (typeNode == 0) {
                /*
                 *  Defaults to Array type
                 */
                type = (EjsType*) cp->ejs->arrayType;
                mprAssert(type);
                typeNode = createNode(cp, N_QNAME);
                mprAssert(typeNode);
                setId(typeNode, (char*) type->qname.name);
            }

            //  TODO - should we set the array size?
            //  TBD - Update the array size if > 8 elts
            newNode = appendNode(newNode, typeNode);

            np = createNode(cp, N_EXPRESSIONS);
            np = appendNode(np, newNode);
            np = appendNode(np, elementsNode);
        }
    }
    lit = createNode(cp, N_ARRAY_LITERAL);
    np = appendNode(lit, np);
    return LEAVE(cp, np);
}


/*
 *  Elements (54)
 *      ElementList
 *      ElementComprehension
 *
 *  Input sequence
 *
 *
 *  AST
 *      N_EXPRESSIONS
 *          N_ASSIGN_OP
 *              N_DOT
 *                  N_REF
 *                  N_LITERAL
 *              ANY
 */
static EcNode *parseElements(EcCompiler *cp, EcNode *newNode)
{
    EcNode      *np;

    ENTER(cp);

    np = parseElementList(cp, newNode);

#if FUTURE
    if (peekToken(cp) == T_FOR) {
        np = parseElementComprehension(cp, np);
    }
#endif

    return LEAVE(cp, np);
}


/*
 *  ElementList (56)
 *      EMPTY
 *      LiteralElement
 *      , ElementList
 *      LiteralElement , ElementList
 *
 *  Input sequence
 *
 *  AST
 *      N_EXPRESSIONS
 *          N_ASSIGN_OP
 *              N_DOT
 *                  N_REF
 *                  N_LITERAL
 *              ANY
 */
static EcNode *parseElementList(EcCompiler *cp, EcNode *newNode)
{
    EcNode      *np, *valueNode, *left;
    int         index;

    ENTER(cp);

    np = createNode(cp, N_EXPRESSIONS);
    index = 0;

    do {
        /*
         *  Leading comma, or dual commas means a gap in the indicies
         */
        if (peekToken(cp) == T_COMMA) {
            getToken(cp);
            index++;
            continue;

        } else if (cp->peekToken->tokenId == T_RBRACKET) {
            break;
        }

        valueNode = parseLiteralElement(cp);
        if (valueNode) {
            /*
             *  Update the array index
             */
            mprAssert(valueNode->kind == N_ASSIGN_OP);

            left = valueNode->left;
            mprAssert(left->kind == N_DOT);

            /*
             *  Set the array index
             */
            mprAssert(left->right->kind == N_LITERAL);
            left->right->literal.var = (EjsVar*) ejsCreateNumber(cp->ejs, index);

            /*
             *  Update the reference (array) node reference. This refers to the actual array object.
             */
            mprAssert(left->left->kind == N_REF);
            left->left->ref.node = newNode;
            np = appendNode(np, valueNode);

        } else {
            np = 0;
        }

    } while (np);

    //  TODO - should set the array size to "index"

    return LEAVE(cp, np);
}


/*
 *  LiteralElement (60)
 *      AssignmentExpression -noList, allowin-
 *
 *  Input sequence
 *
 *  AST
 *      N_ASSIGN_OP
 *          N_DOT
 *              N_REF
 *              N_LITERAL (empty - caller must set node->var)
 *          ANY
 */
static EcNode *parseLiteralElement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_DOT);

    //  TODO - what are we using a N_REF here for?
    np = appendNode(np, createNode(cp, N_REF));
    np = appendNode(np, createNode(cp, N_LITERAL));

    np = createAssignNode(cp, np, parseAssignmentExpression(cp));
    if (np == 0) {
        return LEAVE(cp, np);
    }

    /*
     *  To allow multiple literal elements, we must not consume the object on the stack.
     */
    mprAssert(np->kind == N_ASSIGN_OP);

    //  TOD - bad name. Only used by literals
    np->needDupObj = 1;

    return LEAVE(cp, np);
}


#if UNUSED
/*
 *  ElementComprehension (61)
 *      LiteralElement ForExpression OptionalIfExpression
 *
 *  parseElements will parse the LiteralElement, so refactored to be:
 *
 *  ElementComprehension
 *      ForExpression IfExpression
 *
 *  Input sequence
 *      for
 *
 *  AST
 */
static EcNode *parseElementComprehension(EcCompiler *cp, EcNode *literalElement)
{
    EcNode      *np;

    ENTER(cp);

#if FUTURE
    np = parseForExpression(cp);
    np = parseIfExpression(cp);
#endif

    return LEAVE(cp, np);
}


/*
 *  ForInExpressionList (62)
 *      ForExpression
 *      ForExpressionList ForExpression
 *
 *  Input sequence
 *
 *  AST
 */
static EcNode *parseForInExpressionList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}


/*
 *  ForInExpression (64)
 *      for ( ForInBinding in ListExpression -allowin- )
 *      for each ( ForInBinding in ListExpression -allowin- )
 *      ForExpressionList ForExpression
 *
 *  Input sequence
 *
 *  AST
 */
static EcNode *parseForInExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif


/*
 *  OptionalIfCondition (66)
 *      EMPTY
 *      if ParenListExpression
 *
 *  Input
 *      this
 *
 *  AST
 */


/*
 *  XMLInitializer (68)
 *      XMLMarkup
 *      XMLElement
 *      < > XMLElementContent </ >
 *
 *  Input
 *      <!--
 *      [CDATA
 *      <?
 *      <
 *
 *  AST
 *      Add data to literal.data buffer
 *
 */
struct EcNode *parseXMLInitializer(EcCompiler *cp)
{
#if BLD_FEATURE_EJS_E4X
    EcNode      *np;
    EjsVar      *vp;

    ENTER(cp);

    np = createNode(cp, N_LITERAL);
    np->literal.data = mprCreateBuf(np, 0, 0);

    vp = (EjsVar*) ejsCreateXML(cp->ejs, 0, 0, 0, 0);
    np->literal.var = vp;
    mprStealBlock(np, vp);

    switch (peekToken(cp)) {
    case T_XML_COMMENT_START:
    case T_CDATA_START:
    case T_XML_PI_START:
        return parseXMLMarkup(cp, np);

    case T_LT:
        if (peekAheadToken(cp, 2) == T_GT) {
            getToken(cp);
            getToken(cp);
            mprPutStringToBuf(np->literal.data, "<>");
            np = parseXMLElementContent(cp, np);
            if (getToken(cp) != T_LT_SLASH) {
                return LEAVE(cp, expected(cp, "</"));
            }
            if (getToken(cp) != T_GT) {
                return LEAVE(cp, expected(cp, ">"));
            }
            mprPutStringToBuf(np->literal.data, "</>");

        } else {
            return parseXMLElement(cp, np);
        }
        break;

    default:
        //  TODO - should really ensure the token is eaten before returning to prevent a swamp of errors
        return LEAVE(cp, unexpected(cp));
    }
    np = 0;
    return LEAVE(cp, np);
#else
    return parseError(cp, "E4X and XML support is not configured");
#endif
}


/*
 *  XMLElementContent (71)
 *      { ListExpression } XMLElementContent
 *      XMLMarkup XMLElementContent
 *      XMLText XMLElementContent
 *      XMLElement XMLElementContent
 *      EMPTY
 *  Input
 *      {
 *      <!--
 *      [CDATA
 *      <?
 *      <
 *      Text
 *  AST
 */
struct EcNode *parseXMLElementContent(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (np == 0) {
        return LEAVE(cp, np);
    }
    
    switch (peekToken(cp)) {
    case T_LBRACE:
        getToken(cp);
        //  TODO - must handle embedded list expressions
        np = parseListExpression(cp);
        if (getToken(cp) != T_RBRACE) {
            return LEAVE(cp, expected(cp, "}"));
        }
        //  TODO - must append or something ?
        np = parseXMLElementContent(cp, np);
        break;

    case T_XML_COMMENT_START:
    case T_CDATA_START:
    case T_XML_PI_START:
        np = parseXMLMarkup(cp, np);
        break;

    case T_LT:
        np = parseXMLElement(cp, np);
        np = parseXMLElementContent(cp, np);
        break;

    case T_LT_SLASH:
        break;

    case T_EOF:
    case T_ERR:
        return LEAVE(cp, 0);

    default:
        /* TODO _ should only call XMLText if there are no embedded { or <. We should split text at these tokens */
        np = parseXMLText(cp, np);
        //  TODO must append or something?
        np = parseXMLElementContent(cp, np);
    }
    return LEAVE(cp, np);
}


/*
 *  XMLElement (76)
 *      < XMLTagContent XMLWhitespace />
 *      < XMLTagContent XMLWhitespace > XMLElementContent </ XMLTagName XMLWhitespace >
 *
 *  Input
 *      <
 *
 *  AST
 *      Add data to literal.data buffer
 *
 */
struct EcNode *parseXMLElement(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (getToken(cp) != T_LT) {
        return LEAVE(cp, expected(cp, "<"));
    }
    addTokenToBuf(cp, np);

    np = parseXMLTagContent(cp, np);
    if (np == 0) {
        return LEAVE(cp, np);
    }

    if (getToken(cp) == T_SLASH_GT) {
        addTokenToBuf(cp, np);
        return LEAVE(cp, np);

    } else if (cp->token->tokenId != T_GT) {
        return LEAVE(cp, unexpected(cp));
    }

    addTokenToBuf(cp, np);

    np = parseXMLElementContent(cp, np);
    if (getToken(cp) != T_LT_SLASH) {
        return LEAVE(cp, expected(cp, "</"));
    }
    addTokenToBuf(cp, np);

    np = parseXMLTagName(cp, np);
    if (getToken(cp) != T_GT) {
        return LEAVE(cp, expected(cp, ">"));
    }
    addTokenToBuf(cp, np);

    return LEAVE(cp, np);
}


/*
 *  XMLTagContent (79)
 *      XMLTagName XMLAttributes
 *
 *  Input
 *      {
 *      UnicodeLetter
 *      _       underscore
 *      :       colon
 *
 *  AST
 *      Add data to literal.data buffer
 *
 */
struct EcNode *parseXMLTagContent(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    np = parseXMLTagName(cp, np);
    if (np) {
        np = parseXMLAttributes(cp, np);
    }
    return LEAVE(cp, np);
}


/*
 *  XMLTagName (80)
 *      { ListExpression }
 *      XMLName
 *
 *  Input
 *      {
 *      UnicodeLetter
 *      _       underscore
 *      :       colon
 *
 *  AST
 *      Add data to literal.data buffer
 */
struct EcNode *parseXMLTagName(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (np == 0) {
        return LEAVE(cp, np);
    }
    
    if (peekToken(cp) == T_LBRACE) {
        getToken(cp);
        //  TODO - what about embedded expressions?
        np = parseListExpression(cp);
        if (getToken(cp) != T_RBRACE) {
            return LEAVE(cp, expected(cp, "}"));
        }
    } else {
        np = parseXMLName(cp, np);
    }
    return LEAVE(cp, np);
}


/*
 *  XMLAttributes (82)
 *      XMLWhitespace { ListExpression }
 *      XMLAttribute XMLAttributes
 *      EMPTY
 *  Input
 *
 *  AST
 *      Add data to literal.data buffer
 */
struct EcNode *parseXMLAttributes(EcCompiler *cp, EcNode *np)
{
    int         tid;

    ENTER(cp);

    tid = peekToken(cp);
    if (tid == T_LBRACE) {
        /* TODO BUG - not yet supported */
        parseListExpression(cp);
        if (peekToken(cp) == T_RBRACE) {
            return LEAVE(cp, expected(cp, "}"));
        }

    } else while (tid == T_COLON || tid == T_ID) {
        np = parseXMLAttribute(cp, np);
        tid = peekToken(cp);
    }
    return LEAVE(cp, np);
}

/*
 *  XMLAttributes (85)
 *      XMLWhitespace XMLName XMLWhitespace = XMLWhitepace { ListExpression (allowIn) }
 *      XMLWhitespace XMLName XMLWhitespace = XMLAttributeValue
 *
 *  Input
 *      UnicodeLetter
 *      _       underscore
 *      :       colon
 *
 *  AST
 *      Add data to literal.data buffer
 *
 */
struct EcNode *parseXMLAttribute(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    mprPutCharToBuf(np->literal.data, ' ');
    np = parseXMLName(cp, np);

    if (getToken(cp) != T_ASSIGN) {
        return LEAVE(cp, expected(cp, "="));
    }
    mprPutCharToBuf(np->literal.data, '=');

    if (peekToken(cp) == T_LBRACE) {
        //  TODO - must handle embedded expressions
        np = parseListExpression(cp);
        if (getToken(cp) != T_RBRACE) {
            return LEAVE(cp, expected(cp, "}"));
        }
    } else {
        np = parseXMLAttributeValue(cp, np);
    }

    return LEAVE(cp, np);
}


/*
 *  ThisExpression (87)
 *      this
 *      this callee
 *      this generator
 *
 *  Input
 *      this
 *
 *  AST
 */
static EcNode *parseThisExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_THIS) {
        return LEAVE(cp, unexpected(cp));
    }

    np = createNode(cp, N_THIS);

    switch (peekToken(cp)) {
    case T_TYPE:
        getToken(cp);
        np->thisNode.thisKind = N_THIS_TYPE;
        break;

    case T_FUNCTION:
        getToken(cp);
        np->thisNode.thisKind = N_THIS_FUNCTION;
        break;

    case T_CALLEE:
        getToken(cp);
        np->thisNode.thisKind = N_THIS_CALLEE;
        break;

    case T_GENERATOR:
        getToken(cp);
        np->thisNode.thisKind = N_THIS_GENERATOR;
        break;
    }

    return LEAVE(cp, np);
}



/*
 *  PrimaryExpression (90)
 *      null
 *      true
 *      false
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      ThisExpression
 *      XMLInitializer
 *      ParenListExpression
 *      ArrayLiteral
 *      ObjectLiteral
 *      FunctionExpression
 *      AttributeName
 *      PrimaryName
 *
 *  Input sequence
 *      null
 *      true
 *      false
 *      this
 *      function
 *      Identifier
 *      ContextuallyReservedIdentifier
 *      {
 *      [
 *      (
 *      @
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *
 *
 *  AST
 *      N_FUNCTION
 *      N_NEW           (array / object literals)
 *      N_LITERAL
 *      N_QNAME
 *      N_THIS
 *
 */
static EcNode *parsePrimaryExpression(EcCompiler *cp)
{
    EcNode      *np;
    EjsVar      *vp;
    int         tid;

    ENTER(cp);

    tid = peekToken(cp);
    if (cp->peekToken->groupMask & G_CONREV) {
        tid = T_ID;
    }

    np = 0;

     switch (tid) {

    case T_STRING:
        if (peekAheadToken(cp, 2) == T_COLON_COLON) {
            np = parsePrimaryName(cp);
        } else {
            //  TODO - put this code into a function
            getToken(cp);
            vp = (EjsVar*) ejsCreateString(cp->ejs, (char*) cp->token->text);
            np = createNode(cp, N_LITERAL);
            np->literal.var = vp;
            mprStealBlock(np, vp);
        }
        break;

    case T_ID:
        np = parsePrimaryName(cp);
        break;

    case T_AT:
        np = appendNode(np, parseAttributeName(cp));
        break;

    case T_NUMBER:
        //  TODO - OPT lots of replicated code here
        getToken(cp);
        vp = ejsParseVar(cp->ejs, (char*) cp->token->text, -1);
        np = createNode(cp, N_LITERAL);
        np->literal.var = vp;
        break;

    case T_NULL:
        getToken(cp);
        /*
         *  TODO This testing of cp->empty may not be required when contextually reserved identifiers is suupported.
         */
        if (cp->empty) {
            np = createNode(cp, N_QNAME);
            setId(np, (char*) cp->token->text);
        } else {
            np = createNode(cp, N_LITERAL);
            np->literal.var = cp->ejs->nullValue;
        }
        break;

    case T_UNDEFINED:
        getToken(cp);
        if (cp->empty) {
            np = createNode(cp, N_QNAME);
            setId(np, (char*) cp->token->text);
        } else {
            np = createNode(cp, N_LITERAL);
            np->literal.var = cp->ejs->undefinedValue;
        }
        break;

    case T_TRUE:
        getToken(cp);
        if (cp->empty) {
            np = createNode(cp, N_QNAME);
            setId(np, (char*) cp->token->text);
        } else {
            np = createNode(cp, N_LITERAL);
            vp = (EjsVar*) ejsCreateBoolean(cp->ejs, 1);
            np->literal.var = vp;
        }
        break;

    case T_FALSE:
        getToken(cp);
        if (cp->empty) {
            np = createNode(cp, N_QNAME);
            setId(np, (char*) cp->token->text);
        } else {
            np = createNode(cp, N_LITERAL);
            vp = (EjsVar*) ejsCreateBoolean(cp->ejs, 0);
            np->literal.var = vp;
        }
        break;

    case T_THIS:
        np = parseThisExpression(cp);
        break;

    case T_LPAREN:
        np = parseParenListExpression(cp);
        break;

    case T_LBRACKET:
        np = parseArrayLiteral(cp);
        break;

    case T_LBRACE:
        np = parseObjectLiteral(cp);
        break;

    case T_FUNCTION:
        np = parseFunctionExpression(cp);
        break;

    case T_VOID:
    case T_NAMESPACE:
    case T_TYPEOF:
        getToken(cp);
        if (cp->empty) {
            np = createNode(cp, N_QNAME);
            setId(np, (char*) cp->token->text);
        } else {
            np = unexpected(cp);
        }
        break;

    case T_LT:
    case T_XML_COMMENT_START:
    case T_CDATA_START:
    case T_XML_PI_START:
        np = parseXMLInitializer(cp);
        break;

    case T_DIV:
        np = parseRegularExpression(cp);
        break;

    case T_ERR:
    default:
        getToken(cp);
        np = unexpected(cp);
        break;
    }

    return LEAVE(cp, np);
}



static EcNode *parseRegularExpression(EcCompiler *cp)
{
    EcNode      *np;
    EjsVar      *vp;
    int         id;

    ENTER(cp);

    /*
     *  Flush peek ahead buffer
     */
    while (cp->input->putBack) {
        getToken(cp);
    }

    id = ecGetRegExpToken(cp->input);
    updateTokenInfo(cp);
    cp->peekToken = 0;
#if BLD_DEBUG
    cp->peekTokenName = 0;
#endif

    if (id != T_REGEXP) {
        return LEAVE(cp, parseError(cp, "Can't parse regular expression"));
    }

#if BLD_FEATURE_REGEXP
    vp = (EjsVar*) ejsCreateRegExp(cp->ejs, (char*) cp->token->text);
    if (vp == 0) {
        return LEAVE(cp, parseError(cp, "Can't compile regular expression"));
    }
#else
    vp = (EjsVar*) cp->ejs->undefinedValue;
#endif

    np = createNode(cp, N_LITERAL);
    np->literal.var = vp;
    mprStealBlock(np, vp);

    return LEAVE(cp, np);
}



/*
 *  SuperExpression (104)
 *      super
 *      super ParenExpression
 *
 *  Input
 *      super
 *
 *  AST
 *      N_SUPER
 *
 *  NOTES / TODO:
 *      Using Arguments instead of ParenExpression so we can have multiple args.
 */
static EcNode *parseSuperExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_SUPER) {
        np = unexpected(cp);

    } else {
        if (peekToken(cp) == T_LPAREN) {
            np = createNode(cp, N_SUPER);
//          np = appendNode(np, parseParenExpression(cp));
            np = appendNode(np, parseArguments(cp));

        } else {
            //  TODO - not yet implemented
            np = createNode(cp, N_SUPER);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  Arguments (106)
 *      ( )
 *      ( ArgumentList )
 *
 *  Input
 *      (
 *
 *  AST
 *      N_ARGS
 */
static EcNode *parseArguments(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_LPAREN) {
        np = parseError(cp, "Expecting \"(\"");

    } else if (peekToken(cp) == T_RPAREN) {
        getToken(cp);
        np = createNode(cp, N_ARGS);

    } else {
        np = parseArgumentList(cp);
        if (np && getToken(cp) != T_RPAREN) {
            np = parseError(cp, "Expecting \")\"");
        }
    }

    return LEAVE(cp, np);
}



/*
 *  ArgumentList (118)
 *      AssignmentExpression
 *      ArgumentList , AssignmentExpression
 *
 *  Input
 *
 *  AST N_ARGS
 *      children: arguments
 */
static EcNode *parseArgumentList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_ARGS);
    np = appendNode(np, parseAssignmentExpression(cp));

    while (np && peekToken(cp) == T_COMMA) {
        getToken(cp);
        np = appendNode(np, parseAssignmentExpression(cp));
    }

    return LEAVE(cp, np);
}



/*
 *  PropertyOperator (110)
 *      . ReservedIdentifier
 *      . PropertyName
 *      . AttributeName
 *      .. QualifiedName
 *      . ParenListExpression
 *      . ParenListExpression :: QualifiedNameIdentifier
 *      Brackets
 *      TypeApplication
 *
 *  Input
 *      .
 *      ..
 *      [
 *
 *  AST
 *      N_DOT
 */
static EcNode *parsePropertyOperator(EcCompiler *cp)
{
    EcNode      *np, *name;
    char        *id;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_DOT:
        np = createNode(cp, N_DOT);
        getToken(cp);
        switch (peekToken(cp)) {
        case T_LPAREN:
            np = appendNode(np, parseParenListExpression(cp));
            break;

        /* TODO - should handle all contextually reserved identifiers here */
        case T_TYPE:
        case T_ID:
            if (cp->token->groupMask & G_RESERVED) {
                np = appendNode(np, parseIdentifier(cp));
            } else {
                np = appendNode(np, parsePropertyName(cp));
            }
            break;

        case T_AT:
            np = appendNode(np, parseAttributeName(cp));
            break;

        case T_SUPER:
            getToken(cp);
            np = appendNode(np, createNode(cp, N_SUPER));
            break;

        default:
            getToken(cp);
            np = unexpected(cp);
            break;
        }
        break;

    case T_LBRACKET:
        np = createNode(cp, N_DOT);
        np = appendNode(np, parseBrackets(cp));
        break;

    case T_DOT_DOT:
        getToken(cp);
        np = createNode(cp, N_DOT);
        name = parseQualifiedName(cp);
        if (name && name->kind == N_QNAME) {
            //  TODO OPT - work out a way for the lexer to keep "." in the name
            mprAllocStrcat(name, &id, -1, NULL, ".", name->qname.name, NULL);
            mprFree((char*) name->qname.name);
            name->qname.name = id;
        }
        np = appendNode(np, name);
        break;

    default:
        getToken(cp);
        np = parseError(cp, "Expecting property operator . .. or [");
        break;
    }
    return LEAVE(cp, np);
}



/*
 *  Brackets (125)
 *      [ ListExpression ]
 *      [ SliceExpression ]
 *
 *  Input
 *      [
 *
 *  AST
 *      N_EXPRESSIONS
 */
static EcNode *parseBrackets(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    //  TODO - not implementing SliceExpression
    if (getToken(cp) != T_LBRACKET) {
        np = parseError(cp, "Expecting \"[\"");
    }

    if (peekToken(cp) == T_COLON) {
        /*
         *  Slice expression
         */
        /* First optional expression in a slice expression is empty */
        np = parseOptionalExpression(cp);
        if (getToken(cp) != T_COLON) {
            np = parseError(cp, "Expecting \":\"");
        }
        np = appendNode(np, parseOptionalExpression(cp));

    } else {

        np = parseListExpression(cp);

        if (peekToken(cp) == T_COLON) {
            /*
             *  Slice expression
             */
            np = appendNode(np, parseOptionalExpression(cp));
            if (peekToken(cp) != T_COLON) {
                getToken(cp);
                np = parseError(cp, "Expecting \":\"");
            }
            np = appendNode(np, parseOptionalExpression(cp));
        }
    }

    if (getToken(cp) != T_RBRACKET) {
        np = parseError(cp, "Expecting \"[\"");
    }
    return LEAVE(cp, np);
}



/*
 *  TypeApplication (120)
 *      .< TypeExpressionList >
 *
 *  Input
 *      .<
 *
 *  AST
 */


#if FUTURE
/*
 *  SliceExpression (121)
 *      OptionalExpression : OptionalExpression
 *      OptionalExpression : OptionalExpression : OptionalExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseSliceExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



/*
 *  OptionalExpression (123)
 *      ListExpression -allowin-
 *      EMPTY
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseOptionalExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = parseListExpression(cp);
    return LEAVE(cp, np);
}



/*
 *  MemberExpression (125) -a,b-
 *      PrimaryExpression -a,b-
 *      new MemberExpression Arguments
 *      SuperExpression PropertyOperator
 *      MemberExpression PropertyOperator
 *
 *  Input
 *      null
 *      true
 *      false
 *      this
 *      function
 *      Identifier
 *      {
 *      [
 *      (
 *      @
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *      super
 *      new
 *
 *  AST
 *      N_FUNCTION
 *      N_DOT
 *          left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
 *          right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
 *      N_LITERAL
 *      N_NEW           (array / object literals)
 *      N_QNAME
 *      N_SUPER
 *      N_THIS
 */
static EcNode *parseMemberExpression(EcCompiler *cp)
{
    EcNode      *np, *newNode;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_SUPER:
        np = parseSuperExpression(cp);
        if (peekToken(cp) == T_DOT || cp->peekToken->tokenId == T_DOT_DOT ||
                cp->peekToken->tokenId == T_LBRACKET) {
            np = insertNode(parsePropertyOperator(cp), np, 0);
        }
        break;

    case T_NEW:
        getToken(cp);
        newNode = createNode(cp, N_NEW);
        newNode = appendNode(newNode, parseMemberExpression(cp));
        np = createNode(cp, N_NEW);
        np = appendNode(np, newNode);
        np = appendNode(np, parseArguments(cp));
        break;

    default:
        np = parsePrimaryExpression(cp);
        break;
    }

    while (np && (peekToken(cp) == T_DOT || cp->peekToken->tokenId == T_DOT_DOT ||
            cp->peekToken->tokenId == T_LBRACKET)) {
        if (np->lineNumber == cp->peekToken->lineNumber) {
            np = insertNode(parsePropertyOperator(cp), np, 0);
        } else {
            break;
        }
    }

    return LEAVE(cp, np);
}



/*
 *  CallExpression (129) -a,b-
 *      MemberExpression Arguments
 *      CallExpression Arguments
 *      CallExpression PropertyOperator
 *
 *  Input
 *
 *  AST
 *      N_CALL
 *
 *  "me" is to to an already parsed member expression
 */
static EcNode *parseCallExpression(EcCompiler *cp, EcNode *me)
{
    EcNode      *np;

    ENTER(cp);

    np = 0;

    while (1) {
        peekToken(cp);
        if (me && me->lineNumber != cp->peekToken->lineNumber) {
            return LEAVE(cp, np);
        }
        switch (peekToken(cp)) {
        case T_LPAREN:
            np = createNode(cp, N_CALL);
            np = appendNode(np, me);
            np = appendNode(np, parseArguments(cp));
            break;

        case T_DOT:
        case T_DOT_DOT:
        case T_LBRACKET:
            if (me == 0) {
                getToken(cp);
                return LEAVE(cp, unexpected(cp));
            }
            np = insertNode(parsePropertyOperator(cp), me, 0);
            break;

        default:
            if (np == 0) {
                getToken(cp);
                return LEAVE(cp, unexpected(cp));
            }
            return LEAVE(cp, np);
        }
        if (np == 0) {
            return LEAVE(cp, np);
        }
        me = np;
    }

    return LEAVE(cp, np);
}



/*
 *  NewExpression (132)
 *      MemberExpression
 *      new NewExpression
 *
 *  Input
 *      null
 *      true
 *      false
 *      this
 *      function
 *      Identifier
 *      {
 *      [
 *      (
 *      @
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *      super
 *      new
 *
 *  AST
 *      N_FUNCTION
 *      N_DOT
 *          left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
 *          right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
 *      N_LITERAL
 *      N_NEW           (array / object literals)
 *      N_QNAME
 *      N_SUPER
 *      N_THIS
 */
static EcNode *parseNewExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) == T_NEW) {
        getToken(cp);
        np = createNode(cp, N_NEW);
        np = appendNode(np, parseNewExpression(cp));

    } else {
        np = parseMemberExpression(cp);
    }
    return LEAVE(cp, np);
}



/*
 *  LeftHandSideExpression (134) -a,b-
 *      NewExpression
 *      CallExpression
 *
 *  Where NewExpression is:
 *      MemberExpression
 *      new NewExpression
 *
 *  Where CallExpression is:
 *      MemberExpression Arguments
 *      CallExpression Arguments
 *      CallExpression PropertyOperator
 *
 *  Where MemberExpression is:
 *      PrimaryExpression -a,b-
 *      new MemberExpression Arguments
 *      SuperExpression PropertyOperator
 *      MemberExpression PropertyOperator
 *
 *  So look ahead problem on MemberExpression. We don't know if it is a newExpression or a CallExpression. This requires
 *  large lookahead. So, refactored to be:
 *
 *  LeftHandSideExpression (136) -a,b-
 *      new MemberExpression LeftHandSidePrime
 *      MemberExpression LeftHandSidePrime
 *
 *  and where LeftHandSidePrime is:
 *      Arguments LeftHandSidePrime
 *      PropertyOperator LeftHandSidePrime
 *      EMPTY
 *
 *  Input
 *      null
 *      true
 *      false
 *      this
 *      function
 *      Identifier
 *      {
 *      [
 *      (
 *      @
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *      super
 *      new
 *
 *  Also:
 *      new MemberExpression
 *      MemberExpression
 *      MemberExpression (
 *      MemberExpression .
 *      MemberExpression ..
 *      MemberExpression [
 *
 *
 *  AST
 *      N_CALL
 *      N_FUNCTION
 *      N_DOT
 *          left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
 *          right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
 *      N_LITERAL
 *      N_NEW           (array / object literals)
 *      N_QNAME
 *      N_SUPER
 *      N_THIS
 */
static EcNode *parseLeftHandSideExpression(EcCompiler *cp)
{
    EcNode      *np, *callNode;

    ENTER(cp);

    if (peekToken(cp) == T_NEW) {
        np = parseNewExpression(cp);

    } else {
        np = parseMemberExpression(cp);
    }

    if (np) {
        /*
         *  Refactored CallExpression processing
         */
        switch (peekToken(cp)) {
        case T_LPAREN:
        case T_DOT:
        case T_DOT_DOT:
        case T_LBRACKET:
            if (np->lineNumber == cp->peekToken->lineNumber) {
                np = parseCallExpression(cp, np);
            }
            break;

        default:
            if (np->kind == N_NEW) {
                /*
                 *  Create a dummy call to the constructor
                 */
                callNode = createNode(cp, N_CALL);
                np = appendNode(callNode, np);
                np = appendNode(np, createNode(cp, N_ARGS));
            }
            break;
        }
    }

    return LEAVE(cp, np);
}



#if UNUSED
/*
 *  LeftHandSidePrime (psudo) -a,b-
 *      Arguments LeftHandSidePrime
 *      PropertyOperator LeftHandSidePrime
 *      EMPTY
 *
 *  Input
 *      (
 *      .
 *      ..
 *      null
 *      true
 *      false
 *      this
 *      function
 *      Identifier
 *      {
 *      [
 *      (
 *      @
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *      super
 *      new
 *
 *  AST
 *      N_CALL
 *      N_FUNCTION
 *      N_DOT
 *          left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
 *          right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
 *      N_LITERAL
 *      N_NEW           (array / object literals)
 *      N_QNAME
 *      N_SUPER
 *      N_THIS
 */
static EcNode *parseLeftHandSidePrime(EcCompiler *cp, EcNode *np)
{
    EcNode      *callNode;

    ENTER(cp);

    do {
        switch (peekToken(cp)) {
        case T_LPAREN:
#if UNUSED
            callNode = createNode(cp, N_CALL);
            np = appendNode(callNode, np);
            np = appendNode(np, parseArguments(cp));
#endif
            break;

        case T_DOT:
        case T_DOT_DOT:
        case T_LBRACKET:
            np = appendNode(np, parsePropertyOperator(cp));
            break;

        default:
            return LEAVE(cp, np);
        }

    } while (np);

    return LEAVE(cp, np);
}
#endif



/*
 *  UnaryTypeExpression (136)
 *      LeftHandSideExpression
 *      type NullableTypeExpression
 *
 *  Input
 *      type
 *
 *  AST
 */
static EcNode *parseUnaryTypeExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    //  TODO

    if (peekToken(cp) == T_TYPE) {
        getToken(cp);
        //  TODO - do something with "type"
        np = parseNullableTypeExpression(cp);
    } else {
        np = parseLeftHandSideExpression(cp);
    }

    return LEAVE(cp, np);
}



/*
 *  PostfixExpression (138)
 *      UnaryTypeExpression
 *      LeftHandSideExpression [no line break] ++
 *      LeftHandSideExpression [no line break] --
 *
 *  Input
 *
 *  AST
 */
static EcNode *parsePostfixExpression(EcCompiler *cp)
{
    EcNode      *parent, *np;

    ENTER(cp);

    if (peekToken(cp) == T_TYPE) {
        np = parseUnaryTypeExpression(cp);
    } else {
        np = parseLeftHandSideExpression(cp);
        if (np) {
            if (peekToken(cp) == T_PLUS_PLUS || cp->peekToken->tokenId == T_MINUS_MINUS) {
                getToken(cp);
                parent = createNode(cp, N_POSTFIX_OP);
                np = appendNode(parent, np);
            }
        }
    }

    return LEAVE(cp, np);
}



/*
 *  UnaryExpression (141)
 *      PostfixExpression
 *      delete PostfixExpression
 *      void UnaryExpression
 *      typeof UnaryExpression
 *      ++ PostfixExpression
 *      -- PostfixExpression
 *      + UnaryExpression
 *      - UnaryExpression
 *      ~ UnaryExpression           (bitwise not)
 *      ! UnaryExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseUnaryExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_DELETE:
    case T_LOGICAL_NOT:
    case T_PLUS:
    case T_PLUS_PLUS:
    case T_MINUS:
    case T_MINUS_MINUS:
    case T_TILDE:
    case T_TYPEOF:
    case T_VOID:
        getToken(cp);
        np = createNode(cp, N_UNARY_OP);
        np = appendNode(np, parsePostfixExpression(cp));
        break;

    default:
        np = parsePostfixExpression(cp);
    }
    return LEAVE(cp, np);
}



/*
 *  MultiplicativeExpression (152) -a,b-
 *      UnaryExpression
 *      MultiplicativeExpression * UnaryExpression
 *      MultiplicativeExpression / UnaryExpression
 *      MultiplicativeExpression % UnaryExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseMultiplicativeExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseUnaryExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_MUL:
        case T_DIV:
        case T_MOD:
            getToken(cp);
            np = createBinaryNode(cp, np, parseUnaryExpression(cp));
            break;

        default:
            return LEAVE(cp, np);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  AdditiveExpression (156)
 *      MultiplicativeExpression
 *      AdditiveExpression + MultiplicativeExpression
 *      AdditiveExpression - MultiplicativeExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseAdditiveExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseMultiplicativeExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_PLUS:
        case T_MINUS:
            getToken(cp);
            np = createBinaryNode(cp, np, parseMultiplicativeExpression(cp));
            break;

        default:
            return LEAVE(cp, np);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  ShiftExpression (159) -a,b-
 *      AdditiveExpression
 *      ShiftExpression << AdditiveExpression
 *      ShiftExpression >> AdditiveExpression
 *      ShiftExpression >>> AdditiveExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseShiftExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseAdditiveExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_LSH:
        case T_RSH:
        case T_RSH_ZERO:
            getToken(cp);
            np = createBinaryNode(cp, np, parseAdditiveExpression(cp));
            break;

        default:
            return LEAVE(cp, np);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  RelationalExpression (163) -allowin-
 *      ShiftExpression
 *      RelationalExpression < ShiftExpression
 *      RelationalExpression > ShiftExpression
 *      RelationalExpression <= ShiftExpression
 *      RelationalExpression >= ShiftExpression
 *      RelationalExpression [in] ShiftExpression
 *      RelationalExpression instanceOf ShiftExpression
 *      RelationalExpression cast ShiftExpression
 *      RelationalExpression to ShiftExpression
 *      RelationalExpression is ShiftExpression
 *      RelationalExpression like ShiftExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseRelationalExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseShiftExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_IN:
            if (cp->state->noin) {
                return LEAVE(cp, np);
            }
            /* Fall through */

        case T_LT:
        case T_LE:
        case T_GT:
        case T_GE:
        case T_INSTANCEOF:
        case T_IS:
        case T_LIKE:
        case T_CAST:
            getToken(cp);
            np = createBinaryNode(cp, np, parseShiftExpression(cp));
            break;

        default:
            return LEAVE(cp, np);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  EqualityExpression (182)
 *      RelationalExpression
 *      EqualityExpression == RelationalExpression
 *      EqualityExpression != RelationalExpression
 *      EqualityExpression === RelationalExpression
 *      EqualityExpression !== RelationalExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseEqualityExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseRelationalExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_EQ:
        case T_NE:
        case T_STRICT_EQ:
        case T_STRICT_NE:
            getToken(cp);
            np = createBinaryNode(cp, np, parseRelationalExpression(cp));
            break;

        default:
            return LEAVE(cp, np);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  BitwiseAndExpression (187)
 *      EqualityExpression
 *      BitwiseAndExpression & EqualityExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseBitwiseAndExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseEqualityExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_BIT_AND:
            getToken(cp);
            np = createBinaryNode(cp, np, parseEqualityExpression(cp));
            break;

        default:
            return LEAVE(cp, np);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  BitwiseXorExpression (189)
 *      BitwiseAndExpression
 *      BitwiseXorExpression ^ BitwiseAndExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseBitwiseXorExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseBitwiseAndExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_BIT_XOR:
            getToken(cp);
            np = createBinaryNode(cp, np, parseBitwiseAndExpression(cp));
            break;

        default:
            return LEAVE(cp, np);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  BitwiseOrExpression (191)
 *      BitwiseXorExpression
 *      BitwiseOrExpression | BitwiseXorExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseBitwiseOrExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseBitwiseXorExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_BIT_OR:
            getToken(cp);
            np = createBinaryNode(cp, np, parseBitwiseXorExpression(cp));
            break;

        default:
            return LEAVE(cp, np);
        }
    }
    return LEAVE(cp, np);
}


/*
 *  LogicalAndExpression (193)
 *      BitwiseOrExpression
 *      LogicalAndExpression && BitwiseOrExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseLogicalAndExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseBitwiseOrExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_LOGICAL_AND:
            getToken(cp);
            np = createBinaryNode(cp, np, parseBitwiseOrExpression(cp));
            break;

        default:
            return LEAVE(cp, np);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  LogicalOrExpression (195)
 *      LogicalAndExpression
 *      LogicalOrExpression || LogicalOrExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseLogicalOrExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseLogicalAndExpression(cp);

    while (np) {
        switch (peekToken(cp)) {
        case T_LOGICAL_OR:
            getToken(cp);
            np = createBinaryNode(cp, np, parseLogicalOrExpression(cp));
            break;

        default:
            return LEAVE(cp, np);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  ConditionalExpression (197) -allowList,b-
 *      LetExpression -b-
 *      YieldExpression -b-
 *      LogicalOrExpression -a,b-
 *      LogicalOrExpression -allowList,b-
 *          ? AssignmentExpression : AssignmentExpression
 *
 *  ConditionalExpression (197) -noList,b-
 *      SimpleYieldExpression
 *      LogicalOrExpression -a,b-
 *      LogicalOrExpression -allowList,b-
 *          ? AssignmentExpression : AssignmentExpression
 *
 *  Input
 *      let
 *      yield
 *      (
 *      .
 *      ..
 *      null
 *      true
 *      false
 *      this
 *      function
 *      Identifier
 *      delete
 *      {
 *      [
 *      (
 *      @
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *      super
 *      new
 *
 *  AST
 *      N_CALL
 *      N_EXPRESSIONS
 *      N_FUNCTION
 *      N_DOT
 *          left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
 *          right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
 *      N_LITERAL
 *      N_NEW           (array / object literals)
 *      N_QNAME
 *      N_SUPER
 *      N_THIS
 *
 *  TODO - what about let and yield
 */
static EcNode *parseConditionalExpression(EcCompiler *cp)
{
    EcNode      *np, *cond;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_LET:
        np = parseLetExpression(cp);
        break;

    case T_YIELD:
        np = parseYieldExpression(cp);
        break;

    default:
        np = parseLogicalOrExpression(cp);
        if (np) {
            if (peekToken(cp) == T_QUERY) {
                getToken(cp);
                cond = np;
                np = createNode(cp, N_IF);
                np->tenary.cond = linkNode(np, cond);
                np->tenary.thenBlock = linkNode(np, parseAssignmentExpression(cp));
                if (getToken(cp) != T_COLON) {
                    // putToken(cp);
                    np = parseError(cp, "Expecting \":\"");
                } else {
                    np->tenary.elseBlock = linkNode(np, parseAssignmentExpression(cp));
                }
            }
        }
    }

    return LEAVE(cp, np);
}



/*
 *  NonAssignmentExpression -allowList,b- (199)
 *      LetExpression
 *      YieldExpression
 *      LogicalOrExpression -a,b-
 *      LogicalOrExpression -allowList,b-
 *          ? AssignmentExpression : AssignmentExpression
 *
 *  NonAssignmentExpression -noList,b-
 *      SimpleYieldExpression
 *      LogicalOrExpression -a,b-
 *      LogicalOrExpression -allowList,b-
 *          ? AssignmentExpression : AssignmentExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseNonAssignmentExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_LET:
        np = parseLetExpression(cp);
        break;

    case T_YIELD:
        np = parseYieldExpression(cp);
        break;

    default:
        np = parseLogicalOrExpression(cp);
        if (np) {
            if (peekToken(cp) == T_QUERY) {
                getToken(cp);
                np = parseAssignmentExpression(cp);
                if (getToken(cp) != T_COLON) {
                    // putToken(cp);
                    np = parseError(cp, "Expecting \":\"");
                } else {
                    np = parseAssignmentExpression(cp);
                }
            }
        }
    }
    return LEAVE(cp, np);
}



/*
 *  LetExpression (204)
 *      let ( LetBindingList ) ListExpression
 *
 *  Input
 *      let
 *
 *  AST
 */
static EcNode *parseLetExpression(EcCompiler *cp)
{
    EcNode  *np;

    if (getToken(cp) != T_LET) {
        return LEAVE(cp, expected(cp, "let"));
    }
    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, expected(cp, "("));
    }
    np = parseLetBindingList(cp);

    //  TODO INCOMPLETE
    if (getToken(cp) != T_RPAREN) {
        return LEAVE(cp, expected(cp, ")"));
    }
    return 0;
}



/*
 *  LetBindingList (205)
 *      EMPTY
 *      NonemptyLetBindingList -allowList-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseLetBindingList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



#if UNUSED
/*
 *  NonemptyLetBindingList (207) -a-
 *      VariableBinding -a,allowin-
 *      VariableBinding -noList,allowin- , NonemptyLetBindingList -a-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseNonemptyLetBindingList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



/*
 *  YieldExpression (209)
 *      yield
 *      yield [no line break] ListExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseYieldExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}


/*
 *  Rewrite compound assignment. Eg. given x += 3  rewrite as
 *      x = x + 3;
 */
static EcNode *rewriteCompoundAssignment(EcCompiler *cp, int subId, EcNode *lhs, EcNode *rhs)
{
    EcNode      *np;

    ENTER(cp);

    /*
     *  Map the operator token to its non-assignment counterpart
     */
    np = createNode(cp, N_BINARY_OP);
    np->tokenId = subId - 1;

    np = appendNode(np, lhs);
    np = appendNode(np, rhs);
    np = createAssignNode(cp, lhs, np);

    return LEAVE(cp, np);
}



/*
 *  AssignmentExpression (211)
 *      ConditionalExpression
 *      Pattern -a,b-allowin- = AssignmentExpression -a,b-
 *      SimplePattern -a,b-allowExpr- CompoundAssignmentOperator
 *              AssignmentExpression -a,b-
 *
 *  Where
 *      SimplePattern is:
 *          LeftHandSideExpression -a,b-
 *          Identifier
 *      ConditionalExpression is:
 *          LetExpression -b-
 *          YieldExpression -b-
 *          LogicalOrExpression -a,b-
 *
 *  TODO: Should LogicalAssignmentOperator and CompoundAssignmentOperator
 *      be Merged?
 *
 *  Input
 *      (
 *      .
 *      ..
 *      null
 *      true
 *      false
 *      this
 *      function
 *      Identifier
 *      {
 *      [
 *      (
 *      @
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *      super
 *      new
 *      let
 *      yield
 *
 *  AST
 *      N_CALL
 *      N_EXPRESSIONS
 *      N_FUNCTION
 *      N_DOT
 *          left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
 *          right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
 *      N_LITERAL
 *      N_NEW           (array / object literals)
 *      N_QNAME
 *      N_SUPER
 *      N_THIS
 *      N_DELETE
 *
 *  TODO - what obout let and yield
 */
static EcNode *parseAssignmentExpression(EcCompiler *cp)
{
    EcNode      *np;
    int         subId;

    ENTER(cp);

    /*
     *  A LeftHandSideExpression is allowed in both ConditionalExpression and in a SimplePattern. So allow the longest
     *  matching production to have first  crack at the input.
     */
    np = parseConditionalExpression(cp);
    if (np) {
        if (peekToken(cp) == T_ASSIGN) {
            getToken(cp);
            subId = cp->token->subId;
            if (cp->token->groupMask & G_COMPOUND_ASSIGN) {
                np = rewriteCompoundAssignment(cp, subId, np, parseAssignmentExpression(cp));

            } else {
                //  TODO - BUG. This creates the N_ASSIGN_OP too late and it gets the
                //  source line numbers after the RHS
                np = createAssignNode(cp, np, parseAssignmentExpression(cp));
            }
        }
    }

    return LEAVE(cp, np);
}



#if UNUSED
/*
 *  TODO - refactored
 *  CompoundAssignmentOperator (227)
 *      *=
 *      /=
 *      %=
 *      +=
 *      -=
 *      <<=
 *      >>=
 *      >>>=
 *      &=
 *      ^=
 *      |=
 *      &&=
 *      ||=
 *
 *  Input (see above)
 *
 *  AST
 */
static EcNode *parseCompoundAssignmentOperator(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



/*
 *  ListExpression (227)
 *      AssignmentExpression -allowList,b-
 *      ListExpression -b- , AssignmentExpression -allowList,b-
 *
 *  Input
 *      x = ...
 *
 *  AST
 *      N_EXPRESSIONS
 *
 */
static EcNode *parseListExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_EXPRESSIONS);
    mprAssert(np);

    do {
        np = appendNode(np, parseAssignmentExpression(cp));
    } while (np && getToken(cp) == T_COMMA);

    if (np) {
        putToken(cp);
    }

    return LEAVE(cp, np);
}



/*
 *  Pattern -a,b,g- (231)
 *      SimplePattern -a,b-g-
 *      ObjectPattern
 *      ArrayPattern
 *
 *  Input
 *      Identifier
 *      {
 *      [
 *      TODO much more
 *
 *  AST
 */
static EcNode *parsePattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
#if TODO
    case T_LBRACKET:
        np = parseArrayPattern(cp);
        break;

    case T_LBRACE:
        np = parseObjectPattern(cp);
        break;
#endif

    default:
        np = parseSimplePattern(cp);
        break;
    }

    return LEAVE(cp, np);
}



/*
 *  SimplePattern -a,b,noExpr- (232)
 *      Identifier
 *
 *  SimplePattern -a,b,noExpr- (233)
 *      LeftHandSideExpression -a,b-
 *
 *  Input
 *
 *  AST
 *      N_QNAME
 *      N_LIST_EXPRESSION
 */
static EcNode *parseSimplePattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseLeftHandSideExpression(cp);
    if (np == 0 && peekToken(cp) == T_ID) {
        np = parseIdentifier(cp);
    }

    return LEAVE(cp, np);
}



#if UNUSED
/*
 *  ObjectPattern -g- (234)
 *      { DestructuringFieldList }
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseObjectPattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  DestructuringFieldList -g- (248)
 *      EMPTY
 *      DestructuringField
 *      DestructuringField , DestructuringField
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseDestructuringFieldList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}


/*
 *  DestructuringField -g- (251)
 *      FieldName : Pattern -noList,allowin,g-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseDestructuringField(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  ArrayPattern (240)
 *      [ DestructuringElementList ]
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseArrayPattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    //  TODO
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}


/*
 *  DestructuringElementList -g- (253)
 *      EMPTY
 *      DestructuringElement
 *      , DestructuringElementList
 *      DestructuringElement , DestructuringElementList
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseDestructuringElement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



/*
 *  TypedIdentifier (258)
 *      SimplePattern -noList,noin,noExpr-
 *      SimplePattern -a,b,noExpr- : TypeExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseTypedIdentifier(EcCompiler *cp)
{
    EcNode      *np, *typeNode;

    ENTER(cp);

    np = parseSimplePattern(cp);

    if (peekToken(cp) == T_COLON) {

        getToken(cp);

        typeNode = parseTypeExpression(cp);

        if (typeNode) {
            np->typeNode = linkNode(np, typeNode);

        } else {
            np = parseError(cp, "Expecting type");
        }
    }

    return LEAVE(cp, np);
}



/*
 *  TypedPattern (248)
 *      SimplePattern -a,b,noExpr-
 *      SimplePattern -a,b,noExpr- : NullableTypeExpression
 *      ObjectPattern -noExpr-
 *      ObjectPattern -noExpr- : TypeExpression
 *      ArrayPattern -noExpr-
 *      ArrayPattern -noExpr- : TypeExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseTypedPattern(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
#if UNUSED
    case T_LBRACKET:
        np = parseArrayPattern(cp);
        if (peekToken(cp) == T_COLON) {
            getToken(cp);
            np->typeNode = linkNode(np, parseTypeExpression(cp));
        }
        break;

    case T_LBRACE:
        np = parseObjectPattern(cp);
        if (peekToken(cp) == T_COLON) {
            getToken(cp);
            np->typeNode = linkNode(np, parseTypeExpression(cp));
        }
        break;
#endif

    default:
        np = parseSimplePattern(cp);
        if (peekToken(cp) == T_COLON) {
            getToken(cp);
            np->typeNode = linkNode(np, parseNullableTypeExpression(cp));
        }
        break;
    }
    mprAssert(np == 0 || np->kind == N_QNAME);

#if UNUSED
    if (np) {
        np->name.isType = 1;
    }
#endif

    return LEAVE(cp, np);
}



/*
 *  NullableTypeExpression (266)
 *      null
 *      undefined
 *      TypeExpression
 *      TypeExpression ?            # Nullable
 *      TypeExpression !            # Non-Nullable
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseNullableTypeExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_NULL:
    case T_UNDEFINED:
        np = createNode(cp, N_QNAME);
        setId(np, (char*) cp->token->text);
        np->name.isType = 1;
        break;

    default:
        np = parseTypeExpression(cp);
        if (peekToken(cp) == T_QUERY) {
            getToken(cp);
            np->nullable = 1;

        } else if (cp->peekToken->tokenId == T_LOGICAL_NOT) {
            getToken(cp);
            np->nullable = 0;
        }
        break;
    }
    return LEAVE(cp, np);
}


/*
 *  TypeExpression (271)
 *      FunctionType
 *      UnionType
 *      RecordType
 *      ArrayType
 *      PrimaryName
 *
 *  Input
 *      function
 *      (
 *      {
 *      [
 *      Identifier
 *
 *  AST
 *      N_QNAME
 *      N_DOT
 */
static EcNode *parseTypeExpression(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
#if UNUSED
    case T_FUNCTION:
        np = appendNode(np, parseFunctionType(cp));
        break;

    case T_LPAREN:
        np = appendNode(np, parseUnionType(cp));
        break;

    case T_LBRACE:
        np = appendNode(np, parseRecordType(cp));
        break;

    case T_LBRACKET:
        appendNode(np, parseFunctionType(cp));
        break;
#endif

    case T_STRING:
    case T_ID:
    case T_MUL:
        np = parsePrimaryName(cp);
        if (np) {
            np->name.isType = 1;
        }

        break;

    default:
        getToken(cp);
        np = unexpected(cp);
    }

    return LEAVE(cp, np);
}



#if UNUSED
/*
 *  FunctionType (276)
 *      function FunctionSignatureType
 *
 *  Input sequnces
 *      function ...
 *
 *  AST
 */
static EcNode *parseFunctionType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  FunctionSignatureType (277)
 *      TypeParameters ( ParametersType ) ResultType
 *      TypeParameters ( this : PrimaryName ) ResultType
 *      TypeParameters ( this : PrimaryName , NonemptyParameters )
 *              ResultType
 *
 *  Input sequnces
 *      function ...
 *
 *  AST
 */
static EcNode *parseFunctionSignatureType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  ParametersType (280)
 *      EMPTY
 *      NonemptyParametersType
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseParametersType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  NonemptyParametersType (282)
 *      ParameterInitType
 *      ParameterInitType , NonemptyParametersType
 *      RestParameterType
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseNonemptyParametersType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  ParameterInitType (285)
 *      ParameterType
 *      ParameterType =
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseParameterInitType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  RestParameterType (288)
 *      ...
 *      ... ParameterType
 *
 *  Input
 *      ...
 *
 *  AST
 */
static EcNode *parseRestParameterType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  UnionType (290)
 *      ( TypeExpressionList )
 *
 *  Input
 *      (
 *
 *  AST
 */
static EcNode *parseUnionType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  RecordType (291)
 *      { FieldTypeList }
 *
 *  Input
 *      {
 *
 *  AST
 */
static EcNode *parseRecordType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  FieldTypeList (292)
 *      EMPTY
 *      NonemptyFieldTypeList
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseFieldTypeList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  NonemptyFieldTypeList (294)
 *      FieldType
 *      FieldType , NonemptyFieldTypeList
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseNonemptyFieldTypeList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  FieldType (296)
 *      FieldName : NullableTypeExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseFieldType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}

#endif



/*
 *  ArrayType (297)
 *      [ ElementTypeList ]
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseArrayType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_LBRACKET) {
        np = expected(cp, "[");
    } else {
        np = parseElementTypeList(cp);
        if (np) {
            if (getToken(cp) != T_LBRACKET) {
                np = expected(cp, "[");
            }
        }
    }

    return LEAVE(cp, np);
}



/*
 *  ElementTypeList (298)
 *      EMPTY
 *      NullableTypeExpression
 *      , ElementTypeList
 *      NullableTypeExpression , ElementTypeList
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseElementTypeList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



#if FUTURE
/*
 *  TypeExpressionList (302)
 *      NullableTypeExpression
 *      TypeExpressionList , NullableTypeExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseTypeExpressionList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



/*
 *  Statement (289) -t,o-
 *      Block -t-
 *      BreakStatement Semicolon -o-
 *      ContinueStatement Semicolon -o-
 *      DefaultXMLNamespaceStatement Semicolon -o-
 *      DoStatement Semicolon -o-
 *      ExpresssionStatement Semicolon -o-
 *      ForStatement -o-
 *      IfStatement -o-
 *      LabeledStatement -o-
 *      LetStatement -o-
 *      ReturnStatement Semicolon -o-
 *      SwitchStatement
 *      SwitchTypeStatement
 *      ThrowStatement Semicolon -o-
 *      TryStatement
 *      WhileStatement -o-
 *      WithStatement -o-
 *
 *  Input
 *      EMPTY
 *      {
 *      (
 *      .
 *      ..
 *      [
 *      (
 *      @
 *      break
 *      continue
 *      ?? DefaultXML
 *      do
 *      for
 *      if
 *      let
 *      return
 *      switch
 *      throw
 *      try
 *      while
 *      with
 *      null
 *      true
 *      false
 *      this
 *      function
 *      Identifier
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *      super
 *      new
 *
 *  AST
 *      N_BLOCK
 *      N_CONTINUE
 *      N_BREAK
 *      N_FOR
 *      N_FOR_IN
 *      N_HASH
 *      N_IF
 *      N_SWITCH
 *      N_THROW
 *      N_TRY
 *      N_WHILE
 *
 */
static EcNode *parseStatement(EcCompiler *cp)
{
    EcNode      *np;
    int         expectSemi, tid;

    ENTER(cp);

    expectSemi = 0;
    np = 0;

    switch ((tid = peekToken(cp))) {
    case T_AT:
    case T_DELETE:
    case T_DOT:
    case T_DOT_DOT:
    case T_FALSE:
    case T_FUNCTION:
    case T_LBRACKET:
    case T_LPAREN:
    case T_MINUS_MINUS:
    case T_NEW:
    case T_NUMBER:
    case T_NULL:
    case T_PLUS_PLUS:
    case T_STRING:
    case T_SUPER:
    case T_THIS:
    case T_TRUE:
    case T_TYPEOF:
        np = parseExpressionStatement(cp);
        expectSemi++;
        break;

    case T_BREAK:
        np = parseBreakStatement(cp);
        expectSemi++;
        break;

    case T_CONTINUE:
        np = parseContinueStatement(cp);
        expectSemi++;
        break;

    case T_DO:
        np = parseDoStatement(cp);
        expectSemi++;
        break;

    case T_FOR:
        np = parseForStatement(cp);
        break;

    case T_HASH:
        np = parseHashStatement(cp);
        break;

    case T_ID:
        if (tid == T_ID && peekAheadToken(cp, 2) == T_COLON) {
            np = parseLabeledStatement(cp);
        } else {
            np = parseExpressionStatement(cp);
            expectSemi++;
        }
        break;

    case T_IF:
        np = parseIfStatement(cp);
        break;

    case T_LBRACE:
        np = parseBlockStatement(cp);
        break;

    case T_LET:
        np = parseLetStatement(cp);
        break;

    case T_RETURN:
        np = parseReturnStatement(cp);
        expectSemi++;
        break;

    case T_SWITCH:
        np = parseSwitchStatement(cp);
        break;

    case T_THROW:
        np = parseThrowStatement(cp);
        expectSemi++;
        break;

    case T_TRY:
        np = parseTryStatement(cp);
        break;

    case T_WHILE:
        np = parseWhileStatement(cp);
        break;

    case T_WITH:
        np = parseWithStatement(cp);
        break;

    default:
        getToken(cp);
        np = unexpected(cp);
    }

    if (np && expectSemi) {
        if (getToken(cp) != T_SEMICOLON) {
            if (np->lineNumber < cp->token->lineNumber || cp->token->tokenId == T_EOF || cp->token->tokenId == T_NOP) {
                putToken(cp);
            } else {
                np = unexpected(cp);
            }
        }
    }
    return LEAVE(cp, np);
}



/*
 *  Substatement -o- (320)
 *      EmptyStatement
 *      Statement -o-
 *
 *  Statement:
 *      Block -t-
 *      BreakStatement Semicolon -o-
 *      ContinueStatement Semicolon -o-
 *      DefaultXMLNamespaceStatement Semicolon -o-
 *      DoStatement Semicolon -o-
 *      ExpresssionStatement Semicolon -o-
 *      ForStatement -o-
 *      IfStatement -o-
 *      LabeledStatement -o-
 *      LetStatement -o-
 *      ReturnStatement Semicolon -o-
 *      SwitchStatement
 *      SwitchTypeStatement
 *      ThrowStatement Semicolon -o-
 *      TryStatement
 *      WhileStatement -o-
 *      WithStatement -o-
 *
 *  Input
 *      EMPTY
 *      {
 *      (
 *      .
 *      ..
 *      [
 *      (
 *      @
 *      break
 *      continue
 *      ?? DefaultXML
 *      do
 *      for
 *      if
 *      let
 *      return
 *      switch
 *      throw
 *      try
 *      while
 *      with
 *      null
 *      true
 *      false
 *      this
 *      function
 *      Identifier
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *      super
 *      new
 *
 *  AST
 *      N_NOP
 *      N_BLOCK
 *      N_CONTINUE
 *      N_BREAK
 *      N_FOR
 *      N_FOR_IN
 *      N_IF
 *      N_SWITCH
 *      N_THROW
 *      N_TRY
 *      N_WHILE
 */
//  TODO - this is not right and needs to be able to handle EMPTY substatements

static EcNode *parseSubstatement(EcCompiler *cp)
{
    EcNode      *np;
    int         tid;

    ENTER(cp);

    np = 0;

    /*
     *  TODO: Missing: DefaultXML
     */
    switch ((tid = peekToken(cp))) {
    case T_AT:
    case T_BREAK:
    case T_CONTINUE:
    case T_DO:
    case T_DOT:
    case T_DOT_DOT:
    case T_FALSE:
    case T_FOR:
    case T_FUNCTION:
    case T_IF:
    case T_LBRACE:
    case T_LBRACKET:
    case T_LET:
    case T_LPAREN:
    case T_NEW:
    case T_NUMBER:
    case T_NULL:
    case T_RETURN:
    case T_STRING:
    case T_SUPER:
    case T_SWITCH:
    case T_THIS:
    case T_THROW:
    case T_TRUE:
    case T_TRY:
    case T_WHILE:
    case T_WITH:
        np = parseStatement(cp);
        break;

    case T_ID:
        if (peekAheadToken(cp, 2) == T_COLON) {
            /* Labeled expression */
            np = parseStatement(cp);
        } else {
            np = parseStatement(cp);
        }
        break;


    default:
        np = createNode(cp, N_NOP);
    }

    return LEAVE(cp, np);
}



#if UNUSED
/*
 *  Semicolon -abbrev- (322)
 *      ;
 *      VirtualSemicolon
 *      EMPTY
 *
 *  Semicolon -noshortif- (325)
 *      ;
 *      VirtualSemicolon
 *      EMPTY
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseSemicolon(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



/*
 *  EmptyStatement (33)
 *      ;
 *
 *  Input
 *      EMPTY
 *
 *  AST
 *      N_NOP
 */
static EcNode *parseEmptyStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = createNode(cp, N_NOP);;
    return LEAVE(cp, np);
}



/*
 *  ExpressionStatement (331)
 *      [lookahead !function,{}] ListExpression -allowin-
 *
 *  Input
 *      (
 *      .
 *      ..
 *      null
 *      true
 *      false
 *      this
 *      function
 *      delete
 *      Identifier
 *      [
 *      (
 *      @
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *      super
 *      new
 *
 *  AST
 *      N_CALL
 *      N_DELETE
 *      N_DOT
 *          left: N_QNAME | N_DOT | N_EXPRESSIONS | N_FUNCTION
 *          right: N_QNAME | N_EXPRESSIONS | N_FUNCTION
 *      N_EXPRESSIONS
 *      N_LITERAL
 *      N_NEW           (array / object literals)
 *      N_QNAME
 *      N_SUPER
 *      N_THIS
 */
static EcNode *parseExpressionStatement(EcCompiler *cp)
{
    EcNode      *np;
    int         tid;

    ENTER(cp);

    tid = peekToken(cp);
    if (tid == T_FUNCTION || tid == T_LBRACE) {
        //  TODO
        np = createNode(cp, 0);

    } else {
        np = parseListExpression(cp);
    }

    return LEAVE(cp, np);
}



/*
 *  BlockStatement (318)
 *      Block
 *
 *  Input
 *      {
 *
 *  AST
 *      N_BLOCK
 */
static EcNode *parseBlockStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = parseBlock(cp);
    return LEAVE(cp, np);
}



/*
 *  LabeledStatement -o- (319)
 *      Identifier : Substatement
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseLabeledStatement(EcCompiler *cp)
{
    getToken(cp);
    return parseError(cp, "Labeled statements are not yet implemented");
}




/*
 *  IfStatement -abbrev- (320)
 *      if ParenListExpression Substatement
 *      if ParenListExpression Substatement else Substatement
 *
 *  IfStatement -full- (322)
 *      if ParenListExpression Substatement
 *      if ParenListExpression Substatement else Substatement
 *
 *  IfStatement -noShortif- (324)
 *      if ParenListExpression Substatement else Substatement
 *
 *  Input
 *      if ...
 *
 *  AST
 */
static EcNode *parseIfStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_IF) {
        return LEAVE(cp, parseError(cp, "Expecting \"if\""));
    }
    if (peekToken(cp) != T_LPAREN) {
        getToken(cp);
        return LEAVE(cp, parseError(cp, "Expecting \"(\""));
    }

    np = createNode(cp, N_IF);
    np->tenary.cond = linkNode(np, parseParenListExpression(cp));
    np->tenary.thenBlock = linkNode(np, parseSubstatement(cp));

    if (peekToken(cp) == T_ELSE) {
        getToken(cp);
        np->tenary.elseBlock = linkNode(np, parseSubstatement(cp));
    }

    return LEAVE(cp, np);
}



/*
 *  SwitchStatement (328)
 *      switch ParenListExpression { CaseElements }
 *      switch type ( ListExpression -allowList,allowin- : TypeExpression )
 *              { TypeCaseElements }
 *
 *  Input
 *      switch ...
 *
 *  AST
 *      N_SWITCH
 *          N_EXPRESSIONS           ( ListExpression )
 *          N_CASE_ELEMENTS
 */
static EcNode *parseSwitchStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_SWITCH);

    if (getToken(cp) != T_SWITCH) {
        np = unexpected(cp);

    } else {
        if (peekToken(cp) != T_TYPE) {
            np = appendNode(np, parseParenListExpression(cp));
            if (getToken(cp) != T_LBRACE) {
                np = parseError(cp, "Expecting \"{\"");
            } else {
                np = appendNode(np, parseCaseElements(cp));
                if (getToken(cp) != T_RBRACE) {
                    np = parseError(cp, "Expecting \"{\"");
                }
            }

        } else {
#if FUTURE
            //  switch type
            getToken(cp);
            if (getToken(cp) != T_LPAREN) {
                np = parseError(cp, "Expecting \"(\"");
            } else {
                x = parseListExpression(cp);
                if (getToken(cp) != T_COLON) {
                    np = parseError(cp, "Expecting \":\"");
                } else {
                    x = parseTypeExpression(cp);
                    if (getToken(cp) != T_RPAREN) {
                        np = parseError(cp, "Expecting \")\"");
                    } else  if (getToken(cp) != T_LBRACE) {
                        np = parseError(cp, "Expecting \"{\"");
                    } else {
                        x = parseTypeCaseElements(cp);
                        if (getToken(cp) != T_RBRACE) {
                            np = parseError(cp, "Expecting \"}\"");
                        }
                    }
                }
                parseListExpression(cp);
            }
#endif
        }
    }
    return LEAVE(cp, np);
}



/*
 *  CaseElements (342)
 *      EMPTY
 *      CaseLabel
 *      CaseLabel CaseElementsPrefix CaseLabel
 *      CaseLabel CaseElementsPrefix Directive -abbrev-
 *
 *  Refactored as:
 *      EMPTY
 *      CaseLable Directives
 *      CaseElements
 *
 *  Input
 *      case
 *      default
 *
 *  AST
 *      N_CASE_ELEMENTS
 *          N_CASE_LABEL: kind, expression
 */
static EcNode *parseCaseElements(EcCompiler *cp)
{
    EcNode      *np, *caseLabel, *directives;

    ENTER(cp);

    np = createNode(cp, N_CASE_ELEMENTS);

    while (peekToken(cp) == T_CASE || cp->peekToken->tokenId == T_DEFAULT) {

        caseLabel = parseCaseLabel(cp);
        directives = createNode(cp, N_DIRECTIVES);
        caseLabel = appendNode(caseLabel, directives);

        while (peekToken(cp) != T_CASE && cp->peekToken->tokenId != T_DEFAULT) {
            if (cp->peekToken->tokenId == T_RBRACE) {
                break;
            }
            directives = appendNode(directives, parseDirective(cp));
        }
        np = appendNode(np, caseLabel);
    }

    return LEAVE(cp, np);
}



#if UNUSED && NOT_REQUIRED
/*
 *  CaseElementsPrefix (346)
 *      EMPTY
 *      CaseElementsPrefix CaseLabel
 *      CaseElementsPrefix Directive -full-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseCaseElementsPrefix(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



/*
 *  CaseLabel (349)
 *      case ListExpression -allowin- :
 *      default :
 *
 *  Input
 *      case .. :
 *      default :
 *
 *  AST
 *      N_CASE_LABEL  kind, expression
 */
static EcNode *parseCaseLabel(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = 0;

    if (peekToken(cp) == T_CASE) {
        getToken(cp);
        np = createNode(cp, N_CASE_LABEL);
        np->caseLabel.kind = EC_SWITCH_KIND_CASE;
        np->caseLabel.expression = linkNode(np, parseListExpression(cp));

    } else if (cp->peekToken->tokenId == T_DEFAULT) {
        getToken(cp);
        np = createNode(cp, N_CASE_LABEL);
        np->caseLabel.kind = EC_SWITCH_KIND_DEFAULT;
    }
    if (getToken(cp) != T_COLON) {
        np = expected(cp, ":");
    }
    return LEAVE(cp, np);
}



#if UNUSED
/*
 *  TypeCaseElements (351)
 *      TypeCaseElement
 *      TypeCaseElement TypeCaseElement
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseTypeCaseElements(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  TypeCaseElement (353)
 *      case ( TypedPattern -noList,noIn- ) Block -local-
 *      default Block -local-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseTypeCaseElement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



/*
 *  DoStatement (355)
 *      do Substatement -abbrev- while ParenListExpresison
 *
 *  Input
 *      do
 *
 *  AST
 *      N_FOR
 */
static EcNode *parseDoStatement(EcCompiler *cp)
{
    EcNode      *np;


    ENTER(cp);

    np = createNode(cp, N_DO);

    if (getToken(cp) != T_DO) {
        return LEAVE(cp, unexpected(cp));
    }

    //  TODO - should remove the N_DO ast and just reuse N_FOR

    np->forLoop.body = linkNode(np, parseSubstatement(cp));

    if (getToken(cp) != T_WHILE) {
        np = expected(cp, "while");

    } else {
        np->forLoop.cond = linkNode(np, parseParenListExpression(cp));
    }

    return LEAVE(cp, np);
}



/*
 *  WhileStatement (356)
 *      while ParenListExpresison Substatement -o-
 *
 *  Input
 *      while
 *
 *  AST
 *      N_FOR
 */
static EcNode *parseWhileStatement(EcCompiler *cp)
{
    EcNode      *np, *initializer;

    ENTER(cp);

    initializer = 0;

    if (getToken(cp) != T_WHILE) {
        return LEAVE(cp, parseError(cp, "Expecting \"while\""));
    }


    /*
     *  Convert into a "for" AST
     */
    np = createNode(cp, N_FOR);
    np->forLoop.cond = linkNode(np, parseParenListExpression(cp));
    np->forLoop.body = linkNode(np, parseSubstatement(cp));

    return LEAVE(cp, np);
}



/*
 *  ForStatement -o- (357)
 *      for ( ForInitializer ; OptionalExpression ; OptionalExpression )
 *              Substatement
 *      for ( ForInBinding in ListExpression -allowin- ) Substatement
 *      for each ( ForInBinding in ListExpression -allowin- ) Substatement
 *
 *  Where:
 *
 *  ForIntializer (360)
 *      EMPTY
 *      ListExpression -noin-
 *      VariableDefinition -noin-
 *
 *  ForInBinding (363)
 *      Pattern -allowList,noIn,allowExpr-
 *      VariableDefinitionKind VariableBinding -allowList,noIn-
 *
 *  VariableDefinition -b- (429)
 *      VariableDefinitionKind VariableBindingList -allowList,b-
 *
 *  VariableDefinitionKind (430)
 *      const
 *      let
 *      let const
 *      var
 *
 *  MemberExpression Input tokens
 *      null
 *      true
 *      false
 *      this
 *      function
 *      Identifier
 *      {
 *      [
 *      (
 *      @
 *      NumberLiteral
 *      StringLiteral
 *      RegularExpression
 *      XMLInitializer: <!--, [CDATA, <?, <
 *      super
 *      new
 *
 *  Input
 *      for ( 'const|let|let const|var' '[|{'
 *
 *  AST
 *      N_FOR
 *      N_FOR_IN
 */
static EcNode *parseForStatement(EcCompiler *cp)
{
    EcNode      *np, *initializer, *body, *iterGet, *block, *callGet;
    int         each, forIn;

    ENTER(cp);

    initializer = 0;
    np = 0;
    forIn = 0;
    each = 0;

    if (getToken(cp) != T_FOR) {
        return LEAVE(cp, parseError(cp, "Expecting \"for\""));
    }

    if (peekToken(cp) == T_EACH) {
        each++;
        getToken(cp);
    }

    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, parseError(cp, "Expecting \"(\""));
    }

    //  TODO - currently only parsing VariableDefinition. Need to switch
    //  to ForInitializer and ForInBinding instead
    //  np = parseForInitializer(cp);
    //  np = parseForInBinding(cp);

    if (peekToken(cp) == T_ID && peekAheadToken(cp, 2) == T_IN) {
        /*
         *  For in forces the variable to be a let scoped var
         */
        initializer = createNode(cp, N_VAR_DEFINITION);
        if (initializer) {
            initializer->def.varKind = KIND_LET;
            initializer = parseVariableBindingList(cp, initializer, 0);
        }

    } else if (peekToken(cp) == T_CONST || cp->peekToken->tokenId == T_LET || cp->peekToken->tokenId == T_VAR) {
        initializer = parseVariableDefinition(cp, 0);

    } else if (cp->peekToken->tokenId != T_SEMICOLON) {
        cp->state->noin = 1;
        initializer = parseListExpression(cp);
    }
    if (initializer == 0 && cp->error) {
        return LEAVE(cp, 0);
    }

    if (getToken(cp) == T_SEMICOLON) {
        forIn = 0;
        np = createNode(cp, N_FOR);
        np->forLoop.initializer = linkNode(np, initializer);
        if (peekToken(cp) != T_SEMICOLON) {
            np->forLoop.cond = linkNode(np, parseOptionalExpression(cp));
        }
        if (getToken(cp) != T_SEMICOLON) {
            np = parseError(cp, "Expecting \";\"");
        } else if (peekToken(cp) != T_RPAREN) {
            np->forLoop.perLoop = linkNode(np, parseOptionalExpression(cp));
        }

    } else if (cp->token->tokenId == T_IN) {
        forIn = 1;
        np = createNode(cp, N_FOR_IN);
        np->forInLoop.iterVar = linkNode(np, initializer);

        /*
         *  Create a "listExpression.get/values" node
         */
        iterGet = appendNode(createNode(cp, N_DOT), parseListExpression(cp));
        iterGet = appendNode(iterGet, createNameNode(cp, (each) ? "getValues" : "get", EJS_ITERATOR_NAMESPACE));

        /*
         *  Create a call node for "get"
         */
        callGet = createNode(cp, N_CALL);
        callGet = appendNode(callGet, iterGet);
        callGet = appendNode(callGet, createNode(cp, N_ARGS));
        np->forInLoop.iterGet = linkNode(np, callGet);

        np->forInLoop.iterNext = linkNode(np, createNode(cp, N_NOP));

        if (np->forInLoop.iterVar == 0 || np->forInLoop.iterGet == 0) {
            return LEAVE(cp, 0);
        }

    } else {
        return LEAVE(cp, unexpected(cp));
    }

    if (getToken(cp) != T_RPAREN) {
        np = parseError(cp, "Expecting \")\"");
    }

    body = linkNode(np, parseSubstatement(cp));
    if (body == 0) {
        return LEAVE(cp, body);
    }

    /*
     *  Fixup the body block and move it outside the entire for loop.
     */
    if (body->kind == N_BLOCK) {
        block = body;
        body = removeNode(block, block->left);

    } else {
        block = createNode(cp, N_BLOCK);
    }

    if (forIn) {
        np->forInLoop.body = linkNode(np, body);
        np->forInLoop.each = each;

    } else {
        if (each) {
            return LEAVE(cp, parseError(cp, "\"for each\" can only be used with \"for .. in\""));
        }
        mprAssert(np != body);
        np->forLoop.body = linkNode(np, body);
    }

    /*
     *  Now make the for loop node a child of the outer block. Block will initially be a child of np, so must re-parent first
     */
    mprAssert(block != np);
    mprStealBlock(cp->state, block);
    np = appendNode(block, np);

    return LEAVE(cp, np);
}



#if UNUSED
/*
 *  ForIntializer (360)
 *      EMPTY
 *      ListExpression -noin-
 *      VariableDefinition -noin-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseForInitializer(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  ForInBinding (363)
 *      Pattern -allowList,noIn,allowExpr-
 *      VariableDefinitionKind VariableBinding -allowList,noIn-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseForInBinding(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



/*
 *  HashStatement (EJS)
 *      # ListExpression
 *
 *  Input
 *      # expression directive
 *
 *  AST
 *      N_HASH
 */
static EcNode *parseHashStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_HASH) {
        return LEAVE(cp, parseError(cp, "Expecting \"#\""));
    }

    np = createNode(cp, N_HASH);
    np->hash.expr = linkNode(np, parseListExpression(cp));
    np->hash.body = linkNode(np, parseDirective(cp));

    return LEAVE(cp, np);
}



/* MOB OptionalExpression was replicated at 365 */


/*
 *  LetStatement (367)
 *      let ( LetBindingList ) Substatement -o-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseLetStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  WithStatement -o- (368)
 *      with ( ListExpression -allowin- ) Substatement -o-
 *      with ( ListExpression -allowin- : TypeExpression ) Substatement -o-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseWithStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_WITH) {
        return LEAVE(cp, expected(cp, "with"));
    }
    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, expected(cp, "("));
    }

    np = createNode(cp, N_WITH);
    np->with.object = linkNode(np, parseListExpression(cp));

    if (getToken(cp) != T_RPAREN) {
        return LEAVE(cp, expected(cp, ")"));
    }
    np->with.statement = linkNode(np, parseSubstatement(cp));
    return LEAVE(cp, np);
}




/*
 *  ContinueStatement (370)
 *      continue
 *      continue [no line break] Identifier
 *
 *  Input
 *      continue
 *
 *  AST
 *      N_CONTINUE
 */
static EcNode *parseContinueStatement(EcCompiler *cp)
{
    EcNode      *np;
    int         lineNumber;

    ENTER(cp);

    if (getToken(cp) != T_CONTINUE) {
        np = expected(cp, "continue");
    } else {
        np = createNode(cp, N_CONTINUE);
        lineNumber = cp->token->lineNumber;
        if (peekToken(cp) == T_ID && lineNumber == cp->peekToken->lineNumber) {
            np = appendNode(np, parseIdentifier(cp));
        }
    }
    return LEAVE(cp, np);
}



/*
 *  BreakStatement (372)
 *      break
 *      break [no line break] Identifier
 *
 *  Input
 *      break
 *
 *  AST
 *      N_BREAK
 */
static EcNode *parseBreakStatement(EcCompiler *cp)
{
    EcNode      *np;
    int         lineNumber;

    ENTER(cp);

    if (getToken(cp) != T_BREAK) {
        np = expected(cp, "break");
    } else {
        np = createNode(cp, N_BREAK);
        lineNumber = cp->token->lineNumber;
        if (peekToken(cp) == T_ID && lineNumber == cp->peekToken->lineNumber) {
            np = appendNode(np, parseIdentifier(cp));
        }
    }
    return LEAVE(cp, np);
}



/*
 *  ReturnStatement (374)
 *      return
 *      return [no line break] ListExpression -allowin-
 *
 *  Input
 *      return ...
 *
 *  AST
 *      N_RETURN
 */
static EcNode *parseReturnStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_RETURN) {
        //  TODO - make a convenience function
        np = unexpected(cp);

    } else {

        if (cp->state->currentFunctionNode == 0) {
            np = parseError(cp, "Return statemeout outside function");

        } else {
            //  TODO - handle no line break
            np = createNode(cp, N_RETURN);
            if (peekToken(cp) != T_SEMICOLON && np->lineNumber == cp->peekToken->lineNumber) {
                np = appendNode(np, parseListExpression(cp));
            }
        }
    }
    return LEAVE(cp, np);
}



/*
 *  ThrowStatement (376)
 *      throw ListExpression -allowin-
 *
 *  Input
 *      throw ...
 *
 *  AST
 */
static EcNode *parseThrowStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_THROW) {
        return LEAVE(cp, unexpected(cp));
    }

    np = createNode(cp, N_THROW);
    np = appendNode(np, parseListExpression(cp));

    return LEAVE(cp, np);
}



/*
 *  TryStatement (377)
 *      try Block -local- CatchClauses
 *      try Block -local- CatchClauses finally Block -local-
 *      try Block -local- finally Block -local-
 *
 *  Input
 *      try ...
 *
 *  AST
 */
static EcNode *parseTryStatement(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    /*
     *  Just ignore try / catch for now
     */
    if (getToken(cp) != T_TRY) {
        return LEAVE(cp, unexpected(cp));
    }
    np = createNode(cp, N_TRY);
    if (np) {
        np->exception.tryBlock = linkNode(np, parseBlock(cp));
        if (peekToken(cp) == T_CATCH) {
            np->exception.catchClauses = linkNode(np, parseCatchClauses(cp));
        }
        if (peekToken(cp) == T_FINALLY) {
            getToken(cp);
            np->exception.finallyBlock = linkNode(np, parseBlock(cp));
        }
    }
    return LEAVE(cp, np);
}



/*
 *  CatchClauses (380)
 *      CatchClause
 *      CatchClauses CatchClause
 *
 *  Input
 *      catch
 */
static EcNode *parseCatchClauses(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) != T_CATCH) {
        getToken(cp);
        return LEAVE(cp, unexpected(cp));
    }
    np = createNode(cp, N_CATCH_CLAUSES);

    do {
        np = appendNode(np, parseCatchClause(cp));
    } while (peekToken(cp) == T_CATCH);

    return LEAVE(cp, np);
}



/*
 *  CatchClause (382)
 *      catch ( Parameter ) Block -local-
 *
 *  Input
 *      catch
 *
 *  AST
 *      T_CATCH
 */
static EcNode *parseCatchClause(EcCompiler *cp)
{
    EcNode      *np, *block, *arg, *varDef;

    ENTER(cp);


    if (getToken(cp) != T_CATCH) {
        return LEAVE(cp, unexpected(cp));
    }

    np = createNode(cp, N_CATCH);

    /*
     *  EJS enhancement: allow no (Parameter)
     */
    varDef = 0;
    arg = 0;
    if (peekToken(cp) == T_LPAREN) {
        getToken(cp);
        varDef = parseParameter(cp, 0);
        if (varDef) {
            mprAssert(varDef->kind == N_VAR_DEFINITION);
            varDef->def.varKind = KIND_LET;
            arg = varDef->left;
            mprAssert(arg->kind == N_QNAME);
            removeNode(varDef, arg);
            arg->qname.space = cp->state->namespace;
        }
        if (getToken(cp) != T_RPAREN) {
            return LEAVE(cp, unexpected(cp));
        }
        /*
         *  Insert an assign node and value
         */
        if (varDef) {
            arg = createAssignNode(cp, arg, createNode(cp, N_CATCH_ARG));
            varDef = appendNode(varDef, arg);
        }
    }
    np->catchBlock.arg = varDef;

    block = parseBlock(cp);
    if (block) {
        if (varDef) {
            block = insertNode(block, varDef, 0);
        }
    }

    np = appendNode(np, block);


    return LEAVE(cp, np);
}



/* -t- == global, class, interface, local */

/*
 *  Directives -t- (367)
 *      EMPTY
 *      DirectivesPrefix Directive -t,abbrev-
 *
 *  Input
 *      #
 *      import
 *      use
 *      {
 *      (
 *      break
 *      continue
 *      ?? DefaultXML
 *      do
 *      ?? ExpressionS
 *      for
 *      if
 *      label :
 *      let
 *      new
 *      return
 *      switch
 *      throw
 *      try
 *      while
 *      with
 *      internal, intrinsic, private, protected, public
 *      final, native, override, prototype, static
 *      [ attribute AssignmentExpression
 *      Identifier
 *      const
 *      let
 *      var
 *      function
 *      interface
 *      namespace
 *      type
 *      module
 *
 *  AST
 *      N_DIRECTIVES
 */
static EcNode *parseDirectives(EcCompiler *cp)
{
    EcNode      *np;
    EcState     *saveState;
    EcState     *state;

    ENTER(cp);

    np = createNode(cp, N_DIRECTIVES);
    state = cp->state;
    state->topVarBlockNode = np;

    saveState = cp->directiveState;
    cp->directiveState = state;

    state->blockNestCount++;

    do {
        switch (peekToken(cp)) {
        case T_ERR:
            cp->directiveState = saveState;
            getToken(cp);
            return LEAVE(cp, unexpected(cp));

        case T_EOF:
            cp->directiveState = saveState;
            return LEAVE(cp, np);

        case T_USE:
            np = appendNode(np, parseDirectivesPrefix(cp));
            break;

        case T_RBRACE:
            if (state->blockNestCount == 1) {
                getToken(cp);
            }
            cp->directiveState = saveState;
            return LEAVE(cp, np);

        case T_SEMICOLON:
            getToken(cp);
            break;

        case T_ATTRIBUTE:
        case T_BREAK:
        case T_CLASS:
        case T_CONST:
        case T_CONTINUE:
        case T_DELETE:
        case T_DO:
        case T_FALSE:
        case T_FOR:
        case T_FINAL:
        case T_FUNCTION:
        case T_HASH:
        case T_ID:
        case T_IF:
        case T_INTERFACE:
        case T_MINUS_MINUS:
        case T_LBRACKET:
        case T_LBRACE:
        case T_LPAREN:
        case T_LET:
        case T_NAMESPACE:
        case T_NATIVE:
        case T_NEW:
        case T_NUMBER:
        case T_RESERVED_NAMESPACE:
        case T_RETURN:
        case T_PLUS_PLUS:
        case T_STRING:
        case T_SUPER:
        case T_SWITCH:
        case T_THIS:
        case T_THROW:
        case T_TRUE:
        case T_TRY:
        case T_TYPEOF:
        case T_VAR:
        case T_WHILE:
        case T_MODULE:
        case T_WITH:
            np = appendNode(np, parseDirective(cp));
            break;

        case T_NOP:
            if (state->blockNestCount == 1) {
                getToken(cp);
                break;

            } else {
                /*
                 *  NOP tokens are injected when reading from the console. If we are nested,
                 *  we need to eat all input. Then continue.
                 */
                ecResetInput(cp);
            }
            break;

        default:
            getToken(cp);
            np = unexpected(cp);
            cp->directiveState = saveState;
            return LEAVE(cp, np);
        }

        if (cp->error && !cp->fatalError) {
            np = ecResetError(cp, np, 1);
        }

        //  TODO - refactor
    } while (np && (!cp->interactive || state->blockNestCount > 1));

    cp->directiveState = saveState;
    return LEAVE(cp, np);
}



/*
 *
 *  DirectivesPrefix -t- (369)
 *      EMPTY
 *      Pragmas
 *      DirectivesPrefix Directive -t,full-
 *
 *  Rewritten as:
 *      DirectivesPrefix
 *
 *  Input
 *      use
 *      import
 *
 *  AST
 *      N_PRAGMAS
 */
static EcNode *parseDirectivesPrefix(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_PRAGMAS);

    do {
        switch (peekToken(cp)) {
        case T_ERR:
            return LEAVE(cp, unexpected(cp));

        case T_EOF:
            return LEAVE(cp, np);;

        case T_USE:
            np = parsePragmas(cp, np);
            break;

        default:
            return LEAVE(cp, np);
        }

    } while (np);

    return LEAVE(cp, np);
}




/*
 *  Scan ahead and see if this is an annotatable directive
 */
static int isAttribute(EcCompiler *cp)
{
    int     i, tid;

    /*
     *  Assume we have just seen an ID. Handle the following patterns:
     *      nspace var
     *      nspace function
     *      nspace class
     *      nspace interface
     *      nspace let
     *      nspace const
     *      nspace type
     *      nspace namespace
     *      a.nspace namespace
     *      a.b.c.nspace::nspace namespace
     */
    for (i = 2; i < EC_MAX_LOOK_AHEAD + 2; i++) {
        peekAheadToken(cp, i);
        tid = cp->peekToken->tokenId;
        switch (tid) {
        case T_ATTRIBUTE:
        case T_CLASS:
        case T_CONST:
        case T_FUNCTION:
        case T_INTERFACE:
        case T_LET:
        case T_MUL:
        case T_NAMESPACE:
        case T_RESERVED_NAMESPACE:
        case T_TYPE:
//      case T_MODULE:
        case T_VAR:
            return 1;

        case T_COLON_COLON:
        case T_DOT:
            break;

        default:
            return 0;
        }

        /*
         *  Just saw a "." or "::".  Make sure this is part of a PropertyName
         */
        tid = peekAheadToken(cp, ++i);
        if (tid != T_ID && tid != T_RESERVED_NAMESPACE && tid == T_MUL && tid != T_STRING && tid != T_NUMBER &&
                tid != T_LBRACKET && !(cp->peekToken->groupMask & G_RESERVED)) {
            return 0;
        }
    }

    return 0;
}



/*
 *  Directive -t,o- (372)
 *      EmptyStatement
 *      Statement
 *      AnnotatableDirective -t,o-
 *
 *  Input
 *      #
 *      {
 *      break
 *      continue
 *      ?? DefaultXML
 *      do
 *      ?? Expressions
 *      for
 *      if
 *      label :
 *      let
 *      return
 *      switch
 *      throw
 *      try
 *      while
 *      with
 *      *
 *      internal, intrinsic, private, protected, public
 *      final, native, override, prototype, static
 *      [ attribute AssignmentExpression
 *      Identifier
 *      const
 *      let
 *      var
 *      function
 *      interface
 *      namespace
 *      type
 *
 *  AST
 *      N_BLOCK
 *      N_CONTINUE
 *      N_BREAK
 *      N_FOR
 *      N_FOR_IN
 *      N_HASH
 *      N_IF
 *      N_SWITCH
 *      N_THROW
 *      N_TRY
 *      N_WHILE
 */
static EcNode *parseDirective(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (peekToken(cp)) {
    case T_EOF:
        getToken(cp);
        return LEAVE(cp, 0);

    case T_ERR:
        getToken(cp);
        return LEAVE(cp, unexpected(cp));

    /* EmptyStatement */
    case T_SEMICOLON:
        np = parseEmptyStatement(cp);
        break;

    /* Statement */
    /*
     *  TBD -- missing:
     *      - DefaultXMLNamespaceStatement
     *      - ExpressionStatement
     *      - LabeledStatement
     */
    case T_LBRACE:
    case T_BREAK:
    case T_CONTINUE:
    case T_DELETE:
    case T_DO:
    case T_FOR:
    case T_HASH:
    case T_IF:
    case T_RETURN:
    case T_SUPER:
    case T_SWITCH:
    case T_THROW:
    case T_TRY:
    case T_WHILE:
    case T_WITH:
        np = parseStatement(cp);
        break;

    case T_ID:
        if (isAttribute(cp)) {
            np = parseAnnotatableDirective(cp, 0);
        } else {
            np = parseStatement(cp);
        }
        break;

    /* AnnotatableDirective */
    case T_ATTRIBUTE:
    case T_CLASS:
    case T_CONST:
    case T_FUNCTION:
    case T_INTERFACE:
    case T_LET:
    case T_MUL:
    case T_NAMESPACE:
    case T_RESERVED_NAMESPACE:
    case T_TYPE:
    case T_MODULE:
    case T_VAR:
        np = parseAnnotatableDirective(cp, 0);
        break;

    case T_STRING:
        //  TDOO - should we test let ...?
        if (peekAheadToken(cp, 2) == T_VAR || peekAheadToken(cp, 2) == T_CLASS || peekAheadToken(cp, 2) == T_FUNCTION) {
            np = parseAnnotatableDirective(cp, 0);
        } else {
            np = parseStatement(cp);
        }
        break;

#if FUTURE
    /* IncludeDirective */
    case T_INCLUDE:
        np = parseIncludeStatement(cp);
        break;
#endif

    default:
        np = parseStatement(cp);
    }

    return LEAVE(cp, np);
}



/*
 *  AnnotatableDirective -global,o- (375)
 *      Attributes [no line break] AnnotatableDirective -t,o-
 *      VariableDefinition -allowin- Semicolon -o-
 *      FunctionDefinition -global-
 *      ClassDefinition
 *      InterfaceDefintion
 *      NamespaceDefinition Semicolon -o-
 *      TypeDefinition Semicolon
 *      PackageDefinition
 *      ModuleDefinition
 *
 *  AnnotatableDirective -interface,o- (384)
 *      Attributes [no line break] AnnotatableDirective -t,o-
 *      FunctionDeclaration Semicolon -o-
 *      TypeDefinition Semicolon -o-
 *
 *  AnnotatableDirective -t,o- (387)
 *      Attributes [no line break] AnnotatableDirective -t,o-
 *      VariableDefinition -allowin- Semicolon -o-
 *      FunctionDeclaration -t-
 *      NamespaceDefintion Semicolon -o-
 *      TypeDefinition Semicolon -o-
 *
 *  Input
 *      internal, intrinsic, private, protected, public
 *      final, native, override, prototype, static
 *      [ attribute AssignmentExpression
 *      Identifier
 *      const
 *      let
 *      var
 *      function
 *      interface
 *      namespace
 *      type
 *      module
 *      package
 *
 *  AST
 *      N_CLASS
 *      N_FUNCTION
 *      N_QNAME
 *      N_NAMESPACE
 *      N_VAR_DEFINITION
 *      N_MODULE??
 */
static EcNode *parseAnnotatableDirective(EcCompiler *cp, EcNode *attributes)
{
    EcState     *state;
    EcNode      *nextAttribute, *np;
    int         expectSemi;

    ENTER(cp);

    np = 0;
    expectSemi = 0;
    state = cp->state;

    switch (peekToken(cp)) {

    /* Attributes AnnotatableDirective */
    case T_STRING:
    case T_ATTRIBUTE:
    case T_RESERVED_NAMESPACE:
    case T_ID:
        nextAttribute = parseAttribute(cp);
        if (nextAttribute) {
            getToken(cp);
            putToken(cp);
            if (nextAttribute->lineNumber < cp->token->lineNumber) {
                /* Must be no line break after the attribute */
                return LEAVE(cp, unexpected(cp));
            }

            /*
             *  Aggregate the attributes and pass in. Must do this to allow "private static var a, b, c"
             */
            if (attributes) {
                nextAttribute->attributes |= attributes->attributes;
                if (attributes->qname.space && nextAttribute->qname.space) {
                    return LEAVE(cp, parseError(cp, "Can't define multiple namespaces for directive"));
                }
                if (attributes->qname.space) {
                    nextAttribute->qname.space = mprStrdup(nextAttribute, attributes->qname.space);
                }
            }

            np = parseAnnotatableDirective(cp, nextAttribute);
        }
        break;


    case T_CONST:
    case T_LET:
    case T_VAR:
        np = parseVariableDefinition(cp, attributes);
        expectSemi++;
        break;

    case T_FUNCTION:
        if (state->inInterface) {
            np = parseFunctionDeclaration(cp, attributes);
        } else {
            np = parseFunctionDefinition(cp, attributes);
        }
        break;

    case T_CLASS:
        if (state->inClass == 0) {
            /* Nested classes are not supported */
            np = parseClassDefinition(cp, attributes);
        } else {
            getToken(cp);
            np = unexpected(cp);
        }
        break;

    case T_INTERFACE:
        if (state->inClass == 0) {
            np = parseInterfaceDefinition(cp, attributes);
        } else {
            np = unexpected(cp);
        }
        break;

    case T_NAMESPACE:
        np = parseNamespaceDefinition(cp, attributes);
        expectSemi++;
        break;

    case T_TYPE:
        np = parseTypeDefinition(cp, attributes);
        expectSemi++;
        break;

    case T_MODULE:
        np = parseModuleDefinition(cp);
        break;

    default:
        getToken(cp);
        np = parseError(cp, "Unknown directive \"%s\"", cp->token->text);
    }

    if (np && expectSemi) {
        if (getToken(cp) != T_SEMICOLON) {
            if (np->lineNumber < cp->token->lineNumber || cp->token->tokenId == T_EOF) {
                putToken(cp);
            } else if (cp->token->tokenId != T_NOP) {
                np = unexpected(cp);
            }
        }
    }

    return LEAVE(cp, np);
}



/*
 *  Attribute -global- (391)
 *      NamespaceAttribute
 *      dynamic
 *      final
 *      native
 *      [ AssignmentExpression allowList,allowIn]
 *
 *  Attribute -class- (396)
 *      NamespaceAttribute
 *      final
 *      native
 *      override
 *      prototype
 *      static
 *      [ AssignmentExpression allowList,allowIn]
 *
 *  Attribute -interface- (419)
 *      NamespaceAttribute
 *
 *  Attribute -local- (420)
 *      NamespaceAttribute
 *
 *
 *  Input -common-
 *      NamespaceAttribute
 *          Path . Identifier
 *          Identifier
 *          public
 *          internal
 *      final
 *      native
 *      [
 *
 *  Input -global-
 *      NamespaceAttribute -global-
 *          intrinsic
 *      dynamic
 *
 *  Input -class-
 *      NamespaceAttribute -class-
 *          intrinsic
 *          private
 *          protected
 *      override
 *      prototype
 *      static
 *
 *  AST
 *      N_ATTRIBUTES
 *          attribute
 *              flags
 *          attributes
 */
static EcNode *parseAttribute(EcCompiler *cp)
{
    EcNode      *np;
    EcState     *state;
    int         inClass, subId;

    ENTER(cp);

    state = cp->state;
    np = 0;
    inClass = (cp->state->inClass) ? 1 : 0;

    if (state->currentFunctionNode /* || inInterface */) {
        np = parseNamespaceAttribute(cp);
        return LEAVE(cp, np);
    }

    peekToken(cp);
    subId = cp->peekToken->subId;
    switch (cp->peekToken->tokenId) {
    case T_ID:
    case T_RESERVED_NAMESPACE:
    case T_STRING:
        if (!inClass && (subId == T_PRIVATE || subId ==  T_PROTECTED)) {
            getToken(cp);
            return LEAVE(cp, parseError(cp, "Can't use private or protected in this context"));
        }
        np = parseNamespaceAttribute(cp);
        break;

    case T_ATTRIBUTE:
        getToken(cp);
        np = createNode(cp, N_ATTRIBUTES);
        switch (cp->token->subId) {
        case T_DYNAMIC:
            if (inClass) {
                np = unexpected(cp);
            } else {
                np->attributes |= EJS_ATTR_DYNAMIC_INSTANCE;
            }
            break;

        case T_FINAL:
            np->attributes |= EJS_ATTR_FINAL;
            break;

        case T_NATIVE:
            np->attributes |= EJS_ATTR_NATIVE;
            break;

        case T_OVERRIDE:
            if (inClass) {
                np->attributes |= EJS_ATTR_OVERRIDE;
            } else {
                np = unexpected(cp);
            }
            break;

#if ECMA
        case T_PROTOTYPE:
            if (inClass) {
                np->attributes |= EJS_ATTR_PROTOTYPE;
            } else {
                np = unexpected(cp);
            }
            break;
#endif

        case T_STATIC:
            if (inClass) {
                np->attributes |= EJS_ATTR_STATIC;
            } else {
                np = unexpected(cp);
            }
            break;

        case T_ENUMERABLE:
            if (inClass) {
                np->attributes |= EJS_ATTR_ENUMERABLE;
            } else {
                np = unexpected(cp);
            }
            break;

        default:
            np = parseError(cp, "Unknown or invalid attribute in this context %s", cp->token->text);
        }
        break;

    case T_LBRACKET:
        np = appendNode(np, parseAssignmentExpression(cp));
        break;

    default:
        np = parseError(cp, "Unknown or invalid attribute in this context %s", cp->token->text);
        break;
    }

    return LEAVE(cp, np);
}



/*
 *  NamespaceAttribute -global- (405)
 *      public
 *      internal
 *      intrinsic
 *      PrimaryName
 *
 *  NamespaceAttribute -class- (409)
 *      ReservedNamespace
 *      PrimaryName
 *
 *  Input -common-
 *      Identifier
 *      internal
 *      public
 *
 *  Input -global-
 *      intrinsic
 *
 *  Input -class-
 *      intrinsic
 *      private
 *      protected
 *
 *  AST
 *      N_ATTRIBUTES
 *          attribute
 *              flags
 *          left: N_QNAME | N_DOT
 */
static EcNode *parseNamespaceAttribute(EcCompiler *cp)
{
    EcNode      *np, *qualifier;
    int         inClass, subId;

    ENTER(cp);

    inClass = (cp->state->inClass) ? 1 : 0;

    peekToken(cp);
    subId = cp->peekToken->subId;

    np = createNode(cp, N_ATTRIBUTES);
    np->lineNumber = cp->peekToken->lineNumber;

    switch (cp->peekToken->tokenId) {
    case T_RESERVED_NAMESPACE:
        if (!inClass && (subId == T_PRIVATE || subId ==  T_PROTECTED)) {
            return LEAVE(cp, unexpected(cp));
        }
        qualifier = parseReservedNamespace(cp);
        np->attributes = qualifier->attributes;
        np->specialNamespace = qualifier->specialNamespace;
        np->qname.space = mprStrdup(np, qualifier->qname.space);
        break;

    case T_ID:
        qualifier = parsePrimaryName(cp);
        if (qualifier->kind == N_QNAME) {
            np->attributes = qualifier->attributes;
            np->qname.space = mprStrdup(np, qualifier->qname.name);
        } else {
            /*
             *  This is a N_DOT expression compile-time constant expression.
             */
            //  TODO - not debugged yet
            np->qualifierNode = linkNode(np, qualifier);
        }
        break;

    case T_STRING:
        getToken(cp);
        np->qname.space = mprStrdup(np, (char*) cp->token->text);
        np->literalNamespace = 1;
        break;

    case T_ATTRIBUTE:
        getToken(cp);
        np = parseError(cp, "Attribute \"%s\" not supported on local variables", cp->token->text);
        break;

    default:
        np = unexpected(cp);
        break;
    }

    return LEAVE(cp, np);
}



/*
 *  VariableDefinition -b- (411)
 *      VariableDefinitionKind VariableBindingList -allowList,b-
 *
 *  Input
 *      const
 *      let
 *      let const
 *      var
 *
 *  AST
 *      N_VAR_DEFINITION
 *          def: varKind
 */
static EcNode *parseVariableDefinition(EcCompiler *cp, EcNode *attributes)
{
    EcNode      *np;

    ENTER(cp);

    np = parseVariableDefinitionKind(cp, attributes);
    np = parseVariableBindingList(cp, np, attributes);

    return LEAVE(cp, np);
}



/*
 *  VariableDefinitionKind (412)
 *      const
 *      let
 *      let const
 *      var
 *
 *  Input
 *
 *  AST
 *      N_VAR_DEFINITION
 *          def: varKind
 *
 */
static EcNode *parseVariableDefinitionKind(EcCompiler *cp, EcNode *attributes)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_VAR_DEFINITION);
    setNodeDoc(cp, np);

    switch (getToken(cp)) {
    case T_CONST:
        np->def.varKind = KIND_CONST;
        np->attributes |= EJS_ATTR_CONST;
        break;

    case T_LET:
        if (attributes && attributes->attributes & EJS_ATTR_STATIC) {
            np = parseError(cp, "Static and let are not a valid combination. Use var instead.");

        } else {
            np->def.varKind = KIND_LET;
            if (peekToken(cp) == T_CONST) {
                np->def.varKind |= KIND_CONST;
            }
        }
        break;

    case T_VAR:
        np->def.varKind = KIND_VAR;
        break;

    default:
        np = parseError(cp, "Bad variable definition kind");
    }

    return LEAVE(cp, np);
}



/*
 *  VariableBindingList -a,b- (416)
 *      VariableBinding
 *      VariableBindingList -noList,b- , VariableBinding -a,b-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseVariableBindingList(EcCompiler *cp, EcNode *varList, EcNode *attributes)
{
    ENTER(cp);

    varList = appendNode(varList, parseVariableBinding(cp, varList, attributes));

    while (peekToken(cp) == T_COMMA) {
        getToken(cp);
        varList = appendNode(varList, parseVariableBinding(cp, varList, attributes));
    }

    return LEAVE(cp, varList);
}




/*
 *  VariableBinding -a,b- (418)
 *      TypedIdentifier (258)
 *      TypedPattern (260) -noList,noIn- VariableInitialisation -a,b-
 *
 *  TypedIdentifier (258)
 *      SimplePattern -noList,noin,noExpr-
 *      SimplePattern -a,b,noExpr- : TypeExpression
 *
 *  TypedPattern (260)
 *      SimplePattern -a,b,noExpr-
 *      SimplePattern -a,b,noExpr- : NullableTypeExpression
 *      ObjectPattern -noExpr-
 *      ObjectPattern -noExpr- : TypeExpression
 *      ArrayPattern -noExpr-
 *      ArrayPattern -noExpr- : TypeExpression
 *
 *  Input
 *      TODO
 *
 *  AST
 *      N_QNAME variableId
 *      N_ASSIGN
 *          left: N_QNAME variableId
 *          right: N_LITERAL (TODO more here)
 *
 */
static EcNode *parseVariableBinding(EcCompiler *cp, EcNode *varList, EcNode *attributes)
{
    EcNode      *np;

    ENTER(cp);

    np = 0;

    switch (peekToken(cp)) {
    case T_LBRACKET:
        //  TODO
        break;

    case T_LBRACE:
        //  TODO
        break;

    default:
        np = parseTypedIdentifier(cp);
        if (np == 0) {
            return LEAVE(cp, np);
        }
        if (np->kind != N_QNAME) {
            return LEAVE(cp, parseError(cp, "Bad variable name"));
        }
        np->attributes = varList->attributes;
        applyAttributes(cp, np, attributes, 0);
        copyDocString(cp, np, varList);

        if (STRICT_MODE(cp)) {
            if (np->typeNode == 0) {
                parseError(cp, "Variable untyped. Variables must be typed when declared in strict mode");
                np = ecResetError(cp, np, 0);
                /* Keep parsing */
            }
        }
        if (peekToken(cp) == T_ASSIGN) {
            np = createAssignNode(cp, np, parseVariableInitialisation(cp));
        }

        break;
    }

    return LEAVE(cp, np);
}



/*
 *  VariableInitialisation -a,b- (426)
 *      = AssignmentExpression -a,b-
 *
 *  Input
 *      =
 *
 *  AST
 *      N_EXPRESSIONS
 */
static EcNode *parseVariableInitialisation(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) == T_ASSIGN) {
        np = parseAssignmentExpression(cp);

    } else {
        np = unexpected(cp);
    }

    return LEAVE(cp, np);
}



/*
 *  FunctionDeclaration (421)                                   # For interfaces
 *      function FunctionName FunctionSignature
 *
 *  Notes:
 *      This is for function declarations in interfaces only.
 *
 *  Input
 *      function
 *
 *  AST
 *      N_FUNCTION
 *          function: name, getter, setter, block,
 *              children: parameters
 */
static EcNode *parseFunctionDeclaration(EcCompiler *cp, EcNode *attributes)
{
    EcNode      *np;
    int         tid;

    ENTER(cp);

    cp->state->defaultNamespace = NULL;

    if (getToken(cp) != T_FUNCTION) {
        return LEAVE(cp, parseError(cp, "Expecting \"function\""));
    }

    tid = peekToken(cp);
    if (tid != T_ID && tid != T_GET && tid != T_SET) {
        getToken(cp);
        return LEAVE(cp, parseError(cp, "Expecting function or class name"));
    }

    np = parseFunctionName(cp);
    if (np) {
        setNodeDoc(cp, np);
        applyAttributes(cp, np, attributes, 0);
        np = parseFunctionSignature(cp, np);
        if (np) {
            np->function.isMethod = 1;
            if (STRICT_MODE(cp)) {
                if (np->function.resultType == 0) {
                    np = parseError(cp, 
                        "Function has not defined a return type. Fuctions must be typed when declared in strict mode");
                }
            }
        }
    }

    return LEAVE(cp, np);
}



/*
 *  FunctionDefinition -class- (424)
 *      function ClassName ConstructorSignature FunctionBody -allowin-
 *      function FunctionName FunctionSignature FunctionBody -allowin-
 *
 *  TODO - should this be a different production? YES
 *  FunctionDeclaration -t- (442)
 *      function FunctionName FunctionSignature FunctionBody -allowin-
 *      let function FunctionName FunctionSignature FunctionBody -allowin-
 *      const function FunctionName FunctionSignature FunctionBody -allowin-
 *
 *  Input
 *      function
 *      let
 *      const
 *
 *  AST N_FUNCTION
 *      function: name, getter, setter, block,
 *          children: parameters
 */
static EcNode *parseFunctionDefinition(EcCompiler *cp, EcNode *attributeNode)
{
    EcNode      *np, *className;
    EcState     *state;

    ENTER(cp);

    state = cp->state;
    state->defaultNamespace = NULL;

    if (getToken(cp) != T_FUNCTION) {
        return LEAVE(cp, parseError(cp, "Expecting \"function\""));
    }

    if (getToken(cp) != T_ID && !(cp->token->groupMask & (G_CONREV | G_OPERATOR))) {
        return LEAVE(cp, parseError(cp, "Expecting function or class name"));
    }

    //  TODO - improve error handling on all this

    if (cp->state->currentClassName.name && strcmp(cp->state->currentClassName.name, (char*) cp->token->text) == 0) {
        /*
         *  Constructor
         */
        putToken(cp);
        np = createNode(cp, N_FUNCTION);
        setNodeDoc(cp, np);
        applyAttributes(cp, np, attributeNode, EJS_PUBLIC_NAMESPACE);
        className = parseClassName(cp);

        if (className) {
            np->qname.name = mprStrdup(np, className->qname.name);
            np->function.isConstructor = 1;
            cp->state->currentClassNode->klass.constructor = np;
        }

        if (np) {
            np = parseConstructorSignature(cp, np);
            if (np) {
                cp->state->currentFunctionNode = np;
                if (!(np->attributes & EJS_ATTR_NATIVE)) {
                    np->function.body = linkNode(np, parseFunctionBody(cp, np));
                    mprStealBlock(np, np->function.body);
                    if (np->function.body == 0) {
                        return LEAVE(cp, 0);
                    }
                }
                np->function.isMethod = 1;
            }
        }

    } else {
        putToken(cp);
        np = parseFunctionName(cp);
        if (np) {
            setNodeDoc(cp, np);
            applyAttributes(cp, np, attributeNode, 0);
            np = parseFunctionSignature(cp, np);
            if (np) {
                cp->state->currentFunctionNode = np;
                if (attributeNode && (attributeNode->attributes & EJS_ATTR_NATIVE)) {
                    if (peekToken(cp) == T_LBRACE) {
                        return LEAVE(cp, parseError(cp, "Native functions declarations must not have bodies"));
                    }

                } else {
#if UNUSED
                    if (peekToken(cp) != T_LBRACE) {
                        np->function.noBlock = 1;
                    }
#endif
                    np->function.body = linkNode(np, parseFunctionBody(cp, np));
                    if (np->function.body == 0) {
                        return LEAVE(cp, 0);
                    }
                }
                if (state->inClass && !state->inFunction && 
                        cp->classState->blockNestCount == (cp->state->blockNestCount - 1)) {
                    np->function.isMethod = 1;
                }
            }

            if (STRICT_MODE(cp)) {
                if (np->function.resultType == 0) {
                    parseError(cp, "Function has not defined a return type. Functions must be typed in strict mode");
                    np = ecResetError(cp, np, 0);
                    /* Keep parsing */
                }
            }
        }
    }

    return LEAVE(cp, np);
}



/*
 *  FunctionName (427)
 *      Identifier
 *      OverloadedOperator
 *      get Identifier
 *      set Identifier
 *
 *  Input
 *      Identifier
 *      get
 *      set
 *      + - ~ * / % < > <= >= == << >> >>> & | === != !==
 *
 *  AST N_FUNCTION
 *      function: name, getter, setter
 */
static EcNode *parseFunctionName(EcCompiler *cp)
{
    EcNode      *name, *np;
    int         accessorId, tid;

    ENTER(cp);

    tid = peekToken(cp);
    if (tid != T_GET && tid != T_SET) {
        if (cp->peekToken->groupMask & G_CONREV) {
            tid = T_ID;
        }
    }

    switch (tid) {
    case T_GET:
    case T_SET:
    case T_DELETE:
        getToken(cp);
        accessorId = cp->token->tokenId;

        tid = peekToken(cp);
        if (cp->peekToken->groupMask & G_CONREV) {
            tid = T_ID;
        }
        if (tid == T_LPAREN) {
            /*
             *  Function is called get() or set(). So put back the name and fall through to T_ID
             */
            putToken(cp);

        } else {
            if (tid != T_ID) {
                getToken(cp);
                return LEAVE(cp, parseError(cp, "Expecting identifier"));
            }
            name = parseIdentifier(cp);
            np = createNode(cp, N_FUNCTION);
            if (accessorId == T_GET) {
                np->function.getter = 1;
                np->qname.name = mprStrdup(np, name->qname.name);
                np->attributes |= EJS_ATTR_GETTER;
            } else {
                np->function.setter = 1;
                np->attributes |= EJS_ATTR_SETTER;
                mprAllocSprintf(np, (char**) &np->qname.name, -1, "set-%s", name->qname.name);
            }
            break;
        }
        /* Fall through */

    case T_ID:
        name = parseIdentifier(cp);
        np = createNode(cp, N_FUNCTION);
        np->qname.name = mprStrdup(np, name->qname.name);
        break;

    default:
        getToken(cp);
        if (cp->token->groupMask == G_OPERATOR) {
            putToken(cp);
            np = parseOverloadedOperator(cp);
        } else {
            np = unexpected(cp);
        }
    }

    return LEAVE(cp, np);
}


/*
 *  OverloadedOperator (431)
 *      + - ~ * / % < > <= >= == << >> >>> & | === != !==
 *
 *  Input
 *      + - ~ * / % < > <= >= == << >> >>> & | === != !==
 *      [ . (  =        EJS exceptions
 *
 *  AST
 *      N_QNAME
 */
static EcNode *parseOverloadedOperator(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    switch (getToken(cp)) {
    /*
     *  EJS extensions
     */
    case T_LBRACKET:
    case T_LPAREN:
    case T_DOT:
    case T_ASSIGN:
        /* Fall through */

    case T_PLUS:
    case T_MINUS:
    case T_TILDE:
    case T_MUL:
    case T_DIV:
    case T_MOD:
    case T_LT:
    case T_GT:
    case T_LE:
    case T_GE:
    case T_EQ:
    case T_LSH:
    case T_RSH:
    case T_RSH_ZERO:
    case T_BIT_AND:
    case T_BIT_OR:
    case T_STRICT_EQ:
    case T_NE:
    case T_STRICT_NE:
        /* Node holds the token */
        np = createNode(cp, N_FUNCTION);
        np->qname.name = mprStrdup(np, (char*) cp->token->text);
        break;

    default:
        np = unexpected(cp);
        break;
    }

    return LEAVE(cp, np);
}



/*
 *  FunctionSignature (450) (See also FunctionSignatureType)
 *      TypeParameters ( Parameters ) ResultType
 *      TypeParameters ( this : PrimaryName ) ResultType
 *      TypeParameters ( this : PrimaryName , NonemptyParameters )
 *              ResultType
 *
 *  Input
 *      .< TypeParameterList >
 *
 *  AST
 */
static EcNode *parseFunctionSignature(EcCompiler *cp, EcNode *np)
{
    EcNode      *parameters;

    if (np == 0) {
        return np;
    }

    ENTER(cp);

    mprAssert(np->kind == N_FUNCTION);

    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, parseError(cp, "Expecting \"(\""));
    }

    np->function.parameters = linkNode(np, createNode(cp, N_ARGS));

    if (peekToken(cp) == T_ID || cp->peekToken->tokenId == T_ELIPSIS) {
        if (strcmp((char*) cp->peekToken->text, "this") == 0) {
            //  TODO
        } else {
            parameters = parseParameters(cp, np->function.parameters);
            if (parameters == 0) {
                while (getToken(cp) != T_RPAREN && cp->token->tokenId != T_EOF);
                return LEAVE(cp, 0);
            }
            np->function.parameters = linkNode(np, parameters);
        }
    }
    if (getToken(cp) != T_RPAREN) {
        return LEAVE(cp, parseError(cp, "Expecting \")\""));
    }

    if (np) {
        if (peekToken(cp) == T_COLON) {
            np->function.resultType = linkNode(np, parseResultType(cp));
        }
    }

    return LEAVE(cp, np);
}



#if UNUSED
/*
 *  TypeParameters (453)
 *      EMPTY
 *      .< TypeParameterList >
 *  Input
 *
 *  AST
 */
static EcNode *parseTypeParameters(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  TypeParametersList (455)
 *      Identifier
 *      Identifier , TypeParameterList
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseTypeParameterList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



/*
 *  Parameters (457)
 *      EMPTY
 *      NonemptyParameters
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseParameters(EcCompiler *cp, EcNode *args)
{
    ENTER(cp);

    if (peekToken(cp) != T_RPAREN) {
        args = parseNonemptyParameters(cp, args);
    }

    return LEAVE(cp, args);
}



/*
 *  NonemptyParameters (459)
 *      ParameterInit
 *      ParameterInit , NonemptyParameters
 *      RestParameter
 *
 *  Input
 *      Identifier
 *      ...
 *
 *  AST
 *      N_ARGS
 *          N_VAR_DEFN
 *              N_QNAME
 *              N_ASSIGN_OP
 *                  N_QNAME, N_LITERAL
 *
 */
static EcNode *parseNonemptyParameters(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (peekToken(cp) == T_ELIPSIS) {
        np = appendNode(np, parseRestParameter(cp));

    } else {
        np = appendNode(np, parseParameterInit(cp, np));
        if (np) {
            if (peekToken(cp) == T_COMMA) {
                getToken(cp);
                np = parseNonemptyParameters(cp, np);
            }
        }
    }
    return LEAVE(cp, np);
}



/*
 *  ParameterInit (462)
 *      Parameter
 *      Parameter = NonAssignmentExpression -noList,allowIn-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseParameterInit(EcCompiler *cp, EcNode *args)
{
    EcNode      *np, *assignOp, *lastArg;

    ENTER(cp);

    np = parseParameter(cp, 0);

    if (peekToken(cp) == T_ASSIGN) {
        getToken(cp);
        /*
         *  Insert a N_ASSIGN_OP node under the VAR_DEFN
         */
        assignOp = createNode(cp, N_ASSIGN_OP);
        assignOp = appendNode(assignOp, np->left);
        mprAssert(mprGetListCount(np->children) == 1);
        mprRemoveItem(np->children, mprGetItem(np->children, 0));
        assignOp = appendNode(assignOp, parseNonAssignmentExpression(cp));
        np = appendNode(np, assignOp);

        if (assignOp) {
            appendDocString(cp, args->parent, assignOp->left, assignOp->right);
        }

    } else if (args->children) {
        lastArg = (EcNode*) mprGetLastItem(args->children);
        if (lastArg && lastArg->left->kind == N_ASSIGN_OP) {
            np = parseError(cp, "Cannot have required parameters after parameters with initializers");
        }
    }

    return LEAVE(cp, np);
}



/*
 *  Parameter (464)
 *      ParameterKind TypedPattern -noList,noIn-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseParameter(EcCompiler *cp, bool rest)
{
    EcNode      *np, *parameter;

    ENTER(cp);

    np = parseParameterKind(cp);
    parameter = parseTypedPattern(cp);
    np = appendNode(np, parameter);

    if (parameter) {
        if (STRICT_MODE(cp)) {
            if (parameter->typeNode == 0 && !rest) {
                parseError(cp, "Parameter untyped. Parameters must be typed when declared in strict mode.");
                np = ecResetError(cp, np, 0);
                /* Keep parsing */
            }
        }
    }

    return LEAVE(cp, np);
}



/*
 *  ParameterKind (465)
 *      EMPTY
 *      const
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseParameterKind(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_VAR_DEFINITION);

    if (peekToken(cp) == T_CONST) {
        getToken(cp);
        np->def.varKind = KIND_CONST;
    }

    return LEAVE(cp, np);
}




/*
 *  RestParameter (467)
 *      ...
 *      ... Parameter
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseRestParameter(EcCompiler *cp)
{
    EcNode      *np, *varNode;

    ENTER(cp);

    if (getToken(cp) == T_ELIPSIS) {
        np = parseParameter(cp, 1);
        if (np && np->left) {
            if (np->left->kind == N_QNAME) {
                varNode = np->left;
            } else if (np->left->kind == N_ASSIGN_OP) {
                varNode = np->left->left;
            } else {
                varNode = 0;
                mprAssert(0);
            }
            if (varNode) {
                mprAssert(varNode->kind == N_QNAME);
                varNode->name.isRest = 1;
            }
        }

    } else {
        np = unexpected(cp);
    }

    return LEAVE(cp, np);
}



/*
 *  ResultType (469)
 *      EMPTY
 *      : void
 *      : NullableTypeExpression
 *
 *  Input
 *
 *  AST
 *      N_DOT
 *      N_QNAME
 *      N_VOID
 *
 *  NOTE: we do not handle EMPTY here. Caller must handle.
 */
static EcNode *parseResultType(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) == T_COLON) {
        getToken(cp);
        if (peekToken(cp) == T_VOID) {
            getToken(cp);
            np = createNode(cp, N_QNAME);
            setId(np, "Void");
            np->name.isType = 1;

        } else {
            np = parseNullableTypeExpression(cp);
        }

    } else {
        /*  Don't handle EMPTY here */
        mprAssert(0);
        np = unexpected(cp);;
    }
    return LEAVE(cp, np);
}



/*
 *  ConstructorSignature (472)
 *      TypeParameters ( Parameters )
 *      TypeParameters ( Parameters ) : ConstructorInitializer
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseConstructorSignature(EcCompiler *cp, EcNode *np)
{
    if (np == 0) {
        return np;
    }

    ENTER(cp);

    mprAssert(np->kind == N_FUNCTION);

    /*
     *  TODO - not implementing TypeParameters
     */
    if (getToken(cp) != T_LPAREN) {
        return LEAVE(cp, parseError(cp, "Expecting \"(\""));
    }

    np->function.parameters = linkNode(np, createNode(cp, N_ARGS));
    np->function.parameters =
        linkNode(np, parseParameters(cp, np->function.parameters));

    if (getToken(cp) != T_RPAREN) {
        return LEAVE(cp, parseError(cp, "Expecting \")\""));
    }

    if (np) {
        if (peekToken(cp) == T_COLON) {
            getToken(cp);
            np->function.constructorSettings = linkNode(np, parseConstructorInitializer(cp));
            // mprStealBlock(np, np->function.settings);
        }
    }

    return LEAVE(cp, np);
}



/*
 *  ConstructorInitializer (462)
 *      InitializerList
 *      InitializerList SuperInitializer
 *      SuperInitializer
 *
 *  Input
 *      TDB
 *      super
 *
 *  AST
 *      N_DIRECTIVES
 */
static EcNode *parseConstructorInitializer(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_DIRECTIVES);

    if (peekToken(cp) != T_SUPER) {
        np = parseInitializerList(cp, np);
    }

    if (peekToken(cp) == T_SUPER) {
        np = appendNode(np, parseSuperInitializer(cp));
    }

    return LEAVE(cp, np);
}



/*
 *  InitializerList (465)
 *      Initializer
 *      InitializerList , Initializer
 *
 *  Input
 *      TBD
 *
 *  AST
 *      N_DIRECTIVES
 */
static EcNode *parseInitializerList(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    mprAssert(np && np->kind == N_DIRECTIVES);

    while (1) {
        np = appendNode(np, parseInitializer(cp));
        if (peekToken(cp) == T_COMMA) {
            getToken(cp);
        } else {
            break;
        }
    }

    return LEAVE(cp, np);
}



/*
 *  Initializer (467)
 *      Pattern -noList,noIn,noExpr- VariableInitialisation -nolist,allowIn-
 *
 *  Input
 *      TBD
 *
 *  AST
 *      N_ASSIGN
 */
static EcNode *parseInitializer(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parsePattern(cp);

    if (peekToken(cp) != T_ASSIGN) {
        return LEAVE(cp, expected(cp, "="));
    }

    np = createAssignNode(cp, np, parseVariableInitialisation(cp));

    return LEAVE(cp, np);
}



/*
 *  SuperInitializer (481)
 *      super Arguments
 *
 *  Input
 *      super
 *
 *  AST
 *      N_SUPER
 */
static EcNode *parseSuperInitializer(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (getToken(cp) != T_SUPER) {
        return LEAVE(cp, expected(cp, "super"));
    }
    np = createNode(cp, N_SUPER);

    np = appendNode(np, parseArguments(cp));
    return LEAVE(cp, np);
}



/*
 *  FunctionBody -b- (469)
 *      Block -local-
 *      AssignmentExpression -b-
 *
 *  Input
 *      {
 *      (
 *
 *  AST
 */
static EcNode *parseFunctionBody(EcCompiler *cp, EcNode *fun)
{
    EcNode      *np, *end, *ret;

    ENTER(cp);

    cp->state->inFunction = 1;
    cp->state->namespace = EJS_PRIVATE_NAMESPACE;

    if (peekToken(cp) == T_LBRACE) {
        np = parseBlock(cp);
        if (np) {
            np = np->left;
        }

    } else {
        np = createNode(cp, N_DIRECTIVES);
        ret = createNode(cp, N_RETURN);
        ret = appendNode(ret, parseAssignmentExpression(cp));
        np = appendNode(np, ret);
    }

    if (np) {
        end = createNode(cp, N_END_FUNCTION);
#if UNUSED
        end->endfunction.function = fun;
#endif
        np = appendNode(np, end);
    }

    return LEAVE(cp, np);
}




/*
 *  ClassDefinition (484)
 *      class ClassName ClassInheritance ClassBody
 *
 *  Input
 *      class id ...
 *
 *  AST
 *      N_CLASS
 *          name
 *              id
 *          extends: id
 *
 */
static EcNode *parseClassDefinition(EcCompiler *cp, EcNode *attributeNode)
{
    EcState     *state;
    EcNode      *np, *classNameNode, *inheritance, *constructor;
    int         tid;

    ENTER(cp);

    state = cp->state;

    if (getToken(cp) != T_CLASS) {
        return LEAVE(cp, expected(cp, "class"));
    }

    if (peekToken(cp) != T_ID) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "identifier"));
    }

    np = createNode(cp, N_CLASS);
    state->currentClassNode = np;
    state->topVarBlockNode = np;
    cp->classState = state;
    state->defaultNamespace = NULL;

    classNameNode = parseClassName(cp);
    if (classNameNode == 0) {
        return LEAVE(cp, 0);
    }

    applyAttributes(cp, np, attributeNode, 0);
    setNodeDoc(cp, np);

    //  TODO - what about namespace
    np->qname.name = mprStrdup(np, classNameNode->qname.name);
    state->currentClassName = np->qname;
    state->inClass = 1;

    tid = peekToken(cp);
    if (tid == T_EXTENDS || tid == T_IMPLEMENTS) {
        inheritance = parseClassInheritance(cp);
        if (inheritance->klass.extends) {
            np->klass.extends = mprStrdup(np, inheritance->klass.extends);
        }
        if (inheritance->klass.implements) {
            np->klass.implements = inheritance->klass.implements;
            mprStealBlock(np, np->klass.implements);
        }
    }

    if (peekToken(cp) != T_LBRACE) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "{"));
    }

    np = appendNode(np, parseClassBody(cp));

    if (np && np->klass.constructor == 0) {
        /*
         *  Create a default constructor because the user did not supply a constructor. We always
         *  create a constructor node even if one is not required (or generated). This makes binding easier later.
         */
        constructor = createNode(cp, N_FUNCTION);
        np->klass.constructor = linkNode(np, constructor);
        constructor->qname.name = mprStrdup(np, np->qname.name);
        applyAttributes(cp, constructor, 0, EJS_PUBLIC_NAMESPACE);
        constructor->function.isConstructor = 1;
        constructor->function.isDefaultConstructor = 1;
    }

    return LEAVE(cp, np);
}



/*
 *  ClassName (485)
 *      ParameterisedTypeName
 *      ParameterisedTypeName !
 *  Input
 *
 *  AST
 */
static EcNode *parseClassName(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = parseParameterisedTypeName(cp);
    if (peekToken(cp) == T_LOGICAL_NOT) {
        getToken(cp);
        //  TODO - Merge in AST data.  How to represent "!"
    }
    return LEAVE(cp, np);
}



/*
 *  ParameterisedTypeName (487)
 *      Identifier
 *      Identifier TypeParameters
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseParameterisedTypeName(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) != T_ID) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "identifier"));
    }
    np = parseIdentifier(cp);

#if FUTURE
    if (peekToken(cp) == T_DOT_LESS) {
        np = parseTypeParameters(cp);
        //  TODO - Merge AST results
    }
#endif

    return LEAVE(cp, np);
}



/*
 *  ClassInheritance (489)
 *      EMPTY
 *      extends PrimaryName
 *      implements TypeIdentifierList
 *      extends PrimaryName implements TypeIdentifierList
 *
 *  Input
 *
 *  AST N_CLASS
 *      extends: id
 *      left: implements list if ids
 */
static EcNode *parseClassInheritance(EcCompiler *cp)
{
    EcNode      *np, *id;

    ENTER(cp);

    np = createNode(cp, N_CLASS);

    switch (getToken(cp)) {
    case T_EXTENDS:
        id = parsePrimaryName(cp);
        if (id) {
            np->klass.extends = mprStrdup(np, id->qname.name);
        }

        if (peekToken(cp) == T_IMPLEMENTS) {
            getToken(cp);
            np->klass.implements = linkNode(np, parseTypeIdentifierList(cp));
            mprStealBlock(np, np->klass.implements);
        }
        break;

    case T_IMPLEMENTS:
        np->klass.implements = linkNode(np, parseTypeIdentifierList(cp));
        mprStealBlock(np, np->klass.implements);
        break;

    default:
        putToken(cp);
        break;
    }
    return LEAVE(cp, np);
}




/*
 *  TypeIdentifierList (493)
 *      PrimaryName
 *      PrimaryName , TypeIdentifierList
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseTypeIdentifierList(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    np = createNode(cp, N_TYPE_IDENTIFIERS);
    while (peekToken(cp) == T_ID) {
        np = appendNode(np, parsePrimaryName(cp));
        if (peekToken(cp) != T_COMMA) {
            break;
        }
        getToken(cp);
    }

    /*
     *  Discard the first NOP node
     */
    return LEAVE(cp, np);
}



/*
 *  ClassBody (495)
 *      Block -class-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseClassBody(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) != T_LBRACE) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "class body { }"));
    }

    np = parseBlock(cp);
    if (np) {
        np = np->left;
        mprAssert(np->kind == N_DIRECTIVES);
    }

    return LEAVE(cp, np);
}



/*
 *  InterfaceDefinition (496)
 *      interface ClassName InterfaceInheritance InterfaceBody
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseInterfaceDefinition(EcCompiler *cp, EcNode *attributeNode)
{
    EcState     *state;
    EcNode      *np, *classNameNode, *inheritance;

    ENTER(cp);
    
    state = cp->state;

    //  TODO - incomplete

    np = 0;
    if (getToken(cp) != T_INTERFACE) {
        return LEAVE(cp, expected(cp, "interface"));
    }
    
    if (peekToken(cp) != T_ID) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "identifier"));
    }

    np = createNode(cp, N_CLASS);
    state->currentClassNode = np;
    state->topVarBlockNode = np;
    cp->classState = state;
    state->defaultNamespace = NULL;
    
    classNameNode = parseClassName(cp);
    if (classNameNode == 0) {
        return LEAVE(cp, 0);
    }

    applyAttributes(cp, np, attributeNode, 0);
    setNodeDoc(cp, np);
    
    //  TODO - what about namespace?
    np->qname.name = mprStrdup(np, classNameNode->qname.name);
    np->klass.isInterface = 1;
    state->currentClassName.name = np->qname.name;
    //  TODO - is this used? I think not.
    state->inInterface = 1;
    
    if (peekToken(cp) == T_EXTENDS) {
        inheritance = parseInterfaceInheritance(cp);
        if (inheritance->klass.extends) {
            np->klass.extends = mprStrdup(np, inheritance->klass.extends);
        }
    }

    if (peekToken(cp) != T_LBRACE) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "{"));
    }

    np = appendNode(np, parseInterfaceBody(cp));

    return LEAVE(cp, np);
}



/*
 *  InterfaceInheritance (497)
 *      EMPTY
 *      extends TypeIdentifierList
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseInterfaceInheritance(EcCompiler *cp)
{
    EcNode      *np, *id;

    ENTER(cp);

    np = createNode(cp, N_CLASS);

    if (peekToken(cp) == T_EXTENDS) {
        id = parseTypeIdentifierList(cp);
        if (id) {
            np->klass.extends = mprStrdup(np, id->qname.name);
        }
    }
    return LEAVE(cp, np);
}



/*
 *  InterfaceBody (499)
 *      Block -interface-
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseInterfaceBody(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);

    if (peekToken(cp) != T_LBRACE) {
        getToken(cp);
        return LEAVE(cp, expected(cp, "interface body { }"));
    }

//    cp->state->namespace = EJS_PUBLIC_NAMESPACE;
    
    np = parseBlock(cp);
    if (np) {
        np = np->left;
        mprAssert(np->kind == N_DIRECTIVES);
    }

    return LEAVE(cp, np);
}



/*
 *  NamespaceDefinition (500)
 *      namespace Identifier NamespaceInitialisation
 *
 *  Input
 *      namespace
 *
 *  AST
 *      N_NAMESPACE
 */
static EcNode *parseNamespaceDefinition(EcCompiler *cp, EcNode *attributeNode)
{
    EcNode      *varDefNode, *assignNode, *nameNode, *typeNode, *namespaceNode;
    EjsVar      *vp;

    ENTER(cp);

    if (getToken(cp) != T_NAMESPACE) {
        return LEAVE(cp, unexpected(cp));
    }

    /*
     *  Handle namespace definitions like:
     *      let NAME : Namespace = NAMESPACE_LITERAL
     */
    nameNode = parseIdentifier(cp);
    nameNode->name.isNamespace = 1;
    setNodeDoc(cp, nameNode);

    /*
     *  Hand-craft a "Namespace" type node
     */
    typeNode = createNode(cp, N_QNAME);
    typeNode->qname.name = "Namespace";
    nameNode->typeNode = linkNode(nameNode, typeNode);
    applyAttributes(cp, nameNode, attributeNode, 0);

    if (peekToken(cp) == T_ASSIGN) {
        namespaceNode = parseNamespaceInitialisation(cp, nameNode);

    } else {
        /*
         *  Create a namespace literal node from which to assign.
         */
        namespaceNode = createNode(cp, N_LITERAL);
        vp = (EjsVar*) ejsCreateNamespace(cp->ejs, nameNode->qname.name, nameNode->qname.name);
        namespaceNode->literal.var = vp;
        nameNode->name.value = vp;
        mprStealBlock(namespaceNode, vp);
    }

    assignNode = createAssignNode(cp, nameNode, namespaceNode);

    varDefNode = createNode(cp, N_VAR_DEFINITION);
//  TODO - temporarily set this to VAR. Fix when block scope is working properly.
//  varDefNode->def.varKind = KIND_LET;
    varDefNode->def.varKind = KIND_VAR;

    varDefNode = appendNode(varDefNode, assignNode);

    return LEAVE(cp, varDefNode);
}


/*
 *  NamespaceInitialisation (501)
 *      EMPTY
 *      = StringLiteral
 *      = SimpleQualifiedName
 *
 *  Input
 *      =
 *
 *  AST
 *      N_LITERAL
 *      N_QNAME
 *      N_DOT
 */
static EcNode *parseNamespaceInitialisation(EcCompiler *cp, EcNode *nameNode)
{
    EcNode      *np;
    EjsVar      *vp;

    ENTER(cp);

    if (getToken(cp) != T_ASSIGN) {
        return LEAVE(cp, unexpected(cp));
    }

    if (peekToken(cp) == T_STRING) {
        getToken(cp);
        np = createNode(cp, N_LITERAL);
        vp = (EjsVar*) ejsCreateNamespace(cp->ejs, nameNode->qname.name, mprStrdup(np, (char*) cp->token->text));
        np->literal.var = vp;
        mprStealBlock(np, vp);

    } else {
        np = parsePrimaryName(cp);
    }

    return LEAVE(cp, np);
}



/*
 *  TypeDefinition (504)
 *      type ParameterisedTypeName TypeInitialisation
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseTypeDefinition(EcCompiler *cp, EcNode *attributeNode)
{
    EcNode      *np;

    ENTER(cp);
    //  TODO
    np = parseTypeInitialisation(cp);
    return LEAVE(cp, np);
}



/*
 *  TypeInitialisation (505)
 *      = NullableTypeExpression
 *
 *  Input
 *
 *  AST
 */
static EcNode *parseTypeInitialisation(EcCompiler *cp)
{
    EcNode      *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



/*
 *  ModuleDefinition (493)
 *      module ModuleBody
 *      module ModuleName ModuleBody
 *
 *  Input
 *      module ...
 *
 *  AST
 *      N_MODULE
 */
static EcNode *parseModuleDefinition(EcCompiler *cp)
{
    EcNode      *np, *moduleName, *body;
    cchar       *name, *namespace;
    int         next, isDefault, pos;

    ENTER(cp);

    if (getToken(cp) != T_MODULE) {
        return LEAVE(cp, unexpected(cp));
    }

    if (peekToken(cp) == T_LBRACE) {
        /*
         *  No module name. Set the namespace to the unique internal namespace name.
         */
        np = createNode(cp, N_MODULE);
        np->module.name = mprStrdup(np, EJS_DEFAULT_MODULE);
        namespace = cp->fileState->namespace;
        isDefault = 1;

    } else {
        moduleName = parseModuleName(cp);
        if (moduleName == 0) {
            return LEAVE(cp, 0);
        }
        namespace = moduleName->qname.name;
        isDefault = 0;
        np = createNode(cp, N_MODULE);
        np->module.name = mprStrdup(np, namespace);
    }
    np->qname.name = np->module.name;
    cp->state->defaultNamespace = namespace;

    body = parseModuleBody(cp);
    if (body == 0) {
        return LEAVE(cp, 0);
    }
    
    /* 
     *  Append the module namespace and also modules provided via ec/ejs --use switch
     */
    pos = 0;
    if (!isDefault) {
        body = insertNode(body, createNamespaceNode(cp, cp->fileState->namespace, 0, 1), pos++);
    }
    body = insertNode(body, createNamespaceNode(cp, namespace, 1, 1), pos++);
    for (next = 0; (name = mprGetNextItem(cp->useModules, &next)) != 0; ) {
        body = insertNode(body, createNamespaceNode(cp, name, 0, 1), pos++);
    }
    
    mprAssert(body->kind == N_BLOCK);
    np = appendNode(np, body);

    return LEAVE(cp, np);
}



/*
 *  ModuleName (494)
 *      Identifier
 *      ModuleName . Identifier
 *
 *  Input
 *      ID
 *      ID. ... .ID
 *
 *  AST
 *      N_QNAME
 *          name: name
 */
static EcNode *parseModuleName(EcCompiler *cp)
{
    EcNode      *np, *idp;
    Ejs *ejs;
    EjsVar      *lastPackage;
    char        *name;

    ENTER(cp);

    np = parseIdentifier(cp);
    if (np == 0) {
        return LEAVE(cp, np);
    }
    name = mprStrdup(np, np->qname.name);

    ejs = cp->ejs;
    lastPackage = 0;

    while (np && getToken(cp) == T_DOT) {
        /*
         *  Stop if not "identifier"
         */
        if (peekAheadToken(cp, 1) != T_ID) {
            break;
        }

        idp = parseIdentifier(cp);
        if (idp == 0) {
            return LEAVE(cp, idp);
        }
        np = appendNode(np, idp);
        mprReallocStrcat(np, &name, 0, -1, ".", idp->qname.name, 0L);
    }
    putToken(cp);

    setId(np, name);

    return LEAVE(cp, np);
}



/*
 *  ModuleBody (496)
 *      Block -global-
 *
 *  Input
 *      {
 *
 *  AST
 *      N_BLOCK
 */
static EcNode *parseModuleBody(EcCompiler *cp)
{
    return parseBlock(cp);
}



/*
 *  Pragma (505)
 *      UsePragma Semicolon |
 *      ImportPragma Semicolon
 *
 *  Input
 *      use ...
 *      import ...
 *
 *  AST
 *      N_IMPORT
 *      N_PRAGMAS
 */
static EcNode *parsePragma(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    switch (peekToken(cp)) {
    case T_USE:
        np = parseUsePragma(cp, np);
        break;

    default:
        np = unexpected(cp);
        break;
    }

    return LEAVE(cp, np);
}





/*
 *  Pragmas (497)
 *      Pragma
 *      Pragmas Pragma
 *
 *  Input
 *      use ...
 *      import
 *
 *  AST
 *      N_PRAGMAS
 *          N_IMPORT
 *          N_PRAGMA
 */
static EcNode *parsePragmas(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    while (peekToken(cp) == T_USE) {
        np = parsePragma(cp, np);
        if (np == 0) {
            break;
        }
    }
    return LEAVE(cp, np);
}




/*
 *  UsePragma (501)
 *      use PragmaItems
 *
 *  Input
 *      use ...
 *
 *  AST
 *      N_PRAGMAS
 */
static EcNode *parseUsePragma(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (peekToken(cp) != T_USE) {
        getToken(cp);
        np = parseError(cp, "Expecting \"use\"");
    } else {
        getToken(cp);
        np = parsePragmaItems(cp, np);
    }

    return LEAVE(cp, np);
}



/*
 *  PragmaItems (502)
 *      PragmaItem
 *      PragmaItems , PragmaItem
 *
 *  Input
 *      decimal
 *      default
 *      namespace
 *      standard
 *      strict
 *      module
 *      language
 *
 *  AST
 *      N_PRAGMAS
 *      N_MODULE
 */
static EcNode *parsePragmaItems(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    do {
        np = appendNode(np, parsePragmaItem(cp));
    } while (peekToken(cp) == T_COMMA);

    return LEAVE(cp, np);
}


//  TODO - need use ecma|enhanced
/*
 *  PragmaItem (504)
 *      decimal LeftHandSideExpression
 *      default namespace PrimaryName
 *      // default number [decimal | default | double | int | long | uint | ulong]
 *      namespace PrimaryName
 *      standard
 *      strict
 *      module ModuleName OptionalStringLiteral
 *      language ecma | plus | fixed
 *
 *  Input
 *      See above
 *
 *  AST
 *      N_PRAGMA
 *      N_MODULE
 */
static EcNode *parsePragmaItem(EcCompiler *cp)
{
    EcNode      *np, *ns, *moduleName, *lang;
    EcState     *upper;
    int         attributes;

    ENTER(cp);

    attributes = 0;

    np = createNode(cp, N_PRAGMA);
    np->pragma.mode = cp->fileState->mode;
    np->pragma.lang = cp->fileState->lang;

    /*
     *  PragmaIdentifiers (737)
     */
    switch (getToken(cp)) {
    case T_DECIMAL:
        np->pragma.decimalContext = linkNode(np, parseLeftHandSideExpression(cp));
        break;

    case T_DEFAULT:
        getToken(cp);
        if (cp->token->tokenId == T_NAMESPACE) {
            if (peekToken(cp) == T_STRING) {
                getToken(cp);
                np = createNode(cp, N_USE_NAMESPACE);
                np->qname.name = mprStrdup(np, (char*) cp->token->text);
                np->useNamespace.isLiteral = 1;

            } else {
                ns = parsePrimaryName(cp);
                if (ns) {
                    np = createNode(cp, N_USE_NAMESPACE);
                    //  TODO YYY - should not be in qname
                    np->qname.name = mprStrdup(np, ns->qname.name);
                }
            }
            if (np) {
                /*
                 *  Must apply this default namespace upwards to all blocks below the blockState. It will define the
                 *  new namespace value. Note that functions and classes null this so it does not propagate into classes or
                 *  functions.
                 */
                for (upper = cp->state->prev; upper; upper = upper->prev) {
                    upper->defaultNamespace = np->qname.name;
                    if (upper == cp->blockState) {
                        break;
                    }
                }
                cp->blockState->namespace = np->qname.name;
                np->useNamespace.isDefault = 1;
            }
        }
        break;

    case T_STANDARD:
        np->pragma.mode = PRAGMA_MODE_STANDARD;
        cp->fileState->mode = np->pragma.mode;
        break;

    case T_STRICT:
        np->pragma.mode = PRAGMA_MODE_STRICT;
        cp->fileState->mode = np->pragma.mode;
        break;

    case T_NAMESPACE:
        np = createNode(cp, N_USE_NAMESPACE);
        if (peekToken(cp) == T_STRING) {
            getToken(cp);
            np->qname.name = mprStrdup(np, (char*) cp->token->text);
            np->useNamespace.isLiteral = 1;

        } else {
            ns = parsePrimaryName(cp);
            if (ns) {
                np = appendNode(np, ns);
                //  TODO - not necessary now that packages are goie
                if (ns->kind == N_DOT) {
                    np->qname.name = ns->right->qname.name;
                } else {
                    np->qname.name = ns->qname.name;
                }
            }
        }
        break;

    case T_MODULE:
        np = createNode(cp, N_USE_MODULE);
        moduleName = parseModuleName(cp);
        //  TODO - would be nice to have a one line steal and assign i.e. "mpr->name = mprSteal(np, moduleName->name)"
        np->qname.name = mprStrdup(np, moduleName->qname.name);
        if (peekToken(cp) == T_STRING) {
            getToken(cp);
            np->useModule.url = mprStrdup(np, (char*) cp->token->text);
        }
        /*
         *  Create a use namespace node
         */
        ns = createNode(cp, N_USE_NAMESPACE);
        ns->qname.name = mprStrdup(ns, np->qname.name);
        ns->useNamespace.isLiteral = 1;
        if (ns) {
            /*
             *  Must apply this default namespace upwards to all blocks below the blockState
             */
            for (upper = cp->state->prev; upper != cp->blockState; upper = upper->prev) {
                upper->defaultNamespace = ns->qname.name;
            }
            cp->blockState->namespace = ns->qname.name;
            ns->useNamespace.isDefault = 1;
        }
        np = appendNode(np, ns);
        break;

    case T_LANG:
        lang = parseIdentifier(cp);
        if (lang == 0) {
            return LEAVE(cp, lang);
        }
        np->pragma.lang = cp->fileState->lang;
        if (strcmp(lang->qname.name, "ecma") == 0) {
            cp->fileState->lang = np->pragma.lang = EJS_SPEC_ECMA;
        } else if (strcmp(lang->qname.name, "plus") == 0) {
            cp->fileState->lang = np->pragma.lang = EJS_SPEC_PLUS;
        } else if (strcmp(lang->qname.name, "fixed") == 0) {
            cp->fileState->lang = np->pragma.lang = EJS_SPEC_FIXED;
        } else {
            np = parseError(cp, "Unknown language specification ");
        }
        break;

    default:
        np = parseError(cp, "Unknown pragma identifier");
    }

    return LEAVE(cp, np);
}



/*
 *  Block -t- (514)
 *      { Directives }
 *
 *  Input
 *      {
 *
 *  AST
 *      N_BLOCK
 */
static EcNode *parseBlock(EcCompiler *cp)
{
    EcNode      *np;
    EcState     *state, *saveState;

    ENTER(cp);

    state = cp->state;
    saveState = cp->blockState;
    cp->blockState = state;

    if (getToken(cp) != T_LBRACE) {
        // putToken(cp);
        np = parseError(cp, "Expecting \"{\"");

    } else {
        np = createNode(cp, N_BLOCK);
        np = appendNode(np, parseDirectives(cp));

        if (np) {
            if (getToken(cp) != T_RBRACE) {
                // putToken(cp);
                np = parseError(cp, "Expecting \"}\"");
            }
        }
    }
    cp->blockState = saveState;

    return LEAVE(cp, np);
}



/*
 *  Program (515)
 *      Directives -global-
 *
 *  Input
 *      AnyDirective
 *
 *  AST N_PROGRAM
 *      N_DIRECTIVES ...
 *
 */
static EcNode *parseProgram(EcCompiler *cp, cchar *path)
{
    EcState     *state;
    EcNode      *np, *module, *block;
    cchar       *name;
    int         next;

    //  TODO -- could change enter to always create a node?
    ENTER(cp);

    state = cp->state;

    np = createNode(cp, N_PROGRAM);

    /*
     *  Create an unforgeable internal namespace name
     */
    np->qname.space = EJS_PUBLIC_NAMESPACE;
    if (path) {
        mprAllocSprintf(np, (char**) &np->qname.name, -1, "%s-%d", EJS_INTERNAL_NAMESPACE, cp->uid++);
    } else {
        np->qname.name = EJS_INTERNAL_NAMESPACE;
    }
    state->namespace = np->qname.name;
    cp->fileState->namespace = state->namespace;

    /*
     *  Create the default module node
     */
    module = createNode(cp, N_MODULE);
    module->qname.name = EJS_DEFAULT_MODULE;

    /*
     *  Create a block to hold the namespaces. Add a use namespace node for the default module and add modules specified 
     *  via --use switch
     */
    block = createNode(cp, N_BLOCK);
    block = appendNode(block, createNamespaceNode(cp, cp->fileState->namespace, 0, 1));
    for (next = 0; (name = mprGetNextItem(cp->useModules, &next)) != 0; ) {
        block = appendNode(block, createNamespaceNode(cp, name, 0, 1));
    }

    block = appendNode(block, parseDirectives(cp));
    module = appendNode(module, block);
    np = appendNode(np, module);

    if (!cp->interactive && peekToken(cp) != T_EOF) {
        if (np) {
            np = unexpected(cp);
        }
        return LEAVE(cp, np);
    }

    /*
     *  Reset the line number to prevent debug source lines preceeding these elements
     */
    if (np) {
        np->lineNumber = 0;
    }
    if (module) {
        module->lineNumber = 0;
    }
    if (block) {
        block->lineNumber = 0;
    }
    return LEAVE(cp, np);
}




#if UNUSED
static EcNode *parseBreak(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseContinue(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseDo(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseFor(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseIf(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseLet(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseReturn(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseSwitch(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseThrow(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);

//  np = createNode(cp, N_THROW);

    return LEAVE(cp, np);
}



static EcNode *parseTry(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseWhile(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseWith(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseVarDefinition(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseFunctionDefinition(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}



static EcNode *parseInclude(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif



#if NOT_USED_IN_GRAMMAR
static EcNode *parseSuper(EcCompiler *cp)
{
    EcNode  *np;

    ENTER(cp);
    np = 0;
    mprAssert(0);
    return LEAVE(cp, np);
}
#endif


/*
 *  Report an error. Return a null EcNode so callers can report an error and return the null in one statement.
 */
static EcNode *parseError(EcCompiler *cp, char *fmt, ...)
{
    EcToken     *tp;
    va_list     arg;
    char        *msg;

    va_start(arg, fmt);

    if (mprAllocVsprintf(cp, &msg, 0, fmt, arg) < 0) {
        msg = "Memory allocation error";
    }

    cp->errorCount++;
    cp->error = 1;

    tp = cp->token;
    if (tp) {
        ecReportError(cp, "error", tp->filename, tp->lineNumber, tp->currentLine, tp->column, msg);

    } else {
        ecReportError(cp, "error", 0, 0, 0, 0, msg);
    }

    mprFree(msg);
    va_end(arg);

    return 0;
}



EcNode *ecParseWarning(EcCompiler *cp, char *fmt, ...)
{
    EcToken     *tp;
    va_list     arg;
    char        *msg;

    va_start(arg, fmt);

    if (mprAllocVsprintf(cp, &msg, 0, fmt, arg) < 0) {
        msg = "Memory allocation error";
    }

    cp->warningCount++;

    tp = cp->token;
    ecReportError(cp, "warning", tp->filename, tp->lineNumber, tp->currentLine, tp->column, msg);

    mprFree(msg);
    va_end(arg);

    return 0;
}



/*
 *  Recover from a parse error to allow parsing to continue.
 */
EcNode *ecResetError(EcCompiler *cp, EcNode *np, bool eatInput)
{
    int     tid;

    mprAssert(cp->error);

    if (cp->error) {
        if (!cp->fatalError && cp->errorCount < EC_MAX_ERRORS) {
            cp->error = 0;
            np = createNode(cp, N_DIRECTIVES);
        }
    }


    /*
     *  Try to resync by eating input up to the next statement / directive
     */
    while (!cp->interactive) {
        tid = peekToken(cp);
        if (tid == T_SEMICOLON || tid == T_RBRACE || tid == T_RBRACKET || tid == T_RPAREN || tid == T_ERR || tid == T_EOF)  {
            break;
        }
        if (np && np->lineNumber < cp->peekToken->lineNumber) {
            /* Virtual semicolon */
            break;
        }
        getToken(cp);
    }

    return np;
}



#if FUTURE
/*
 *  Returns an allocated buffer. Caller must free.
 */
static char *detab(EcCompiler *cp, char *src)
{
    char    *p, *dest;
    int     tabCount;

    tabCount = 0;

    for (p = src; *p; p++) {
        if (*p == '\t') {
            tabCount++;
        }
    }

    dest = mprAlloc(cp, strlen(src) + 1 + (tabCount * cp->tabWidth));
    if (dest == 0) {
        mprAssert(dest);
        return src;
    }
    for (p = dest; *src; src++) {
        if (*src== '\t') {
            *p++ = ' ';
            *p++ = ' ';
            *p++ = ' ';
            *p++ = ' ';
        } else {
            *p++ = *src;
        }
    }
    *p = '\0';

    return dest;
}
#endif

/*
 *  Create a line of spaces with an "^" pointer at the current parse error.
 *  Returns an allocated buffer. Caller must free.
 */
static char *makeHighlight(EcCompiler *cp, char *src, int col)
{
    char    *p, *dest;
    int     tabCount, len, i;

    tabCount = 0;

    for (p = src; *p; p++) {
        if (*p == '\t') {
            tabCount++;
        }
    }

    len = (int) strlen(src) + (tabCount * cp->tabWidth);
    len = max(len, col);

    /*
     *  Allow for "^" to be after the last char, plus one null.
     */
    dest = (char*) mprAlloc(cp, len + 2);
    if (dest == 0) {
        mprAssert(dest);
        return src;
    }
    for (i = 0, p = dest; *src; src++, i++) {
        if (*src== '\t') {
            *p++ = *src;
        } else {
            *p++ = ' ';
        }
    }

    /*
     *  Cover the case where the ^ must go after the end of the input
     */
    dest[col] = '^';
    if (p == &dest[col]) {
        ++p;
    }
    *p = '\0';

    return dest;
}



void ecReportError(EcCompiler *cp, cchar *severity, cchar *filename, int lineNumber, char *currentLine, int column, 
        char *msg)
{
    cchar   *appName;
    char    *highlightPtr;
    int     errCode;

    //  TODO - Need to calculate error codes.
    errCode = 0;

    appName = mprGetAppName(cp);
    if (filename == 0 || *filename == '\0') {
        filename = "stdin";
    }

#if FUTURE_WITH_ERROR_CODES
    if (currentLine) {
        highlightPtr = makeHighlight(cp, (char*) currentLine, column);
        mprErrorPrintf(cp, "%s: %s: %d: %d: %s: %s\n  %s  \n  %s\n", appName, filename, lineNumber, errCode, severity,
            msg, currentLine, highlightPtr);

    } else if (lineNumber >= 0) {
        mprErrorPrintf(cp, "%s: %s: %d: %d: %s: %s\n", appName, filename, lineNumber, errCode, severity, msg);

    } else {
        mprErrorPrintf(cp, "%s: %s: 0: %d: %s: %s\n", appName, filename, errCode, severity, msg);
    }
#else
    if (currentLine) {
        highlightPtr = makeHighlight(cp, (char*) currentLine, column);
        mprErrorPrintf(cp, "%s: %s: %d: %s: %s\n  %s  \n  %s\n", appName, filename, lineNumber, severity,
            msg, currentLine, highlightPtr);

    } else if (lineNumber >= 0) {
        mprErrorPrintf(cp, "%s: %s: %d: %s: %s\n", appName, filename, lineNumber, severity, msg);

    } else {
        mprErrorPrintf(cp, "%s: %s: 0: %s: %s\n", appName, filename, severity, msg);
    }
#endif

    mprBreakpoint();
}



/*
 *
 */
static void updateTokenInfo(EcCompiler *cp)
{
    mprAssert(cp);
    mprAssert(cp->input);

    cp->token = cp->input->token;

#if BLD_DEBUG
    /*
     *  Update source file and line number information.
     */
    if (cp->token) {
        cp->tokenName = tokenNames[cp->token->tokenId];
        cp->currentLine = cp->token->currentLine;
    }
#endif
}



/*
 *  Get the next input token. May have been previous obtained and putback.
 */
static int getToken(EcCompiler *cp)
{
    int         id;

    if (cp->fatalError) {
        return T_ERR;
    }

    id = ecGetToken(cp->input);

    updateTokenInfo(cp);

    cp->peekToken = 0;
#if BLD_DEBUG
    cp->peekTokenName = 0;
#endif

    return id;
}



/*
 *  Peek ahead (K) tokens and return the token id
 */
static int peekAheadToken(EcCompiler *cp, int ahead)
{
    EcToken     *token;

    token = peekAheadTokenStruct(cp, ahead);
    if (token == 0) {
        return EJS_ERR;
    }
    return token->tokenId;
}



int ecPeekToken(EcCompiler *cp)
{
    return peekAheadToken(cp, 1);
}


/*
 *  Peek ahead (K) tokens and return the token.
 */
static EcToken *peekAheadTokenStruct(EcCompiler *cp, int ahead)
{
    EcToken     *token, *currentToken, *tokens[EC_MAX_LOOK_AHEAD];
    int         i;

    mprAssert(ahead > 0 && ahead <= EC_MAX_LOOK_AHEAD);

    cp->peeking = 1;

    if (ahead == 1) {

        /*
         *  Fast look ahead of one token.
         */
        if (cp->input->putBack) {
#if BLD_DEBUG
            cp->peekTokenName = tokenNames[cp->input->putBack->tokenId];
#endif
            cp->peekToken = cp->input->putBack;
            return cp->input->putBack;
        }
    }

    /*
     *  takeToken will take the current token and remove it from the input
     *  We must preserve the current token throughout.
     */
    currentToken = ecTakeToken(cp->input);
    for (i = 0; i < ahead; i++) {
        if (ecGetToken(cp->input) < 0) {
            cp->peeking = 0;
            mprAssert(0);
            return 0;
        }
        tokens[i] = ecTakeToken(cp->input);
    }

    /*
     *  Peek at the token of interest
     */
    token = tokens[i - 1];

    for (i = ahead - 1; i >= 0; i--) {
        putSpecificToken(cp, tokens[i]);
    }

    if (currentToken) {
        ecPutSpecificToken(cp->input, currentToken);
        ecGetToken(cp->input);
        updateTokenInfo(cp);
    }

#if BLD_DEBUG
    cp->peekTokenName = tokenNames[token->tokenId];
#endif

    cp->peekToken = token;
    cp->peeking = 0;

    return token;
}



static void putToken(EcCompiler *cp)
{
    ecPutToken(cp->input);
}



static void putSpecificToken(EcCompiler *cp, EcToken *token)
{
    ecPutSpecificToken(cp->input, token);
}



/*
 *  Create a new node. This will be automatically freed when returning from a non-terminal production (ie. the state
 *  is destroyed). Returning results are preserved by stealing the node from the state memory context.
 *
 *  NOTE: we are using a tree based memory allocator with destructors.
 */
// TODO - move to a node.c

static EcNode *createNode(EcCompiler *cp, int kind)
{
    EcNode      *np;
    EcToken     *token;
    int         len;

    mprAssert(cp->state);

    np = mprAllocObjZeroed(cp->state, EcNode);
    if (np == 0) {
        cp->memError = 1;
        return 0;
    }

    np->seqno = cp->nextSeqno++;
    np->kind = kind;
    np->cp = cp;
    np->slotNum = -1;

#if BLD_DEBUG
    np->kindName = nodes[kind];
#endif

    np->lookup.slotNum = -1;

    /*
     *  Remember the current input token. Don't do for initial program and module nodes.
     */
    if (cp->token == 0 && cp->state->blockNestCount > 0) {
        /*
         *  TODO - OPT. putToken will clear the peek token. So must re-peek incase the caller has done
         *  a peek and expects cp->peek to contain valid values.
         */
        getToken(cp);
        putToken(cp);
        peekToken(cp);
    }

    token = cp->token;
    if (token) {
        np->tokenId = token->tokenId;
        np->groupMask = token->groupMask;
        np->subId = token->subId;

#if BLD_DEBUG
        if (token->tokenId >= 0) {
            np->tokenName = tokenNames[token->tokenId];
        }
#endif
    }

    np->children = mprCreateList(np);

    if (token && token->currentLine) {
        //  TODO - really want a way to get a direct pointer to the input code line without strdup
        np->filename = mprStrdup(np, token->filename);
        np->currentLine = mprStrdup(np, token->currentLine);
        len = (int) strlen(np->currentLine);
        if (len > 0 && np->currentLine[len - 1] == '\n') {
            np->currentLine[len - 1] = '\0';
        }
        np->lineNumber = token->lineNumber;
        np->column = token->column;

        //  TODO - remove
        mprLog(np, 9, "At line %d, token \"%s\", line %s", token->lineNumber, token->text, np->currentLine);
    }

    /*
     *  Per AST node type initialisation
     */
    switch (kind) {
    case N_LITERAL:
        break;
    }

    return np;
}



static void setNodeDoc(EcCompiler *cp, EcNode *np)
{
#if BLD_FEATURE_EJS_DOC
    Ejs     *ejs;

    ejs = cp->ejs;

    if (ejs->flags & EJS_FLAG_DOC && cp->input->doc) {
        np->doc = cp->input->doc;
        cp->input->doc = 0;
        mprStealBlock(np, np->doc);
    }
#endif
}


static void appendDocString(EcCompiler *cp, EcNode *np, EcNode *parameter, EcNode *value)
{
#if BLD_FEATURE_EJS_DOC
    char        *doc, arg[MPR_MAX_STRING];
    int         found;
    
    if (!(cp->ejs->flags & EJS_FLAG_DOC)) {
        return;
    }
    if (np == 0 || parameter == 0 || parameter->kind != N_QNAME || value == 0 || value->kind != N_QNAME) {
        return;
    }

    if (np->doc) {
        //  TODO - OPT
        found = 0;
        mprSprintf(arg, sizeof(arg), "@param %s ", parameter->qname.name);
        if (strstr(np->doc, arg) != 0) {
            found++;
        } else {
            mprSprintf(arg, sizeof(arg), "@params %s ", parameter->qname.name);
            if (strstr(np->doc, arg) != 0) {
                found++;
            }
        }
        if (found) {
            mprAllocSprintf(np, &doc, -1, "%s\n@default %s %s", np->doc, parameter->qname.name, value->qname.name);
        } else {
            mprAllocSprintf(np, &doc, -1, "%s\n@param %s\n@default %s %s", np->doc, parameter->qname.name,
                parameter->qname.name, value->qname.name);
        }
        mprFree(np->doc);
        np->doc = doc;
    }
#endif
}


static void copyDocString(EcCompiler *cp, EcNode *np, EcNode *from)
{
#if BLD_FEATURE_EJS_DOC
    Ejs     *ejs;

    ejs = cp->ejs;

    if (ejs->flags & EJS_FLAG_DOC && from->doc) {
        np->doc = from->doc;
        from->doc = 0;
        mprStealBlock(np, np->doc);
    }
#endif
}



/*
 *  This is used outside the parser. It must reset the line number as the
 *  node will not correspond to any actual source code line;
 */
EcNode *ecCreateNode(EcCompiler *cp, int kind)
{
    EcNode  *node;

    node = createNode(cp, kind);
    if (node) {
        node->lineNumber = -1;
        node->currentLine = 0;
    }
    return node;
}



static EcNode *createNameNode(EcCompiler *cp, cchar *name, cchar *space)
{
    EcNode      *np;

    np = createNode(cp, N_QNAME);
    if (np) {
        np->qname.name = mprStrdup(np, name);
        np->qname.space = mprStrdup(np, space);
    }
    return np;
}


static EcNode *createNamespaceNode(EcCompiler *cp, cchar *name, bool isDefault, bool isLiteral)
{
    EcNode      *np;
    
    np = createNode(cp, N_USE_NAMESPACE);
    np->qname.name = mprStrdup(np, name);
    np->useNamespace.isDefault = isDefault;
    np->useNamespace.isLiteral = isLiteral;
    return np;
}



/*
 *  This is used outside the parser.
 */
EcNode *ecLinkNode(EcNode *np, EcNode *child)
{
    return linkNode(np, child);
}



/*
 *  Create a binary tree node.
 */
static EcNode *createBinaryNodeInner(EcCompiler *cp, EcNode *lhs, EcNode *rhs, EcNode *parent)
{
    mprAssert(cp);
    mprAssert(lhs);
    mprAssert(parent);

    /*
     *  appendNode will return the parent if no error
     */
    parent = appendNode(parent, lhs);
    parent = appendNode(parent, rhs);

    return parent;
}


/*
 *  Create an assignment op node.
 */
static EcNode *createAssignNodeInner(EcCompiler *cp, EcNode *lhs, EcNode *rhs, EcNode *parent)
{
    mprAssert(cp);
    mprAssert(lhs);
    mprAssert(parent);

    /*
     *  appendNode will return the parent if no error
     */
    parent = appendNode(parent, lhs);
    parent = appendNode(parent, rhs);

    return parent;
}


/*
 *  Add a child node. If an allocation error, return 0, otherwise return the
 *  parent node.
 */
static EcNode *appendNode(EcNode *np, EcNode *child)
{
    EcCompiler      *cp;
    MprList         *list;
    int             index;

    mprAssert(np != child);

    if (child == 0 || np == 0) {
        return 0;
    }
    list = np->children;

    cp = np->cp;

    index = mprAddItem(list, child);
    if (index < 0) {
        cp->memError = 1;
        return 0;
    }

    if (index == 0) {
        np->left = (EcNode*) list->items[index];
    } else if (index == 1) {
        np->right = (EcNode*) list->items[index];
    }

    child->parent = np;
    mprStealBlock(list, child);

    return np;
}



EcNode *ecAppendNode(EcNode *np, EcNode *child)
{
    return appendNode(np, child);
}



EcNode *ecChangeNode(EcNode *np, EcNode *oldNode, EcNode *newNode)
{
    EcNode      *child;
    MprList     *list;
    int         next, index;

    next = 0;
    while ((child = (EcNode*) mprGetNextItem(np->children, &next))) {
        if (child == oldNode) {
            index = next - 1;
            mprSetItem(np->children, index, newNode);
            mprStealBlock(np, newNode);
            list = np->children;
            if (index == 0) {
                np->left = (EcNode*) list->items[index];
            } else if (index == 1) {
                np->right = (EcNode*) list->items[index];
            }
            newNode->parent = np;
            return np;
        }
    }
    mprAssert(0);
    return 0;
}



/*
 *  Link a node. This only steals the node.
 */
static EcNode *linkNode(EcNode *np, EcNode *node)
{
    if (node == 0 || np == 0) {
        return 0;
    }

    node->parent = np;
    mprStealBlock(np, node);

    return node;
}



/*
 *  Insert a child node. If an allocation error, return 0, otherwise return the parent node.
 */
static EcNode *insertNode(EcNode *np, EcNode *child, int pos)
{
    EcCompiler      *cp;
    MprList         *list;
    int             index, len;

    if (child == 0 || np == 0) {
        return 0;
    }
    list = np->children;

    cp = np->cp;

    index = mprInsertItemAtPos(list, pos, child);
    if (index < 0) {
        cp->memError = 1;
        return 0;
    }

    len = mprGetListCount(list);
    if (len > 0) {
        np->left = (EcNode*) list->items[0];
    }
    if (len > 1) {
        np->right = (EcNode*) list->items[1];
    }

    child->parent = np;
    mprStealBlock(list, child);

    return np;
}



/*
 *  Remove a child node and return it.
 */
static EcNode *removeNode(EcNode *np, EcNode *child)
{
    EcCompiler      *cp;
    MprList         *list;
    int             index;

    if (child == 0 || np == 0) {
        return 0;
    }
    list = np->children;

    cp = np->cp;

    index = mprRemoveItem(list, child);
    mprAssert(index >= 0);

    if (index == 0) {
        np->left = np->right;
    } else if (index == 1) {
        np->right = 0;
    }
    child->parent = 0;

    return child;
}


/* XXX */

static void setId(EcNode *np, char *name)
{
    mprAssert(np);
    mprAssert(np->kind == N_QNAME || np->kind == N_VOID);
    mprAssert(name);

    //  TODO OPT -- could steal?
    if (np->qname.name != name) {
        mprFree((char*) np->qname.name);
        np->qname.name = mprStrdup(np, name);
    }
}



static EcNode *unexpected(EcCompiler *cp)
{
    int     junk = 0;

    /*
     *  This is just to avoid a Vxworks 5.4 linker bug. The link crashes when this function has no local vars.
     */
    dummy(junk);
    return parseError(cp, "Unexpected input \"%s\"", cp->token->text);
}


/* YYY */

static EcNode *expected(EcCompiler *cp, const char *str)
{
    return parseError(cp, "Expected input \"%s\"", str);
}


static const char *getExt(const char *path)
{
    char    *cp;

    if ((cp = strrchr(path, '.')) != 0) {
        return cp;
    }
    return "";
}



static void applyAttributes(EcCompiler *cp, EcNode *np, EcNode *attributeNode, cchar *overrideNamespace)
{
    EcNode      *qualifierNode;
    EcState     *state;
    cchar       *namespace;
    int         attributes;

    state = cp->state;

    attributes = 0;
    namespace = 0;
    qualifierNode = 0;

    if (attributeNode) {
        /*
         *  Attribute node passed in.
         */
        attributes = attributeNode->attributes;
        qualifierNode = attributeNode->qualifierNode;
        if (attributeNode->qname.space) {
            namespace = mprStrdup(np, attributeNode->qname.space);
        }
        if (attributeNode->literalNamespace) {
            np->literalNamespace = 1;
        }

    } else {
        /*
         *  "space"::var
         */
        if (np->qname.space) {
            namespace = np->qname.space;
        }
    }

    if (namespace == 0) {
        if (overrideNamespace) {
            namespace = overrideNamespace;
        } else if (cp->blockState->defaultNamespace) {
            namespace = cp->blockState->defaultNamespace;
        } else {
            namespace = cp->blockState->namespace;
        }
    }
    mprAssert(namespace && *namespace);

    if (state->inFunction) {
        /*
         *  Functions don't need qualification of private properties.
         */
        //  CHECK that namespace should be empty here always? or allow user defined namespaces. Then should not be using format...
        if (strcmp(namespace, EJS_PRIVATE_NAMESPACE) == 0) {
            namespace = (char*) mprStrdup(np, namespace);
        } else {
            namespace = (char*) ejsFormatReservedNamespace(np, 0, namespace);
        }

    } else if (state->inClass) {
        if (strcmp(namespace, EJS_INTERNAL_NAMESPACE) == 0) {
            namespace = mprStrdup(np, cp->fileState->namespace);

        } else if (strcmp(namespace, EJS_PRIVATE_NAMESPACE) == 0 || strcmp(namespace, EJS_PROTECTED_NAMESPACE) == 0) {
            namespace = (char*) ejsFormatReservedNamespace(np, &state->currentClassName, namespace);

        } else {
            namespace = (char*) mprStrdup(np, namespace);
        }

    } else {
        if (strcmp(namespace, EJS_INTERNAL_NAMESPACE) == 0) {
            namespace = mprStrdup(np, cp->fileState->namespace);

        } else {
            namespace = (char*) mprStrdup(np, namespace);
        }
    }
    np->qname.space = namespace;

    mprLog(np, 7, "Parser apply attributes namespace = \"%s\", current line %s", namespace, np->currentLine);
    mprAssert(np->qname.space && np->qname.space[0]);

    np->attributes |= attributes;
    if (qualifierNode) {
        //  TODO - more to debug this.
        np->qualifierNode = qualifierNode;
    }
}



static void addTokenToBuf(EcCompiler *cp, EcNode *np)
{
    MprBuf      *buf;

    if (np) {
        buf = np->literal.data;
        mprPutStringToBuf(buf, (cchar*) cp->token->text);
        mprAddNullToBuf(buf);
        mprLog(cp, 7, "Literal: \n%s\n", buf->start);
    }
}



/*
 *  Reset the input. Eat all tokens, clear errors, exceptions and the result value. Used by ejs for console input.
 */
void ecResetInput(EcCompiler *cp)
{
    EcToken     *tp;

    while ((tp = cp->input->putBack) != 0 && (tp->tokenId == T_EOF || tp->tokenId == T_NOP)) {
        ecGetToken(cp->input);
    }
    cp->input->stream->flags &= ~EC_STREAM_EOL;

    cp->error = 0;
    cp->ejs->exception = 0;
    cp->ejs->result = cp->ejs->undefinedValue;
}



void ecSetOptimizeLevel(EcCompiler *cp, int level)
{
    cp->optimizeLevel = level;
}



void ecSetWarnLevel(EcCompiler *cp, int level)
{
    cp->warnLevel = level;
}



void ecSetDefaultMode(EcCompiler *cp, int mode)
{
    cp->defaultMode = mode;
}



void ecSetTabWidth(EcCompiler *cp, int width)
{
    cp->tabWidth = width;
}



void ecSetOutputFile(EcCompiler *cp, cchar *outputFile)
{
    if (outputFile) {
        mprFree(cp->outputFile);
        cp->outputFile = mprStrdup(cp, outputFile);
    }
}



void ecSetCertFile(EcCompiler *cp, cchar *certFile)
{
    mprFree(cp->certFile);
    cp->certFile = mprStrdup(cp, certFile);
}


/*
 *  Just part of a VxWorks 5.4 compiler bug to avoid a linker crash
 */
static void dummy(int junk) {
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
