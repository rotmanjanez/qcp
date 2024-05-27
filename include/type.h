#ifndef QCP_TYPE_H
#define QCP_TYPE_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "operator.h"
#include "token.h"
// ---------------------------------------------------------------------------
#include <vector>
// ---------------------------------------------------------------------------
namespace qcp {
namespace type {
// ---------------------------------------------------------------------------
enum class Kind {
   // clang-format off
   BOOL, CHAR, SHORT, INT, LONG, LONGLONG,
   
   FLOAT, DOUBLE, LONGDOUBLE,

   DECIMAL32, DECIMAL64, DECIMAL128,
   COMPLEX,

   NULLPTR_T,
   VOID,

   PTR_T, ARRAY_T, STRUCT_T, ENUM_T, UNION_T, FN_T,
   UNDEF,
   // clang-format on
};
// ---------------------------------------------------------------------------
template <typename _EmitterT>
struct Base;
// ---------------------------------------------------------------------------
template <typename _EmitterT>
class BaseFactory;
// ---------------------------------------------------------------------------
template <typename _EmitterT>
struct TaggedType;
// ---------------------------------------------------------------------------
template <typename _EmitterT>
class Type {
   using Base = Base<_EmitterT>;
   using Factory = BaseFactory<_EmitterT>;
   using Token = token::Token;

   template <typename T>
   friend std::ostream& operator<<(std::ostream& os, const Type<T>& ty);

   friend Factory;

   friend typename Factory::DeclTypeBaseRef;

   explicit Type(std::pair<unsigned short, std::vector<Base>&> ini) : index_{ini.first}, types_{&ini.second} {}

   public:
   Type() : index_{0}, types_(nullptr) {}

   Type(const Type& other) = default;
   Type(Type&& other) = default;
   Type& operator=(const Type& other) = default;
   Type& operator=(Type&& other) = default;

   static Type discardQualifiers(const Type& other) {
      Type ty{other};
      ty.qualifiers = {};
      return ty;
   }

   static Type qualified(const Type& other, token::Kind kind) {
      Type ty{other};
      ty.addQualifier(kind);
      return ty;
   }

   void addQualifier(token::Kind kind) {
      switch (kind) {
         case token::Kind::CONST:
            qualifiers.CONST = true;
            break;
         case token::Kind::RESTRICT:
            qualifiers.RESTRICT = true;
            break;
         case token::Kind::VOLATILE:
            qualifiers.VOLATILE = true;
            break;
         // todo: case token::Kind::ATOMIC:
         // todo:    ty.qualifiers.ATOMIC = true;
         // todo:    break;
         default:
            assert(false && "Invalid token::Kind to qualify Type with");
            break;
      }
   }

   Type promote() const {
      // todo: assert(Base().kind <= Kind::LONGLONG && "Invalid Base::Kind to promote()");
      // todo: promote integers
      return *this;
   }

   Kind kind() const {
      return base().kind;
   }

   Type getRetTy() const {
      return base().fnTy.retTy;
   }

   const std::vector<Type>& getParamTys() const {
      return base().fnTy.paramTys;
   }

   Type getElemTy() const {
      return base().arrayTy.elemTy;
   }

   const std::vector<TaggedType<_EmitterT>>& getMembers() const {
      return base().structOrUnionTy.members;
   }

   static Type commonRealType(op::Kind op, const Type& lhs, const Type& rhs);

   const Base& base() const {
      assert(types_ && "Type is not initialized");
      return (*types_)[index_];
   }

   typename _EmitterT::ty_t* emitterType() const {
      return base().ref;
   }

   bool operator==(const Type& other) const {
      return qualifiers == other.qualifiers && index_ == other.index_;
   }

   auto operator<=>(const Type& other) const {
      return base().rank() <=> other.base().rank();
   }

   operator bool() const {
      return index_ != 0 && types_ != nullptr;
   }

   struct qualifiers {
      bool operator==(const qualifiers& other) const = default;
      bool operator!=(const qualifiers& other) const = default;

      bool CONST = false;
      bool RESTRICT = false;
      bool VOLATILE = false;
      // todo: bool ATOMIC = false;
   } qualifiers;

   private:
   // todo: maybe short is not enough
   unsigned short index_;
   std::vector<Base>* types_;
};
// ---------------------------------------------------------------------------
template <typename _EmitterT>
struct TaggedType {
   using Type = Type<_EmitterT>;

   TaggedType(const TaggedType& other) = default;
   TaggedType(TaggedType&& other) = default;
   TaggedType& operator=(const TaggedType& other) = default;
   TaggedType& operator=(TaggedType&& other) = default;

   TaggedType(Ident name, Type ty) : name{name}, ty{ty} {}

   bool operator==(const TaggedType& other) const {
      return ty == other.ty;
   }

   auto operator<=>(const TaggedType& other) const {
      return ty <=> other.ty;
   }

   operator bool() const {
      return ty;
   }

   template <typename T>
   friend std::ostream& operator<<(std::ostream& os, const TaggedType<T>& ty);

   Ident name{};
   Type ty{};
};
// ---------------------------------------------------------------------------
template <typename _EmitterT>
std::ostream& operator<<(std::ostream& os, const TaggedType<_EmitterT>& ty) {
   if (ty.name) {
      os << ty.name << ": ";
   }
   os << ty.ty;
   return os;
}
// ---------------------------------------------------------------------------
// Type
// ---------------------------------------------------------------------------
template <typename _EmitterT>
std::ostream& operator<<(std::ostream& os, const Type<_EmitterT>& ty) {
   if (ty.qualifiers.CONST) {
      os << "const ";
   }
   if (ty.qualifiers.RESTRICT) {
      os << "restrict ";
   }
   if (ty.qualifiers.VOLATILE) {
      os << "volatile ";
   }
   // todo: if (ty.qualifiers.ATOMIC) {
   // todo:    os << "_Atomic ";
   // todo: }
   os << ty.base();
   return os;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Type<_EmitterT> Type<_EmitterT>::commonRealType(op::Kind kind, const Type& lhs, const Type& rhs) {
   // todo: undef not necessary in when type of identifier is known
   if (lhs.base().kind == Kind::UNDEF || rhs.base().kind == Kind::UNDEF) {
      return Type();
   }

   // Usual arithmetic conversions
   // todo: If one operand has decimal floating type, the other operand shall not have standard floating, complex, or imaginary type.

   if (kind <= op::Kind::ALIGNOF) {
      // todo: differtent unary operators yield different types. replace by a switch statement
      return Type::discardQualifiers(lhs);
   }

   const Type& higher = lhs > rhs ? lhs : rhs;

   if (higher.base().kind >= Kind::FLOAT) {
      return Type::discardQualifiers(higher);
   }
   // todo: enums

   // integer promotion
   // todo: what happens with qualifiers?
   const Type lhsPromoted{lhs.promote()};
   const Type rhsPromoted{rhs.promote()};

   if (lhsPromoted == rhsPromoted) {
      return Type::discardQualifiers(lhsPromoted);
   } else if (lhsPromoted.base().unsingedTy == rhsPromoted.base().unsingedTy) {
      return Type::discardQualifiers(lhsPromoted > rhsPromoted ? lhsPromoted : rhsPromoted);
   } else {
      const Type& unsignedTy = lhsPromoted.base().unsingedTy ? lhsPromoted : rhsPromoted;
      const Type& signedTy = lhsPromoted.base().unsingedTy ? rhsPromoted : lhsPromoted;
      if (unsignedTy >= signedTy) {
         return Type::discardQualifiers(unsignedTy);
      } else { //  if (signedTy > unsignedTy) {
         return Type::discardQualifiers(signedTy);
      }

      // } else {
      // todo: (jr) what to do here?
      // return Type(signedTy.base().kind, true);
      // }
   }
}
} // namespace type
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TYPE_H