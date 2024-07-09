// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "stringpool.h"
// ---------------------------------------------------------------------------
#include <cassert>
#include <iostream>
#include <algorithm>
#include <string_view>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
std::vector<std::string> StringPool::strings_(1);
std::unordered_map<std::size_t, std::vector<unsigned>> StringPool::map_{};
// ---------------------------------------------------------------------------
unsigned StringPool::insert(const char* str) {
   return insert(std::string(str));
}
// ---------------------------------------------------------------------------
unsigned StringPool::insert(std::string&& str) {
   std::size_t h = std::hash<std::string>{}(str);
   auto mIt = map_.find(h);
   if (mIt != map_.end()) {
      std::vector<unsigned>& range = mIt->second;
      auto vIt = std::find_if(range.begin(), range.end(), [&str](unsigned idx) {
         return strings_[idx] == str;
      });

      if (vIt != range.end()) {
         return *vIt;
      }
   } 
   unsigned idx = strings_.size();
   strings_.emplace_back(std::move(str));
   if (mIt == map_.end()) {
      map_[h] = {idx};
   } else {
      map_[h].push_back(idx);
   }
   return idx;
}
// ---------------------------------------------------------------------------
Ident::operator std::string_view() const {
   return StringPool::get(tag);
}
// ---------------------------------------------------------------------------
Ident::operator std::string() const {
   return std::string(StringPool::get(tag));
}
// ---------------------------------------------------------------------------
Ident::operator bool() const {
   return tag != 0;
}
// ---------------------------------------------------------------------------
bool Ident::operator==(const Ident& other) const {
   return tag == other.tag;
}
// ---------------------------------------------------------------------------
bool Ident::operator!=(const Ident& other) const {
   return tag != other.tag;
}
// ---------------------------------------------------------------------------
std::string_view StringPool::get(unsigned idx) {
   assert(idx < strings_.size() && "StringPool::get: index out of bounds");
   return strings_[idx];
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Ident& ident) {
   return os << static_cast<std::string_view>(ident);
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------