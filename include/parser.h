#ifndef QCP_PARSER_H
#define QCP_PARSER_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "diagnostics.h"
#include "token.h"
#include "tokenizer.h"
#include "type.h"
// ---------------------------------------------------------------------------
#include <string_view>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker> // typename _EmmiterT
class Parser {
   public:
   // using ssa = typename _EmmiterT::SSARefT;
   using TK = token::Kind;
   using Token = token::Token;
   using tokenizer_t = Tokenizer<_DiagnosticTracker>;

   struct Expr {
      using TY = Type<std::string>;

      explicit Expr(op::Kind op) : op{op}, opspec{getOpSpec(token)}, lhs{}, rhs{} {}
      Expr(op::Kind op, const TY& ty) : op{op}, opspec{getOpSpec(op)}, ty{ty}, lhs{}, rhs{} {}
      Expr(op::Kind op, std::unique_ptr<Expr>&& lhs, std::unique_ptr<Expr> rhs) : op{op}, opspec{getOpSpec(op)}, ty{TY::commonRealType(op, lhs->ty, rhs->ty)}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
      Expr(op::Kind op, int8_t precedence, bool lassoc, std::unique_ptr<Expr>&& lhs, std::unique_ptr<Expr>&& rhs) : op{op}, opspec{precedence, lassoc}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
      Expr(op::Kind op, std::unique_ptr<Expr>&& lhs) : op{op}, opspec{getOpSpec(op)}, ty{lhs->ty}, lhs{std::move(lhs)}, rhs{} {}
      Expr(op::Kind op, int8_t precedence, bool lassoc, std::unique_ptr<Expr>&& lhs) : op{op}, opspec{precedence, lassoc}, ty{lhs->ty}, lhs{std::move(lhs)}, rhs{} {}

      // todo: (jr) tokens can also be identifier and not only constants
      explicit Expr(const Token& token) : token{token}, opspec{0, false}, ty{TY::fromToken(token)}, lhs{}, rhs{} {}

      op::Kind op;
      Token token;
      op::OpSpec opspec;
      TY ty;
      std::unique_ptr<Expr> lhs;
      std::unique_ptr<Expr> rhs;

      void dump(std::ostream& os, int indent = 0) {
         os << std::string(indent, ' ')
            << '(' << ty << ") ";

         if (!lhs && !rhs) {
            os << token << '\n';
         } else {
            os << op << '\n';
         }

         if (lhs) {
            lhs->dump(os, indent + 2);
         }
         if (rhs) {
            rhs->dump(os, indent + 2);
         }
      }
   };
   using ssa = std::unique_ptr<Expr>;
   // , _EmmiterT& emitter emitter_{emitter},
   Parser(std::string_view prog, _DiagnosticTracker& diagnostics) : tokenizer_{prog, diagnostics}, pos_{tokenizer_.begin()}, diagnostics_{diagnostics} {}

   void parse() {
      // emitter_.emitPrologue();
      // while (pos_ != tokenizer_.end()) {
      //    parseExpr();
      // }
      // emitter_.emitEpilogue();

      std::cout << std::string(80, '-') << '\n';
      auto expr{parseExpr()};
      std::cout << std::string(80, '-') << '\n';
      expr->dump(std::cout);
      std::cout << std::string(80, '-') << '\n';
      std::cout << "types:\n";
      for (auto ty : BaseTypeFactory<std::string>::getTypes()) {
         std::cout << ty << '\n';
      }
   }

   private:
   ssa parseExpr(short precedence = 15, short indent = 0) {
      ssa lhs = parsePrimaryExpr(indent);
      Token cur{*pos_};
      while (cur.getType() != TK::END) {
         Token cur{*pos_};
         op::OpSpec spec{op::getOpSpec(cur)};

         std::cout << std::string(indent, ' ') << "cur: " << cur << " p:" << static_cast<int>(spec.precedence) << '\n';

         if (spec.precedence > precedence) {
            break;
         }
         switch (cur.getType()) {
            case TK::END:
            case TK::R_BRACE:
            case TK::R_BRACKET:
               return lhs;
            case TK::INC:
            case TK::DEC:
               lhs = std::make_unique<Expr>(cur.getType() == TK::INC ? op::Kind::POSTINC : op::Kind::POSTDEC, std::move(lhs));
               ++pos_;
               continue;
            case TK::L_BRACKET:
               ++pos_;
               lhs = std::make_unique<Expr>(op::Kind::SUBSCRIPT, 1, true, std::move(lhs), parseExpr(15, indent + 4));
               if (pos_->getType() != TK::R_BRACKET) {
                  diagnostics_.report("expected ']'");
               }
               if (lhs->lhs->opspec.precedence > 1) {
                  auto tmp = std::move(lhs->lhs);
                  lhs->lhs = std::move(tmp->lhs);
                  tmp->lhs = std::move(lhs);
                  lhs = std::move(tmp);
               }
               ++pos_;
               continue;
            case TK::DEREF:
            case TK::PERIOD:
               ++pos_;
               assert(pos_->getType() == TK::IDENT && "member access"); // todo: (jr) not implemented
               lhs = std::make_unique<Expr>(cur.getType() == TK::DEREF ? op::Kind::MEMBER_DEREF : op::Kind::MEMBER, std::move(lhs), std::make_unique<Expr>(*pos_));
               ++pos_;
               continue;
            case TK::L_BRACE:
               assert(false && "function call and compound literal"); // todo: (jr) not implemented
            default:
               // binary operator
               // store current token and parse rhs
               cur = *pos_;
               ++pos_;
         }

         if (cur.getType() == TK::END) {
            return lhs;
         }

         ssa rhs = parseExpr(spec.precedence - spec.leftAssociative, indent + 4);

         if (spec.precedence < lhs->opspec.precedence) {
            op::Kind op = lhs->op;

            auto tmp = std::move(lhs->rhs);
            lhs->rhs = std::make_unique<Expr>(op, std::move(tmp), std::move(rhs));

            lhs = std::move(lhs);
         } else {
            lhs = std::make_unique<Expr>(op::getBinOpKind(cur), std::move(lhs), std::move(rhs));
         }
         cur = *pos_;
      }
      return lhs;
   }

   ssa parsePrimaryExpr(short indent = 0) {
      Token t = *pos_;
      ++pos_;
      ssa expr;
      op::Kind kind;
      std::cout << std::string(indent, ' ') << "primary: " << t << '\n';
      switch (t.getType()) {
         case TK::ICONST:
         case TK::U_ICONST:
         case TK::L_ICONST:
         case TK::UL_ICONST:
         case TK::LL_ICONST:
         case TK::ULL_ICONST:
         case TK::FCONST:
         case TK::DCONST:
         case TK::LDCONST:
            expr = std::make_unique<Expr>(t);
            break;
         case TK::IDENT:
         case TK::SLITERAL:
         case TK::WB_ICONST:
         case TK::UWB_ICONST:
            // todo: (jr) is this also a string? case TK::CLITERAL:
            std::cout << std::string(indent, ' ') << "primary: " << t << '\n';
            expr = std::make_unique<Expr>(t);
            break;

         // todo: (jr) type cast
         case TK::L_BRACE:
            std::cout << std::string(indent, ' ') << "-- new subexpr\n";
            expr = parseExpr(indent + 4);
            expr->opspec.precedence = 0;
            std::cout << std::string(indent, ' ') << "-- end subexpr\n";
            if (pos_->getType() != TK::R_BRACE) {
               diagnostics_.report("expected ')'");
            }
            ++pos_;
            break;

         // todo: (jr) generic selection
         default:
            // unary prefix operator
            switch (t.getType()) {
               case TK::INC:
                  kind = op::Kind::PREINC;
                  break;
               case TK::DEC:
                  kind = op::Kind::PREDEC;
                  break;
               case TK::PLUS:
                  kind = op::Kind::UNARY_PLUS;
                  break;
               case TK::MINUS:
                  kind = op::Kind::UNARY_MINUS;
                  break;
               case TK::BW_INV:
                  kind = op::Kind::BW_NOT;
                  break;
               case TK::NEG:
                  kind = op::Kind::L_NOT;
                  break;
               case TK::ASTERISK:
                  kind = op::Kind::DEREF;
                  break;
               case TK::L_AND:
                  kind = op::Kind::ADDROF;
                  break;
               case TK::SIZEOF:
               case TK::ALIGNOF:
                  assert(false && "Not implemented"); // todo: (jr) not implemented
               default:
                  std::cerr << "unexpected token: " << t << '\n';
                  assert(false && "unexpected token"); // todo: (jr) unexpected token
                  diagnostics_.report("expected primary expression");
                  return expr;
            }
            expr = std::make_unique<Expr>(kind, 2, false, parsePrimaryExpr(indent + 4));
      }
      return expr;
   }

   tokenizer_t tokenizer_;
   typename tokenizer_t::const_iterator pos_;
   // _EmmiterT& emitter_;
   _DiagnosticTracker diagnostics_{};
};
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_PARSER_H
