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
// ---------------------------------------------------------------------------
namespace qcp {
namespace token {
// ---------------------------------------------------------------------------
// clang-format off
enum Kind {
   // tokens in operator precedence order (as complete as possible) starting from the arethmetic operators (precedence 3)
   ASTERISK, DIV, PERCENT,
   PLUS, MINUS,
   SHL, SHR,
   L_ANGLE, LE, R_ANGLE, GE,
   EQ, NE,
   BW_AND, BW_XOR, BW_OR,
   L_AND, L_OR,
   __UNUSED__, // TERNARY CONDITIONAL MISSING
   ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, REM_ASSIGN, SHL_ASSIGN, SHR_ASSIGN, BW_AND_ASSIGN, BW_XOR_ASSIGN, BW_OR_ASSIGN,
   COMMA,
   // end of operator precedence order

   UNKNOWN,

   IDENT,

   ICONST, U_ICONST, L_ICONST, UL_ICONST, LL_ICONST, ULL_ICONST,
   
   FCONST, DCONST, LDCONST,
   
   WB_ICONST, UWB_ICONST,
   SLITERAL, CLITERAL,

   // punctuators
   L_BRACKET, R_BRACKET, L_BRACE, R_BRACE, L_C_BRKT, R_C_BRKT,
   PERIOD, // .
   DEREF, // ->

   INC, DEC, 

   BW_INV, // ~
   NEG, // !

   QMARK, // ?
   COLON, // :
   D_COLON, // ::
   SEMICOLON, // ;
   ELLIPSIS, // ...

   // todo: #
   // todo: ##

   // storage class specifier except for typedef
   AUTO, CONSTEXPR, EXTERN, REGISTER, STATIC, THREAD_LOCAL, TYPEDEF,

   // function specifier
   INLINE, NORETURN,

   // other keywords
   SWITCH, BREAK, CASE, CONTINUE, DEFAULT, WHILE, DO, ELSE, FOR, GOTO, IF, RETURN,
   FALSE, TRUE, NULLPTR,
   
   STATIC_ASSERT, SIZEOF, ALIGNOF,
   GENERIC, IMAGINARY,
   
   // type specifier
   VOID, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, SIGNED, UNSIGNED, 
   BITINT, BOOL, COMPLEX,
   DECIMAL32, DECIMAL64, DECIMAL128,

   STRUCT, UNION, ENUM,
   TYPEOF, TYPEOF_UNQUAL,

   CONST, RESTRICT, VOLATILE, ATOMIC,
   ALIGNAS,

   END,
};
// clang-format on
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Kind& tt);
// ---------------------------------------------------------------------------
struct GPerfToken {
   const char* keyword;
   int tokenType = Kind::UNKNOWN;
};
// ---------------------------------------------------------------------------
struct ConstExprValue {
   ConstExprValue(long double value) : ld{value} {}
   ConstExprValue(double value) : d{value} {}
   ConstExprValue(float value) : f{value} {}
   ConstExprValue(unsigned long long value) : ull{value} {}
   ConstExprValue(unsigned long value) : ul{value} {}
   ConstExprValue(unsigned value) : u{value} {}
   ConstExprValue(long long value) : ll{value} {}
   ConstExprValue(long value) : l{value} {}
   ConstExprValue(int value) : i{value} {}

   union {
      long double ld;
      double d;
      float f;
      unsigned long long ull;
      unsigned long ul;
      unsigned u;
      long long ll;
      long l;
      int i;
   };
};
// ---------------------------------------------------------------------------
class Token {
   public:
   Token() : type{Kind::UNKNOWN} {}

   Token(SrcLoc loc, float value) : type{Kind::FCONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, double value) : type{Kind::DCONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, long double value) : type{Kind::LDCONST}, value{value}, loc_{loc} {}

   Token(SrcLoc loc, unsigned long long value) : type{Kind::ULL_ICONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, unsigned long value) : type{Kind::UL_ICONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, unsigned value) : type{Kind::U_ICONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, long long value) : type{Kind::LL_ICONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, long value) : type{Kind::L_ICONST}, value{value}, loc_{loc} {}
   Token(SrcLoc loc, int value) : type{Kind::ICONST}, value{value}, loc_{loc} {}

   Token(SrcLoc loc, std::string_view value, Kind type = Kind::IDENT) : type{type}, ident{value}, loc_{loc} {}

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
      if constexpr (std::is_same_v<T, double>) {
         return value.f;
      } else if constexpr (std::is_same_v<T, unsigned long long>) {
         return value.ull;
      } else if constexpr (std::is_same_v<T, unsigned long>) {
         return value.ul;
      } else if constexpr (std::is_same_v<T, unsigned>) {
         return value.u;
      } else if constexpr (std::is_same_v<T, long long>) {
         return value.ll;
      } else if constexpr (std::is_same_v<T, long>) {
         return value.l;
      } else if constexpr (std::is_same_v<T, int>) {
         return value.i;
      } else if constexpr (std::is_same_v<T, Ident>) {
         return ident;
      } else {
         assert(false && "Unsupported type");
      }
   }

   SrcLoc getLoc() const {
      return loc_;
   }

   ConstExprValue getRawValue() const {
      return value;
   }

   private:
   Kind type;
   union {
      Ident ident;
      ConstExprValue value;
   };
   SrcLoc loc_;
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Token& token);
// ---------------------------------------------------------------------------
} // namespace token
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOKEN_H