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
template <typename _EmitterT>
class Parser {
   using trait = emitter::emitter_traits<_EmitterT>;
   using Token = token::Token;
   using TK = token::Kind;

   using TYK = type::Kind;
   using Type = type::Type<_EmitterT>;
   using Factory = type::TypeFactory<_EmitterT>;
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

   using ScopeInfo = ScopeInfo<_EmitterT>;
   using Expr = Expr<_EmitterT>;
   using expr_t = std::unique_ptr<Expr>;

   using value_t = typename _EmitterT::value_t;

   public:
   Parser(std::string_view prog,
          DiagnosticTracker &diagnostics,
          std::ostream &logStream = std::cout) : tokenizer_{prog, diagnostics},
                                                 pos_{tokenizer_.begin()},
                                                 diagnostics_{diagnostics},
                                                 tracer_{logStream},
                                                 factory_{emitter_} {}

   struct SwitchState {
      SwitchState(sw_t *sw) : sw{sw} {}

      sw_t *sw;
      std::vector<iconst_t *> values{};
      std::vector<bb_t *> blocks{};
   };

   struct State {
      Ident fnName;
      fn_t *fn = nullptr;
      bb_t *entry = nullptr;
      bb_t *bb = nullptr;
      Type retTy{};

      ssa_t *_func_ = nullptr;
      std::vector<locateable<bb_t *>> unsealedBlocks{};
      std::unordered_map<Ident, bb_t *> labels{};
      std::unordered_map<Ident, std::vector<locateable<bb_t *>>> incompleteGotos{};
      std::vector<std::pair<bb_t *, value_t>> outstandingReturns{};
      std::vector<std::vector<bb_t *>> missingBreaks{};
      std::vector<bb_t *> continueTargets{};
      std::vector<SwitchState> switches{};
   } state;

   void markSealed(bb_t *bb);

   bool isSealed(bb_t *bb);

   void completeBreaks(bb_t *target);

   _EmitterT &getEmitter() {
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
   void parseOptMemberDeclaratorList(Type specifierQualifier, std::vector<tagged<Type>> &members, std::vector<SrcLoc> &locs);
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
   // todo: remove
   void not_implemented() {
      emitter_.dumpToStdout();
      std::cerr << DiagnosticMessage(diagnostics_, "not implemented\n", pos_->getLoc(), DiagnosticMessage::Kind::NOTE) << std::endl;
      throw std::runtime_error("not implemented");
   }

   bool consumeOpt(TK kind) {
      if (pos_->getKind() == kind) {
         ++pos_;
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

   void emitAssignment(SrcLoc opLoc, Type lTy, ssa_t *lValue, expr_t &rhs) {
      Type rTy = rhs->ty;
      if (!lTy || !rTy) {
         diagnostics_ << opLoc << "invalid operands to binary expression ('" << lTy << "' and '" << rTy << "')" << std::endl;
      } else if (lTy->isArithmeticTy() && rTy->isArithmeticTy()) {
         emitter_.emitStore(state.bb, lTy, cast(rhs, lTy), lValue);
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
            emitter_.emitStore(state.bb, lTy, asRVal(rhs), lValue);
         }
      } else if (lTy->isBoolTy() && rTy->isPointerTy()) {
         emitter_.emitStore(state.bb, lTy, asRVal(rhs), lValue);
      } else {
         diagnostics_ << opLoc << "invalid operands to binary expression ('" << lTy << "' and '" << rTy << "')" << std::endl;
      }
   }

   void emitAssignment(SrcLoc opLoc, expr_t &lhs, expr_t &rhs) {
      if (ssa_t **lValue = std::get_if<ssa_t *>(&lhs->value)) {
         emitAssignment(opLoc, lhs->ty, lValue, rhs);
      } else {
         diagnostics_ << opLoc << "lvalue required as left operand of assignment" << std::endl;
      }
   }

   const_or_iconst_t parseBracedInitializer(Type ty) {
      expect(TK::L_C_BRKT);
      if (consumeAnyOf(TK::R_C_BRKT)) {
         // todo
         return;
      }
      bool hasComma = true;
      while (*pos_ && pos_->getKind() != TK::R_C_BRKT) {
         if (!hasComma) {
            diagnostics_ << pos_->getLoc() << "expected ',' or '}'" << std::endl;
         }

         if (pos_->getKind() == TK::L_C_BRKT) {
            parseBracedInitializer();
         } else {
            parseAssignmentExpr();
         }

         if (consumeAnyOf(TK::COMMA)) {
            hasComma = true;
         }
      }
      expect(TK::R_C_BRKT);
   }

   Type arrToPtrDecay(Type ty) {
      if (ty->isArrayTy()) {
         ty = factory_.ptrTo(ty->getElemTy());
         ty = factory_.harden(ty);
         return ty;
      }
      return ty;
   }

   value_t arrToPtrDecay(expr_t &expr) {
      assert(expr->ty->isArrayTy() && "expr must have array type");
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
         diagnostics_ << " with different types ('" << aTy << "' vs '" << bTy << "')";
      }
      diagnostics_ << std::endl;
   }

   /*template <bool returnCompoundLiteral>
   typename std::conditional<returnCompoundLiteral, const_or_iconst_t, void>::type parseInizializerImpl(Type ty, ssa_t* var) {
      if (consumeAnyOf(TK::L_C_BRKT)) {
         if constexpr (returnCompoundLiteral) {
            const_or_iconst_t value = parseInizializerListImpl<returnCompoundLiteral>(ty, var);
            expect(TK::R_C_BRKT);
            return value;
         } else {
            parseInizializerListImpl<returnCompoundLiteral>(ty, var);
            expect(TK::R_C_BRKT);
         }
      } else {
         expr_t expr = parseAssignmentExpr();
         if constexpr (returnCompoundLiteral) {
            // todo: check if expr is a constant expression
            return cast(expr, ty);
         } else {
            emitter_.emitStore(state.bb, ty, cast(expr, ty), var);
         }
      }
   }

   template <bool returnCompoundLiteral>
   // return void if returnCompoundLiteral is false
   typename std::conditional<returnCompoundLiteral, const_or_iconst_t, void>::type parseInizializerListImpl(Type ty, ssa_t* var) {
      assert(returnCompoundLiteral && !var && "var must be nullptr if returnCompoundLiteral is false");
      if (ty->isScalarTy()) {
         const_or_iconst_t value;
         if
         if (ty->isIntegerTy() || ty->isBoolTy()) {
            value = emitter_.emitIConst(ty, 0);
         } else if (ty->isFloatingTy()) {
            value = emitter_.emitFPConst(ty, 0.0);
         } else if (ty->isPointerTy()) {
            value = emitter_.emitNullPtr(ty);
         } else {
            not_implemented();
         }
         if (returnCompoundLiteral) {
            return value;
         } else {
            emitter_.emitStore(state.bb, ty, value, var);
         }

      } else if (ty->isArrayTy()) {
         if (consumeAnyOf(TK::L_C_BRKT)) {
            std::vector<const_or_iconst_t> values{};
            while (pos_ && pos_->getKind() != TK::R_C_BRKT) {
               if constexpr (!returnCompoundLiteral) {
                  // todo: parse designator
                  if (ty->isFixedSizeArrayTy() && values.size() > ty->getArraySize()) {
                     diagnostics_ << pos_->getLoc() << "excess elements in array initializer" << std::endl;
                  }
                  ssa_t* gep = emitter_.emitGEP(state.bb, ty, var, std::array<std::uint32_t, 2>{0, values.size()});
               }
               values.push_back(inizializerListImpl<returnCompoundLiteral>(ty->getArrayElementType(), nullptr, fn));
               if (consumeAnyOf(TK::COMMA)) {
                  continue;
               }
               break;
            }
            expect(TK::R_C_BRKT);

            if (ty->isFixedArrayTy()) {
               const_or_iconst_t value = defaultInitializerValue(ty->getArrayElementType());
               return emitter_.emitArrayConst(ty, value);
            } else {
               not_implemented();
            }
         } else if (ty->isStructTy()) {
            std::vector<const_or_iconst_t> values{};
            for (auto it = ty->getMember().begin(); it != ty->getMember().end(); ++it) {
               if constexpr (std::is_same_v<std::invoke_result_t<__Fn, ssa_t*>::type, void>) {
                  ssa_t* var = emitter_.emitGEP(state.bb, it->ty, var, std::array<std::uint32_t, 2>{0, std::distance(ty->getMember().begin(), it)});
                  fn(ty, var);
               } else {
                  values.push_back(fn(*it, nullptr))
               }
            }
            if constexpr (!std::is_same_v<std::invoke_result_t<__Fn, ssa_t*>::type, void>) {
               return emitter_.emitStructConst(ty, values);
            }
         } else if (ty->isUnionTy()) {
         }
      }
   }*/

   template <bool returnCompoundLiteral>
   const_or_iconst_t defaultInitializeImpl(Type ty, ssa_t *var) {
      const_or_iconst_t c;
      if (ty->isPointerTy()) {
         c = emitter_.emitNullPtr(ty);
         goto scalar_value;
      } else if (ty->isIntegerTy() || ty->isBoolTy()) {
         c = emitter_.emitIConst(ty, 0);
         goto scalar_value;
      } else if (ty->isFloatingTy()) {
         c = emitter_.emitFPConst(ty, 0.0);
         goto scalar_value;
      } else if (ty->isArrayTy()) {
         if constexpr (returnCompoundLiteral) {
            return emitter_.emitArrayConst(ty, defaultInitializeImpl<true>(ty->getElemTy(), nullptr));
         } else if (ty->isFixedSizeArrayTy()) {
            for (unsigned i = 0; i < ty->getArraySize(); ++i) {
               ssa_t *ptr = emitter_.emitGEP(state.bb, ty->getElemTy(), var, std::array<uint64_t, 2>{0, i});
               defaultInitializeImpl<false>(ty->getElemTy(), ptr);
            }
         } else {
            not_implemented();
         }
      } else if (ty->isAggregateTy()) {
         if constexpr (returnCompoundLiteral) {
            std::vector<const_or_iconst_t> values{};
            for (auto &member : ty->getMembers()) {
               values.push_back(defaultInitializeImpl<true>(member, nullptr));
            }
            return emitter_.emitStructConst(ty, values);
         } else {
            for (unsigned int i = 0; i < ty->getMembers().size(); ++i) {
               Type memberTy = ty->getMembers()[i].ty;
               std::array<uint64_t, 2> indices{0, i};
               ssa_t *ptr = emitter_.emitGEP(state.bb, memberTy, var, indices);
               defaultInitializeImpl<false>(memberTy, ptr);
            }
         }
      } else if (ty->isUnionTy()) {
         if constexpr (returnCompoundLiteral) {
            if (ty->getMembers().empty()) {
               return {};
            }
            return defaultInitializeImpl<true>(ty->getMembers().front(), nullptr);
         } else {
            defaultInitializeImpl<false>(ty->getMembers().front(), var);
         }
      } else {
         not_implemented();
      }
      return {};

   scalar_value:
      if constexpr (returnCompoundLiteral) {
         return c;
      } else {
         emitter_.emitStore(state.bb, ty, c, var);
      }
   }

   void defaultInitialize(Type ty, ssa_t *var) {
      defaultInitializeImpl<false>(ty, var);
   }

   const_or_iconst_t getDefaultValue(Type ty) {
      return defaultInitializeImpl<true>(ty, nullptr);
   }

   Tokenizer tokenizer_;
   typename Tokenizer::const_iterator pos_;
   _EmitterT emitter_{};

   Scope<Ident, ScopeInfo> varScope_{};
   Scope<Ident, locateable<Type>> typeScope_{};

   DiagnosticTracker &diagnostics_;
   Tracer tracer_;
   Factory factory_;

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
   std::vector<ScopeInfo *> externDeclarations_;
   std::vector<ScopeInfo *> missingDefaultInitiations_;

   const Ident FUNC{"__func__"};
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
template <std::size_t N>
token::Token Parser<_EmitterT>::expect(TK kind, const char *where, const char *what, std::array<TK, N> likelyNext, bool usePrevTokenEndLoc) {
   if (pos_->getKind() == kind) {
      Token t{*pos_};
      ++pos_;

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
      ++pos_;
   }
   return Token{};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
token::Token Parser<_EmitterT>::expect(TK kind, const char *where, const char *what, bool usePrevTokenEndLoc) {
   return expect<0>(kind, where, what, {}, usePrevTokenEndLoc);
}
// ---------------------------------------------------------------------------
// emit folded constant expression or operation depending on the type of value
// ---------------------------------------------------------------------------
#ifdef __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename _EmitterT>
template <typename cFn, typename Fn, typename... Args>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::emitUnOpImpl(cFn __cFn, Fn __fn, Type ty, value_t value, Args... args) {
   if (ssa_t **ssa = std::get_if<ssa_t *>(&value)) {
      if (!state.bb) {
         diagnostics_ << pos_->getLoc() << "cannot emit instruction outside of function" << std::endl; // todo: this message should not be necessary
         return emitter_.emitPoison();
      }
      return __fn(state.bb, ty, *ssa, std::forward<Args>(args)...);
   }
   return asValue(__cFn(state.bb, ty, getConst(value), std::forward<Args>(args)...));
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::emitNeg(Type ty, value_t value) {
   return emitUnOpImpl([this](auto &&...args) { return emitter_.emitConstNeg(args...); },
                       [this](auto &&...args) { return emitter_.emitNeg(args...); },
                       ty, value);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::emitBWNeg(Type ty, value_t value) {
   return emitUnOpImpl([this](auto &&...args) { return emitter_.emitConstBWNeg(args...); },
                       [this](auto &&...args) { return emitter_.emitBWNeg(args...); },
                       ty, value);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::emitNot(Type ty, value_t value) {
   return emitUnOpImpl([this](auto &&...args) { return emitter_.emitConstNot(args...); },
                       [this](auto &&...args) { return emitter_.emitNot(args...); },
                       ty, value);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::emitCast(Type fromTy, value_t value, Type toTy, qcp::type::Cast cast) {
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
template <typename _EmitterT>
void Parser<_EmitterT>::notePrevWhatHere(const char *what, SrcLoc loc) {
   diagnostics_ << DiagnosticMessage::Kind::NOTE << loc << "previous " << what << " is here" << std::endl;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::notePrevDeclHere(ScopeInfo *info) {
   assert(info && "info must not be nullptr");
   notePrevWhatHere("declaration", info->loc.truncate(0));
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::notePrevDefHere(SrcLoc loc) {
   notePrevWhatHere("definition", loc.truncate(0));
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::errorVarIncompleteType(SrcLoc loc, Type ty) {
   loc = loc.truncate(0);
   if (ty->isArrayTy()) {
      diagnostics_ << loc << "definition of variable with array type needs an explicit size or an initializer" << std::endl;
   } else {
      diagnostics_ << loc << "variable has incomplete type '" << ty << '\'' << std::endl;
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::errorAssignToConst(SrcLoc loc, Type ty, Ident ident) {
   diagnostics_ << loc << "cannot assign to variable ";
   if (ident) {
      diagnostics_ << '\'' << ident << "' ";
   }
   diagnostics_ << "with const-qualified type '" << ty << '\'' << std::endl;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::noteConstDeclHere(SrcLoc loc, Ident ident) {
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
template <typename _EmitterT>
typename Parser<_EmitterT>::const_or_iconst_t Parser<_EmitterT>::getConst(value_t value) const {
   if (const_t **c = std::get_if<const_t *>(&value)) {
      return *c;
   } else if (iconst_t **c = std::get_if<iconst_t *>(&value)) {
      return *c;
   }
   assert(false && "unreachable"); // todo: llvm unreachable
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::asValue(const_or_iconst_t c) const {
   if (const_t **c_ = std::get_if<const_t *>(&c)) {
      return *c_;
   }
   return std::get<iconst_t *>(c);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::cast(SrcLoc loc, Type from, Type to, value_t value, bool explicitCast) {
   if (from == to) {
      return value;
   }

   if (to->isVoidTy()) {
      return emitter_.emitPoison(); // emitpoison here?
   }

   type::Cast kind;
   if (!from) {
      kind = type::Cast::BITCAST;
   } else if ((from->isIntegerTy() || from->isBoolTy()) && (to->isIntegerTy() || to->isBoolTy())) {
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
      return emitter_.emitPoison();
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
template <typename _EmitterT>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::asRVal(expr_t &expr) {
   if (!expr->ty || (isSealed(state.bb) && expr->mayBeLval)) {
      return emitter_.emitPoison();
   } else if (expr->ty->isArrayTy()) {
      return arrToPtrDecay(expr);
   } else if (expr->mayBeLval && expr->ty->kind() != TYK::FN_T) {
      return emitter_.emitLoad(state.bb, expr->ty, std::get<ssa_t *>(expr->value), expr->ident);
   }
   return expr->value;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::cast(expr_t &expr, Type to, bool explicitCast) {
   return cast(expr->loc, expr->ty, to, asRVal(expr), explicitCast);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::asBoolByComparison(expr_t &expr, op::Kind op) {
   if (isSealed(state.bb)) {
      return emitter_.emitPoison();
   }
   const_or_iconst_t zero;
   if (expr->ty->isIntegerTy()) {
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
template <typename _EmitterT>
typename Parser<_EmitterT>::value_t Parser<_EmitterT>::isTruethy(expr_t &expr) {
   if (!expr->ty) {
      return emitter_.emitPoison();
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
template <typename _EmitterT>
void Parser<_EmitterT>::markSealed(bb_t *bb) {
   auto end = std::remove_if(state.unsealedBlocks.begin(), state.unsealedBlocks.end(), [bb](auto &bbloc) {
      return static_cast<bb_t *>(bbloc) == bb;
   });
   state.unsealedBlocks.erase(end, state.unsealedBlocks.end());
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Parser<_EmitterT>::isSealed(bb_t *bb) {
   return bb && std::find_if(state.unsealedBlocks.begin(), state.unsealedBlocks.end(), [bb](auto &bbloc) {
                   return static_cast<bb_t *>(bbloc) == bb;
                }) == state.unsealedBlocks.end();
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::completeBreaks(bb_t *target) {
   for (auto &bb : state.missingBreaks.back()) {
      emitJump(bb, target);
   }
   state.missingBreaks.pop_back();
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parse() {
   SrcLoc loc;
   while (*pos_) {
      diagnostics_.unsilence();
      loc = pos_->getLoc();
      if (consumeAnyOf(TK::PP_START)) {
         tracer_ << ENTER{"parsePreprocessorDirective"};
         Token lineNo = consumeAnyOf(TK::ICONST, TK::L_ICONST, TK::LL_ICONST);
         Token file = consumeAnyOf(TK::SLITERAL);
         std::vector<int> cmds{};
         while (pos_->getKind() != TK::END && pos_->getKind() != TK::PP_END) {
            if (Token t = consumeAnyOf(TK::ICONST)) {
               cmds.push_back(t.getValue<int>());
            } else {
               ++pos_;
            }
         }
         consumeAnyOf(TK::PP_END);
         if (lineNo && file) {
            std::string fileName{file.getString()};
            diagnostics_.registerFileMapping(fileName, lineNo.getValue<int>());
         }
         tracer_ << EXIT{"parsePreprocessorDirective"};
         continue;
      }
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
      for (auto bbloc : state.unsealedBlocks) {
         if (state.retTy->kind() == TYK::VOID) {
            emitter_.emitRet(static_cast<bb_t *>(bbloc), static_cast<ssa_t *>(nullptr));
         } else {
            diagnostics_ << bbloc.loc() << "missing return statement in function returning non-void" << std::endl;
         }
      }
   }
   for (ScopeInfo *var : missingDefaultInitiations_) {
      if (!var->ty->isCompleteTy()) {
         errorVarIncompleteType(var->loc, var->ty);
         continue;
      }
      const_or_iconst_t c = getDefaultValue(var->ty);
      emitter_.setInitValueGlobalVar(var->ssa(), c);
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Parser<_EmitterT>::parseParameterList(std::vector<Type> &paramTys, std::vector<std::pair<Ident, SrcLoc>> &paramNames) {
   tracer_ << ENTER{"parseParameterList"};
   auto varSg = varScope_.guard();
   auto tySg = typeScope_.guard();
   while (isDeclarationSpecifier(pos_->getKind()) || pos_->getKind() == TK::ELLIPSIS) {
      if (consumeAnyOf(TK::ELLIPSIS)) {
         tracer_ << EXIT{"parseParameterList"};
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
         Type ty = factory_.ptrTo(decl.ty->getElemTy());
         ty.qualifiers = decl.ty.qualifiers;
         decl.ty = factory_.harden(ty);
      }

      paramTys.push_back(decl.ty);
      paramNames.emplace_back(decl.ident, decl.nameLoc);

      if (decl.ident && !varScope_.insert(decl.ident, ScopeInfo(decl.ty, decl.nameLoc))) {
         diagnostics_ << decl.nameLoc.truncate(0) << "redefinition of parameter '" << decl.ident << '\'' << std::endl;
         notePrevDeclHere(varScope_.find(decl.ident));
      }

      tracer_ << "parameter " << std::quoted(static_cast<std::string>(decl.ident)) << ": " << decl.ty << std::endl;
      if (!consumeAnyOf(TK::COMMA)) {
         break;
      }
   }
   tracer_ << EXIT{"parseParameterList"};
   return false;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
int Parser<_EmitterT>::parseOptLabelList(std::vector<attr_t> &attr) {
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
               continue;
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
         ++pos_;
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
void Parser<_EmitterT>::parseDeclStmt(const std::vector<attr_t> &attr) {
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
            diagnostics_ << decl.nameLoc.truncate(0) << "nested function definitions not allowed" << std::endl;
         }

         if (decl.ty->kind() == TYK::FN_T) {
            // set function of state, potentially reusing the function if it was already declared
            state.fn = info ? info->fn() : emitter_.emitFnProto(decl.ty, decl.ident);
            state.retTy = decl.ty->getRetTy();
            state.fnName = decl.ident;

            if (canInsert) {
               info = varScope_.insert(decl.ident, ScopeInfo(decl.ty, decl.nameLoc, state.fn, !!state.entry));
            }

            if (mayBeFunctionDef && pos_->getKind() == TK::L_C_BRKT) {
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
                  decl.ty = factory_.undefTy();
               } else if (info->ty->kind() == TYK::FN_T) {
                  diagnostics_ << "Illegal Inizilizer. Only variables can be initialized" << std::endl;
               } else if (varScope_.isTopLevel() && !canInsert && info->ssa() && !isMissingDefaultInitiation(info->ssa()) && !isExternalDeclaration(info->ssa())) {
                  errorRedef(decl.nameLoc, decl.ident, decl.ty, info->ty);
                  notePrevDefHere(info->loc);
               }
               expr_t initExpr;
               info->hasDefOrInit = true;
               if (consumeAnyOf(TK::L_C_BRKT)) {
                  // initializer-list
                  // todo:
                  // parseInitializerList();
                  not_implemented();
                  expect(TK::R_C_BRKT);
               } else {
                  initExpr = parseAssignmentExpr();
                  if (!varScope_.isTopLevel() && initExpr->ty && initExpr->ty->isArrayTy()) {
                     optArrToPtrDecay(initExpr);
                  }
                  if (ssa_t **ssa = std::get_if<ssa_t *>(&initExpr->value); !state.bb && ssa) {
                     diagnostics_ << initExpr->loc << "initializer element is not a compile-time constant" << std::endl;
                  }
               }
               if (!decl.ty || !initExpr->ty) {
                  continue;
               } else if (decl.ty->isVoidTy() || initExpr->ty->isVoidTy()) {
                  diagnostics_ << decl.nameLoc.truncate(0) << "invalid use of void expresion" << std::endl;
               } else if (isSealed(state.bb)) {
                  // todo: diagnostics_ << "Variable initialization is unreachable" << std::endl;
               } else if (isGlobal) {
                  // todo cast if necessary
                  if (ssa_t **ssa = std::get_if<ssa_t *>(&initExpr->value)) {
                     diagnostics_ << initExpr->loc << "initializer element is not a compile-time constant" << std::endl;
                     emitter_.setInitValueGlobalVar(var, getDefaultValue(decl.ty));
                  } else {
                     emitter_.setInitValueGlobalVar(var, getConst(initExpr->value));
                  }
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

               } else {
                  emitAssignment(assignToken.getLoc(), decl.ty, var, initExpr);
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
template <typename _EmitterT>
void Parser<_EmitterT>::parseCompoundStmt() {
   tracer_ << ENTER{"parseCompoundStmt"};
   expect(TK::L_C_BRKT);

   while (pos_->getKind() != TK::R_C_BRKT && pos_->getKind() != TK::END) {
      auto attr = parseOptAttributeSpecifierSequence();
      parseOptLabelList(attr);
      if (isDeclarationStart(pos_->getKind())) {
         parseDeclStmt(attr);
      } else {
         parseUnlabledStmt(attr);
      }
   }
   expect(TK::R_C_BRKT);
   diagnostics_.unsilence();

   tracer_ << EXIT{"parseCompoundStmt"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseUnlabledStmt(const std::vector<attr_t> &attr) {
   TK kind = pos_->getKind();

   if (kind == TK::L_C_BRKT) {
      // todo: attribute-specifier-sequenceopt
      varScope_.enter();
      typeScope_.enter();
      parseCompoundStmt();
      typeScope_.leave();
      varScope_.leave();
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
   bb_t *fromBB = state.bb;
   bb_t *then = nullptr;
   bb_t *thenEnd = nullptr;
   bb_t *otherwise = nullptr;
   expr_t cond{};
   if (consumeAnyOf(TK::IF)) {
      expect(TK::L_BRACE);
      cond = parseConditionExpr();
      expect(TK::R_BRACE);
      state.bb = then = newBB();
      parseStmt();
      thenEnd = state.bb;
      bb_t *cont = nullptr;
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
      optArrToPtrDecay(cond);
      expect(TK::R_BRACE);
      if (!cond->ty->isIntegerTy()) {
         diagnostics_ << "Switch condition must have integer type" << std::endl;
         cond->value = emitter_.emitPoison();
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
   tracer_ << EXIT{"parseSelectionStmt"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseIterationStmt() {
   tracer_ << ENTER{"parseIterationStmt"};
   bool forLoop = false;
   bool doWhile = false;
   bb_t *fromBB = state.bb;
   std::array<bb_t *, 3> bb{nullptr};
   expr_t cond = nullptr;
   state.missingBreaks.emplace_back();

   if (consumeAnyOf(TK::DO)) {
      doWhile = true;
   } else if (consumeAnyOf(TK::FOR)) {
      forLoop = true;
      varScope_.enter();
      typeScope_.enter();
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
   state.continueTargets.push_back(bb[0]);
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
      typeScope_.leave();
      varScope_.leave();
   }

   completeBreaks(state.bb);
   state.continueTargets.pop_back();
   tracer_ << EXIT{"parseIterationStmt"};
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseJumpStmt() {
   tracer_ << ENTER{"parseJumpStmt"};
   SrcLoc loc = pos_->getLoc().truncate(0);
   if (consumeAnyOf(TK::GOTO)) {
      SrcLoc loc = pos_->getLoc();
      Ident label = expect(TK::IDENT).template getValue<Ident>();
      expect(TK::SEMICOLON);
      if (isSealed(state.bb)) {
         tracer_ << EXIT{"parseJumpStmt"};
         return;
      }
      if (state.labels.contains(label)) {
         emitter_.emitJump(state.bb, state.labels[label]);
      } else {
         state.incompleteGotos[label].emplace_back(state.bb, loc);
      }
      markSealed(state.bb);
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
         tracer_ << EXIT{"parseJumpStmt"};
         return;
      }
      state.outstandingReturns.push_back({state.bb, value});
      markSealed(state.bb);
   } else {
      not_implemented();
   }
   tracer_ << EXIT{"parseJumpStmt"};
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Parser<_EmitterT>::parseFunctionDefinition(Declarator &decl) {
   auto varSg = varScope_.guard();
   auto tySg = typeScope_.guard();
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
}
// ---------------------------------------------------------------------------
// parse expressions
// ---------------------------------------------------------------------------
#ifdef __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseExpr(short minPrec) {
   expr_t lhs = parsePrimaryExpr();
   while (*pos_ && pos_->getKind() >= token::Kind::ASTERISK && pos_->getKind() <= token::Kind::QMARK) {
      TK tk = pos_->getKind();
      if (tk == TK::R_BRACE || tk == TK::R_BRACKET || tk == TK::R_C_BRKT) {
         break;
      }

      if (isPostfixExprStart(pos_->getKind())) {
         lhs = parsePostfixExpr(std::move(lhs));
      } else {
         value_t result;
         SrcLoc opLoc = pos_->getLoc();
         if (tk > TK::COMMA || tk < TK::ASTERISK) {
            diagnostics_ << pos_->getLoc() << "invalid operator" << std::endl;
            ++pos_;
            continue;
         }
         op::Kind op = op::getBinOpKind(tk);
         op::OpSpec spec{op::getOpSpec(op)};

         if (minPrec && spec.precedence >= minPrec) {
            break;
         }
         ++pos_;

         optArrToPtrDecay(lhs);
         bb_t *fromBB = nullptr;
         value_t boolv;
         if (op == op::Kind::L_AND || op == op::Kind::L_OR) {
            // shortcircuit for logical operators
            boolv = isTruethy(lhs);
            fromBB = state.bb;
            state.bb = newBB();
         }

         expr_t rhs = parseExpr(spec.precedence + !spec.leftAssociative);

         optArrToPtrDecay(rhs);

         Type resTy = factory_.undefTy();
         if (!lhs->ty || !rhs->ty) {
            // todo: better diagnostics
            diagnostics_ << opLoc << "invalid operands to binary expression ('" << lhs->ty << "' and '" << rhs->ty << "')" << std::endl;
            goto emit_poisonious_binary_op;
         } else if (!lhs->ty->isVoidTy() && !rhs->ty->isVoidTy()) {
            resTy = factory_.commonRealType(lhs->ty, rhs->ty);

            if (resTy->isFloatingTy() && op::isBitwiseOp(op)) {
               diagnostics_ << opLoc << "invalid operands to binary expression ('" << lhs->ty << "' and '" << rhs->ty << "')" << std::endl;
               resTy = factory_.undefTy();
               goto emit_poisonious_binary_op;
            }
            // todo: check if constexpr should be emitted
            // todo: change type of expr depending on the operation
            if (state.bb) {
               value_t lhsValue = static_cast<ssa_t *>(nullptr);
               value_t rhsValue;
               if (op != op::Kind::ASSIGN) {
                  lhsValue = cast(lhs, resTy);
               }
               rhsValue = cast(rhs, resTy);

               if (op >= op::Kind::ASSIGN && op <= op::Kind::BW_XOR_ASSIGN) {
                  // assign operations
                  if (lhs->ty.qualifiers.CONST) {
                     errorAssignToConst(opLoc, lhs->ty, lhs->ident);
                     if (lhs->ident) {
                        noteConstDeclHere(varScope_.find(lhs->ident)->loc, lhs->ident);
                     }
                     goto emit_poisonious_binary_op;
                  } else {
                     goto non_const_assignment_expr;
                  }
               }
               if (std::holds_alternative<ssa_t *>(lhsValue) || std::holds_alternative<ssa_t *>(rhsValue)) {
               non_const_assignment_expr:
                  ssa_t **lval = std::get_if<ssa_t *>(&lhs->value);
                  result = emitter_.emitBinOp(state.bb, resTy, op, lhsValue, rhsValue, lval ? *lval : nullptr);
               } else {
                  result = asValue(emitter_.emitConstBinOp(state.bb, resTy, op, getConst(lhsValue), getConst(rhsValue)));
               }

               if (isComparisonOp(op)) {
                  resTy = factory_.boolTy();
               }
            }
         } else {
         emit_poisonious_binary_op:
            diagnostics_ << opLoc << "invalid operands to binary expression ('" << lhs->ty << "' and '" << rhs->ty << "')" << std::endl;

            result = emitter_.emitPoison();
         }

         if (op == op::Kind::L_AND || op == op::Kind::L_OR) {
            // shortcircuit for logical operators
            bb_t *otherwise = state.bb;
            state.bb = newBB();
            emitBranch(fromBB, state.bb, otherwise, boolv);
         }

         lhs = std::make_unique<Expr>(op, resTy, std::move(lhs), std::move(rhs), result);
      }
   }
   return lhs;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseConditionExpr() {
   expr_t cond = parseExpr();
   optArrToPtrDecay(cond);
   cond->value = isTruethy(cond);
   return cond;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseAssignmentExpr() {
   return parseExpr(14);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseConditionalExpr() {
   return parseExpr(13);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseUnaryExpr() {
   return parseExpr(2);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parsePostfixExpr(expr_t &&lhs) {
   TK kind = pos_->getKind();
   SrcLoc opLoc = pos_->getLoc();
   assert(isPostfixExprStart(kind) && "not a postfix expression start");
   if (consumeAnyOf(TK::INC, TK::DEC)) {
      // postfix increment/decrement
      op::Kind op = kind == TK::INC ? op::Kind::POSTINC : op::Kind::POSTDEC;
      if (lhs->mayBeLval) {
         // todo: check if ssa_t is actually there
         value_t result = emitter_.emitIncDecOp(state.bb, lhs->ty, op, std::get<ssa_t *>(lhs->value));
         return std::make_unique<Expr>(lhs->loc, op, lhs->ty, std::move(lhs), result);
      } else {
         diagnostics_ << opLoc << "lvalue required as increment/decrement operand" << std::endl;
      }
      return lhs;
   } else if (consumeAnyOf(TK::L_BRACKET)) {
      // array subscript
      optArrToPtrDecay(lhs);
      expr_t subscript = parseExpr();
      optArrToPtrDecay(subscript);
      expect(TK::R_BRACKET);

      Type elemTy;
      value_t result;
      if (!lhs->ty || lhs->ty->kind() != TYK::PTR_T) {
         diagnostics_ << lhs->loc << "subscripted value must be pointer type, not " << lhs->ty << std::endl;
         goto invalid_subscript;
      } else if (subscript->ty && subscript->ty->isIntegerTy()) {
         elemTy = lhs->ty->getPointedToTy();
         if (!elemTy->isCompleteTy()) {
            diagnostics_ << lhs->loc << "subscripted value is an array of incomplete type" << std::endl;
            goto invalid_subscript;
         }
         result = emitter_.emitGEP(state.bb, elemTy, asRVal(lhs), asRVal(subscript));
      } else {
         diagnostics_ << (opLoc | subscript->loc) << "array subscript is not an integer" << std::endl;
      invalid_subscript:
         result = emitter_.emitPoison();
         elemTy = factory_.undefTy();
      }

      return std::make_unique<Expr>(op::Kind::SUBSCRIPT, elemTy, std::move(lhs), std::move(subscript), result, true);
   } else if (consumeAnyOf(TK::L_BRACE)) {
      // function call
      Type fnTy{};
      if (lhs->ty->kind() == TYK::FN_T) {
         fnTy = lhs->ty;
      } else if (lhs->ty->kind() == TYK::PTR_T && lhs->ty->getPointedToTy()->kind() == TYK::FN_T) {
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
            arg = cast(expr, factory_.promote(lhs->ty));
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
      if (fnTy && lhs->ty->kind() == TYK::FN_T) {
         // call directly
         ssa = emitter_.emitCall(state.bb, std::get<fn_t *>(lhs->value), args);
      } else if (fnTy) {
         // call with function pointer and type
         ssa = emitter_.emitCall(state.bb, fnTy, std::get<ssa_t *>(lhs->value), args);
      } else {
         retTy = factory_.undefTy();
         ssa = emitter_.emitPoison();
      }

      return std::make_unique<Expr>(pos_.getPrevLoc(), op::Kind::CALL, retTy, std::move(lhs), ssa);
   } else {
      // member access
      consumeAnyOf(TK::DEREF, TK::PERIOD);
      Ident member = expect(TK::IDENT).template getValue<Ident>();

      value_t ptr = lhs->value;
      Type ty = lhs->ty;
      if (kind == TK::DEREF) {
         if (lhs->ty->kind() != TYK::PTR_T) {
            diagnostics_ << "member access does not have pointer type" << std::endl;
            ptr = emitter_.emitPoison();
            ty = factory_.undefTy();
         } else {
            ty = lhs->ty->getPointedToTy();
         }
      }
      value_t result;
      if (ty->kind() == TYK::STRUCT_T || ty->kind() == TYK::UNION_T) {
         // auto it = std::find_if(ty->membersBegin(), ty->membersEnd(), [&member](const auto& m) { return m.name() == member; });
         auto it = ty->membersBegin();
         while (it != ty->membersEnd() && it->name() != member) {
            ++it;
         }
         if (it == ty->membersEnd()) {
            diagnostics_ << pos_.getPrevLoc() << "no member named '" << member << "' in '" << ty << '\'' << std::endl;
            result = emitter_.emitPoison();
            ty = factory_.undefTy();
         } else if (!state.bb) {
            diagnostics_ << pos_.getPrevLoc() << "member access in constant expression" << std::endl;
         } else {
            if (ty->kind() == TYK::STRUCT_T) {
               std::span<const std::uint32_t> indices{it.GEPDerefValues().begin(), it.GEPDerefValues().size()};
               if (kind == TK::PERIOD) {
                  // remove first zero index if dereferencing is not needed
                  indices = indices.subspan(1);
               }
               result = emitter_.emitGEP(state.bb, ty, ptr, indices);
            } else {
               result = ptr;
            }
            ty = *it;
         }

      } else {
         diagnostics_ << lhs->loc << "member access does not have struct or union type" << std::endl;
         result = emitter_.emitPoison();
         ty = factory_.undefTy();
      }
      return std::make_unique<Expr>(lhs->loc, op::Kind::MEMBER, ty, std::move(lhs), result, true);
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parsePrimaryExpr() {
   expr_t expr{};
   op::Kind kind;
   Token t = *pos_;

   TK tk = t.getKind();
   if (tk >= TK::ICONST && tk <= TK::LDCONST) {
      // Constant Token
      ++pos_;
      Type ty = factory_.fromToken(t);
      value_t value;
      if (tk >= TK::FCONST) {
         value = asValue(emitter_.emitFPConst(ty, t.getValue<double>()));
      } else {
         value = asValue(emitter_.emitIConst(ty, t.getValue<unsigned long long>()));
      }
      return std::make_unique<Expr>(t.getLoc(), ty, value);
   } else if (consumeAnyOf(TK::SLITERAL)) {
      // string literals
      std::string_view str = t.getString();
      const_t *sliteral = emitter_.emitStringLiteral(str);
      Type ty = factory_.arrayOf(factory_.charTy(), str.size() + 1);
      ty = factory_.harden(ty);
      return std::make_unique<Expr>(t.getLoc(), ty, sliteral);
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
      return std::make_unique<Expr>(t.getLoc(), ty, emitter_.emitIConst(ty, value));
   } else if (consumeAnyOf(TK::IDENT)) {
      // Identifier
      Ident name = t.getValue<Ident>();
      ScopeInfo *info = varScope_.find(name);
      Type ty;
      value_t value;
      if (info) {
         if (info->template is<ssa_t *>()) {
            value = info->ssa();
         } else if (info->template is<fn_t *>()) {
            value = info->fn();
         } else {
            value = info->iconst();
         }
         ty = info->ty;
      } else if (name == FUNC) {
         if (!state.fnName) {
            diagnostics_ << t.getLoc() << "__func__ outside function" << std::endl;
            ty = factory_.undefTy();
            value = emitter_.emitPoison();
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
         value = emitter_.emitPoison();
         ty = factory_.undefTy();
      }
      return std::make_unique<Expr>(t.getLoc(), ty, value, name);
   } else if (consumeAnyOf(TK::L_BRACE)) {
      // brace-enclosed expression or type cast
      if (isTypeSpecifierQualifier(pos_->getKind())) {
         // type cast
         SrcLoc loc = pos_->getLoc();
         Type ty = parseTypeName();
         expect(TK::R_BRACE);
         if (pos_->getKind() == TK::L_C_BRKT) {
            // compound literal
            not_implemented();
         }
         expr_t operand = parseExpr(2);
         optArrToPtrDecay(operand);
         value_t value = cast(operand, ty, true);
         return std::make_unique<Expr>(loc, op::Kind::CAST, ty, std::move(operand), value);
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
      if (brace && isTypeSpecifierQualifier(pos_->getKind())) {
         ty = parseTypeName();
      } else {
         expr = parseUnaryExpr();
         ty = expr->ty;
      }
      if (brace) {
         expect(TK::R_BRACE);
      }
      return std::make_unique<Expr>(pos_->getLoc() | expr->loc, factory_.sizeTy(), emitter_.sizeOf(ty));
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
   } else {
      // unary prefix operator
      ++pos_;
      expr_t operand = parseExpr(2);
      if (tk != TK::BW_AND) {
         optArrToPtrDecay(operand);
      }
      Type ty;
      value_t value;
      // todo: (jr) not very beautiful
      switch (tk) {
         case TK::INC:
         case TK::DEC:
            if (operand->ty.qualifiers.CONST) {
               diagnostics_ << operand->loc << "cannot assign to variable with const-qualified type '" << operand->ty << "'" << std::endl;
               value = emitter_.emitPoison();
               ty = factory_.undefTy();
            } else {
               kind = (t.getKind() == TK::INC) ? op::Kind::PREINC : op::Kind::PREDEC;
               // todo: (jr) check for lvalue
               value = emitter_.emitIncDecOp(state.bb, operand->ty, kind, std::get<ssa_t *>(operand->value));
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
                  value = emitter_.emitPoison();
               } else {
                  value = asRVal(operand);
               }
               ty = operand->ty->getPointedToTy();
            } else {
               value = emitter_.emitPoison();
               ty = factory_.undefTy();
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
               value = emitter_.emitPoison();
               ty = factory_.undefTy();
               diagnostics_ << operand->loc << "Expected lvalue" << std::endl;
            }
            break;
         case TK::ALIGNOF:
         case TK::WB_ICONST:
         case TK::UWB_ICONST:
            // todo: (jr) generic selection
            not_implemented(); // todo: (jr) not implemented
         default:
            diagnostics_ << "Unexpected token for primary expression: " << t.getKind() << std::endl;
            return expr;
      }
      // todo: type
      expr = std::make_unique<Expr>(t.getLoc(), kind, ty, std::move(operand), value);
      if (kind == op::Kind::DEREF) {
         expr->mayBeLval = true;
      }
   }
   return expr;
}
// ---------------------------------------------------------------------------
#endif // __IDE_MARKER
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::Declarator Parser<_EmitterT>::parseDeclarator(Type specifierQualifier) {
   tracer_ << ENTER{"parseDeclarator"};
   Declarator decl = parseDeclaratorImpl();
   if (decl.ty) {
      if (decl.ty->isFnTy() && !specifierQualifier->isCompleteTy() && !specifierQualifier->isVoidTy()) {
         diagnostics_ << decl.nameLoc << "incomplete result type '" << specifierQualifier << "' in function declaration" << std::endl;
         *decl.base = factory_.intTy();
      } else {
         *decl.base = specifierQualifier;
      }
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
typename Parser<_EmitterT>::Type Parser<_EmitterT>::parseAbstractDeclarator(Type specifierQualifier) {
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
typename Parser<_EmitterT>::Declarator Parser<_EmitterT>::parseMemberDeclarator(Type specifierQualifier) {
   tracer_ << ENTER{"parseMemberDeclarator"};
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
   tracer_ << EXIT{"parseMemberDeclarator"};
   return decl;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::Declarator Parser<_EmitterT>::parseDirectDeclarator(Type specifierQualifier) {
   tracer_ << ENTER{"parseDirectDeclarator"};
   Declarator decl{parseDeclarator(specifierQualifier)};
   if (!decl.ident) {
      diagnostics_ << decl.nameLoc << "Expected identifier in declaration" << std::endl;
   }
   tracer_ << EXIT{"parseDirectDeclarator"};
   return decl;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::Declarator Parser<_EmitterT>::parseDeclaratorImpl() {
   Type lhsTy;
   DeclTypeBaseRef lhsBase;
   Declarator decl{};

   while (consumeAnyOf(TK::ASTERISK)) {
      parseOptAttributeSpecifierSequence();
      // todo: (jr) type-qualifier-list
      Type ty = factory_.ptrTo({});
      while (isTypeQualifier(pos_->getKind())) {
         ty.addQualifier(pos_->getKind());
         ++pos_;
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
         bool qualifiers[4] = {false, false, false, false};
         bool qualified = false;
         while (isTypeQualifier(pos_->getKind())) {
            qualified = true;
            qualifiers[static_cast<int>(pos_->getKind()) - static_cast<int>(TK::CONST)] = true;
            ++pos_;
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
            if (!decl.ident && qualified) {
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
template <typename _EmitterT>
void Parser<_EmitterT>::parseOptMemberDeclaratorList(Type specifierQualifier, std::vector<tagged<Type>> &members, std::vector<SrcLoc> &locs) {
   parseOptAttributeSpecifierSequence();
   if (isDeclaratorStart(pos_->getKind())) {
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
   } else {
      not_implemented(); // todo: (jr) not implemented
      // static-assert-declaration
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Parser<_EmitterT>::DeclarationSpecifier Parser<_EmitterT>::parseDeclarationSpecifierList(bool storageClassSpecifiers, bool functionSpecifiers) {
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
   for (Token t = *pos_; isDeclarationSpecifier(t.getKind()); t = *pos_) {
      TK type = t.getKind();

      if (isStorageClassSpecifier(type)) {
         ++pos_;
         if (!storageClassSpecifiers) {
            diagnostics_ << pos_->getLoc() << "Storage class specifier not allowed here" << std::endl;
         } else if (scSpec[0] == TK::END) {
            scSpec[0] = type;
         } else if (scSpec[1] == TK::END) {
            scSpec[1] = type;
         } else {
            diagnostics_ << pos_->getLoc() << "Multiple storage class specifiers" << std::endl;
         }
      } else if (isTypeQualifier(type)) {
         ++pos_;
         qualifiers[static_cast<int>(type) - static_cast<int>(TK::CONST)] = true;
      } else if (ty) {
         diagnostics_ << "Type already set" << std::endl;
         break;
      } else if (type == TK::BITINT) {
         not_implemented(); // todo: (jr) not implemented
      } else if (type >= TK::BOOL && type <= TK::UNSIGNED) {
         ++pos_;
         tycount[type]++;
      } else if (consumeAnyOf(TK::STRUCT, TK::UNION)) {
         // todo: incomplete struct or union
         parseOptAttributeSpecifierSequence();
         if (Token t = consumeAnyOf(TK::IDENT)) {
            tag = t.getValue<Ident>();
            completesTy = static_cast<Type *>(typeScope_.find(tag));
            tagLoc = t.getLoc();
         }
         std::vector<tagged<Type>> members;
         std::vector<SrcLoc> locs{};
         if (consumeAnyOf(TK::L_C_BRKT)) {
            // struct-declaration-list
            while (pos_->getKind() != TK::R_C_BRKT && pos_->getKind() != TK::END) {
               parseOptAttributeSpecifierSequence();
               if (!isTypeSpecifierQualifier(pos_->getKind())) {
                  diagnostics_ << pos_->getLoc() << "type name requires a specifier or qualifier" << std::endl;
                  ++pos_;
                  continue;
               }
               Type memberTy = parseSpecifierQualifierList();
               parseOptMemberDeclaratorList(memberTy, members, locs);
               expect(TK::SEMICOLON, "at end of declaration list");
            }
            expect(TK::R_C_BRKT);
            ty = factory_.structOrUnion(type, members, false, tag);
         } else if (!tag) {
            diagnostics_ << "Expected identifier or member declaration but got " << *pos_ << std::endl;
         } else {
            ty = factory_.structOrUnion(type, {}, true, tag);
         }
      } else if (consumeAnyOf(TK::ENUM)) {
         parseOptAttributeSpecifierSequence();
         Type underlyingTy{};
         Type maxTy{};
         Type currentTy;
         if (Token t = consumeAnyOf(TK::IDENT)) {
            tag = t.getValue<Ident>();
            completesTy = static_cast<Type *>(typeScope_.find(tag));
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
            if constexpr (_EmitterT::INT_HAS_64_BIT) {
               maxIntValue = std::numeric_limits<std::int64_t>::max();
            } else {
               maxIntValue = std::numeric_limits<std::int32_t>::max();
            }
            if constexpr (_EmitterT::LONG_HAS_64_BIT) {
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
                        currentTy = factory_.integralTy(kind, currentTy->isSignedTy());
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
      } else if (Token t = consumeAnyOf(TK::TYPEOF, TK::TYPEOF_UNQUAL)) {
         expect(TK::L_BRACE);
         if (isTypeSpecifierQualifier(pos_->getKind())) {
            ty = parseTypeName();
         } else {
            ty = parseExpr()->ty;
         }
         if (t.getKind() == TK::TYPEOF_UNQUAL) {
            ty = Type::discardQualifiers(ty);
         }
         expect(TK::R_BRACE);
      } else {
         switch (type) {
            case TK::TYPEDEF:
            case TK::ATOMIC:
            case TK::ALIGNAS:
               not_implemented(); // todo: (jr) not implemented
               break;
            case TK::VOID:
               ty = factory_.voidTy();
               break;
            default:
               diagnostics_ << "Unexpected token: " << type << std::endl;
               break;
         }
         ++pos_;
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
      if (!typeScope_.canInsert(tag) && (*completesTy)->isCompleteTy() && ty->isCompleteTy() && *completesTy != ty) {
         errorRedef(tagLoc, tag);
         notePrevDefHere(typeScope_.find(tag)->loc());
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
      typeScope_.insert(tag, {tagLoc, ty});
   }
   return declSpec;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::Type Parser<_EmitterT>::parseSpecifierQualifierList() {
   return parseDeclarationSpecifierList(false, false).ty;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
std::vector<typename Parser<_EmitterT>::attr_t> Parser<_EmitterT>::parseOptAttributeSpecifierSequence() {
   // todo: (jr) not implemented
   std::vector<Token> attrs{};
   static Ident GNU_ATTRIBUTE_IDENTIFIER{"__attribute__"};
   bool isGNUAttribute = false;
   if (pos_->getKind() == TK::IDENT && pos_->getValue<Ident>() == GNU_ATTRIBUTE_IDENTIFIER) {
      isGNUAttribute = true;
   }
   while ((pos_->getKind() == TK::L_BRACKET && pos_.peek() == TK::L_BRACKET) || isGNUAttribute) {
      ++pos_;
      if (isGNUAttribute) {
         ++pos_;
      }
      unsigned attrCount = 0;
      // attribute-list
      while ((isGNUAttribute && pos_->getKind() != TK::R_BRACE) || (!isGNUAttribute && pos_->getKind() != TK::R_BRACKET)) {
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
                  ++pos_;
               }
            } while (!braces.empty());
            expect(TK::L_BRACE);
         }
      }
      ++pos_;
      isGNUAttribute || expect(TK::R_BRACE);
   }
   return attrs;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::Type Parser<_EmitterT>::parseTypeName() {
   tracer_ << ENTER{"parseTypeName"};
   Token t = *pos_;
   // parse specifier-qualifier-list
   Type ty = parseSpecifierQualifierList();

   // todo: is this needed? if (isAbstractDeclaratorStart(pos_->getKind()))
   auto attr = parseOptAttributeSpecifierSequence();
   Type declTy = parseAbstractDeclarator(ty);

   tracer_ << EXIT{"parseTypeName"};
   return declTy;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_PARSER_H
