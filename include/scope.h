#ifndef SCOPE_H
#define SCOPE_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "type.h"
// ---------------------------------------------------------------------------
#include <unordered_map>
#include <vector>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
// Inspired by implementation of Alexis Engelke
// ---------------------------------------------------------------------------
template <typename T, typename U>
class Scope {
   struct Entry {
      unsigned level;
      unsigned generation;
      U value;
   };

   class ScopeGuard {
      public:
      ScopeGuard(Scope& scope) : scope_{scope} {
         scope_.enter();
      }

      ~ScopeGuard() {
         scope_.leave();
      }

      private:
      Scope& scope_;
   };

   public:
   void enter();

   void leave();

   ScopeGuard guard() {
      return ScopeGuard{*this};
   }

   U* find(const T& name);

   U* insert(const T& name, const U& value);

   bool canInsert(const T& name);

   bool isTopLevel() const {
      return level_ == 0;
   }

   private:
   Entry* findEntry(const T& name);

   unsigned level_ = 0;
   std::vector<unsigned> generations_{0};
   std::unordered_map<T, std::vector<Entry>> symbols_{};
};
// ---------------------------------------------------------------------------
template <typename _EmitterT>
struct ScopeInfo {
   using TY = type::Type<_EmitterT>;
   using ssa_t = typename _EmitterT::ssa_t;
   using fn_t = typename _EmitterT::fn_t;

   ScopeInfo() = default;
   ScopeInfo(TY ty) : ty{ty}, ssa{nullptr} {}
   ScopeInfo(TY ty, ssa_t* ssa, bool hasDefOrInit) : ty{ty}, hasDefOrInit{hasDefOrInit}, ssa{ssa} {}
   ScopeInfo(TY ty, fn_t* fn, bool hasDefOrInit) : ty{ty}, hasDefOrInit{hasDefOrInit}, fn{fn} {}

   TY ty;
   bool hasDefOrInit;
   union {
      ssa_t* ssa;
      fn_t* fn;
   };
};
// ---------------------------------------------------------------------------
// Scope
// ---------------------------------------------------------------------------
template <typename T, typename U>
Scope<T, U>::Entry* Scope<T, U>::findEntry(const T& name) {
   auto mapIt = symbols_.find(name);
   if (mapIt == symbols_.end()) {
      return nullptr;
   }
   auto& vec = mapIt->second;
   auto vecIt = std::find_if(vec.rbegin(), vec.rend(), [&](const auto& e) {
      return e.level <= level_ && e.generation == generations_[e.level];
   });
   if (vecIt == vec.rend()) {
      return nullptr;
   }
   return &*vecIt;
}
// ---------------------------------------------------------------------------
template <typename T, typename U>
bool Scope<T, U>::canInsert(const T& name) {
   Entry* e = findEntry(name);
   return !e || e->level < level_;
}
// ---------------------------------------------------------------------------
template <typename T, typename U>
U* Scope<T, U>::find(const T& name) {
   Entry* e = findEntry(name);
   return e ? &e->value : nullptr;
}
// ---------------------------------------------------------------------------
template <typename T, typename U>
U* Scope<T, U>::insert(const T& name, const U& value) {
   auto& vec = symbols_[name];
   Entry* e = findEntry(name);

   if (e && e->level == level_) {
      return nullptr;
   }

   vec.push_back({level_, generations_[level_], value});
   return &vec.back().value;
}
// ---------------------------------------------------------------------------
template <typename T, typename U>
void Scope<T, U>::enter() {
   ++level_;
   if (generations_.size() <= level_) {
      generations_.push_back(0);
   } else {
      ++generations_[level_];
   }
}
// ---------------------------------------------------------------------------
template <typename T, typename U>
void Scope<T, U>::leave() {
   assert(level_ > 0 && "Cannot leave root scope");
   --level_;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // SCOPE_H