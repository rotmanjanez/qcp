#ifndef QCP_PARSER_H
#define QCP_PARSER_H
// ---------------------------------------------------------------------------
// used to collapse multiple functions at once in the IDE
#define __IDE_MARKER
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "diagnostics.h"
#include "emittertraits.h"
#include "expr.h"
#include "scope.h"
#include "scopeinfo.h"
#include "token.h"
#include "tokencounter.h"
#include "tokenizer.h"
#include "tracer.h"
#include "typefactory.h"
// ---------------------------------------------------------------------------
#include <iomanip>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
// ---------------------------------------------------------------------------
namespace qcp {
// ---------------------------------------------------------------------------
template <typename T>
class Parser {
   using trait = emitter::emitter_traits<T>;
   using Token = token::Token;
   using TK = token::Kind;

   using TYK = type::Kind;
   using Type = type::Type<T>;
   using Factory = type::TypeFactory<T>;
   using DeclTypeBaseRef = typename Factory::DeclTypeBaseRef;

   using OpSpec = op::OpSpec;

   using attr_t = Token;

   using bb_t = typename trait::bb_t;
   using fn_t = typename trait::fn_t;
   using ssa_t = typename trait::ssa_t;
   using sw_t = typename trait::sw_t;
   using const_t = typename trait::const_t;
   using iconst_t = typename trait::iconst_t;
   using const_or_iconst_t = typename trait::const_or_iconst_t;
   using phi_t = typename trait::phi_t;

   using ScopeInfo = scope::ScopeInfo<T>;
   using expr_t = std::unique_ptr<Expr<T>>;

   using value_t = typename T::value_t;

   public:
   Parser(std::string_view prog,
          DiagnosticTracker &diagnostics,
          std::ostream &logStream = std::cout) : tokenizer_{prog, diagnostics},
                                                 pos_{tokenizer_.begin()},
                                                 diagnostics_{diagnostics},
                                                 tracer_{logStream},
                                                 factory_{emitter_} {}

   void addIntTypeDef(const std::string &name) {
      typedefScope_.insert(Ident{name}, locatable<Type>{{}, factory_.intTy()});
   }

   struct SwitchState {
      SwitchState(sw_t *sw) : sw{sw} {}

      sw_t *sw;
      std::vector<iconst_t *> values{};
      std::vector<bb_t *> blocks{};
   };

   struct State {
      bool eval = true;
      Ident fnName;
      fn_t *fn = nullptr;
      bb_t *entry = nullptr;
      bb_t *bb = nullptr;
      Type retTy{};

      ssa_t *_func_ = nullptr;

      // function specifier can only appear at the top level, so we can put it here instead of the Declerator struct
      std::array<bool, 2> functionSpec{false, false};

      bool &inlineFn() {
         return functionSpec[0];
      }

      bool &noreturnFn() {
         return functionSpec[1];
      }

      bb_t *shortCircuitBB = nullptr;
      std::vector<locatable<bb_t *>> unsealedBlocks{};
      std::unordered_map<Ident, bb_t *> labels{};
      std::unordered_map<Ident, std::vector<locatable<bb_t *>>> incompleteGotos{};
      std::vector<std::pair<bb_t *, value_t>> outstandingReturns{};
      std::vector<std::vector<bb_t *>> missingBreaks{};
      std::vector<bb_t *> continueTargets{};
      std::vector<SwitchState> switches{};
   } state;

   void markSealed(bb_t *bb);

   bool isSealed(bb_t *bb);

   void completeBreaks(bb_t *target);

   T &getEmitter() {
      return emitter_;
   }

   void parse();

   struct Declarator {
      Type ty;
      DeclTypeBaseRef base;
      Ident ident;
      SrcLoc nameLoc;
      std::vector<std::pair<Ident, SrcLoc>> paramNames{};
   };

   struct DeclarationSpecifier {
      std::array<TK, 2> storageClass{TK::END, TK::END};
      Type ty;
   };

   // statements
   void parseStmt();
   void parseCompoundStmt();
   void parseUnlabledStmt(const std::vector<attr_t> &attr);
   void parseSelectionStmt();
   void parseIterationStmt();
   void parseJumpStmt();
   void parseDeclStmt(const std::vector<attr_t> &attr);

   void parseFunctionDefinition(Declarator &decl);

   // Expressions
   expr_t parseExpr(short minPrec = 0);
   expr_t parsePostfixExpr(expr_t &&lhs);
   expr_t parseConditionExpr();
   expr_t parseConditionalExpr();
   expr_t parseAssignmentExpr();
   expr_t parseUnaryExpr();
   expr_t parsePrimaryExpr();

   Type parseTypeName();
   Type parseSpecifierQualifierList();

   DeclarationSpecifier parseDeclarationSpecifierList(bool storageClassSpecifiers = true, bool functionSpecifiers = true);

   private:
   int parseOptLabelList(std::vector<attr_t> &attr);
   void parseMemberDeclaratorList(Type specifierQualifier, std::vector<tagged<Type>> &members, std::vector<SrcLoc> &locs);
   std::vector<attr_t> parseOptAttributeSpecifierSequence();

   Type parseAbstractDeclarator(Type specifierQualifier);
   Declarator parseDirectDeclarator(Type specifierQualifier);
   // todo: make move
   Declarator parseDeclarator(Type specifierQualifier);
   Declarator parseMemberDeclarator(Type specifierQualifier);
   // todo: make move
   // todo: make move
   Declarator parseDeclaratorImpl();

   bool parseParameterList(std::vector<Type> &paramTys, std::vector<std::pair<Ident, SrcLoc>> &paramNames);

   // operations on values
   const_or_iconst_t getConst(value_t value) const;
   value_t asValue(const_or_iconst_t c) const;
   value_t cast(SrcLoc loc, Type from, Type to, value_t value, bool explicitCast = false);

   // operations on expressions
   value_t cast(expr_t &expr, Type to, bool explicitCast = false);
   value_t asBoolByComparison(expr_t &expr, op::Kind op);
   value_t isTruethy(expr_t &expr);
   value_t asRVal(expr_t &expr);

   inline bb_t *newBB() {
      bb_t *bb = emitter_.emitBB(state.fn);
      if (!state.unsealedBlocks.empty()) {
         state.unsealedBlocks.back().loc() |= pos_->getLoc();
      }
      state.unsealedBlocks.emplace_back(bb, pos_->getLoc());
      return bb;
   }

   void emitJump(bb_t *fromBB, bb_t *toBB) {
      emitter_.emitJump(fromBB, toBB);
      markSealed(fromBB);
   }

   void emitBranch(bb_t *fromBB, bb_t *trueBB, bb_t *falseBB, value_t cond) {
      emitter_.emitBranch(fromBB, trueBB, falseBB, cond);
      markSealed(fromBB);
   }

   void emitJumpIfNotSealed(bb_t *fromBB, bb_t *toBB) {
      if (!toBB || !fromBB || isSealed(fromBB)) {
         return;
      }
      emitJump(fromBB, toBB);
   }

   template <typename... Tokens>
   bool hasAnyOf(Tokens... tokens) {
      static_assert(sizeof...(Tokens) > 0, "consumeAnyOf() must have at least one argument");
      static_assert((std::is_same_v<TK, Tokens> && ...), "all arguments must be of type TK");
      return (pos_ && ((pos_->getKind() == tokens) || ...));
   }

   template <typename... Tokens>
   Token consumeAnyOf(Tokens... tokens) {
      if (hasAnyOf(tokens...)) {
         Token t{*pos_};
         advance();
         return t;
      }
      return Token{};
   }

   // todo: remove
   void not_implemented() {
      emitter_.dumpToStdout();
      std::cerr << DiagnosticMessage(diagnostics_, "not implemented\n", pos_->getLoc(), DiagnosticMessage::Kind::NOTE) << std::endl;
      assert(false && "not implemented");
   }

   bool consumeOpt(TK kind) {
      if (hasAnyOf(kind)) {
         advance();
         return true;
      }
      return false;
   }

   template <std::size_t N>
   Token expect(TK kind, const char *where = nullptr, const char *what = nullptr, std::array<TK, N> likelyNext = {}, bool usePrevTokenEndLoc = true);
   Token expect(TK kind, const char *where = nullptr, const char *what = nullptr, bool usePrevTokenEndLoc = true);

   // emit folded constant expression or operation depending on the type of value
   template <typename cFn, typename Fn, typename... Args>
   value_t emitUnOpImpl(cFn __cFn, Fn __fn, Type ty, value_t value, Args... args);
   value_t emitNeg(Type ty, value_t value);
   value_t emitBWNeg(Type ty, value_t value);
   value_t emitNot(Type ty, value_t value);
   value_t emitCast(Type fromTy, value_t value, Type toTy, qcp::type::Cast cast);

   value_t castAssignmentTarget(SrcLoc opLoc, Type lTy, expr_t &rhs) {
      optArrToPtrDecay(rhs);
      if (rhs->ty && rhs->ty->isFnTy()) {
         rhs->value = emitter_.emitFnPtr(rhs->ty, std::get<fn_t *>(rhs->value));
         rhs->ty = factory_.harden(factory_.ptrTo(rhs->ty));
      }
      Type rTy = rhs->ty;
      if (!lTy || !rTy) {
         errorInvalidOpToBinaryExpr(opLoc, lTy, rTy);
      } else if (lTy->isArithmeticTy() && rTy->isArithmeticTy()) {
         return cast(rhs, lTy);
      } else if (lTy->isPointerTy() && rTy->isPointerTy()) {
         // compares base types and therefore does not check for qualifiers
         Type lPointee = lTy->getPointedToTy();
         Type rPointee = rTy->getPointedToTy();
         // todo: check for nullptr_t and null pointer constant
         if (!lPointee->isCompatibleWith(*rPointee) && !(rPointee->isVoidTy() || lPointee->isVoidTy())) {
            diagnostics_ << opLoc << "incompatible pointer types assigning to '" << lTy << "' from '" << rTy << '\'' << std::endl;
         } else if (lPointee.qualifiers < rPointee.qualifiers) {
            diagnostics_ << opLoc << "discards qualifiers from pointer target type ('" << rTy << "' to '" << lTy << "')" << std::endl;
         } else {
            return asRVal(rhs);
         }
      } else if (lTy->isBoolTy() && rTy->isPointerTy()) {
         return isTruethy(rhs);
      } else if (lTy->isStructTy() && rTy->isStructTy() && lTy->isCompatibleWith(*rTy)) {
         // todo: composite type here?
         return rhs->value;
      } else {
         errorInvalidOpToBinaryExpr(opLoc, lTy, rTy);
      }
      return {};
   }

   void emitAssignment(SrcLoc opLoc, Type lTy, ssa_t *lValue, expr_t &rhs) {
      if (!state.eval) {
         return;
      }
      if (lTy->isStructTy()) {
         // assign each member
         for (auto it = lTy.membersBegin(); it != lTy.membersEnd(); ++it) {
            std::span<const std::uint64_t> GEPValues{it.GEPDerefValues()};
            // no deref needed for struct members
            GEPValues = GEPValues.subspan(1);
            expr_t rhsVal = std::make_unique<Expr<T>>(rhs->loc, *it, emitter_.emitGEP(state.bb, lTy, rhs->value, GEPValues));
            ssa_t *lValue = emitter_.emitGEP(state.bb, lTy, std::get<ssa_t *>(rhs->value), GEPValues);
            emitAssignment(opLoc, *it, lValue, rhsVal);
         }
      } else {
         value_t rVal = castAssignmentTarget(opLoc, lTy, rhs);
         if (!std::holds_alternative<std::monostate>(rVal)) {
            emitter_.emitStore(state.bb, lTy, asRVal(rhs), lValue);
         }
      }
   }

   void emitAssignment(SrcLoc opLoc, expr_t &lhs, expr_t &rhs) {
      if (ssa_t **lValue = std::get_if<ssa_t *>(&lhs->value)) {
         emitAssignment(opLoc, lhs->ty, *lValue, rhs);
      } else {
         diagnostics_ << opLoc << "lvalue required as left operand of assignment" << std::endl;
      }
   }

   Type arrToPtrDecay(Type ty) {
      if (ty->isArrayTy()) {
         ty = factory_.ptrTo(ty->getElementTy());
         ty = factory_.harden(ty);
         return ty;
      }
      return ty;
   }

   value_t arrToPtrDecay(const expr_t &expr) {
      assert(expr->ty->isArrayTy() && "expr must have array type");
      if (!state.eval) {
         return {};
      }
      value_t value = expr->value;
      ssa_t *ptr;
      if (const_t **c = std::get_if<const_t *>(&value)) {
         ptr = emitter_.emitGlobalVar(expr->ty, Ident());
         emitter_.setInitValueGlobalVar(ptr, *c);
      } else {
         ptr = std::get<ssa_t *>(value);
      }
      std::array<std::uint32_t, 2> indices{0, 0};
      return emitter_.emitGEP(state.bb, expr->ty, ptr, indices);
   }

   void optArrToPtrDecay(expr_t &expr) {
      if (!expr->ty) {
         return;
      }
      if (expr->ty->isArrayTy()) {
         expr->value = arrToPtrDecay(expr);
         expr->ty = arrToPtrDecay(expr->ty);
         expr->mayBeLval = false;
      }
   }

   // Error messages
   void notePrevWhatHere(const char *what, SrcLoc loc);
   void notePrevDeclHere(ScopeInfo *info);
   void notePrevDefHere(SrcLoc loc);
   void errorVarIncompleteType(SrcLoc loc, Type ty);
   void noteForwardDeclHere(SrcLoc loc, Type ty) {
      diagnostics_ << loc.truncate(0) << "forward declaration of '" << ty << '\'' << std::endl;
   }
   void errorAssignToConst(SrcLoc loc, Type ty, Ident ident);
   void noteConstDeclHere(SrcLoc loc, Ident ident);
   void errorRedef(SrcLoc loc, Ident ident, Type aTy = Type(), Type bTy = Type()) {
      diagnostics_ << loc.truncate(0) << "redefinition of '" << ident << '\'';
      if (aTy && bTy && aTy != bTy) {
         diagnostics_ << " with a different type: '" << aTy << "' vs '" << bTy << "'";
      }
      diagnostics_ << std::endl;
   }
   void errorInvalidOpToBinaryExpr(SrcLoc loc, Type lTy, Type rty) {
      diagnostics_ << loc << "invalid operands to binary expression ('" << lTy << "' and '" << rty << "')" << std::endl;
   }
   void errorInvalidOpToBinaryExpr(SrcLoc loc, expr_t &lhs, expr_t &rhs) {
      errorInvalidOpToBinaryExpr(loc, lhs->ty, rhs->ty);
   }

   template <bool returnCompoundLiteral>
   inline value_t parseInizializerImpl(Type root, typename Type::const_iterator &tyIt, ssa_t *var) {
      value_t val{};
      if (consumeAnyOf(TK::L_C_BRKT)) {
         if (consumeAnyOf(TK::R_C_BRKT)) {
            // default initialization
            val = asValue(emitter_.emitZeroConst(root));
         } else {
            val = parseInizializerListImpl<returnCompoundLiteral>(root, tyIt, var);
            expect(TK::R_C_BRKT);
         }
      } else {
         expr_t expr = parseAssignmentExpr();
         // todo: check if expr is a constant expression
         val = castAssignmentTarget(pos_->getLoc(), *tyIt, expr);
      }
      if (!returnCompoundLiteral && state.eval) {
         std::span<const std::uint64_t> gepValsSpan{tyIt.GEPDerefValues()};
         ssa_t *elemPtr = var;
         if (gepValsSpan.size() > 2) {
            elemPtr = emitter_.emitGEP(state.bb, root, var, gepValsSpan.subspan(1));
         }
         // todo: posloc not correct here
         if (!std::holds_alternative<std::monostate>(val)) {
            emitter_.emitStore(state.bb, *tyIt, val, elemPtr);
         }
      }
      if (returnCompoundLiteral) {
         if (std::holds_alternative<std::monostate>(val)) {
            diagnostics_ << pos_->getLoc() << "initializer element not present" << std::endl;
         } else if (!state.bb && std::holds_alternative<ssa_t *>(val)) {
            diagnostics_ << pos_->getLoc() << "initializer element is not a compile-time constant" << std::endl;
         } else {
            return val;
         }
      }
      return {};
   }

   template <bool returnCompoundLiteral>
   inline value_t parseInizializerListImpl(Type root, typename Type::const_iterator &tyIt, ssa_t *var) {
      assert(returnCompoundLiteral ^ !!var);
      tyIt.enter();
      std::vector<std::pair<tagged<Type> *, const_or_iconst_t>> values{};
      if ((*tyIt)->isStructTy() || (*tyIt)->isUnionTy()) {
         values.reserve((*tyIt)->getMembers().size());
      } else if ((*tyIt)->isArrayTy() && (*tyIt)->isFixedSizeArrayTy()) {
         values.reserve((*tyIt)->getArraySize());
      }
      do {
         if (hasAnyOf(TK::PERIOD, TK::L_BRACKET)) {
            tyIt = parseDesigtion(root);
         }

         value_t val = parseInizializerImpl<returnCompoundLiteral>(root, tyIt, var);
         // values.emplace_back(&*tyIt, val);
         ++tyIt;
         if (hasAnyOf(TK::R_C_BRKT)) {
            break;
         }
         expect(TK::COMMA);
      } while (true);
      // todo: default init missing values, multi level designation
      tyIt.leave();
      if constexpr (returnCompoundLiteral) {
         // if ((*tyIt)->isStructTy()) {
         //    return emitter_.emitStructConst(*tyIt, values);
         // } else if ((*tyIt)->isArrayTy()) {
         //    return emitter_.emitArrayConst(*tyIt, values);
         // } else if ((*tyIt)->isUnionTy()) {
         //    return values.back().second;
         // } else {
         not_implemented();
         // }
      }
      return {};
   }

   typename Type::const_iterator parseDesigtion(Type ty) {
      auto tyIt = ty.begin();
      do {
         if (consumeAnyOf(TK::PERIOD)) {
            Token ident = expect(TK::IDENT);
            // todo: check that it is actually a struct type
            auto end = tyIt->end();
            not_implemented();
            // tyIt = std::find_if(tyIt, end, [&](auto &member) {
            //    return member->ident == ident;
            // });
            if (tyIt == end) {
               diagnostics_ << ident.getLoc() << "no member named '" << ident << "' in '" << *tyIt << '\'' << std::endl;
            }
         } else {
            expect(TK::L_BRACKET);
            expr_t expr = parseExpr();
            expect(TK::R_BRACKET);
            not_implemented();
         }
      } while (hasAnyOf(TK::PERIOD, TK::L_BRACKET));
      expect(TK::ASSIGN);
      return tyIt;
   }

   value_t parseInitializer(Type ty) {
      auto it = ty.begin();
      return parseInizializerImpl<true>(ty, it, nullptr);
   }

   void parseAndInitializeLocalVar(Type ty, ssa_t *var) {
      auto it = ty.begin();
      parseInizializerImpl<false>(ty, it, var);
   }

   Tokenizer tokenizer_;
   typename Tokenizer::const_iterator pos_;
   T emitter_{};

   scope::Scope<Ident, ScopeInfo> varScope_{};
   scope::Scope<Ident, locatable<Type>> tagScope_{};
   scope::Scope<Ident, locatable<Type>> typedefScope_{};

   void enter() {
      varScope_.enter();
      tagScope_.enter();
      typedefScope_.enter();
   }

   void leave() {
      varScope_.leave();
      tagScope_.leave();
      typedefScope_.leave();
   }

   DiagnosticTracker &diagnostics_;
   Tracer tracer_;
   Factory factory_;

   void advance(int n = 1) {
      for (int i = 0; i < n; ++i) {
         advanceOne();
      }
   }

   void advanceOne() {
      if (pos_ && !hasAnyOf(TK::PP_START)) {
         ++pos_;
      }
      while (hasAnyOf(TK::PP_START)) {
         ++pos_;
         Token lineNo, file;
         if (hasAnyOf(TK::ICONST, TK::L_ICONST, TK::LL_ICONST)) {
            lineNo = *pos_;
            ++pos_;
         }
         if (hasAnyOf(TK::SLITERAL)) {
            file = *pos_;
            ++pos_;
         }
         std::vector<int> cmds{};
         while (pos_ && pos_->getKind() != TK::PP_END) {
            if (hasAnyOf(TK::ICONST)) {
               cmds.push_back(pos_->getValue<int>());
            } else {
               diagnostics_ << pos_->getLoc() << "unexpected token in gcc-style preprocessor info" << std::endl;
            }
            ++pos_;
         }
         if (pos_) {
            // consume PP_END
            ++pos_;
         }
         if (lineNo && file) {
            std::string fileName{file.getString()};
            diagnostics_.registerFileMapping(fileName, lineNo.getValue<int>());
         }
      }
   }

   // todo: may be part of custom scope datastructure
   bool isMissingDefaultInitiation(ssa_t *var) {
      return std::find_if(missingDefaultInitiations_.begin(), missingDefaultInitiations_.end(), [&](auto *v) {
                return v->ssa() == var;
             }) != missingDefaultInitiations_.end();
   }

   bool isExternalDeclaration(ssa_t *var) {
      return std::find_if(externDeclarations_.begin(), externDeclarations_.end(), [&](auto *v) {
                return v->ssa() == var;
             }) != externDeclarations_.end();
   }

   bool isTypedef() {
      return pos_->getKind() == TK::IDENT && typedefScope_.find(pos_->getValue<Ident>());
   }

   std::vector<ScopeInfo *> externDeclarations_;
   std::vector<ScopeInfo *> missingDefaultInitiations_;
};
// ---------------------------------------------------------------------------
// token classification
// ---------------------------------------------------------------------------
#ifdef __IDE_MARKER
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
bool isPostfixExprStart(token::Kind kind) {
   return kind == token::Kind::INC || kind == token::Kind::DEC || kind == token::Kind::L_BRACKET || kind == token::Kind::L_BRACE || kind == token::Kind::DEREF || kind == token::Kind::PERIOD;
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------
template <typename T>
template <std::size_t N>
token::Token Parser<T>::expect(TK kind, const char *where, const char *what, std::array<TK, N> likelyNext, bool usePrevTokenEndLoc) {
   if (Token t = consumeAnyOf(kind)) {
      return t;
   }

   if (usePrevTokenEndLoc) {
      SrcLoc prev = pos_.getPrevLoc();
      diagnostics_ << SrcLoc(prev.locEnd());
   } else {
      diagnostics_ << pos_->getLoc();
   }
   diagnostics_ << "expected ";
   if (what) {
      diagnostics_ << what;
   } else {
      diagnostics_ << "'" << kind << "'";
   }
   if (where) {
      diagnostics_ << " " << where;
   } else {
      diagnostics_ << " but got " << pos_->getKind();
   }
   diagnostics_ << std::endl;
   auto it = std::find(likelyNext.begin(), likelyNext.end(), pos_->getKind());
   // only advance if the next token is not going to be consumed by the parser
   // this improves error messages
   if (it == likelyNext.end()) {
      advance();
   }
   return Token{};
}
// ---------------------------------------------------------------------------
template <typename T>
token::Token Parser<T>::expect(TK kind, const char *where, const char *what, bool usePrevTokenEndLoc) {
   return expect<0>(kind, where, what, {}, usePrevTokenEndLoc);
}
// ---------------------------------------------------------------------------
// emit folded constant expression or operation depending on the type of value
// ---------------------------------------------------------------------------
#ifdef __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename T>
template <typename cFn, typename Fn, typename... Args>
typename Parser<T>::value_t Parser<T>::emitUnOpImpl(cFn __cFn, Fn __fn, Type ty, value_t value, Args... args) {
   if (!state.eval || std::holds_alternative<std::monostate>(value)) {
      return {};
   }
   if (ssa_t **ssa = std::get_if<ssa_t *>(&value)) {
      if (!state.bb) {
         diagnostics_ << pos_->getLoc() << "cannot emit instruction outside of function" << std::endl; // todo: this message should not be necessary
         return {};
      }
      return __fn(state.bb, ty, *ssa, std::forward<Args>(args)...);
   }
   return asValue(__cFn(state.bb, ty, getConst(value), std::forward<Args>(args)...));
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::value_t Parser<T>::emitNeg(Type ty, value_t value) {
   return emitUnOpImpl([this](auto &&...args) { return emitter_.emitConstNeg(args...); },
                       [this](auto &&...args) { return emitter_.emitNeg(args...); },
                       ty, value);
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::value_t Parser<T>::emitBWNeg(Type ty, value_t value) {
   return emitUnOpImpl([this](auto &&...args) { return emitter_.emitConstBWNeg(args...); },
                       [this](auto &&...args) { return emitter_.emitBWNeg(args...); },
                       ty, value);
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::value_t Parser<T>::emitNot(Type ty, value_t value) {
   return emitUnOpImpl([this](auto &&...args) { return emitter_.emitConstNot(args...); },
                       [this](auto &&...args) { return emitter_.emitNot(args...); },
                       ty, value);
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::value_t Parser<T>::emitCast(Type fromTy, value_t value, Type toTy, qcp::type::Cast cast) {
   return emitUnOpImpl([this](auto &&...args) { return emitter_.emitConstCast(args...); },
                       [this](auto &&...args) { return emitter_.emitCast(args...); },
                       fromTy, value, toTy, cast);
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
// diagnostic messages helper
// ---------------------------------------------------------------------------
#ifdef __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::notePrevWhatHere(const char *what, SrcLoc loc) {
   diagnostics_ << DiagnosticMessage::Kind::NOTE << loc << "previous " << what << " is here" << std::endl;
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::notePrevDeclHere(ScopeInfo *info) {
   assert(info && "info must not be nullptr");
   notePrevWhatHere("declaration", info->loc.truncate(0));
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::notePrevDefHere(SrcLoc loc) {
   notePrevWhatHere("definition", loc.truncate(0));
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::errorVarIncompleteType(SrcLoc loc, Type ty) {
   loc = loc.truncate(0);
   if (ty->isArrayTy()) {
      diagnostics_ << loc << "definition of variable with array type needs an explicit size or an initializer" << std::endl;
   } else {
      diagnostics_ << loc << "variable has incomplete type '" << ty << '\'' << std::endl;
   }
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::errorAssignToConst(SrcLoc loc, Type ty, Ident ident) {
   diagnostics_ << loc << "cannot assign to variable ";
   if (ident) {
      diagnostics_ << '\'' << ident << "' ";
   }
   diagnostics_ << "with const-qualified type '" << ty << '\'' << std::endl;
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::noteConstDeclHere(SrcLoc loc, Ident ident) {
   diagnostics_ << DiagnosticMessage::Kind::NOTE << loc << "variable ";
   if (ident) {
      diagnostics_ << '\'' << ident << "' ";
   }
   diagnostics_ << "declared const here " << std::endl;
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
// operations on values
// ---------------------------------------------------------------------------
#ifdef __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::const_or_iconst_t Parser<T>::getConst(value_t value) const {
   if (const_t **c = std::get_if<const_t *>(&value)) {
      return *c;
   } else if (iconst_t **c = std::get_if<iconst_t *>(&value)) {
      return *c;
   }
   assert(false && "unreachable"); // todo: llvm unreachable
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::value_t Parser<T>::asValue(const_or_iconst_t c) const {
   if (std::holds_alternative<std::monostate>(c)) {
      return {};
   }
   if (const_t **c_ = std::get_if<const_t *>(&c)) {
      return *c_;
   }
   return std::get<iconst_t *>(c);
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::value_t Parser<T>::cast(SrcLoc loc, Type from, Type to, value_t value, bool explicitCast) {
   if (from == to || !state.eval || !from || !to) {
      return value;
   }

   if (to->isVoidTy()) {
      return {};
   }

   type::Cast kind;
   if (!from) {
      kind = type::Cast::BITCAST;
   } else if (from->isIntegerTy() && to->isIntegerTy()) {
      if (from->rank() > to->rank()) {
         kind = type::Cast::TRUNC;
      } else if (from->rank() < to->rank()) {
         kind = (from->isSignedTy() || from->isSignedCharlikeTy()) ? type::Cast::SEXT : type::Cast::ZEXT;
      } else {
         return value;
      }
   } else if (from->isIntegerTy() && to->isFloatingTy()) {
      kind = from->isSignedTy() ? type::Cast::SITOFP : type::Cast::UITOFP;
   } else if (from->isFloatingTy() && to->isIntegerTy()) {
      kind = type::Cast::FPTOSI;
   } else if (from->isFloatingTy() && to->isFloatingTy() && from->rank() > to->rank()) {
      kind = type::Cast::FPTRUNC;
   } else if (from->isFloatingTy() && to->isFloatingTy() && from->rank() < to->rank()) {
      kind = type::Cast::FPEXT;
   } else if ((from->isPointerTy() && to->isIntegerTy()) ||
              (from->isIntegerTy() && to->isPointerTy())) {
      if (!from->isPointerTy()) {
         kind = type::Cast::INTTOPTR;

         value = cast(loc, from, factory_.uintptrTy(), value, explicitCast);
      } else {
         kind = type::Cast::PTRTOINT;
      }
      if (!explicitCast) {
         diagnostics_ << loc << "incompatible integer to pointer conversion from '" << from << "' to '" << to << '\'' << std::endl;
      }
   } else if ((from->isPointerTy() || from->isArrayTy()) && to->isPointerTy()) {
      // todo: check if the pointer types are compatible
      return value;
   } else {
      diagnostics_ << loc << "invalid cast from '" << from << "' to '" << to << '\'' << std::endl;
      return {};
   }
   return emitCast(from, value, to, kind);
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
// operations on expressions
// ---------------------------------------------------------------------------
#ifdef __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::value_t Parser<T>::asRVal(expr_t &expr) {
   if (!state.eval || !expr->ty || (isSealed(state.bb) && expr->mayBeLval)) {
      return {};
   } else if (expr->ty->isArrayTy()) {
      return arrToPtrDecay(expr);
   } else if (expr->mayBeLval && expr->ty->kind() != TYK::FN_T) {
      return emitter_.emitLoad(state.bb, expr->ty, std::get<ssa_t *>(expr->value), expr->ident);
   }
   return expr->value;
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::value_t Parser<T>::cast(expr_t &expr, Type to, bool explicitCast) {
   return cast(expr->loc, expr->ty, to, asRVal(expr), explicitCast);
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::value_t Parser<T>::asBoolByComparison(expr_t &expr, op::Kind op) {
   if (!state.eval || isSealed(state.bb)) {
      return {};
   }
   const_or_iconst_t zero;
   if (expr->ty->isIntegerTy() || expr->ty->isPointerTy()) {
      zero = emitter_.emitIConst(expr->ty, 0);
   } else {
      zero = emitter_.emitFPConst(expr->ty, 0.0);
   }
   auto value = asRVal(expr);
   if (std::holds_alternative<ssa_t *>(value)) {
      return emitter_.emitBinOp(state.bb, expr->ty, op, value, asValue(zero));
   }
   return asValue(emitter_.emitConstBinOp(state.bb, expr->ty, op, getConst(value), zero));
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::value_t Parser<T>::isTruethy(expr_t &expr) {
   if (!state.eval || !expr->ty) {
      return {};
   } else if (expr->ty->isBoolTy()) {
      return asRVal(expr);
   }
   return asBoolByComparison(expr, op::Kind::NE);
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
// control flow
// ---------------------------------------------------------------------------
#ifdef __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::markSealed(bb_t *bb) {
   auto end = std::remove_if(state.unsealedBlocks.begin(), state.unsealedBlocks.end(), [bb](auto &bbloc) {
      return static_cast<bb_t *>(bbloc) == bb;
   });
   state.unsealedBlocks.erase(end, state.unsealedBlocks.end());
}
// ---------------------------------------------------------------------------
template <typename T>
bool Parser<T>::isSealed(bb_t *bb) {
   return bb && std::find_if(state.unsealedBlocks.begin(), state.unsealedBlocks.end(), [bb](auto &bbloc) {
                   return static_cast<bb_t *>(bbloc) == bb;
                }) == state.unsealedBlocks.end();
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::completeBreaks(bb_t *target) {
   for (auto &bb : state.missingBreaks.back()) {
      emitJump(bb, target);
   }
   state.missingBreaks.pop_back();
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::parse() {
   SrcLoc loc;
   if (hasAnyOf(TK::PP_START)) {
      advance();
   }
   while (*pos_) {
      diagnostics_.unsilence();
      loc = pos_->getLoc();

      std::vector<attr_t> attr = parseOptAttributeSpecifierSequence();
      state = State();
      parseDeclStmt(attr);
      loc |= pos_->getLoc();
      for (auto &[lbl, gotos] : state.incompleteGotos) {
         for (auto bbloc : gotos) {
            if (state.labels.contains(lbl)) {
               emitJump(static_cast<bb_t *>(bbloc), state.labels[lbl]);
            } else {
               diagnostics_ << bbloc.loc().truncate(0) << "use of undeclared label '" << lbl << '\'' << std::endl;
            }
         }
      }
      if (!state.fn || !state.entry) {
         continue;
      }

      // handle gotos and returns and unselaed blocks
      if (state.outstandingReturns.size() == 1) {
         emitter_.emitRet(state.outstandingReturns[0].first, state.outstandingReturns[0].second);
      } else if (state.outstandingReturns.size() > 1) {
         ssa_t *retVar = emitter_.emitLocalVar(state.fn, state.entry, state.retTy, Ident("__retVar"), true);
         bb_t *retBB = emitter_.emitBB(state.fn, nullptr, Ident("__retBB"));
         for (auto [bb, val] : state.outstandingReturns) {
            emitter_.emitStore(bb, state.retTy, val, retVar);
            emitter_.emitJump(bb, retBB);
         }
         ssa_t *retVal = emitter_.emitLoad(retBB, state.retTy, retVar, Ident("__retVar"));
         emitter_.emitRet(retBB, retVal);
      }
      static Ident MAIN{"main"};
      for (auto bbloc : state.unsealedBlocks) {
         if (state.retTy->kind() == TYK::VOID) {
            emitter_.emitRet(static_cast<bb_t *>(bbloc), static_cast<ssa_t *>(nullptr));
         } else if (state.fnName == MAIN) {
            emitter_.emitRet(static_cast<bb_t *>(bbloc), emitter_.emitIConst(state.retTy, 0));
         } else {
            diagnostics_ << bbloc.loc() << "missing return statement in function returning non-void" << std::endl;
         }
      }
      emitter_.finalizeFn(state.fn);
   }
   for (ScopeInfo *var : missingDefaultInitiations_) {
      if (!var->ty->isCompleteTy()) {
         errorVarIncompleteType(var->loc, var->ty);
         continue;
      }
      emitter_.zeroInitGlobalVar(var->ty, var->ssa());
   }
}
// ---------------------------------------------------------------------------
template <typename T>
bool Parser<T>::parseParameterList(std::vector<Type> &paramTys, std::vector<std::pair<Ident, SrcLoc>> &paramNames) {
   enter();
   while (isDeclarationSpecifier(pos_->getKind()) || isTypedef() || hasAnyOf(TK::ELLIPSIS)) {
      if (consumeAnyOf(TK::ELLIPSIS)) {
         leave();
         return true;
      }
      parseOptAttributeSpecifierSequence();

      Type ty = parseSpecifierQualifierList();
      // todo: handle names in casts/abstract declarators
      Declarator decl = parseDeclarator(ty);

      if (decl.ty->kind() == TYK::VOID) {
         if (paramTys.size() > 0) {
            diagnostics_ << "void must be the only parameter" << std::endl;
         }
         if (decl.ident) {
            diagnostics_ << "void must not have a name" << std::endl;
         }
         if (pos_->getKind() != TK::R_BRACE) {
            diagnostics_ << pos_->getLoc() << "'void' must be the first and only parameter if specified" << std::endl;
         }
         break;
      } else if (!decl.ty->isCompleteTy()) {
         diagnostics_ << decl.nameLoc << "parameter has incomplete type '" << decl.ty << '\'' << std::endl;
      }

      if (decl.ty->isArrayTy()) {
         Type ty = factory_.ptrTo(decl.ty->getElementTy());
         ty.qualifiers = decl.ty.qualifiers;
         decl.ty = factory_.harden(ty);
      }

      paramTys.push_back(decl.ty);
      paramNames.emplace_back(decl.ident, decl.nameLoc);

      if (decl.ident && !varScope_.insert(decl.ident, ScopeInfo(decl.ty, decl.nameLoc))) {
         diagnostics_ << decl.nameLoc.truncate(0) << "redefinition of parameter '" << decl.ident << '\'' << std::endl;
         notePrevDeclHere(varScope_.find(decl.ident));
      }

      if (!consumeAnyOf(TK::COMMA)) {
         break;
      }
   }
   leave();
   return false;
}
// ---------------------------------------------------------------------------
template <typename T>
int Parser<T>::parseOptLabelList(std::vector<attr_t> &attr) {
   int labelCount = 0;
   bb_t *labelTarget = nullptr;
   const char *expectWhere = nullptr;
   for (TK kind = pos_->getKind(); isLabelStmtStart(kind, pos_.peek()); kind = pos_->getKind(), ++labelCount) {
      if (!labelTarget) {
         labelTarget = newBB();
         emitJumpIfNotSealed(state.bb, labelTarget);
         state.bb = labelTarget;
      }
      SrcLoc loc = pos_->getLoc();
      if (consumeAnyOf(TK::CASE)) {
         // case-statement
         expectWhere = "after 'case'";
         expr_t expr = parseExpr();
         optArrToPtrDecay(expr);
         iconst_t **value = std::get_if<iconst_t *>(&expr->value);
         if (state.switches.empty()) {
            diagnostics_ << loc.truncate(0) << "'case' statement not in switch statement" << std::endl;
         } else if (!expr->ty->isIntegerTy()) {
            diagnostics_ << expr->loc << "Case expression must be of integer type" << std::endl;
         } else if (!value) {
            diagnostics_ << expr->loc << "Case expression must be a constant" << std::endl;
         } else {
            SwitchState &sw = state.switches.back();
            auto it = std::find(sw.values.begin(), sw.values.end(), *value);
            if (it != sw.values.end()) {
               diagnostics_ << expr->loc << "duplicate case value" << std::endl;
            } else {
               sw.values.push_back(*value);
               sw.blocks.push_back(labelTarget);
            }
            emitter_.addSwitchCase(sw.sw, *value, labelTarget);
         }
      } else if (consumeAnyOf(TK::DEFAULT)) {
         // default-statement
         expectWhere = "after 'default'";
         if (state.switches.empty()) {
            diagnostics_ << loc.truncate(0) << "'default' statement not in switch statement" << std::endl;
         } else {
            SwitchState &sw = state.switches.back();
            auto it = std::find(sw.values.begin(), sw.values.end(), nullptr);
            if (it == sw.values.end()) {
               sw.values.push_back(nullptr);
               sw.blocks.push_back(labelTarget);
            } else {
               diagnostics_ << "Duplicate default case" << std::endl;
            }
            emitter_.addSwitchDefault(sw.sw, labelTarget);
         }
      } else {
         // labeled-statement
         Ident label = pos_->getValue<Ident>();
         if (state.labels.contains(label)) {
            diagnostics_ << "Redefinition of label " << label << std::endl;
         }
         state.labels[label] = labelTarget;
         advance();
      }
      expect(TK::COLON, expectWhere);
      attr = parseOptAttributeSpecifierSequence();
   }
   return labelCount;
}
// ---------------------------------------------------------------------------
// parse statements
// ---------------------------------------------------------------------------
#ifdef __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::parseStmt() {
   std::vector<attr_t> attr = parseOptAttributeSpecifierSequence();
   parseOptLabelList(attr);
   parseUnlabledStmt(attr);
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::parseDeclStmt(const std::vector<attr_t> &attr) {
   // todo: static_assert-declaration

   if (attr.size() > 0 && consumeAnyOf(TK::SEMICOLON)) {
      // todo: (jr) attribute declaration
   } else {
      DeclarationSpecifier declSpec = parseDeclarationSpecifierList();
      bool mayBeFunctionDef = true;
      if (consumeAnyOf(TK::SEMICOLON)) {
         // no init declarator list
         return;
      }
      do {
         Declarator decl = parseDirectDeclarator(declSpec.ty);
         factory_.clearFragments();
         if (!decl.ident) {
            continue;
         }

         if (declSpec.storageClass[0] == TK::TYPEDEF || declSpec.storageClass[1] == TK::TYPEDEF) {
            if (!typedefScope_.canInsert(decl.ident)) {
               diagnostics_ << decl.nameLoc << "redefinition of typedef '" << decl.ident << '\'' << std::endl;
               locatable<Type> *prev = typedefScope_.find(decl.ident);
               notePrevDefHere(prev->loc());
            } else {
               typedefScope_.insert(decl.ident, locatable<Type>(decl.nameLoc, decl.ty));
            }
            continue;
         }

         ScopeInfo *info = varScope_.find(decl.ident);
         bool canInsert = varScope_.canInsert(decl.ident);

         if (!canInsert && !varScope_.isTopLevel()) {
            errorRedef(decl.nameLoc, decl.ident, decl.ty, info->ty);
            notePrevDefHere(info->loc);
         } else if (varScope_.isTopLevel() && info && !info->ty.isCompatibleWith(decl.ty)) {
            diagnostics_ << decl.nameLoc.truncate(0) << "conflicting types for '" << decl.ident << '\'' << std::endl;
            notePrevDeclHere(info);
            decl.ty = info->ty;
         } else if (!varScope_.isTopLevel() && decl.ty->kind() == TYK::FN_T) {
            diagnostics_ << decl.nameLoc.truncate(0) << "function definition is not allowed here" << std::endl;
         }

         if (decl.ty->kind() == TYK::FN_T) {
            // set function of state, potentially reusing the function if it was already declared
            state.fn = info ? info->fn() : emitter_.emitFnProto(decl.ty, state.inlineFn(), state.noreturnFn(), decl.ident);
            state.retTy = decl.ty->getRetTy();
            state.fnName = decl.ident;

            if (canInsert) {
               info = varScope_.insert(decl.ident, ScopeInfo(decl.ty, decl.nameLoc, state.fn, !!state.entry));
            }

            if (mayBeFunctionDef && hasAnyOf(TK::L_C_BRKT)) {
               if (info && info->hasDefOrInit) {
                  errorRedef(decl.nameLoc, decl.ident, decl.ty, info->ty);
                  notePrevDefHere(info->loc);
               }
               // function body
               info->hasDefOrInit = true;
               parseFunctionDefinition(decl);
            }

            if (state.entry) {
               // function definition cannot be in parameter list
               return;
            }

         } else {
            bool isGlobal = varScope_.isTopLevel() || declSpec.storageClass[0] == TK::STATIC || declSpec.storageClass[1] == TK::STATIC;

            ssa_t *var = nullptr;
            if (isGlobal && decl.ty->isCompleteTy() && (canInsert || !info || !(info->ssa()))) {
               Ident name = decl.ident;
               if (state.fnName) {
                  name = Ident(state.fnName + "." + decl.ident);
               }
               var = emitter_.emitGlobalVar(decl.ty, name);
            } else if (!isGlobal) {
               if (!decl.ty->isCompleteTy()) {
                  errorVarIncompleteType(decl.nameLoc, decl.ty);
                  if (info) {
                     noteForwardDeclHere(info->loc, info->ty);
                  }
                  decl.ty = factory_.undefTy();
               } else {
                  var = emitter_.emitLocalVar(state.fn, state.entry, decl.ty, decl.ident);
               }
            }

            if (canInsert) {
               info = varScope_.insert(decl.ident, ScopeInfo(decl.ty, decl.nameLoc, var, isGlobal));
            } else if (info && !info->ssa()) {
               info->ssa() = var;
            } else {
               var = info->ssa();
            }

            if (!info->ty->isCompleteTy() && decl.ty->isCompleteTy()) {
               info->ty = decl.ty;
            }

            if (Token assignToken = consumeAnyOf(TK::ASSIGN)) {
               if (!decl.ty->isCompleteTy()) {
                  errorVarIncompleteType(decl.nameLoc, decl.ty);
                  if (info) {
                     noteForwardDeclHere(info->loc, info->ty);
                  }
                  decl.ty = factory_.undefTy();
               } else if (info->ty->kind() == TYK::FN_T) {
                  diagnostics_ << "Illegal Inizilizer. Only variables can be initialized" << std::endl;
               } else if (varScope_.isTopLevel() && !canInsert && info->ssa() && !isMissingDefaultInitiation(info->ssa()) && !isExternalDeclaration(info->ssa())) {
                  errorRedef(decl.nameLoc, decl.ident, decl.ty, info->ty);
                  notePrevDefHere(info->loc);
               }
               if (!decl.ty) {
                  // TODO: error message todo
                  continue;
               }
               info->hasDefOrInit = true;
               if (isGlobal) {
                  value_t val = parseInitializer(decl.ty);
                  if (!std::holds_alternative<std::monostate>(val)) {
                     emitter_.setInitValueGlobalVar(var, getConst(val));
                  }
               } else {
                  parseAndInitializeLocalVar(decl.ty, var);
               }
               if (isSealed(state.bb)) {
                  // todo: diagnostics_ << "Variable initialization is unreachable" << std::endl;
               }

               if (isGlobal) {
                  auto it = std::find_if(missingDefaultInitiations_.begin(), missingDefaultInitiations_.end(), [&](auto *v) {
                     return v->ssa() == var;
                  });
                  if (it != missingDefaultInitiations_.end()) {
                     missingDefaultInitiations_.erase(it);
                  }
                  it = std::find(externDeclarations_.begin(), externDeclarations_.end(), info);
                  if (it != externDeclarations_.end()) {
                     externDeclarations_.erase(it);
                  }
               }
            } else if (isGlobal && decl.ident) {
               if (declSpec.storageClass[0] == TK::EXTERN) {
                  externDeclarations_.push_back(info);
               } else {
                  missingDefaultInitiations_.push_back(info);
               }
            }
         }
         mayBeFunctionDef = false;
      } while (consumeAnyOf(TK::COMMA));
      const char *expectWhere = varScope_.isTopLevel() ? "after top level declarator" : "at end of declaration";
      expect(TK::SEMICOLON, expectWhere);
   }
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::parseCompoundStmt() {
   expect(TK::L_C_BRKT);

   while (pos_->getKind() != TK::R_C_BRKT && pos_->getKind() != TK::END) {
      auto attr = parseOptAttributeSpecifierSequence();
      parseOptLabelList(attr);
      if (isDeclarationStart(pos_->getKind()) || isTypedef()) {
         parseDeclStmt(attr);
      } else {
         parseUnlabledStmt(attr);
      }
   }
   expect(TK::R_C_BRKT);
   diagnostics_.unsilence();
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::parseUnlabledStmt(const std::vector<attr_t> &attr) {
   TK kind = pos_->getKind();

   if (kind == TK::L_C_BRKT) {
      // todo: attribute-specifier-sequenceopt
      enter();
      parseCompoundStmt();
      leave();
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
template <typename T>
void Parser<T>::parseSelectionStmt() {
   expr_t cond{};
   if (consumeAnyOf(TK::IF)) {
      bb_t *fromBB;
      bb_t *then;
      bb_t *thenEnd = nullptr;
      bb_t *otherwiseStart = nullptr;

      expect(TK::L_BRACE);
      cond = parseConditionExpr();
      expect(TK::R_BRACE);
      fromBB = state.bb;
      state.bb = then = newBB();
      parseStmt();
      thenEnd = state.bb;
      if (consumeAnyOf(TK::ELSE)) {
         state.bb = otherwiseStart = newBB();
         parseStmt();
      }

      bb_t *cont = nullptr;
      if (!(otherwiseStart && isSealed(thenEnd) && isSealed(state.bb))) {
         // do not create a new BB if both branches are sealed
         cont = newBB();
      }

      otherwiseStart = otherwiseStart ? otherwiseStart : cont;
      emitBranch(fromBB, then, otherwiseStart, cond->value);
      if (cont && cont != otherwiseStart) {
         emitJumpIfNotSealed(state.bb, cont);
      }
      emitJumpIfNotSealed(thenEnd, cont);
      state.bb = cont; // todo: ask alexis if this is okey because unreachable code throws an error here

   } else {
      expect(TK::SWITCH);
      expect(TK::L_BRACE);
      cond = parseExpr();
      optArrToPtrDecay(cond);
      expect(TK::R_BRACE);
      if (!cond->ty->isIntegerTy()) {
         diagnostics_ << "Switch condition must have integer type" << std::endl;
         cond->value = {};
      }
      state.switches.emplace_back(emitter_.emitSwitch(state.bb, asRVal(cond)));
      state.missingBreaks.emplace_back();
      markSealed(state.bb);
      parseStmt();
      auto defaultIt = std::find(state.switches.back().values.begin(), state.switches.back().values.end(), nullptr);
      bb_t *cont = newBB();
      emitJumpIfNotSealed(state.bb, cont);
      if (defaultIt == state.switches.back().values.end()) {
         emitter_.addSwitchDefault(state.switches.back().sw, cont);
      }
      completeBreaks(cont);
      state.switches.pop_back();
      state.bb = cont;
   }
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::parseIterationStmt() {
   bool forLoop = false;
   bool doWhile = false;
   bb_t *fromBB = state.bb;

   std::array<bb_t *, 2> condBBs{};
   std::array<bb_t *, 2> bodyBBs{};
   std::array<bb_t *, 2> updateBBs{};

   expr_t cond = nullptr;
   state.missingBreaks.emplace_back();

   if (consumeAnyOf(TK::DO)) {
      doWhile = true;
   } else if (consumeAnyOf(TK::FOR)) {
      forLoop = true;
      enter();
      expect(TK::L_BRACE);
      std::vector<attr_t> attr{parseOptAttributeSpecifierSequence()};
      if (isDeclarationStart(pos_->getKind()) || isTypedef()) {
         parseDeclStmt(attr);
      } else if (pos_->getKind() != TK::SEMICOLON) {
         parseExpr();
         expect(TK::SEMICOLON);
      }
      if (pos_->getKind() != TK::SEMICOLON) {
         state.bb = condBBs.front() = newBB();
         cond = parseConditionExpr();
         condBBs.back() = state.bb;
      }
      expect(TK::SEMICOLON);
      if (pos_->getKind() != TK::R_BRACE) {
         bodyBBs.front() = newBB();
         state.bb = updateBBs.front() = newBB();
         parseExpr();
         updateBBs.back() = state.bb;
      }
      expect(TK::R_BRACE);
   } else {
   parse_while:
      expect(TK::WHILE);
      expect(TK::L_BRACE);
      state.bb = condBBs.front() = newBB();
      cond = parseConditionExpr();
      condBBs.back() = state.bb;
      expect(TK::R_BRACE);
      if (doWhile) {
         goto end_parse_body;
      }
   }
   if (!bodyBBs.front()) {
      bodyBBs.front() = newBB();
   }
   state.continueTargets.push_back(condBBs.front());
   state.bb = bodyBBs.front();
   parseStmt();
   bodyBBs.back() = state.bb;
   if (doWhile) {
      goto parse_while;
   } else if (forLoop && updateBBs.front()) {
      emitJump(state.bb, updateBBs.front());
   }
end_parse_body:
   if (doWhile) {
      expect(TK::SEMICOLON);
   }
   state.bb = newBB();
   emitJump(fromBB, doWhile ? bodyBBs.front() : condBBs.front());
   emitJump(updateBBs.back() ? updateBBs.back() : bodyBBs.back(), condBBs.front() ? condBBs.front() : bodyBBs.front());
   if (cond) {
      emitBranch(condBBs.back(), bodyBBs.front(), state.bb, cond->value);
   }
   if (forLoop) {
      leave();
   }

   completeBreaks(state.bb);
   state.continueTargets.pop_back();
}
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::parseJumpStmt() {
   SrcLoc loc = pos_->getLoc().truncate(0);
   if (consumeAnyOf(TK::GOTO)) {
      SrcLoc loc = pos_->getLoc();
      Ident label = expect(TK::IDENT).template getValue<Ident>();
      expect(TK::SEMICOLON);
      if (isSealed(state.bb)) {
         return;
      }
      if (state.labels.contains(label)) {
         emitter_.emitJump(state.bb, state.labels[label]);
      } else {
         state.incompleteGotos[label].emplace_back(state.bb, loc);
      }
      markSealed(state.bb);
      state.bb = newBB();
   } else if (consumeAnyOf(TK::CONTINUE)) {
      if (state.continueTargets.empty()) {
         diagnostics_ << loc << "'continue' statement not in loop statement" << std::endl;
      } else {
         emitJump(state.bb, state.continueTargets.back());
      }
   } else if (consumeAnyOf(TK::BREAK)) {
      if (state.missingBreaks.empty()) {
         diagnostics_ << loc << "'break' statement not in loop or switch statement" << std::endl;
      } else if (isSealed(state.bb)) {
         diagnostics_ << loc << "break unreachable" << std::endl;
      } else {
         state.missingBreaks.back().emplace_back(state.bb);
         markSealed(state.bb);
      }
   } else if (consumeAnyOf(TK::RETURN)) {
      value_t value;
      expr_t expr;
      SrcLoc retLoc = pos_.getPrevLoc().truncate(0);
      if (pos_->getKind() != TK::SEMICOLON) {
         expr = parseExpr();
         optArrToPtrDecay(expr);
         if (state.retTy->kind() == TYK::VOID) {
            diagnostics_ << retLoc << "void function '" << state.fnName << "' should not return a value" << std::endl;
         } else {
            value = cast(expr, state.retTy);
         }
      } else if (state.retTy->kind() != TYK::VOID) {
         diagnostics_ << retLoc << "non-void function '" << state.fnName << "' should return a value" << std::endl;
      }
      expect(TK::SEMICOLON);
      if (isSealed(state.bb)) {
         return;
      }
      if (state.retTy->isVoidTy()) {
         emitter_.emitRet(state.bb, {});
      } else {
         state.outstandingReturns.push_back({state.bb, value});
      }
      markSealed(state.bb);
   } else {
      not_implemented();
   }
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename T>
void Parser<T>::parseFunctionDefinition(Declarator &decl) {
   enter();
   state.entry = state.bb = emitter_.emitFn(state.fn);
   state.unsealedBlocks.emplace_back(state.bb, pos_->getLoc());
   for (unsigned i = 0; i < decl.ty->getParamTys().size(); ++i) {
      Type paramTy = decl.ty->getParamTys()[i];
      auto [name, loc] = decl.paramNames[i];
      ssa_t *val = emitter_.getParam(state.fn, i);
      ssa_t *var = emitter_.emitLocalVar(state.fn, state.entry, paramTy, name);
      emitter_.emitStore(state.entry, paramTy, val, var);
      varScope_.insert(name, ScopeInfo(paramTy, loc, var, true));
   }

   // function-definition
   parseCompoundStmt();
   while (consumeAnyOf(TK::R_C_BRKT)) {
      diagnostics_ << pos_.getPrevLoc() << "extraneous closing brace ('}')" << std::endl;
   }
   leave();
}
// ---------------------------------------------------------------------------
// parse expressions
// ---------------------------------------------------------------------------
#ifdef __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename T>
Parser<T>::expr_t Parser<T>::parseExpr(short minPrec) {
   expr_t lhs = parsePrimaryExpr();
   while (*pos_ && pos_->getKind() >= token::Kind::ASTERISK && pos_->getKind() <= token::Kind::QMARK) {
      if (hasAnyOf(TK::R_BRACE, TK::R_BRACKET, TK::R_C_BRKT)) {
         break;
      }

      TK tk = pos_->getKind();
      if (isPostfixExprStart(pos_->getKind())) {
         lhs = parsePostfixExpr(std::move(lhs));
      } else if (tk == TK::QMARK) {
         // ternary operator
         if (minPrec && op::getOpSpec(op::Kind::COND).precedence >= minPrec) {
            break;
         }
         advance();
         value_t boolv = isTruethy(lhs);

         bb_t *fromBB = state.bb;
         bb_t *otherwiseBB = newBB();
         bb_t *thenBB = state.bb = newBB();
         bb_t *contBB = newBB();
         emitBranch(fromBB, thenBB, otherwiseBB, boolv);

         expr_t thenV = parseExpr();
         thenBB = state.bb;
         state.bb = otherwiseBB;
         expect(TK::COLON);
         expr_t otherwiseV = parseConditionalExpr();
         otherwiseBB = state.bb;

         emitJump(thenBB, contBB);
         emitJump(otherwiseBB, contBB);

         std::array<std::pair<value_t, bb_t *>, 2> phiArgs = {
             std::make_pair(thenV->value, thenBB),
             std::make_pair(otherwiseV->value, otherwiseBB)};
         Type resTy = factory_.commonRealType(thenV->ty, otherwiseV->ty);
         value_t result = emitter_.emitPhi(contBB, resTy, phiArgs);
         lhs = std::make_unique<Expr<T>>(op::Kind::COND, resTy, std::move(thenV), std::move(otherwiseV), result);
      } else {
         SrcLoc opLoc = pos_->getLoc();
         if (tk > TK::COMMA || tk < TK::ASTERISK) {
            diagnostics_ << pos_->getLoc() << "invalid operator" << std::endl;
            advance();
            continue;
         }
         op::Kind op = op::getBinOpKind(tk);
         op::OpSpec spec{op::getOpSpec(op)};

         if (minPrec && spec.precedence >= minPrec) {
            break;
         }
         advance();

         optArrToPtrDecay(lhs);

         // for short circuiting
         value_t boolv;
         bb_t *ifBB, *otherwiseStartBB;
         if (op == op::Kind::L_AND || op == op::Kind::L_OR) {
            // shortcircuit for logical operators
            boolv = isTruethy(lhs);
            ifBB = state.bb;
            state.bb = otherwiseStartBB = newBB();
         }

         expr_t rhs = parseExpr(spec.precedence + !spec.leftAssociative);

         optArrToPtrDecay(rhs);

         Type resTy = factory_.undefTy();
         Type rTy = rhs->ty;
         Type lTy = lhs->ty;
         value_t result{};
         if (!lTy || !rTy) {
            // todo: better diagnostics
            errorInvalidOpToBinaryExpr(opLoc, lhs, rhs);
         } else if (op == op::Kind::L_AND || op == op::Kind::L_OR) {
            // shortcircuit for logical operators
            value_t otherwisev = isTruethy(rhs);
            bb_t *otherwiseEndBB = state.bb;
            state.bb = newBB();
            if (op == op::Kind::L_OR) {
               emitBranch(ifBB, state.bb, otherwiseStartBB, boolv);
            } else {
               emitBranch(ifBB, otherwiseStartBB, state.bb, boolv);
            }
            emitJump(otherwiseEndBB, state.bb);
            std::array<std::pair<value_t, bb_t *>, 2> phiArgs = {
                std::make_pair(boolv, ifBB),
                std::make_pair(otherwisev, otherwiseEndBB)};
            resTy = factory_.boolTy();
            if (state.eval) {
               result = emitter_.emitPhi(state.bb, resTy, phiArgs);
            }
         } else if (!lTy->isVoidTy() && !rTy->isVoidTy()) {
            Type opTy;
            ssa_t *lhsLVal = nullptr;
            if (op::isAssignmentOp(op)) {
               if (!lhs->mayBeLval) {
                  diagnostics_ << opLoc << "lvalue required as left operand of assignment" << std::endl;
               } else {
                  lhsLVal = std::get<ssa_t *>(lhs->value);
               }
               if (lTy.qualifiers.CONST) {
                  errorAssignToConst(opLoc, lTy, lhs->ident);
                  if (lhs->ident) {
                     noteConstDeclHere(varScope_.find(lhs->ident)->loc, lhs->ident);
                  }
               }
               rhs->value = castAssignmentTarget(opLoc, lTy, rhs);
               opTy = resTy = lTy;
            } else {
               opTy = factory_.commonRealType(lTy, rTy);
               if (op::isComparisonOp(op)) {
                  resTy = factory_.boolTy();
                  if (lTy->isPointerTy() && rTy->isPointerTy()) {
                     opTy = factory_.voidPtrTy(); // todo: ask alexis how to handle this
                     if (!(lTy->isVoidPointerTy() || lTy->isNullPointerTy() || rTy->isVoidPointerTy() || rTy->isNullPointerTy() || lTy->isCompatibleWith(*rTy))) {
                        diagnostics_ << opLoc << "comparison of distinct pointer types" << std::endl;
                     }
                  } else if (!opTy) {
                     diagnostics_ << opLoc << "comparison of distinct types '" << lTy << "' and '" << rTy << '\'' << std::endl;
                  }
               } else {
                  resTy = opTy;
                  if ((!resTy) || (resTy->isFloatingTy() && op::isBitwiseOp(op))) {
                     errorInvalidOpToBinaryExpr(opLoc, lhs, rhs);
                  }
               }
               rhs->value = cast(rhs, opTy);
            }
            if (op != op::Kind::ASSIGN) {
               lhs->value = cast(lhs, opTy);
            }

            if (state.eval) {
               if (op::isAssignmentOp(op) || std::holds_alternative<ssa_t *>(lhs->value) || std::holds_alternative<ssa_t *>(rhs->value)) {
                  result = emitter_.emitBinOp(state.bb, opTy, op, lhs->value, rhs->value, lhsLVal);
               } else {
                  if (!state.bb) {
                     diagnostics_ << pos_->getLoc() << "expression in constant context" << std::endl;
                  } else {
                     result = asValue(emitter_.emitConstBinOp(state.bb, opTy, op, getConst(lhs->value), getConst(rhs->value)));
                  }
               }
            }
         }

         lhs = std::make_unique<Expr<T>>(op, resTy, std::move(lhs), std::move(rhs), result);
      }
   }
   return lhs;
}
// ---------------------------------------------------------------------------
template <typename T>
Parser<T>::expr_t Parser<T>::parseConditionExpr() {
   expr_t cond = parseExpr();
   optArrToPtrDecay(cond);
   cond->value = isTruethy(cond);
   return cond;
}
// ---------------------------------------------------------------------------
template <typename T>
Parser<T>::expr_t Parser<T>::parseAssignmentExpr() {
   return parseExpr(14);
}
// ---------------------------------------------------------------------------
template <typename T>
Parser<T>::expr_t Parser<T>::parseConditionalExpr() {
   return parseExpr(13);
}
// ---------------------------------------------------------------------------
template <typename T>
Parser<T>::expr_t Parser<T>::parseUnaryExpr() {
   return parseExpr(2);
}
// ---------------------------------------------------------------------------
template <typename T>
Parser<T>::expr_t Parser<T>::parsePostfixExpr(expr_t &&lhs) {
   TK kind = pos_->getKind();
   SrcLoc opLoc = pos_->getLoc();
   assert(isPostfixExprStart(kind) && "not a postfix expression start");
   if (consumeAnyOf(TK::INC, TK::DEC)) {
      // postfix increment/decrement
      op::Kind op = kind == TK::INC ? op::Kind::POSTINC : op::Kind::POSTDEC;
      value_t result;
      if (!lhs->mayBeLval) {
         // todo: check if ssa_t is actually there
         diagnostics_ << opLoc << "lvalue required as increment/decrement operand" << std::endl;
      } else if (lhs->ty.qualifiers.CONST) {
         errorAssignToConst(opLoc, lhs->ty, lhs->ident);
      } else if (!lhs->ty->isRealTy() && !lhs->ty->isPtrTy()) {
         const char *op = kind == TK::INC ? "increment" : "decrement";
         diagnostics_ << opLoc << "cannot " << op << " value of type '" << lhs->ty << '\'' << std::endl;
      } else {
         if (state.eval) {
            result = emitter_.emitIncDecOp(state.bb, lhs->ty, op, std::get<ssa_t *>(lhs->value));
         }
         return std::make_unique<Expr<T>>(lhs->loc, op, lhs->ty, std::move(lhs), result);
      }
   } else if (consumeAnyOf(TK::L_BRACKET)) {
      // array subscript
      optArrToPtrDecay(lhs);
      expr_t subscript = parseExpr();
      expect(TK::R_BRACKET);

      Type elemTy;
      value_t result;
      if (!lhs->ty || lhs->ty->kind() != TYK::PTR_T) {
         diagnostics_ << lhs->loc << "subscripted value must be pointer type, not " << lhs->ty << std::endl;
      } else if (!subscript->ty || !subscript->ty->isIntegerTy()) {
         diagnostics_ << (opLoc | subscript->loc) << "array subscript is not an integer" << std::endl;
      } else if (!lhs->ty->getPointedToTy()->isCompleteTy()) {
         diagnostics_ << lhs->loc << "subscripted value is an array of incomplete type" << std::endl;
      } else {
         elemTy = lhs->ty->getPointedToTy();
         if (state.eval) {
            result = emitter_.emitGEP(state.bb, elemTy, asRVal(lhs), asRVal(subscript));
         }
         return std::make_unique<Expr<T>>(op::Kind::SUBSCRIPT, elemTy, std::move(lhs), std::move(subscript), result, true);
      }

   } else if (consumeAnyOf(TK::L_BRACE)) {
      // function call
      Type fnTy{};
      if (lhs->ty && lhs->ty->kind() == TYK::FN_T) {
         fnTy = lhs->ty;
      } else if (lhs->ty && lhs->ty->kind() == TYK::PTR_T && lhs->ty->getPointedToTy()->kind() == TYK::FN_T) {
         fnTy = lhs->ty->getPointedToTy();
         lhs->value = asRVal(lhs);
         lhs->mayBeLval = false;
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
         value_t arg;
         if (fnTy && args.size() < fnTy->getParamTys().size()) {
            arg = cast(expr, fnTy->getParamTys()[args.size()]);
         } else if (fnTy && fnTy->isVarArgFnTy()) {
            arg = cast(expr, factory_.promote(expr->ty));
         } else {
            diagnostics_ << "too many arguments" << std::endl;
         }
         args.push_back(arg);
      }
      if (fnTy && args.size() < fnTy->getParamTys().size()) {
         diagnostics_ << "too few arguments" << std::endl;
      }
      // todo: (jr) not implemented
      expect(TK::R_BRACE);
      ssa_t *ssa = nullptr;
      Type retTy = fnTy ? fnTy->getRetTy() : factory_.undefTy();
      if (state.eval) {
         if (fnTy && lhs->ty->kind() == TYK::FN_T) {
            // call directly
            ssa = emitter_.emitCall(state.bb, std::get<fn_t *>(lhs->value), args);
         } else if (fnTy) {
            // call with function pointer and type
            ssa = emitter_.emitCall(state.bb, fnTy, lhs->value, args);
         } else {
            retTy = factory_.undefTy();
         }
      }

      return std::make_unique<Expr<T>>(pos_.getPrevLoc(), op::Kind::CALL, retTy, std::move(lhs), ssa);
   } else {
      // member access
      consumeAnyOf(TK::DEREF, TK::PERIOD);
      Ident member = expect(TK::IDENT).template getValue<Ident>();

      value_t ptr;

      Type ty = lhs->ty;
      if (kind == TK::DEREF && lhs->ty->kind() != TYK::PTR_T) {
         diagnostics_ << "member access does not have pointer type" << std::endl;
      } else if (kind == TK::DEREF) {
         ptr = asRVal(lhs);
         ty = lhs->ty->getPointedToTy();
      } else {
         ptr = lhs->value;
      }

      value_t result = ptr;
      if (ty->kind() != TYK::STRUCT_T && ty->kind() != TYK::UNION_T) {
         diagnostics_ << lhs->loc << "member access does not have struct or union type" << std::endl;
      } else {
         auto it = std::find_if(ty.membersBegin(), ty.membersEnd(), [&member](const auto &m) { return m.name() == member; });
         if (it == ty.membersEnd()) {
            diagnostics_ << pos_.getPrevLoc() << "no member named '" << member << "' in '" << ty << '\'' << std::endl;
         } else if (!state.bb) {
            diagnostics_ << pos_.getPrevLoc() << "member access in constant expression" << std::endl;
         } else {
            if (ty->kind() == TYK::STRUCT_T) {
               std::span<const std::uint64_t> indices{it.GEPDerefValues()};
               // remove first zero index, because is not needed here
               indices = indices.subspan(1);
               if (state.eval && !(indices.empty() || (indices.size() == 1 && indices.front() == 0))) {
                  result = emitter_.emitGEP(state.bb, ty, ptr, indices);
               }
            }
            return std::make_unique<Expr<T>>(lhs->loc, kind == TK::DEREF ? op::Kind::MEMBER_DEREF : op::Kind::MEMBER, *it, std::move(lhs), result, true);
         }
      }
   }
   return std::make_unique<Expr<T>>(lhs->loc, op::Kind::END, Type(), std::move(lhs), value_t());
}
// ---------------------------------------------------------------------------
template <typename T>
Parser<T>::expr_t Parser<T>::parsePrimaryExpr() {
   expr_t expr{};
   op::Kind kind;
   Token t = *pos_;

   TK tk = t.getKind();
   if (tk >= TK::ICONST && tk <= TK::LDCONST) {
      // Constant Token
      advance();
      Type ty = factory_.fromToken(t);
      value_t value{};
      if (state.eval) {
         if (tk >= TK::FCONST) {
            value = asValue(emitter_.emitFPConst(ty, t.getValue<double>()));
         } else {
            value = asValue(emitter_.emitIConst(ty, t.getValue<unsigned long long>()));
         }
      }
      return std::make_unique<Expr<T>>(t.getLoc(), ty, value);
   } else if (consumeAnyOf(TK::SLITERAL)) {
      // string literals
      std::string_view str = t.getString();
      const_t *sliteral = nullptr;
      if (state.eval) {
         sliteral = emitter_.emitStringLiteral(str);
      }
      Type ty = factory_.arrayOf(factory_.charTy(), str.size() + 1);
      ty = factory_.harden(ty);
      return std::make_unique<Expr<T>>(t.getLoc(), ty, sliteral);
   } else if (consumeAnyOf(TK::CLITERAL)) {
      // character literals
      std::string_view str = t.getString();
      if (str.size() > 1) {
         diagnostics_ << DiagnosticMessage::Kind::WARNING << t.getLoc() << "multi-character character constant" << std::endl;
      }
      int value = 0;
      for (const char c : str) {
         value = (value << 8) | c;
      }
      Type ty = factory_.intTy();
      iconst_t *iconst = nullptr;
      if (state.eval) {
         iconst = emitter_.emitIConst(ty, value);
      }
      return std::make_unique<Expr<T>>(t.getLoc(), ty, iconst);
   } else if (consumeAnyOf(TK::IDENT)) {
      static const Ident FUNC{"__func__"};

      // Identifier
      Ident name = t.getValue<Ident>();
      ScopeInfo *info = varScope_.find(name);
      Type ty{};
      value_t value{};
      if (info) {
         value = info->data();
         ty = info->ty;
      } else if (name == FUNC) {
         if (!state.fnName) {
            diagnostics_ << t.getLoc() << "__func__ outside function" << std::endl;
         } else {
            std::string_view fnName{state.fnName};
            ty = factory_.arrayOf(factory_.charTy(), fnName.size() + 1);
            ty = factory_.harden(ty);
            if (!state._func_) {
               const_t *str = emitter_.emitStringLiteral(fnName);
               state._func_ = emitter_.emitGlobalVar(ty, FUNC + "." + fnName);
               emitter_.setInitValueGlobalVar(state._func_, str);
            }
            value = state._func_;
         }
      } else {
         diagnostics_ << pos_.getPrevLoc().truncate(0) << "use of undeclared identifier '" << name << "'" << std::endl;
      }
      return std::make_unique<Expr<T>>(t.getLoc(), ty, value, name);
   } else if (consumeAnyOf(TK::L_BRACE)) {
      // brace-enclosed expression or type cast
      if (isTypeSpecifierQualifier(pos_->getKind()) || isTypedef()) {
         // type cast
         SrcLoc loc = pos_->getLoc();
         Type ty = parseTypeName();
         expect(TK::R_BRACE);
         if (hasAnyOf(TK::L_C_BRKT)) {
            // compound literal
            value_t val = parseInitializer(ty);
            ssa_t *var = emitter_.emitGlobalVar(ty, state.fnName.prefix(".") + ".compound_literal");
            emitter_.setInitValueGlobalVar(state._func_, getConst(val));
            return std::make_unique<Expr<T>>(loc, ty, var);
         } else {
            expr_t operand = parseExpr(2);
            optArrToPtrDecay(operand);
            value_t value = cast(operand, ty, true);
            return std::make_unique<Expr<T>>(loc, op::Kind::CAST, ty, std::move(operand), value);
         }
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
      Type ty;
      if (brace && (isTypeSpecifierQualifier(pos_->getKind()) || isTypedef())) {
         ty = parseTypeName();
      } else {
         expr = parseUnaryExpr();

         ty = expr->ty;
      }
      if (brace) {
         expect(TK::R_BRACE);
      }
      return std::make_unique<Expr<T>>(pos_->getLoc() | expr->loc, factory_.sizeTy(), emitter_.sizeOf(ty));
   } else if (consumeAnyOf(TK::GENERIC)) {
      // generic selection
      expect(TK::L_BRACE);
      expr_t control = parseAssignmentExpr();
      expect(TK::COMMA);
      do {
         Type ty{};
         if (!consumeAnyOf(TK::DEFAULT)) {
            ty = parseTypeName();
         }
         expect(TK::COLON);
         expr = parseAssignmentExpr(); // todo: not all expressions may be evaluated
      } while (consumeAnyOf(TK::COMMA));
      expect(TK::R_BRACE);
      not_implemented();
   } else if (consumeAnyOf(TK::TRUE, TK::FALSE)) {
      // boolean literals
      Type ty = factory_.boolTy();
      value_t value{};
      if (state.eval) {
         value = emitter_.emitIConst(ty, t.getKind() == TK::TRUE);
      }
      return std::make_unique<Expr<T>>(t.getLoc(), ty, value);
   } else {
      // unary prefix operator
      advance();
      expr_t operand = parseExpr(2);
      if (tk != TK::BW_AND) {
         optArrToPtrDecay(operand);
      }
      Type ty{};
      value_t value{};
      // todo: (jr) not very beautiful
      switch (tk) {
         case TK::INC:
         case TK::DEC:
            if (operand->ty.qualifiers.CONST) {
               errorAssignToConst(t.getLoc(), operand->ty, operand->ident);
            } else {
               kind = (t.getKind() == TK::INC) ? op::Kind::PREINC : op::Kind::PREDEC;
               // todo: (jr) check for lvalue
               if (state.eval) {
                  value = emitter_.emitIncDecOp(state.bb, operand->ty, kind, std::get<ssa_t *>(operand->value));
               }
               ty = operand->ty;
            }
            break;
         case TK::PLUS:
            kind = op::Kind::UNARY_PLUS;
            ty = factory_.promote(operand->ty);
            value = cast(operand, ty);
            break;
         case TK::MINUS:
            kind = op::Kind::UNARY_MINUS;
            ty = factory_.promote(operand->ty);
            value = cast(operand, ty);
            value = emitNeg(ty, value);
            break;
         case TK::NEG:
            kind = op::Kind::L_NOT;
            ty = factory_.boolTy();
            value = isTruethy(operand);
            value = emitNeg(ty, value);
            break;
         case TK::BW_INV:
            kind = op::Kind::BW_NOT;
            ty = factory_.promote(operand->ty);
            value = cast(operand, ty);
            value = emitBWNeg(ty, value);
            break;
         case TK::ASTERISK:
            kind = op::Kind::DEREF;
            if (operand->ty->kind() == TYK::PTR_T) {
               if (!operand->ty->getPointedToTy()->isCompleteTy()) {
                  diagnostics_ << operand->loc << "indirection on pointer to incomplete type '" << operand->ty->getPointedToTy() << "'" << std::endl;
               } else if (state.eval) {
                  value = asRVal(operand);
               }
               ty = operand->ty->getPointedToTy();
            } else {
               diagnostics_ << (t.getLoc() | operand->loc) << "indirection requires pointer operand ('" << operand->ty << "' invalid)" << std::endl;
            }
            break;
         case TK::BW_AND:
            kind = op::Kind::ADDROF;
            if (operand->mayBeLval) {
               value = operand->value;
               ty = factory_.ptrTo(operand->ty);
               ty = factory_.harden(ty);
               factory_.clearFragments();
            } else {
               diagnostics_ << operand->loc << "Expected lvalue" << std::endl;
            }
            break;
         case TK::ALIGNOF:
         case TK::WB_ICONST:
         case TK::UWB_ICONST:
            // todo: (jr) generic selection
            not_implemented(); // todo: (jr) not implemented
            break;
         default:
            diagnostics_ << "Unexpected token for primary expression: " << t.getKind() << std::endl;
            return expr;
      }
      // todo: type
      expr = std::make_unique<Expr<T>>(t.getLoc(), kind, ty, std::move(operand), value);
      if (kind == op::Kind::DEREF) {
         expr->mayBeLval = true;
      }
   }
   return expr;
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::Declarator Parser<T>::parseDeclarator(Type specifierQualifier) {
   Declarator decl = parseDeclaratorImpl();
   if (decl.ty) {
      *decl.base = specifierQualifier;
      if (decl.ty->isFnTy() && !(decl.ty->getRetTy()->isCompleteTy() || decl.ty->getRetTy()->isVoidTy())) {
         diagnostics_ << decl.nameLoc << "incomplete result type '" << specifierQualifier << "' in function declaration" << std::endl;
         *decl.base = factory_.intTy();
      }
      // now, there might be duplicate types
   } else {
      decl.ty = specifierQualifier;
   }

   decl.ty = factory_.harden(decl.ty);
   decl.base = DeclTypeBaseRef{};
   return decl;
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::Type Parser<T>::parseAbstractDeclarator(Type specifierQualifier) {
   Declarator decl = parseDeclarator(specifierQualifier);

   if (decl.ident) {
      diagnostics_ << "Identifier '" << decl.ident << "' not allowed in abstract declarator" << std::endl;
   }

   return decl.ty;
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::Declarator Parser<T>::parseMemberDeclarator(Type specifierQualifier) {
   SrcLoc loc = pos_->getLoc();
   Declarator decl;
   if (isDeclaratorStart(pos_->getKind())) {
      decl = parseDeclarator(specifierQualifier);
   } else {
      decl.ty = specifierQualifier;
   }
   if (consumeAnyOf(TK::COLON)) {
      not_implemented();
   }

   loc |= pos_->getLoc();
   return decl;
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::Declarator Parser<T>::parseDirectDeclarator(Type specifierQualifier) {
   Declarator decl{parseDeclarator(specifierQualifier)};
   if (!decl.ident) {
      diagnostics_ << decl.nameLoc << "Expected identifier in declaration" << std::endl;
   }
   return decl;
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::Declarator Parser<T>::parseDeclaratorImpl() {
   Type lhsTy;
   DeclTypeBaseRef lhsBase;
   Declarator decl{};

   while (consumeAnyOf(TK::ASTERISK)) {
      parseOptAttributeSpecifierSequence();
      // todo: (jr) type-qualifier-list
      Type ty = factory_.ptrTo({});
      while (isTypeQualifier(pos_->getKind())) {
         ty.addQualifier(pos_->getKind());
         advance();
      }
      lhsBase.chain(ty);
      if (!lhsTy) {
         lhsTy = ty;
      }
   }

   Type rhsTy{};
   DeclTypeBaseRef rhsBase{};

   // direct-declarator
   decl.nameLoc = pos_->getLoc();
   if (Token t = consumeAnyOf(TK::IDENT)) {
      decl.ident = t.getValue<Ident>();
   }

   if (!decl.ident && hasAnyOf(TK::L_BRACE)) {
      TK next{pos_.peek()};

      // todo: (jr) non abstract declarator start
      if (isAbstractDeclaratorStart(next)) {
         // abstract-declarator
         expect(TK::L_BRACE);

         Declarator inner = parseDeclaratorImpl();
         rhsTy = inner.ty;
         rhsBase = inner.base;
         decl.ident = inner.ident;
         decl.paramNames = std::move(inner.paramNames);

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
         expr_t expr;

         static_ = consumeOpt(TK::STATIC);

         // todo: (jr) type-qualifier-list only for non abstract declarator
         token::TokenCounter<TK::CONST, TK::ATOMIC> qcount;
         while (isTypeQualifier(pos_->getKind())) {
            ++qcount[pos_->getKind()];
            advance();
         }
         if (consumeOpt(TK::STATIC)) {
            if (static_) {
               diagnostics_ << "static used multiple times" << std::endl;
            }
            static_ = true;
         }

         if (consumeOpt(TK::ASTERISK)) {
            unspecSize = true;
            if (static_) {
               diagnostics_ << "static and * used together" << std::endl;
            }
            if (!decl.ident && std::any_of(qcount.begin(), qcount.end(), [](auto c) { return c > 0; })) {
               diagnostics_ << "Type qualifiers not allowed in abstract declarator" << std::endl;
            }
         } else {
            if (pos_->getKind() != TK::R_BRACKET) {
               unspecSize = false;
               expr = parseAssignmentExpr();
               if (!expr->ty->isIntegerTy()) {
                  diagnostics_ << expr->loc << "size of array has non-integer type '" << expr->ty << "'" << std::endl;
               }
               size = expr->value;
            } else if (static_) {
               diagnostics_ << "Static array must have a size" << std::endl;
            }
         }

         // todo: unspecSize, varLen
         // todo: static
         Type arrTy;
         if (iconst_t **s = std::get_if<iconst_t *>(&size); s && !unspecSize) {
            std::size_t size = 0;
            if (expr->ty->isSignedTy()) {
               long long val = emitter_.getIntegerValue(*s);
               if (val < 0) {
                  diagnostics_ << expr->loc;
                  if (decl.ident) {
                     diagnostics_ << '\'' << decl.ident << "' declared as an array with a negative size" << std::endl;
                  } else {
                     diagnostics_ << "array size must be a positive integer constant expression" << std::endl;
                  }
               } else {
                  size = static_cast<std::size_t>(val);
               }
            } else {
               size = emitter_.getUIntegerValue(*s);
            }
            arrTy = factory_.arrayOf({}, size);
         } else if (ssa_t **s = std::get_if<ssa_t *>(&size); s && !unspecSize) {
            arrTy = factory_.arrayOf({}, *s);
         } else {
            if (!unspecSize) { // todo: this is unreachable
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

         std::vector<Type> paramTys{};
         bool varargs = parseParameterList(paramTys, decl.paramNames);

         expect(TK::R_BRACE, nullptr, "parameter declarator or ')'", std::array<TK, 3>{TK::COMMA, TK::SEMICOLON, TK::L_C_BRKT});

         Type fnTy{factory_.function({}, paramTys, varargs)};
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
template <typename T>
void Parser<T>::parseMemberDeclaratorList(Type specifierQualifier, std::vector<tagged<Type>> &members, std::vector<SrcLoc> &locs) {
   do {
      Declarator decl = parseMemberDeclarator(specifierQualifier);
      auto it = std::find_if(members.begin(), members.end(), [&decl](const auto &m) { return m.name() == decl.ident; });
      if (it != members.end()) {
         diagnostics_ << decl.nameLoc << "duplicate member '" << decl.ident << "'" << std::endl;
         notePrevWhatHere("declaration", locs[std::distance(members.begin(), it)]);
      }
      members.push_back({decl.ident, decl.ty});
      locs.push_back(decl.nameLoc);
   } while (consumeAnyOf(TK::COMMA));
}
// ---------------------------------------------------------------------------
template <typename T>
typename Parser<T>::DeclarationSpecifier Parser<T>::parseDeclarationSpecifierList(bool storageClassSpecifiers, bool functionSpecifiers) {
   // todo: (jr) storageClassSpecifiers and functionSpecifiers not implemented
   (void) storageClassSpecifiers;
   (void) functionSpecifiers;

   DeclarationSpecifier declSpec{};
   Type &ty{declSpec.ty};
   Type *completesTy = nullptr;
   Ident tag;
   SrcLoc tagLoc;

   token::TokenCounter<TK::BOOL, TK::UNSIGNED> tycount{};
   auto &scSpec{declSpec.storageClass};

   bool qualifiers[3] = {false, false, false};
   Token startToken = *pos_;
   for (Token t = *pos_; isDeclarationSpecifier(t.getKind()) || isTypedef(); t = *pos_) {
      TK kind = t.getKind();
      if (isStorageClassSpecifier(kind)) {
         advance();
         if (!storageClassSpecifiers) {
            diagnostics_ << pos_->getLoc() << "storage class specifier not allowed here" << std::endl;
         } else if (scSpec[0] == TK::END) {
            scSpec[0] = kind;
         } else if (scSpec[1] == TK::END) {
            scSpec[1] = kind;
         } else {
            diagnostics_ << pos_->getLoc() << "multiple storage class specifiers" << std::endl;
         }
      } else if (isFunctionSpecifier(kind)) {
         advance();
         if (!functionSpecifiers) {
            diagnostics_ << pos_->getLoc() << "function specifier not allowed here" << std::endl;
         } else {
            int idx = kind == TK::INLINE ? 0 : 1;
            bool &fnSpec = state.functionSpec[idx];
            if (fnSpec) {
               diagnostics_ << pos_->getLoc() << "function specifier '" << kind << "' already set" << std::endl;
            }
            fnSpec = true;
         }
      } else if (isTypeQualifier(kind)) {
         advance();
         qualifiers[static_cast<int>(kind) - static_cast<int>(TK::CONST)] = true;
      } else if (kind == TK::IDENT) {
         auto *tt = typedefScope_.find(t.getValue<Ident>());
         if (tt) {
            advance();
            if (ty) {
               diagnostics_ << pos_->getLoc() << "multiple type specifier not allowed" << std::endl;
            } else {
               ty = static_cast<Type>(*tt);
            }
         } else {
            break;
         }
      } else if (ty) {
         diagnostics_ << pos_->getLoc() << "Type already set" << std::endl;
         break;
      } else if (kind == TK::BITINT) {
         not_implemented(); // todo: (jr) not implemented
      } else if (kind >= TK::BOOL && kind <= TK::UNSIGNED) {
         advance();
         tycount[kind]++;
      } else if (consumeAnyOf(TK::STRUCT, TK::UNION)) {
         // todo: incomplete struct or union
         parseOptAttributeSpecifierSequence();
         if (Token t = consumeAnyOf(TK::IDENT)) {
            tag = t.getValue<Ident>();
            completesTy = static_cast<Type *>(tagScope_.find(tag));
            tagLoc = t.getLoc();
         }
         std::vector<tagged<Type>> members;
         std::vector<SrcLoc> locs{};
         if (consumeAnyOf(TK::L_C_BRKT)) {
            // struct-declaration-list
            while (pos_->getKind() != TK::R_C_BRKT && pos_->getKind() != TK::END) {
               parseOptAttributeSpecifierSequence();
               if (!(isTypeSpecifierQualifier(pos_->getKind()) || isTypedef())) {
                  diagnostics_ << pos_->getLoc() << "type name requires a specifier or qualifier" << std::endl;
                  advance();
                  continue;
               }
               Type memberTy = parseSpecifierQualifierList();
               if ((memberTy->isStructTy() || memberTy->isUnionTy()) && !memberTy->getTag()) {
                  members.push_back({{}, memberTy});
                  locs.push_back(pos_->getLoc());
               } else {
                  parseMemberDeclaratorList(memberTy, members, locs);
               }
               expect(TK::SEMICOLON, "at end of declaration list");
            }
            expect(TK::R_C_BRKT);
            if (members.empty()) {
               diagnostics_ << tagLoc << "struct or union  has no members" << std::endl;
               members.push_back({{}, factory_.intTy()});
            }
            ty = factory_.structOrUnion(kind, members, false, tag);
            std::vector<Ident> names;
            std::transform(ty.membersBegin(), ty.membersEnd(), std::back_inserter(names), [](const auto &m) { return m.name(); });
            for (auto it = names.begin(); it != names.end(); ++it) {
               auto other = std::find(it + 1, names.end(), *it);
               if (other != names.end()) {
                  diagnostics_ << pos_->getLoc() << "duplicate member '" << *it << "' in '" << ty << '\'' << std::endl;
               }
            }
         } else if (!tag) {
            diagnostics_ << "Expected identifier or member declaration but got " << *pos_ << std::endl;
         } else {
            ty = factory_.structOrUnion(kind, {}, true, tag);
         }
      } else if (consumeAnyOf(TK::ENUM)) {
         parseOptAttributeSpecifierSequence();
         Type underlyingTy{};
         Type maxTy{};
         Type currentTy;
         if (Token t = consumeAnyOf(TK::IDENT)) {
            tag = t.getValue<Ident>();
            completesTy = static_cast<Type *>(tagScope_.find(tag));
            tagLoc = t.getLoc();
         }
         if (consumeAnyOf(TK::COLON)) {
            underlyingTy = maxTy = currentTy = parseSpecifierQualifierList();
         } else {
            currentTy = factory_.intTy();
         }
         if (consumeAnyOf(TK::L_C_BRKT)) {
            // enumerator-list
            std::size_t value = static_cast<std::size_t>(-1);
            std::size_t maxIntValue;
            std::size_t maxLongValue;
            if constexpr (T::INT_HAS_64_BIT) {
               maxIntValue = std::numeric_limits<std::int64_t>::max();
            } else {
               maxIntValue = std::numeric_limits<std::int32_t>::max();
            }
            if constexpr (T::LONG_HAS_64_BIT) {
               maxLongValue = std::numeric_limits<std::int64_t>::max();
            } else {
               maxLongValue = std::numeric_limits<std::int32_t>::max();
            }
            do {
               Token enumConstant = expect(TK::IDENT);
               Ident name = enumConstant.getValue<Ident>();
               parseOptAttributeSpecifierSequence();
               if (consumeAnyOf(TK::ASSIGN)) {
                  expr_t constant = parseConditionalExpr();
                  if (iconst_t **val = std::get_if<iconst_t *>(&constant->value)) {
                     if (constant->ty->isSignedTy()) {
                        value = static_cast<std::size_t>(emitter_.getIntegerValue(*val));
                     } else {
                        value = emitter_.getUIntegerValue(*val);
                     }
                     if (!underlyingTy && value > maxIntValue) {
                        currentTy = constant->ty;
                     }
                  } else {
                     diagnostics_ << constant->loc << "enumerator value is not an integer constant expression" << std::endl;
                     continue;
                  }
               } else {
                  ++value;
                  if (value > maxIntValue && currentTy->kind() != TYK::LONG) {
                     TYK kind = TYK::LONG;
                     if (value > maxLongValue) {
                        kind = TYK::LONGLONG;
                     }
                     if (currentTy->kind() != kind) {
                        currentTy = factory_.integralTy(kind, currentTy->signedness());
                        currentTy = factory_.harden(currentTy);
                     }
                  }
               }
               iconst_t *c = emitter_.emitIConst(currentTy, value);
               if (!varScope_.canInsert(name)) {
                  diagnostics_ << enumConstant.getLoc().truncate(0) << "redefinition of enumerator '" << name << "'" << std::endl;
                  notePrevDefHere(varScope_.find(name)->loc);
               } else {
                  varScope_.insert(name, ScopeInfo(currentTy, enumConstant.getLoc(), c, true));
               }
               if (maxTy) {
                  maxTy = factory_.commonRealType(maxTy, currentTy);
               } else {
                  maxTy = currentTy;
               }
            } while (consumeAnyOf(TK::COMMA) && pos_->getKind() != TK::R_C_BRKT);
            consumeAnyOf(TK::COMMA);
            expect(TK::R_C_BRKT);
         } else {
            // todo: assert that attribute-specifier-sequence is empty
         }
         ty = factory_.enumTy(maxTy, !!underlyingTy, tag);
      } else if (consumeAnyOf(TK::TYPEOF, TK::TYPEOF_UNQUAL)) {
         expect(TK::L_BRACE);
         if (isTypeSpecifierQualifier(pos_->getKind()) || isTypedef()) {
            ty = parseTypeName();
         } else {
            state.eval = false;
            ty = parseExpr()->ty;
            state.eval = true;
         }
         if (kind == TK::TYPEOF_UNQUAL) {
            ty = Type::discardQualifiers(ty);
         }
         expect(TK::R_BRACE);
      } else {
         switch (kind) {
            case TK::ATOMIC:
            case TK::ALIGNAS:
               not_implemented(); // todo: (jr) not implemented
               break;
            case TK::VOID:
               ty = factory_.voidTy();
               break;
            default:
               diagnostics_ << pos_->getLoc() << "Unexpected token: " << kind << std::endl;
               break;
         }
         advance();
      }
      parseOptAttributeSpecifierSequence();
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

      type::Sign sign = unsigned_ ? type::Sign::UNSIGNED : type::Sign::SIGNED;
      if (unsigned_ && signed_) {
         diagnostics_ << "specifier singed and unsigned used together" << std::endl;
      }

      if (tycount.consume(TK::CHAR)) {
         // int token cannot be used with char
         tycount[TK::INT] += int_;
         if (!signed_ && !unsigned_) {
            ty = factory_.charTy();
         } else {
            ty = factory_.integralTy(TYK::CHAR, sign);
         }
      } else if (tycount.consume(TK::SHORT)) {
         ty = factory_.integralTy(TYK::SHORT, sign);
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
         ty = factory_.integralTy(TYK::LONG, sign);
      } else if (longs == 2) {
         tycount[TK::LONG] -= longs;
         ty = factory_.integralTy(TYK::LONGLONG, sign);
      } else {
         // must be int
         ty = factory_.integralTy(TYK::INT, sign);
         if (!(int_ || unsigned_ || signed_)) {
            diagnostics_ << pos_->getLoc().truncate(0) << "a type specifier is required for all declarations" << std::endl;
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
   if (completesTy) {
      assert(tag && "Expected tag");
      if (!tagScope_.canInsert(tag) && (*completesTy)->isCompleteTy() && ty->isCompleteTy() && *completesTy != ty) {
         errorRedef(tagLoc, tag);
         notePrevDefHere(tagScope_.find(tag)->loc());
         completesTy = nullptr;
      } else if (!ty->isCompleteTy()) {
         ty = *completesTy;
         completesTy = nullptr;
      }
   }

   ty = factory_.harden(ty, completesTy); // also harden base types
   if (completesTy) {
      *completesTy = ty;
   }
   if (tag) {
      tagScope_.insert(tag, {tagLoc, ty});
   }
   return declSpec;
}
// ---------------------------------------------------------------------------
template <typename T>
Parser<T>::Type Parser<T>::parseSpecifierQualifierList() {
   return parseDeclarationSpecifierList(false, false).ty;
}
// ---------------------------------------------------------------------------
template <typename T>
std::vector<typename Parser<T>::attr_t> Parser<T>::parseOptAttributeSpecifierSequence() {
   // todo: (jr) not implemented
   std::vector<Token> attrs{};
   static Ident GNU_ATTRIBUTE_IDENTIFIER{"__attribute__"};
   bool isGNUAttribute = false;
   if (hasAnyOf(TK::IDENT) && pos_->getValue<Ident>() == GNU_ATTRIBUTE_IDENTIFIER) {
      isGNUAttribute = true;
   }
   while (pos_ && ((hasAnyOf(TK::L_BRACKET) && pos_.peek() == TK::L_BRACKET) || isGNUAttribute)) {
      advance();
      if (isGNUAttribute) {
         advance(2);
      }
      unsigned attrCount = 0;
      // attribute-list
      while (pos_ && ((isGNUAttribute && pos_->getKind() != TK::R_BRACE) || (!isGNUAttribute && pos_->getKind() != TK::R_BRACKET))) {
         // attribute
         if (attrCount) {
            expect(TK::COMMA);
         }

         Token t = expect(TK::IDENT);
         Ident attr = t.getValue<Ident>();
         if (consumeAnyOf(TK::D_COLON)) {
            t = expect(TK::IDENT);
            attr += "::";
            attr += t.getValue<Ident>();
         }
         attrs.push_back(t);
         ++attrCount;
         if (consumeAnyOf(TK::L_BRACE)) {
            // attribute-argument-clause
            std::vector<TK> braces;
            do {
               if (Token t = consumeAnyOf(TK::L_BRACE, TK::L_BRACKET, TK::L_C_BRKT)) {
                  braces.push_back(t.getKind());
               } else if (Token t = consumeAnyOf(TK::R_BRACE, TK::R_BRACKET, TK::R_C_BRKT)) {
                  if (braces.empty() || braces.back() != t.getKind()) {
                     diagnostics_ << t.getLoc() << "unmatched closing brace" << std::endl;
                  } else {
                     braces.pop_back();
                  }
               } else {
                  advance();
               }
            } while (pos_ && !braces.empty());
            expect(TK::L_BRACE);
         }
      }
      advance();
      isGNUAttribute || expect(TK::R_BRACE);
   }
   return attrs;
}
// ---------------------------------------------------------------------------
template <typename T>
Parser<T>::Type Parser<T>::parseTypeName() {
   Token t = *pos_;
   // parse specifier-qualifier-list
   Type ty = parseSpecifierQualifierList();

   // todo: is this needed? if (isAbstractDeclaratorStart(pos_->getKind()))
   auto attr = parseOptAttributeSpecifierSequence();
   Type declTy = parseAbstractDeclarator(ty);

   return declTy;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_PARSER_H
