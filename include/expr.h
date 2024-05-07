#ifndef QCP_EXPR_H
#define QCP_EXPR_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "operator.h"
#include "token.h"
// ---------------------------------------------------------------------------
#include <ostream>
// ---------------------------------------------------------------------------
// some expression tracking necessary for types and operator precedence
namespace qcp {
// ---------------------------------------------------------------------------
template <typename _TY>
class Expr {
   template <typename T>
   friend void dump(std::ostream& os, const Expr<T>& expr, int indent);

   public:
   using TY = _TY;
   using Token = token::Token;
   using ConstExprValue = token::ConstExprValue;
   using OpSpec = op::OpSpec;
   using OpKind = op::Kind;

   explicit Expr(OpKind op) : op{op}, opspec{getOpSpec(token)}, lhs{}, rhs{} {}

   Expr(OpKind op, const TY& ty) : op{op}, opspec{getOpSpec(op)}, ty{ty}, lhs{}, rhs{} {}
   Expr(OpKind op, const TY& ty, std::unique_ptr<Expr>&& lhs) : op{op}, opspec{getOpSpec(op)}, ty{ty}, lhs{std::move(lhs)}, rhs{} {}

   Expr(OpKind op, std::unique_ptr<Expr>&& lhs) : op{op}, opspec{getOpSpec(op)}, ty{lhs->ty}, lhs{std::move(lhs)}, rhs{} {}
   Expr(OpKind op, std::unique_ptr<Expr>&& lhs, std::unique_ptr<Expr> rhs) : op{op}, opspec{getOpSpec(op)}, ty{TY::commonRealType(op, lhs->ty, rhs->ty)}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}

   Expr(OpKind op, int8_t precedence, bool lassoc, std::unique_ptr<Expr>&& lhs) : op{op}, opspec{precedence, lassoc}, ty{lhs->ty}, lhs{std::move(lhs)}, rhs{} {}
   Expr(OpKind op, int8_t precedence, bool lassoc, std::unique_ptr<Expr>&& lhs, std::unique_ptr<Expr>&& rhs) : op{op}, opspec{precedence, lassoc}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}

   // todo: (jr) tokens can also be identifier and not only constants
   explicit Expr(const Token& token, bool isConstExpr = false) : opspec{0, false}, ty{TY::fromToken(token)}, token{token}, lhs{}, rhs{} {}

   Expr(const Token& token, std::unique_ptr<Expr>&& lhs, std::unique_ptr<Expr>&& rhs) : opspec{0, false}, ty{}, token{token}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}

   void setPrec(int8_t prec) {
      opspec.precedence = prec;
   }

   private:
   OpKind op;
   OpSpec opspec;
   TY ty;
   bool isConstExpr;

   union {
      Token token;
      ConstExprValue value{0};
   };

   std::unique_ptr<Expr> lhs;
   std::unique_ptr<Expr> rhs;
};
// ---------------------------------------------------------------------------
template <typename T>
void dump(std::ostream& os, const Expr<T>& expr, int indent = 0) {
   os << std::string(indent, ' ');
   if (expr.ty) {
      os << '(' << expr.ty << ") ";
   }

   if (expr.token) {
      os << expr.token << std::endl;
   } else {
      os << expr.op << std::endl;
   }

   if (expr.lhs) {
      dump(os, *expr.lhs, indent + 2);
   }
   if (expr.rhs) {
      dump(os, *expr.rhs, indent + 2);
   }
}
// ---------------------------------------------------------------------------
template <typename T>
std::ostream& operator<<(std::ostream& os, const Expr<T>& expr) {
   dump(os, expr);
   return os;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_EXPR_H
