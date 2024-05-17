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

   using Scope = Scope<Ident, std::pair<TY, ssa_t*>>;

   Expr(bb_t* bb, Ident i) : op{},
                             opspec{0, 0},
                             mayBeLval{true},
                             ident{i} {
      auto& [iTy, iSSA] = *(scope().find(i));
      ty = iTy;
      ssa_ = iSSA;
   }

   ~Expr() {
      std::cout << *this << std::endl;
   }

   Expr(bb_t* bb, OpKind op, const TY& ty, std::unique_ptr<Expr>&& lhs) : op{op},
                                                                          opspec{getOpSpec(op)},
                                                                          ty{ty},
                                                                          lhs_{std::move(lhs)},
                                                                          rhs_{} {}

   Expr(bb_t* bb, OpKind op, std::unique_ptr<Expr>&& lhs) : op{op},
                                                            opspec{getOpSpec(op)},
                                                            ty{lhs->ty},
                                                            lhs_{std::move(lhs)},
                                                            rhs_{},
                                                            ssa_{emitter().emitUnOp(bb, Ident(), op, lhs_->ssa(bb))} {}

   Expr(bb_t* bb, OpKind op, std::unique_ptr<Expr>&& lhs, std::unique_ptr<Expr> rhs) : op{op},
                                                                                       opspec{getOpSpec(op)},
                                                                                       ty{TY::commonRealType(op, lhs->ty, rhs->ty)},
                                                                                       lhs_{std::move(lhs)},
                                                                                       rhs_{std::move(rhs)},
                                                                                       ssa_{emitter().emitBinOp(bb, Ident(), op, lhs_->ssa(bb), rhs_->ssa(bb))} {}

   // todo: (jr) tokens can also be identifier and not only constants
   Expr(bb_t* bb, const Token& token, bool isConstExpr = false) : opspec{0, false},
                                                                  ty{TY::fromToken(token)},
                                                                  lhs_{},
                                                                  rhs_{},
                                                                  ssa_{emitter().emitConst(bb, ty.emitterType(), Ident(), token.getValue<int>())} {}

   void setPrec(int8_t prec) {
      opspec.precedence = prec;
   }

   ssa_t* ssa(bb_t* bb) {
      if (mayBeLval) {
         return emitter().emitLoad(bb, ident, ty.emitterType(), ssa_);
      }
      return ssa_;
   }

   static void setEmitter(_EmitterT& emitter) {
      emitter_ = &emitter;
   }

   static void setScope(Scope& scope) {
      scope_ = &scope;
   }

   private:
   static _EmitterT* emitter_;
   static Scope* scope_;

   OpKind op;
   OpSpec opspec;
   bool isConstExpr = false;
   bool mayBeLval = false;

   TY ty;

   // todo: remove this
   Ident ident;

   std::unique_ptr<Expr> lhs_;
   std::unique_ptr<Expr> rhs_;

   ssa_t* ssa_ = nullptr;

   _EmitterT& emitter() {
      assert(emitter_ && "Emitter not set");
      return *emitter_;
   }

   Scope& scope() {
      assert(scope_ && "Scope not set");
      return *scope_;
   }
};
// ---------------------------------------------------------------------------
template <typename _EmitterT>
_EmitterT* Expr<_EmitterT>::emitter_ = nullptr;
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Expr<_EmitterT>::Scope* Expr<_EmitterT>::scope_ = nullptr;
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
