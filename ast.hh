#include <cstdio>
#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <type_traits>
#include <functional>

#include <cpputils/string.hh>
#include <cpputils/common.hh>

/*******************************************************************************
*                          Visitable implementation                           *
*******************************************************************************/

template<typename...>
struct TypeList {};

namespace detail {

template<typename...>
struct IAbstractVisitableImpl;

template<typename TVisitor, typename R, typename ...Args>
struct IAbstractVisitableImpl<TypeList<TVisitor, R, Args...>> {
    virtual R accept(TVisitor* visitor, Args... args) = 0;
};

template<typename...>
struct IKek;

template<typename ...TSigs>
struct IKek<TypeList<TSigs...>> : IAbstractVisitableImpl<TSigs>... {
  using IAbstractVisitableImpl<TSigs>::accept...;
};

template<typename...>
struct VisitableImpl;

template<typename T, typename TVisitor, typename R, typename ...Args, typename TSigs>
struct VisitableImpl<T, TypeList<TVisitor, R, Args...>, TSigs> : IKek<TSigs> {
  R accept(TVisitor* visitor, Args... args) override {
    return visitor->visit(dynamic_cast<T*>(this), args...);
  }
};

}  // namespace detail

template<typename...>
struct TVisitable;

template<typename TDerived, typename ...TSigs>
// Provides a default implementation of `accept` method. Iherit from it to not
// write anything manually
struct TVisitable<TDerived, TypeList<TSigs...>> : virtual detail::VisitableImpl<TDerived, TSigs, TypeList<TSigs...>>... {
  using detail::VisitableImpl<TDerived, TSigs, TypeList<TSigs...>>::accept...;

  virtual ~TVisitable() = default;
};

/*******************************************************************************
*                                   My shit                                   *
*******************************************************************************/


struct TStackVisitor;
struct TPrintVisitor;
struct TNameVisitor;

using TVisitorList = TypeList<
  TypeList<TStackVisitor, std::string, std::string>,
  TypeList<TPrintVisitor, void>,
  TypeList<TNameVisitor, std::string>
>;

/*******************************************************************************
*                                     Ast                                     *
*******************************************************************************/

using TNode = detail::IKek<TVisitorList>;

using TPtr = std::shared_ptr<TNode>;

struct TTree : TVisitable<TTree, TVisitorList> {
  std::vector<TPtr> children;
};

struct TNumber : TVisitable<TNumber, TVisitorList> {
  int val;
};

struct TString : TVisitable<TString, TVisitorList> {
  std::string val;
};

struct TId : TVisitable<TId, TVisitorList> {
  std::string val;
};

/*******************************************************************************
*                                  Visitors                                   *
*******************************************************************************/

struct TStackVisitor {
  std::string visit(TNumber* n, std::string stack) {
    return stack + ":number " + utils::ToString(n->val);
  }

  std::string visit(TString* str, std::string stack) {
    return stack + ":string " + str->val;
  }

  std::string visit(TId* id, std::string stack) {
    return stack + ":id " + id->val;
  }

  std::string visit(TTree* node, std::string stack) {
    return stack + ":node with " + utils::ToString(node->children.size()) + " children";
  }
};

struct TPrintVisitor {
public:
  TPrintVisitor() = delete;
  TPrintVisitor(std::ostream& os_, const char* indent_ = "    ")
    : os{os_}, indent{indent_}, indent_level{0} {}
  TPrintVisitor(const TPrintVisitor&) = default;

protected:
  void visit(TNumber* n, std::string){
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

  std::ostream &os;
  const char* indent;
  int indent_level = 0;
};

struct TNameVisitor {
  std::string visit(TNumber*) { return "number"; }
  std::string visit(TString*) { return "string"; }
  std::string visit(TId*) { return "identifier"; }
  std::string visit(TTree*) { return "unknown"; }
};

template<typename T>
T& GetInstance() {
  static T instance{};
  return instance;
}

template<typename ...Args>
TPtr MakePtr(Args&&... args) {
  return std::make_shared<TTree>(std::forward<Args>(args)...);
}

