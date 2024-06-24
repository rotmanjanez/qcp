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
      case Kind::DCONST:
      case Kind::LDCONST:
      case Kind::ICONST:
      case Kind::U_ICONST:
      case Kind::L_ICONST:
      case Kind::UL_ICONST:
      case Kind::LL_ICONST:
      case Kind::ULL_ICONST:
      case Kind::IDENT:
         os << token.getKind() << "(TODO: not implemented)"; // << token.value << ")";
         break;
      case Kind::SLITERAL:
      case Kind::CLITERAL:
         os << token.getKind() << "(TODO: not implemented)"; // << token.value << ")";
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