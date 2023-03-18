#include <gtest/gtest.h>

#include <iostream>
#include <vector>

// #include <fmt/core.h>
// #include <fmt/ranges.h>

// #include <re2/re2.h>

// #include <nlohmann/json.hpp>
// #include <range/v3/action/sort.hpp>
// #include <range/v3/action/unique.hpp>
// #include <range/v3/view/all.hpp>
// TODO: there are problems with spdlog and fmt - can't make them work together
// for now, I think I just have to use them exclusively
#include <spdlog/spdlog.h>

#include "ast.hh"
#include "cpputils/common.hh"

#include "driver.hh"
#include "parser.hh"

using TParam = std::pair<std::string, std::vector<std::pair<yy::parser::token_kind_type, std::string>>>;

struct TTokenizerTest : public ::testing::TestWithParam<TParam> {};

using tkt = yy::parser::token_kind_type;
const std::vector<TParam> PARAMS = {
  {
    "nota bene not",
    {
      {tkt::ID, "nota"},
      {tkt::ID, "bene"},
      {tkt::NOT, "not"},
    }
  },
  {
    R"(int print hello 42 "123")",
    {
      {tkt::ID, "int"},
      {tkt::ID, "print"},
      {tkt::ID, "hello"},
      {tkt::NUMBER, "42"},
      {tkt::STRING, "123"},
    }
  },
  {
    "and or not+*:if for== !=())(",
    {
      {tkt::AND, "and"},
      {tkt::OR, "or"},
      {tkt::NOT, "not"},
      {tkt::PLUS, "+"},
      {tkt::ASTERISK, "*"},
      {tkt::COLON, ":"},
      {tkt::IF, "if"},
      {tkt::FOR, "for"},
      {tkt::EQ, "=="},
      {tkt::NEQ, "!="},
      {tkt::LPAREN, "("},
      {tkt::RPAREN, ")"},
      {tkt::RPAREN, ")"},
      {tkt::LPAREN, "("},
    }
  },
  {
    "if 42 :\n    print(\"saw 42\")\n print()",
    {
      {tkt::IF, "if"},
      {tkt::NUMBER, "42"},
      {tkt::COLON, ":"},
      {tkt::LF, ""},
      {tkt::INDENT, ""},
      {tkt::ID, "print"},
      {tkt::LPAREN, "("},
      {tkt::STRING, "saw 42"},
      {tkt::RPAREN, ")"},
      {tkt::LF, ""},
      {tkt::DEDENT, ""},
      {tkt::ID, "print"},
      {tkt::LPAREN, "("},
      {tkt::RPAREN, ")"},
    }
  },
  {
    R"(
1
    2
    3
        4
            5
    6
            7
8)",
    {
      {tkt::NUMBER, "1"},
      {tkt::LF, ""},
      {tkt::INDENT, ""},
      {tkt::NUMBER, "2"},
      {tkt::LF, ""},
      {tkt::NUMBER, "3"},
      {tkt::LF, ""},
      {tkt::INDENT, ""},
      {tkt::NUMBER, "4"},
      {tkt::LF, ""},
      {tkt::INDENT, ""},
      {tkt::NUMBER, "5"},
      {tkt::LF, ""},
      {tkt::DEDENT, ""},
      {tkt::DEDENT, ""},
      {tkt::NUMBER, "6"},
      {tkt::LF, ""},
      {tkt::INDENT, ""},
      {tkt::INDENT, ""},
      {tkt::NUMBER, "7"},
      {tkt::LF, ""},
      {tkt::DEDENT, ""},
      {tkt::DEDENT, ""},
      {tkt::DEDENT, ""},
      {tkt::NUMBER, "8"},
    }
  },
  {
    R"(

1
        2
3
        4
                5


    6
            7
8)",
    {
      {tkt::NUMBER, "1"},
      {tkt::LF, ""},
      {tkt::INDENT, ""},
      {tkt::INDENT, ""},
      {tkt::NUMBER, "2"},
      {tkt::LF, ""},
      {tkt::DEDENT, ""},
      {tkt::DEDENT, ""},
      {tkt::NUMBER, "3"},
      {tkt::LF, ""},
      {tkt::INDENT, ""},
      {tkt::INDENT, ""},
      {tkt::NUMBER, "4"},
      {tkt::LF, ""},
      {tkt::INDENT, ""},
      {tkt::INDENT, ""},
      {tkt::NUMBER, "5"},
      {tkt::LF, ""},
      {tkt::DEDENT, ""},
      {tkt::DEDENT, ""},
      {tkt::DEDENT, ""},
      {tkt::NUMBER, "6"},
      {tkt::LF, ""},
      {tkt::INDENT, ""},
      {tkt::INDENT, ""},
      {tkt::NUMBER, "7"},
      {tkt::LF, ""},
      {tkt::DEDENT, ""},
      {tkt::DEDENT, ""},
      {tkt::DEDENT, ""},
      {tkt::NUMBER, "8"},
    }
  },
  {
    "a = 1\nb = 2",
    {
      {tkt::ID, "a"},
      {tkt::ASS, "="},
      {tkt::NUMBER, "1"},
      {tkt::LF, ""},
      {tkt::ID, "b"},
      {tkt::ASS, "="},
      {tkt::NUMBER, "2"},
    }
  },
  {
    "a = 1\nprint(a)",
    {
      {tkt::ID, "a"},
      {tkt::ASS, "="},
      {tkt::NUMBER, "1"},
      {tkt::LF, ""},
      {tkt::ID, "print"},
      {tkt::LPAREN, "("},
      {tkt::ID, "a"},
      {tkt::RPAREN, ")"},
    }
  },
};

TEST_P(TTokenizerTest, Tokens) {
  auto [contents, expected] = GetParam();
  std::stringstream ss{contents};
  TMyLexer lex{&ss};

  std::vector<std::pair<yy::parser::token_kind_type, std::string>> got;
  for (auto lexRes = lex.mylex(); lexRes.type != yy::parser::token_kind_type::YYEOF; lexRes = lex.mylex()) {
    got.emplace_back(lexRes.type, lexRes.text);
  }

  EXPECT_EQ(expected, got);
}

INSTANTIATE_TEST_SUITE_P(Parametrized, TTokenizerTest, testing::ValuesIn(PARAMS));

TEST(VisitorTest, BasicAssertions) {
  std::shared_ptr<TNode> t = std::make_shared<TTree>("an empty node", std::vector<TPtr>{});

  TNameVisitor nv;
  ASSERT_EQ(t->accept(&nv), "tree");

  t = std::make_shared<TTree>("branch", std::vector<TPtr>{
      std::make_shared<TId>("identifier1"),
      std::make_shared<TString>("string literal 1"),
      std::make_shared<TNumber>(42),
      std::make_shared<TTree>("nested branch", std::vector<TPtr>{
          std::make_shared<TNumber>(228),
          std::make_shared<TId>("identifier2"),
      }),
  });

  spdlog::info("Started printing AST");
  {
    TPrintVisitor pv{std::cout, "    "};
    t->accept(&pv);
  }
  spdlog::info("Finished printing AST");
}

// Demonstrate some basic assertions.
// TEST(ParserTest, BasicAssertions) {
//   // Expect two strings not to be equal.
//   EXPECT_STRNE("hello", "world");
//   // Expect equality.
//   EXPECT_EQ(7 * 6, 42);
//   std::vector<int> vi{9, 4, 5, 2, 9, 1, 0, 2, 6, 7, 4, 5, 6, 5, 9, 2, 7,
//                       1, 4, 5, 3, 8, 5, 0, 2, 9, 3, 7, 5, 7, 5, 5, 6, 1,
//                       4, 3, 1, 8, 4, 0, 7, 8, 8, 2, 6, 5, 3, 4, 5};
//   vi |= ranges::actions::sort | ranges::actions::unique;
//   // prints: [0,1,2,3,4,5,6,7,8,9]
//   std::string res = fmt::to_string(fmt::join(ranges::views::all(vi), ","));
//   // fmt::println("Hello, the answer is [{}]", res);
//   ASSERT_FALSE(RE2::FullMatch(res, "1,2"));
//   ASSERT_TRUE(RE2::PartialMatch(res, "1,2"));
//
//   using nlohmann::json;
//
//   // spdlog::info has some difficulties converting MakeString to string
//   std::string kek = utils::MakeString()
//                     << json::parse(R"({ "hello" : ["world", 42] })");
//   spdlog::info(kek);
//   spdlog::error("fuck!");
// }
