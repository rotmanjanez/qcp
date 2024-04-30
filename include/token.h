#ifndef QCP_TOKEN_H
#define QCP_TOKEN_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "loc.h"
// ---------------------------------------------------------------------------
#include <cassert>
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
   AUTO, CONSTEXPR, EXTERN, REGISTER, STATIC, THREAD_LOCAL,

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
   TYPEDEF, TYPEOF, TYPEOF_UNQUAL,

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
class Token {
   public:
   Token() : type{Kind::UNKNOWN} {}

   explicit Token(float value) : type{Kind::FCONST}, fvalue{value} {}
   explicit Token(double value) : type{Kind::DCONST}, dvalue{value} {}
   explicit Token(long double value) : type{Kind::LDCONST}, ldvalue{value} {}

   explicit Token(unsigned long long value) : type{Kind::ULL_ICONST}, ullValue{value} {}
   explicit Token(unsigned long value) : type{Kind::UL_ICONST}, ulValue{value} {}
   explicit Token(unsigned value) : type{Kind::U_ICONST}, uValue{value} {}
   explicit Token(long long value) : type{Kind::LL_ICONST}, llValue{value} {}
   explicit Token(long value) : type{Kind::L_ICONST}, lValue{value} {}
   explicit Token(int value) : type{Kind::ICONST}, iValue{value} {}

   explicit Token(std::string_view value, Kind type = Kind::IDENT) : type{type}, ident{value} {}

   explicit Token(Kind type) : type{type} {}
   explicit Token(const GPerfToken& gperfToken) : type{static_cast<Kind>(gperfToken.tokenType)} {}

   Kind getType() const {
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
         return fvalue;
      } else if constexpr (std::is_same_v<T, unsigned long long>) {
         return ullValue;
      } else if constexpr (std::is_same_v<T, unsigned long>) {
         return ulValue;
      } else if constexpr (std::is_same_v<T, unsigned>) {
         return uValue;
      } else if constexpr (std::is_same_v<T, long long>) {
         return llValue;
      } else if constexpr (std::is_same_v<T, long>) {
         return lValue;
      } else if constexpr (std::is_same_v<T, int>) {
         return iValue;
      } else if constexpr (std::is_same_v<T, std::string_view>) {
         return ident;
      } else {
         assert(false && "Unsupported type");
      }
   }

   SrcLoc getLoc() const {
      return loc_;
   }

   private:
   Kind type;
   union {
      std::string_view ident;
      long double ldvalue;
      double dvalue;
      float fvalue;
      unsigned long long ullValue;
      unsigned long ulValue;
      unsigned uValue;
      long long llValue;
      long lValue;
      int iValue;
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