/**
 *  ecLex.c - Lexical analyzer
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
 *  TODO - must preserve inter-token white space for XML
 */

/********************************** Includes **********************************/

#include    "ejs.h"
#include    "ecCompiler.h"

/*********************************** Locals ***********************************/

typedef struct ReservedWord
{
    char    *name;
    int     groupMask;
    int     tokenId;
    int     subId;
} ReservedWord;



/*
 *  Reserved keyword table
 *  The "true", "false", "null" and "undefined" are handled as global variables
 */

static ReservedWord keywords[] =
{
#if UNSUPPORED && TODO
  { "as",               T_XXXXXXXXX,        T_AS,                       0, },
#endif

  { "break",            G_RESERVED,         T_BREAK,                    0, },
  { "case",             G_RESERVED,         T_CASE,                     0, },

#if UNUSED
    //  TODO -- how is this used
  { "call",             G_CONREV,           T_CALL,                     0, },
#endif

  { "callee",           G_CONREV,           T_CALLEE,                   0, },
  { "cast",             G_CONREV,           T_CAST,                     0, },
  { "catch",            G_RESERVED,         T_CATCH,                    0, },
  { "class",            G_RESERVED,         T_CLASS,                    0, },
  { "const",            G_CONREV,           T_CONST,                    0, },
  { "continue",         G_RESERVED,         T_CONTINUE,                 0, },

#if UNUSED
    //  TODO -- how is this used
  { "debugger",         G_CONREV,           T_DEBUGGER,                 0, },
#endif

  { "default",          G_RESERVED,         T_DEFAULT,                  0, },
  { "delete",           G_RESERVED,         T_DELETE,                   0, },
  { "do",               G_RESERVED,         T_DO,                       0, },
  { "dynamic",          G_CONREV,           T_ATTRIBUTE,                T_DYNAMIC, },
  { "each",             G_CONREV,           T_EACH,                     0, },
  { "else",             G_RESERVED,         T_ELSE,                     0, },
  { "enum",             G_RESERVED,         T_ENUM,                     T_ENUM, },
  { "extends",          G_RESERVED,         T_EXTENDS,                  0, },
  { "false",            G_RESERVED,         T_FALSE,                    0, },
  { "final",            G_CONREV,           T_ATTRIBUTE,                T_FINAL, },
  { "finally",          G_RESERVED,         T_FINALLY,                  0, },
  { "for",              G_RESERVED,         T_FOR,                      0, },
  { "function",         G_RESERVED,         T_FUNCTION,                 0, },
  { "generator",        G_CONREV,           T_GENERATOR,                0, },
  { "get",              G_CONREV,           T_GET,                      0, },
  { "goto",             G_CONREV,           T_GOTO,                     0, },
  { "has",              G_CONREV,           T_HAS,                      0, },
  { "if",               G_RESERVED,         T_IF,                       0, },
  { "implements",       G_CONREV,           T_IMPLEMENTS,               0, },
  { "in",               G_RESERVED,         T_IN,                       0, },
  { "include",          G_CONREV,           T_INCLUDE,                  0, },
  { "instanceof",       G_RESERVED,         T_INSTANCEOF,               0, },
  { "interface",        G_CONREV,           T_INTERFACE,                0, },
  { "internal",         G_CONREV,           T_RESERVED_NAMESPACE,       T_INTERNAL, },
  { "intrinsic",        G_CONREV,           T_RESERVED_NAMESPACE,       T_INTRINSIC, },
  { "is",               G_CONREV,           T_IS,                       0, },
  { "lang",             G_CONREV,           T_LANG,                     0, },
  { "like",             G_CONREV,           T_LIKE,                     0, },
  { "let",              G_CONREV,           T_LET,                      0, },
  { "module",           G_RESERVED,         T_MODULE,                   0, },
  { "namespace",        G_CONREV,           T_NAMESPACE,                0, },
  { "native",           G_CONREV,           T_ATTRIBUTE,                T_NATIVE, },
  { "new",              G_RESERVED,         T_NEW,                      0, },
  { "null",             G_RESERVED,         T_NULL,                     0, },
  { "override",         G_CONREV,           T_ATTRIBUTE,                T_OVERRIDE, },
  { "private",          G_CONREV,           T_RESERVED_NAMESPACE,       T_PRIVATE, },
  { "protected",        G_CONREV,           T_RESERVED_NAMESPACE,       T_PROTECTED, },
  { "prototype",        G_CONREV,           T_ATTRIBUTE,                T_PROTOTYPE, },
  { "public",           G_CONREV,           T_RESERVED_NAMESPACE,       T_PUBLIC, },
  { "return",           G_RESERVED,         T_RETURN,                   0, },
  { "set",              G_CONREV,           T_SET,                      0, },
  { "standard",         G_CONREV,           T_STANDARD,                 0, },
  { "static",           G_CONREV,           T_ATTRIBUTE,                T_STATIC, },
  { "strict",           G_CONREV,           T_STRICT,                   0, },
  { "super",            G_RESERVED,         T_SUPER,                    0, },
  { "switch",           G_RESERVED,         T_SWITCH,                   0, },
  { "this",             G_RESERVED,         T_THIS,                     0, },
  { "throw",            G_RESERVED,         T_THROW,                    0, },
  { "to",               G_CONREV,           T_TO,                       0, },
  { "true",             G_RESERVED,         T_TRUE,                     0, },
  { "try",              G_RESERVED,         T_TRY,                      0, },
  { "typeof",           G_RESERVED,         T_TYPEOF,                   0, },
  { "type",             G_CONREV,           T_TYPE,                     0, },
  { "var",              G_RESERVED,         T_VAR,                      0, },
  { "undefined",        G_CONREV,           T_UNDEFINED,                0, },
  { "use",              G_CONREV,           T_USE,                      0, },
  { "void",             G_RESERVED,         T_VOID,                     0, },
  { "while",            G_RESERVED,         T_WHILE,                    0, },
  { "with",             G_RESERVED,         T_WITH,                     0, },
  { "yield",            G_CONREV,           T_YIELD,                    0, },

    /*
     *  Ejscript enhancements
     */
  { "enumerable",       G_RESERVED,         T_ATTRIBUTE,                T_ENUMERABLE, },
  { "readonly",         G_RESERVED,         T_ATTRIBUTE,                T_READONLY, },
  { "synchronized",     G_RESERVED,         T_ATTRIBUTE,                T_SYNCHRONIZED, },
  { "volatile",         G_CONREV,           T_VOLATILE,                 0, },

#if UNUSED
  { "eval",             G_CONREV,           T_EVAL,                     0, },
  { "blended",          G_CONREV,           T_BLENDED,                  0, },
  { "decimal",          G_CONREV,           T_DECIMAL,                  0, },
  { "double",           G_CONREV,           T_DOUBLE,                   0, },
  { "int",              G_CONREV,           T_INT,                      0, },
  { "uint",             G_CONREV,           T_UINT,                     0, },
  { "boolean",          G_CONREV,           T_BOOLEAN,                  0, },
  { "long",             G_CONREV,           T_LONG,                     0, },
  { "ulong",            G_CONREV,           T_ULONG,                    0, },
  //    TODO - rename
  { "number",           G_CONREV,           T_NUMBER_WORD,              0, },
  { "xml",              G_CONREV,           T_XML,                      0, },
#endif

  /*
   *    Reserved but not implemented
   */
  { "abstract",         G_RESERVED,         T_ABSTRACT,                 0, },
  { 0,                  0,                  0, },
};

/***************************** Forward Declarations ***************************/

static int  addCharToToken(EcToken *tp, int c);
static int  addFormattedStringToToken(EcToken *tp, char *fmt, ...);
static int  addStringToToken(EcToken *tp, char *str);
static int  decodeNumber(EcInput *input, int radix, int length);
static void initializeToken(EcToken *tp, EcStream *stream);
static int  destroyLexer(EcLexer *lp);
static int  finishToken(EcToken *tp, int tokenId, int subId, int groupMask);
static int  getNumberToken(EcInput *input, EcToken *tp, int c);
static int  getAlphaToken(EcInput *input, EcToken *tp, int c);
static int  getComment(EcInput *input, EcToken *tp, int c);
static int  getNextChar(EcStream *stream);
static int  getQuotedToken(EcInput *input, EcToken *tp, int c);
static int  makeSubToken(EcToken *tp, int c, int tokenId, int subId, int groupMask);
static int  makeToken(EcToken *tp, int c, int tokenId, int groupMask);
static void putBackChar(EcStream *stream, int c);
static void setTokenCurrentLine(EcToken *tp);

/************************************ Code ************************************/


EcLexer *ecCreateLexer(EcCompiler *cp)
{
    EcLexer         *lp;
    ReservedWord    *rp;
    int             size;

    lp = mprAllocObjWithDestructorZeroed(cp, EcLexer, destroyLexer);
    if (lp == 0) {
        return 0;
    }

    lp->input = mprAllocObjZeroed(lp, EcInput);
    if (lp->input == 0) {
        mprFree(lp);
        return 0;
    }
    lp->input->lexer = lp;
    lp->input->compiler = cp;
    lp->compiler = cp;

    size = sizeof(keywords) / sizeof(ReservedWord);
    lp->keywords = mprCreateHash(lp, size);
    if (lp->keywords == 0) {
        mprFree(lp);
        return 0;
    }

    for (rp = keywords; rp->name; rp++) {
        mprAddHash(lp->keywords, rp->name, rp);
    }

    return lp;
}


static int destroyLexer(EcLexer *lp)
{
    return 0;
}


void ecDestroyLexer(EcCompiler *cp)
{
    mprFree(cp->lexer);
    cp->lexer = 0;
}


int ecGetToken(EcInput *input)
{
    EcToken     *token, *tp;
    EcStream    *stream;
    int         c;

    //  TODO - functionalize this section

    token = input->token;

    if ((tp = input->putBack) != 0) {
        input->putBack = tp->next;
        input->token = tp;

        /*
         *  Move any old token to free list
         */
        if (token) {
            token->next = input->freeTokens;
            input->freeTokens = token;
        }
        return tp->tokenId;
    }

    if (token == 0) {
        //  TBD -- need an API for this
        input->token = mprAllocObjZeroed(input, EcToken);
        if (input->token == 0) {
            //  TBD -- err code
            return -1;
        }
        input->token->lineNumber = 1;
    }

    stream = input->stream;
    tp = input->token;
    mprAssert(tp);

    initializeToken(tp, stream);

    while (1) {

        c = getNextChar(stream);

        /*
         *  Overloadable operators
         *
         *  + - ~ * / % < > <= >= == << >> >>> & | === != !==
         *
         *  TODO FUTURE, we could allow also:  ".", "[", "("
         *
         *  TODO: what about unary !, ^
         */
        switch (c) {
        default:
        number:
            if (isdigit(c)) {
                return getNumberToken(input, tp, c);

            } else if (c == '\\') {
                c = getNextChar(stream);
                if (c == '\n') {
                    break;
                }
                putBackChar(stream, c);
                c = '\n';
            }
            if (isalpha(c) || c == '_' || c == '\\' || c == '$') {
                return getAlphaToken(input, tp, c);
            }
            return makeToken(tp, 0, T_ERR, 0);

        case -1:
            return makeToken(tp, 0, T_ERR, 0);

        case 0:
            if (stream->flags & EC_STREAM_EOL) {
                return makeToken(tp, 0, T_NOP, 0);
            } else {
                return makeToken(tp, 0, T_EOF, 0);
            }

        case ' ':
        case '\t':
            break;

        case '\r':
        case '\n':
            if (tp->textLen == 0 && tp->lineNumber != stream->lineNumber) {
                tp->currentLine = 0;
            }
            break;

        case '"':
        case '\'':
            return getQuotedToken(input, tp, c);

        case '#':
            return makeToken(tp, c, T_HASH, 0);

        case '[':
            //  EJS extension to consider this an operator
            return makeToken(tp, c, T_LBRACKET, G_OPERATOR);

        case ']':
            return makeToken(tp, c, T_RBRACKET, 0);

        case '(':
            //  EJS extension to consider this an operator
            return makeToken(tp, c, T_LPAREN, G_OPERATOR);

        case ')':
            return makeToken(tp, c, T_RPAREN, 0);

        case '{':
            return makeToken(tp, c, T_LBRACE, 0);

        case '}':
            return makeToken(tp, c, T_RBRACE, 0);

        case '@':
            return makeToken(tp, c, T_AT, 0);

        case ';':
            return makeToken(tp, c, T_SEMICOLON, 0);

        case ',':
            return makeToken(tp, c, T_COMMA, 0);

        case '?':
            return makeToken(tp, c, T_QUERY, 0);

        case '~':
            return makeToken(tp, c, T_TILDE, G_OPERATOR);

        case '+':
            c = getNextChar(stream);
            if (c == '+') {
                addCharToToken(tp, '+');
                return makeToken(tp, c, T_PLUS_PLUS, G_OPERATOR);
            } else if (c == '=') {
                addCharToToken(tp, '+');
                return makeSubToken(tp, c, T_ASSIGN, T_PLUS_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
            }
            putBackChar(stream, c);
            return makeToken(tp, '+', T_PLUS, G_OPERATOR);

        case '-':
            c = getNextChar(stream);
            if (isdigit(c)) {
                putBackChar(stream, c);
                return makeToken(tp, '-', T_MINUS, G_OPERATOR);

            } else if (c == '-') {
                addCharToToken(tp, '-');
                return makeToken(tp, c, T_MINUS_MINUS, G_OPERATOR);

            } else if (c == '=') {
                addCharToToken(tp, '-');
                return makeSubToken(tp, c, T_ASSIGN, T_MINUS_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
            }
            putBackChar(stream, c);
            return makeToken(tp, '-', T_MINUS, G_OPERATOR);

        case '*':
            c = getNextChar(stream);
            if (c == '=') {
                addCharToToken(tp, '*');
                return makeSubToken(tp, c, T_ASSIGN, T_MUL_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
            }
            putBackChar(stream, c);
            return makeToken(tp, '*', T_MUL, G_OPERATOR);

        case '/':
            c = getNextChar(stream);
            if (c == '=') {
                addCharToToken(tp, '/');
                return makeSubToken(tp, c, T_ASSIGN, T_DIV_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);

            } else if (c == '>') {
                addCharToToken(tp, '/');
                return makeToken(tp, c, T_SLASH_GT, G_OPERATOR);

            } else if (c == '*' || c == '/') {
                /*
                 *  C and C++ comments
                 */
                if (getComment(input, tp, c) < 0) {
                    return tp->tokenId;
                }
#if BLD_FEATURE_EJS_DOC
                if (tp->text && tp->text[0] == '*') {
                    mprFree(input->doc);
                    input->doc = mprStrdup(input, (char*) tp->text);
                }
#endif
                initializeToken(tp, stream);
                break;
            }
            putBackChar(stream, c);
            return makeToken(tp, '/', T_DIV, G_OPERATOR);

        case '%':
            c = getNextChar(stream);
            if (c == '=') {
                addCharToToken(tp, '%');
                return makeSubToken(tp, c, T_ASSIGN, T_MOD_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
            }
            putBackChar(stream, c);
            return makeToken(tp, '%', T_MOD, G_OPERATOR);

        case '.':
            c = getNextChar(stream);
            if (c == '.') {
                c = getNextChar(stream);
                if (c == '.') {
                    addStringToToken(tp, "..");
                    return makeToken(tp, c, T_ELIPSIS, 0);
                }
                putBackChar(stream, c);
                addCharToToken(tp, '.');
                return makeToken(tp, '.', T_DOT_DOT, 0);

            } else if (c == '<') {
                addCharToToken(tp, '.');
                return makeToken(tp, c, T_DOT_LESS, 0);

            } else if (isdigit(c)) {
                putBackChar(stream, c);
                goto number;
            }
            putBackChar(stream, c);
            //  EJS extension to consider this an operator
            return makeToken(tp, '.', T_DOT, G_OPERATOR);

        case ':':
            c = getNextChar(stream);
            if (c == ':') {
                addCharToToken(tp, ':');
                return makeToken(tp, c, T_COLON_COLON, 0);
            }
            putBackChar(stream, c);
            return makeToken(tp, ':', T_COLON, 0);

        case '!':
            c = getNextChar(stream);
            if (c == '=') {
                c = getNextChar(stream);
                if (c == '=') {
                    addStringToToken(tp, "!=");
                    return makeToken(tp, c, T_STRICT_NE, G_OPERATOR);
                }
                putBackChar(stream, c);
                addCharToToken(tp, '!');
                return makeToken(tp, '=', T_NE, G_OPERATOR);
            }
            putBackChar(stream, c);
            return makeToken(tp, '!', T_LOGICAL_NOT, G_OPERATOR);

#if UNUSED
        case '~':
            c = getNextChar(stream);
            if (c == '=') {
                addStringToToken(tp, "~=");
                return makeSubToken(tp, c, T_ASSIGN, T_BIT_NEG_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
            }
            putBackChar(stream, c);
            return makeToken(tp, '~', T_BIT_NEG, G_OPERATOR);
#endif

        case '&':
            c = getNextChar(stream);
            if (c == '&') {
                addCharToToken(tp, '&');
                c = getNextChar(stream);
                if (c == '=') {
                    addCharToToken(tp, '&');
                    return makeSubToken(tp, '=', T_ASSIGN, T_LOGICAL_AND_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
                }
                putBackChar(stream, c);
                return makeToken(tp, '&', T_LOGICAL_AND, G_OPERATOR);

            } else if (c == '=') {
                addCharToToken(tp, '&');
                return makeSubToken(tp, c, T_ASSIGN, T_BIT_AND_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
            }
            putBackChar(stream, c);
            return makeToken(tp, '&', T_BIT_AND, G_OPERATOR);

        case '<':
            c = getNextChar(stream);
            if (c == '=') {
                addCharToToken(tp, '<');
                return makeToken(tp, c, T_LE, G_OPERATOR);
            } else if (c == '<') {
                c = getNextChar(stream);
                if (c == '=') {
                    addStringToToken(tp, "<<");
                    return makeSubToken(tp, c, T_ASSIGN, T_LSH_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
                }
                putBackChar(stream, c);
                addCharToToken(tp, '<');
                return makeToken(tp, c, T_LSH, G_OPERATOR);

            } else if (c == '/') {
                addCharToToken(tp, '<');
                return makeToken(tp, c, T_LT_SLASH, 0);
            }
            putBackChar(stream, c);
            return makeToken(tp, '<', T_LT, G_OPERATOR);

        case '=':
            c = getNextChar(stream);
            if (c == '=') {
                c = getNextChar(stream);
                if (c == '=') {
                    addStringToToken(tp, "==");
                    return makeToken(tp, c, T_STRICT_EQ, G_OPERATOR);
                }
                putBackChar(stream, c);
                addCharToToken(tp, '=');
                return makeToken(tp, c, T_EQ, G_OPERATOR);
            }
            putBackChar(stream, c);
            return makeToken(tp, '=', T_ASSIGN, G_OPERATOR);

        case '>':
            c = getNextChar(stream);
            if (c == '=') {
                addCharToToken(tp, '<');
                return makeToken(tp, c, T_GE, G_OPERATOR);
            } else if (c == '>') {
                c = getNextChar(stream);
                if (c == '=') {
                    addStringToToken(tp, ">>");
                    return makeSubToken(tp, c, T_ASSIGN, T_RSH_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
                } else if (c == '>') {
                    c = getNextChar(stream);
                    if (c == '=') {
                        addStringToToken(tp, ">>>");
                        return makeSubToken(tp, c, T_ASSIGN, T_RSH_ZERO_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
                    }
                    putBackChar(stream, c);
                    addStringToToken(tp, ">>");
                    return makeToken(tp, '>', T_RSH_ZERO, G_OPERATOR);
                }
                putBackChar(stream, c);
                addCharToToken(tp, '>');
                return makeToken(tp, '>', T_RSH, G_OPERATOR);
            }
            putBackChar(stream, c);
            return makeToken(tp, '>', T_GT, G_OPERATOR);

        case '^':
            c = getNextChar(stream);
            if (c == '^') {
                addCharToToken(tp, '^');
                c = getNextChar(stream);
                if (c == '=') {
                    addCharToToken(tp, '^');
                    return makeSubToken(tp, '=', T_ASSIGN, T_LOGICAL_XOR_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
                }
                putBackChar(stream, c);
                return makeToken(tp, '^', T_LOGICAL_XOR, G_OPERATOR);

            } else if (c == '=') {
                addCharToToken(tp, '^');
                return makeSubToken(tp, '=', T_ASSIGN, T_BIT_XOR_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
            }
            putBackChar(stream, c);
            return makeToken(tp, '^', T_BIT_XOR, G_OPERATOR);

        case '|':
            c = getNextChar(stream);
            if (c == '|') {
                addCharToToken(tp, '|');
                c = getNextChar(stream);
                if (c == '=') {
                    addCharToToken(tp, '|');
                    return makeSubToken(tp, '=', T_ASSIGN, T_LOGICAL_OR_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);
                }
                putBackChar(stream, c);
                return makeToken(tp, '|', T_LOGICAL_OR, G_OPERATOR);

            } else if (c == '=') {
                addCharToToken(tp, '|');
                return makeSubToken(tp, '=', T_ASSIGN, T_BIT_OR_ASSIGN, G_OPERATOR | G_COMPOUND_ASSIGN);

            }
            putBackChar(stream, c);
            return makeToken(tp, '|', T_BIT_OR, G_OPERATOR);
        }
    }
}


int ecGetRegExpToken(EcInput *input)
{
    EcToken     *token, *tp;
    EcStream    *stream;
    int         c;

    stream = input->stream;
    tp = token = input->token;
    mprAssert(tp != 0);

    initializeToken(tp, stream);
    addCharToToken(tp, '/');

    while (1) {

        c = getNextChar(stream);

        switch (c) {
        case -1:
            return makeToken(tp, 0, T_ERR, 0);

        case 0:
            if (stream->flags & EC_STREAM_EOL) {
                return makeToken(tp, 0, T_NOP, 0);
            }
            return makeToken(tp, 0, T_EOF, 0);

        case '/':
            addCharToToken(tp, '/');
            while (1) {
                c = getNextChar(stream);
                if (c != 'g' && c != 'i' && c != 'm' && c != 'y') {
                    putBackChar(stream, c);
                    break;
                }
                addCharToToken(tp, c);
            }
            return makeToken(tp, 0, T_REGEXP, 0);

        case '\\':
            c = getNextChar(stream);
            if (c == '\r' || c == '\n' || c == 0) {
                ecReportError(input->compiler, "warning", stream->name, stream->lineNumber, 0, stream->column,
                    "Illegal newline in regular expression");
                return makeToken(tp, 0, T_ERR, 0);
            }
            addCharToToken(tp, '\\');
            addCharToToken(tp, c);
            break;

        case '\r':
        case '\n':
            ecReportError(input->compiler, "warning", stream->name, stream->lineNumber, 0, stream->column,
                "Illegal newline in regular expression");
            return makeToken(tp, 0, T_ERR, 0);

        default:
            addCharToToken(tp, c);
        }
    }
}


/*
 *  Put back the current input token
 */
int ecPutToken(EcInput *input)
{
    ecPutSpecificToken(input, input->token);
    input->token = 0;

    return 0;
}


/*
 *  Put the given (specific) token back on the input queue. The current input
 *  token is unaffected.
 */
int ecPutSpecificToken(EcInput *input, EcToken *tp)
{
    mprAssert(tp);
    mprAssert(tp->tokenId > 0);

    /*
     *  Push token back on the input putBack queue
     */
    tp->next = input->putBack;
    input->putBack = tp;

    return 0;
}


/*
 *  Take the current token. Memory ownership retains with input unless the caller calls mprSteal.
 */
EcToken *ecTakeToken(EcInput *input)
{
    EcToken *token;

    token = input->token;
    input->token = 0;
    return token;
}


void ecFreeToken(EcInput *input, EcToken *token)
{
    token->next = input->freeTokens;
    input->freeTokens = token;
}


/*
 *  TODO rationalize with ecParser T_NUMBER code. This could be a lot faster.
 */
static int getNumberToken(EcInput *input, EcToken *tp, int c)
{
    EcStream    *stream;
    int         lowc, isHex, isFloat;

    isHex = isFloat = 0;

    stream = input->stream;
    if (c == '0') {
        addCharToToken(tp, c);
        c = getNextChar(stream);
        if (tolower(c) == 'x') {
            do {
                addCharToToken(tp, c);
                c = getNextChar(stream);
            } while (isxdigit(c));

            putBackChar(stream, c);
            return finishToken(tp, T_NUMBER, -1, 0);
        }
    }

    lowc = tolower(c);
    while (isdigit(lowc) || lowc == '.' || lowc == 'e' || lowc == 'f') {
        if (lowc == '.' || lowc == 'e' || lowc == 'f') {
            isFloat++;
        }
        addCharToToken(tp, c);
        c = getNextChar(stream);
        lowc = tolower(c);
    }

    putBackChar(stream, c);

    return finishToken(tp, T_NUMBER, -1, 0);
}


static int getAlphaToken(EcInput *input, EcToken *tp, int c)
{
    ReservedWord    *rp;
    EcStream        *stream;

    /*
     *  We know that c is an alpha already
     */
    //  TBD -- does ES4 allow $

    stream = input->stream;

    while (isalnum(c) || c == '_' || c == '$' || c == '\\') {
        if (c == '\\') {
            c = getNextChar(stream);
            if (c == '\n' || c == '\r') {
                break;
            } else if (c == 'u') {
                c = decodeNumber(input, 16, 4);
            }
        }
        addCharToToken(tp, c);
        c = getNextChar(stream);
    }
    if (c) {
        putBackChar(stream, c);
    }

    //  TODO - need to take into account contextually reserved and
    //  full reserved words.
    rp = (ReservedWord*) mprLookupHash(input->lexer->keywords, (char*) tp->text);
    if (rp) {
        return finishToken(tp, rp->tokenId, rp->subId, rp->groupMask);
    } else {
        return finishToken(tp, T_ID, -1, 0);
    }
}


//  TODO - handle triple quoting
static int getQuotedToken(EcInput *input, EcToken *tp, int c)
{
    EcStream    *stream;
    int         quoteType;

    stream = input->stream;
    quoteType = c;

    for (c = getNextChar(stream); c && c != quoteType; c = getNextChar(stream)) {
        if (c == 0) {
            return makeToken(tp, 0, T_ERR, 0);
        }
        if (c == '\\') {
            c = getNextChar(stream);
            switch (c) {
            //  TBD -- others
            case '\\':
                break;
            case '\'':
            case '\"':
                break;
            case 'b':
                c = '\b';
                break;
            case 'f':
                c = '\f';
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case 'u':
            case 'x':
                c = decodeNumber(input, 16, 4);
                break;
            case '0':
                c = decodeNumber(input, 8, 3);
                break;
            default:
                break;
            }
        }
        addCharToToken(tp, c);
    }
    return finishToken(tp, T_STRING, -1, 0);
}


static int decodeNumber(EcInput *input, int radix, int length)
{
    char        buf[16];
    int         i, c, lowerc;

    for (i = 0; i < length; i++) {
        c = getNextChar(input->stream);
        if (c == 0) {
            break;
        }
        if (radix <= 10) {
            if (!isdigit(c)) {
                break;
            }
        } else if (radix == 16) {
            lowerc = tolower(c);
            if (!isdigit(c) && !('a' <= c && c <= 'f')) {
                break;
            }
        }
        buf[i] = c;
    }

    if (i < length) {
        putBackChar(input->stream, c);
    }

    buf[i] = '\0';
    return mprAtoi(buf, radix);
}


/*
 *  C, C++ and doc style comments. Return token or zero for no token.
 */
static int getComment(EcInput *input, EcToken *tp, int c)
{
    EcStream    *stream;
    int         form, startLine;

    startLine = tp->stream->lineNumber;

    stream = input->stream;
    form = c;

    //  TODO - would be great to warn about comment nested in a comment.

    for (form = c; c > 0;) {
        c = getNextChar(stream);
        if (c <= 0) {
            /*
             *  Unterminated Comment
             */
            addFormattedStringToToken(tp, "Unterminated comment starting on line %d", startLine);
            makeToken(tp, 0, form == '/' ? T_EOF: T_ERR, 0);
            return 1;
        }

        if (form == '/') {
            if (c == '\n') {
                break;
            }

        } else {
            if (c == '*') {
                c = getNextChar(stream);
                if (c == '/') {
                    break;
                }
                addCharToToken(tp, '*');
                putBackChar(stream, c);

            } else if (c == '/') {
                c = getNextChar(stream);
                if (c == '*') {
                    /*
                     *  Nested comment
                     */
                    if (input->compiler->warnLevel > 0) {
                        ecReportError(input->compiler, "warning", stream->name, stream->lineNumber, 0,
                                stream->column, "Possible nested comment");
                    }
                }
                addCharToToken(tp, '/');
            }
        }
        addCharToToken(tp, c);
    }

    return 0;
}


//  TODO OPT
static int addCharToToken(EcToken *tp, int c)
{
    if (tp->currentLine == 0) {
        setTokenCurrentLine(tp);
    }

    if (tp->textLen >= (tp->textBufSize - 1)) {
        //  TODO - seem to be reallocing a lot. Are tokens being reused?
        tp->text = (uchar*) mprRealloc(tp, tp->text, tp->textBufSize += EC_TOKEN_INCR);
        if (tp->text == 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }
    tp->text[tp->textLen++] = c;

    mprAssert(tp->textLen < tp->textBufSize);
    tp->text[tp->textLen] = '\0';
    return 0;
}


static int addStringToToken(EcToken *tp, char *str)
{
    char    *cp;

    for (cp = str; *cp; cp++) {
        if (addCharToToken(tp, *cp) < 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }
    return 0;
}


static int addFormattedStringToToken(EcToken *tp, char *fmt, ...)
{
    va_list     args;
    char        *buf;

    va_start(args, fmt);
    mprAllocVsprintf(tp, &buf, MPR_MAX_STRING, fmt, args);
    addStringToToken(tp, buf);
    mprFree(buf);
    va_end(args);

    return 0;
}


/*
 *  Called at the start of every token to initialize the token
 */
static void initializeToken(EcToken *tp, EcStream *stream)
{
    tp->textLen = 0;
    tp->stream = stream;

    if (tp->lineNumber != stream->lineNumber) {
        tp->currentLine = 0;
    }
}


/*
 *  Set the token's source debug information. CurrentLine is less one because we have
 *  already consumed one character.
 */
static void setTokenCurrentLine(EcToken *tp)
{
    tp->currentLine = tp->stream->currentLine;
    tp->lineNumber = tp->stream->lineNumber;
    tp->column = tp->stream->column - 1;
    tp->filename = tp->stream->name;
}


/*
 *  Called to complete any token
 */
static int finishToken(EcToken *tp, int tokenId, int subId, int groupMask)
{
    EcStream        *stream;
    char            *end;
    int             len;

    mprAssert(tp);
    stream = tp->stream;
    mprAssert(stream);

    tp->tokenId = tokenId;
    tp->subId = subId;
    tp->groupMask = groupMask;

    /*
     *  TODO optimize this. Would be really nice to tokenize the input and just return pointers into the stream->buf
     */
    if (tp->currentLine == 0) {
        setTokenCurrentLine(tp);
    }
    if (tp->currentLine) {
        end = strchr(tp->currentLine, '\n');
        len = end ? (int) (end - tp->currentLine) : (int) strlen(tp->currentLine);

        tp->currentLine = (char*) mprMemdup(tp, tp->currentLine, len + 1);
        tp->currentLine[len] = '\0';
        mprAssert(tp->currentLine);

        mprLog(tp, 9, "Lex lineNumber %d \"%s\" %s", tp->lineNumber, tp->text, tp->currentLine);
    }

    return tokenId;
}


static int makeToken(EcToken *tp, int c, int tokenId, int groupMask)
{
    if (c && addCharToToken(tp, c) < 0) {
        return T_ERR;
    }
    return finishToken(tp, tokenId, -1, groupMask);
}


static int makeSubToken(EcToken *tp, int c, int tokenId, int subId, int groupMask)
{
    if (addCharToToken(tp, c) < 0) {
        return T_ERR;
    }
    return finishToken(tp, tokenId, subId, groupMask);
}


/*
 *  Get the next input char from the input stream.
 */
static int getNextChar(EcStream *stream)
{
    int         c;

    if (stream->nextChar >= stream->end && stream->gets) {

        if (stream->gets(stream) < 0) {
            return 0;
        }
    }

    if (stream->nextChar < stream->end) {
        c = *stream->nextChar++;
        if (c == '\n') {
            stream->lineNumber++;
            stream->lastColumn = stream->column;
            stream->column = 0;
            stream->lastLine = stream->currentLine;
            stream->currentLine = stream->nextChar;
        } else {
            stream->column++;
        }
        return c;
    }

    return 0;
}


/*
 *  Put back a character onto the input stream.
 */
static void putBackChar(EcStream *stream, int c)
{
    if (stream->buf < stream->nextChar && c) {
        stream->nextChar--;
        mprAssert(c == *stream->nextChar);
        if (c == '\n') {
            stream->currentLine = stream->lastLine;
            stream->column = stream->lastColumn + 1;
            stream->lineNumber--;
            mprAssert(stream->lineNumber >= 0);
        }
        stream->column--;
        mprAssert(stream->column >= 0);
    }
}


char *ecGetInputStreamName(EcLexer *lp)
{
    mprAssert(lp);
    mprAssert(lp->input);
    mprAssert(lp->input->stream);

    return lp->input->stream->name;
}


int ecOpenFileStream(EcLexer *lp, const char *path)
{
    EcFileStream    *fs;
    MprFileInfo     info;
    int             c;

    fs = mprAllocObjZeroed(lp->input, EcFileStream);
    if (fs == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    if ((fs->file = mprOpen(lp, path, O_RDONLY | O_BINARY, 0666)) == 0) {
        mprFree(fs);
        return MPR_ERR_CANT_OPEN;
    }

    if (mprGetFileInfo(fs, path, &info) < 0 || info.size < 0) {
        mprFree(fs);
        return MPR_ERR_CANT_ACCESS;
    }

    /* Sanity check */
    mprAssert(info.size < (10 * 1024 * 1024));
    mprAssert(info.size >= 0);

    fs->stream.buf = (char*) mprAlloc(fs, (int) info.size + 1);
    if (fs->stream.buf == 0) {
        mprFree(fs);
        return MPR_ERR_NO_MEMORY;
    }
    if (mprRead(fs->file, fs->stream.buf, (int) info.size) != (int) info.size) {
        mprFree(fs);
        return MPR_ERR_CANT_READ;
    }

    fs->stream.buf[info.size] = '\0';
    fs->stream.nextChar = fs->stream.buf;
    fs->stream.end = &fs->stream.buf[info.size];
    fs->stream.currentLine = fs->stream.buf;
    fs->stream.lineNumber = 1;
    fs->stream.compiler = lp->compiler;

    fs->stream.name = mprStrdup(lp, path);

    mprFree(lp->input->stream);
    lp->input->stream = (EcStream*) fs;

    lp->input->putBack = 0;
    lp->input->token = 0;
    lp->input->state = 0;
    lp->input->next = 0;

    /*
     *  Initialize the stream line and column data.
     */
    c = getNextChar(&fs->stream);
    putBackChar(&fs->stream, c);



    return 0;
}


int ecOpenMemoryStream(EcLexer *lp, const uchar *buf, int len)
{
    EcMemStream     *ms;
    int             c;

    ms = mprAllocObjZeroed(lp->input, EcMemStream);
    if (ms == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    ms->stream.lineNumber = 0;

    ms->stream.buf = mprMemdup(ms, buf, len);
    ms->stream.nextChar = ms->stream.buf;
    ms->stream.end = &ms->stream.buf[len];
    ms->stream.currentLine = ms->stream.buf;
    ms->stream.lineNumber = 1;
    ms->stream.compiler = lp->compiler;

    mprFree(lp->input->stream);
    lp->input->stream = (EcStream*) ms;

    lp->input->putBack = 0;
    lp->input->token = 0;
    lp->input->state = 0;
    lp->input->next = 0;

    /*
     *  Initialize the stream line and column data.
     */
    c = getNextChar(&ms->stream);
    putBackChar(&ms->stream, c);

    return 0;
}


int ecOpenConsoleStream(EcLexer *lp, EcStreamGet gets)
{
    EcConsoleStream     *cs;

    cs = mprAllocObjZeroed(lp->input, EcConsoleStream);
    if (cs == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    /*
     *  TODO - need API for this
     */
    cs->stream.lineNumber = 0;
    cs->stream.nextChar = 0;
    cs->stream.end = 0;
    cs->stream.currentLine = 0;
    cs->stream.gets = gets;
    cs->stream.compiler = lp->compiler;

    /*
     *  TODO - need API for this
     */
    mprFree(lp->input->stream);
    lp->input->stream = (EcStream*) cs;

    lp->input->putBack = 0;
    lp->input->token = 0;
    lp->input->state = 0;
    lp->input->next = 0;

    return 0;
}


void ecCloseStream(EcLexer *lp)
{
    /*
     *  This will close file streams
     */
    mprFree(lp->input->stream);
    lp->input->stream = 0;
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
