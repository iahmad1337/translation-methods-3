#pragma once

#include <cpputils/common.hh>
#include <cpputils/string.hh>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "visit.hh"

struct TPrintVisitor;
struct TNameVisitor;

using TVisitorList = TypeList<TypeList<TPrintVisitor, void>,
                              TypeList<TNameVisitor, std::string> >;

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
    os << utils::Format("TTree with `%` children\n", node->children.size());
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

template <typename T>
T& GetInstance() {
  static T instance{};
  return instance;
}

template <typename... Args>
TPtr MakePtr(Args&&... args) {
  return std::make_shared<TTree>(std::forward<Args>(args)...);
}
