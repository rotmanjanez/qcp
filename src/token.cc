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
   static const char* wordmap[] = {
#define ENUM_AS_STRING
#include "defs/tokens.def"
#undef ENUM_AS_STRING
   };
   os << wordmap[static_cast<int>(tt)];
   return os;
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Token& token) {
   switch (token.type) {
      case Kind::FCONST:
         os << "FCONST(" << token.value.f << ")";
         break;
      case Kind::DCONST:
         os << "DCONST(" << token.value.d << ")";
         break;
      case Kind::LDCONST:
         os << "LDCONST(" << token.value.ld << ")";
         break;
      case Kind::ICONST:
         os << "ICONST(" << token.value.i << ")";
         break;
      case Kind::U_ICONST:
         os << "U_ICONST(" << token.value.u << ")";
         break;
      case Kind::L_ICONST:
         os << "L_ICONST(" << token.value.l << ")";
         break;
      case Kind::UL_ICONST:
         os << "UL_ICONST(" << token.value.ul << ")";
         break;
      case Kind::LL_ICONST:
         os << "LL_ICONST(" << token.value.ll << ")";
         break;
      case Kind::ULL_ICONST:
         os << "ULL_ICONST(" << token.value.ull << ")";
         break;
      case Kind::IDENT:
         os << "IDENT(" << token.ident << ")";
         break;
      case Kind::SLITERAL:
         os << "SLITERAL(" << std::quoted(static_cast<std::string_view>(token.ident)) << ")";
         break;
      case Kind::CLITERAL:
         os << "CLITERAL(" << std::quoted(static_cast<std::string_view>(token.ident)) << ")";
         break;
      // Handle other Kind values here
      default:
         os << token.getKind();
         break;
   }
   return os;
}
// ---------------------------------------------------------------------------
} // namespace token
} // namespace qcp
// ---------------------------------------------------------------------------