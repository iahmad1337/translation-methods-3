#pragma once

#include <iostream>

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "parser.hh"

#undef YY_DECL
#define YY_DECL int TMyLexer::yylex()

struct TYYLexCtx {
    /// If positive - denotes the number of INDENTs we have to return before
    /// calling lex.yylex() again
    /// If negative - denotes the number of DEDENTs we have to return before
    /// calling lex.yylex() again
    int dentsLeft{0};
    int currentIndentLevel{0};
    TPtr result;
    yy::parser::location_type loc{};
    std::string curToken;
    yy::parser::token_kind_type curTokenKind{};
};

struct TMyLexer : public yyFlexLexer {
  using yyFlexLexer::yyFlexLexer;

  // It is automatically overriden in scanner.cc thanks to YY_DECL
  int yylex() override;

  TYYLexCtx ctx;
};

namespace yy {
// Forward declare the lexing function
parser::symbol_type yylex(TMyLexer* lex);
}  // namespace yy
