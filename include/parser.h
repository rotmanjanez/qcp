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
#include "type.h"
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
   using Factory = type::BaseFactory<_EmitterT>;
   using DeclTypeBaseRef = typename Factory::DeclTypeBaseRef;

   using OpSpec = op::OpSpec;

   using attr_t = Token;

   using Expr = Expr<TY>;
   using ssa = std::unique_ptr<Expr>;
   using bb_t = typename _EmitterT::bb_t;
   using phi_t = typename _EmitterT::phi_t;
   using ssa_t = typename _EmitterT::ssa_t;
   using ty_t = typename _EmitterT::ty_t;
   using fn_t = typename _EmitterT::fn_t;

   Parser(std::string_view prog, DiagnosticTracker& diagnostics, std::ostream& logStream = std::cout) : tokenizer_{prog, diagnostics},
                                                                                                        pos_{tokenizer_.begin()},
                                                                                                        diagnostics_{diagnostics},
                                                                                                        tracer_{logStream} {
      Factory::setEmitter(emitter_);
   }

   /*
(6.9.1) external-declaration:
   function-definition
   declaration
(6.9.2) function-definition:
   attribute-specifier-sequenceopt declaration-specifiers declarator function-body
*/

   void parse() {
      std::cout << std::string(80, '-') << '\n';
      tracer_ << ENTER{"parseTranslationUnit"};
      while (*pos_) {
         std::vector<attr_t> attr = parseOptAttributeSpecifierSequence();
         TY ty = parseDeclarationSpecifierList();
         auto [declTy, name] = parseDeclarator(ty);
         if (pos_->getKind() == TK::L_C_BRKT) {
            tracer_ << ENTER{"function-definition"};
            fn_ = emitter_.emitFnProto(name, declTy.emitterType());
            auto fn_entry_ = emitter_.emitFn(fn_);

            // function-definition
            parseCompoundStmt(fn_entry_);
            tracer_ << EXIT{"function-definition"};
         } else {
            assert(false && "not implemented");
         }
      }

      std::cout << std::string(80, '-') << '\n';
      std::cout << "types:\n";
      for (auto& ty : Factory::getTypes()) {
         std::cout << ty << '\n';
      }
      tracer_ << EXIT{"parseTranslationUnit"};
   }

   bb_t* parseStmt(bb_t* parent);
   bb_t* parseCompoundStmt(bb_t* parent);
   bb_t* parseUnlabledStmt(bb_t* parent, const std::vector<attr_t>& attr);
   bb_t* parseDeclStmt(bb_t* parent, const std::vector<attr_t>& attr);
   ssa parseExpr(short midPrec = 0);
   ssa parsePrimaryExpr();
   TY parseTypeName();
   TY parseSpecifierQualifierList();
   TY parseDeclarationSpecifierList(bool storageClassSpecifiers = true, bool functionSpecifiers = true);

   private:
   void parseMemberDeclaratorList();
   std::vector<attr_t> parseOptAttributeSpecifierSequence();

   struct Declarator {
      TY ty;
      DeclTypeBaseRef base;
      Ident ident;
   };

   TY parseAbstractDeclarator(TY specifierQualifier);
   std::pair<TY, Ident> parseDeclarator(TY specifierQualifier);

   Declarator parseDeclaratorImpl();
   std::vector<TY> parseParameterList();

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
      return Token{};
   }

   Tokenizer tokenizer_;
   typename Tokenizer::const_iterator pos_;
   _EmitterT emitter_{};
   fn_t* fn_ = nullptr;
   Scope<Ident, TY> scope_{};
   DiagnosticTracker& diagnostics_;
   Tracer tracer_;
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
std::vector<typename Parser<_EmitterT>::TY> Parser<_EmitterT>::parseParameterList() {
   std::vector<TY> params;
   while (isDeclarationSpecifier(pos_->getKind())) {
      parseOptAttributeSpecifierSequence();
      TY ty = parseSpecifierQualifierList();

      // todo: (jr) not abstract declarator
      // todo: (jr) isDeclaratorStart
      if (isAbstractDeclaratorStart(pos_->getKind())) {
         Declarator decl = parseDeclaratorImpl();
         *decl.base = ty;
         ty = decl.ty;
      }
      params.push_back(ty);
   }
   return params;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::bb_t* Parser<_EmitterT>::parseStmt(bb_t* parent) {
   tracer_ << ENTER{"parseStmt"};
   std::vector<attr_t> attr = parseOptAttributeSpecifierSequence();
   ssa expr;

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
      parent = parseUnlabledStmt(parent, attr);
   }
   tracer_ << EXIT{"parseStmt"};
   return parent;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::bb_t* Parser<_EmitterT>::parseDeclStmt(bb_t* parent, const std::vector<attr_t>& attr) {
   // todo: static_assert-declaration
   tracer_ << ENTER{"parseDeclStmt"};

   if (attr.size() > 0 && consumeAnyOf(TK::SEMICOLON)) {
      // todo: (jr) attribute declaration
   } else {
      TY ty = parseDeclarationSpecifierList();

      do {
         auto [declTy, name] = parseDeclarator(ty);
         scope_.insert(name, declTy);
         tracer_ << "declarator: " << name << std::endl;
         tracer_ << "type: " << declTy << std::endl;

         // todo: (jr) initializer

         if (consumeAnyOf(TK::ASSIGN)) {
            if (consumeAnyOf(TK::L_C_BRKT)) {
               // todo: initializer-list
               tracer_ << " = { ... }" << std::endl;
               parseOpt(TK::COMMA);
               expect(TK::R_C_BRKT);
            } else {
               // expression
               auto expr = parseExpr();
               tracer_ << " = " << *expr << std::endl;
               // todo: handle semantics
            }
         }
      } while (consumeAnyOf(TK::COMMA));
      expect(TK::SEMICOLON);
   }

   tracer_ << EXIT{"parseDeclStmt"};
   return parent;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::bb_t* Parser<_EmitterT>::parseCompoundStmt(bb_t* parent) {
   tracer_ << ENTER{"parseCompoundStmt"};
   expect(TK::L_C_BRKT);
   scope_.enter();

   while (pos_->getKind() != TK::R_C_BRKT && pos_->getKind() != TK::END) {
      auto attr = parseOptAttributeSpecifierSequence();

      if (isTypeSpecifierQualifier(pos_->getKind())) {
         parent = parseDeclStmt(parent, attr);
      } else if (pos_->getKind() == TK::IDENT && pos_.peek() == TK::COLON) {
         // todo: handle label
      } else {
         parent = parseUnlabledStmt(parent, attr);
      }
   }
   scope_.leave();
   expect(TK::R_C_BRKT);
   tracer_ << EXIT{"parseCompoundStmt"};
   return parent;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::bb_t* Parser<_EmitterT>::parseUnlabledStmt(bb_t* parent, const std::vector<attr_t>& attr) {
   tracer_ << ENTER{"parseUnlabledStmt"};
   Token t = *pos_;
   TK kind = t.getKind();
   if (kind == TK::L_C_BRKT) {
      // todo: attribute-specifier-sequenceopt
      parseCompoundStmt(parent);
   } else if (isSelectionStmtStart(kind)) {
      // selection-statement
      // todo: ask alexis about string literals in emitter
      bb_t* then = emitter_.emitBB(Ident("then"), fn_);
      bb_t* otherwise;

      if (consumeAnyOf(TK::IF)) {
         expect(TK::L_BRACE);
         ssa cond = parseExpr();
         expect(TK::R_BRACE);
         parseStmt(then);

         if (consumeAnyOf(TK::ELSE)) {
            otherwise = emitter_.emitBB(Ident("else"), fn_);
            parseStmt(otherwise);
         }

      } else {
         expect(TK::SWITCH);
         expect(TK::L_BRACE);
         ssa cond = parseExpr();
         expect(TK::R_BRACE);
         // todo: parseStmt();
      }
   } else if (isIterationStmtStart(kind)) {
      // iteration-statement
      ssa cond;
      bb_t* body = emitter_.emitBB(Ident("body"), fn_);
      if (consumeAnyOf(TK::WHILE)) {
         expect(TK::L_BRACE);
         cond = parseExpr();
         expect(TK::R_BRACE);
         parseStmt(body);
      } else if (consumeAnyOf(TK::DO)) {
         parseStmt(body);
         expect(TK::WHILE);
         expect(TK::L_BRACE);
         cond = parseExpr();
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
   } else {
      ssa expr;
      // expression-statement
      if (pos_->getKind() != TK::SEMICOLON) {
         expr = parseExpr();
      }
      expect(TK::SEMICOLON);
      if (!expr && !attr.empty()) {
         diagnostics_ << "Attribute specifier without expression" << std::endl;
      }
   }
   tracer_ << EXIT{"parseUnlabledStmt"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::ssa Parser<_EmitterT>::parseExpr(short midPrec) {
   tracer_ << ENTER{"parseExpr"};
   tracer_ << "current token: " << *pos_ << std::endl;

   ssa lhs = parsePrimaryExpr();
   tracer_ << "current token: " << *pos_ << std::endl;
   while (*pos_ && pos_->getKind() != TK::SEMICOLON) {
      op::OpSpec spec{op::getOpSpec(*pos_)};

      if (midPrec && spec.precedence >= midPrec) {
         break;
      }

      TK tk = pos_->getKind();

      switch (tk) {
         case TK::END:
         case TK::R_BRACE:
         case TK::R_BRACKET:
            tracer_ << EXIT{"parseExpr"};
            return lhs;
         case TK::INC:
         case TK::DEC:
            lhs = std::make_unique<Expr>(tk == TK::INC ? op::Kind::POSTINC : op::Kind::POSTDEC, std::move(lhs));
            ++pos_;
            continue;
         case TK::L_BRACKET:
            ++pos_;
            lhs = std::make_unique<Expr>(op::Kind::SUBSCRIPT, 1, true, std::move(lhs), parseExpr());
            expect(TK::R_BRACKET);
            continue;
         case TK::DEREF:
         case TK::PERIOD:
            ++pos_;
            assert(pos_->getKind() == TK::IDENT && "member access"); // todo: (jr) not implemented
            lhs = std::make_unique<Expr>(tk == TK::DEREF ? op::Kind::MEMBER_DEREF : op::Kind::MEMBER, std::move(lhs), std::make_unique<Expr>(*pos_));
            ++pos_;
            continue;
         case TK::L_BRACE:
            assert(false && "function call and compound literal"); // todo: (jr) not implemented
         default:
            // binary operator
            ++pos_;
      }

      ssa rhs = parseExpr(spec.precedence + spec.leftAssociative);
      lhs = std::make_unique<Expr>(op::getBinOpKind(tk), std::move(lhs), std::move(rhs));
   }
   tracer_ << EXIT{"parseExpr"};
   return lhs;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::ssa Parser<_EmitterT>::parsePrimaryExpr() {
   tracer_ << ENTER{"parsePrimaryExpr"};
   tracer_ << "current token: " << *pos_ << std::endl;
   ssa expr;
   op::Kind kind;
   Token t = *pos_;

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
         expr = std::make_unique<Expr>(t);
         // todo:expr->isConstExpr = true;
         ++pos_;
         break;
      case TK::IDENT:
      case TK::SLITERAL:
      case TK::WB_ICONST:
      case TK::UWB_ICONST:
         // todo: (jr) is this also a string? case TK::CLITERAL:
         expr = std::make_unique<Expr>(t);
         ++pos_;
         break;

      // todo: (jr) type cast
      case TK::L_BRACE:
         ++pos_;
         if (isTypeSpecifierQualifier(pos_->getKind())) {
            TY ty = parseTypeName();
            expect(TK::R_BRACE);
            expr = std::make_unique<Expr>(op::Kind::CAST, ty, parseExpr(2));
         } else {
            expr = parseExpr();
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
         expr = std::make_unique<Expr>(kind, 2, false, parseExpr(2));
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

   TY ty = Factory::harden(decl.ty);
   tracer_ << "ty: " << ty << std::endl;
   tracer_ << EXIT{"parseAbstractDeclarator"};
   return ty;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
std::pair<typename Parser<_EmitterT>::TY, Ident> Parser<_EmitterT>::parseDeclarator(TY specifierQualifier) {
   tracer_ << ENTER{"parseDeclarator"};
   Declarator decl = parseDeclaratorImpl();
   if (decl.ty) {
      *decl.base = specifierQualifier;
      // now, there might be duplicate types
   } else {
      decl.ty = specifierQualifier;
   }
   decl.ty = Factory::harden(decl.ty);

   if (!decl.ident) {
      diagnostics_ << "Expected identifier in declaration" << std::endl;
   }
   tracer_ << EXIT{"parseDeclarator"};
   return {decl.ty, decl.ident};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::Declarator Parser<_EmitterT>::parseDeclaratorImpl() {
   tracer_ << ENTER{"parseDeclaratorImpl"};

   TY lhsTy;
   DeclTypeBaseRef lhsBase;
   Declarator decl;

   while (consumeAnyOf(TK::ASTERISK)) {
      parseOptAttributeSpecifierSequence();
      // todo: (jr) type-qualifier-list
      TY ty = TY::ptrTo({});
      while (isTypeQualifier(pos_->getKind())) {
         ty.addQualifier(pos_->getKind());
         ++pos_;
      }
      lhsBase.chain(ty);
      if (!lhsTy) {
         lhsTy = ty;
      }
   }

   TY rhsTy;
   DeclTypeBaseRef rhsBase;

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
         bool static_ = false;
         bool unspecSize = true;
         bool varLen = false;
         std::size_t size = 0;

         // array-declarator
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
               auto expr = parseExpr(14);
               // todo: assert(expr->ty == TY{TYK::INT} && "array size must be integer");
               size = 3; // todo: expr->token.template getValue<int>();
               unspecSize = false;
            }
         }

         // todo: unspecSize, varLen
         // todo: static
         TY arrTy{TY::arrayOf({}, size, varLen, unspecSize)};
         rhsBase.chain(arrTy);
         if (!rhsTy) {
            rhsTy = arrTy;
         }

         expect(TK::R_BRACKET);
      } else {
         // function-declarator
         // todo: parameter-type-listopt

         auto params = parseParameterList();
         bool varargs = false;

         if (params.size() == 0 && consumeAnyOf(TK::ELLIPSIS)) {
            varargs = true;
         } else if (consumeAnyOf(TK::COMMA)) {
            expect(TK::ELLIPSIS);
            varargs = true;
         }

         TY fnTy{TY::function({}, params, varargs)};
         rhsBase.chain(fnTy);
         if (!rhsTy) {
            rhsTy = fnTy;
         }
         expect(TK::R_BRACE);
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
Parser<_EmitterT>::TY Parser<_EmitterT>::parseDeclarationSpecifierList(bool storageClassSpecifiers, bool functionSpecifiers) {
   // todo: (jr) storageClassSpecifiers and functionSpecifiers not implemented
   (void) storageClassSpecifiers;
   (void) functionSpecifiers;

   tracer_ << ENTER{"parseDeclarationSpecifierList"};
   TY ty;
   Token ident;

   TokenCounter<TK::CHAR, TK::DECIMAL128> tycount{};
   bool qualifiers[3] = {false, false, false};

   for (Token t = *pos_; t.getKind() >= TK::VOID && t.getKind() <= TK::ALIGNAS; t = *++pos_) {
      TK type = t.getKind();

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
      std::vector<TY> members;
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
                  members.push_back(memberTy);
                  expect(TK::SEMICOLON);
               } while (pos_->getKind() != TK::R_C_BRKT && pos_->getKind() != TK::END);
            } else if (!ident) {
               diagnostics_ << "Expected identifier or member declaration but got " << *pos_ << std::endl;
            }
            // todo: incomplete struct or union
            ty = TY::structOrUnion(type, members, false);
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
            ty = TY(TYK::VOID);
            break;
         default:
            diagnostics_ << "Unexpected token: " << type << std::endl;
            break;
      }
   }

   if (!ty) {
      // float | DECIMAL[32|64|128] | COMPLEX | BOOL
      if (tycount.consume(TK::FLOAT)) {
         ty = TY(TYK::FLOAT);
      } else if (tycount.consume(TK::DECIMAL32)) {
         ty = TY(TYK::DECIMAL32);
      } else if (tycount.consume(TK::DECIMAL64)) {
         ty = TY(TYK::DECIMAL64);
      } else if (tycount.consume(TK::DECIMAL128)) {
         ty = TY(TYK::DECIMAL128);
      } else if (tycount[TK::COMPLEX]) {
         assert(false && "Not implemented"); // todo: (jr) not implemented
      } else if (tycount.consume(TK::BOOL)) {
         ty = TY(TYK::BOOL);
      } else {
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
            ty = TY(TYK::CHAR, unsigned_);
         } else if (tycount.consume(TK::SHORT)) {
            ty = TY(TYK::SHORT, unsigned_);
         } else if (tycount[TK::DOUBLE] >= 1) {
            --tycount[TK::DOUBLE];
            if (longs == 1) {
               --tycount[TK::LONG];
               ty = TY(TYK::LONGDOUBLE);
            } else {
               ty = TY(TYK::DOUBLE);
            }
         } else if (longs == 1) {
            tycount[TK::LONG] -= longs;
            ty = TY(TYK::LONG, unsigned_);
         } else if (longs == 2) {
            tycount[TK::LONG] -= longs;
            ty = TY(TYK::LONGLONG, unsigned_);
         } else {
            // must be int
            ty = TY(TYK::INT, unsigned_);
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

   // todo: parseOptAttributeSpecifierSequence();

   tracer_ << EXIT{"parseDeclarationSpecifierList"};
   return ty;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::TY Parser<_EmitterT>::parseSpecifierQualifierList() {
   return parseDeclarationSpecifierList(false, false);
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

   // todo: is this needed? if (isAbstractDeclaratorStart(pos_->getKind())) {
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
