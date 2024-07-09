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
template <typename T>
class TypeFactory {
   using Ty = Type<T>;
   using emitter_t = T;
   using ssa_t = typename T::ssa_t;
   using iconst_t = typename T::iconst_t;
   using Token = token::Token;
   using ty_t = typename T::ty_t;

   public:
   TypeFactory(emitter_t& emitter) : emitter{emitter}, types(8), typeFragments(1) {
      types[1] = Base<T>(Kind::VOID);
      types[2] = Base<T>(Kind::BOOL);
      types[3] = Base<T>(Kind::CHAR);
      types[4] = Base<T>(Kind::INT, Sign::SIGNED);
      types[5] = Base<T>(Kind::INT, Sign::UNSIGNED);
      types[6] = Base<T>(Kind::LONGLONG, Sign::UNSIGNED);
      types[7] = Base<T>(voidTy());
      for (auto it = types.begin() + 1; it != types.end(); ++it) {
         it->populateEmitterType(*this, emitter);
      }
   }

   // returns a already hardened type
   Ty undefTy() {
      return Ty{types, 0};
   }

   // returns a already hardened type
   Ty voidTy() {
      return Ty{types, 1};
   }

   // returns a already hardened type
   Ty boolTy() {
      return Ty{types, static_cast<unsigned short>(2)};
   }

   // returns a already hardened type
   Ty charTy() {
      return Ty{types, static_cast<unsigned short>(3)};
   }

   // returns a already hardened type
   Ty sizeTy() {
      return Ty{types, static_cast<unsigned short>(6)};
   }

   // returns a already hardened type
   Ty uintptrTy() {
      return sizeTy();
   }

   // returns a already hardened type
   Ty intTy(Sign signedness = Sign::SIGNED) {
      assert (signedness != Sign::UNSPECIFIED && "Sign must be specified");
      unsigned short unsignedTy = signedness == Sign::UNSIGNED ? 1 : 0;
      return Ty{types, static_cast<unsigned short>(4 + unsignedTy)};
   }

   // returns a already hardened type
   Ty voidPtrTy() {
      return Ty{types, 7};
   }


   Ty integralTy(Kind integerKind, Sign signedness) {
      return Ty{construct(integerKind, signedness)};
   }

   Ty realTy(Kind realKind) {
      return Ty{construct(realKind)};
   }

   Ty ptrTo(const Ty& other) {
      return Ty{construct(other)};
   }

   Ty arrayOf(const Ty& base) {
      return Ty{construct(base, static_cast<ssa_t*>(nullptr), true)};
   }

   Ty arrayOf(const Ty& base, ssa_t* size, bool unspecifiedSize = false) {
      return Ty{construct(base, size, unspecifiedSize)};
   }

   Ty arrayOf(const Ty& base, std::size_t size, bool unspecifiedSize = false) {
      return Ty{construct(base, size, unspecifiedSize)};
   }

   Ty enumTy(Ty underlyingTy, bool fixedUnerlyingTy, Ident tag = Ident()) {
      return Ty{construct(underlyingTy, fixedUnerlyingTy, tag)};
   }

   // todo: replace with move
   Ty function(const Ty& retTy, const std::vector<Ty>& paramTys, bool isVarArgFnTy = false) {
      return Ty{construct(retTy, paramTys, isVarArgFnTy)};
   }

   // todo: replace with move
   Ty structOrUnion(token::Kind tk, const std::vector<tagged<Ty>>& members, bool incomplete, Ident tag = Ident()) {
      return Ty{construct(tk, members, incomplete, tag)};
   }

   Ty fromToken(const Token& token) {
      token::Kind tk = token.getKind();
      if (tk >= token::Kind::ICONST && tk <= token::Kind::LDCONST) {
         return fromConstToken(token);
      }

      return Ty();
   }

   Ty fromConstToken(const Token& token) {
      token::Kind tk = token.getKind();
      assert(tk >= token::Kind::ICONST && tk <= token::Kind::LDCONST && "Invalid token::Kind to create Ty from");

      int tkVal = static_cast<int>(tk);
      Sign sign = tkVal & 1 ? Sign::SIGNED : Sign::UNSIGNED;
      Ty ty;
      switch (tk) {
         case token::Kind::ICONST:
         case token::Kind::U_ICONST:
            ty = intTy(sign);
            break;
         case token::Kind::L_ICONST:
         case token::Kind::UL_ICONST:
            ty = Ty{construct(Kind::LONG, sign)};
            break;
         case token::Kind::LL_ICONST:
         case token::Kind::ULL_ICONST:
            ty = Ty{construct(Kind::LONGLONG, sign)};
            break;
         case token::Kind::FCONST:
         case token::Kind::DCONST:
         case token::Kind::LDCONST:
            ty = Ty{construct(static_cast<Kind>(tkVal - static_cast<int>(token::Kind::FCONST) + static_cast<int>(Kind::FLOAT)))};
            break;
         default:
            assert(false && "Invalid token::Kind to create Ty from");
      }
      return harden(ty);
   }

   Ty promote(Ty ty) {
      if (ty->isArithmeticTy() && ty->rank() < static_cast<int>(Kind::INT)) {
         Sign signedess = ty->isSignedTy() ? Sign::SIGNED : Sign::UNSIGNED;
         return intTy(signedess);
      }
      // todo: aother probotions: eg bit field
      return ty;
   }

   class DeclTypeBaseRef {
      public:
      DeclTypeBaseRef() : ty{} {}
      DeclTypeBaseRef(const Ty& ty) : ty{ty} {}

      Ty& operator*() {
         Ty* derivedFrom = (*ty.types_)[ty.index_].derivedFrom();
         if (!derivedFrom) {
            std::cerr << "Ty '" << ty << "' is not derived from another type" << std::endl;
            assert(false && "Ty is not derived from another type");
         }
         return *derivedFrom;
      }

      void chain(Ty ty) {
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
      Ty ty;
   };

   template <typename... Args>
   std::pair<unsigned short, std::vector<Base<T>>&> construct(Args... args);

   Ty harden(Ty ty, Ty* completesTy = nullptr) {
      if (!ty || ty.types_ == &types) {
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
         if (completesTy->types_ == &types) {
            Base<T>& completesBase = types[completesTy->index_];
            completesBase = std::move(typeFragments[ty.index_]);
            completesBase.ref_ = nullptr;
            completesBase.populateEmitterType(*this, emitter);
            ty.index_ = completesTy->index_;
         } else {
            assert(false && "must be hardened"); // todo: tchnically, i think it must not, but anyways
            // completesTy->types_->emplace_back(std::move(typeFragments[ty.index_]));
            // completesTy->index_ = ty.index_ = completesTy->types_->size() - 1;
            // completesTy->types_ = &types;
         }
         ty.types_ = &types;
         return ty;
      } else {
         types.emplace_back(std::move(typeFragments[ty.index_]));
         types.back().populateEmitterType(*this, emitter);
         return {Ty(types, types.size() - 1)};
      }
   }

   void clearFragments() {
      typeFragments.clear();
      typeFragments.emplace_back();
   }

   // ---------------------------------------------------------------------------
   Ty commonRealType(Ty lhs, Ty rhs) {
      // todo: undef not necessary in when type of identifier is known
      if (!lhs || !rhs){
         return rhs;
      } else if (!rhs) {
         return lhs;
      } else if ( !lhs->isBasicTy() || !rhs->isBasicTy()) {
         return Ty();
      }

      // Usual arithmetic conversions
      // todo: If one operand has decimal floating type, the other operand shall not have standard floating, complex, or imaginary type.

      if (lhs->isEnumTy()) {
         lhs = lhs->getUnderlyingTy();
      }
      if (rhs->isEnumTy()) {
         rhs = rhs->getUnderlyingTy();
      }

      const Ty& higher = lhs > rhs ? lhs : rhs;

      if (higher->kind() >= Kind::FLOAT) {
         return higher;
      }

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
            Ty crt{construct(rhs->kind(), Sign::UNSIGNED)};
            return harden(crt);
         }
      }
   }

   private:
   emitter_t& emitter;
   std::vector<Base<T>> types;
   std::vector<Base<T>> typeFragments;
};
// ---------------------------------------------------------------------------
// TypeFactory
// ---------------------------------------------------------------------------
template <typename T>
template <typename... Args>
std::pair<unsigned short, std::vector<Base<T>>&> TypeFactory<T>::construct(Args... args) {
   auto it = std::find(types.begin(), types.end(), Base<T>{args...});
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