#include <spdlog/spdlog.h>

#include "driver.hh"


TMyLexer::TMyLexRes TMyLexer::_mylex() {
  if (ctx.indentsLeft < 0) {
    ctx.indentsLeft++;
    return { yy::parser::token_kind_type::DEDENT, "", ctx.loc };
  }
  if (ctx.indentsLeft > 0) {
    ctx.indentsLeft--;
    return { yy::parser::token_kind_type::INDENT, "", ctx.loc };
  }

  if (ctx.pendingToken) {
    ctx.pendingToken = false;
    return { ctx.curTokenKind, ctx.curToken, ctx.loc };
  }

  int token_type = yylex();

  if (ctx.prevTokenKind == yy::parser::token_kind_type::LF) {
    assert(token_type != yy::parser::token_kind_type::LF);
    if (token_type == yy::parser::token_kind_type::INDENT) {
      // this is a special kind of token, we should analyze the number of indents
      int level = ctx.curToken.size() / 4;

      ctx.indentsLeft = level - ctx.currentIndentLevel;
      ctx.currentIndentLevel = level;

      if (ctx.indentsLeft == 0) {
        yylex();
        return { ctx.curTokenKind, ctx.curToken, ctx.loc };
      }

      // TODO: sketchy!
      return _mylex();
    } else {
      // return -level dedents before touching the lexed part and save the lexed
      ctx.indentsLeft = -ctx.currentIndentLevel;
      ctx.currentIndentLevel = 0;
      ctx.pendingToken = true;

      return _mylex();
    }
  }

  return { ctx.curTokenKind, ctx.curToken, ctx.loc };
}

namespace yy
{
  parser::symbol_type yylex(TMyLexer* lex) {
    auto [type, text, loc] = lex->mylex();
#define CASE_T(x) case parser::token_kind_type::x: { return parser::make_##x(text, loc); }
#define CASE(x) case parser::token_kind_type::x: { return parser::make_##x(loc); }

    switch (type) {
      CASE(END_OF_FILE)
      CASE(INDENT)
      CASE(DEDENT)
      CASE(LF)
      CASE(EQ)
      CASE(NEQ)
      CASE(LESS)
      CASE(GREATER)
      CASE(PLUS)
      CASE(ASTERISK)
      CASE(ASS)
      CASE(LPAREN)
      CASE(RPAREN)
      CASE(COLON)
      CASE(IF)
      CASE(FOR)
      CASE(IN)
      CASE(AND)
      CASE(OR)
      CASE(NOT)
      CASE_T(ID)
      CASE_T(STRING)
      case parser::token_kind_type::NUMBER: {
                                              return parser::make_NUMBER(text, loc);
                                            }
      default:
        return parser::make_YYerror(loc);
    }
#undef CASE
#undef CASE_T
  }

  // Mandatory error function
  void parser::error (const parser::location_type& loc, const std::string& msg) {
    std::cerr << loc << ": " << msg << '\n';
  }
}
