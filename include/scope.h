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

   public:
   void enter();

   void leave();

   U* find(const T& name);

   bool insert(const T& name, const U& value);

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
   ScopeInfo(TY ty, ssa_t* ssa) : ty{ty}, ssa{ssa} {}
   ScopeInfo(TY ty, fn_t* fn) : ty{ty}, fn{fn} {}

   TY ty;
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
U* Scope<T, U>::find(const T& name) {
   Entry* e = findEntry(name);
   return e ? &e->value : nullptr;
}
// ---------------------------------------------------------------------------
template <typename T, typename U>
bool Scope<T, U>::insert(const T& name, const U& value) {
   auto& vec = symbols_[name];
   Entry* e = findEntry(name);

   if (e && e->level == level_) {
      return false;
   }

   vec.push_back({level_, generations_[level_], value});
   std::cout << "Inserted: " << name << " (lvl: " << level_ << " gen: " << generations_[level_] << ")" << std::endl;
   return true;
}
// ---------------------------------------------------------------------------
template <typename T, typename U>
void Scope<T, U>::enter() {
   std::cout << "Entering scope (lvl: " << level_ << " gen: " << generations_[level_] << ")" << std::endl;
   for (const auto& [name, vec] : symbols_) {
      if (find(name)) {
         std::cout << "In scope: " << name << std::endl;
      } else {
         // std::cout << "Not in scope: " << name << "(lvl: " << vec.back().level << " gen: " << vec.back().generation << " )" << std::endl;
      }
   }
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
   std::cout << "Leaving scope" << std::endl;
   // list all symbols that are going out of scope
   for (const auto& [name, vec] : symbols_) {
      if (vec.empty()) {
         continue;
      }
      if (vec.back().generation == generations_[level_]) {
         std::cout << "Out of scope: " << name << std::endl;
      }
   }
   assert(level_ > 0 && "Cannot leave root scope");
   --level_;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // SCOPE_H