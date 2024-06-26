// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "stringpool.h"
// ---------------------------------------------------------------------------
#include <cassert>
#include <iostream>
#include <algorithm>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
std::vector<std::string> StringPool::strings_(1);
// ---------------------------------------------------------------------------
unsigned StringPool::insert(const char* str) {
   return insert(std::string(str));
}
// ---------------------------------------------------------------------------
unsigned StringPool::insert(std::string&& str) {
   auto it = std::find(strings_.begin(), strings_.end(), str);
   if (it != strings_.end()) {
      return std::distance(strings_.begin(), it);
   }
   strings_.emplace_back(std::move(str));
   return strings_.size() - 1;
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