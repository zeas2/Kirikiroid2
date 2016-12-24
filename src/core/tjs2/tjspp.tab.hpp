namespace TJS {

/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     PT_LPARENTHESIS = 258,
     PT_RPARENTHESIS = 259,
     PT_ERROR = 260,
     PT_COMMA = 261,
     PT_EQUAL = 262,
     PT_NOTEQUAL = 263,
     PT_EQUALEQUAL = 264,
     PT_LOGICALOR = 265,
     PT_LOGICALAND = 266,
     PT_VERTLINE = 267,
     PT_CHEVRON = 268,
     PT_AMPERSAND = 269,
     PT_LT = 270,
     PT_GT = 271,
     PT_LTOREQUAL = 272,
     PT_GTOREQUAL = 273,
     PT_PLUS = 274,
     PT_MINUS = 275,
     PT_ASTERISK = 276,
     PT_SLASH = 277,
     PT_PERCENT = 278,
     PT_EXCLAMATION = 279,
     PT_UN = 280,
     PT_SYMBOL = 281,
     PT_NUM = 282
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 49 "tjspp.y"

	tjs_int32		val;
	tjs_int			nv;



/* Line 1676 of yacc.c  */
#line 86 "tjspp.tab.hpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif





}
