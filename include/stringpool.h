#ifndef QCP__STRINGPOOL_H
#define QCP__STRINGPOOL_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
class StringPool {
   public:
   static unsigned insert(const char *str);
   static unsigned insert(std::string &&str);

   static std::string_view get(unsigned idx);

   private:
   static std::unordered_map<std::size_t, std::vector<unsigned>> map_;
   static std::vector<std::string> strings_;
};
// ---------------------------------------------------------------------------
class Ident {
   public:
   Ident() : tag{0} {}
   explicit Ident(unsigned tag) : tag{tag} {}
   explicit Ident(std::string_view str) : tag{StringPool::insert(std::string(str))} {}
   explicit Ident(const char *str) : tag{StringPool::insert(str)} {}
   explicit Ident(std::string &&str) : tag{StringPool::insert(std::move(str))} {}

   operator std::string_view() const;
   operator std::string() const;
   operator bool() const;

   bool operator==(const Ident &other) const;
   bool operator!=(const Ident &other) const;

   // stringlike template
   template <typename T>
   Ident operator+(T rhs) const;

   template <typename T>
   Ident operator+(T *rhs) const;

   template <typename T>
   Ident &operator+=(T rhs);

   template <typename T>
   Ident &operator+=(T *rhs);

   Ident prefix(const char *str) const {
      return Ident{std::string(str) + std::string(*this)};
   }

   friend std::ostream &operator<<(std::ostream &os, const Ident &ident);
   friend std::hash<Ident>;

   private:
   unsigned tag;
};
// ---------------------------------------------------------------------------
template <typename T>
Ident Ident::operator+(T rhs) const {
   return Ident{std::string(*this) + std::string(rhs)};
}
// ---------------------------------------------------------------------------
template <typename T>
Ident Ident::operator+(T *rhs) const {
   return Ident{std::string(*this) + std::string(rhs)};
}
// ---------------------------------------------------------------------------
template <typename T>
Ident &Ident::operator+=(T rhs) {
   *this = *this + rhs;
   return *this;
}
// ---------------------------------------------------------------------------
template <typename T>
Ident &Ident::operator+=(T *rhs) {
   *this = *this + rhs;
   return *this;
}
// ---------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const Ident &ident);
// ---------------------------------------------------------------------------
template <typename T>
class tagged : public T {
   using T::T;

   template <typename U>
   friend std::ostream &operator<<(std::ostream &os, const tagged<U> &t);

   public:
   template <typename... Args>
   tagged(Ident name, Args &&...args) : T{std::forward<Args>(args)...}, name_{name} {}

   tagged(const T &other) : T{other} {};
   tagged(T &&other) : T{std::move(other)} {};

   operator T &() const {
      return static_cast<const T &>(*this);
   }

   operator T &() {
      return static_cast<T &>(*this);
   }

   bool operator==(const tagged &other) const {
      return name_ == other.name_ && static_cast<const T &>(*this) == static_cast<const T &>(other);
   }

   Ident name() const {
      return name_;
   }

   private:
   Ident name_{};
};
// ---------------------------------------------------------------------------
template <typename T>
std::ostream &operator<<(std::ostream &os, const tagged<T> &t) {
   return os << t.name() << ": " << static_cast<const T &>(t);
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
namespace std {
// ---------------------------------------------------------------------------
template <>
struct hash<qcp::Ident> {
   size_t operator()(qcp::Ident const &i) const noexcept {
      return hash<unsigned>{}(i.tag);
   }
};
// ---------------------------------------------------------------------------
} // namespace std
// ---------------------------------------------------------------------------
#endif // QCP__STRINGPOOL_H