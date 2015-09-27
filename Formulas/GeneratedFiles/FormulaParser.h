
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

/* "%code requires" blocks.  */

/* Line 1676 of yacc.c  */
#line 20 "FormulaParser.y"


#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif




/* Line 1676 of yacc.c  */
#line 51 "GeneratedFiles/FormulaParser.h"

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOKEN_OR = 258,
     TOKEN_AND = 259,
     TOKEN_NEQ = 260,
     TOKEN_EQ = 261,
     TOKEN_GTEQ = 262,
     TOKEN_GT = 263,
     TOKEN_LTEQ = 264,
     TOKEN_LT = 265,
     TOKEN_MINUS = 266,
     TOKEN_PLUS = 267,
     TOKEN_PERCENT = 268,
     TOKEN_DIV = 269,
     TOKEN_MUL = 270,
     TOKEN_UNARY_NEG = 271,
     TOKEN_NOT = 272,
     TOKEN_ERR = 273,
     TOKEN_LPAREN = 274,
     TOKEN_RPAREN = 275,
     TOKEN_TRUE = 276,
     TOKEN_FALSE = 277,
     TOKEN_NAME = 278,
     TOKEN_NUMBER = 279,
     TOKEN_ID = 280
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 37 "FormulaParser.y"

    float f_value;
	const char *n_value;
    ASTNode *expression;



/* Line 1676 of yacc.c  */
#line 101 "GeneratedFiles/FormulaParser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




