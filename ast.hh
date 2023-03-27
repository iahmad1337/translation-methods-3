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

const char* input() {
  static const int MAX_STR_SIZE = 512;
  char* result = (char*)malloc(MAX_STR_SIZE);
  if (!result) {
    perror("input allocation");
    abort();
  }
  if (!fgets(result, MAX_STR_SIZE, stdin)) {
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

int InRange(const struct TRange* r, int i) {
  if (r->from == r->to && r->step == 0) {
    return 0;  // empty range
  }
  if (r->from < r->to) {
    return r->from <= i && i <= r->to;
  } else {
    return r->to <= i && i <= r->from;
  }
}

int {{vars}};

int main() {
{{program}}
}
)";

constexpr auto FOR_TEMPLATE = R"(
{
  const struct TRange __{{iterator}}_range = {{range_expr}};
  for (int {{iterator}} = __{{iterator}}_range.from; InRange(&__{{iterator}}_range, i); i += __{{iterator}}_range.step) {
{{statements}}
  }
}
)";

constexpr auto IF_TEMPLATE = R"(
if ({{condition}}) {
{{statements}}
} else {
{{if_cont}}
}
)";
constexpr auto WHILE_TEMPLATE = R"(
while ({{condition}}) {
{{statements}}
}
)";

struct TPyToCVisitor {
  std::string visit(TNumber* n) {
    return utils::ToString(n->val);
  }

  std::string visit(TString* s) {
    return utils::MakeString() << '"' << s->val << '"';
  }

  std::string visit(TId* id) {
    return id->val;
  }

  std::string visit(TTree* t) {
    spdlog::info("entering {}", t->name);
    if (t->name == "file") {
      auto program = JoinChildren(t, "\n");
      TLevelGuard _guard{&indentLevel};
      return utils::Replace(C_TEMPLATE, {
          {"{{vars}}", utils::Join(vars, ", ")},
          {"{{program}}", AddIndent(program)},
      });
    } else if (t->name == "statements") {
      return JoinChildren(t, "\n");
    } else if (t->name == "if_stmt") {
      auto [condition, statements, cont] = VisitChildren<3>(t);

      TLevelGuard _guard{&indentLevel};
      return utils::Replace(IF_TEMPLATE, {
          {"{{condition}}", condition},
          {"{{statements}}", AddIndent(statements)},
          {"{{if_cont}}", AddIndent(cont)},
      });
    } else if (t->name == "else_stmt") {
      auto [statements] = VisitChildren<1>(t);

      return statements;
    } else if (t->name == "while_loop") {
      auto [condition, statements] = VisitChildren<2>(t);

      TLevelGuard _guard{&indentLevel};
      return utils::Replace(WHILE_TEMPLATE, {
          {"{{condition}}", condition},
          {"{{statements}}", AddIndent(statements)},
      });
    } else if (t->name == "for_loop") {
      auto [iterator, range_expr, statements] = VisitChildren<3>(t);

      TLevelGuard _guard{&indentLevel};
      return utils::Replace(FOR_TEMPLATE, {
          {"{{iterator}}", iterator},
          {"{{range_expr}}", range_expr},
          {"{{statements}}", AddIndent(statements)},
      });
    } else if (t->name == "assign") {
      auto [lhs, rhs] = VisitChildren<2>(t);
      vars.insert(lhs);
      spdlog::info("assigning to {}", lhs);
      return utils::Format("% = %", lhs, rhs);
    } else if (t->name == "invoke") {
      auto funcName = t->children[0]->accept(this);
      auto argTree = dynamic_cast<TTree*>(t->children[1].get());
      assert(argTree);
      auto args = ProcessChildren(argTree);
      if (funcName == "print") {
        assert(args.size() == 1);
        if (args[0].front() == '"' && args[0].back() == '"') {
          return utils::Format(R"(printf(%))", args[0]);
        } else {
          return utils::Format(R"(printf("\%d\\n", %))", args[0]);
        }
      } else if (funcName == "int") {
        assert(args.size() == 1);
        return utils::Format("atoi(%)", args[0]);
      } else if (funcName == "input") {
        assert(args.size() == 0);
        return "input()";
      } else if (funcName == "range") {
        switch (args.size()) {
          case 1:
            return utils::Format("{ .from = 0, .to = %, .step = 1 }", args[0]);
          case 2:
            return utils::Format("{ .from = %, .to = %, .step = 1 }", args[0], args[1]);
          case 3:
            return utils::Format("{ .from = %, .to = %, .step = % }", args[0], args[1], args[2]);
          default:
            return "{ .from = 0, .to = 0, .step = 0 }";  // empty range on invalid call
        }
      } else {
        return utils::Format("%(%)", funcName, utils::Join(", ", args.begin(), args.end()));
      }
    } else if (t->name == "arglist") {
      return JoinChildren(t);
    } else if (t->name == "simple_stmt") {
      auto [stmt] = VisitChildren<1>(t);
      return utils::MakeString() << stmt << ";";
    } else if (utils::OneOf(t->name, {"||", "&&", "==", "!=", "<", ">", "-", "+", "*"})) {
      // binary operators
      auto [lhs, rhs] = VisitChildren<2>(t);
      return utils::Format("(% % %)", lhs, t->name, rhs);
    } else if (utils::OneOf(t->name, {"!"})) {
      // unary operators
      auto [arg] = VisitChildren<1>(t);
      return utils::Format("(! %)", arg);
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
    if (str.back() == '\n') {
      str.pop_back();
    }
    constexpr auto newIndent = "\n  ";
    str.insert(0, "  ");
    return utils::Replace(str, { {"\n", newIndent} });
  }

  struct TLevelGuard {
    explicit TLevelGuard(int* level) : levelHolder{level} {
      (*levelHolder)++;
    }

    ~TLevelGuard() {
      assert(levelHolder);
      (*levelHolder)--;
    }

  private:
    int* levelHolder = nullptr;
  };

  int indentLevel{1};
  std::unordered_set<std::string> vars = { "__dummy" };
};
