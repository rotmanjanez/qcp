#ifndef QCP_SCOPEINFO_H
#define QCP_SCOPEINFO_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "emittertraits.h"
#include "loc.h"
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
#define VARIANT_ACCESS_METHODS(name, type, member) \
   type& name() { return std::get<type>(data_); }  \
   const type& name() const { return std::get<type>(data_); }
// ---------------------------------------------------------------------------
namespace emitter {
template <typename _EmitterT>
struct emitter_traits;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
class ScopeInfo {
   using trait = emitter::emitter_traits<_EmitterT>;

   using Type = typename trait::Type;
   using ssa_t = typename trait::ssa_t;
   using fn_t = typename trait::fn_t;
   using iconst_t = typename trait::iconst_t;
   using value_t = typename trait::value_t;

   public:
   ScopeInfo() = default;
   ScopeInfo(Type ty, SrcLoc loc) : ty{ty}, loc{loc}, data_{static_cast<ssa_t*>(nullptr)} {}

   template <typename T>
   ScopeInfo(Type ty, SrcLoc loc, T t, bool hasDefOrInit) : ty{ty}, loc{loc}, hasDefOrInit{hasDefOrInit}, data_{t} {}

   // copy and move operations
   ScopeInfo(const ScopeInfo& other) = default;
   ScopeInfo(ScopeInfo&& other) noexcept = default;
   ScopeInfo& operator=(const ScopeInfo& other) = default;
   ScopeInfo& operator=(ScopeInfo&& other) noexcept = default;

   Type ty;
   SrcLoc loc;
   bool hasDefOrInit;

   template <typename T>
   bool is() const {
      return std::holds_alternative<T>(data_);
   }

   VARIANT_ACCESS_METHODS(ssa, ssa_t*, data_)
   VARIANT_ACCESS_METHODS(fn, fn_t*, data_)
   VARIANT_ACCESS_METHODS(iconst, iconst_t*, data_)

   private:
   value_t data_;
};
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif