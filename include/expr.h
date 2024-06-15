#ifndef QCP_EXPR_H
#define QCP_EXPR_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "operator.h"
#include "scope.h"
#include "token.h"
#include "type.h"
// ---------------------------------------------------------------------------
#include <ostream>
// ---------------------------------------------------------------------------
// some expression tracking necessary for types and operator precedence
namespace qcp {
// ---------------------------------------------------------------------------
template <typename _EmitterT>
class Expr {
   template <typename T>
   friend void dump(std::ostream& os, const Expr<T>& expr, int indent);

   public:
   using TY = type::Type<_EmitterT>;
   using Token = token::Token;
   using ConstExprValue = token::ConstExprValue;
   using OpSpec = op::OpSpec;
   using OpKind = op::Kind;

   using ssa_t = typename _EmitterT::ssa_t;
   using const_t = typename _EmitterT::const_t;
   using bb_t = typename _EmitterT::bb_t;
   using fn_t = typename _EmitterT::fn_t;
   using value_t = typename _EmitterT::value_t;

   using ScopeInfo = ScopeInfo<_EmitterT>;
   using Scope = Scope<Ident, ScopeInfo>;

   using expr_t = std::unique_ptr<Expr>;

   Expr(SrcLoc loc, TY ty, value_t value, Ident id = Ident()) : op{},
                                                                opspec{0, 0},
                                                                // todo: may be redundant
                                                                isConstExpr{std::holds_alternative<const_t*>(value)},
                                                                mayBeLval_{!!id},
                                                                ty{ty},
                                                                loc{loc},
                                                                ident{id},
                                                                value{value} {}

   Expr(OpKind op, TY ty, expr_t&& lhs, value_t value) : op{op},
                                                         opspec{getOpSpec(op)},
                                                         isConstExpr{std::holds_alternative<const_t*>(value)},
                                                         ty{ty},
                                                         loc{lhs->loc},
                                                         lhs_{std::move(lhs)},
                                                         rhs_{},
                                                         value{value} {}

   bool isIntConstExpr() const {
      return isConstExpr && ty.isIntegerTy();
   }

   Expr(OpKind op, const TY& ty, expr_t&& lhs, expr_t&& rhs, value_t value, bool mayBeLval = false, Ident name = Ident()) : op{op},
                                                                                                                            opspec{getOpSpec(op)},
                                                                                                                            mayBeLval_{mayBeLval},
                                                                                                                            ty{ty},
                                                                                                                            loc{lhs->loc | rhs->loc},
                                                                                                                            ident{name},
                                                                                                                            lhs_{std::move(lhs)},
                                                                                                                            rhs_{std::move(rhs)},
                                                                                                                            value{value} {
      if (lhs_->isConstExpr && rhs_->isConstExpr && !((op >= OpKind::ASSIGN && op <= OpKind::COMMA))) {
         isConstExpr = true;
      }
   }

   void setPrec(int8_t prec) {
      opspec.precedence = prec;
   }

   bool mayBeLval() const {
      return mayBeLval_;
   }

   private:
   OpKind op;
   OpSpec opspec;
   bool isConstExpr = false;
   bool mayBeLval_ = false;

   public:
   TY ty;
   const SrcLoc loc;

   // todo: remove this?
   Ident ident;

   private:
   expr_t lhs_;
   expr_t rhs_;

   public:
   value_t value;
};
// ---------------------------------------------------------------------------
template <typename T>
void dump(std::ostream& os, const Expr<T>& expr, int indent = 0) {
   os << std::string(indent, ' ');
   if (expr.ty) {
      os << '(' << expr.ty << ") ";
   }

   if (expr.ident) {
      os << expr.ident << std::endl;
   } else {
      os << expr.op << std::endl;
   }

   if (expr.lhs_) {
      dump(os, *expr.lhs_, indent + 2);
   }
   if (expr.rhs_) {
      dump(os, *expr.rhs_, indent + 2);
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
