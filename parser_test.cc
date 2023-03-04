#include <vector>
#include <iostream>

#include <gtest/gtest.h>

// #include <fmt/core.h>
// #include <fmt/ranges.h>

#include <range/v3/action/sort.hpp>
#include <range/v3/action/unique.hpp>
#include <range/v3/view/all.hpp>

#include <re2/re2.h>
#include <nlohmann/json.hpp>
// TODO: there are problems with spdlog and fmt - can't make them work together
// for now, I think I just have to use them exclusively
#include <spdlog/spdlog.h>

#include "cpputils/common.hh"

#include "ast.hh"

TEST(VisitorTest, BasicAssertions) {
  std::shared_ptr<TNode> t = std::make_shared<TTree>(
    std::vector<TPtr>{}
  );

  TStackVisitor sv;
  TNameVisitor nv;
  auto [stack, name] = std::make_tuple(
      t->accept(&sv, "stack beginning"),
      t->accept(&nv)
  );
  ASSERT_EQ(stack, "stack beginning:node with 0 children");
  ASSERT_EQ(name, "unknown");

  t = std::make_shared<TTree>(
    std::vector<TPtr>{
      std::make_shared<TId>("identifier1"),
      std::make_shared<TString>("string literal 1"),
      std::make_shared<TNumber>(42),
      std::make_shared<TTree>(
        std::vector<TPtr>{
          std::make_shared<TNumber>(228),
          std::make_shared<TId>("identifier2"),
        }
      ),
    }
  );
  spdlog::info("Started printing AST");
  {
    TPrintVisitor pv{std::cout, "    "};
    t->accept(&pv);
  }
  spdlog::info("Finished printing AST");
}

// Demonstrate some basic assertions.
TEST(ParserTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
    std::vector<int> vi{9, 4, 5, 2, 9, 1, 0, 2, 6, 7, 4, 5, 6, 5, 9, 2, 7,
                      1, 4, 5, 3, 8, 5, 0, 2, 9, 3, 7, 5, 7, 5, 5, 6, 1,
                      4, 3, 1, 8, 4, 0, 7, 8, 8, 2, 6, 5, 3, 4, 5};
  vi |= ranges::actions::sort | ranges::actions::unique;
  // prints: [0,1,2,3,4,5,6,7,8,9]
  std::string res = fmt::to_string(fmt::join(ranges::views::all(vi), ","));
  // fmt::println("Hello, the answer is [{}]", res);
  ASSERT_FALSE(RE2::FullMatch(res, "1,2"));
  ASSERT_TRUE(RE2::PartialMatch(res, "1,2"));

  using nlohmann::json;

  // spdlog::info has some difficulties converting MakeString to string
  std::string kek = utils::MakeString() << json::parse(R"({ "hello" : ["world", 42] })");
  spdlog::info(kek);
  spdlog::error("fuck!");
}
