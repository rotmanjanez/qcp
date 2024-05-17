#ifndef SCOPE_H
#define SCOPE_H
// ---------------------------------------------------------------------------
// qcp
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
   unsigned level_ = 0;
   std::vector<unsigned> generations_{0};
   std::unordered_map<T, std::vector<Entry>> symbols_{};
};
// ---------------------------------------------------------------------------
// Scope
// ---------------------------------------------------------------------------
template <typename T, typename U>
U* Scope<T, U>::find(const T& name) {
   auto it = symbols_.find(name);
   if (it == symbols_.end()) {
      return nullptr;
   }
   Entry& e = it->second.back();
   if (e.generation != generations_[level_]) {
      return nullptr;
   }
   return &e.value;
}
// ---------------------------------------------------------------------------
template <typename T, typename U>
bool Scope<T, U>::insert(const T& name, const U& value) {
   auto& vec = symbols_[name];
   if (vec.empty() || vec.back().generation != generations_[level_]) {
      vec.push_back({level_, generations_[level_], value});
      return true;
   }
   return false;
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