#pragma once

/*******************************************************************************
 *                          Visitable implementation                           *
 *******************************************************************************/

template <typename...>
struct TypeList {};

namespace detail {

template <typename...>
struct IAbstractVisitable;

template <>
struct IAbstractVisitable<TypeList<>> {
  void accept() = delete;

  virtual ~IAbstractVisitable() = default;
};

template <typename TVisitor, typename R, typename... Args, typename... TSigTail>
struct IAbstractVisitable<TypeList<TypeList<TVisitor, R, Args...>, TSigTail...>>
    : IAbstractVisitable<TypeList<TSigTail...>> {
  using TBase = IAbstractVisitable<TypeList<TSigTail...>>;
  using TBase::accept;

  virtual R accept(TVisitor* visitor, Args... args) = 0;  // TODO: nolint
};

template <typename...>
struct VisitableImpl;

template <typename T, typename TSigs>
struct VisitableImpl<T, TypeList<>, TSigs> : IAbstractVisitable<TSigs> {};

template <typename T, typename TVisitor, typename R, typename... Args,
          typename... TRestSigs, typename TSigs>
struct VisitableImpl<T, TypeList<TypeList<TVisitor, R, Args...>, TRestSigs...>,
                     TSigs> : VisitableImpl<T, TypeList<TRestSigs...>, TSigs> {
  using TBase = VisitableImpl<T, TypeList<TRestSigs...>, TSigs>;
  using TBase::accept;

  R accept(TVisitor* visitor, Args... args) override {
    return visitor->visit(dynamic_cast<T*>(this), args...);
  }
};

}  // namespace detail

template <typename TVisitorList>
// An abstract class representing an object which can be visited by the any
// visitor in the list
using IVisitable = detail::IAbstractVisitable<TVisitorList>;

template <typename...>
struct TVisitable;

template <typename TDerived, typename... TSigs>
// Provides a default implementation of `accept` method. Iherit from it to not
// write anything manually
struct TVisitable<TDerived, TypeList<TSigs...>>
    : detail::VisitableImpl<TDerived, TypeList<TSigs...>, TypeList<TSigs...>> {
};
