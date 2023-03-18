
%option c++
%option yyclass="TMyScanner"
%option noyywrap nounput noinput debug
%option outfile="scanner.cc"
/* %option header-file="scanner.hh" */

%{
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string>

#include "driver.hh"
#include "parser.hh"

#define YY_USER_ACTION \
    ctx.loc.step(); \
    ctx.loc.columns(YYLeng()); \

#define DEFAULT_TOKEN(x) \
    { \
        ctx.curToken = YYText(); \
        ctx.curTokenKind = yy::parser::token_kind_type::x; \
        return ctx.curTokenKind; \
    }

%}

ID  [A-Za-z_][A-Za-z0-9_]*
NUMBER [0-9]+
STRING \"[^\n"]+\"
/* LF [^ \n]+\n */
LF [ ]*\n

%%

^[ ]*\n       {/* Ignore blank lines. TODO: move location by one line */}
^[ ]*  {
    // TODO: move location everytime w
    // NOTE: this won't match on empty lines as the empty line rule precedes us
    ctx.curToken = YYText();
    ctx.curTokenKind = yy::parser::token_kind_type::INDENT;
    return ctx.curTokenKind;
}

[ ]+ { /* skip the spaces */ }

"("  DEFAULT_TOKEN(LPAREN)
")"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::RPAREN;
        return ctx.curTokenKind;
     }
"+"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::PLUS;
        return ctx.curTokenKind;
     }
"*"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::ASS;
        return ctx.curTokenKind;
     }
"=="  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::EQ;
        return ctx.curTokenKind;
      }
"!="  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::NEQ;
        return ctx.curTokenKind;
      }
"not"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::NOT;
        return ctx.curTokenKind;
      }
"and"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::AND;
        return ctx.curTokenKind;
      }
"or"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::OR;
        return ctx.curTokenKind;
      }
"if"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::IF;
        return ctx.curTokenKind;
      }
"for"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::FOR;
        return ctx.curTokenKind;
      }
"in"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::IN;
        return ctx.curTokenKind;
      }
":"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::COLON;
        return ctx.curTokenKind;
     }
"<"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::LESS;
        return ctx.curTokenKind;
     }
">"  {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::GREATER;
        return ctx.curTokenKind;
     }


{ID} {
        ctx.curToken = YYText();
        ctx.curTokenKind = yy::parser::token_kind_type::ID;
        return ctx.curTokenKind;
     }
{STRING} {
            std::string result{YYText()};
            // remove the quotes
            result.erase(result.begin());
            result.pop_back();

            // TODO: it's actually more efficient to do all of the operations on
            // curToken without any moves
            ctx.curToken = std::move(result);
            ctx.curTokenKind = yy::parser::token_kind_type::STRING;
            return ctx.curTokenKind;
         }
{NUMBER} {
            /* (std::stoi(std::string{yytext}), ctx.loc) */
            ctx.curToken = YYText();
            ctx.curTokenKind = yy::parser::token_kind_type::NUMBER;
            return ctx.curTokenKind;
         }

{LF}    {
            ctx.curToken = "";
            ctx.curTokenKind = yy::parser::token_kind_type::LF;
            return ctx.curTokenKind;
        }

<<EOF>>  {
            ctx.curToken = "";
            ctx.curTokenKind = yy::parser::token_kind_type::END_OF_FILE;
            return ctx.curTokenKind;
        }

. { return yy::parser::token_kind_type::YYerror; }

%%

