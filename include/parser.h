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
#include <variant>
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
   using Factory = type::TypeFactory<_EmitterT>;
   using DeclTypeBaseRef = typename Factory::DeclTypeBaseRef;

   using OpSpec = op::OpSpec;

   using attr_t = Token;

   using bb_t = typename _EmitterT::bb_t;
   using phi_t = typename _EmitterT::phi_t;
   using ssa_t = typename _EmitterT::ssa_t;
   using const_t = typename _EmitterT::const_t;
   using iconst_t = typename _EmitterT::iconst_t;
   using const_or_iconst_t = typename _EmitterT::const_or_iconst_t;
   using ty_t = typename _EmitterT::ty_t;
   using fn_t = typename _EmitterT::fn_t;
   using sw_t = typename _EmitterT::sw_t;

   using ScopeInfo = ScopeInfo<_EmitterT>;
   using Scope = Scope<Ident, ScopeInfo>;
   using Expr = Expr<_EmitterT>;
   using expr_t = std::unique_ptr<Expr>;

   using value_t = typename _EmitterT::value_t;

   Parser(std::string_view prog,
          DiagnosticTracker& diagnostics,
          std::ostream& logStream = std::cout) : tokenizer_{prog, diagnostics},
                                                 pos_{tokenizer_.begin()},
                                                 diagnostics_{diagnostics},
                                                 tracer_{logStream},
                                                 factory_{emitter_} {}

   struct SwitchState {
      SwitchState(sw_t* sw) : sw{sw} {}

      sw_t* sw;
      std::vector<iconst_t*> values{};
      std::vector<bb_t*> blocks{};
   };

   struct State {
      fn_t* fn = nullptr;
      bb_t* entry = nullptr;
      bb_t* bb = nullptr;
      TY retTy{};

      std::vector<bb_t*> unsealedBlocks{};
      std::unordered_map<Ident, bb_t*> labels{};
      std::unordered_map<Ident, std::vector<bb_t*>> incompleteGotos{};
      std::vector<std::pair<bb_t*, value_t>> missingReturns{};
      std::vector<SwitchState> switches{};

   } state;

   void markSealed(bb_t* bb) {
      state.unsealedBlocks.erase(std::remove(state.unsealedBlocks.begin(), state.unsealedBlocks.end(), bb), state.unsealedBlocks.end());
   }

   bool isSealed(bb_t* bb) {
      return bb && std::find(state.unsealedBlocks.begin(), state.unsealedBlocks.end(), bb) == state.unsealedBlocks.end();
   }

   _EmitterT& getEmitter() {
      return emitter_;
   }

   void parse();

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

   // statements
   void parseStmt();
   void parseCompoundStmt();
   void parseUnlabledStmt(const std::vector<attr_t>& attr);
   void parseSelectionStmt();
   void parseIterationStmt();
   void parseJumpStmt();
   void parseDeclStmt(const std::vector<attr_t>& attr);

   void parseFunctionDefinition(Declarator& decl);

   // Expressions
   expr_t parseExpr(short midPrec = 0);
   expr_t parseConditionExpr();
   expr_t parseAssignmentExpr() {
      return parseExpr(14);
   }
   expr_t parseUnaryExpr() {
      return parseExpr(2);
   }
   expr_t parsePrimaryExpr();

   TY parseTypeName();
   TY parseSpecifierQualifierList();

   DeclarationSpecifier parseDeclarationSpecifierList(bool storageClassSpecifiers = true, bool functionSpecifiers = true);

   expr_t parseInitializerList();

   private:
   int parseOptLabelList(std::vector<attr_t>& attr);
   void parseMemberDeclaratorList();
   std::vector<attr_t> parseOptAttributeSpecifierSequence();

   TY parseAbstractDeclarator(TY specifierQualifier);
   Declarator parseDirectDeclarator(TY specifierQualifier);
   // todo: make move
   Declarator parseDeclarator(TY specifierQualifier);
   // todo: make move
   Declarator parseDeclaratorImpl();

   bool parseParameterList(std::vector<TY>& paramTys, std::vector<Ident>& paramNames);

   const_or_iconst_t getConst(value_t value) {
      if (const_t** c = std::get_if<const_t*>(&value)) {
         return *c;
      } else if (iconst_t** c = std::get_if<iconst_t*>(&value)) {
         return *c;
      }
      assert(false && "unreachable"); // todo: llvm unreachable
   }

   value_t asValue(const_or_iconst_t c) {
      if (const_t** c_ = std::get_if<const_t*>(&c)) {
         return *c_;
      }
      return std::get<iconst_t*>(c);
   }

   value_t asRVal(expr_t& expr) {
      if (!expr->ty || isSealed(state.bb)) {
         return emitter_.emitPoison();
      }
      if (expr->mayBeLval() && expr->ty.kind() != TYK::FN_T) {
         return emitter_.emitLoad(state.bb, expr->ty, std::get<ssa_t*>(expr->value), expr->ident);
      }
      return expr->value;
   }

   value_t cast(TY from, TY to, value_t value) {
      if (from == to) {
         return value;
      }

      type::Cast kind;
      if (!from) {
         kind = type::Cast::BITCAST;
      } else if ((from.isIntegerTy() || to.isBoolTy()) && (to.isIntegerTy() || to.isBoolTy())) {
         if (from.rank() > to.rank()) {
            kind = type::Cast::TRUNC;
         } else if (from.rank() < to.rank()) {
            kind = (from.isSignedTy() || from.isSignedCharlikeTy()) ? type::Cast::SEXT : type::Cast::ZEXT;
         } else {
            // different signess does not require a cast in llvm? todo: ask alexis
            return value;
         }
      } else if (from.isIntegerTy() && to.isFloatingTy()) {
         kind = from.isSignedTy() ? type::Cast::SITOFP : type::Cast::UITOFP;
      } else if (from.isFloatingTy() && to.isIntegerTy()) {
         kind = type::Cast::FPTOSI;
      } else if (from.isFloatingTy() && to.isFloatingTy() && from.rank() > to.rank()) {
         kind = type::Cast::FPTRUNC;
      } else if (from.isFloatingTy() && to.isFloatingTy() && from.rank() < to.rank()) {
         kind = type::Cast::FPEXT;
      } else {
         diagnostics_ << "Invalid cast from " << from << " to " << to << std::endl;
         assert(false && "invalid cast");
      }
      return emitCast(from, value, to, kind);
   }

   value_t asBoolByComparison(expr_t& expr, op::Kind op) {
      if (isSealed(state.bb)) {
         return emitter_.emitPoison();
      }
      const_or_iconst_t zero = emitter_.emitConst(expr->ty, 0);
      auto value = asRVal(expr);
      if (std::holds_alternative<ssa_t*>(value)) {
         return emitter_.emitBinOp(state.bb, expr->ty, op, value, asValue(zero));
      }
      return asValue(emitter_.emitConstBinOp(state.bb, expr->ty, op, getConst(value), zero));
   }

   value_t isTruethy(expr_t& expr) {
      if (!expr->ty) {
         return emitter_.emitPoison();
      } else if (expr->ty.isBoolTy()) {
         return asRVal(expr);
      }
      return asBoolByComparison(expr, op::Kind::NE);
   }

   inline bb_t* newBB() {
      bb_t* bb = emitter_.emitBB(state.fn);
      state.unsealedBlocks.push_back(bb);
      return bb;
   }

   void emitJump(bb_t* fromBB, bb_t* toBB) {
      emitter_.emitJump(fromBB, toBB);
      markSealed(fromBB);
   }

   void emitBranch(bb_t* fromBB, bb_t* trueBB, bb_t* falseBB, value_t cond) {
      emitter_.emitBranch(fromBB, trueBB, falseBB, cond);
      markSealed(fromBB);
   }

   void emitJumpIfNotSealed(bb_t* fromBB, bb_t* toBB) {
      if (isSealed(fromBB)) {
         return;
      }
      emitJump(fromBB, toBB);
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

   template <typename cFn, typename Fn, typename... Args>
   value_t emitUnOpImpl(cFn __cFn, Fn __fn, TY ty, value_t value, Args... args) {
      if (!state.bb) {
         return emitter_.emitPoison();
      }
      if (ssa_t** ssa = std::get_if<ssa_t*>(&value)) {
         return __fn(state.bb, ty, *ssa, std::forward<Args>(args)...);
      }
      return asValue(__cFn(state.bb, ty, getConst(value), std::forward<Args>(args)...));
   }

   value_t emitNeg(TY ty, value_t value) {
      return emitUnOpImpl([this](auto&&... args) { return emitter_.emitConstNeg(args...); },
                          [this](auto&&... args) { return emitter_.emitNeg(args...); },
                          ty, value);
   }

   value_t emitBWNeg(TY ty, value_t value) {
      return emitUnOpImpl([this](auto&&... args) { return emitter_.emitConstBWNeg(args...); },
                          [this](auto&&... args) { return emitter_.emitBWNeg(args...); },
                          ty, value);
   }

   value_t emitNot(TY ty, value_t value) {
      return emitUnOpImpl([this](auto&&... args) { return emitter_.emitConstNot(args...); },
                          [this](auto&&... args) { return emitter_.emitNot(args...); },
                          ty, value);
   }

   value_t emitCast(TY fromTy, value_t value, TY toTy, qcp::type::Cast cast) {
      return emitUnOpImpl([this](auto&&... args) { return emitter_.emitConstCast(args...); },
                          [this](auto&&... args) { return emitter_.emitCast(args...); },
                          fromTy, value, toTy, cast);
   }

   Tokenizer tokenizer_;
   typename Tokenizer::const_iterator pos_;
   _EmitterT emitter_{};

   Scope scope_{};

   DiagnosticTracker& diagnostics_;
   Tracer tracer_;
   Factory factory_;

   // todo: may be part of custom scope datastructure
   bool isMissingDefaultInitiation(ssa_t* var) {
      return std::find_if(missingDefaultInitiations_.begin(), missingDefaultInitiations_.end(), [&](auto* v) {
                return v->ssa == var;
             }) != missingDefaultInitiations_.end();
   }

   bool isExternalDeclaration(ssa_t* var) {
      return std::find_if(externDeclarations_.begin(), externDeclarations_.end(), [&](auto* v) {
                return v->ssa == var;
             }) != externDeclarations_.end();
   }
   std::vector<ScopeInfo*> externDeclarations_;
   std::vector<ScopeInfo*> missingDefaultInitiations_;
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
   return (kind >= token::Kind::AUTO && kind <= token::Kind::THREAD_LOCAL) || kind == token::Kind::TYPEDEF || kind == token::Kind::EXTERN || kind == token::Kind::STATIC || kind == token::Kind::REGISTER;
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
bool isDeclarationStart(token::Kind kind) {
   return isDeclarationSpecifier(kind) || kind == token::Kind::STATIC_ASSERT;
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
bool isLabelStmtStart(token::Kind first, token::Kind second) {
   return (first == token::Kind::IDENT && second == token::Kind::COLON) || first == token::Kind::CASE || first == token::Kind::DEFAULT;
}
// ---------------------------------------------------------------------------
bool isComparisonOp(op::Kind op) {
   return op >= op::Kind::LT && op <= op::Kind::NE;
}
// ---------------------------------------------------------------------------
bool isIncDecOp(op::Kind op) {
   return op == op::Kind::PREDEC || op == op::Kind::PREINC || op == op::Kind::POSTDEC || op == op::Kind::POSTINC;
}
// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parse() {
   while (*pos_) {
      std::vector<attr_t> attr = parseOptAttributeSpecifierSequence();
      state = State();
      parseDeclStmt(attr);
      for (auto& [lbl, gotos] : state.incompleteGotos) {
         for (bb_t* bb : gotos) {
            if (state.labels.contains(lbl)) {
               emitJump(bb, state.labels[lbl]);
            } else {
               diagnostics_ << "Undefined label " << lbl << std::endl;
            }
         }
      }
      if (!state.fn || !state.entry) {
         continue;
      }

      // handle gotos and returns and unselaed blocks
      for (auto bb : state.unsealedBlocks) {
         diagnostics_ << "Unsealed block " << bb << std::endl;
      }
      if (state.missingReturns.size() == 0) {
         if (state.retTy.kind() != TYK::VOID) {
            diagnostics_ << "Missing return statement" << std::endl;
         }
         emitter_.emitRet(state.bb, static_cast<ssa_t*>(nullptr));
      } else if (state.missingReturns.size() == 1) {
         emitter_.emitRet(state.missingReturns[0].first, state.missingReturns[0].second);
      } else {
         ssa_t* retVar = emitter_.emitLocalVar(state.fn, state.entry, state.retTy, Ident("__retVar"), true);
         bb_t* retBB = emitter_.emitBB(state.fn, nullptr, Ident("__retBB"));
         for (auto [bb, val] : state.missingReturns) {
            emitter_.emitStore(bb, state.retTy, val, retVar);
            emitter_.emitJump(bb, retBB);
         }
         ssa_t* retVal = emitter_.emitLoad(retBB, state.retTy, retVar, Ident("__retVar"));
         emitter_.emitRet(retBB, retVal);
      }
   }
   for (ScopeInfo* var : missingDefaultInitiations_) {
      const_or_iconst_t c = emitter_.emitConst(var->ty, 0);
      emitter_.setInitValueGlobalVar(var->ssa, c);
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Parser<_EmitterT>::parseParameterList(std::vector<TY>& paramTys, std::vector<Ident>& paramNames) {
   tracer_ << ENTER{"parseParameterList"};
   auto sg = scope_.guard();
   while (isDeclarationSpecifier(pos_->getKind()) || pos_->getKind() == TK::ELLIPSIS) {
      if (consumeAnyOf(TK::ELLIPSIS)) {
         tracer_ << EXIT{"parseParameterList"};
         return true;
      }
      parseOptAttributeSpecifierSequence();
      TY ty = parseSpecifierQualifierList();

      // todo: handle names in casts/abstract declerators
      Declarator decl = parseDeclarator(ty);
      if (decl.ty.kind() == TYK::VOID) {
         if (paramTys.size() > 0) {
            diagnostics_ << "void must be the only parameter" << std::endl;
         }
         if (decl.ident) {
            diagnostics_ << "void must not have a name" << std::endl;
         }
         break;
      }

      if (decl.ty.kind() == TYK::ARRAY_T) {
         TY ty = factory_.ptrTo(decl.ty.getElemTy());
         ty.qualifiers = decl.ty.qualifiers;
         decl.ty = factory_.harden(ty);
      }

      paramTys.push_back(decl.ty);
      paramNames.push_back(decl.ident);

      if (decl.ident && !scope_.insert(decl.ident, ScopeInfo(decl.ty))) {
         diagnostics_ << "Redefinition of parameter " << decl.ident << std::endl;
      }

      tracer_ << "parameter: " << decl.ty << std::endl;
      if (!consumeAnyOf(TK::COMMA)) {
         break;
      }
   }
   tracer_ << EXIT{"parseParameterList"};
   return false;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
int Parser<_EmitterT>::parseOptLabelList(std::vector<attr_t>& attr) {
   int labelCount = 0;
   bb_t* labelTarget = nullptr;
   for (TK kind = pos_->getKind(); isLabelStmtStart(kind, pos_.peek()); kind = pos_->getKind(), ++labelCount) {
      if (!labelTarget) {
         labelTarget = newBB();
         emitJumpIfNotSealed(state.bb, labelTarget);
         state.bb = labelTarget;
      }
      if (consumeAnyOf(TK::CASE)) {
         // case-statement
         expr_t expr = parseExpr();
         if (state.switches.empty()) {
            diagnostics_ << "Case outside of switch" << std::endl;
         }
         if (!expr->ty.isIntegerTy()) {
            diagnostics_ << "Case expression must be of integer type" << std::endl;
         }
         iconst_t* value;
         if (iconst_t** c = std::get_if<iconst_t*>(&expr->value)) {
            value = *c;
         } else {
            diagnostics_ << "Case expression must be a constant" << std::endl;
            continue;
         }
         SwitchState& sw = state.switches.back();
         auto it = std::find(sw.values.begin(), sw.values.end(), value);
         if (it != sw.values.end()) {
            diagnostics_ << "Duplicate case value" << std::endl;
         } else {
            sw.values.push_back(value);
            sw.blocks.push_back(labelTarget);
         }
         emitter_.addSwitchCase(sw.sw, value, labelTarget);
      } else if (consumeAnyOf(TK::DEFAULT)) {
         // default-statement
         if (state.switches.empty()) {
            diagnostics_ << "Default outside of switch" << std::endl;
         }
         SwitchState& sw = state.switches.back();
         auto it = std::find(sw.values.begin(), sw.values.end(), nullptr);
         if (it == sw.values.end()) {
            sw.values.push_back(nullptr);
            sw.blocks.push_back(labelTarget);
         } else {
            diagnostics_ << "Duplicate default case" << std::endl;
            continue;
         }
         emitter_.addSwitchDefault(sw.sw, labelTarget);

      } else {
         // labeled-statement
         Ident label = pos_->getValue<Ident>();
         if (state.labels.contains(label)) {
            diagnostics_ << "Redefinition of label " << label << std::endl;
         }
         state.labels[label] = labelTarget;
         ++pos_;
      }
      expect(TK::COLON);
      attr = parseOptAttributeSpecifierSequence();
   }
   return labelCount;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseStmt() {
   tracer_ << ENTER{"parseStmt"};
   std::vector<attr_t> attr = parseOptAttributeSpecifierSequence();
   parseOptLabelList(attr);
   parseUnlabledStmt(attr);
   tracer_ << EXIT{"parseStmt"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseDeclStmt(const std::vector<attr_t>& attr) {
   // todo: static_assert-declaration

   if (attr.size() > 0 && consumeAnyOf(TK::SEMICOLON)) {
      // todo: (jr) attribute declaration
   } else {
      DeclarationSpecifier declSpec = parseDeclarationSpecifierList();
      bool mayBeFunctionDef = true;
      do {
         Declarator decl = parseDeclarator(declSpec.ty);
         factory_.clearFragments();

         ScopeInfo* info = scope_.find(decl.ident);
         bool canInsert = scope_.canInsert(decl.ident);

         std::string_view what = decl.ty.kind() == TYK::FN_T ? "function " : "variable ";
         if (!canInsert && !scope_.isTopLevel()) {
            diagnostics_ << "Redefinition of " << what << decl.ident << std::endl;
         } else if (scope_.isTopLevel() && info && info->ty != decl.ty) {
            diagnostics_ << "Redecleration of " << what << decl.ident << " with a different type: " << declSpec.ty << " vs " << info->ty << std::endl;
         } else if (!scope_.isTopLevel() && decl.ty.kind() == TYK::FN_T) {
            diagnostics_ << "Nested function definitions not allowed" << std::endl;
         }

         if (decl.ty.kind() == TYK::FN_T) {
            // set function of state, potentially reusing the function if it was already declared
            state.fn = info ? info->fn : emitter_.emitFnProto(decl.ty, decl.ident);
            state.retTy = decl.ty.getRetTy();

            if (mayBeFunctionDef && pos_->getKind() == TK::L_C_BRKT) {
               if (info && info->hasDefOrInit) {
                  diagnostics_ << "Redefinition of function " << decl.ident << std::endl;
               }
               // function body
               parseFunctionDefinition(decl);
            }

            if (canInsert) {
               info = scope_.insert(decl.ident, ScopeInfo(decl.ty, state.fn, !!state.entry));
            }

            if (state.entry) {
               // function definition cannot be in parameter list
               return;
            }

         } else {
            bool isGlobal = scope_.isTopLevel() || declSpec.storageClass[0] == TK::STATIC || declSpec.storageClass[1] == TK::STATIC;

            ssa_t* var;
            if (canInsert) {
               var = isGlobal ?
                   emitter_.emitGlobalVar(decl.ty, decl.ident) :
                   emitter_.emitLocalVar(state.fn, state.entry, decl.ty, decl.ident);

               info = scope_.insert(decl.ident, ScopeInfo(decl.ty, var, isGlobal));
            } else {
               var = info->ssa;
            }

            if (consumeAnyOf(TK::ASSIGN)) {
               if (info->ty.kind() == TYK::FN_T) {
                  diagnostics_ << "Illegal Inizilizer. Only variables can be initialized" << std::endl;
               } else if (scope_.isTopLevel() && !canInsert && info->ssa && !isMissingDefaultInitiation(info->ssa) && !isExternalDeclaration(info->ssa)) {
                  diagnostics_ << "Redefinition of variable " << decl.ident << std::endl;
               }
               value_t value;
               info->hasDefOrInit = true;
               if (consumeAnyOf(TK::L_C_BRKT)) {
                  // initializer-list
                  // todo:
                  parseInitializerList();
                  assert(false && "not implemented");
                  expect(TK::R_C_BRKT);
               } else {
                  expr_t expr = parseAssignmentExpr();
                  // todo: track if constant in expr
                  value = asRVal(expr);
               }
               if (isSealed(state.bb)) {
                  // todo: diagnostics_ << "Variable initialization is unreachable" << std::endl;
               } else if (isGlobal) {
                  emitter_.setInitValueGlobalVar(var, getConst(value));

                  auto it = std::find(missingDefaultInitiations_.begin(), missingDefaultInitiations_.end(), info);
                  if (it != missingDefaultInitiations_.end()) {
                     missingDefaultInitiations_.erase(it);
                  }
                  it = std::find(externDeclarations_.begin(), externDeclarations_.end(), info);
                  if (it != externDeclarations_.end()) {
                     externDeclarations_.erase(it);
                  }

               } else {
                  // todo: cast if necessary
                  emitter_.setInitValueLocalVar(state.fn, decl.ty, var, value);
               }
            } else if (isGlobal) {
               if (declSpec.storageClass[0] == TK::EXTERN) {
                  externDeclarations_.push_back(info);
               } else {
                  missingDefaultInitiations_.push_back(info);
               }
            }
         }
         mayBeFunctionDef = false;
      } while (consumeAnyOf(TK::COMMA));
      expect(TK::SEMICOLON);
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseInitializerList() {
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
   return nullptr;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseFunctionDefinition(Declarator& decl) {
   auto sg = scope_.guard();
   state.entry = state.bb = emitter_.emitFn(state.fn);
   state.unsealedBlocks.push_back(state.bb);
   for (unsigned i = 0; i < decl.ty.getParamTys().size(); ++i) {
      TY paramTy = decl.ty.getParamTys()[i];
      Ident paramName = decl.paramNames[i];
      ssa_t* paramValue = emitter_.getParam(state.fn, i);
      ssa_t* localParamValue = emitter_.emitLocalVar(state.fn, state.entry, paramTy, paramName);
      emitter_.setInitValueLocalVar(state.fn, paramTy, localParamValue, paramValue);
      scope_.insert(paramName, ScopeInfo(paramTy, localParamValue, true));
   }

   // function-definition
   parseCompoundStmt();
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseCompoundStmt() {
   tracer_ << ENTER{"parseCompoundStmt"};
   expect(TK::L_C_BRKT);

   while (pos_->getKind() != TK::R_C_BRKT && pos_->getKind() != TK::END) {
      auto attr = parseOptAttributeSpecifierSequence();
      parseOptLabelList(attr);
      if (isTypeSpecifierQualifier(pos_->getKind())) {
         parseDeclStmt(attr);
      } else {
         parseUnlabledStmt(attr);
      }
   }
   expect(TK::R_C_BRKT);
   tracer_ << EXIT{"parseCompoundStmt"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseUnlabledStmt(const std::vector<attr_t>& attr) {
   TK kind = pos_->getKind();

   if (kind == TK::L_C_BRKT) {
      // todo: attribute-specifier-sequenceopt
      scope_.enter();
      parseCompoundStmt();
      scope_.leave();
   } else if (isSelectionStmtStart(kind)) {
      parseSelectionStmt();
   } else if (isIterationStmtStart(kind)) {
      parseIterationStmt();
   } else if (isJumpStmtStart(kind)) {
      parseJumpStmt();
   } else {
      // expression-statement
      expr_t expr;
      if (kind != TK::SEMICOLON) {
         expr = parseExpr();
      }
      expect(TK::SEMICOLON);
      if (!expr && !attr.empty()) {
         diagnostics_ << "Attribute specifier without expression" << std::endl;
      }
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseSelectionStmt() {
   tracer_ << ENTER{"parseSelectionStmt"};
   bb_t* fromBB = state.bb;
   bb_t* then = nullptr;
   bb_t* thenEnd = nullptr;
   bb_t* otherwise = nullptr;
   expr_t cond{};
   if (consumeAnyOf(TK::IF)) {
      expect(TK::L_BRACE);
      cond = parseConditionExpr();
      expect(TK::R_BRACE);
      state.bb = then = newBB();
      parseStmt();
      thenEnd = state.bb;
      bb_t* cont = nullptr;
      if (consumeAnyOf(TK::ELSE)) {
         state.bb = otherwise = newBB();
         parseStmt();
      }
      cont = newBB();
      otherwise = otherwise ? otherwise : cont;
      emitBranch(fromBB, then, otherwise, cond->value);
      if (cont != otherwise) {
         emitJumpIfNotSealed(state.bb, cont);
      }
      emitJumpIfNotSealed(thenEnd, cont);
      state.bb = cont;
   } else {
      expect(TK::SWITCH);
      expect(TK::L_BRACE);
      cond = parseExpr();
      expect(TK::R_BRACE);
      if (!cond->ty.isIntegerTy()) {
         diagnostics_ << "Switch condition must have integer type" << std::endl;
         cond->value = emitter_.emitPoison();
      }
      state.switches.emplace_back(emitter_.emitSwitch(state.bb, asRVal(cond)));
      markSealed(state.bb);
      parseStmt();
   }
   tracer_ << EXIT{"parseSelectionStmt"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseIterationStmt() {
   tracer_ << ENTER{"parseIterationStmt"};
   bool forLoop = false;
   bool doWhile = false;
   bb_t* fromBB = state.bb;
   std::array<bb_t*, 3> bb{nullptr};
   expr_t cond = nullptr;

   if (consumeAnyOf(TK::DO)) {
      doWhile = true;
   } else if (consumeAnyOf(TK::FOR)) {
      forLoop = true;
      scope_.enter();
      expect(TK::L_BRACE);
      std::vector<attr_t> attr{parseOptAttributeSpecifierSequence()};
      if (isDeclarationStart(pos_->getKind())) {
         parseDeclStmt(attr);
      } else if (pos_->getKind() != TK::SEMICOLON) {
         parseExpr();
      }
      if (pos_->getKind() != TK::SEMICOLON) {
         state.bb = bb[0] = newBB();
         cond = parseConditionExpr();
      }
      expect(TK::SEMICOLON);
      if (pos_->getKind() != TK::R_BRACE) {
         bb[1] = newBB();
         state.bb = bb[2] = newBB();
         parseExpr();
      }
      expect(TK::R_BRACE);
   } else {
   parse_while:
      expect(TK::WHILE);
      expect(TK::L_BRACE);
      state.bb = bb[0] = newBB();
      cond = parseConditionExpr();
      expect(TK::R_BRACE);
      if (doWhile) {
         goto end_parse_body;
      }
   }
   if (!bb[1]) {
      bb[1] = newBB();
   }
   state.bb = bb[1];
   parseStmt();
   if (doWhile) {
      goto parse_while;
   } else if (forLoop && bb[2]) {
      emitJump(state.bb, bb[2]);
   }
end_parse_body:
   if (doWhile) {
      expect(TK::SEMICOLON);
   }
   state.bb = newBB();
   // set last BB to body or update expression (for loop)
   bb[2] = bb[2] ? bb[2] : bb[1];

   emitJump(fromBB, doWhile ? bb[1] : bb[0]);
   emitJump(bb[2], bb[0] ? bb[0] : bb[1]);
   if (cond) {
      emitBranch(bb[0], state.bb, bb[1], cond->value);
   }
   if (forLoop) {
      scope_.leave();
   }
   tracer_ << EXIT{"parseIterationStmt"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseJumpStmt() {
   tracer_ << ENTER{"parseJumpStmt"};
   if (consumeAnyOf(TK::GOTO)) {
      Ident label = expect(TK::IDENT).template getValue<Ident>();
      expect(TK::SEMICOLON);
      if (isSealed(state.bb)) {
         tracer_ << EXIT{"parseJumpStmt"};
         return;
      }
      if (state.labels.contains(label)) {
         emitter_.emitJump(state.bb, state.labels[label]);
      } else {
         state.incompleteGotos[label].push_back(state.bb);
      }
      markSealed(state.bb);
   } else if (consumeAnyOf(TK::CONTINUE)) {
      assert(false && "not implemented");
   } else if (consumeAnyOf(TK::BREAK)) {
      assert(false && "not implemented");
   } else if (consumeAnyOf(TK::RETURN)) {
      value_t value;
      expr_t expr;
      if (pos_->getKind() != TK::SEMICOLON) {
         expr = parseExpr();
         value = asRVal(expr);
      }
      expect(TK::SEMICOLON);
      if (isSealed(state.bb)) {
         tracer_ << EXIT{"parseJumpStmt"};
         return;
      }
      value = cast(expr->ty, state.retTy, value);
      state.missingReturns.push_back({state.bb, value});
      markSealed(state.bb);
   } else {
      assert(false && "not implemented");
   }
   tracer_ << EXIT{"parseJumpStmt"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseConditionExpr() {
   expr_t cond = parseExpr();
   cond->value = isTruethy(cond);
   return cond;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseExpr(short midPrec) {
   expr_t lhs = parsePrimaryExpr();
   while (*pos_ && pos_->getKind() >= token::Kind::ASTERISK && pos_->getKind() <= token::Kind::QMARK) {
      op::OpSpec spec{op::getOpSpec(*pos_)};

      if (midPrec && spec.precedence >= midPrec) {
         break;
      }

      TK tk = pos_->getKind();
      op::Kind kind;
      value_t result;
      expr_t subscript;
      switch (tk) {
         TK next;
         case TK::END:
         case TK::R_BRACE:
         case TK::R_BRACKET:
            return lhs;
         case TK::INC:
         case TK::DEC:
            kind = tk == TK::INC ? op::Kind::POSTINC : op::Kind::POSTDEC;
            if (lhs->mayBeLval()) {
               // todo: checl if ssa_t is actually there
               result = emitter_.emitIncDecOp(state.bb, lhs->ty, kind, std::get<ssa_t*>(lhs->value));
               lhs = std::make_unique<Expr>(kind, lhs->ty, std::move(lhs), result);
            } else {
               diagnostics_ << "lvalue required as increment/decrement operand" << std::endl;
            }
            ++pos_;
            continue;
         case TK::L_BRACKET:
            ++pos_;
            if (lhs->ty.kind() != TYK::PTR_T) {
               diagnostics_ << "subscripted value must be pointer type, not " << lhs->ty << std::endl;
            }
            subscript = parseExpr();
            expect(TK::R_BRACKET);
            if (!subscript->ty.isIntegerTy()) {
               diagnostics_ << "subscript must be an integer, not " << subscript->ty << std::endl;
            }
            result = emitter_.emitGEP(state.bb, lhs->ty, asRVal(lhs), asRVal(subscript));
            lhs = std::make_unique<Expr>(kind, lhs->ty.getPointedToTy(), std::move(lhs), std::move(subscript), result, true);
            continue;
         case TK::DEREF:
         case TK::PERIOD:
            ++pos_;
            assert(pos_->getKind() == TK::IDENT && "member access"); // todo: (jr) not implemented
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
               TY fnTy{};
               if (lhs->ty.kind() == TYK::FN_T) {
                  fnTy = lhs->ty;
               } else if (lhs->ty.kind() == TYK::PTR_T && lhs->ty.getPointedToTy().kind() == TYK::FN_T) {
                  fnTy = lhs->ty.getPointedToTy();
               } else {
                  diagnostics_ << "called object is not a function or function pointer" << std::endl;
               }
               std::vector<value_t> args;
               while (pos_->getKind() != TK::R_BRACE) {
                  if (args.size() > 0) {
                     expect(TK::COMMA);
                  }
                  expr_t expr = parseAssignmentExpr();
                  // todo: type cast
                  value_t arg = asRVal(expr);
                  if (fnTy && args.size() < fnTy.getParamTys().size()) {
                     arg = cast(expr->ty, fnTy.getParamTys()[args.size()], arg);
                  } else if (fnTy && fnTy.isVarArg()) {
                     arg = cast(expr->ty, factory_.promote(expr->ty), arg);
                  } else {
                     diagnostics_ << "too many arguments" << std::endl;
                  }
                  args.push_back(arg);
               }
               if (args.size() < fnTy.getParamTys().size()) {
                  diagnostics_ << "too few arguments" << std::endl;
               }
               // todo: (jr) not implemented
               expect(TK::R_BRACE);
               ssa_t* ssa = nullptr;
               TY retTy;
               if (fnTy && lhs->ty.kind() == TYK::FN_T) {
                  // call directly
                  ssa = emitter_.emitCall(state.bb, static_cast<fn_t*>(std::get<ssa_t*>(lhs->value)), args);
               } else if (fnTy) {
                  // call with function pointer and type
                  ssa = emitter_.emitCall(state.bb, fnTy, std::get<ssa_t*>(lhs->value), args);
               }

               lhs = std::make_unique<Expr>(op::Kind::CALL, fnTy.getRetTy(), std::move(lhs), ssa);
               continue;
            }
         default:
            // binary operator
            ++pos_;
      }

      kind = op::getBinOpKind(tk);

      expr_t rhs = parseExpr(spec.precedence + !spec.leftAssociative);

      TY resTy = factory_.commonRealType(lhs->ty, rhs->ty);
      // todo: delete this, because it is not needed (however, clang does it, so it makes testing easier)
      // if (kind >= op::Kind::ASSIGN && kind <= op::Kind::BW_XOR_ASSIGN) {
      //    std::swap(lhs, rhs);
      // }
      value_t lhsValue = cast(lhs->ty, resTy, asRVal(lhs));
      value_t rhsValue = cast(rhs->ty, resTy, asRVal(rhs));

      // todo: delete this, because it is not needed (however, clang does it, so it makes testing easier)
      // if (kind >= op::Kind::ASSIGN && kind <= op::Kind::BW_XOR_ASSIGN) {
      //    std::swap(lhs, rhs);
      //    std::swap(lhsValue, rhsValue);
      // }

      // todo: check if constexpr should be emitted
      // todo: change type of expr depending on the operation
      {
         const_t** l = std::get_if<const_t*>(&lhsValue);
         const_t** r = std::get_if<const_t*>(&rhsValue);
         if (l && r && kind >= op::Kind::ASSIGN && kind <= op::Kind::BW_XOR_ASSIGN) {
            result = asValue(emitter_.emitConstBinOp(state.bb, resTy, kind, *l, *r));
         } else {
            ssa_t** lval = std::get_if<ssa_t*>(&lhs->value);
            result = emitter_.emitBinOp(state.bb, resTy, kind, lhsValue, rhsValue, lval ? *lval : nullptr);
         }
      }
      if (isComparisonOp(kind)) {
         resTy = factory_.boolTy();
      }
      lhs = std::make_unique<Expr>(kind, resTy, std::move(lhs), std::move(rhs), result);
   }
   return lhs;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parsePrimaryExpr() {
   expr_t expr{};
   op::Kind kind;
   Token t = *pos_;

   TY ty;
   Ident name;
   ScopeInfo* info;
   value_t value;
   TK tk = t.getKind();
   if (tk >= TK::ICONST && tk <= TK::LDCONST) {
      // Constant Token
      ++pos_;
      ty = factory_.fromToken(t);
      value = asValue(emitter_.emitConst(ty, t.getValue<int>()));
      return std::make_unique<Expr>(t.getLoc(), ty, value);
   } else if (tk == TK::IDENT) {
      // Identifier
      name = t.getValue<Ident>();
      info = scope_.find(name);
      if (info) {
         value = info->ssa;
         ty = info->ty;
      } else {
         diagnostics_ << t.getLoc() << "Undeclared identifier '" << name << "'" << std::endl;
         value = emitter_.emitPoison();
         ty = factory_.undefTy();
      }
      ++pos_;
      return std::make_unique<Expr>(t.getLoc(), ty, value, name);
   } else if (consumeAnyOf(TK::L_BRACE)) {
      // brace-enclosed expression or type cast
      if (isTypeSpecifierQualifier(pos_->getKind())) {
         // type cast
         TY ty = parseTypeName();
         expect(TK::R_BRACE);
         expr_t operand = parseExpr(2);
         value = cast(operand->ty, ty, asRVal(operand));
         return std::make_unique<Expr>(op::Kind::CAST, ty, std::move(operand), value);
      } else {
         // expression
         expr = parseExpr();
         expr->setPrec(0);
         expect(TK::R_BRACE);
         return expr;
      }
   } else if (consumeAnyOf(TK::SIZEOF)) {
      // sizeof operator
      bool brace = consumeAnyOf(TK::L_BRACE);
      if (brace && isTypeSpecifierQualifier(pos_->getKind())) {
         ty = parseTypeName();
      } else {
         expr = parseUnaryExpr();
         ty = expr->ty;
      }
      if (brace) {
         expect(TK::R_BRACE);
      }
      return std::make_unique<Expr>(pos_->getLoc(), factory_.sizetTy(), emitter_.sizeOf(ty));
   }

   // unary prefix operator
   ++pos_;
   expr_t operand = parseExpr(2);
   // todo: (jr) not very beautiful
   switch (tk) {
      case TK::INC:
      case TK::DEC:
         kind = (t.getKind() == TK::INC) ? op::Kind::PREINC : op::Kind::PREDEC;
         // todo: (jr) check for lvalue
         value = emitter_.emitIncDecOp(state.bb, operand->ty, kind, std::get<ssa_t*>(operand->value));
         ty = operand->ty;
         break;
      case TK::PLUS:
         ty = factory_.promote(operand->ty);
         value = cast(operand->ty, ty, asRVal(operand));
         break;
      case TK::MINUS:
         ty = factory_.promote(operand->ty);
         value = cast(operand->ty, ty, asRVal(operand));
         value = emitNeg(ty, value);
         break;
      case TK::NEG:
         ty = factory_.boolTy();
         value = isTruethy(operand);
         value = emitNeg(ty, value);
         break;
      case TK::BW_INV:
         ty = factory_.promote(operand->ty);
         value = cast(operand->ty, ty, asRVal(operand));
         value = emitBWNeg(ty, value);
         break;
      case TK::ASTERISK:
         kind = op::Kind::DEREF;
         if (operand->ty.kind() == TYK::PTR_T) {
            value = emitter_.emitLoad(state.bb, operand->ty.getPointedToTy(), std::get<ssa_t*>(asRVal(operand)));
         } else {
            value = emitter_.emitPoison();
            diagnostics_ << operand->loc << "Expected pointer" << std::endl;
         }
         ty = operand->ty.getPointedToTy();
         break;
      case TK::BW_AND:
         kind = op::Kind::ADDROF;
         if (operand->mayBeLval()) {
            value = operand->value;
         } else {
            value = emitter_.emitPoison();
            diagnostics_ << operand->loc << "Expected lvalue" << std::endl;
         }
         ty = factory_.ptrTo(operand->ty);
         ty = factory_.harden(ty);
         factory_.clearFragments();
         break;
      case TK::ALIGNOF:
      case TK::SLITERAL:
      case TK::WB_ICONST:
      case TK::UWB_ICONST:
         // todo: (jr) generic selection
         // todo: (jr) is this also a string? case TK::CLITERAL:
         assert(false && "Not implemented"); // todo: (jr) not implemented
      default:
         diagnostics_ << "Unexpected token for primary expression: " << t.getKind() << std::endl;
         return expr;
   }
   // todo: type
   expr = std::make_unique<Expr>(kind, ty, std::move(operand), value);
   return expr;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::Declarator Parser<_EmitterT>::parseDeclarator(TY specifierQualifier) {
   tracer_ << ENTER{"parseDeclarator"};
   Declarator decl = parseDeclaratorImpl();
   if (decl.ty) {
      *decl.base = specifierQualifier;
      // now, there might be duplicate types
   } else {
      decl.ty = specifierQualifier;
   }

   decl.ty = factory_.harden(decl.ty);
   decl.base = DeclTypeBaseRef{};
   tracer_ << EXIT{"parseDeclarator"};
   return decl;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::TY Parser<_EmitterT>::parseAbstractDeclarator(TY specifierQualifier) {
   // todo: (jr) dublicate code
   // todo: remove dublicate types
   tracer_ << ENTER{"parseAbstractDeclarator"};
   Declarator decl = parseDeclarator(specifierQualifier);

   if (decl.ident) {
      diagnostics_ << "Identifier '" << decl.ident << "' not allowed in abstract declarator" << std::endl;
   }

   tracer_ << EXIT{"parseAbstractDeclarator"};
   return decl.ty;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::Declarator Parser<_EmitterT>::parseDirectDeclarator(TY specifierQualifier) {
   tracer_ << ENTER{"parseDirectDeclarator"};
   SrcLoc loc = pos_->getLoc();

   Declarator decl{parseDeclarator(specifierQualifier)};

   loc |= pos_->getLoc();
   if (!decl.ident) {
      diagnostics_ << loc << "Expected identifier in declaration" << std::endl;
   }
   tracer_ << EXIT{"parseDirectDeclarator"};
   return decl;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::Declarator Parser<_EmitterT>::parseDeclaratorImpl() {
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
         value_t size;

         static_ = parseOpt(TK::STATIC);

         // todo: (jr) type-qualifier-list only for non abstract declarator
         bool qualifiers[4] = {false, false, false, false};
         bool qualified = false;
         while (isTypeQualifier(pos_->getKind())) {
            qualified = true;
            qualifiers[static_cast<int>(pos_->getKind()) - static_cast<int>(TK::CONST)] = true;
            ++pos_;
         }
         if (parseOpt(TK::STATIC)) {
            if (static_) {
               diagnostics_ << "static used multiple times" << std::endl;
            }
            static_ = true;
         }

         if (parseOpt(TK::ASTERISK)) {
            unspecSize = true;
            if (static_) {
               diagnostics_ << "static and * used together" << std::endl;
            }
            if (!decl.ident && qualified) {
               diagnostics_ << "Type qualifiers not allowed in abstract declarator" << std::endl;
            }
         } else {
            if (pos_->getKind() != TK::R_BRACKET) {
               expr_t expr = parseAssignmentExpr();
               if (!expr->ty.isIntegerTy()) {
                  diagnostics_ << "Array size must be an integer type" << std::endl;
               }
               size = asRVal(expr);
            } else if (static_) {
               diagnostics_ << "Static array must have a size" << std::endl;
            }
         }

         // todo: unspecSize, varLen
         // todo: static
         TY arrTy;
         if (iconst_t** s = std::get_if<iconst_t*>(&size); s && !unspecSize) {
            arrTy = factory_.arrayOf({}, *s);
         } else if (ssa_t** s = std::get_if<ssa_t*>(&size); s && !unspecSize) {
            arrTy = factory_.arrayOf({}, *s);
         } else {
            if (!unspecSize) {
               diagnostics_ << "Array size must be a constant integer expression" << std::endl;
            }
            arrTy = factory_.arrayOf({});
         }
         rhsBase.chain(arrTy);
         if (!rhsTy) {
            rhsTy = arrTy;
         }

         expect(TK::R_BRACKET);
      } else {
         // function-declarator
         // todo: parameter-type-listopt

         std::vector<TY> paramTys{};
         bool varargs = parseParameterList(paramTys, decl.paramNames);
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

   DeclarationSpecifier declSpec{};
   TY& ty{declSpec.ty};
   Token ident;

   TokenCounter<TK::BOOL, TK::UNSIGNED> tycount{};
   auto& scSpec{declSpec.storageClass};

   bool qualifiers[3] = {false, false, false};

   for (Token t = *pos_; isDeclarationSpecifier(t.getKind()); t = *++pos_) {
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

      if (type >= TK::BOOL && type <= TK::UNSIGNED) {
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
      constexpr std::array<TK, 6> singleTypeSpecifier = {TK::FLOAT, TK::DECIMAL32, TK::DECIMAL64, TK::DECIMAL128, TK::COMPLEX, TK::BOOL};
      for (TK t : singleTypeSpecifier) {
         if (tycount.consume(t)) {
            TYK v = static_cast<TYK>(static_cast<int>(t) - static_cast<int>(TK::BOOL) + static_cast<int>(TYK::BOOL));
            ty = factory_.realTy(v);
            break;
         }
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
         if (!signed_ && !unsigned_) {
            ty = factory_.charTy();
         } else {
            ty = factory_.integralTy(TYK::CHAR, unsigned_);
         }
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
   auto attr = parseOptAttributeSpecifierSequence();
   TY declTy = parseAbstractDeclarator(ty);

   tracer_ << EXIT{"parseTypeName"};
   return declTy;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_PARSER_H
