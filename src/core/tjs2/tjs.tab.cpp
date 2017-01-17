#include "tjsCommHead.h"

/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 1 "syntax/tjs.y"

/*---------------------------------------------------------------------------*/
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
/*---------------------------------------------------------------------------*/
/* tjs.y */
/* TJS2 bison input file */


#include <memory>


#include "tjsInterCodeGen.h"
#include "tjsScriptBlock.h"
#include "tjsError.h"
#include "tjsArray.h"
#include "tjsDictionary.h"

#define YYMALLOC	::malloc
#define YYREALLOC	::realloc
#define YYFREE		::free

/* param */
#define YYPARSE_PARAM pm
#define YYLEX_PARAM pm

/* script block */
#define sb ((tTJSScriptBlock*)pm)

/* current context */
#define cc (sb->GetCurrentContext())

/* current node */
#define cn (cc->GetCurrentNode())

/* lexical analyzer */
#define lx (sb->GetLexicalAnalyzer())


namespace TJS {

/* yylex/yyerror prototype decl */
#define YYLEX_PROTO_DECL int yylex(YYSTYPE *yylex, void *pm);

int __yyerror(char * msg, void *pm);


#define yyerror(msg) __yyerror(msg, pm);



/* Line 189 of yacc.c  */
#line 129 "tjs.tab.cpp"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_COMMA = 258,
     T_EQUAL = 259,
     T_AMPERSANDEQUAL = 260,
     T_VERTLINEEQUAL = 261,
     T_CHEVRONEQUAL = 262,
     T_MINUSEQUAL = 263,
     T_PLUSEQUAL = 264,
     T_PERCENTEQUAL = 265,
     T_SLASHEQUAL = 266,
     T_BACKSLASHEQUAL = 267,
     T_ASTERISKEQUAL = 268,
     T_LOGICALOREQUAL = 269,
     T_LOGICALANDEQUAL = 270,
     T_RBITSHIFTEQUAL = 271,
     T_LARITHSHIFTEQUAL = 272,
     T_RARITHSHIFTEQUAL = 273,
     T_QUESTION = 274,
     T_LOGICALOR = 275,
     T_LOGICALAND = 276,
     T_VERTLINE = 277,
     T_CHEVRON = 278,
     T_AMPERSAND = 279,
     T_NOTEQUAL = 280,
     T_EQUALEQUAL = 281,
     T_DISCNOTEQUAL = 282,
     T_DISCEQUAL = 283,
     T_SWAP = 284,
     T_LT = 285,
     T_GT = 286,
     T_LTOREQUAL = 287,
     T_GTOREQUAL = 288,
     T_RARITHSHIFT = 289,
     T_LARITHSHIFT = 290,
     T_RBITSHIFT = 291,
     T_PERCENT = 292,
     T_SLASH = 293,
     T_BACKSLASH = 294,
     T_ASTERISK = 295,
     T_EXCRAMATION = 296,
     T_TILDE = 297,
     T_DECREMENT = 298,
     T_INCREMENT = 299,
     T_NEW = 300,
     T_DELETE = 301,
     T_TYPEOF = 302,
     T_PLUS = 303,
     T_MINUS = 304,
     T_SHARP = 305,
     T_DOLLAR = 306,
     T_ISVALID = 307,
     T_INVALIDATE = 308,
     T_INSTANCEOF = 309,
     T_LPARENTHESIS = 310,
     T_DOT = 311,
     T_LBRACKET = 312,
     T_THIS = 313,
     T_SUPER = 314,
     T_GLOBAL = 315,
     T_RBRACKET = 316,
     T_CLASS = 317,
     T_RPARENTHESIS = 318,
     T_COLON = 319,
     T_SEMICOLON = 320,
     T_LBRACE = 321,
     T_RBRACE = 322,
     T_CONTINUE = 323,
     T_FUNCTION = 324,
     T_DEBUGGER = 325,
     T_DEFAULT = 326,
     T_CASE = 327,
     T_EXTENDS = 328,
     T_FINALLY = 329,
     T_PROPERTY = 330,
     T_PRIVATE = 331,
     T_PUBLIC = 332,
     T_PROTECTED = 333,
     T_STATIC = 334,
     T_RETURN = 335,
     T_BREAK = 336,
     T_EXPORT = 337,
     T_IMPORT = 338,
     T_SWITCH = 339,
     T_IN = 340,
     T_INCONTEXTOF = 341,
     T_FOR = 342,
     T_WHILE = 343,
     T_DO = 344,
     T_IF = 345,
     T_VAR = 346,
     T_CONST = 347,
     T_ENUM = 348,
     T_GOTO = 349,
     T_THROW = 350,
     T_TRY = 351,
     T_SETTER = 352,
     T_GETTER = 353,
     T_ELSE = 354,
     T_CATCH = 355,
     T_OMIT = 356,
     T_SYNCHRONIZED = 357,
     T_WITH = 358,
     T_INT = 359,
     T_REAL = 360,
     T_STRING = 361,
     T_OCTET = 362,
     T_FALSE = 363,
     T_NULL = 364,
     T_TRUE = 365,
     T_VOID = 366,
     T_NAN = 367,
     T_INFINITY = 368,
     T_UPLUS = 369,
     T_UMINUS = 370,
     T_EVAL = 371,
     T_POSTDECREMENT = 372,
     T_POSTINCREMENT = 373,
     T_IGNOREPROP = 374,
     T_PROPACCESS = 375,
     T_ARG = 376,
     T_EXPANDARG = 377,
     T_INLINEARRAY = 378,
     T_ARRAYARG = 379,
     T_INLINEDIC = 380,
     T_DICELM = 381,
     T_WITHDOT = 382,
     T_THIS_PROXY = 383,
     T_WITHDOT_PROXY = 384,
     T_CONSTVAL = 385,
     T_SYMBOL = 386,
     T_REGEXP = 387,
     T_VARIANT = 388
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 60 "syntax/tjs.y"

	tjs_int			num;
	tTJSExprNode *		np;



/* Line 214 of yacc.c  */
#line 305 "tjs.tab.cpp"
} YYSTYPE;
YYLEX_PROTO_DECL

# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 317 "tjs.tab.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1527

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  134
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  108
/* YYNRULES -- Number of rules.  */
#define YYNRULES  271
/* YYNRULES -- Number of states.  */
#define YYNSTATES  458

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   388

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    10,    13,    17,    19,
      21,    23,    26,    28,    30,    32,    34,    36,    39,    42,
      45,    47,    49,    51,    53,    55,    57,    59,    61,    63,
      65,    66,    71,    72,    73,    81,    82,    83,    93,    94,
      95,   103,   104,   109,   119,   120,   121,   124,   126,   127,
     129,   130,   132,   135,   138,   141,   143,   147,   150,   155,
     156,   159,   162,   165,   168,   171,   174,   175,   182,   183,
     189,   190,   194,   198,   204,   205,   207,   209,   213,   216,
     221,   223,   227,   228,   235,   237,   239,   242,   245,   246,
     254,   255,   259,   264,   267,   268,   274,   275,   278,   279,
     285,   287,   291,   293,   296,   300,   301,   308,   309,   316,
     320,   323,   324,   330,   332,   336,   341,   345,   347,   349,
     353,   355,   359,   361,   365,   369,   373,   377,   381,   385,
     389,   393,   397,   401,   405,   409,   413,   417,   421,   425,
     427,   433,   435,   439,   441,   445,   447,   451,   453,   457,
     459,   463,   465,   469,   473,   477,   481,   483,   487,   491,
     495,   499,   501,   505,   509,   513,   515,   519,   523,   525,
     529,   533,   537,   540,   543,   545,   548,   551,   554,   557,
     560,   563,   566,   569,   572,   575,   578,   581,   584,   587,
     590,   593,   597,   602,   605,   610,   613,   618,   621,   623,
     627,   629,   633,   638,   640,   641,   646,   649,   652,   655,
     656,   660,   662,   664,   666,   668,   670,   672,   674,   676,
     678,   680,   682,   683,   687,   688,   692,   697,   699,   701,
     705,   706,   708,   710,   712,   713,   718,   720,   724,   725,
     727,   728,   735,   736,   738,   742,   746,   750,   751,   753,
     754,   762,   763,   765,   767,   771,   774,   777,   779,   781,
     783,   785,   786,   795,   796,   798,   802,   807,   812,   816,
     820,   824
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     135,     0,    -1,   136,    -1,    -1,   137,   138,    -1,    -1,
     138,   139,    -1,   138,     1,    65,    -1,   140,    -1,   141,
      -1,    65,    -1,   198,    65,    -1,   149,    -1,   152,    -1,
     143,    -1,   146,    -1,   154,    -1,    81,    65,    -1,    68,
      65,    -1,    70,    65,    -1,   159,    -1,   164,    -1,   173,
      -1,   181,    -1,   187,    -1,   188,    -1,   190,    -1,   192,
      -1,   193,    -1,   196,    -1,    -1,    66,   142,   138,    67,
      -1,    -1,    -1,    88,   144,    55,   198,    63,   145,   139,
      -1,    -1,    -1,    89,   147,   139,    88,    55,   198,    63,
     148,    65,    -1,    -1,    -1,    90,    55,   150,   198,   151,
      63,   139,    -1,    -1,   149,    99,   153,   139,    -1,    87,
      55,   155,    65,   157,    65,   158,    63,   139,    -1,    -1,
      -1,   156,   160,    -1,   198,    -1,    -1,   198,    -1,    -1,
     198,    -1,   160,    65,    -1,    91,   161,    -1,    92,   161,
      -1,   162,    -1,   161,     3,   162,    -1,   131,   163,    -1,
     131,   163,     4,   197,    -1,    -1,    64,   131,    -1,    64,
     111,    -1,    64,   104,    -1,    64,   105,    -1,    64,   106,
      -1,    64,   107,    -1,    -1,    69,   131,   165,   168,   163,
     141,    -1,    -1,    69,   167,   168,   163,   141,    -1,    -1,
      55,   172,    63,    -1,    55,   169,    63,    -1,    55,   170,
       3,   172,    63,    -1,    -1,   170,    -1,   171,    -1,   170,
       3,   171,    -1,   131,   163,    -1,   131,   163,     4,   197,
      -1,    40,    -1,   131,   163,    40,    -1,    -1,    75,   131,
      66,   174,   175,    67,    -1,   176,    -1,   178,    -1,   176,
     178,    -1,   178,   176,    -1,    -1,    97,    55,   131,   163,
      63,   177,   141,    -1,    -1,   180,   179,   141,    -1,    98,
      55,    63,   163,    -1,    98,   163,    -1,    -1,    62,   131,
     182,   183,   141,    -1,    -1,    73,   197,    -1,    -1,    73,
     197,     3,   184,   185,    -1,   186,    -1,   185,     3,   186,
      -1,   197,    -1,    80,    65,    -1,    80,   198,    65,    -1,
      -1,    84,    55,   198,    63,   189,   141,    -1,    -1,   103,
      55,   198,    63,   191,   139,    -1,    72,   198,    64,    -1,
      71,    64,    -1,    -1,    96,   194,   139,   195,   139,    -1,
     100,    -1,   100,    55,    63,    -1,   100,    55,   131,    63,
      -1,    95,   198,    65,    -1,   200,    -1,   199,    -1,   199,
      90,   198,    -1,   200,    -1,   199,     3,   200,    -1,   201,
      -1,   201,    29,   200,    -1,   201,     4,   200,    -1,   201,
       5,   200,    -1,   201,     6,   200,    -1,   201,     7,   200,
      -1,   201,     8,   200,    -1,   201,     9,   200,    -1,   201,
      10,   200,    -1,   201,    11,   200,    -1,   201,    12,   200,
      -1,   201,    13,   200,    -1,   201,    14,   200,    -1,   201,
      15,   200,    -1,   201,    18,   200,    -1,   201,    17,   200,
      -1,   201,    16,   200,    -1,   202,    -1,   202,    19,   201,
      64,   201,    -1,   203,    -1,   202,    20,   203,    -1,   204,
      -1,   203,    21,   204,    -1,   205,    -1,   204,    22,   205,
      -1,   206,    -1,   205,    23,   206,    -1,   207,    -1,   206,
      24,   207,    -1,   208,    -1,   207,    25,   208,    -1,   207,
      26,   208,    -1,   207,    27,   208,    -1,   207,    28,   208,
      -1,   209,    -1,   208,    30,   209,    -1,   208,    31,   209,
      -1,   208,    32,   209,    -1,   208,    33,   209,    -1,   210,
      -1,   209,    34,   210,    -1,   209,    35,   210,    -1,   209,
      36,   210,    -1,   211,    -1,   210,    48,   211,    -1,   210,
      49,   211,    -1,   213,    -1,   211,    37,   213,    -1,   211,
      38,   213,    -1,   211,    39,   213,    -1,   212,   213,    -1,
     211,    40,    -1,   214,    -1,    41,   213,    -1,    42,   213,
      -1,    43,   213,    -1,    44,   213,    -1,    45,   221,    -1,
      53,   213,    -1,    52,   213,    -1,   214,    52,    -1,    46,
     213,    -1,    47,   213,    -1,    50,   213,    -1,    51,   213,
      -1,    48,   213,    -1,    49,   213,    -1,    24,   213,    -1,
      40,   213,    -1,   214,    54,   213,    -1,    55,   104,    63,
     213,    -1,   104,   213,    -1,    55,   105,    63,   213,    -1,
     105,   213,    -1,    55,   106,    63,   213,    -1,   106,   213,
      -1,   215,    -1,   215,    86,   214,    -1,   218,    -1,    55,
     198,    63,    -1,   215,    57,   198,    61,    -1,   221,    -1,
      -1,   215,    56,   216,   131,    -1,   215,    44,    -1,   215,
      43,    -1,   215,    41,    -1,    -1,    56,   217,   131,    -1,
     130,    -1,   131,    -1,    58,    -1,    59,    -1,   166,    -1,
      60,    -1,   111,    -1,   224,    -1,   228,    -1,   233,    -1,
     238,    -1,    -1,    11,   219,   132,    -1,    -1,    38,   220,
     132,    -1,   215,    55,   222,    63,    -1,   101,    -1,   223,
      -1,   222,     3,   223,    -1,    -1,    40,    -1,   212,    -1,
     197,    -1,    -1,    57,   225,   226,    61,    -1,   227,    -1,
     226,     3,   227,    -1,    -1,   197,    -1,    -1,    37,    57,
     229,   230,   232,    61,    -1,    -1,   231,    -1,   230,     3,
     231,    -1,   197,     3,   197,    -1,   131,    64,   197,    -1,
      -1,     3,    -1,    -1,    55,    92,    63,    57,   234,   235,
      61,    -1,    -1,   236,    -1,   237,    -1,   236,     3,   237,
      -1,    49,   130,    -1,    48,   130,    -1,   130,    -1,   111,
      -1,   233,    -1,   238,    -1,    -1,    55,    92,    63,    37,
      57,   239,   240,    61,    -1,    -1,   241,    -1,   240,     3,
     241,    -1,   130,     3,    49,   130,    -1,   130,     3,    48,
     130,    -1,   130,     3,   130,    -1,   130,     3,   111,    -1,
     130,     3,   233,    -1,   130,     3,   238,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   217,   217,   222,   222,   228,   230,   231,   238,   239,
     244,   245,   246,   247,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     268,   268,   275,   276,   275,   282,   285,   282,   291,   292,
     291,   298,   298,   304,   314,   315,   315,   317,   323,   324,
     329,   330,   335,   339,   340,   347,   348,   353,   355,   360,
     362,   363,   364,   365,   366,   367,   372,   372,   382,   382,
     395,   397,   398,   399,   403,   405,   409,   410,   414,   416,
     421,   423,   435,   434,   443,   444,   445,   446,   450,   450,
     461,   461,   470,   471,   477,   477,   484,   486,   487,   487,
     492,   493,   497,   502,   503,   510,   509,   517,   516,   523,
     524,   529,   529,   536,   537,   538,   544,   549,   553,   554,
     559,   560,   565,   566,   567,   568,   569,   570,   571,   572,
     573,   574,   575,   576,   577,   578,   579,   580,   581,   586,
     587,   595,   596,   600,   601,   606,   607,   611,   612,   616,
     617,   621,   622,   623,   624,   625,   629,   630,   631,   632,
     633,   637,   638,   639,   640,   645,   646,   647,   651,   652,
     653,   654,   655,   659,   663,   664,   665,   666,   667,   668,
     669,   670,   671,   672,   673,   674,   675,   676,   677,   678,
     679,   680,   681,   682,   683,   684,   685,   686,   690,   691,
     696,   697,   698,   699,   700,   700,   704,   705,   706,   707,
     707,   715,   717,   720,   721,   722,   723,   724,   725,   726,
     727,   728,   729,   729,   732,   732,   740,   745,   746,   747,
     751,   752,   753,   754,   760,   760,   769,   770,   775,   776,
     781,   781,   791,   793,   794,   799,   800,   807,   809,   816,
     816,   826,   828,   834,   835,   841,   842,   843,   844,   845,
     846,   851,   851,   863,   865,   866,   871,   872,   873,   874,
     875,   876
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "\",\"", "\"=\"", "\"&=\"", "\"|=\"",
  "\"^=\"", "\"-=\"", "\"+=\"", "\"%=\"", "\"/=\"", "\"\\\\=\"", "\"*=\"",
  "\"||=\"", "\"&&=\"", "\">>>=\"", "\"<<=\"", "\">>=\"", "\"?\"",
  "\"||\"", "\"&&\"", "\"|\"", "\"^\"", "\"&\"", "\"!=\"", "\"==\"",
  "\"!==\"", "\"===\"", "\"<->\"", "\"<\"", "\">\"", "\"<=\"", "\">=\"",
  "\">>\"", "\"<<\"", "\">>>\"", "\"%\"", "\"/\"", "\"\\\\\"", "\"*\"",
  "\"!\"", "\"~\"", "\"--\"", "\"++\"", "\"new\"", "\"delete\"",
  "\"typeof\"", "\"+\"", "\"-\"", "\"#\"", "\"$\"", "\"isvalid\"",
  "\"invalidate\"", "\"instanceof\"", "\"(\"", "\".\"", "\"[\"",
  "\"this\"", "\"super\"", "\"global\"", "\"]\"", "\"class\"", "\")\"",
  "\":\"", "\";\"", "\"{\"", "\"}\"", "\"continue\"", "\"function\"",
  "\"debugger\"", "\"default\"", "\"case\"", "\"extends\"", "\"finally\"",
  "\"property\"", "\"private\"", "\"public\"", "\"protected\"",
  "\"static\"", "\"return\"", "\"break\"", "\"export\"", "\"import\"",
  "\"switch\"", "\"in\"", "\"incontextof\"", "\"for\"", "\"while\"",
  "\"do\"", "\"if\"", "\"var\"", "\"const\"", "\"enum\"", "\"goto\"",
  "\"throw\"", "\"try\"", "\"setter\"", "\"getter\"", "\"else\"",
  "\"catch\"", "\"...\"", "\"synchronized\"", "\"with\"", "\"int\"",
  "\"real\"", "\"string\"", "\"octet\"", "\"false\"", "\"null\"",
  "\"true\"", "\"void\"", "\"NaN\"", "\"Infinity\"", "T_UPLUS", "T_UMINUS",
  "T_EVAL", "T_POSTDECREMENT", "T_POSTINCREMENT", "T_IGNOREPROP",
  "T_PROPACCESS", "T_ARG", "T_EXPANDARG", "T_INLINEARRAY", "T_ARRAYARG",
  "T_INLINEDIC", "T_DICELM", "T_WITHDOT", "T_THIS_PROXY",
  "T_WITHDOT_PROXY", "T_CONSTVAL", "T_SYMBOL", "T_REGEXP", "T_VARIANT",
  "$accept", "program", "global_list", "$@1", "def_list",
  "block_or_statement", "statement", "block", "$@2", "while", "$@3", "$@4",
  "do_while", "$@5", "$@6", "if", "$@7", "$@8", "if_else", "$@9", "for",
  "for_first_clause", "$@10", "for_second_clause", "for_third_clause",
  "variable_def", "variable_def_inner", "variable_id_list", "variable_id",
  "variable_type", "func_def", "$@11", "func_expr_def", "$@12",
  "func_decl_arg_opt", "func_decl_arg_list", "func_decl_arg_at_least_one",
  "func_decl_arg", "func_decl_arg_collapse", "property_def", "$@13",
  "property_handler_def_list", "property_handler_setter", "$@14",
  "property_handler_getter", "$@15", "property_getter_handler_head",
  "class_def", "$@16", "class_extender", "$@17", "extends_list",
  "extends_name", "return", "switch", "$@18", "with", "$@19", "case",
  "try", "$@20", "catch", "throw", "expr_no_comma", "expr", "comma_expr",
  "assign_expr", "cond_expr", "logical_or_expr", "logical_and_expr",
  "inclusive_or_expr", "exclusive_or_expr", "and_expr", "identical_expr",
  "compare_expr", "shift_expr", "add_sub_expr", "mul_div_expr",
  "mul_div_expr_and_asterisk", "unary_expr", "incontextof_expr",
  "priority_expr", "$@21", "$@22", "factor_expr", "$@23", "$@24",
  "func_call_expr", "call_arg_list", "call_arg", "inline_array", "$@25",
  "array_elm_list", "array_elm", "inline_dic", "$@26", "dic_elm_list",
  "dic_elm", "dic_dummy_elm_opt", "const_inline_array", "$@27",
  "const_array_elm_list_opt", "const_array_elm_list", "const_array_elm",
  "const_inline_dic", "$@28", "const_dic_elm_list", "const_dic_elm", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   134,   135,   137,   136,   138,   138,   138,   139,   139,
     140,   140,   140,   140,   140,   140,   140,   140,   140,   140,
     140,   140,   140,   140,   140,   140,   140,   140,   140,   140,
     142,   141,   144,   145,   143,   147,   148,   146,   150,   151,
     149,   153,   152,   154,   155,   156,   155,   155,   157,   157,
     158,   158,   159,   160,   160,   161,   161,   162,   162,   163,
     163,   163,   163,   163,   163,   163,   165,   164,   167,   166,
     168,   168,   168,   168,   169,   169,   170,   170,   171,   171,
     172,   172,   174,   173,   175,   175,   175,   175,   177,   176,
     179,   178,   180,   180,   182,   181,   183,   183,   184,   183,
     185,   185,   186,   187,   187,   189,   188,   191,   190,   192,
     192,   194,   193,   195,   195,   195,   196,   197,   198,   198,
     199,   199,   200,   200,   200,   200,   200,   200,   200,   200,
     200,   200,   200,   200,   200,   200,   200,   200,   200,   201,
     201,   202,   202,   203,   203,   204,   204,   205,   205,   206,
     206,   207,   207,   207,   207,   207,   208,   208,   208,   208,
     208,   209,   209,   209,   209,   210,   210,   210,   211,   211,
     211,   211,   211,   212,   213,   213,   213,   213,   213,   213,
     213,   213,   213,   213,   213,   213,   213,   213,   213,   213,
     213,   213,   213,   213,   213,   213,   213,   213,   214,   214,
     215,   215,   215,   215,   216,   215,   215,   215,   215,   217,
     215,   218,   218,   218,   218,   218,   218,   218,   218,   218,
     218,   218,   219,   218,   220,   218,   221,   222,   222,   222,
     223,   223,   223,   223,   225,   224,   226,   226,   227,   227,
     229,   228,   230,   230,   230,   231,   231,   232,   232,   234,
     233,   235,   235,   236,   236,   237,   237,   237,   237,   237,
     237,   239,   238,   240,   240,   240,   241,   241,   241,   241,
     241,   241
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     0,     2,     0,     2,     3,     1,     1,
       1,     2,     1,     1,     1,     1,     1,     2,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     4,     0,     0,     7,     0,     0,     9,     0,     0,
       7,     0,     4,     9,     0,     0,     2,     1,     0,     1,
       0,     1,     2,     2,     2,     1,     3,     2,     4,     0,
       2,     2,     2,     2,     2,     2,     0,     6,     0,     5,
       0,     3,     3,     5,     0,     1,     1,     3,     2,     4,
       1,     3,     0,     6,     1,     1,     2,     2,     0,     7,
       0,     3,     4,     2,     0,     5,     0,     2,     0,     5,
       1,     3,     1,     2,     3,     0,     6,     0,     6,     3,
       2,     0,     5,     1,     3,     4,     3,     1,     1,     3,
       1,     3,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     1,
       5,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     3,     3,     3,     3,     1,     3,     3,     3,
       3,     1,     3,     3,     3,     1,     3,     3,     1,     3,
       3,     3,     2,     2,     1,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     3,     4,     2,     4,     2,     4,     2,     1,     3,
       1,     3,     4,     1,     0,     4,     2,     2,     2,     0,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     3,     0,     3,     4,     1,     1,     3,
       0,     1,     1,     1,     0,     4,     1,     3,     0,     1,
       0,     6,     0,     1,     3,     3,     3,     0,     1,     0,
       7,     0,     1,     1,     3,     2,     2,     1,     1,     1,
       1,     0,     8,     0,     1,     3,     4,     4,     3,     3,
       3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       3,     0,     2,     5,     1,     0,     0,   222,     0,     0,
     224,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   209,   234,   213,   214,
     216,     0,    10,    30,     0,    68,     0,     0,     0,     0,
       0,     0,     0,     0,    32,    35,     0,     0,     0,     0,
     111,     0,     0,     0,     0,   217,   211,   212,     6,     8,
       9,    14,    15,    12,    13,    16,    20,     0,    21,   215,
      22,    23,    24,    25,    26,    27,    28,    29,     0,   118,
     120,   122,   139,   141,   143,   145,   147,   149,   151,   156,
     161,   165,     0,   168,   174,   198,   200,   203,   218,   219,
     220,   221,     7,     0,    68,   189,   240,     0,   190,   175,
     176,   177,   178,     0,     0,   179,   183,   184,   187,   188,
     185,   186,   181,   180,     0,     0,     0,     0,     0,     0,
     238,    94,     5,    18,    66,    70,    19,   110,     0,     0,
     103,     0,    17,     0,    45,     0,     0,    38,    59,    53,
      55,    54,     0,     0,     0,   193,   195,   197,    41,    52,
      11,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   173,   172,   182,     0,   208,   207,   206,   230,   204,
       0,     0,   223,   242,   225,     0,     0,     0,     0,   201,
     210,   239,   117,     0,   236,    96,     0,    70,    74,    59,
     109,    82,   104,     0,     0,     0,    47,     0,     0,     0,
       0,    57,     0,   116,     0,     0,     0,   121,   119,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   138,   137,   136,   123,     0,   142,   144,   146,   148,
     150,   152,   153,   154,   155,   157,   158,   159,   160,   162,
     163,   164,   166,   167,   169,   170,   171,   191,   231,   227,
     233,   232,     0,   228,     0,     0,   199,   212,     0,   247,
     243,     0,   249,   192,   194,   196,   238,   235,     0,     0,
      31,    59,    80,    59,     0,    75,    76,     0,     0,     0,
     105,    48,    46,     0,     0,    39,    62,    63,    64,    65,
      61,    60,     0,    56,   113,     0,   107,    42,     0,   230,
     226,   205,   202,     0,     0,   248,     0,   261,   251,   237,
      97,    95,     0,    78,    72,     0,    71,    69,     0,    59,
       0,    84,    85,    90,     0,     0,    49,    33,     0,     0,
      58,     0,   112,     0,   140,   229,   246,   245,   244,   241,
     263,     0,     0,     0,   258,   257,   259,     0,   252,   253,
     260,    98,    67,     0,    81,    77,     0,     0,     0,    93,
      83,    86,    87,     0,   106,    50,     0,     0,     0,   114,
       0,   108,     0,     0,   264,   256,   255,   250,     0,     0,
      79,    73,    59,    59,    91,     0,    51,    34,    36,    40,
     115,     0,     0,   262,   254,    99,   100,   102,     0,    92,
       0,     0,     0,     0,   269,   268,   270,   271,   265,     0,
      88,    43,    37,   267,   266,   101,     0,    89
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,     3,     5,    58,    59,    60,   132,    61,
     145,   406,    62,   146,   441,    63,   239,   369,    64,   246,
      65,   234,   235,   365,   425,    66,    67,   149,   150,   241,
      68,   227,    69,   135,   229,   314,   315,   316,   317,    70,
     319,   360,   361,   456,   362,   403,   363,    71,   225,   309,
     419,   435,   436,    72,    73,   364,    74,   373,    75,    76,
     153,   335,    77,   221,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,   294,   129,    96,   103,   107,    97,   292,   293,
      98,   130,   223,   224,    99,   213,   299,   300,   346,   100,
     348,   387,   388,   389,   101,   380,   413,   414
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -340
static const yytype_int16 yypact[] =
{
    -340,    22,  -340,  -340,  -340,   307,    -5,  -340,  1257,   -23,
    -340,  1257,  1257,  1257,  1257,  1257,    40,  1257,  1257,  1257,
    1257,  1257,  1257,  1257,  1257,   595,  -340,  -340,  -340,  -340,
    -340,   -56,  -340,  -340,    66,     2,    85,    97,  1257,    32,
     691,    99,   117,   118,  -340,  -340,   119,    52,    52,  1257,
    -340,   132,  1257,  1257,  1257,  -340,  -340,  -340,  -340,  -340,
    -340,  -340,  -340,    92,  -340,  -340,  -340,   127,  -340,  -340,
    -340,  -340,  -340,  -340,  -340,  -340,  -340,  -340,   131,     1,
    -340,   226,    94,   163,   181,   186,   189,   129,    -1,   105,
      68,   168,  1257,  -340,   -27,    67,  -340,  -340,  -340,  -340,
    -340,  -340,  -340,    78,  -340,  -340,  -340,    93,  -340,  -340,
    -340,  -340,  -340,   734,   138,   145,  -340,  -340,  -340,  -340,
    -340,  -340,  -340,  -340,   159,   830,   926,  1022,   161,    95,
    1257,  -340,  -340,  -340,  -340,   172,  -340,  -340,   165,   179,
    -340,   182,  -340,  1257,  1065,   191,   499,  -340,   187,   249,
    -340,   249,   192,   499,  1257,  -340,  -340,  -340,  -340,  -340,
    -340,  1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,
    1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,
    1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,
    1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,  1257,
    1257,  -340,  -340,  -340,  1257,  -340,  -340,  -340,  1161,  -340,
    1257,    40,  -340,  1300,  -340,   -31,  1257,  1257,  1257,  -340,
    -340,  -340,  -340,    15,  -340,   183,   403,   172,   -24,   187,
    -340,  -340,  -340,   195,   194,    53,  -340,  1257,   174,  1257,
      21,   256,    52,  -340,   164,   206,   499,  -340,  -340,  -340,
    -340,  -340,  -340,  -340,  -340,  -340,  -340,  -340,  -340,  -340,
    -340,  -340,  -340,  -340,  -340,   207,   163,   181,   186,   189,
     129,    -1,    -1,    -1,    -1,   105,   105,   105,   105,    68,
      68,    68,   168,   168,  -340,  -340,  -340,  -340,  1257,  -340,
    -340,  1257,    18,  -340,   139,   211,  -340,   209,   271,   272,
    -340,   219,  -340,  -340,  -340,  -340,  1257,  -340,  1257,   212,
    -340,   187,  -340,   187,   214,   276,  -340,   217,   212,    23,
    -340,  1257,  -340,   218,   227,  -340,  -340,  -340,  -340,  -340,
    -340,  -340,  1257,  -340,   228,   499,  -340,  -340,  1257,  1396,
    -340,  -340,  -340,  1257,  1257,  1300,   223,  -340,     4,  -340,
     282,  -340,   212,    90,  -340,   -24,  -340,  -340,   231,   -45,
     220,   190,   193,  -340,   212,   224,  -340,  -340,  1257,   229,
    -340,   -49,  -340,   499,  -340,  -340,  -340,  -340,  -340,  -340,
     167,   169,   170,   201,  -340,  -340,  -340,   233,   295,  -340,
    -340,  -340,  -340,  1257,  -340,  -340,   238,   171,   240,  -340,
    -340,  -340,  -340,   212,  -340,  1257,   499,   241,   499,  -340,
     242,  -340,   303,    25,  -340,  -340,  -340,  -340,     4,  1257,
    -340,  -340,   187,   187,  -340,   246,  -340,  -340,  -340,  -340,
    -340,    35,   167,  -340,  -340,   308,  -340,  -340,   247,  -340,
     499,   248,   184,   185,  -340,  -340,  -340,  -340,  -340,  1257,
    -340,  -340,  -340,  -340,  -340,  -340,   212,  -340
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -340,  -340,  -340,  -340,   180,  -145,  -340,  -298,  -340,  -340,
    -340,  -340,  -340,  -340,  -340,  -340,  -340,  -340,  -340,  -340,
    -340,  -340,  -340,  -340,  -340,  -340,    81,   269,    77,  -224,
    -340,  -340,  -340,  -340,    96,  -340,  -340,   -35,   -33,  -340,
    -340,  -340,   -38,  -340,   -36,  -340,  -340,  -340,  -340,  -340,
    -340,  -340,  -123,  -340,  -340,  -340,  -340,  -340,  -340,  -340,
    -340,  -340,  -340,  -196,   -25,  -340,  -128,  -176,  -340,   147,
     148,   146,   149,   150,   -19,    29,  -138,   -37,  -201,    50,
     122,   314,  -340,  -340,  -340,  -340,  -340,   319,  -340,    -3,
    -340,  -340,  -340,    31,  -340,  -340,  -340,    -6,  -340,  -339,
    -340,  -340,  -340,   -78,  -325,  -340,  -340,   -90
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -204
static const yytype_int16 yytable[] =
{
     128,   238,   222,   265,   161,   318,   301,   291,   244,   386,
     398,   351,   290,   138,   409,   141,   312,   298,   306,   240,
     357,   339,     4,   390,   152,   203,   302,   204,   432,   189,
     190,   191,   192,   247,   106,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,     7,   381,   382,   392,   279,   280,   281,   105,   383,
     102,   108,   109,   110,   111,   112,   404,   116,   117,   118,
     119,   120,   121,   122,   123,   131,   307,     9,    10,   386,
     222,   340,   410,   442,   443,   222,   433,   352,   128,   353,
     383,   162,   446,   390,   393,   113,    26,    27,    28,    29,
      30,   337,   155,   156,   157,   424,   447,   313,   205,   104,
     206,   207,   350,   179,   180,   384,   196,   197,   233,   236,
     358,   359,   208,   209,   210,   326,   327,   328,   329,   245,
     394,   133,   330,   134,   385,   399,   370,   248,   291,   193,
     194,   195,   202,   290,    47,    48,   444,   376,   377,   298,
     136,    55,   331,   211,   185,   186,   187,   188,   457,   282,
     283,   137,   374,   139,   142,   445,   271,   272,   273,   274,
      56,    57,   143,   144,   147,   155,   156,   157,   222,   205,
     222,   206,   207,   148,   181,   295,  -203,   154,  -203,  -203,
     372,   158,   159,   208,   209,   210,   160,   420,   438,   439,
    -203,  -203,  -203,   182,   222,   198,   199,   200,   201,   183,
     212,   222,   323,   184,   325,   222,   222,   222,   275,   276,
     277,   278,   215,   437,   219,   214,   220,   228,   411,   230,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   231,   237,   232,   284,   285,
     286,   240,   242,   437,   287,   178,   308,   243,   320,   321,
     332,   427,   324,   429,   334,   222,   303,   304,   305,   336,
     341,   338,   342,   343,   344,   345,   347,   354,    33,   355,
     356,   367,   368,   371,   379,   391,   397,   400,   359,   405,
     358,   222,   408,   124,   417,   451,   366,   412,   418,   415,
     416,   421,   422,   423,   428,   430,   431,    -4,     6,   440,
     450,   449,   226,   452,   453,   454,   322,   151,     7,   333,
     395,   222,   396,   311,   402,   401,   455,   266,   268,   267,
     114,     8,   269,   296,   270,   115,   375,   349,   108,   378,
     434,   202,   448,   407,     9,    10,     0,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,     0,    25,    26,    27,    28,    29,    30,     0,    31,
       0,     0,    32,    33,     0,    34,    35,    36,    37,    38,
     426,     0,    39,     0,     0,     0,     0,    40,    41,     0,
       0,    42,     0,     0,    43,    44,    45,    46,    47,    48,
       0,     0,    49,    50,     6,     0,     0,     0,     0,     0,
      51,    52,    53,    54,     7,     0,     0,     0,    55,     0,
       0,     0,     0,     0,     0,     0,     0,     8,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    56,    57,     0,
       9,    10,     0,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,     0,    25,    26,
      27,    28,    29,    30,     0,    31,     0,     0,    32,    33,
     310,    34,    35,    36,    37,    38,     0,     0,    39,     0,
       0,     0,     0,    40,    41,     0,     0,    42,     0,     0,
      43,    44,    45,    46,    47,    48,     0,     0,    49,    50,
       0,     0,     0,     0,     0,     0,    51,    52,    53,    54,
       7,     0,     0,     0,    55,     0,     0,     0,     0,     0,
       0,     0,     0,     8,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    56,    57,     0,     9,    10,     0,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,     0,    25,    26,    27,    28,    29,    30,
       0,    31,     0,     0,    32,    33,     0,    34,    35,    36,
      37,    38,     0,     0,    39,     0,     0,     0,     0,    40,
      41,     0,     0,    42,     0,     0,    43,    44,    45,    46,
      47,    48,     0,     0,    49,    50,     0,     0,     0,     0,
       0,     0,    51,    52,    53,    54,     7,     0,     0,     0,
      55,     0,     0,     0,     0,     0,     0,     0,     0,     8,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    56,
      57,     0,     9,    10,     0,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,     0,
      25,    26,    27,    28,    29,    30,     0,     0,     0,     0,
       0,     0,     0,     0,   104,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   124,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   125,
     126,   127,     7,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    56,    57,     0,     9,    10,
       0,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,     7,    25,    26,    27,    28,
      29,    30,     0,     0,     0,     0,   140,     0,     8,     0,
     104,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     9,    10,     0,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,     0,    25,
      26,    27,    28,    29,    30,    52,    53,    54,     0,     0,
       0,     0,    55,   104,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    56,    57,     0,     0,     0,   124,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    52,    53,
      54,     7,     0,     0,     0,    55,     0,     0,     0,     0,
       0,     0,     0,     0,     8,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    56,    57,     0,     9,    10,     0,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,     0,    25,    26,    27,    28,    29,
      30,     0,     0,   216,     0,     0,     0,     0,     0,   104,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    52,    53,    54,     7,     0,     0,
       0,    55,     0,     0,     0,     0,     0,     0,     0,     0,
       8,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      56,    57,     0,     9,    10,     0,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
       0,    25,    26,    27,    28,    29,    30,     0,     0,   217,
       0,     0,     0,     0,     0,   104,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      52,    53,    54,     7,     0,     0,     0,    55,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    56,    57,     0,     9,
      10,     0,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,     7,    25,    26,    27,
      28,    29,    30,     0,     0,   218,     0,     0,     0,     8,
       0,   104,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     9,    10,     0,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,     0,
      25,    26,    27,    28,    29,    30,    52,    53,    54,     0,
     -44,     0,     0,    55,   104,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    56,    57,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    52,
      53,    54,     7,     0,     0,     0,    55,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    56,    57,     0,     9,    10,
       0,   288,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,     0,    25,    26,    27,    28,
      29,    30,     0,     0,     0,     0,     0,     0,     0,     0,
     104,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   289,     0,     0,    52,    53,    54,     7,     0,
       0,     0,    55,     0,     0,     0,     0,     0,     0,     0,
       0,     8,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    56,    57,     0,     9,    10,     0,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,     7,    25,    26,    27,    28,    29,    30,     0,     0,
       0,     0,     0,     0,     8,     0,   104,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     9,    10,     0,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,     0,    25,    26,    27,    28,    29,
      30,    52,    53,    54,     0,     0,     0,     0,    55,   104,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    56,    57,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    52,    53,    54,     7,     0,     0,
       0,    55,     0,     0,     0,     0,     0,     0,     0,     0,
       8,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      56,   297,     0,     9,    10,     0,   288,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
       0,    25,    26,    27,    28,    29,    30,     0,     0,     0,
       0,     0,     0,     0,     0,   104,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      52,    53,    54,     0,     0,     0,     0,    55,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    56,    57
};

static const yytype_int16 yycheck[] =
{
      25,   146,   130,   179,     3,   229,    37,   208,   153,   348,
      55,   309,   208,    38,    63,    40,    40,   213,     3,    64,
     318,     3,     0,   348,    49,    52,    57,    54,     3,    30,
      31,    32,    33,   161,    57,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,    11,    48,    49,   352,   193,   194,   195,     8,    55,
      65,    11,    12,    13,    14,    15,   364,    17,    18,    19,
      20,    21,    22,    23,    24,   131,    61,    37,    38,   418,
     208,    63,   131,    48,    49,   213,    61,   311,   113,   313,
      55,    90,   431,   418,     4,    55,    56,    57,    58,    59,
      60,   246,    52,    53,    54,   403,   431,   131,    41,    69,
      43,    44,   308,    19,    20,   111,    48,    49,   143,   144,
      97,    98,    55,    56,    57,   104,   105,   106,   107,   154,
      40,    65,   111,   131,   130,   359,   332,   162,   339,    34,
      35,    36,    92,   339,    91,    92,   111,   343,   344,   345,
      65,   111,   131,    86,    25,    26,    27,    28,   456,   196,
     197,    64,   338,   131,    65,   130,   185,   186,   187,   188,
     130,   131,    55,    55,    55,   125,   126,   127,   306,    41,
     308,    43,    44,   131,    21,   210,    41,    55,    43,    44,
     335,    99,    65,    55,    56,    57,    65,   393,   422,   423,
      55,    56,    57,    22,   332,    37,    38,    39,    40,    23,
     132,   339,   237,    24,   239,   343,   344,   345,   189,   190,
     191,   192,    63,   419,    63,   132,   131,    55,   373,    64,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    66,    55,    65,   198,   199,
     200,    64,     3,   449,   204,    29,    73,    65,    63,    65,
       4,   406,    88,   408,   100,   393,   216,   217,   218,    63,
     131,    64,    61,    64,     3,     3,    57,    63,    66,     3,
      63,    63,    55,    55,    61,     3,    55,    67,    98,    65,
      97,   419,    63,    92,    61,   440,   321,   130,     3,   130,
     130,    63,   131,    63,    63,    63,     3,     0,     1,    63,
      63,     3,   132,    65,   130,   130,   235,    48,    11,   242,
     355,   449,   355,   227,   362,   361,   449,   180,   182,   181,
      16,    24,   183,   211,   184,    16,   339,   306,   288,   345,
     418,   291,   432,   368,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    -1,    55,    56,    57,    58,    59,    60,    -1,    62,
      -1,    -1,    65,    66,    -1,    68,    69,    70,    71,    72,
     405,    -1,    75,    -1,    -1,    -1,    -1,    80,    81,    -1,
      -1,    84,    -1,    -1,    87,    88,    89,    90,    91,    92,
      -1,    -1,    95,    96,     1,    -1,    -1,    -1,    -1,    -1,
     103,   104,   105,   106,    11,    -1,    -1,    -1,   111,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   130,   131,    -1,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    -1,    55,    56,
      57,    58,    59,    60,    -1,    62,    -1,    -1,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    75,    -1,
      -1,    -1,    -1,    80,    81,    -1,    -1,    84,    -1,    -1,
      87,    88,    89,    90,    91,    92,    -1,    -1,    95,    96,
      -1,    -1,    -1,    -1,    -1,    -1,   103,   104,   105,   106,
      11,    -1,    -1,    -1,   111,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    24,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   130,   131,    -1,    37,    38,    -1,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    -1,    55,    56,    57,    58,    59,    60,
      -1,    62,    -1,    -1,    65,    66,    -1,    68,    69,    70,
      71,    72,    -1,    -1,    75,    -1,    -1,    -1,    -1,    80,
      81,    -1,    -1,    84,    -1,    -1,    87,    88,    89,    90,
      91,    92,    -1,    -1,    95,    96,    -1,    -1,    -1,    -1,
      -1,    -1,   103,   104,   105,   106,    11,    -1,    -1,    -1,
     111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   130,
     131,    -1,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    -1,
      55,    56,    57,    58,    59,    60,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    69,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    92,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,
     105,   106,    11,    -1,    -1,    -1,   111,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   130,   131,    -1,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    11,    55,    56,    57,    58,
      59,    60,    -1,    -1,    -1,    -1,    65,    -1,    24,    -1,
      69,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    37,    38,    -1,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    -1,    55,
      56,    57,    58,    59,    60,   104,   105,   106,    -1,    -1,
      -1,    -1,   111,    69,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   130,   131,    -1,    -1,    -1,    92,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,   105,
     106,    11,    -1,    -1,    -1,   111,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    24,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   130,   131,    -1,    37,    38,    -1,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    -1,    55,    56,    57,    58,    59,
      60,    -1,    -1,    63,    -1,    -1,    -1,    -1,    -1,    69,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   104,   105,   106,    11,    -1,    -1,
      -1,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     130,   131,    -1,    37,    38,    -1,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      -1,    55,    56,    57,    58,    59,    60,    -1,    -1,    63,
      -1,    -1,    -1,    -1,    -1,    69,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     104,   105,   106,    11,    -1,    -1,    -1,   111,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    24,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   130,   131,    -1,    37,
      38,    -1,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    11,    55,    56,    57,
      58,    59,    60,    -1,    -1,    63,    -1,    -1,    -1,    24,
      -1,    69,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    37,    38,    -1,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    -1,
      55,    56,    57,    58,    59,    60,   104,   105,   106,    -1,
      65,    -1,    -1,   111,    69,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   130,   131,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,
     105,   106,    11,    -1,    -1,    -1,   111,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   130,   131,    -1,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    -1,    55,    56,    57,    58,
      59,    60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      69,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   101,    -1,    -1,   104,   105,   106,    11,    -1,
      -1,    -1,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   130,   131,    -1,    37,    38,    -1,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    11,    55,    56,    57,    58,    59,    60,    -1,    -1,
      -1,    -1,    -1,    -1,    24,    -1,    69,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    -1,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    -1,    55,    56,    57,    58,    59,
      60,   104,   105,   106,    -1,    -1,    -1,    -1,   111,    69,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   130,   131,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   104,   105,   106,    11,    -1,    -1,
      -1,   111,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      24,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     130,   131,    -1,    37,    38,    -1,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      -1,    55,    56,    57,    58,    59,    60,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    69,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     104,   105,   106,    -1,    -1,    -1,    -1,   111,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   130,   131
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   135,   136,   137,     0,   138,     1,    11,    24,    37,
      38,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    55,    56,    57,    58,    59,
      60,    62,    65,    66,    68,    69,    70,    71,    72,    75,
      80,    81,    84,    87,    88,    89,    90,    91,    92,    95,
      96,   103,   104,   105,   106,   111,   130,   131,   139,   140,
     141,   143,   146,   149,   152,   154,   159,   160,   164,   166,
     173,   181,   187,   188,   190,   192,   193,   196,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   218,   221,   224,   228,
     233,   238,    65,   219,    69,   213,    57,   220,   213,   213,
     213,   213,   213,    55,   215,   221,   213,   213,   213,   213,
     213,   213,   213,   213,    92,   104,   105,   106,   198,   217,
     225,   131,   142,    65,   131,   167,    65,    64,   198,   131,
      65,   198,    65,    55,    55,   144,   147,    55,   131,   161,
     162,   161,   198,   194,    55,   213,   213,   213,    99,    65,
      65,     3,    90,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    29,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    30,
      31,    32,    33,    34,    35,    36,    48,    49,    37,    38,
      39,    40,   213,    52,    54,    41,    43,    44,    55,    56,
      57,    86,   132,   229,   132,    63,    63,    63,    63,    63,
     131,   197,   200,   226,   227,   182,   138,   165,    55,   168,
      64,    66,    65,   198,   155,   156,   198,    55,   139,   150,
      64,   163,     3,    65,   139,   198,   153,   200,   198,   200,
     200,   200,   200,   200,   200,   200,   200,   200,   200,   200,
     200,   200,   200,   200,   200,   201,   203,   204,   205,   206,
     207,   208,   208,   208,   208,   209,   209,   209,   209,   210,
     210,   210,   211,   211,   213,   213,   213,   213,    40,   101,
     197,   212,   222,   223,   216,   198,   214,   131,   197,   230,
     231,    37,    57,   213,   213,   213,     3,    61,    73,   183,
      67,   168,    40,   131,   169,   170,   171,   172,   163,   174,
      63,    65,   160,   198,    88,   198,   104,   105,   106,   107,
     111,   131,     4,   162,   100,   195,    63,   139,    64,     3,
      63,   131,    61,    64,     3,     3,   232,    57,   234,   227,
     197,   141,   163,   163,    63,     3,    63,   141,    97,    98,
     175,   176,   178,   180,   189,   157,   198,    63,    55,   151,
     197,    55,   139,   191,   201,   223,   197,   197,   231,    61,
     239,    48,    49,    55,   111,   130,   233,   235,   236,   237,
     238,     3,   141,     4,    40,   171,   172,    55,    55,   163,
      67,   178,   176,   179,   141,    65,   145,   198,    63,    63,
     131,   139,   130,   240,   241,   130,   130,    61,     3,   184,
     197,    63,   131,    63,   141,   158,   198,   139,    63,   139,
      63,     3,     3,    61,   237,   185,   186,   197,   163,   163,
      63,   148,    48,    49,   111,   130,   233,   238,   241,     3,
      63,   139,    65,   130,   130,   186,   177,   141
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  /*switch (yytype)
    {

      default:
	break;
    }*/
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:

/* Line 1455 of yacc.c  */
#line 222 "syntax/tjs.y"
    { sb->PushContextStack(TJS_W("global"),
												ctTopLevel); ;}
    break;

  case 4:

/* Line 1455 of yacc.c  */
#line 224 "syntax/tjs.y"
    { sb->PopContextStack(); ;}
    break;

  case 7:

/* Line 1455 of yacc.c  */
#line 231 "syntax/tjs.y"
    { if(sb->CompileErrorCount>20)
												YYABORT;
											  else yyerrok; ;}
    break;

  case 11:

/* Line 1455 of yacc.c  */
#line 245 "syntax/tjs.y"
    { cc->CreateExprCode((yyvsp[(1) - (2)].np)); ;}
    break;

  case 17:

/* Line 1455 of yacc.c  */
#line 251 "syntax/tjs.y"
    { cc->DoBreak(); ;}
    break;

  case 18:

/* Line 1455 of yacc.c  */
#line 252 "syntax/tjs.y"
    { cc->DoContinue(); ;}
    break;

  case 19:

/* Line 1455 of yacc.c  */
#line 253 "syntax/tjs.y"
    { cc->DoDebugger(); ;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 268 "syntax/tjs.y"
    { cc->EnterBlock(); ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 270 "syntax/tjs.y"
    { cc->ExitBlock(); ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 275 "syntax/tjs.y"
    { cc->EnterWhileCode(false); ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 276 "syntax/tjs.y"
    { cc->CreateWhileExprCode((yyvsp[(4) - (5)].np), false); ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 277 "syntax/tjs.y"
    { cc->ExitWhileCode(false); ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 282 "syntax/tjs.y"
    { cc->EnterWhileCode(true); ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 285 "syntax/tjs.y"
    { cc->CreateWhileExprCode((yyvsp[(6) - (7)].np), true); ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 286 "syntax/tjs.y"
    { cc->ExitWhileCode(true); ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 291 "syntax/tjs.y"
    { cc->EnterIfCode(); ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 292 "syntax/tjs.y"
    { cc->CreateIfExprCode((yyvsp[(4) - (4)].np)); ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 293 "syntax/tjs.y"
    { cc->ExitIfCode(); ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 298 "syntax/tjs.y"
    { cc->EnterElseCode(); ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 299 "syntax/tjs.y"
    { cc->ExitElseCode(); ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 308 "syntax/tjs.y"
    { cc->ExitForCode(); ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 314 "syntax/tjs.y"
    { cc->EnterForCode(); ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 315 "syntax/tjs.y"
    { cc->EnterForCode(); ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 317 "syntax/tjs.y"
    { cc->EnterForCode();
											  cc->CreateExprCode((yyvsp[(1) - (1)].np)); ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 323 "syntax/tjs.y"
    { cc->CreateForExprCode(NULL); ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 324 "syntax/tjs.y"
    { cc->CreateForExprCode((yyvsp[(1) - (1)].np)); ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 329 "syntax/tjs.y"
    { cc->SetForThirdExprCode(NULL); ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 330 "syntax/tjs.y"
    { cc->SetForThirdExprCode((yyvsp[(1) - (1)].np)); ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 353 "syntax/tjs.y"
    { cc->AddLocalVariable(
												lx->GetString((yyvsp[(1) - (2)].num))); ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 355 "syntax/tjs.y"
    { cc->InitLocalVariable(
											  lx->GetString((yyvsp[(1) - (4)].num)), (yyvsp[(4) - (4)].np)); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 372 "syntax/tjs.y"
    { sb->PushContextStack(
												lx->GetString((yyvsp[(2) - (2)].num)),
											  ctFunction);
											  cc->EnterBlock();;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 377 "syntax/tjs.y"
    { cc->ExitBlock(); sb->PopContextStack(); ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 382 "syntax/tjs.y"
    { sb->PushContextStack(
												TJS_W("(anonymous)"),
											  ctExprFunction);
											  cc->EnterBlock(); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 387 "syntax/tjs.y"
    { cc->ExitBlock();
											  tTJSVariant v(cc);
											  sb->PopContextStack();
											  (yyval.np) = cc->MakeNP0(T_CONSTVAL);
											  (yyval.np)->SetValue(v); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 414 "syntax/tjs.y"
    { cc->AddFunctionDeclArg(
												lx->GetString((yyvsp[(1) - (2)].num)), NULL); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 416 "syntax/tjs.y"
    { cc->AddFunctionDeclArg(
												lx->GetString((yyvsp[(1) - (4)].num)), (yyvsp[(4) - (4)].np)); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 421 "syntax/tjs.y"
    { cc->AddFunctionDeclArgCollapse(
												NULL); ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 423 "syntax/tjs.y"
    { cc->AddFunctionDeclArgCollapse(
												lx->GetString((yyvsp[(1) - (3)].num))); ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 435 "syntax/tjs.y"
    { sb->PushContextStack(
												lx->GetString((yyvsp[(2) - (3)].num)),
												ctProperty); ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 439 "syntax/tjs.y"
    { sb->PopContextStack(); ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 450 "syntax/tjs.y"
    { sb->PushContextStack(
												TJS_W("(setter)"),
												ctPropertySetter);
											  cc->EnterBlock();
											  cc->SetPropertyDeclArg(
												lx->GetString((yyvsp[(3) - (5)].num))); ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 456 "syntax/tjs.y"
    { cc->ExitBlock();
											  sb->PopContextStack(); ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 461 "syntax/tjs.y"
    { sb->PushContextStack(
												TJS_W("(getter)"),
												ctPropertyGetter);
											  cc->EnterBlock(); ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 465 "syntax/tjs.y"
    { cc->ExitBlock();
											  sb->PopContextStack(); ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 477 "syntax/tjs.y"
    { sb->PushContextStack(
												lx->GetString((yyvsp[(2) - (2)].num)),
												ctClass); ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 481 "syntax/tjs.y"
    { sb->PopContextStack(); ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 486 "syntax/tjs.y"
    { cc->CreateExtendsExprCode((yyvsp[(2) - (2)].np), true); ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 487 "syntax/tjs.y"
    { cc->CreateExtendsExprCode((yyvsp[(2) - (3)].np), false); ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 497 "syntax/tjs.y"
    { cc->CreateExtendsExprCode((yyvsp[(1) - (1)].np), false); ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 502 "syntax/tjs.y"
    { cc->ReturnFromFunc(NULL); ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 503 "syntax/tjs.y"
    { cc->ReturnFromFunc((yyvsp[(2) - (3)].np)); ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 510 "syntax/tjs.y"
    { cc->EnterSwitchCode((yyvsp[(3) - (4)].np)); ;}
    break;

  case 106:

/* Line 1455 of yacc.c  */
#line 511 "syntax/tjs.y"
    { cc->ExitSwitchCode(); ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 517 "syntax/tjs.y"
    { cc->EnterWithCode((yyvsp[(3) - (4)].np)); ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 518 "syntax/tjs.y"
    { cc->ExitWithCode(); ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 523 "syntax/tjs.y"
    { cc->ProcessCaseCode((yyvsp[(2) - (3)].np)); ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 524 "syntax/tjs.y"
    { cc->ProcessCaseCode(NULL); ;}
    break;

  case 111:

/* Line 1455 of yacc.c  */
#line 529 "syntax/tjs.y"
    { cc->EnterTryCode(); ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 532 "syntax/tjs.y"
    { cc->ExitTryCode(); ;}
    break;

  case 113:

/* Line 1455 of yacc.c  */
#line 536 "syntax/tjs.y"
    { cc->EnterCatchCode(NULL); ;}
    break;

  case 114:

/* Line 1455 of yacc.c  */
#line 537 "syntax/tjs.y"
    { cc->EnterCatchCode(NULL); ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 538 "syntax/tjs.y"
    { cc->EnterCatchCode(
												lx->GetString((yyvsp[(3) - (4)].num))); ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 544 "syntax/tjs.y"
    { cc->ProcessThrowCode((yyvsp[(2) - (3)].np)); ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 549 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 553 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 554 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_IF, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 559 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 560 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_COMMA, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 565 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 566 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_SWAP, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 567 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_EQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 568 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_AMPERSANDEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 569 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_VERTLINEEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 570 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_CHEVRONEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 571 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_MINUSEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 572 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_PLUSEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 573 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_PERCENTEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 574 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_SLASHEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 575 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_BACKSLASHEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 576 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_ASTERISKEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 577 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_LOGICALOREQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 578 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_LOGICALANDEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 579 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_RARITHSHIFTEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 580 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_LARITHSHIFTEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 581 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_RBITSHIFTEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 586 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 589 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP3(T_QUESTION, (yyvsp[(1) - (5)].np), (yyvsp[(3) - (5)].np), (yyvsp[(5) - (5)].np)); ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 595 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 596 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_LOGICALOR, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 600 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 602 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_LOGICALAND, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 606 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 607 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_VERTLINE, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 611 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 612 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_CHEVRON, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 616 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 617 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_AMPERSAND, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 621 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 622 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_NOTEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 623 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_EQUALEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 624 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_DISCNOTEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 625 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_DISCEQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 629 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 630 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_LT, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 631 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_GT, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 632 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_LTOREQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 633 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_GTOREQUAL, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 637 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 638 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_RARITHSHIFT, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 639 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_LARITHSHIFT, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 640 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_RBITSHIFT, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 645 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 646 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_PLUS, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 647 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_MINUS, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 651 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 652 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_PERCENT, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 653 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_SLASH, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 654 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_BACKSLASH, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 655 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_ASTERISK, (yyvsp[(1) - (2)].np), (yyvsp[(2) - (2)].np)); ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 659 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (2)].np); ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 663 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 664 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_EXCRAMATION, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 665 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_TILDE, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 666 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_DECREMENT, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 667 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_INCREMENT, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 668 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(2) - (2)].np); (yyval.np)->SetOpecode(T_NEW); ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 669 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_INVALIDATE, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 670 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_ISVALID, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 671 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_ISVALID, (yyvsp[(1) - (2)].np)); ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 672 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_DELETE, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 673 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_TYPEOF, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 674 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_SHARP, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 675 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_DOLLAR, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 676 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_UPLUS, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 677 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_UMINUS, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 678 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_IGNOREPROP, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 679 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_PROPACCESS, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 680 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_INSTANCEOF, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 681 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_INT, (yyvsp[(4) - (4)].np)); ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 682 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_INT, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 683 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_REAL, (yyvsp[(4) - (4)].np)); ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 684 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_REAL, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 685 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_STRING, (yyvsp[(4) - (4)].np)); ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 686 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_STRING, (yyvsp[(2) - (2)].np)); ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 690 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 692 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_INCONTEXTOF, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 696 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 697 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(2) - (3)].np); ;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 698 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_LBRACKET, (yyvsp[(1) - (4)].np), (yyvsp[(3) - (4)].np)); ;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 699 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 700 "syntax/tjs.y"
    { lx->SetNextIsBareWord(); ;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 701 "syntax/tjs.y"
    { tTJSExprNode * node = cc->MakeNP0(T_CONSTVAL);
												  node->SetValue(lx->GetValue((yyvsp[(4) - (4)].num)));
												  (yyval.np) = cc->MakeNP2(T_DOT, (yyvsp[(1) - (4)].np), node); ;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 704 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_POSTINCREMENT, (yyvsp[(1) - (2)].np)); ;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 705 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_POSTDECREMENT, (yyvsp[(1) - (2)].np)); ;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 706 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_EVAL, (yyvsp[(1) - (2)].np)); ;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 707 "syntax/tjs.y"
    { lx->SetNextIsBareWord(); ;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 708 "syntax/tjs.y"
    { tTJSExprNode * node = cc->MakeNP0(T_CONSTVAL);
												  node->SetValue(lx->GetValue((yyvsp[(3) - (3)].num)));
												  (yyval.np) = cc->MakeNP1(T_WITHDOT, node); ;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 715 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP0(T_CONSTVAL);
												  (yyval.np)->SetValue(lx->GetValue((yyvsp[(1) - (1)].num))); ;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 717 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP0(T_SYMBOL);
												  (yyval.np)->SetValue(tTJSVariant(
													lx->GetString((yyvsp[(1) - (1)].num)))); ;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 720 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP0(T_THIS); ;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 721 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP0(T_SUPER); ;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 722 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 723 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP0(T_GLOBAL); ;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 724 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP0(T_VOID); ;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 725 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 726 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 727 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 728 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 729 "syntax/tjs.y"
    { lx->SetStartOfRegExp(); ;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 730 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP0(T_REGEXP);
												  (yyval.np)->SetValue(lx->GetValue((yyvsp[(3) - (3)].num))); ;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 732 "syntax/tjs.y"
    { lx->SetStartOfRegExp(); ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 733 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP0(T_REGEXP);
												  (yyval.np)->SetValue(lx->GetValue((yyvsp[(3) - (3)].num))); ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 740 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_LPARENTHESIS, (yyvsp[(1) - (4)].np), (yyvsp[(3) - (4)].np)); ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 745 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP0(T_OMIT); ;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 746 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_ARG, (yyvsp[(1) - (1)].np)); ;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 747 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_ARG, (yyvsp[(3) - (3)].np), (yyvsp[(1) - (3)].np)); ;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 751 "syntax/tjs.y"
    { (yyval.np) = NULL; ;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 752 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_EXPANDARG, NULL); ;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 753 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_EXPANDARG, (yyvsp[(1) - (1)].np)); ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 754 "syntax/tjs.y"
    { (yyval.np) = (yyvsp[(1) - (1)].np); ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 760 "syntax/tjs.y"
    { tTJSExprNode *node =
										  cc->MakeNP0(T_INLINEARRAY);
										  cc->PushCurrentNode(node); ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 764 "syntax/tjs.y"
    { (yyval.np) = cn; cc->PopCurrentNode(); ;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 769 "syntax/tjs.y"
    { cn->Add((yyvsp[(1) - (1)].np)); ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 770 "syntax/tjs.y"
    { cn->Add((yyvsp[(3) - (3)].np)); ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 775 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_ARRAYARG, NULL); ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 776 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP1(T_ARRAYARG, (yyvsp[(1) - (1)].np)); ;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 781 "syntax/tjs.y"
    { tTJSExprNode *node =
										  cc->MakeNP0(T_INLINEDIC);
										  cc->PushCurrentNode(node); ;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 786 "syntax/tjs.y"
    { (yyval.np) = cn; cc->PopCurrentNode(); ;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 793 "syntax/tjs.y"
    { cn->Add((yyvsp[(1) - (1)].np)); ;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 794 "syntax/tjs.y"
    { cn->Add((yyvsp[(3) - (3)].np)); ;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 799 "syntax/tjs.y"
    { (yyval.np) = cc->MakeNP2(T_DICELM, (yyvsp[(1) - (3)].np), (yyvsp[(3) - (3)].np)); ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 800 "syntax/tjs.y"
    { tTJSVariant val(lx->GetString((yyvsp[(1) - (3)].num)));
										  tTJSExprNode *node0 = cc->MakeNP0(T_CONSTVAL);
										  node0->SetValue(val);
										  (yyval.np) = cc->MakeNP2(T_DICELM, node0, (yyvsp[(3) - (3)].np)); ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 816 "syntax/tjs.y"
    { tTJSExprNode *node =
										  cc->MakeNP0(T_CONSTVAL);
										  iTJSDispatch2 * dsp = TJSCreateArrayObject();
										  node->SetValue(tTJSVariant(dsp, dsp));
										  dsp->Release();
										  cc->PushCurrentNode(node); ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 823 "syntax/tjs.y"
    { (yyval.np) = cn; cc->PopCurrentNode(); ;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 841 "syntax/tjs.y"
    { cn->AddArrayElement(- lx->GetValue((yyvsp[(2) - (2)].num))); ;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 842 "syntax/tjs.y"
    { cn->AddArrayElement(+ lx->GetValue((yyvsp[(2) - (2)].num))); ;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 843 "syntax/tjs.y"
    { cn->AddArrayElement(lx->GetValue((yyvsp[(1) - (1)].num))); ;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 844 "syntax/tjs.y"
    { cn->AddArrayElement(tTJSVariant());  ;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 845 "syntax/tjs.y"
    { cn->AddArrayElement((yyvsp[(1) - (1)].np)->GetValue()); ;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 846 "syntax/tjs.y"
    { cn->AddArrayElement((yyvsp[(1) - (1)].np)->GetValue()); ;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 851 "syntax/tjs.y"
    { tTJSExprNode *node =
										  cc->MakeNP0(T_CONSTVAL);
										  iTJSDispatch2 * dsp = TJSCreateDictionaryObject();
										  node->SetValue(tTJSVariant(dsp, dsp));
										  dsp->Release();
										  cc->PushCurrentNode(node); ;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 858 "syntax/tjs.y"
    { (yyval.np) = cn; cc->PopCurrentNode(); ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 871 "syntax/tjs.y"
    { cn->AddDictionaryElement(lx->GetValue((yyvsp[(1) - (4)].num)), - lx->GetValue((yyvsp[(4) - (4)].num))); ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 872 "syntax/tjs.y"
    { cn->AddDictionaryElement(lx->GetValue((yyvsp[(1) - (4)].num)), + lx->GetValue((yyvsp[(4) - (4)].num))); ;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 873 "syntax/tjs.y"
    { cn->AddDictionaryElement(lx->GetValue((yyvsp[(1) - (3)].num)), lx->GetValue((yyvsp[(3) - (3)].num))); ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 874 "syntax/tjs.y"
    { cn->AddDictionaryElement(lx->GetValue((yyvsp[(1) - (3)].num)), tTJSVariant()); ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 875 "syntax/tjs.y"
    { cn->AddDictionaryElement(lx->GetValue((yyvsp[(1) - (3)].num)), (yyvsp[(3) - (3)].np)->GetValue()); ;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 876 "syntax/tjs.y"
    { cn->AddDictionaryElement(lx->GetValue((yyvsp[(1) - (3)].num)), (yyvsp[(3) - (3)].np)->GetValue()); ;}
    break;



/* Line 1455 of yacc.c  */
#line 3730 "tjs.tab.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 881 "syntax/tjs.y"



}
