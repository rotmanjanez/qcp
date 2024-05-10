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
   static unsigned insert(const char* str);
   static unsigned insert(std::string_view str);

   static std::string_view get(unsigned idx);

   private:
   static std::vector<std::string_view> strings_;
};
// ---------------------------------------------------------------------------
class Ident {
   public:
   Ident() : tag{0} {}
   explicit Ident(unsigned tag) : tag{tag} {}
   explicit Ident(std::string_view str) : tag{StringPool::insert(str)} {}
   explicit Ident(const char* str) : tag{StringPool::insert(str)} {}

   operator std::string_view() const;
   operator std::string() const;
   operator bool() const;

   friend std::ostream& operator<<(std::ostream& os, const Ident& ident);
   friend std::hash<Ident>;

   private:
   unsigned tag;
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Ident& ident);
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
namespace std {
// ---------------------------------------------------------------------------
template <>
struct std::hash<qcp::Ident> {
   std::size_t operator()(qcp::Ident const& i) const noexcept {
      return hash<unsigned>{}(i.tag);
   }
};
// ---------------------------------------------------------------------------
} // namespace std
// ---------------------------------------------------------------------------
#endif // QCP__STRINGPOOL_H