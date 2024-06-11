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
   using Factory = type::TypeFactory<_EmitterT>;
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

   struct State {
      fn_t* fn = nullptr;
      bb_t* entry = nullptr;
      bb_t* bb = nullptr;
      TY retTy{};

      std::unordered_map<Ident, bb_t*> labels{};
      std::unordered_map<Ident, std::vector<bb_t*>> incompleteGotos{};
      std::vector<std::pair<bb_t*, ssa_t*>> missingReturns{};
   } state;

   _EmitterT& getEmitter() {
      return emitter_;
   }

   void parse() {
      tracer_ << ENTER{"parseTranslationUnit"};
      while (*pos_) {
         std::vector<attr_t> attr = parseOptAttributeSpecifierSequence();
         state = State();
         parseDeclStmt(attr);
         for (auto& [lbl, gotos] : state.incompleteGotos) {
            for (bb_t* bb : gotos) {
               if (state.labels.contains(lbl)) {
                  emitter_.emitJump(bb, state.labels[lbl]);
               } else {
                  diagnostics_ << "Undefined label " << lbl << std::endl;
               }
            }
         }
         if (!state.fn || !state.entry) {
            continue;
         }
         // handle gotos and returns
         if (state.missingReturns.size() == 0) {
            if (state.retTy.kind() != TYK::VOID) {
               diagnostics_ << "Missing return statement" << std::endl;
            }
            emitter_.emitRet(state.bb, nullptr);
         } else if (state.missingReturns.size() == 1) {
            emitter_.emitRet(state.missingReturns[0].first, state.missingReturns[0].second);
         } else {
            ssa_t* retVar = emitter_.emitLocalVar(state.fn, state.entry, state.retTy, Ident("__retVar"));
            bb_t* retBB = emitter_.emitBB(Ident("__retBB"), state.fn);
            for (auto [bb, val] : state.missingReturns) {
               emitter_.emitStore(bb, val, retVar);
               emitter_.emitJump(bb, retBB);
            }
            ssa_t* retVal = emitter_.emitLoad(retBB, Ident("__retVar"), state.retTy, retVal);
            emitter_.emitRet(retBB, retVal);
         }
      }
      for (ScopeInfo* var : missingDefaultInitiations_) {
         const_t* c = emitter_.emitConst(var->ty, Ident(), 0);
         emitter_.setInitValueGlobalVar(var->ssa, c);
      }
      tracer_ << EXIT{"parseTranslationUnit"};
   }

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

   void parseStmt();
   void parseCompoundStmt();
   void parseUnlabledStmt(const std::vector<attr_t>& attr);
   void parseDeclStmt(const std::vector<attr_t>& attr);
   void parseFunctionDefinition(Declarator& decl);

   expr_t parseExpr(short midPrec = 0);
   expr_t parseAssignmentExpr() {
      return parseExpr(14);
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

   inline ssa_t* asRVal(expr_t& expr) {
      if (!expr->ty) {
         return emitter_.emitPoison();
      }
      if (expr->mayBeLval() && expr->ty.kind() != TYK::FN_T) {
         return emitter_.emitLoad(state.bb, expr->ident, expr->ty, expr->ssa);
      }
      return expr->ssa;
   }

   ssa_t* cast(TY from, TY to, ssa_t* val) {
      if (from == to) {
         return val;
      }

      type::Cast kind;
      if (!from) {
         kind = type::Cast::BITCAST;
      } else if (from.isIntegerType() && to.isIntegerType()) {
         kind = from.isSigned() ? type::Cast::SEXT : type::Cast::ZEXT;
      } else if (from.isIntegerType() && to.isFloatingType()) {
         kind = from.isSigned() ? type::Cast::SITOFP : type::Cast::UITOFP;
      } else if (from.isFloatingType() && to.isIntegerType()) {
         kind = type::Cast::FPTOSI;
      } else {
         diagnostics_ << "Invalid cast from " << from << " to " << to << std::endl;
         assert(false && "invalid cast");
      }
      return emitter_.emitCast(state.bb, val, to, kind);
   }

   inline ssa_t* parseConditionExpr() {
      bb_t* cond = emitter_.emitBB(Ident("cond"), state.fn);
      emitter_.emitJump(state.bb, cond);
      state.bb = cond;
      expr_t expr = parseExpr();
      return asRVal(expr);
   }

   inline expr_t emitUnOp(Ident name, op::Kind kind, expr_t&& operand) {
      // todo: type
      ssa_t* opVal;
      switch (kind) {
         case op::Kind::PREDEC:
         case op::Kind::PREINC:
         case op::Kind::POSTINC:
         case op::Kind::POSTDEC:
            opVal = operand->ssa;
            break;
         default:
            opVal = asRVal(operand);
      }
      ssa_t* ssa = emitter_.emitUnOp(state.bb, operand->ty, name, kind, opVal);
      return std::make_unique<Expr>(kind, operand->ty, std::move(operand), ssa, name);
   }

   inline expr_t emitUnOp(op::Kind kind, expr_t&& operand) {
      return emitUnOp(Ident(), kind, std::move(operand));
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
// Parser
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
   // todo: (jr) makr not recursive but ruther iterative as long as there are labels
   for (TK kind = pos_->getKind(); isLabelStmtStart(kind, pos_.peek()); kind = pos_->getKind(), ++labelCount) {
      if (!labelTarget) {
         labelTarget = emitter_.emitBB(Ident("label"), state.fn);
         emitter_.emitJump(state.bb, labelTarget);
         state.bb = labelTarget;
      }
      ++pos_;
      if (consumeAnyOf(TK::CASE)) {
         // case-statement
         // expr_t expr = parseExpr();
         assert(false && "not implemented");
      } else if (consumeAnyOf(TK::DEFAULT)) {
         // default-statement
         assert(false && "not implemented");
      } else {
         // labeled-statement
         Ident label = pos_->getValue<Ident>();
         if (state.labels.contains(label)) {
            diagnostics_ << "Redefinition of label " << label << std::endl;
         }
         state.labels[label] = labelTarget;
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
   tracer_ << ENTER{"parseDeclStmt"};

   if (attr.size() > 0 && consumeAnyOf(TK::SEMICOLON)) {
      // todo: (jr) attribute declaration
   } else {
      DeclarationSpecifier declSpec = parseDeclarationSpecifierList();

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
            state.fn = info ? info->fn : emitter_.emitFnProto(decl.ident, decl.ty);
            state.retTy = decl.ty.getRetTy();

            if (pos_->getKind() == TK::L_C_BRKT) {
               // function body
               parseFunctionDefinition(decl);
               goto end_parse_decl;
            }

            if (canInsert) {
               info = scope_.insert(decl.ident, ScopeInfo(decl.ty, state.fn));
            }

         } else {
            bool isGlobal = scope_.isTopLevel() || declSpec.storageClass[0] == TK::STATIC || declSpec.storageClass[1] == TK::STATIC;

            ssa_t* var;
            if (canInsert) {
               var = isGlobal ?
                   emitter_.emitGlobalVar(decl.ty, decl.ident) :
                   emitter_.emitLocalVar(state.fn, state.entry, decl.ty, decl.ident);

               info = scope_.insert(decl.ident, ScopeInfo(decl.ty, var));
            } else {
               var = info->ssa;
            }

            if (consumeAnyOf(TK::ASSIGN)) {
               if (info->ty.kind() == TYK::FN_T) {
                  diagnostics_ << "Illegal Inizilizer. Only variables can be initialized" << std::endl;
               } else if (scope_.isTopLevel() && !canInsert && info->ssa && !isMissingDefaultInitiation(info->ssa) && !isExternalDeclaration(info->ssa)) {
                  diagnostics_ << "Redefinition of variable " << decl.ident << std::endl;
               }
               const_t* value = nullptr;

               if (consumeAnyOf(TK::L_C_BRKT)) {
                  // initializer-list
                  // todo:
                  parseInitializerList();
                  assert(false && "not implemented");
                  expect(TK::R_C_BRKT);
               } else {
                  expr_t expr = parseAssignmentExpr();
                  // todo: track if constant in expr
                  value = static_cast<const_t*>(asRVal(expr));
               }

               if (isGlobal) {
                  emitter_.setInitValueGlobalVar(var, value);

                  auto it = std::find(missingDefaultInitiations_.begin(), missingDefaultInitiations_.end(), info);
                  if (it != missingDefaultInitiations_.end()) {
                     missingDefaultInitiations_.erase(it);
                  }
                  it = std::find(externDeclarations_.begin(), externDeclarations_.end(), info);
                  if (it != externDeclarations_.end()) {
                     externDeclarations_.erase(it);
                  }

               } else {
                  emitter_.setInitValueLocalVar(state.fn, var, value);
               }
            } else if (isGlobal) {
               if (declSpec.storageClass[0] == TK::EXTERN) {
                  externDeclarations_.push_back(info);
               } else {
                  missingDefaultInitiations_.push_back(info);
               }
            }
         }
      } while (consumeAnyOf(TK::COMMA));
      expect(TK::SEMICOLON);
   }
end_parse_decl:
   tracer_ << EXIT{"parseDeclStmt"};
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
   tracer_ << ENTER{"functionDefinition"};
   auto sg = scope_.guard();
   state.entry = state.bb = emitter_.emitFn(state.fn);
   for (unsigned i = 0; i < decl.ty.getParamTys().size(); ++i) {
      TY paramTy = decl.ty.getParamTys()[i];
      Ident paramName = decl.paramNames[i];
      ssa_t* paramValue = emitter_.getParam(state.fn, i);
      ssa_t* localParamValue = emitter_.emitLocalVar(state.fn, state.entry, paramTy, paramName, paramValue);
      scope_.insert(paramName, ScopeInfo(paramTy, localParamValue));
   }

   // function-definition
   parseCompoundStmt();
   // optional return statement for void functions
   if (state.retTy.kind() == TYK::VOID) {
      // emitter_.emitReturn(state.bb, nullptr);
   }

   tracer_ << EXIT{"functionDefinition"};
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
   tracer_ << ENTER{"parseUnlabledStmt"};
   Token t = *pos_;
   TK kind = t.getKind();
   bb_t* cont = nullptr;

   if (kind == TK::L_C_BRKT) {
      // todo: attribute-specifier-sequenceopt
      scope_.enter();
      parseCompoundStmt();
      scope_.leave();
   } else if (isSelectionStmtStart(kind)) {
      // selection-statement
      bb_t* then = emitter_.emitBB(Ident("then"), state.fn);
      bb_t* otherwise = nullptr;
      expr_t cond;

      if (consumeAnyOf(TK::IF)) {
         expect(TK::L_BRACE);
         cond = parseExpr();
         expect(TK::R_BRACE);
         std::swap(state.bb, then);
         parseStmt();
         std::swap(state.bb, then);

         if (consumeAnyOf(TK::ELSE)) {
            otherwise = emitter_.emitBB(Ident("else"), state.fn);
            std::swap(state.bb, otherwise);
            parseStmt();
            std::swap(state.bb, otherwise);
         }

      } else {
         expect(TK::SWITCH);
         expect(TK::L_BRACE);
         cond = parseExpr();
         expect(TK::R_BRACE);
         // todo: parseStmt();
      }

      cont = emitter_.emitBB(Ident("cont"), state.fn);
      if (otherwise) {
         emitter_.emitBranch(state.bb, then, otherwise, asRVal(cond));
         emitter_.emitJump(otherwise, cont);
      } else {
         emitter_.emitBranch(state.bb, then, cont, asRVal(cond));
      }
      emitter_.emitJump(then, cont);
   } else if (isIterationStmtStart(kind)) {
      // iteration-statement
      if (consumeAnyOf(TK::WHILE)) {
         expect(TK::L_BRACE);
         // cond = parseExpr();
         expect(TK::R_BRACE);
         parseStmt();
      } else if (consumeAnyOf(TK::DO)) {
         // emitter_.emitJump(state.bb, body);
         // state.bb= body;
         parseStmt();
         // body = state.bb;
         expect(TK::WHILE);
         expect(TK::L_BRACE);
         // cond = parseConditionExpr();
         // emitter_.emitBranch(state.bb, body, cont, cond);
         expect(TK::R_BRACE);
      } else {
         auto sg = scope_.guard();
         expect(TK::FOR);
         expect(TK::L_BRACE);
         std::vector<attr_t> attr{parseOptAttributeSpecifierSequence()};
         if (isDeclarationStart(pos_->getKind())) {
            parseDeclStmt(attr);
         } else if (pos_->getKind() != TK::SEMICOLON) {
            parseExpr();
         }
         ssa_t* cond = nullptr;
         bb_t* condBB = nullptr;
         bb_t* updateBB = nullptr;
         if (pos_->getKind() != TK::SEMICOLON) {
            cond = parseConditionExpr();
            condBB = state.bb;
         }
         expect(TK::SEMICOLON);
         if (pos_->getKind() != TK::R_BRACE) {
            state.bb = updateBB = emitter_.emitBB(Ident("update"), state.fn);
            parseExpr();
         }
         expect(TK::R_BRACE);
         state.bb = emitter_.emitBB(Ident("body"), state.fn, updateBB);
         parseStmt();
         condBB = condBB ? condBB : state.bb;
         if (updateBB) {
            emitter_.emitJump(updateBB, condBB);
         } else {
            updateBB = condBB;
         }
         emitter_.emitJump(state.bb, updateBB);
         cont = emitter_.emitBB(Ident("cont"), state.fn);
         emitter_.emitBranch(condBB, state.bb, cont, cond);
      }

   } else if (isJumpStmtStart(kind)) {
      // jump-statement
      // todo: (jr) not implemented
      if (consumeAnyOf(TK::GOTO)) {
         Ident label = pos_->getValue<Ident>();
         if (state.labels.contains(label)) {
            emitter_.emitJump(state.bb, state.labels[label]);
         } else {
            state.incompleteGotos[label].push_back(state.bb);
         }
      } else if (consumeAnyOf(TK::CONTINUE)) {
      } else if (consumeAnyOf(TK::BREAK)) {
      } else if (consumeAnyOf(TK::RETURN)) {
         ssa_t* value = nullptr;
         expr_t expr;
         if (pos_->getKind() != TK::SEMICOLON) {
            expr = parseExpr();
            value = asRVal(expr);
         }
         expect(TK::SEMICOLON);
         value = cast(expr->ty, state.retTy, value);
         state.missingReturns.push_back({state.bb, value});
      } else {
         assert(false && "not implemented");
      }
   } else {
      // expression-statement
      expr_t expr;
      if (pos_->getKind() != TK::SEMICOLON) {
         expr = parseExpr();
      }
      expect(TK::SEMICOLON);
      if (!expr && !attr.empty()) {
         diagnostics_ << "Attribute specifier without expression" << std::endl;
      }
   }
   tracer_ << EXIT{"parseUnlabledStmt"};
   if (cont) {
      state.bb = cont;
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parseExpr(short midPrec) {
   tracer_ << ENTER{"parseExpr"};
   expr_t lhs = parsePrimaryExpr();
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
            lhs = emitUnOp(tk == TK::INC ? op::Kind::POSTINC : op::Kind::POSTDEC, std::move(lhs));
            ++pos_;
            continue;
         case TK::L_BRACKET:
            ++pos_;
            // todo: lhs = std::make_unique<Expr>(state.bb, op::Kind::SUBSCRIPT, std::move(lhs), parseExpr(state.bb));
            expect(TK::R_BRACKET);
            continue;
         case TK::DEREF:
         case TK::PERIOD:
            ++pos_;
            assert(pos_->getKind() == TK::IDENT && "member access"); // todo: (jr) not implemented
            // todo: lhs = std::make_unique<Expr>(state.bb, tk == TK::DEREF ? op::Kind::MEMBER_DEREF : op::Kind::MEMBER, std::move(lhs), std::make_unique<Expr>(state.bb, *pos_));
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
                  expr_t expr = parseAssignmentExpr();
                  // todo: type cast
                  args.push_back(asRVal(expr));
               }
               // todo: (jr) not implemented
               expect(TK::R_BRACE);
               ssa_t* ssa = nullptr;
               TY retTy;
               if (lhs->ty.kind() == TYK::FN_T) {
                  ssa = emitter_.emitCall(state.bb, Ident(), static_cast<fn_t*>(lhs->ssa), args);
                  retTy = lhs->ty.getRetTy();
               } else if (lhs->ty.kind() == TYK::PTR_T && lhs->ty.getPointedToTy().kind() == TYK::FN_T) {
                  ssa_t* fn = asRVal(lhs);
                  ssa = emitter_.emitCall(state.bb, Ident(), lhs->ty.getPointedToTy(), fn, args);
                  retTy = lhs->ty.getPointedToTy().getRetTy();
               } else {
                  diagnostics_ << "called object is not a function or function pointer" << std::endl;
               }
               lhs = std::make_unique<Expr>(op::Kind::CALL, retTy, std::move(lhs), ssa);
               continue;
            }
         default:
            // binary operator
            ++pos_;
      }

      op::Kind kind = op::getBinOpKind(tk);
      ssa_t* lhsSsa = (kind != op::Kind::ASSIGN) ? asRVal(lhs) : nullptr;

      expr_t rhs = parseExpr(spec.precedence + !spec.leftAssociative);

      TY resTy = factory_.commonRealType(lhs->ty, rhs->ty);
      lhsSsa = cast(lhs->ty, resTy, lhsSsa);
      ssa_t* rhsSsa = cast(rhs->ty, resTy, asRVal(rhs));

      ssa_t* ssa = emitter_.emitBinOp(state.bb, resTy, Ident(), kind, lhsSsa, rhsSsa, lhs->ssa);

      lhs = std::make_unique<Expr>(kind, resTy, std::move(lhs), std::move(rhs), ssa);
   }
   tracer_ << EXIT{"parseExpr"};
   return lhs;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Parser<_EmitterT>::expr_t Parser<_EmitterT>::parsePrimaryExpr() {
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
         value = emitter_.emitConst(ty, Ident(), t.getValue<int>());
         expr = std::make_unique<Expr>(pos_->getLoc(), ty, value, true);
         // todo:expr->isConstExpr = true;
         ++pos_;
         break;
      case TK::IDENT:
         name = t.getValue<Ident>();
         info = scope_.find(name);
         if (!info) {
            diagnostics_ << t.getLoc() << "Undeclared identifier '" << name << "'" << std::endl;
            value = emitter_.emitPoison();
            ty = factory_.undefTy();
            ty = factory_.harden(ty);
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
         // expr = std::make_unique<Expr>(state.bb, t);
         ++pos_;
         break;

      // todo: (jr) type cast
      case TK::L_BRACE:
         ++pos_;
         if (isTypeSpecifierQualifier(pos_->getKind())) {
            TY ty = parseTypeName();
            expect(TK::R_BRACE);
            expr_t lhs = parseExpr(2);
            // todo: (jr) type cast
            value = cast(lhs->ty, ty, asRVal(lhs));
            expr = std::make_unique<Expr>(op::Kind::CAST, ty, std::move(lhs), value);
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
         expr_t operand = parseExpr(2);
         // todo: type
         expr = emitUnOp(kind, std::move(operand));
   }
   tracer_ << EXIT{"parsePrimaryExpr"};
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
   tracer_ << *pos_ << std::endl;
   tracer_ << "loc: " << loc.loc << std::endl;

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
            varLen = true;
            if (static_) {
               diagnostics_ << "static and * used together" << std::endl;
            }
            if (!decl.ident && qualified) {
               diagnostics_ << "Type qualifiers not allowed in abstract declarator" << std::endl;
            }
         } else {
            if (pos_->getKind() != TK::R_BRACKET) {
               expr_t expr = parseAssignmentExpr();
               if (!expr->ty.isIntegerType()) {
                  diagnostics_ << "Array size must be an integer type" << std::endl;
               }

               // WIP

               unspecSize = false;
            } else if (static_) {
               diagnostics_ << "Static array must have a size" << std::endl;
            }
         }

         // todo: unspecSize, varLen
         // todo: static
         TY arrTy{factory_.arrayOf({}, size, unspecSize)};
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

   TokenCounter<TK::BOOL, TK::COMPLEX> tycount{};
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
   auto attr = parseOptAttributeSpecifierSequence();
   TY declTy = parseAbstractDeclarator(ty);

   tracer_ << "type-name: " << ty << std::endl;
   tracer_ << EXIT{"parseTypeName"};
   return declTy;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_PARSER_H
