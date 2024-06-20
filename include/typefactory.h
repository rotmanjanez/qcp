#ifndef QCP_TYPE_FACTORY_H
#define QCP_TYPE_FACTORY_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "basetype.h"
#include "operator.h"
#include "token.h"
#include "type.h"
// ---------------------------------------------------------------------------
#include <vector>
// ---------------------------------------------------------------------------
namespace qcp {
namespace type {
// ---------------------------------------------------------------------------
constexpr bool UNSIGNED = true;
constexpr bool SIGNED = false;
// ---------------------------------------------------------------------------
template <typename _EmitterT>
class TypeFactory {
   using Base = Base<_EmitterT>;
   using TY = Type<_EmitterT>;
   using emitter_t = _EmitterT;
   using ssa_t = typename _EmitterT::ssa_t;
   using iconst_t = typename _EmitterT::iconst_t;
   using Token = token::Token;
   using ty_t = typename _EmitterT::ty_t;

   public:
   TypeFactory(emitter_t& emitter) : emitter{emitter}, types(8), typeFragments(1) {
      types[1] = Base(Kind::VOID);
      types[2] = Base(Kind::BOOL);
      types[3] = Base(Kind::CHAR);
      types[4] = Base(Kind::INT, SIGNED);
      types[5] = Base(Kind::INT, UNSIGNED);
      types[6] = Base(Kind::LONGLONG, UNSIGNED);
      types[7] = Base(charTy().addQualifier(token::Kind::CONST));
      for (auto it = types.begin() + 1; it != types.end(); ++it) {
         it->populateEmitterType(*this, emitter);
      }
   }

   // returns a already hardened type
   TY undefTy() {
      return TY{types, 0};
   }

   // returns a already hardened type
   TY voidTy() {
      return TY{types, 1};
   }

   // returns a already hardened type
   TY boolTy() {
      return TY{types, static_cast<unsigned short>(2)};
   }

   // returns a already hardened type
   TY charTy() {
      return TY{types, static_cast<unsigned short>(3)};
   }

   // returns a already hardened type
   TY sizetTy() {
      return TY{types, static_cast<unsigned short>(6)};
   }

   // returns a already hardened type
   TY intTy(bool unsignedTy) {
      return TY{types, static_cast<unsigned short>(4 + unsignedTy)};
   }

   // returns a already hardened type
   TY constCharPtrTy() {
      return TY{types, static_cast<unsigned short>(7)};
   }

   TY integralTy(Kind integerKind, bool unsignedTy) {
      return TY{construct(integerKind, unsignedTy)};
   }

   TY realTy(Kind realKind) {
      return TY{construct(realKind)};
   }

   TY ptrTo(const TY& other) {
      return TY{construct(other)};
   }

   TY arrayOf(const TY& base) {
      return TY{construct(base, static_cast<ssa_t*>(nullptr), true)};
   }

   TY arrayOf(const TY& base, ssa_t* size, bool unspecifiedSize = false) {
      return TY{construct(base, size, unspecifiedSize)};
   }
   TY arrayOf(const TY& base, iconst_t* size, bool unspecifiedSize = false) {
      return TY{construct(base, size, unspecifiedSize)};
   }

   // todo: replace with move
   TY function(const TY& retTy, const std::vector<TY>& paramTys, bool isVarArg = false) {
      return TY{construct(retTy, paramTys, isVarArg)};
   }

   // todo: replace with move
   TY structOrUnion(token::Kind tk, const std::vector<TaggedType<_EmitterT>>& members, bool incomplete, Ident tag = Ident()) {
      return TY{construct(tk, members, incomplete, tag)};
   }

   TY fromToken(const Token& token) {
      token::Kind tk = token.getKind();
      if (tk >= token::Kind::ICONST && tk <= token::Kind::LDCONST) {
         return fromConstToken(token);
      }

      return TY();
   }

   TY fromConstToken(const Token& token) {
      token::Kind tk = token.getKind();
      assert(tk >= token::Kind::ICONST && tk <= token::Kind::LDCONST && "Invalid token::Kind to create TY from");

      int tkVal = static_cast<int>(tk);
      bool hasSigness = tk <= token::Kind::ULL_ICONST;

      int tVal;
      if (hasSigness) {
         tkVal -= static_cast<int>(token::Kind::ICONST);
         // compensate for duplicate tokens with signess
         tVal = static_cast<int>(Kind::INT) + tkVal / 2;
      } else {
         tVal = static_cast<int>(Kind::FLOAT) + tkVal - static_cast<int>(token::Kind::FCONST);
      }

      TY ty{construct(static_cast<Kind>(tVal), hasSigness && (tkVal & 1))};
      return harden(ty);
   }

   TY promote(TY ty) {
      if (ty->rank() < static_cast<int>(Kind::INT)) {
         return intTy(!ty->isSignedTy());
      }
      // todo: aother probotions: eg bit field
      return ty;
   }

   class DeclTypeBaseRef {
      public:
      DeclTypeBaseRef() : ty{} {}
      DeclTypeBaseRef(const TY& ty) : ty{ty} {}

      TY& operator*() {
         TY* derivedFrom = (*ty.types_)[ty.index_].derivedFrom();
         if (!derivedFrom) {
            std::cerr << "Type '" << ty << "' is not derived from another type" << std::endl;
            assert(false && "Type is not derived from another type");
         }
         return *derivedFrom;
      }

      void chain(TY ty) {
         DeclTypeBaseRef& this_ = *this;

         if (this_) {
            *this_ = ty;
            this_ = DeclTypeBaseRef(*this_);
         } else {
            this_ = DeclTypeBaseRef(ty);
         }
      }

      operator bool() const {
         return ty && ty->derivedFrom();
      }

      private:
      TY ty;
   };

   template <typename... Args>
   std::pair<unsigned short, std::vector<Base>&> construct(Args... args);

   TY harden(TY ty, TY* completesTy = nullptr) {
      if (!ty || static_cast<ty_t*>(ty)) {
         return ty;
      }

      DeclTypeBaseRef base{ty};
      if (base) {
         *base = harden(*base);
      }

      // check if there is already a type with the same base
      auto it = std::find(types.begin(), types.end(), *ty);
      if (it != types.end()) {
         assert(!completesTy && "completesTy should be nullptr if type is already hardened");
         ty.index_ = std::distance(types.begin(), it);
         ty.types_ = &types;
         return ty;
      }
      if (completesTy) {
         assert(false && "not implemented yet");
         // if (completesTy->types_ == &types) {
         //    // types[completesTy->index_] = std::move(typeFragments[ty.index_]);
         //    ty.index_ = completesTy->index_;
         // } else {
         //    completesTy->types_->emplace_back(std::move(typeFragments[ty.index_]));
         //    completesTy->index_ = ty.index_ = completesTy->types_->size() - 1;
         //    completesTy->types_ = &types;
         // }
         // ty.types_ = &types;
         return ty;
      } else {
         types.emplace_back(std::move(typeFragments[ty.index_]));
         types.back().populateEmitterType(*this, emitter);
         return {TY(types, types.size() - 1)};
      }
   }

   void clearFragments() {
      typeFragments.clear();
      typeFragments.emplace_back();
   }

   // ---------------------------------------------------------------------------
   TY commonRealType(TY lhs, TY rhs) {
      // todo: undef not necessary in when type of identifier is known
      if (!lhs) {
         return rhs;
      } else if (!rhs) {
         return lhs;
      }

      // Usual arithmetic conversions
      // todo: If one operand has decimal floating type, the other operand shall not have standard floating, complex, or imaginary type.

      const TY& higher = lhs > rhs ? lhs : rhs;

      if (higher->kind() >= Kind::FLOAT) {
         return higher;
      }
      // todo: enums

      // integer promotion
      // todo: what happens with qualifiers?
      lhs = promote(lhs);
      rhs = promote(rhs);

      if (lhs == rhs) {
         return lhs;
      } else if (lhs->isSignedTy() == rhs->isSignedTy()) {
         return lhs > rhs ? lhs : rhs;
      } else {
         if (lhs->isSignedTy()) {
            std::swap(lhs, rhs);
         }
         // now, lhs is unsigned and rhs is signed
         if (lhs >= rhs) {
            return lhs;
         } else {
            // todo: check if representable
            // signed long long + unsigned long -> unsigned long long (wenn beide 64 bit)
            TY crt{construct(rhs->kind(), UNSIGNED)};
            return harden(crt);
         }
      }
   }

   private:
   emitter_t& emitter;
   std::vector<Base> types;
   std::vector<Base> typeFragments;
};
// ---------------------------------------------------------------------------
// TypeFactory
// ---------------------------------------------------------------------------
template <typename _EmitterT>
template <typename... Args>
std::pair<unsigned short, std::vector<Base<_EmitterT>>&> TypeFactory<_EmitterT>::construct(Args... args) {
   auto it = std::find(types.begin(), types.end(), Base{args...});
   if (it != types.end()) {
      return {static_cast<unsigned short>(std::distance(types.begin(), it)), types};
   }
   typeFragments.emplace_back(args...);
   return {typeFragments.size() - 1, typeFragments};
}
// ---------------------------------------------------------------------------
} // namespace type
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TYPE_FACTORY_H