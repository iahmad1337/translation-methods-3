#include "driver.hh"

int main(int argc, const char *argv[]) {
  auto lex = std::make_shared<TMyLexer>(&std::cin);
  auto p = yy::parser{lex.get()};
  p.set_debug_level(true);
  return p.parse();
}

