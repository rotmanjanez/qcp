#ifndef QCP_OPERATOR_H
#define QCP_OPERATOR_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "token.h"
// ---------------------------------------------------------------------------
#include <cassert>
// Alexis hates iostream
#include <iostream>
// ---------------------------------------------------------------------------
namespace qcp {
namespace op {
// ---------------------------------------------------------------------------
// Operators sorted by precedence
// clang-format off
enum class Kind {
   POSTINC, POSTDEC, CALL, SUBSCRIPT, MEMBER, MEMBER_DEREF, COMPOUND_LITERAL,
   PREINC, PREDEC, UNARY_PLUS, UNARY_MINUS, L_NOT, BW_NOT, CAST, DEREF, ADDROF, SIZEOF, ALIGNOF,
   MUL, DIV, REM,
   ADD, SUB,
   SHL, SHR,
   LT, LE, GT, GE,
   EQ, NE,
   BW_AND,
   BW_XOR,
   BW_OR,
   L_AND,
   L_OR,

   COND,
   ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, REM_ASSIGN, SHL_ASSIGN, SHR_ASSIGN, BW_AND_ASSIGN, BW_XOR_ASSIGN, BW_OR_ASSIGN,
   COMMA,
};
// clang-format on
// ---------------------------------------------------------------------------
struct OpSpec {
   OpSpec(int8_t precedence, bool leftAssociative) : precedence{precedence}, leftAssociative{leftAssociative} {}

   int8_t precedence;
   bool leftAssociative;
};
// ---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const Kind& kind);
// ---------------------------------------------------------------------------
Kind getBinOpKind(token::Kind tk);
// ---------------------------------------------------------------------------
Kind getBinOpKind(const token::Token& token);
// ---------------------------------------------------------------------------
OpSpec getOpSpec(const Kind kind);
// ---------------------------------------------------------------------------
OpSpec getOpSpec(const token::Token& token);
// ---------------------------------------------------------------------------
} // namespace op
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_OPERATOR_H