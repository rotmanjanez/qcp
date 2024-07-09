#ifndef QCPTRAITS_H
#define QCPTRAITS_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "type.h"
// ---------------------------------------------------------------------------
#include <variant>
// ---------------------------------------------------------------------------
namespace qcp {
namespace emitter {
// ---------------------------------------------------------------------------
template <typename T>
struct emitter_traits {
   using ssa_t = typename T::ssa_t;
   using const_t = typename T::const_t;
   using iconst_t = typename T::iconst_t;
   using phi_t = typename T::phi_t;
   using bb_t = typename T::bb_t;
   using ty_t = typename T::ty_t;
   using fn_t = typename T::fn_t;
   using sw_t = typename T::sw_t;

   using value_t = std::variant<std::monostate, ssa_t*, const_t*, iconst_t*, fn_t*>;
   using const_or_iconst_t = std::variant<std::monostate, const_t*, iconst_t*>;
   using Type = type::Type<T>;
};
// ---------------------------------------------------------------------------
} // namespace emitter
} // namespace qcp
// ---------------------------------------------------------------------------
#endif
