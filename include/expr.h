#ifndef QCP_EXPR_H
#define QCP_EXPR_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "operator.h"
#include "scope.h"
#include "token.h"
#include "type.h"
#include "scopeinfo.h"
// ---------------------------------------------------------------------------
#include <ostream>
// ---------------------------------------------------------------------------
// some expression tracking necessary for types and operator precedence
namespace qcp {
// ---------------------------------------------------------------------------
template <typename T>
class Expr {
   template <typename U>
   friend void dump(std::ostream& os, const Expr<U>& expr, int indent);

   public:
   using trait = emitter::emitter_traits<T>;
   using Type = typename trait::Type;
   using ssa_t = typename trait::ssa_t;
   using value_t = typename trait::value_t;
   using ScopeInfo = scope::ScopeInfo<T>;

   using Token = token::Token;
   using OpSpec = op::OpSpec;
   using OpKind = op::Kind;

   using Scope = scope::Scope<Ident, ScopeInfo>;

   using expr_t = std::unique_ptr<Expr>;

   Expr(SrcLoc loc, Type ty, value_t value, Ident id = Ident(), bool isConstExpr = false) : op{},
                                                                                            opspec{0, 0},
                                                                                            // todo: may be redundant
                                                                                            isConstExpr{isConstExpr || !std::holds_alternative<ssa_t*>(value)},
                                                                                            ty{ty},
                                                                                            loc{loc},
                                                                                            mayBeLval{!!id && std::holds_alternative<ssa_t*>(value)},
                                                                                            ident{id},
                                                                                            value{value} {}

   Expr(SrcLoc opLoc, OpKind op, Type ty, expr_t&& lhs, value_t value, bool mayBeLVal = false) : op{op},
                                                                                                 opspec{getOpSpec(op)},
                                                                                                 isConstExpr{!std::holds_alternative<ssa_t*>(value)},
                                                                                                 ty{ty},
                                                                                                 loc{lhs->loc | opLoc},
                                                                                                 mayBeLval{mayBeLVal},
                                                                                                 lhs_{std::move(lhs)},
                                                                                                 rhs_{},
                                                                                                 value{value} {}

   bool isIntConstExpr() const {
      return isConstExpr && ty.isIntegerTy();
   }

   Expr(OpKind op, const Type& ty, expr_t&& lhs, expr_t&& rhs, value_t value, bool mayBeLval = false, Ident name = Ident()) : op{op},
                                                                                                                              opspec{getOpSpec(op)},
                                                                                                                              ty{ty},
                                                                                                                              loc{lhs->loc | rhs->loc},
                                                                                                                              mayBeLval{mayBeLval},
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

   private:
   OpKind op;
   OpSpec opspec;
   bool isConstExpr = false;

   public:
   Type ty;
   const SrcLoc loc;
   bool mayBeLval = false;

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
