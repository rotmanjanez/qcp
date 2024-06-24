#ifndef QCP_EMITTERTRAITS_H
#define QCP_EMITTERTRAITS_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "scopeinfo.h"
#include "type.h"
// ---------------------------------------------------------------------------
#include <variant>
// ---------------------------------------------------------------------------
namespace qcp {
namespace emitter {
// ---------------------------------------------------------------------------
template <typename _EmitterT>
struct emitter_traits {
   using ssa_t = typename _EmitterT::ssa_t;
   using const_t = typename _EmitterT::const_t;
   using iconst_t = typename _EmitterT::iconst_t;
   using phi_t = typename _EmitterT::phi_t;
   using bb_t = typename _EmitterT::bb_t;
   using ty_t = typename _EmitterT::ty_t;
   using fn_t = typename _EmitterT::fn_t;
   using sw_t = typename _EmitterT::sw_t;

   using value_t = std::variant<ssa_t*, const_t*, iconst_t*, fn_t*>;
   using const_or_iconst_t = std::variant<const_t*, iconst_t*>;
   using Type = type::Type<_EmitterT>;
   using ScopeInfo = ScopeInfo<_EmitterT>;
};
// ---------------------------------------------------------------------------
} // namespace emitter
} // namespace qcp
// ---------------------------------------------------------------------------
#endif
