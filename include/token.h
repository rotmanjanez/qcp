#ifndef QCP_TOKEN_H
#define QCP_TOKEN_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include <iostream>
#include <string>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
enum TokenType {
   UNKNOWN,

   IDENT,

   ICONST,
   U_ICONST,
   L_ICONST,
   UL_ICONST,
   LL_ICONST,
   ULL_ICONST,
   WB_ICONST,
   UWB_ICONST,

   FCONST,
   LITERAL,

   // punctuators
   L_SQ_BRKT, // [
   R_SQ_BRKT, // ]
   L_BRKT, // (
   R_BRKT, // )
   L_C_BRKT, // {
   R_C_BRKT, // }
   PERIOD, // .
   DEREF, // ->
   INC, // ++
   DEC, // --
   BW_AND, // &
   MUL, // *
   PLUS, // +
   MINUS, // -
   BW_INV, // ~
   NEG, // !
   DIV, // /
   MOD, // %
   SHL, // <<
   SHR, // >>
   L_P_BRKT, // <
   R_P_BRKT, // >
   LE, // <=
   GE, // >=
   EQ, // ==
   NE, // !=
   BW_XOR, // ^
   BW_OR, // |
   L_AND, // &&
   L_OR, // ||
   QMARK, // ?
   COLON, // :
   D_COLON, // ::
   SEMICOLON, // ;
   ELLIPSIS, // ...
   ASSIGN, // =
   MUL_ASSIGN, // *=
   DIV_ASSIGN, // /=
   MOD_ASSIGN, // %=
   ADD_ASSIGN, // +=
   SUB_ASSIGN, // -=
   SHL_ASSIGN, // <<=
   SHR_ASSIGN, // >>=
   BW_AND_ASSIGN, // &=
   BW_XOR_ASSIGN, // ^=
   BW_OR_ASSIGN, // |=
   COMMA, // ,
   // todo: #
   // todo: ##

   // keywords
   ALIGNAS,
   ALIGNOF,
   AUTO,
   BOOL,
   BREAK,
   CASE,
   CHAR,
   CONST,
   CONSTEXPR,
   CONTINUE,
   DEFAULT,
   DO,
   DOUBLE,
   ELSE,
   ENUM,
   EXTERN,
   FALSE,
   FLOAT,
   FOR,
   GOTO,
   IF,
   INLINE,
   INT,
   LONG,
   NULLPTR,
   REGISTER,
   RESTRICT,
   RETURN,
   SHORT,
   SIGNED,
   SIZEOF,
   STATIC,
   STATIC_ASSERT,
   STRUCT,
   SWITCH,
   THREAD_LOCAL,
   TRUE,
   TYPEDEF,
   TYPEOF,
   TYPEOF_UNQUAL,
   UNION,
   UNSIGNED,
   VOID,
   VOLATILE,
   WHILE,
   ATOMIC,
   BITINT,
   COMPLEX,
   DECIMAL128,
   DECIMAL32,
   DECIMAL64,
   GENERIC,
   IMAGINARY,
   NORETURN,

   END,
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const TokenType& tt);
// ---------------------------------------------------------------------------
struct GPerfToken {
   const char* keyword;
   int tokenType = TokenType::UNKNOWN;
};
// ---------------------------------------------------------------------------
class Token {
   public:
   Token() : type{TokenType::UNKNOWN} {}

   explicit Token(double value) : type{TokenType::FCONST}, fvalue{value} {}

   explicit Token(unsigned long long value) : type{TokenType::ULL_ICONST}, ullValue{value} {}
   explicit Token(unsigned long value) : type{TokenType::UL_ICONST}, ulValue{value} {}
   explicit Token(unsigned value) : type{TokenType::U_ICONST}, uValue{value} {}
   explicit Token(long long value) : type{TokenType::LL_ICONST}, llValue{value} {}
   explicit Token(long value) : type{TokenType::L_ICONST}, lValue{value} {}
   explicit Token(int value) : type{TokenType::ICONST}, iValue{value} {}

   explicit Token(std::string_view value, TokenType type = TokenType::IDENT) : type{type}, ident{value} {}
   explicit Token(TokenType type) : type{type} {}
   explicit Token(const GPerfToken& gperfToken) : type{static_cast<TokenType>(gperfToken.tokenType)} {}

   TokenType getType() const {
      return type;
   }

   friend std::ostream& operator<<(std::ostream& os, const Token& token);

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

   private:
   TokenType type;
   union {
      std::string_view ident;
      double fvalue;
      unsigned long long ullValue;
      unsigned long ulValue;
      unsigned uValue;
      long long llValue;
      long lValue;
      int iValue;
   };
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Token& token);
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TOKEN_H