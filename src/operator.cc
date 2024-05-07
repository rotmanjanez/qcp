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
std::ostream& operator<<(std::ostream& os, const Kind& kind) {
   static const char* names[] = {
       // clang-format off
      "POSTINC", "POSTDEC", "CALL", "SUBSCRIPT", "MEMBER", "MEMBER_DEREF", "COMPOUND_LITERAL",
      "PREINC", "PREDEC", "UNARY_PLUS", "UNARY_MINUS", "L_NOT", "BW_NOT", "CAST", "DEREF", "ADDROF", "SIZEOF", "ALIGNOF",
      "MUL", "DIV", "REM",
      "ADD", "SUB",
      "SHL", "SHR",
      "LT", "LE", "GT", "GE",
      "EQ", "NE",
      "BW_AND",
      "BW_XOR",
      "BW_OR",
      "L_AND",
      "L_OR",
      "COND",
      "ASSIGN", "ADD_ASSIGN", "SUB_ASSIGN", "MUL_ASSIGN", "DIV_ASSIGN", "REM_ASSIGN", "SHL_ASSIGN", "SHR_ASSIGN", "BW_AND_ASSIGN", "BW_XOR_ASSIGN", "BW_OR_ASSIGN",
      "COMMA",
       // clang-format on
   };
   return os << names[static_cast<int>(kind)];
}
// ---------------------------------------------------------------------------
} // namespace op
} // namespace qcp
// ---------------------------------------------------------------------------
