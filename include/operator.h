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
   #include "defs/operators.def"
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
bool isAssignmentOp(const Kind kind);
// ---------------------------------------------------------------------------
bool isBitwiseOp(const Kind kind);
// ---------------------------------------------------------------------------
} // namespace op
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_OPERATOR_H