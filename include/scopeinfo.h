#ifndef QCP_SCOPEINFO_H
#define QCP_SCOPEINFO_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "emittertraits.h"
#include "loc.h"
#include "type.h"
// ---------------------------------------------------------------------------
#include <variant>
// ---------------------------------------------------------------------------
namespace qcp {
namespace scope {
// ---------------------------------------------------------------------------
template <typename T>
class ScopeInfo {
   using trait = emitter::emitter_traits<T>;

   using ssa_t = typename trait::ssa_t;
   using fn_t = typename trait::fn_t;
   using iconst_t = typename trait::iconst_t;
   using value_t = typename trait::value_t;

   using Ty = type::Type<T>;

   public:
   ScopeInfo() = default;
   ScopeInfo(Ty ty, SrcLoc loc) : ty{ty}, loc{loc}, data_{static_cast<ssa_t*>(nullptr)} {}

   template <typename U>
   ScopeInfo(Ty ty, SrcLoc loc, U u, bool hasDefOrInit) : ty{ty}, loc{loc}, hasDefOrInit{hasDefOrInit}, data_{u} {}

   // copy and move operations
   ScopeInfo(const ScopeInfo& other) = default;
   ScopeInfo(ScopeInfo&& other) noexcept = default;
   ScopeInfo& operator=(const ScopeInfo& other) = default;
   ScopeInfo& operator=(ScopeInfo&& other) noexcept = default;

   Ty ty;
   SrcLoc loc;
   bool hasDefOrInit;

   #define VARIANT_ACCESS_METHODS(name, type, member) \
      type &name() { return std::get<type>(data_); }  \
      const type &name() const { return std::get<type>(data_); }

   VARIANT_ACCESS_METHODS(ssa, ssa_t*, data_)
   VARIANT_ACCESS_METHODS(fn, fn_t*, data_)
   VARIANT_ACCESS_METHODS(iconst, iconst_t*, data_)

   #undef VARIANT_ACCESS_METHODS

   value_t data() const {
      return data_;
   }

   private:
   value_t data_;
};
// ---------------------------------------------------------------------------
} // namespace scope
} // namespace qcp
// ---------------------------------------------------------------------------
#endif