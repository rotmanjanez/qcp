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
         os << token.getValue<float>();
         break;
      case Kind::DCONST:
         os << token.getValue<double>();
         break;
      case Kind::ICONST:
         os << token.getValue<int>();
         break;
      case Kind::LDCONST:
      case Kind::U_ICONST:
      case Kind::L_ICONST:
      case Kind::UL_ICONST:
      case Kind::LL_ICONST:
         assert(false);
      case Kind::ULL_ICONST:
         os << token.getValue<unsigned long long>();
         break;
      case Kind::IDENT:
         os << token.getValue<Ident>();
         break;
      case Kind::SLITERAL:
         os << std::quoted(token.getString());
         break;
      case Kind::CLITERAL:
         os << std::quoted(token.getString(), '\'');
         break;
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