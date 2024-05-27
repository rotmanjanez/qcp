#ifndef QCP_PARSER_H
#define QCP_PARSER_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "diagnostics.h"
#include "expr.h"
#include "scope.h"
#include "token.h"
#include "tokenizer.h"
#include "tracer.h"
#include "typefactory.h"
// ---------------------------------------------------------------------------
#include <memory>
#include <string_view>
#include <utility>
#include <vector>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
template <typename _EmitterT>
class Parser {
   public:
   using Token = token::Token;
   using TK = token::Kind;

   using TYK = type::Kind;
   using TY = type::Type<_EmitterT>;
   using TaggedTY = type::TaggedType<_EmitterT>;
   using Factory = type::BaseFactory<_EmitterT>;
   using DeclTypeBaseRef = typename Factory::DeclTypeBaseRef;

   using OpSpec = op::OpSpec;

   using attr_t = Token;

   using bb_t = typename _EmitterT::bb_t;
   using phi_t = typename _EmitterT::phi_t;
   using ssa_t = typename _EmitterT::ssa_t;
   using const_t = typename _EmitterT::const_t;
   using ty_t = typename _EmitterT::ty_t;
   using fn_t = typename _EmitterT::fn_t;

   using ScopeInfo = ScopeInfo<_EmitterT>;
   using Scope = Scope<Ident, ScopeInfo>;
   using Expr = Expr<_EmitterT>;
   using expr_t = std::unique_ptr<Expr>;

   Parser(std::string_view prog,
          DiagnosticTracker& diagnostics,
          std::ostream& logStream = std::cout) : tokenizer_{prog, diagnostics},
                                                 pos_{tokenizer_.begin()},
                                                 diagnostics_{diagnostics},
                                                 tracer_{logStream},
                                                 factory_{emitter_} {}

   _EmitterT& getEmitter() {
      return emitter_;
   }

   void parse() {
      std::cout << std::string(80, '-') << '\n';
      tracer_ << ENTER{"parseTranslationUnit"};
      while (*pos_) {
         std::vector<attr_t> attr = parseOptAttributeSpecifierSequence();
         State s{};
         parseDeclStmt(s, attr, true);
      }

      std::cout << std::string(80, '-') << '\n';
      tracer_ << EXIT{"parseTranslationUnit"};
   }

   struct State {
      fn_t* fn = nullptr;
      bb_t* entry = nullptr;
      bb_t* bb = nullptr;
   };

   struct Declarator {
      TY ty;
      DeclTypeBaseRef base;
      Ident ident;
      std::vector<Ident> paramNames{};
   };

   struct DeclarationSpecifier {
      std::array<TK, 2> storageClass{TK::END, TK::END};
      TY ty;
   };

   State parseStmt(State s);
   State parseCompoundStmt(State s);
   State parseUnlabledStmt(State s, const std::vector<attr_t>& attr);
   State parseDeclStmt(State s, const std::vector<attr_t>& attr, bool isTopLevel = false);
   expr_t parseExpr(State s, short midPrec = 0);
   expr_t parseAssignmentExpr(State s) {
      return parseExpr(s, 14);
   }
   expr_t parsePrimaryExpr(State s);
   TY parseTypeName();
   TY parseSpecifierQualifierList();

   DeclarationSpecifier parseDeclarationSpecifierList(bool storageClassSpecifiers = true, bool functionSpecifiers = true);

   void parseFunctionDefinition(Declarator& decl, State s);
   expr_t parseInitializerList(State s);

   private:
   void parseMemberDeclaratorList();
   std::vector<attr_t> parseOptAttributeSpecifierSequence();

   TY parseAbstractDeclarator(TY specifierQualifier);
   // todo: make move
   Declarator parseDeclarator(TY specifierQualifier);
   // todo: make move
   Declarator parseDeclaratorImpl();

   void parseParameterList(std::vector<TY>& paramTys, std::vector<Ident>& paramNames);

   inline ssa_t* asRVal(State& s, expr_t& expr) {
      if (expr->mayBeLval() && expr->ty.kind() != TYK::FN_T) {
         return emitter_.emitLoad(s.bb, expr->ident, expr->ty.emitterType(), expr->ssa);
      }
      return expr->ssa;
   }

   inline expr_t emitUnOp(State s, Ident name, op::Kind kind, expr_t&& operand) {
      // todo: type
      ssa_t* ssa = emitter_.emitUnOp(s.bb, name, kind, asRVal(s, operand));
      return std::make_unique<Expr>(kind, operand->ty, std::move(operand), ssa, name);
   }

   inline expr_t emitUnOp(State& s, op::Kind kind, expr_t&& operand) {
      return emitUnOp(s, Ident(), kind, std::move(operand));
   }

   template <typename... Tokens>
   Token consumeAnyOf(Tokens... tokens) {
      static_assert(sizeof...(Tokens) > 0, "consumeAnyOf() must have at least one argument");
      static_assert((std::is_same_v<TK, Tokens> && ...), "all arguments must be of type TK");

      if (((pos_->getKind() == tokens) || ...)) {
         Token t{*pos_};
         ++pos_;
         return t;
      }
      return Token{};
   }

   bool parseOpt(TK kind) {
      if (pos_->getKind() == kind) {
         ++pos_;
         return true;
      }
      return false;
   }

   Token expect(TK kind) {
      if (pos_->getKind() == kind) {
         Token t{*pos_};
         ++pos_;
         return t;
      }
      diagnostics_ << "Expected '" << kind << "' but got " << pos_->getKind() << std::endl;
      ++pos_;
      return Token{};
   }

   Tokenizer tokenizer_;
   typename Tokenizer::const_iterator pos_;
   _EmitterT emitter_{};

   Scope scope_{};
   DiagnosticTracker& diagnostics_;
   Tracer tracer_;
   Factory factory_;
};
// ---------------------------------------------------------------------------
// TokenCounter
// ---------------------------------------------------------------------------
// an array of counters for each token kind
// useful for determining type specifiers
template <token::Kind FROM, token::Kind TO>
class TokenCounter : public std::array<uint8_t, static_cast<std::size_t>(TO - FROM + 1)> {
   public:
   using TK = token::Kind;

   static constexpr TK from = FROM;
   static constexpr TK to = TO;

   uint8_t& operator[](TK kind) {
      return *(this->data() + static_cast<std::size_t>(kind - FROM));
   }

   uint8_t operator[](TK kind) const {
      return *(this->data() + static_cast<std::size_t>(kind - FROM));
   }

   TK tokenAt(std::size_t i) const {
      return static_cast<TK>(i + static_cast<std::size_t>(FROM));
   }

   uint8_t consume(TK kind) {
      if ((*this)[kind]) {
         return (*this)[kind]--;
      }
      return 0;
   }
};
// ---------------------------------------------------------------------------
bool isTypeQualifier(token::Kind kind) {
   return kind >= token::Kind::CONST && kind <= token::Kind::ATOMIC;
}
// ---------------------------------------------------------------------------
bool isTypeSpecifierQualifier(token::Kind kind) {
   return kind >= token::Kind::VOID && kind <= token::Kind::ALIGNAS;
}
// ---------------------------------------------------------------------------
bool isAbstractDeclaratorStart(token::Kind kind) {
   return kind == token::Kind::ASTERISK || kind == token::Kind::L_BRACE || kind == token::Kind::L_BRACKET;
}
// ---------------------------------------------------------------------------
bool isDeclaratorStart(token::Kind kind) {
   return kind == token::Kind::IDENT || isAbstractDeclaratorStart(kind);
}
// ---------------------------------------------------------------------------
bool isStorageClassSpecifier(token::Kind kind) {
   return (kind >= token::Kind::AUTO && kind <= token::Kind::THREAD_LOCAL) || kind == token::Kind::TYPEDEF;
}
// ---------------------------------------------------------------------------
bool isFunctionSpecifier(token::Kind kind) {
   return kind == token::Kind::INLINE || kind == token::Kind::NORETURN;
}
// ---------------------------------------------------------------------------
bool isDeclarationSpecifier(token::Kind kind) {
   return isTypeSpecifierQualifier(kind) || isStorageClassSpecifier(kind) || isFunctionSpecifier(kind);
}
// ---------------------------------------------------------------------------
bool isSelectionStmtStart(token::Kind kind) {
   return kind == token::Kind::IF || kind == token::Kind::SWITCH;
}
// ---------------------------------------------------------------------------
bool isIterationStmtStart(token::Kind kind) {
   return kind == token::Kind::WHILE || kind == token::Kind::DO || kind == token::Kind::FOR;
}
// ---------------------------------------------------------------------------
bool isJumpStmtStart(token::Kind kind) {
   return kind == token::Kind::GOTO || kind == token::Kind::CONTINUE || kind == token::Kind::BREAK || kind == token::Kind::RETURN;
}
// ---------------------------------------------------------------------------
bool isPrimaryBlockStart(token::Kind kind) {
   return kind == token::Kind::L_C_BRKT || isJumpStmtStart(kind) || isSelectionStmtStart(kind) || isIterationStmtStart(kind);
}
// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseParameterList(std::vector<TY>& paramTys, std::vector<Ident>& paramNames) {
   tracer_ << ENTER{"parseParameterList"};
   while (isDeclarationSpecifier(pos_->getKind())) {
      parseOptAttributeSpecifierSequence();
      TY ty = parseSpecifierQualifierList();

      // todo: (jr) not abstract declarator
      // todo: (jr) isDeclaratorStart
      if (isDeclaratorStart(pos_->getKind())) {
         // todo: handle names in casts/abstract declerators
         Declarator decl = parseDeclarator(ty);
         paramTys.push_back(decl.ty);
         paramNames.push_back(decl.ident);
         scope_.insert(decl.ident, ScopeInfo(decl.ty)); // todo: (jr) error on failure
      } else if (isAbstractDeclaratorStart(pos_->getKind())) {
         ty = parseAbstractDeclarator(ty);
         paramTys.push_back(ty);
         paramNames.push_back(Ident());
      }
      tracer_ << "parameter: " << ty << std::endl;
   }
   tracer_ << EXIT{"parseParameterList"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::State Parser<_EmitterT>::parseStmt(State s) {
   tracer_ << ENTER{"parseStmt"};
   std::vector<attr_t> attr = parseOptAttributeSpecifierSequence();
   expr_t expr;

   Token t = *pos_;
   TK kind = t.getKind();
   // todo: (jr) makr not recursive but ruther iterative as long as there are labels
   if (kind == TK::CASE || kind == TK::DEFAULT || (kind == TK::IDENT && pos_.peek() == TK::COLON)) {
      // todo: labeled-statement
      // if (consumeAnyOf(TK::CASE)) {
      //    // case-statement
      //    expr = parseExpr();
      //    // todo: assert constant expression
      // } else if (consumeAnyOf(TK::DEFAULT)) {
      //    // default-statement
      //
      // } else {
      //    // labeled-statement
      //    // todo: handle label
      // }
      // expect(TK::COLON);
      // parseStmt();
   } else {
      s = parseUnlabledStmt(s, attr);
   }
   tracer_ << EXIT{"parseStmt"};
   return s;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::State Parser<_EmitterT>::parseDeclStmt(State s, const std::vector<attr_t>& attr, bool isTopLevel) {
   // todo: static_assert-declaration
   tracer_ << ENTER{"parseDeclStmt"};

   if (attr.size() > 0 && consumeAnyOf(TK::SEMICOLON)) {
      // todo: (jr) attribute declaration
   } else {
      DeclarationSpecifier declSpec = parseDeclarationSpecifierList();

      do {
         scope_.enter();
         Declarator decl = parseDeclarator(declSpec.ty);
         factory_.clearFragments();
         scope_.leave();

         ssa_t* var = nullptr;
         ScopeInfo* info = scope_.find(decl.ident);

         if (decl.ty.kind() == TYK::FN_T) {
            if (info) {
               if (info->ty != decl.ty) {
                  diagnostics_ << "Identifier '" << decl.ident << "' redeclared as different kind of symbol" << std::endl;
                  diagnostics_ << "Previous declaration: " << info->ty << std::endl;
               } else if (info->ssa && !emitter_.isFnProto(static_cast<fn_t*>(info->ssa))) {
                  diagnostics_ << "Redefinition of function '" << decl.ident << "'" << std::endl;
               }
            }

            if (!info || !info->ssa) {
               // function prototype
               s.fn = emitter_.emitFnProto(decl.ident, decl.ty.emitterType());
            } else {
               s.fn = static_cast<fn_t*>(info->ssa);
            }

            if (pos_->getKind() == TK::L_C_BRKT) {
               if (!isTopLevel) {
                  diagnostics_ << "Nested function definitions not allowed" << std::endl;
               }
               // function body
               parseFunctionDefinition(decl, s);
               goto end_parse_decl;
            }

            if (!info) {
               scope_.insert(decl.ident, ScopeInfo(decl.ty, s.fn));
            } else {
               info->ssa = s.fn;
            }

         } else {
            // todo: (jr) initializer
            const_t* value = nullptr;
            if (consumeAnyOf(TK::ASSIGN)) {
               assert(decl.ty.kind() != TYK::FN_T && "Function definition not allowed");
               if (consumeAnyOf(TK::L_C_BRKT)) {
                  // initializer-list
                  // todo:
                  parseInitializerList(s);
                  expect(TK::R_C_BRKT);
               } else {
                  expr_t expr = parseAssignmentExpr(s);
                  // todo: track if constant in expr
                  value = static_cast<const_t*>(asRVal(s, expr));
               }
               // todo: handle semantics
            }

            if (isTopLevel || declSpec.storageClass[0] == TK::STATIC || declSpec.storageClass[1] == TK::STATIC) {
               var = emitter_.emitGlobalVar(decl.ty.emitterType(), decl.ident, value);
            } else {
               var = emitter_.emitLocalVar(s.fn, s.entry, decl.ty.emitterType(), decl.ident, value);
            }

            if (!scope_.insert(decl.ident, ScopeInfo(decl.ty, var))) {
               diagnostics_ << pos_->getLoc() << "Redefinition of variable " << decl.ident << std::endl;
            }
         }

      } while (consumeAnyOf(TK::COMMA));
      expect(TK::SEMICOLON);
   }
end_parse_decl:
   tracer_ << EXIT{"parseDeclStmt"};
   return s;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseInitializerList(State s) {
   if (consumeAnyOf(TK::L_C_BRKT)) {
      // todo: initializer-list
      tracer_ << " = { ... }" << std::endl;
      parseOpt(TK::COMMA);
      expect(TK::R_C_BRKT);
   } else {
      // expression
      auto expr = parseExpr(s);
      tracer_ << " = " << *expr << std::endl;
      // todo: handle semantics
   }
   return nullptr;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseFunctionDefinition(Declarator& decl, State s) {
   tracer_ << ENTER{"function-definition"};
   s.entry = s.bb = emitter_.emitFn(s.fn);
   scope_.enter();
   for (unsigned i = 0; i < decl.ty.getParamTys().size(); ++i) {
      TY paramTy = decl.ty.getParamTys()[i];
      Ident paramName = decl.paramNames[i];
      ssa_t* paramValue = emitter_.getParam(s.fn, i);
      ssa_t* localParamValue = emitter_.emitLocalVar(s.fn, s.entry, paramTy.emitterType(), paramName, paramValue);
      scope_.insert(paramName, ScopeInfo(paramTy, localParamValue));
   }

   // function-definition
   parseCompoundStmt(s);
   scope_.leave();
   tracer_ << EXIT{"function-definition"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::State Parser<_EmitterT>::parseCompoundStmt(State s) {
   tracer_ << ENTER{"parseCompoundStmt"};
   expect(TK::L_C_BRKT);

   while (pos_->getKind() != TK::R_C_BRKT && pos_->getKind() != TK::END) {
      auto attr = parseOptAttributeSpecifierSequence();

      if (isTypeSpecifierQualifier(pos_->getKind())) {
         s = parseDeclStmt(s, attr);
      } else if (pos_->getKind() == TK::IDENT && pos_.peek() == TK::COLON) {
         // todo: handle label
      } else {
         s = parseUnlabledStmt(s, attr);
      }
   }
   expect(TK::R_C_BRKT);
   tracer_ << EXIT{"parseCompoundStmt"};
   return s;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::State Parser<_EmitterT>::parseUnlabledStmt(State s, const std::vector<attr_t>& attr) {
   tracer_ << ENTER{"parseUnlabledStmt"};
   Token t = *pos_;
   TK kind = t.getKind();
   bb_t* cont = nullptr;

   if (kind == TK::L_C_BRKT) {
      // todo: attribute-specifier-sequenceopt
      scope_.enter();
      s = parseCompoundStmt(s);
      scope_.leave();
   } else if (isSelectionStmtStart(kind)) {
      // selection-statement
      bb_t* then = emitter_.emitBB(Ident("then"), s.fn);
      bb_t* otherwise = nullptr;
      expr_t cond;

      if (consumeAnyOf(TK::IF)) {
         expect(TK::L_BRACE);
         cond = parseExpr(s);
         expect(TK::R_BRACE);
         std::swap(s.bb, then);
         parseStmt(s);
         std::swap(s.bb, then);

         if (consumeAnyOf(TK::ELSE)) {
            otherwise = emitter_.emitBB(Ident("else"), s.fn);
            std::swap(s.bb, otherwise);
            parseStmt(s);
            std::swap(s.bb, otherwise);
         }

      } else {
         expect(TK::SWITCH);
         expect(TK::L_BRACE);
         cond = parseExpr(s);
         expect(TK::R_BRACE);
         // todo: parseStmt();
      }

      cont = emitter_.emitBB(Ident("cont"), s.fn);
      if (otherwise) {
         emitter_.emitBranch(s.bb, then, otherwise, asRVal(s, cond));
         emitter_.emitJump(otherwise, cont);
      } else {
         emitter_.emitBranch(s.bb, then, cont, asRVal(s, cond));
      }
      emitter_.emitJump(then, cont);
   } else if (isIterationStmtStart(kind)) {
      // iteration-statement
      expr_t cond;
      bb_t* body = emitter_.emitBB(Ident("body"), s.fn);
      if (consumeAnyOf(TK::WHILE)) {
         expect(TK::L_BRACE);
         cond = parseExpr(s);
         expect(TK::R_BRACE);
         parseStmt(s);
      } else if (consumeAnyOf(TK::DO)) {
         parseStmt(s);
         expect(TK::WHILE);
         expect(TK::L_BRACE);
         cond = parseExpr(s);
         expect(TK::R_BRACE);
      } else {
         expect(TK::FOR);
         expect(TK::L_BRACE);

         // for ( expressionopt ; expressionopt ; expressionopt ) secondary-block
         // for ( declaration expressionopt ; expressionopt ) secondary-block
      }
   } else if (isJumpStmtStart(kind)) {
      // jump-statement
      // todo: (jr) not implemented
      if (consumeAnyOf(TK::GOTO)) {
      } else if (consumeAnyOf(TK::CONTINUE)) {
      } else if (consumeAnyOf(TK::BREAK)) {
      } else if (consumeAnyOf(TK::RETURN)) {
         ssa_t* value = nullptr;
         if (pos_->getKind() != TK::SEMICOLON) {
            expr_t expr = parseExpr(s);
            value = asRVal(s, expr);
         }
         expect(TK::SEMICOLON);
         emitter_.emitRet(s.bb, value);

      } else {
         assert(false && "not implemented");
      }
   } else {
      // expression-statement
      expr_t expr;
      if (pos_->getKind() != TK::SEMICOLON) {
         expr = parseExpr(s);
      }
      expect(TK::SEMICOLON);
      if (!expr && !attr.empty()) {
         diagnostics_ << "Attribute specifier without expression" << std::endl;
      }
   }
   tracer_ << EXIT{"parseUnlabledStmt"};
   if (cont) {
      s.bb = cont;
   }
   return s;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseExpr(State s, short midPrec) {
   tracer_ << ENTER{"parseExpr"};
   tracer_ << "current token: " << *pos_ << std::endl;

   expr_t lhs = parsePrimaryExpr(s);
   tracer_ << "current token: " << *pos_ << std::endl;
   while (*pos_ && pos_->getKind() != TK::SEMICOLON) {
      op::OpSpec spec{op::getOpSpec(*pos_)};

      if (midPrec && spec.precedence >= midPrec) {
         break;
      }

      TK tk = pos_->getKind();

      switch (tk) {
         TK next;
         case TK::END:
         case TK::R_BRACE:
         case TK::R_BRACKET:
            tracer_ << EXIT{"parseExpr"};
            return lhs;
         case TK::INC:
         case TK::DEC:
            lhs = emitUnOp(s, tk == TK::INC ? op::Kind::POSTINC : op::Kind::POSTDEC, std::move(lhs));
            ++pos_;
            continue;
         case TK::L_BRACKET:
            ++pos_;
            // todo: lhs = std::make_unique<Expr>(s.bb, op::Kind::SUBSCRIPT, std::move(lhs), parseExpr(s.bb));
            expect(TK::R_BRACKET);
            continue;
         case TK::DEREF:
         case TK::PERIOD:
            ++pos_;
            assert(pos_->getKind() == TK::IDENT && "member access"); // todo: (jr) not implemented
            // todo: lhs = std::make_unique<Expr>(s.bb, tk == TK::DEREF ? op::Kind::MEMBER_DEREF : op::Kind::MEMBER, std::move(lhs), std::make_unique<Expr>(s.bb, *pos_));
            ++pos_;
            continue;
         case TK::L_BRACE:
            next = pos_.peek();
            if (isStorageClassSpecifier(next) || isTypeSpecifierQualifier(next)) {
               // compound literal
               assert(false && "compound literal"); // todo: (jr) not implemented
            } else {
               // emit function call
               ++pos_;
               std::vector<ssa_t*> args;
               while (pos_->getKind() != TK::R_BRACE) {
                  if (args.size() > 0) {
                     expect(TK::COMMA);
                  }
                  expr_t expr = parseExpr(s);
                  // todo: type cast
                  args.push_back(asRVal(s, expr));
               }
               // todo: (jr) not implemented
               expect(TK::R_BRACE);
               ssa_t* ssa = emitter_.emitCall(s.bb, lhs->ident, static_cast<fn_t*>(asRVal(s, lhs)), args);
               lhs = std::make_unique<Expr>(op::Kind::CALL, lhs->ty.getRetTy(), std::move(lhs), ssa);
               continue;
            }
         default:
            // binary operator
            ++pos_;
      }

      expr_t rhs = parseExpr(s, spec.precedence + !spec.leftAssociative);
      op::Kind kind = op::getBinOpKind(tk);
      ssa_t* ssa = emitter_.emitBinOp(s.bb, Ident(), kind, asRVal(s, lhs), asRVal(s, rhs));
      lhs = std::make_unique<Expr>(kind, TY::commonRealType(kind, lhs->ty, rhs->ty), std::move(lhs), std::move(rhs), ssa);
   }
   std::cout << std::string(80, '-') << std::endl
             << "src loc: " << std::endl;
   std::cout << diagnostics_.getSourceLine(lhs->loc) << std::endl;
   std::cout << std::string(80, '-') << std::endl;
   std::cout << *lhs << std::endl;
   tracer_ << EXIT{"parseExpr"};
   return lhs;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parsePrimaryExpr(State s) {
   tracer_ << ENTER{"parsePrimaryExpr"};
   tracer_ << "current token: " << *pos_ << std::endl;
   expr_t expr;
   op::Kind kind;
   Token t = *pos_;

   TY ty;
   Ident name;
   ScopeInfo* info;
   ssa_t* value;
   switch (t.getKind()) {
      case TK::ICONST:
      case TK::U_ICONST:
      case TK::L_ICONST:
      case TK::UL_ICONST:
      case TK::LL_ICONST:
      case TK::ULL_ICONST:
      case TK::FCONST:
      case TK::DCONST:
      case TK::LDCONST:
         ty = factory_.fromToken(t);
         value = emitter_.emitConst(s.bb, ty.emitterType(), Ident(), t.getValue<int>());
         expr = std::make_unique<Expr>(pos_->getLoc(), ty, value, true);
         // todo:expr->isConstExpr = true;
         ++pos_;
         break;
      case TK::IDENT:
         name = t.getValue<Ident>();
         info = scope_.find(name);
         if (!info) {
            diagnostics_ << "Undeclared identifier '" << name << "'" << std::endl;
         } else {
            value = info->ssa;
            ty = info->ty;
         }
         expr = std::make_unique<Expr>(pos_->getLoc(), ty, name, value);
         ++pos_;
         break;
      case TK::SLITERAL:
      case TK::WB_ICONST:
      case TK::UWB_ICONST:
         // todo: (jr) is this also a string? case TK::CLITERAL:
         assert(false && "Not implemented");
         // expr = std::make_unique<Expr>(s.bb, t);
         ++pos_;
         break;

      // todo: (jr) type cast
      case TK::L_BRACE:
         ++pos_;
         if (isTypeSpecifierQualifier(pos_->getKind())) {
            TY ty = parseTypeName();
            expect(TK::R_BRACE);
            expr_t lhs = parseExpr(s, 2);
            // todo: (jr) type cast
            expr = std::make_unique<Expr>(op::Kind::CAST, ty, std::move(lhs), nullptr);
         } else {
            expr = parseExpr(s);
            expr->setPrec(0);
            expect(TK::R_BRACE);
         }
         break;

      case TK::VOID:
      case TK::CHAR:
      case TK::SHORT:
      case TK::INT:
      case TK::LONG:
      case TK::FLOAT:
      case TK::DOUBLE:
      case TK::SIGNED:
      case TK::UNSIGNED:
      case TK::BITINT:
      case TK::BOOL:
      case TK::COMPLEX:
      case TK::DECIMAL32:
      case TK::DECIMAL64:
      case TK::DECIMAL128:
      case TK::STRUCT:
      case TK::UNION:
      case TK::ENUM:
      case TK::TYPEDEF:
      case TK::TYPEOF:
      case TK::CONST:
      case TK::RESTRICT:
      case TK::VOLATILE:
      case TK::ATOMIC:
      case TK::ALIGNAS:
         assert(false && "Not implemented"); // todo: (jr) not implemented
         break;

      // todo: (jr) generic selection
      default:
         // unary prefix operator
         switch (t.getKind()) {
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
               diagnostics_ << "Unexpected token for primary expression: " << t.getKind() << std::endl;
               tracer_ << EXIT{"parsePrimaryExpr"};
               return expr;
         }
         ++pos_;
         expr_t operand = parseExpr(s, 2);
         // todo: type
         expr = emitUnOp(s, kind, std::move(operand));
   }
   tracer_ << EXIT{"parsePrimaryExpr"};
   return expr;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::TY Parser<_EmitterT>::parseAbstractDeclarator(TY specifierQualifier) {
   // todo: (jr) dublicate code
   // todo: remove dublicate types
   tracer_ << ENTER{"parseAbstractDeclarator"};
   Declarator decl = parseDeclaratorImpl();
   if (decl.ty) {
      *decl.base = specifierQualifier;
      // now, there might be duplicate types
   } else {
      decl.ty = specifierQualifier;
   }

   if (decl.ident) {
      diagnostics_ << "Identifier '" << decl.ident << "' not allowed in abstract declarator" << std::endl;
   }

   TY ty = factory_.harden(decl.ty);
   tracer_ << "ty: " << ty << std::endl;
   tracer_ << EXIT{"parseAbstractDeclarator"};
   return ty;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::Declarator Parser<_EmitterT>::parseDeclarator(TY specifierQualifier) {
   tracer_ << ENTER{"parseDeclarator"};
   SrcLoc loc = pos_->getLoc();
   tracer_ << *pos_ << std::endl;
   tracer_ << "loc: " << loc.loc << std::endl;

   Declarator decl{parseDeclaratorImpl()};
   if (decl.ty) {
      *decl.base = specifierQualifier;
      // now, there might be duplicate types
   } else {
      decl.ty = specifierQualifier;
   }
   decl.ty = factory_.harden(decl.ty);
   loc |= pos_->getLoc();
   if (!decl.ident) {
      diagnostics_ << loc << "Expected identifier in declaration" << std::endl;
   }
   tracer_ << EXIT{"parseDeclarator"};
   return decl;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::Declarator Parser<_EmitterT>::parseDeclaratorImpl() {
   tracer_ << ENTER{"parseDeclaratorImpl"};

   TY lhsTy;
   DeclTypeBaseRef lhsBase;
   Declarator decl{};

   while (consumeAnyOf(TK::ASTERISK)) {
      parseOptAttributeSpecifierSequence();
      // todo: (jr) type-qualifier-list
      TY ty = factory_.ptrTo({});
      while (isTypeQualifier(pos_->getKind())) {
         ty.addQualifier(pos_->getKind());
         ++pos_;
      }
      lhsBase.chain(ty);
      if (!lhsTy) {
         lhsTy = ty;
      }
   }

   TY rhsTy{};
   DeclTypeBaseRef rhsBase{};

   // direct-declarator
   if (Token t = consumeAnyOf(TK::IDENT)) {
      decl.ident = t.getValue<Ident>();
   }

   if (!decl.ident && pos_->getKind() == TK::L_BRACE) {
      TK next{pos_.peek()};

      // todo: (jr) non abstract declarator start
      if (isAbstractDeclaratorStart(next)) {
         // abstract-declarator
         expect(TK::L_BRACE);

         Declarator inner = parseDeclaratorImpl();
         rhsTy = inner.ty;
         rhsBase = inner.base;
         decl.ident = inner.ident;

         expect(TK::R_BRACE);
      } else {
         // function-declarator, handled below
      }
   }
   parseOptAttributeSpecifierSequence();

   while (Token t = consumeAnyOf(TK::L_BRACKET, TK::L_BRACE)) {
      TK kind = t.getKind();
      if (kind == TK::L_BRACKET) {
         // array-declarator
         bool static_ = false;
         bool unspecSize = true;
         bool varLen = false;
         std::size_t size = 0;

         static_ = parseOpt(TK::STATIC);

         // todo: (jr) type-qualifier-list only for non abstract declarator
         static_ |= parseOpt(TK::STATIC);

         if (parseOpt(TK::ASTERISK)) {
            unspecSize = true;
            varLen = true;
            if (static_) {
               diagnostics_ << "static and * used together" << std::endl;
            }
         } else {
            // todo: assignment-expression opt
            // todo: static requires assignment-expression
            if (pos_->getKind() != TK::R_BRACKET) {
               // todo: auto expr = parseExpr(s, 14);
               // todo: assert(expr->ty == TY{TYK::INT} && "array size must be integer");
               size = 3; // todo: expr->token.template getValue<int>();
               unspecSize = false;
            }
         }

         // todo: unspecSize, varLen
         // todo: static
         TY arrTy{factory_.arrayOf({}, size, varLen, unspecSize)};
         rhsBase.chain(arrTy);
         if (!rhsTy) {
            rhsTy = arrTy;
         }

         expect(TK::R_BRACKET);
      } else {
         // function-declarator
         // todo: parameter-type-listopt

         std::vector<TY> paramTys{};
         parseParameterList(paramTys, decl.paramNames);
         bool varargs = false;

         if (paramTys.size() == 0 && consumeAnyOf(TK::ELLIPSIS)) {
            varargs = true;
         } else if (consumeAnyOf(TK::COMMA)) {
            expect(TK::ELLIPSIS);
            varargs = true;
         }
         expect(TK::R_BRACE);

         TY fnTy{factory_.function({}, paramTys, varargs)};
         rhsBase.chain(fnTy);
         if (!rhsTy) {
            rhsTy = fnTy;
         }
      }
      parseOptAttributeSpecifierSequence();
   }

   if (rhsTy && lhsTy) {
      decl.ty = rhsTy;
      *rhsBase = lhsTy;
      decl.base = lhsBase;
   } else if (rhsTy) {
      decl.ty = rhsTy;
      decl.base = rhsBase;
   } else {
      decl.ty = lhsTy;
      decl.base = lhsBase;
   }

   tracer_ << EXIT{"parseDeclaratorImpl"};
   return decl;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseMemberDeclaratorList() {
   parseOptAttributeSpecifierSequence();
   parseSpecifierQualifierList();
   do {
      // todo: (jr) declarator
      // todo: (jr) bitfield
   } while (pos_->getKind() == TK::COLON);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::DeclarationSpecifier Parser<_EmitterT>::parseDeclarationSpecifierList(bool storageClassSpecifiers, bool functionSpecifiers) {
   // todo: (jr) storageClassSpecifiers and functionSpecifiers not implemented
   (void) storageClassSpecifiers;
   (void) functionSpecifiers;

   tracer_ << ENTER{"parseDeclarationSpecifierList"};
   DeclarationSpecifier declSpec{};
   TY& ty{declSpec.ty};
   Token ident;

   TokenCounter<TK::CHAR, TK::DECIMAL128> tycount{};
   auto& scSpec{declSpec.storageClass};

   bool qualifiers[3] = {false, false, false};

   for (Token t = *pos_; t.getKind() >= TK::VOID && t.getKind() <= TK::ALIGNAS; t = *++pos_) {
      TK type = t.getKind();

      if (isStorageClassSpecifier(type)) {
         if (!storageClassSpecifiers) {
            diagnostics_ << pos_->getLoc() << "Storage class specifier not allowed here" << std::endl;
         } else if (scSpec[0] == TK::END) {
            scSpec[0] = type;
         } else if (scSpec[1] == TK::END) {
            scSpec[1] = type;
         } else {
            diagnostics_ << pos_->getLoc() << "Multiple storage class specifiers" << std::endl;
         }
         continue;
      }

      if (isTypeQualifier(type)) {
         qualifiers[static_cast<int>(type) - static_cast<int>(TK::CONST)] = true;
         continue;
      }

      if (ty) {
         diagnostics_ << "Type already set" << std::endl;
         break;
      }

      if (type == TK::BITINT) {
         assert(false && "Not implemented"); // todo: (jr) not implemented
      }

      if (type >= TK::CHAR && type <= TK::DECIMAL128) {
         tycount[type]++;
         continue;
      }
      std::vector<TaggedTY> members;
      switch (type) {
         case TK::STRUCT:
         case TK::UNION:
            ++pos_;
            parseOptAttributeSpecifierSequence();
            ident = consumeAnyOf(TK::IDENT);
            tracer_ << "struct/union: " << ident << std::endl;
            tracer_ << "current token: " << *pos_ << std::endl;
            if (consumeAnyOf(TK::L_C_BRKT)) {
               // struct-declaration-list
               tracer_ << "struct-declaration-list" << std::endl;
               do {
                  // todo: static-assert-declaration
                  parseOptAttributeSpecifierSequence();
                  TY memberTy = parseSpecifierQualifierList();
                  // member-declarator-listop
                  // parseMemberDeclaratorList();
                  // todo: members.push_back(memberTy);
                  expect(TK::SEMICOLON);
               } while (pos_->getKind() != TK::R_C_BRKT && pos_->getKind() != TK::END);
            } else if (!ident) {
               diagnostics_ << "Expected identifier or member declaration but got " << *pos_ << std::endl;
            }
            // todo: incomplete struct or union
            ty = factory_.structOrUnion(type, members, false);
            tracer_ << "token after struct/union: " << *pos_ << std::endl;
            break;
         case TK::ENUM:
         case TK::TYPEDEF:
         case TK::TYPEOF:
         case TK::ATOMIC:
         case TK::ALIGNAS:
            assert(false && "Not implemented"); // todo: (jr) not implemented
            break;
         case TK::VOID:
            ty = factory_.voidTy();
            break;
         default:
            diagnostics_ << "Unexpected token: " << type << std::endl;
            break;
      }
   }

   if (!ty) {
      // float | DECIMAL[32|64|128] | COMPLEX | BOOL
      constexpr std::array<std::pair<TK, TYK>, 6> mapping = {
          {{TK::FLOAT, TYK::FLOAT},
           {TK::DECIMAL32, TYK::DECIMAL32},
           {TK::DECIMAL64, TYK::DECIMAL64},
           {TK::DECIMAL128, TYK::DECIMAL128},
           {TK::COMPLEX, TYK::COMPLEX},
           {TK::BOOL, TYK::BOOL}}};

      for (auto [k, v] : mapping) {
         if (tycount.consume(k)) {
            ty = factory_.realTy(v);
            break;
         }
      }

      if (!ty) {
         // [signed | unsigned] char
         // [signed | unsigned] short [int]
         // [signed | unsigned] int | signed | unsigned
         // [signed | unsigned] long [int]
         // [signed | unsigned] long long [int]
         // long double
         // double

         bool unsigned_ = tycount.consume(TK::UNSIGNED);
         bool signed_ = tycount.consume(TK::SIGNED);
         bool int_ = tycount.consume(TK::INT);

         int longs = std::min(2, static_cast<int>(tycount[TK::LONG]));

         if (unsigned_ && signed_) {
            diagnostics_ << "specifier singed and unsigned used together" << std::endl;
         }

         if (tycount.consume(TK::CHAR)) {
            // int token cannot be used with char
            tycount[TK::INT] += int_;
            ty = factory_.integralTy(TYK::CHAR, unsigned_);
         } else if (tycount.consume(TK::SHORT)) {
            ty = factory_.integralTy(TYK::SHORT, unsigned_);
         } else if (tycount[TK::DOUBLE] >= 1) {
            --tycount[TK::DOUBLE];
            if (longs == 1) {
               --tycount[TK::LONG];
               ty = factory_.realTy(TYK::LONGDOUBLE);
            } else {
               ty = factory_.realTy(TYK::DOUBLE);
            }
         } else if (longs == 1) {
            tycount[TK::LONG] -= longs;
            ty = factory_.integralTy(TYK::LONG, unsigned_);
         } else if (longs == 2) {
            tycount[TK::LONG] -= longs;
            ty = factory_.integralTy(TYK::LONGLONG, unsigned_);
         } else {
            // must be int
            ty = factory_.integralTy(TYK::INT, unsigned_);
         }
      }
   }

   ty.qualifiers.CONST = qualifiers[0];
   ty.qualifiers.RESTRICT = qualifiers[1];
   ty.qualifiers.VOLATILE = qualifiers[2];

   for (int i = 0; i < static_cast<int>(tycount.size()); ++i) {
      if (tycount[tycount.tokenAt(i)]) {
         diagnostics_ << "specifier " << tycount.tokenAt(i) << " used " << static_cast<int>(tycount[tycount.tokenAt(i)]) << " times with " << ty << std::endl;
      }
   }

   if (scSpec[1] != TK::END) {
      if (scSpec[0] > scSpec[1]) {
         std::swap(scSpec[0], scSpec[1]);
      }
      if ((scSpec[0] == TK::AUTO && scSpec[1] == TK::TYPEDEF) ||
          (scSpec[1] == TK::THREAD_LOCAL && !(scSpec[0] == TK::STATIC || scSpec[0] == TK::EXTERN)) ||
          (scSpec[0] == TK::CONSTEXPR && !(scSpec[1] == TK::REGISTER || scSpec[1] == TK::EXTERN))) {
         diagnostics_ << "Invalid storage class specifiers: " << scSpec[0] << " used with " << scSpec[1] << std::endl;
      }
   }

   // todo: parseOptAttributeSpecifierSequence();
   ty = factory_.harden(ty); // also harden base types
   tracer_ << EXIT{"parseDeclarationSpecifierList"};
   return declSpec;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::TY Parser<_EmitterT>::parseSpecifierQualifierList() {
   return parseDeclarationSpecifierList(false, false).ty;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
std::vector<typename Parser<_EmitterT>::attr_t> Parser<_EmitterT>::parseOptAttributeSpecifierSequence() {
   // todo: (jr) not implemented
   std::vector<Token> attrs{};
   while (pos_->getKind() == TK::L_BRACKET && pos_.peek() == TK::L_BRACKET) {
      ++pos_;
      ++pos_;
      unsigned attrCount = 0;
      // attribute-list
      while (!consumeAnyOf(TK::R_BRACKET)) {
         // attribute
         if (attrCount) {
            expect(TK::COMMA);
         }

         Token t = expect(TK::IDENT);
         attrs.push_back(t);
         ++attrCount;

         // todo: attribute-prefixed-token
         // todo: attribute-argument-clause(opt)
      }
      expect(TK::R_BRACKET);
   }
   return attrs;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::TY Parser<_EmitterT>::parseTypeName() {
   tracer_ << ENTER{"parseTypeName"};
   Token t = *pos_;
   // parse specifier-qualifier-list
   TY ty = parseSpecifierQualifierList();

   // todo: is this needed? if (isAbstractDeclaratorStart(pos_->getKind()))
   // todo: attribute-specifier-sequence (opt)
   TY declTy = parseAbstractDeclarator(ty);

   tracer_ << "type-name: " << ty << std::endl;
   tracer_ << EXIT{"parseTypeName"};
   return declTy;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_PARSER_H
