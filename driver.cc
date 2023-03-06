#include "driver.hh"


namespace yy
{
  // The yylex function providing subsequent tokens:
  // TEXT         "I have three numbers for you."
  // NUMBER       1
  // NUMBER       2
  // NUMBER       ...
  // NUMBER       max - 1
  // TEXT         "And that's all!"
  // END_OF_FILE

  parser::symbol_type yylex(TMyLexer* lex) {
    if (lex->ctx.dentsLeft > 0) {
      lex->ctx.dentsLeft--;
      return parser::make_INDENT(lex->ctx.loc);
    }
    if (lex->ctx.dentsLeft < 0) {
      lex->ctx.dentsLeft++;
      return parser::make_DEDENT(lex->ctx.loc);
    }
    lex->yylex();
    return parser::symbol_type(lex->ctx.curTokenKind, lex->ctx.curToken, lex->ctx.loc);
  }

  // Mandatory error function
  void parser::error (const parser::location_type& loc, const std::string& msg) {
    std::cerr << loc << ": " << msg << '\n';
  }
}
