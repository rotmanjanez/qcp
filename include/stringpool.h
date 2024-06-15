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
   static unsigned insert(std::string&& str);

   static std::string_view get(unsigned idx);

   private:
   static std::vector<std::string> strings_;
};
// ---------------------------------------------------------------------------
class Ident {
   public:
   Ident() : tag{0} {}
   explicit Ident(unsigned tag) : tag{tag} {}
   explicit Ident(std::string_view str) : tag{StringPool::insert(std::string(str))} {}
   explicit Ident(const char* str) : tag{StringPool::insert(str)} {}
   explicit Ident(std::string&& str) : tag{StringPool::insert(std::move(str))} {}

   operator std::string_view() const;
   operator std::string() const;
   operator bool() const;

   // stringlike template
   template <typename T>
   Ident operator+(T rhs) const {
      return Ident{std::string(*this) + rhs};
   }

   Ident prefix(const char* str) const {
      return Ident{std::string(str) + std::string(*this)};
   }

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