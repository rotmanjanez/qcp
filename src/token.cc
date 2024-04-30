// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "token.h"
// ---------------------------------------------------------------------------
#include <iomanip>
// ---------------------------------------------------------------------------
namespace qcp {
namespace token {
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Kind& tt) {
   // clang-format off
   static const char* wordmap[] = {
      // tokens in operator precedence order (as complete as possible) starting from the arethmetic operators (precedence 3)
      "ASTERISK", "DIV", "PERCENT",
      "PLUS", "MINUS",
      "SHL", "SHR",
      "L_ANGLE", "LE", "R_ANGLE", "GE",
      "EQ", "NE",
      "BW_AND",
      "BW_XOR",
      "BW_OR",
      "L_AND",
      "L_OR",
      "__UNUSED__", // TERNARY CONDITIONAL MISSING
      "ASSIGN", "ADD_ASSIGN", "SUB_ASSIGN", "MUL_ASSIGN", "DIV_ASSIGN", "REM_ASSIGN", "SHL_ASSIGN", "SHR_ASSIGN", "BW_AND_ASSIGN", "BW_XOR_ASSIGN", "BW_OR_ASSIGN",
      "COMMA",
      // end of operator precedence order

      "UNKNOWN",

      "IDENT",

      "ICONST", "U_ICONST", "L_ICONST", "UL_ICONST", "LL_ICONST", "ULL_ICONST",
      "FCONST", "DCONST", "LDCONST",
      "WB_ICONST", "UWB_ICONST",
      "SLITERAL", "CLITERAL",

      // punctuators
      "L_BRACKET",
      "R_BRACKET",
      "L_BRACE",
      "R_BRACE",
      "L_C_BRKT",
      "R_C_BRKT",
      "PERIOD",
      "DEREF",

      "INC",
      "DEC",

      "BW_INV",
      "NEG",

      "QMARK",
      "COLON",
      "D_COLON",
      "SEMICOLON",
      "ELLIPSIS",
       
      "AUTO", "CONSTEXPR", "EXTERN", "REGISTER", "STATIC", "THREAD_LOCAL",

      "INLINE", "NORETURN",

      "SWITCH", "BREAK", "CASE", "CONTINUE", "DEFAULT", "WHILE", "DO", "ELSE", "FOR", "GOTO", "IF", "RETURN",
      "FALSE", "TRUE", "NULLPTR",

      "STATIC_ASSERT", "SIZEOF", "ALIGNOF",
      "GENERIC", "IMAGINARY",

      "VOID", "CHAR", "SHORT", "INT", "LONG", "FLOAT", "DOUBLE", "SIGNED", "UNSIGNED",
      "BITINT", "BOOL", "COMPLEX",
      "DECIMAL32", "DECIMAL64", "DECIMAL128",

      "STRUCT", "UNION", "ENUM",
      "TYPEDEF", "TYPEOF", "TYPEOF_UNQUAL",

      "CONST", "RESTRICT", "VOLATILE", "ATOMIC",
      "ALIGNAS",

      "END",
   };
   // clang-format on
   os << wordmap[static_cast<int>(tt)];
   return os;
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Token& token) {
   switch (token.type) {
      case Kind::FCONST:
         os << "FCONST(" << token.fvalue << ")";
         break;
      case Kind::DCONST:
         os << "DCONST(" << token.dvalue << ")";
         break;
      case Kind::LDCONST:
         os << "LDCONST(" << token.ldvalue << ")";
         break;
      case Kind::ICONST:
         os << "ICONST(" << token.iValue << ")";
         break;
      case Kind::U_ICONST:
         os << "U_ICONST(" << token.uValue << ")";
         break;
      case Kind::L_ICONST:
         os << "L_ICONST(" << token.lValue << ")";
         break;
      case Kind::UL_ICONST:
         os << "UL_ICONST(" << token.ulValue << ")";
         break;
      case Kind::LL_ICONST:
         os << "LL_ICONST(" << token.llValue << ")";
         break;
      case Kind::ULL_ICONST:
         os << "ULL_ICONST(" << token.ullValue << ")";
         break;
      case Kind::IDENT:
         os << "IDENT(" << token.ident << ")";
         break;
      case Kind::SLITERAL:
         os << "SLITERAL(" << std::quoted(token.ident) << ")";
         break;
      case Kind::CLITERAL:
         os << "CLITERAL(" << std::quoted(token.ident) << ")";
         break;
      // Handle other Kind values here
      default:
         os << token.getType();
         break;
   }
   return os;
}
// ---------------------------------------------------------------------------
} // namespace token
} // namespace qcp
// ---------------------------------------------------------------------------