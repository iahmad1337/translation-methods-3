#include <type_traits>
#include <iostream>
#include <sstream>
#include <fstream>

#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>

#include "driver.hh"
#include "parser.hh"

TPrintVisitor PV{std::cout, "    "};
TNameVisitor NV;
argparse::ArgumentParser program{"pytoc"};

std::optional<TPtr> DoParse(std::istream& is) {
    auto lex = std::make_shared<TMyLexer>(&is);
    auto p = yy::parser{lex.get()};
    if (program["-v"] == true) {
      p.set_debug_level(true);
    }
    if (auto code = p.parse(); code != 0) {
      spdlog::error("parser failed with code {}", code);
      return std::nullopt;
    }
    return lex->ctx.result;
}

int main(int argc, const char *argv[]) {
  /****************************************************************************
  *                                 Argparse                                 *
  ****************************************************************************/

  argparse::ArgumentParser program("parser");
  program.add_argument("-f", "--file")
    .help("accept input from this file");
  program.add_argument("-o", "--outfile")
    .help("print the converted code to this file (write to stdout if not provided)");
  program.add_argument("-v", "--verbose")
    .default_value(false)
    .implicit_value(true);

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& e) {
    spdlog::error("caught exception while parsing args: {}", e.what());
    return 1;
  }

  if (program["-v"] == true) {  // heh
    spdlog::set_level(spdlog::level::info);
    spdlog::info("enabled verbose logging");
  } else {
    spdlog::set_level(spdlog::level::err);
  }

  /****************************************************************************
  *                                 Parsing                                  *
  ****************************************************************************/

  if (program.present("-f")) {
      std::fstream fs{program.get<std::string>("-f")};
      if (auto res = DoParse(fs)) {
        TPyToCVisitor PTCV;
        auto src = res.value()->accept(&PTCV);
        if (program.present("-o")) {
          std::ofstream outfile{program.get<std::string>("-o")};
          outfile << src;
        } else {
          std::cout << src << std::endl;
        }
      }
  } else {
    // interactive mode
    constexpr auto GREETING = R"(
      You have entered the interactive parsing mode!
      You can print any one-liners in the prompt and I will print the AST.
      If you want to terminate the session, just print ":q"
)";
    auto PROMPT = ">> ";
    std::cout << GREETING << std::endl;
    while (true) {
      std::cout << PROMPT << std::flush;
      std::string line;
      std::getline(std::cin, line);

      if (line == ":q") {
        std::cout << "\nbye!" << std::endl;
        break;
      }

      line.push_back('\n');
      std::stringstream ss{line};
      if (auto res = DoParse(ss)) {
        TPyToCVisitor PTCV;
        std::cout << res.value()->accept(&PTCV) << std::endl;
      }
    }
  }
}

