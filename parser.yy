%require "3.8"
%debug
%language "c++"
/* This option only works when we use variant/union */
%define api.token.constructor
%define api.value.type variant
%define api.value.automove
%define parse.assert
%define parse.error detailed
%header
%locations

/* See 3.7.15 for %code directive details */
%code requires // *.hh
{
#include <memory> // std::unique_ptr
#include <string>
#include <vector>

#include "ast.hh"

namespace yy {}

struct TMyLexer;

}

/* 10.1.7.2: add this argument to yylex function */
/* %lex-param { TYYLexCtx* ctx } */
/* Make a field of such type in parser */
%param { TMyLexer* lex }

%code // *.cc
{
#include <climits>  // INT_MIN, INT_MAX
#include <iostream>
#include <sstream>

#include "driver.hh"

}


/* https://www.gnu.org/software/bison/manual/html_node/Token-Decl.html */
%token <std::string> ID;
%token <std::string> STRING;
%token <std::string> NUMBER;
%token INDENT;
%token DEDENT;
%token LF;
%token EQ "==";
%token NEQ "!=";
%token LESS "<";
%token GREATER ">";
%token PLUS "+";
%token MINUS "-";
%token ASTERISK "*";
%token ASS "=";
%token LPAREN "(";
%token RPAREN ")";
%token COLON ":";
%token IF "if";
%token FOR "for";
%token IN "in";
%token AND "and";
%token OR "or";
%token NOT "not";
%token COMMA ",";
%token ELIF "elif";
%token ELSE "else";
%token WHILE "while";

%printer { yyo << $$; } <std::string>;

%%

%start file;

%nterm <std::shared_ptr<TTree>> file;
file:
    statements {
        $$ = std::make_shared<TTree>(
            "file",
            $1
        );
        lex->ctx.result = $$;
    }
;

%nterm <std::vector<TPtr>> statements;
statements:
    statement statements {
        $$ = std::vector<TPtr>{};
        $$.push_back($1);
        $$.insert($$.end(), $2.begin(), $2.end());
    }
    | %empty {
        $$ = std::vector<TPtr>{};
    }
;

%nterm <std::shared_ptr<TTree>> statement;
statement:
    compound_stmt {
        $$ = $1;
    }
    | simple_stmt {
        $$ = $1;
    }
;

%nterm <std::shared_ptr<TTree>> simple_stmt;
simple_stmt:
    expr LF {
        $$ = std::make_shared<TTree>("simple_stmt", $1);
    }
;

// expr in `if` must be of type int
// expr in `for` must be of special `range` type
%nterm <std::shared_ptr<TTree>> compound_stmt;
compound_stmt:
    "if" expr ":" LF INDENT statements DEDENT if_cont {
        $$ = std::make_shared<TTree>(
             "if_stmt",
             std::make_shared<TTree>("condition", $2),
             std::make_shared<TTree>("statements", $6),
             $8
        );
    }
    | "for" ID "in" expr ":" LF INDENT statements DEDENT {

        $$ = std::make_shared<TTree>(
            "for_loop",
            std::make_shared<TTree>("iterator", std::make_shared<TId>($2)),
            std::make_shared<TTree>("range_expr", $4),
            std::make_shared<TTree>("statements", $8)
        );
    }
    | "while" expr ":" LF INDENT statements DEDENT {
         $$ = std::make_shared<TTree>(
            "while_loop",
             std::make_shared<TTree>("condition", $2),
            std::make_shared<TTree>("statements", $6)
         );
    }
;

%nterm <std::shared_ptr<TTree>> if_cont;
if_cont:
    "elif" expr ":" LF INDENT statements DEDENT if_cont {
        $$ = std::make_shared<TTree>(
            "if_stmt",
            std::make_shared<TTree>("condition", $2),
            std::make_shared<TTree>("statements", $6),
            $8
        );
    }
    | "else" ":" LF INDENT statements DEDENT {
        $$ = std::make_shared<TTree>(
            "else_stmt",
            std::make_shared<TTree>("statements", $5)
        );
    }
    | %empty {
        $$ = std::make_shared<TTree>(
            "else_stmt",
            std::make_shared<TTree>("statements")
        );
    }
;

/* all expressions will be typed and the types will be inferred during semantic */
/* analysis */

/* Precedence: https://www.gnu.org/software/bison/manual/html_node/Precedence.html */
/* The order of lines determines the precedence. */
/* There is a caveat for unary operators: */
/* https://www.gnu.org/software/bison/manual/html_node/Contextual-Precedence.html */
%right "=";
%left "or";
%left "and";
%precedence "not";
%nonassoc "<" ">" "==" "!=";
%left "-" "+";
%left "*";

/* TODO: use union in generated C code for values (it's safe in C) */
%nterm <TPtr> expr;
expr:
    NUMBER {
        $$ = std::make_shared<TNumber>(std::stoi($1));
    }
    | STRING {
        $$ = std::make_shared<TString>($1);
    }
    | ID {
        $$ = std::make_shared<TId>($1);
    }
    | ID "=" expr {
        $$ = std::make_shared<TTree>("assign", std::make_shared<TId>($1), $3);
    }
    | ID "(" arglist ")" {
        $$ = std::make_shared<TTree>("invoke", std::make_shared<TId>($1), std::make_shared<TTree>("arglist", $3));
    }
    | "(" expr ")" { $$ = $2; }
    | expr "or" expr { $$ = std::make_shared<TTree>("||", $1, $3); }
    | expr "and" expr { $$ = std::make_shared<TTree>("&&", $1, $3); }
    | "not" expr { $$ = std::make_shared<TTree>("!", $2); }
    | expr "==" expr { $$ = std::make_shared<TTree>("==", $1, $3); }
    | expr "!=" expr { $$ = std::make_shared<TTree>("!=", $1, $3); }
    | expr "<" expr { $$ = std::make_shared<TTree>("<", $1, $3); }
    | expr ">" expr { $$ = std::make_shared<TTree>(">", $1, $3); }
    | expr "-" expr { $$ = std::make_shared<TTree>("-", $1, $3); }
    | expr "+" expr { $$ = std::make_shared<TTree>("+", $1, $3); }
    | expr "*" expr { $$ = std::make_shared<TTree>("*", $1, $3); }
;

%nterm <std::vector<TPtr>> arglist;
arglist:
    expr arglist-cont {
        $$ = std::move($2);
        $$.insert($$.begin(), $1);
    }
    | %empty {
        $$ = std::vector<TPtr>{};
    }
;

%nterm <std::vector<TPtr>> arglist-cont;
arglist-cont:
    %empty {
        $$ = std::vector<TPtr>{};
    }
    | "," expr arglist-cont {
        $$ = std::move($3);
        $$.insert($$.begin(), $2);
    }
;

%%

// Local Variables:
// mode: C++
// End:
