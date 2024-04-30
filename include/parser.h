#ifndef QCP_PARSER_H
#define QCP_PARSER_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "diagnostics.h"
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
// some expression tracking necessary for types and operator precedence
template <typename _TY>
struct Expr {
   using TY = _TY;
   using Token = token::Token;
   using OpSpec = op::OpSpec;
   using OpKind = op::Kind;

   explicit Expr(OpKind op) : op{op}, opspec{getOpSpec(token)}, lhs{}, rhs{} {}

   Expr(OpKind op, const TY& ty) : op{op}, opspec{getOpSpec(op)}, ty{ty}, lhs{}, rhs{} {}

   Expr(OpKind op, std::unique_ptr<Expr>&& lhs) : op{op}, opspec{getOpSpec(op)}, ty{lhs->ty}, lhs{std::move(lhs)}, rhs{} {}
   Expr(OpKind op, std::unique_ptr<Expr>&& lhs, std::unique_ptr<Expr> rhs) : op{op}, opspec{getOpSpec(op)}, ty{TY::commonRealType(op, lhs->ty, rhs->ty)}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}

   Expr(OpKind op, int8_t precedence, bool lassoc, std::unique_ptr<Expr>&& lhs) : op{op}, opspec{precedence, lassoc}, ty{lhs->ty}, lhs{std::move(lhs)}, rhs{} {}
   Expr(OpKind op, int8_t precedence, bool lassoc, std::unique_ptr<Expr>&& lhs, std::unique_ptr<Expr>&& rhs) : op{op}, opspec{precedence, lassoc}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}

   Expr(OpKind op, const TY& ty, std::unique_ptr<Expr>&& lhs) : op{op}, opspec{getOpSpec(op)}, ty{ty}, lhs{std::move(lhs)}, rhs{} {}

   // todo: (jr) tokens can also be identifier and not only constants
   explicit Expr(const Token& token) : opspec{0, false}, ty{TY::fromToken(token)}, token{token}, lhs{}, rhs{} {}

   OpKind op;
   OpSpec opspec;
   TY ty;
   Token token;
   // todo: bool isIntConstExpr;

   int evalIntConstExpr() {
      // todo:
   }

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
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
class Parser {
   public:
   using Token = token::Token;
   using TK = token::Kind;

   using tokenizer_t = Tokenizer<_DiagnosticTracker>;
   using tokenizer_it_t = typename tokenizer_t::const_iterator;

   using TY = type::Type<std::string>;
   using TYK = type::Kind;
   using Factory = type::BaseFactory<std::string>;
   using DeclTypeBaseRef = typename Factory::DeclTypeBaseRef;

   using Expr = Expr<TY>;
   using ssa = std::unique_ptr<Expr>;

   // , _EmmiterT& emitter emitter_{emitter},
   Parser(std::string_view prog, _DiagnosticTracker& diagnostics, std::ostream& logStream = std::cout) : tokenizer_{prog, diagnostics}, pos_{tokenizer_.begin()}, diagnostics_{diagnostics}, tracer_{logStream} {}

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
      for (auto& ty : type::BaseFactory<std::string>::getTypes()) {
         std::cout << ty << '\n';
      }
   }

   // todo: private: but how testing?
   ssa parseExpr(short precedence = 15);
   ssa parsePrimaryExpr();
   TY parseTypeName();
   TY parseSpecifierQualifierList();
   TY parseDeclarationSpecifierList(bool storageClassSpecifiers, bool functionSpecifiers);
   void parseMemberDeclaration();
   void parseOptAttributeSpecifierSequence();
   std::pair<TY, DeclTypeBaseRef> parseDeclarator();

   void expect(TK kind) {
      if (pos_->getType() == kind) {
         ++pos_;
      } else {
         std::cerr << "Expected '" << kind << " but got " << pos_->getType() << "'\n";
         assert(false && "Expected token not found");
         // todo: diagnostics_ << "Expected '" << kind << "'";
      }
   }

   tokenizer_t tokenizer_;
   typename tokenizer_t::const_iterator pos_;
   // _EmmiterT& emitter_;
   _DiagnosticTracker diagnostics_{};
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
// Parser
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
Parser<_DiagnosticTracker>::ssa Parser<_DiagnosticTracker>::parseExpr(short precedence) {
   tracer_ << ENTER{"parseExpr"};
   ssa lhs = parsePrimaryExpr();

   Token cur{*pos_};
   while (cur.getType() != TK::END) {
      Token cur{*pos_};
      op::OpSpec spec{op::getOpSpec(cur)};

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
            lhs = std::make_unique<Expr>(op::Kind::SUBSCRIPT, 1, true, std::move(lhs), parseExpr(15));
            if (lhs->lhs->opspec.precedence > 1) {
               auto tmp = std::move(lhs->lhs);
               lhs->lhs = std::move(tmp->lhs);
               tmp->lhs = std::move(lhs);
               lhs = std::move(tmp);
            }
            expect(TK::R_BRACKET);
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
         tracer_ << EXIT{"parseExpr"};
         return lhs;
      }

      ssa rhs = parseExpr(spec.precedence - spec.leftAssociative);

      if (spec.precedence < lhs->opspec.precedence) {
         op::Kind op = lhs->op;

         auto tmp = std::move(lhs->rhs);
         lhs->rhs = std::make_unique<Expr>(op, std::move(tmp), std::move(rhs));

      } else {
         lhs = std::make_unique<Expr>(op::getBinOpKind(cur), std::move(lhs), std::move(rhs));
      }
      cur = *pos_;
   }
   tracer_ << EXIT{"parseExpr"};
   return lhs;
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
std::pair<typename Parser<_DiagnosticTracker>::TY, typename Parser<_DiagnosticTracker>::DeclTypeBaseRef> Parser<_DiagnosticTracker>::parseDeclarator() {
   tracer_ << ENTER{"parseDeclarator"};
   Token ident;
   TY ptrTy;
   DeclTypeBaseRef base;

   while (pos_->getType() == TK::ASTERISK) {
      ++pos_;
      parseOptAttributeSpecifierSequence();
      // todo: (jr) type-qualifier-list
      if (base) {
         *base = TY::ptrTo({});
         base = DeclTypeBaseRef(*base);
      } else {
         ptrTy = TY::ptrTo({});
         base = DeclTypeBaseRef(ptrTy);
      }
      while (isTypeQualifier(pos_->getType())) {
         ptrTy.addQualifier(pos_->getType());
         ++pos_;
      }
   }

   DeclTypeBaseRef mid;
   TY ty;

   // direct-declarator
   if (pos_->getType() == TK::IDENT) {
      ident = *pos_;
      ++pos_;
   } else if (pos_->getType() == TK::L_BRACE) {
      TK next{pos_.peek()};
      if (next != TK::L_BRACKET && next != TK::L_BRACE && next != TK::R_BRACE) {
         // abstract-declarator
         ++pos_;
         auto [innerTy, innerBase] = parseDeclarator();
         ty = innerTy;
         mid = innerBase;

         expect(TK::R_BRACE);
      } else {
         // function-declarator, handled below
      }
   } else {
      diagnostics_ << "Expected identifier or '('";
   }

   parseOptAttributeSpecifierSequence();

   TK kind = pos_->getType();
   while (kind == TK::L_BRACKET || kind == TK::L_BRACE) {
      ++pos_;
      if (kind == TK::L_BRACKET) {
         bool static_ = false;
         bool unspecSize = true;
         bool varLen = false;
         std::size_t size = 0;

         // array-declarator
         if (pos_->getType() == TK::STATIC) {
            static_ = true;
            ++pos_;
         }
         // todo: (jr) type-qualifier-list
         if (!static_ && pos_->getType() == TK::STATIC) {
            static_ = true;
            ++pos_;
         }

         if (pos_->getType() == TK::ASTERISK) {
            unspecSize = true;
            varLen = true;
            ++pos_;
            if (static_) {
               diagnostics_ << "static and * used together";
            }
         } else {
            // todo: assignment-expression opt
            // todo: static requires assignment-expression
            if (pos_->getType() != TK::R_BRACKET) {
               auto expr = parseExpr(14);
               assert(expr->ty == TY{TYK::INT} && "array size must be integer");
               size = expr->token.template getValue<int>();
               unspecSize = false;
            }
         }

         // todo: unspecSize, varLen
         // todo: static
         TY arrTy{TY::arrayOf({}, size, varLen, unspecSize)};
         if (mid) {
            *mid = arrTy;
            mid = DeclTypeBaseRef(*mid);
         } else {
            ty = arrTy;
            mid = DeclTypeBaseRef(ty);
         }

         expect(TK::R_BRACKET);
      } else {
         // function-declarator
         // todo: parameter-type-listopt
         std::vector<TY> params;
         bool varargs = false;
         while (isDeclarationSpecifier(pos_->getType())) {
            parseOptAttributeSpecifierSequence();
            TY ty = parseSpecifierQualifierList();

            // todo: (jr) not abstract declarator
            if (isAbstractDeclaratorStart(pos_->getType())) {
               auto [declTy, base] = parseDeclarator();
               *base = ty;
               ty = declTy;
            }
            params.push_back(ty);
         }

         if (params.size() == 0 && pos_->getType() == TK::ELLIPSIS) {
            varargs = true;
            ++pos_;
         } else if (pos_->getType() == TK::COMMA) {
            ++pos_;
            expect(TK::ELLIPSIS);
            varargs = true;
         }

         TY fnTy{TY::function({}, params, varargs)};
         if (mid) {
            *mid = fnTy;
            mid = DeclTypeBaseRef(*mid);
         } else {
            ty = fnTy;
            mid = DeclTypeBaseRef(ty);
         }
         expect(TK::R_BRACE);
      }
      parseOptAttributeSpecifierSequence();
      kind = pos_->getType();
   }

   tracer_ << "merging types\n";
   if (!ty) {
      ty = ptrTy;
   } else if (mid) {
      *mid = ptrTy;
   }

   if (!base) {
      if (mid) {
         base = mid;
      } else {
         base = DeclTypeBaseRef(ty);
      }
   }
   tracer_ << EXIT{"parseDeclarator"};
   return {ty, base};
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
void Parser<_DiagnosticTracker>::parseMemberDeclaration() {
   parseOptAttributeSpecifierSequence();
   parseSpecifierQualifierList();
   do {
      // todo: (jr) declarator
      // todo: (jr) bitfield
   } while (pos_->getType() == TK::COLON);
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
Parser<_DiagnosticTracker>::TY Parser<_DiagnosticTracker>::parseSpecifierQualifierList() {
   return parseDeclarationSpecifierList(false, false);
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
Parser<_DiagnosticTracker>::TY Parser<_DiagnosticTracker>::parseDeclarationSpecifierList(bool storageClassSpecifiers, bool functionSpecifiers) {
   // todo: (jr) storageClassSpecifiers and functionSpecifiers not implemented
   (void) storageClassSpecifiers;
   (void) functionSpecifiers;

   tracer_ << ENTER{"parseDeclarationSpecifierList"};
   TY ty;
   Token ident;

   TokenCounter<TK::CHAR, TK::DECIMAL128> tycount{};
   bool qualifiers[3] = {false, false, false};

   for (Token t = *pos_; t.getType() >= TK::VOID && t.getType() <= TK::ALIGNAS; t = *++pos_) {
      TK type = t.getType();

      if (isTypeQualifier(type)) {
         qualifiers[static_cast<int>(type) - static_cast<int>(TK::CONST)] = true;
         continue;
      }

      if (ty) {
         diagnostics_ << "Type already set";
         break;
      }

      if (type == TK::BITINT) {
         assert(false && "Not implemented"); // todo: (jr) not implemented
      }

      if (type >= TK::CHAR && type <= TK::DECIMAL128) {
         tycount[type]++;
         continue;
      }

      switch (type) {
         case TK::STRUCT:
         case TK::UNION:
            parseOptAttributeSpecifierSequence();
            ident = *pos_;
            ++pos_;
            switch (pos_->getType()) {
               case TK::L_BRACE:
                  do {
                     parseMemberDeclaration();
                  } while (pos_->getType() != TK::R_BRACE);
                  break;
               case TK::SEMICOLON:
                  // end of declaration
                  break;
               default:
                  diagnostics_ << "Expected '{' or ';'. Probably missing a semicolon.";
            }
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
            assert(false && "unexpected token");
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
            diagnostics_ << "specifier singed and unsigned used together\n";
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
         diagnostics_ << "unexpected token";
         std::cerr << "specifier " << tycount.tokenAt(i) << " used " << static_cast<int>(tycount[tycount.tokenAt(i)]) << " times with " << ty << '\n';
         // assert(false && "unexpected token");
      }
   }
   tracer_ << EXIT{"parseDeclarationSpecifierList"};
   return ty;
}
template <typename _DiagnosticTracker>
void Parser<_DiagnosticTracker>::parseOptAttributeSpecifierSequence() {
   // todo: (jr) not implemented
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
Parser<_DiagnosticTracker>::TY Parser<_DiagnosticTracker>::parseTypeName() {
   tracer_ << ENTER{"parseTypeName"};
   Token t = *pos_;
   // parse specifier-qualifier-list
   TY ty = parseSpecifierQualifierList();

   if (isAbstractDeclaratorStart(pos_->getType())) {
      // todo: attribute-specifier-sequence (opt)
      auto [declTy, base] = parseDeclarator();
      *base = ty;
      ty = declTy;
   }

   // todo: check that declator is abstract

   tracer_ << "type-name: " << ty << '\n';
   tracer_ << EXIT{"parseTypeName"};
   return ty;
}
// ---------------------------------------------------------------------------
template <typename _DiagnosticTracker>
Parser<_DiagnosticTracker>::ssa Parser<_DiagnosticTracker>::parsePrimaryExpr() {
   tracer_ << ENTER{"parsePrimaryExpr"};
   ssa expr;
   op::Kind kind;
   Token t = *pos_;

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
         if (isTypeSpecifierQualifier(pos_->getType())) {
            TY ty = parseTypeName();
            expect(TK::R_BRACE);
            expr = std::make_unique<Expr>(op::Kind::CAST, ty, parsePrimaryExpr());
         } else {
            expr = parseExpr();
            expr->opspec.precedence = 0;
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
               diagnostics_.report("expected primary expression");
               tracer_ << EXIT{"parsePrimaryExpr"};
               assert(false && "unexpected token"); // todo: (jr) unexpected token
               return expr;
         }
         ++pos_;
         expr = std::make_unique<Expr>(kind, 2, false, parsePrimaryExpr());
   }
   tracer_ << EXIT{"parsePrimaryExpr"};
   return expr;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_PARSER_H
