// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "stringpool.h"
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
std::vector<std::string_view> StringPool::strings_{};
// ---------------------------------------------------------------------------
unsigned StringPool::insert(std::string_view str) {
   auto it = std::find(strings_.begin(), strings_.end(), str);
   if (it != strings_.end()) {
      return std::distance(strings_.begin(), it);
   }
   strings_.push_back(str);
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
std::string_view StringPool::get(unsigned idx) {
   return strings_[idx];
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Ident& ident) {
   return os << static_cast<std::string_view>(ident);
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------