%{
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string>

#include "driver.hh"
#include "parser.hh"

%}

ID  [A-Za-z_][A-Za-z0-9_]*
NUMBER [0-9]+
STRING \"[^\n"]+\"
LF [^ \n]+\n

%option c++
%option yyclass="TMyScanner"
%option noyywrap nounput noinput debug
%option outfile="scanner.cc"
/* %option header-file="scanner.hh" */

%%


^[ ]*\n       {/* Ignore blank lines. */}
^[ ]*[^ \n]+  {
  int indents = yyleng / 4;
  while (ctx.currentIndentLevel < indents) {
      ctx.currentIndentLevel++;
      ctx.dentsLeft++;
  }
  while (ctx.currentIndentLevel > indents) {
      ctx.currentIndentLevel--;
      ctx.dentsLeft--;
  }
  if (ctx.dentsLeft > 0) {
      ctx.dentsLeft--;
      return yy::parser::token_kind_type::INDENT;
  }
  if (ctx.dentsLeft < 0) {
      ctx.dentsLeft++;
      return yy::parser::token_kind_type::DEDENT;
  }
  /* otherwise just ignore it */
}


"("  { return yy::parser::token_kind_type::LPAREN; }
")"  { return yy::parser::token_kind_type::RPAREN; }
"+"  { return yy::parser::token_kind_type::PLUS; }
"*"  { return yy::parser::token_kind_type::ASTERISK; }
"=="  { return yy::parser::token_kind_type::EQ; }
"!="  { return yy::parser::token_kind_type::NEQ; }
"not"  { return yy::parser::token_kind_type::NOT; }
"and"  { return yy::parser::token_kind_type::AND; }
"or"  { return yy::parser::token_kind_type::OR; }
"if"  { return yy::parser::token_kind_type::IF; }
"for"  { return yy::parser::token_kind_type::FOR; }
"in"  { return yy::parser::token_kind_type::IN; }
":"  { return yy::parser::token_kind_type::COLON; }
"<"  { return yy::parser::token_kind_type::LESS; }
">"  { return yy::parser::token_kind_type::GREATER; }


{ID} { return yy::parser::token_kind_type::ID; }
{STRING} {
            std::string result{YYText()};
            // remove the quotes
            result.erase(result.begin());
            result.pop_back();

            ctx.curToken = std::move(result);
            ctx.curTokenKind = yy::parser::token_kind_type::STRING;
            return ctx.curTokenKind;
         }
{NUMBER} {
            /* (std::stoi(std::string{yytext}), ctx.loc) */
            return yy::parser::token_kind_type::NUMBER;
         }

{LF}    { return yy::parser::token_kind_type::LF; }

<<EOF>>  { return yy::parser::token_kind_type::END_OF_FILE; }

. { return yy::parser::token_kind_type::YYerror; }

%%

