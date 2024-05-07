#ifndef QCP__STRINGPOOL_H
#define QCP__STRINGPOOL_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include <string_view>
#include <vector>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
class StringPool {
   public:
   static unsigned insert(std::string_view str);

   static std::string_view get(unsigned idx);

   private:
   static std::vector<std::string_view> strings_;
};
// ---------------------------------------------------------------------------
class Ident {
   public:
   explicit Ident(unsigned tag) : tag{tag} {}
   explicit Ident(std::string_view str) : tag{StringPool::insert(str)} {}

   operator std::string_view() const;
   operator std::string() const;

   friend std::ostream& operator<<(std::ostream& os, const Ident& ident);

   private:
   unsigned tag;
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Ident& ident);
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP__STRINGPOOL_H