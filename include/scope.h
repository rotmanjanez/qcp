#ifndef QCP_SCOPE_H
#define QCP_SCOPE_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <vector>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
// Inspired by implementation of Alexis Engelke
// ---------------------------------------------------------------------------
template <typename T, typename U>
// todo: require default constructor for U and T
class Scope {
   struct Entry {
      Entry() = default;
      Entry(unsigned level, unsigned generation, const U& value) : level{level}, generation{generation}, value{value} {}

      // copy and move operations
      Entry(const Entry& other) = default;
      Entry(Entry&& other) noexcept = default;
      Entry& operator=(const Entry& other) = default;
      Entry& operator=(Entry&& other) noexcept = default;

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

   U* insert(T name, U value);

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
U* Scope<T, U>::insert(T name, U value) {
   std::vector<Entry>& vec = symbols_[name];
   Entry* e = findEntry(name);

   if (e && e->level == level_) {
      return nullptr;
   }

   vec.emplace_back(level_, generations_[level_], std::move(value));
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