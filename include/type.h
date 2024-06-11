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
#include "defs/types.def"
};
// ---------------------------------------------------------------------------
enum class Cast {
   // clang-format off
   TRUNC, ZEXT, SEXT, FPTOUI, FPTOSI, UITOFP, SITOFP, FPTRUNC, FPEXT,
   PTRTOINT, INTTOPTR, BITCAST,
   // clang-format on
};
// ---------------------------------------------------------------------------
template <typename _EmitterT>
struct Base;
// ---------------------------------------------------------------------------
template <typename _EmitterT>
class TypeFactory;
// ---------------------------------------------------------------------------
template <typename _EmitterT>
struct TaggedType;
// ---------------------------------------------------------------------------
template <typename _EmitterT>
class Type {
   using Base = Base<_EmitterT>;
   using TypeFactory = TypeFactory<_EmitterT>;
   using Token = token::Token;

   template <typename T>
   friend std::ostream& operator<<(std::ostream& os, const Type<T>& ty);

   friend TypeFactory;

   friend typename TypeFactory::DeclTypeBaseRef;

   explicit Type(std::pair<unsigned short, std::vector<Base>&> ini) : index_{ini.first}, types_{&ini.second} {}

   explicit Type(std::vector<Base>& types, unsigned short index) : index_{index}, types_{&types} {}

   public:
   Type() : index_{0}, types_(nullptr) {}

   Type(const Type& other) = default;
   Type(Type&& other) = default;
   Type& operator=(const Type& other) = default;
   Type& operator=(Type&& other) = default;

   bool isIntegerType() const {
      switch (base().kind) {
         case Kind::CHAR:
         case Kind::SHORT:
         case Kind::INT:
         case Kind::LONG:
         case Kind::LONGLONG:
            return true;
         default:
            return false;
      }
   }

   bool isFloatingType() const {
      switch (base().kind) {
         case Kind::FLOAT:
         case Kind::DOUBLE:
         case Kind::LONGDOUBLE:
            return true;
         default:
            return false;
      }
   }

   bool isArithmetic() const {
      return isIntegerType() || isFloatingType();
   }

   bool isPointerType() const {
      return base().kind == Kind::PTR_T;
   }

   bool isSigned() const {
      return !base().unsingedTy;
   }

   bool isBasicType() const {
      return isFloatingType() || isIntegerType();
   }

   bool variablyModified() const {
      return base().variablyModified();
   }

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

   Type getPointedToTy() const {
      return base().ptrTy;
   }

   const std::vector<TaggedType<_EmitterT>>& getMembers() const {
      return base().structOrUnionTy.members;
   }

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
} // namespace type
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TYPE_H