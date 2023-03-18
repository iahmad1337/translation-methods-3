#pragma once

#include <iostream>

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "parser.hh"
#include "ast.hh"

#undef YY_DECL
#define YY_DECL int TMyLexer::yylex()

struct TMyLexer : public yyFlexLexer {
  using yyFlexLexer::yyFlexLexer;

  struct TMyLexRes {
    yy::parser::token_kind_type type;
    std::string text;
    yy::parser::location_type loc;
  };

  // It is automatically overriden in scanner.cc thanks to YY_DECL
  int yylex() override;

  TMyLexRes mylex() {
    ctx.prevTokenKind = ctx.curTokenKind;
    return _mylex();
  }

  struct {
    /// If positive - denotes the number of INDENTs we have to return before
    /// calling lex.yylex() again
    /// If negative - denotes the number of DEDENTs we have to return before
    /// calling lex.yylex() again
    int indentsLeft{0};
    int prevIndentLevel{0};
    int currentIndentLevel{0};
    bool pendingToken{false};
    TPtr result;
    yy::parser::location_type loc{};
    std::string curToken;
    yy::parser::token_kind_type prevTokenKind{};
    yy::parser::token_kind_type curTokenKind{};
  } ctx;

private:
  TMyLexRes _mylex();
};

namespace yy {
// Forward declare the lexing function
parser::symbol_type yylex(TMyLexer* lex);
}  // namespace yy
