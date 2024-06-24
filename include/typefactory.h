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
   using Type = Type<_EmitterT>;
   using emitter_t = _EmitterT;
   using ssa_t = typename _EmitterT::ssa_t;
   using iconst_t = typename _EmitterT::iconst_t;
   using Token = token::Token;
   using ty_t = typename _EmitterT::ty_t;

   public:
   TypeFactory(emitter_t& emitter) : emitter{emitter}, types(7), typeFragments(1) {
      types[1] = Base(Kind::VOID);
      types[2] = Base(Kind::BOOL);
      types[3] = Base(Kind::CHAR);
      types[4] = Base(Kind::INT, SIGNED);
      types[5] = Base(Kind::INT, UNSIGNED);
      types[6] = Base(Kind::LONGLONG, UNSIGNED);
      for (auto it = types.begin() + 1; it != types.end(); ++it) {
         it->populateEmitterType(*this, emitter);
      }
   }

   // returns a already hardened type
   Type undefTy() {
      return Type{types, 0};
   }

   // returns a already hardened type
   Type voidTy() {
      return Type{types, 1};
   }

   // returns a already hardened type
   Type boolTy() {
      return Type{types, static_cast<unsigned short>(2)};
   }

   // returns a already hardened type
   Type charTy() {
      return Type{types, static_cast<unsigned short>(3)};
   }

   // returns a already hardened type
   Type sizeTy() {
      return Type{types, static_cast<unsigned short>(6)};
   }

   // returns a already hardened type
   Type uintptrTy() {
      return sizeTy();
   }

   // returns a already hardened type
   Type intTy(bool unsignedTy = false) {
      return Type{types, static_cast<unsigned short>(4 + unsignedTy)};
   }

   Type integralTy(Kind integerKind, bool unsignedTy) {
      return Type{construct(integerKind, unsignedTy)};
   }

   Type realTy(Kind realKind) {
      return Type{construct(realKind)};
   }

   Type ptrTo(const Type& other) {
      return Type{construct(other)};
   }

   Type arrayOf(const Type& base) {
      return Type{construct(base, static_cast<ssa_t*>(nullptr), true)};
   }

   Type arrayOf(const Type& base, ssa_t* size, bool unspecifiedSize = false) {
      return Type{construct(base, size, unspecifiedSize)};
   }

   Type arrayOf(const Type& base, std::size_t size, bool unspecifiedSize = false) {
      return Type{construct(base, size, unspecifiedSize)};
   }

   Type enumTy(Type underlyingTy, Ident tag = Ident()) {
      return Type{construct(underlyingTy, tag)};
   }

   // todo: replace with move
   Type function(const Type& retTy, const std::vector<Type>& paramTys, bool isVarArgFnTy = false) {
      return Type{construct(retTy, paramTys, isVarArgFnTy)};
   }

   // todo: replace with move
   Type structOrUnion(token::Kind tk, const std::vector<TaggedType<_EmitterT>>& members, bool incomplete, Ident tag = Ident()) {
      return Type{construct(tk, members, incomplete, tag)};
   }

   Type fromToken(const Token& token) {
      token::Kind tk = token.getKind();
      if (tk >= token::Kind::ICONST && tk <= token::Kind::LDCONST) {
         return fromConstToken(token);
      }

      return Type();
   }

   Type fromConstToken(const Token& token) {
      token::Kind tk = token.getKind();
      assert(tk >= token::Kind::ICONST && tk <= token::Kind::LDCONST && "Invalid token::Kind to create Type from");

      int tkVal = static_cast<int>(tk);
      bool isSigned = tkVal & 1;
      Type ty;
      switch (tk) {
         case token::Kind::ICONST:
         case token::Kind::U_ICONST:
            ty = intTy(!isSigned);
            break;
         case token::Kind::L_ICONST:
         case token::Kind::UL_ICONST:
            ty = Type{construct(Kind::LONG, !isSigned)};
            break;
         case token::Kind::LL_ICONST:
         case token::Kind::ULL_ICONST:
            ty = Type{construct(Kind::LONGLONG, !isSigned)};
            break;
         case token::Kind::FCONST:
         case token::Kind::DCONST:
         case token::Kind::LDCONST:
            ty = Type{construct(static_cast<Kind>(tkVal - static_cast<int>(token::Kind::FCONST) + static_cast<int>(Kind::FLOAT)))};
            break;
         default:
            assert(false && "Invalid token::Kind to create Type from");
      }
      return harden(ty);
   }

   Type promote(Type ty) {
      if (ty->rank() < static_cast<int>(Kind::INT)) {
         return intTy(!ty->isSignedTy());
      }
      // todo: aother probotions: eg bit field
      return ty;
   }

   class DeclTypeBaseRef {
      public:
      DeclTypeBaseRef() : ty{} {}
      DeclTypeBaseRef(const Type& ty) : ty{ty} {}

      Type& operator*() {
         Type* derivedFrom = (*ty.types_)[ty.index_].derivedFrom();
         if (!derivedFrom) {
            std::cerr << "Type '" << ty << "' is not derived from another type" << std::endl;
            assert(false && "Type is not derived from another type");
         }
         return *derivedFrom;
      }

      void chain(Type ty) {
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
      Type ty;
   };

   template <typename... Args>
   std::pair<unsigned short, std::vector<Base>&> construct(Args... args);

   Type harden(Type ty, Type* completesTy = nullptr) {
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
         if (completesTy->types_ == &types) {
            Base& completesBase = types[completesTy->index_];
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
         return {Type(types, types.size() - 1)};
      }
   }

   void clearFragments() {
      typeFragments.clear();
      typeFragments.emplace_back();
   }

   // ---------------------------------------------------------------------------
   Type commonRealType(Type lhs, Type rhs) {
      // todo: undef not necessary in when type of identifier is known
      if (!lhs) {
         return rhs;
      } else if (!rhs) {
         return lhs;
      }

      // Usual arithmetic conversions
      // todo: If one operand has decimal floating type, the other operand shall not have standard floating, complex, or imaginary type.

      if (lhs->isEnumTy()) {
         lhs = lhs->getUnderlyingTy();
      }
      if (rhs->isEnumTy()) {
         rhs = rhs->getUnderlyingTy();
      }

      const Type& higher = lhs > rhs ? lhs : rhs;

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
            Type crt{construct(rhs->kind(), UNSIGNED)};
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