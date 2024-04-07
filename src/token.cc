#include "token.h"
// ---------------------------------------------------------------------------
#include <iomanip>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const TokenType& tt) {
   static const char* wordmap[] = {
       "UNKNOWN",
       "IDENT",
       "ICONST",
       "U_ICONST",
       "L_ICONST",
       "UL_ICONST",
       "LL_ICONST",
       "ULL_ICONST",
       "WB_ICONST",
       "UWB_ICONST",
       "FCONST",
       "LITERAL",
       "L_SQ_BRKT",
       "R_SQ_BRKT",
       "L_BRKT",
       "R_BRKT",
       "L_C_BRKT",
       "R_C_BRKT",
       "PERIOD",
       "DEREF",
       "INC",
       "DEC",
       "BW_AND",
       "MUL",
       "PLUS",
       "MINUS",
       "BW_INV",
       "NEG",
       "DIV",
       "MOD",
       "SHL",
       "SHR",
       "L_P_BRKT",
       "R_P_BRKT",
       "LE",
       "GE",
       "EQ",
       "NE",
       "BW_XOR",
       "BW_OR",
       "L_AND",
       "L_OR",
       "QMARK",
       "COLON",
       "D_COLON",
       "SEMICOLON",
       "ELLIPSIS",
       "ASSIGN",
       "MUL_ASSIGN",
       "DIV_ASSIGN",
       "MOD_ASSIGN",
       "ADD_ASSIGN",
       "SUB_ASSIGN",
       "SHL_ASSIGN",
       "SHR_ASSIGN",
       "BW_AND_ASSIGN",
       "BW_XOR_ASSIGN",
       "BW_OR_ASSIGN",
       "COMMA",
       "ALIGNAS",
       "ALIGNOF",
       "AUTO",
       "BOOL",
       "BREAK",
       "CASE",
       "CHAR",
       "CONST",
       "CONSTEXPR",
       "CONTINUE",
       "DEFAULT",
       "DO",
       "DOUBLE",
       "ELSE",
       "ENUM",
       "EXTERN",
       "FALSE",
       "FLOAT",
       "FOR",
       "GOTO",
       "IF",
       "INLINE",
       "INT",
       "LONG",
       "NULLPTR",
       "REGISTER",
       "RESTRICT",
       "RETURN",
       "SHORT",
       "SIGNED",
       "SIZEOF",
       "STATIC",
       "STATIC_ASSERT",
       "STRUCT",
       "SWITCH",
       "THREAD_LOCAL",
       "TRUE",
       "TYPEDEF",
       "TYPEOF",
       "TYPEOF_UNQUAL",
       "UNION",
       "UNSIGNED",
       "VOID",
       "VOLATILE",
       "WHILE",
       "ATOMIC",
       "BITINT",
       "COMPLEX",
       "DECIMAL128",
       "DECIMAL32",
       "DECIMAL64",
       "GENERIC",
       "IMAGINARY",
       "NORETURN",
       "END",
   };
   os << wordmap[static_cast<int>(tt)];
   return os;
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Token& token) {
   switch (token.type) {
      case TokenType::FCONST:
         os << "FCONST(" << token.fvalue << ")";
         break;
      case TokenType::ICONST:
         os << "ICONST(" << token.iValue << ")";
         break;
      case TokenType::U_ICONST:
         os << "U_ICONST(" << token.uValue << ")";
         break;
      case TokenType::L_ICONST:
         os << "L_ICONST(" << token.lValue << ")";
         break;
      case TokenType::UL_ICONST:
         os << "UL_ICONST(" << token.ulValue << ")";
         break;
      case TokenType::LL_ICONST:
         os << "LL_ICONST(" << token.llValue << ")";
         break;
      case TokenType::ULL_ICONST:
         os << "ULL_ICONST(" << token.ullValue << ")";
         break;
      case TokenType::IDENT:
         os << "IDENT(" << token.ident << ")";
         break;
      case TokenType::LITERAL:
         os << "LITERAL(" << std::quoted(token.ident) << ")";
         break;
      // Handle other TokenType values here
      default:
         os << token.getType();
         break;
   }
   return os;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------