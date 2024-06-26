#ifndef QCP_TOKEN_H
#define QCP_TOKEN_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "loc.h"
#include "stringpool.h"
// ---------------------------------------------------------------------------
#include <cassert>
// Alexis hates iostream
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
// ---------------------------------------------------------------------------
namespace qcp {
namespace token {
// ---------------------------------------------------------------------------
enum class Kind {
#include "defs/tokens.def"
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Kind& tt);
// ---------------------------------------------------------------------------
struct GPerfToken {
   const char* keyword;
   int tokenType = static_cast<int>(Kind::UNKNOWN);
};
// ---------------------------------------------------------------------------
class Token {
   public:
   using data_t = std::variant<Ident, std::string, unsigned long long, long double>;

   Token() : type{Kind::UNKNOWN} {}

   Token(SrcLoc loc, float value) : type{Kind::FCONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, double value) : type{Kind::DCONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, long double value) : type{Kind::LDCONST}, value{value}, loc_{loc} {}

   Token(SrcLoc loc, unsigned long long value) : type{Kind::ULL_ICONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, unsigned long value) : type{Kind::UL_ICONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, unsigned value) : type{Kind::U_ICONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, long long value) : type{Kind::LL_ICONST}, value{static_cast<unsigned long long>(value)}, loc_{loc} {}
   Token(SrcLoc loc, long value) : type{Kind::L_ICONST}, value{static_cast<unsigned long long>(value)}, loc_{loc} {}
   Token(SrcLoc loc, int value) : type{Kind::ICONST}, value{static_cast<unsigned long long>(value)}, loc_{loc} {}

   Token(SrcLoc loc, std::string&& value, Kind type) : type{type}, value{std::move(value)}, loc_{loc} { assert(type == Kind::SLITERAL || type == Kind::CLITERAL); }
   Token(SrcLoc loc, Ident id, Kind type = Kind::IDENT) : type{type}, value{id}, loc_{loc} { assert(type != Kind::SLITERAL && type != Kind::CLITERAL); }

   explicit Token(Kind type) : type{type}, loc_{} {}
   Token(SrcLoc loc, Kind type) : type{type}, loc_{loc} {}
   Token(SrcLoc loc, const GPerfToken& gperfToken) : type{static_cast<Kind>(gperfToken.tokenType)}, loc_{loc} {}

   Kind getKind() const {
      return type;
   }

   friend std::ostream& operator<<(std::ostream& os, const Token& token);

   operator bool() const {
      return type != Kind::UNKNOWN && type != Kind::END;
   }

   bool operator==(const Token& other) const {
      // todo: compare identifier and values
      return type == other.type;
   }

   auto operator<=>(const Token& other) const {
      return type <=> other.type;
   }

   template <typename T>
   T getValue() const {
      if constexpr (std::is_integral_v<T>) {
         return static_cast<T>(std::get<unsigned long long>(value));
      } else if constexpr (std::is_floating_point_v<T>) {
         return static_cast<T>(std::get<long double>(value));
      } else {
         static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, Ident>, "Invalid type for Token::getValue");
         return std::get<T>(value);
      }
   }

   std::string_view getString() const {
      return std::get<std::string>(value);
   }

   SrcLoc getLoc() const {
      return loc_;
   }

   data_t getRawValue() const {
      return value;
   }

   private:
   Kind type;
   data_t value;
   SrcLoc loc_;
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Token& token);
// ---------------------------------------------------------------------------
} // namespace token
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOKEN_H