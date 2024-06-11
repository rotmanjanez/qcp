// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "operator.h"
// ---------------------------------------------------------------------------
namespace qcp {
namespace op {
// ---------------------------------------------------------------------------
Kind getBinOpKind(token::Kind tk) {
   assert(tk >= token::Kind::ASTERISK && tk <= token::Kind::COMMA && "Invalid token::Kind to get Kind from");
   assert(tk != token::Kind::QMARK && "Ternary operator not supported"); // todo: (jr) implement ternary operator
   return static_cast<Kind>(static_cast<int>(Kind::MUL) + static_cast<int>(tk) - static_cast<int>(token::Kind::ASTERISK));
}
// ---------------------------------------------------------------------------
Kind getBinOpKind(const token::Token& token) {
   return getBinOpKind(token.getKind());
}
// ---------------------------------------------------------------------------
OpSpec getOpSpec(const token::Token& token) {
   token::Kind tk = token.getKind();
   if (tk < token::Kind::ASTERISK || tk > token::Kind::COMMA) {
      return OpSpec{0, false};
   }
   op::Kind kind = getBinOpKind(token);
   return getOpSpec(kind);
}
// ---------------------------------------------------------------------------
OpSpec getOpSpec(const Kind kind) {
   // Operator precedence and associativity
   static const OpSpec opspecs[] = {
       // clang-format off
       {1, true},  {1, true},  {1, true},  {1, true},  {1, true},  {1, true},  {1, true},
       {2, false}, {2, false}, {2, false}, {2, false}, {2, false}, {2, false}, {2, false}, {2, false}, {2, false}, {2, false}, {2, false},
       {3, true},  {3, true},  {3, true},
       {4, true},  {4, true},
       {5, true},  {5, true},
       {6, true},  {6, true},  {6, true},  {6, true},
       {7, true},  {7, true},
       {8, true},
       {9, true},
       {10, true},
       {11, true},
       {12, true},
       {13, false},
       {14, false}, {14, false}, {14, false}, {14, false}, {14, false}, {14, false}, {14, false}, {14, false}, {14, false}, {14, false}, {14, false},
       {15, true},
       // clang-format on
   };
   return opspecs[static_cast<int>(kind)];
}
// ---------------------------------------------------------------------------
bool isAssignmentOp(const Kind kind) {
   return kind >= Kind::ASSIGN && kind <= Kind::BW_OR_ASSIGN;
}
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Kind& kind) {
   static const char* names[] = {
#define ENUM_AS_STRING
#include "defs/operators.def"
#undef ENUM_AS_STRING
   };
   return os << names[static_cast<int>(kind)];
}
// ---------------------------------------------------------------------------
} // namespace op
} // namespace qcp
// ---------------------------------------------------------------------------
