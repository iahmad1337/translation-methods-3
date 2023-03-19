#pragma once

#include <cpputils/common.hh>
#include <cpputils/string.hh>
#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include <array>

#include "visit.hh"

struct TPrintVisitor;
struct TNameVisitor;
struct TPyToCVisitor;
struct TCToCodeVisitor;

using TVisitorList = TypeList<TypeList<TPrintVisitor, void>,
                              TypeList<TNameVisitor, std::string>,
                              TypeList<TPyToCVisitor, std::string>>;

using TNode = IVisitable<TVisitorList>;

using TPtr = std::shared_ptr<TNode>;

struct TTree : TVisitable<TTree, TVisitorList> {
  template<typename ...Args>
  TTree(std::string name_, Args&&... children_) : name{std::move(name_)}, children{std::forward<Args>(children_)...} {}

  std::string name;
  std::vector<TPtr> children;
};

struct TNumber : TVisitable<TNumber, TVisitorList> {
  TNumber(int val_) : val{val_} {}

  static std::shared_ptr<TNumber> make(int val_) {
    return std::make_shared<TNumber>(val_);
  }

  int val;
};

struct TString : TVisitable<TString, TVisitorList> {
  TString(std::string val_) : val{val_} {}

  std::string val;
};

struct TId : TVisitable<TId, TVisitorList> {
  TId(std::string val_) : val{val_} {}

  std::string val;
};

/*******************************************************************************
 *                                  Visitors                                   *
 *******************************************************************************/

struct TPrintVisitor {
 public:
  TPrintVisitor() = delete;
  TPrintVisitor(std::ostream& os_, const char* indent_ = "    ")
      : os{os_}, indent{indent_}, indent_level{0} {}
  TPrintVisitor(const TPrintVisitor&) = default;

  void visit(TNumber* n) {
    AddIndent();
    os << utils::Format("TNumber: `%`\n", n->val);
  }

  void visit(TString* str) {
    AddIndent();
    os << utils::Format("TString: `%`\n", str->val);
  }

  void visit(TId* id) {
    AddIndent();
    os << utils::Format("TId: `%`\n", id->val);
  }

  void visit(TTree* node) {
    AddIndent();
    os << utils::Format("`%` with % children\n", node->name, node->children.size());
    indent_level++;
    for (auto& c : node->children) {
      c->accept(this);
    }
    indent_level--;
  }

 private:
  void AddIndent() {
    for (int i = 0; i < indent_level; i++) {
      os << indent;
    }
  }

  std::ostream& os;
  const char* indent;
  int indent_level = 0;
};

struct TNameVisitor {
  std::string visit(TNumber*) { return "number"; }
  std::string visit(TString*) { return "string"; }
  std::string visit(TId*) { return "identifier"; }
  std::string visit(TTree*) { return "tree"; }
};

constexpr auto C_TEMPLATE = R"(
#include <stdio.h>
#include <stdlib.h>

enum EType {
  NUMBER,
  STRING
};

struct TVar {
  union {
    const char* str;
    int num = 0;
  };
  EType type = NUMBER;
};

void print(TVar* v) {
  switch (v->type) {
    case NUMBER:
      printf("%d\n", v->num);
      break;
    case STRING:
      printf("%s\n", v->str);
      break;
  }
}

const char* input() {
  static const int MAX_STR_SIZE = 512;
  char* result = malloc(MAX_STR_SIZE);
  if (!result) {
    perror("input allocation");
    abort();
  }
  char* succ = gets_s(result, MAX_STR_SIZE);
  if (!succ) {
    perror("input gets_s");
    abort();
  }
  return result;
}

struct TRange {
  int from;
  int to;
  int step;
};

int InRange(const TRange* r, int i) {
  if (r->from < r->to) {
    return r->from <= i && i <= r->to;
  } else {
    return r->to <= i && i <= r->from;
  }
}

TVar {{vars}};

int main() {
{{program}};
}
)";

constexpr auto FOR_TEMPLATE = R"(
{
  const TRange __{{iterator}}_range = {{range_expr}};
  // const TRange __{{iterator}}_range = {
  //   .from = {{from_expr}},
  //   .to = {{to_expr}},
  //   .step = {{step_expr}},
  // };
  for (int {{iterator}} = __{{iterator}}_range.from; InRange(&__{{iterator}}_range, i); i += __{{iterator}}_range.step) {
{{statements}}
  }
}
)";

constexpr auto IF_TEMPLATE = R"(
if ({{condition}}) {
{{statements}}
}
)";

struct TPyToCVisitor {
  std::string visit(TNumber* n) {
    return utils::ToString(n->val);
  }

  std::string visit(TString* s) {
    return utils::MakeString() << '"' << s->val << '"';;
  }

  std::string visit(TId* id) {
    return id->val;
  }

  std::string visit(TTree* t) {
    spdlog::info("entering {}", t->name);
    if (t->name == "file") {
      return utils::Replace(C_TEMPLATE, {
          {"{{vars}}", utils::Join(", ", vars.begin(), vars.end())},
          {"{{program}}", JoinChildren(t, "\n")},
      });
    } else if (t->name == "if_stmt") {
      auto [condition, statements] = VisitChildren<2>(t);

      return utils::Replace(IF_TEMPLATE, {
          {"{{condition}}", condition},
          {"{{statements}}", statements},
      });
    } else if (t->name == "for_loop") {
      auto [iterator, range_expr, statements] = VisitChildren<3>(t);

      return utils::Replace(FOR_TEMPLATE, {
          {"{{iterator}}", iterator},
          {"{{range_expr}}", range_expr},
          {"{{statements}}", statements},
      });
    } else if (t->name == "assign") {
      auto [lhs, rhs] = VisitChildren<2>(t);
      vars.insert(lhs);
      return utils::Format("% = %;", lhs, rhs);
    } else if (t->name == "invoke") {
      auto funcName = t->children[0]->accept(this);
      auto argTree = dynamic_cast<TTree*>(t->children[1].get());
      assert(argTree);
      auto args = ProcessChildren(argTree);
      if (funcName == "print") {
        assert(args.size() == 1);
        return utils::Format("print(&%)", args[0]);
      } else if (funcName == "int") {
        assert(args.size() == 1);
        return utils::Format("atoi(%)", args[0]);
      } else if (funcName == "input") {
        assert(args.size() == 0);
        return "input()";
      } else {
        return utils::Format("%(%)", funcName, utils::Join(", ", args.begin(), args.end()));
      }
    } else if (t->name == "arglist") {
      return JoinChildren(t);
    } else if (t->name == "simple_stmt") {
      auto [stmt] = VisitChildren<1>(t);
      return utils::MakeString() << stmt << ";";
    } else if (utils::OneOf(t->name, {"or", "and", "not", "eq", "neq", "less", "greater", "sub", "add", "multiply"})) {
      auto [lhs, rhs] = VisitChildren<2>(t);
      return utils::Format("%(%, %)", t->name, lhs, rhs);
    } else {
      // else this is a wrapper-node that only has one child
      auto [unwrapped] = VisitChildren<1>(t);
      return unwrapped;
    }
  }

private:
  template<int C>
  std::array<std::string, C> VisitChildren(TTree* t) {
    assert(t->children.size() == C);
    std::array<std::string, C> result;
    for (int i = 0; i < C; i++) {
      result[i] = t->children[i]->accept(this);
    }
    return result;
  }

  std::vector<std::string> ProcessChildren(TTree* t) {
    std::vector<std::string> result(t->children.size());
    std::transform(t->children.begin(), t->children.end(), result.begin(),
        [&vis = *this] (TPtr p) { return p->accept(&vis); });
    return result;
  }

  std::string JoinChildren(TTree* t, std::string_view sep = ", ") {
    auto result = ProcessChildren(t);
    return utils::Join(sep, result.begin(), result.end());
  }

  std::string AddIndent(std::string str) {
    std::string newIndent = "\n";
    for (int i = 0; i < indentLevel; i++) {
      newIndent.append(INDENT);
    }
    return utils::Replace(str, { {"\n", newIndent} });
  }

  static constexpr auto INDENT = "  ";
  int indentLevel{0};
  std::unordered_set<std::string> vars = { "__dummy" };
};
