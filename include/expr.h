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
   using bb_t = typename _EmitterT::bb_t;
   using fn_t = typename _EmitterT::fn_t;

   using ScopeInfo = ScopeInfo<_EmitterT>;
   using Scope = Scope<Ident, ScopeInfo>;

   Expr(SrcLoc loc, TY& ty, Ident i, ssa_t* ssa) : op{},
                                                   opspec{0, 0},
                                                   mayBeLval_{true},
                                                   ty{ty},
                                                   loc{loc},
                                                   ident{i},
                                                   ssa{ssa} {
   }

   Expr(SrcLoc loc, TY& ty, ssa_t* ssa, bool isConstExpr) : op{},
                                                            opspec{0, 0},
                                                            isConstExpr{isConstExpr},
                                                            ty{ty},
                                                            loc{loc},
                                                            ssa{ssa} {}

   Expr(OpKind op, const TY& ty, std::unique_ptr<Expr>&& lhs, ssa_t* ssa, Ident name = Ident()) : op{op},
                                                                                                  opspec{getOpSpec(op)},
                                                                                                  ty{ty},
                                                                                                  loc{lhs->loc},
                                                                                                  ident{name},
                                                                                                  lhs_{std::move(lhs)},
                                                                                                  rhs_{},
                                                                                                  ssa{ssa} {
      if (lhs_->isConstExpr) {
         switch (op) {
            case OpKind::PREINC:
            case OpKind::PREDEC:
            case OpKind::POSTINC:
            case OpKind::POSTDEC:
            case OpKind::CALL:
               break;
            default:
               isConstExpr = true;
         }
      }
   }

   Expr(OpKind op, const TY& ty, std::unique_ptr<Expr>&& lhs, std::unique_ptr<Expr> rhs, ssa_t* ssa, Ident name = Ident()) : op{op},
                                                                                                                             opspec{getOpSpec(op)},
                                                                                                                             ty{ty},
                                                                                                                             loc{lhs->loc | rhs->loc},
                                                                                                                             ident{name},
                                                                                                                             lhs_{std::move(lhs)},
                                                                                                                             rhs_{std::move(rhs)},
                                                                                                                             ssa{ssa} {
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
   std::unique_ptr<Expr> lhs_;
   std::unique_ptr<Expr> rhs_;

   public:
   ssa_t* ssa = nullptr;
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
