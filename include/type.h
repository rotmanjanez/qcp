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
enum class Signedness {
   SIGNED,
   UNSIGNED,
   UNSPECIFIED,
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
   using ty_t = typename _EmitterT::ty_t;

   template <typename T>
   friend std::ostream& operator<<(std::ostream& os, const Type<T>& ty);

   friend TypeFactory;

   friend typename TypeFactory::DeclTypeBaseRef;

   explicit Type(std::pair<unsigned short, std::vector<Base>&> ini) : index_{ini.first}, types_{&ini.second} {}

   Type(std::vector<Base>& types, unsigned short index) : index_{index}, types_{&types} {}

   public:
   // todo: change to bit flags
   struct Qualifiers {
      bool operator==(const Qualifiers& other) const = default;
      bool operator!=(const Qualifiers& other) const = default;

      bool CONST = false;
      bool RESTRICT = false;
      bool VOLATILE = false;
      // todo: bool ATOMIC = false;
   };

   Type() : index_{0}, types_(nullptr) {}

   Type(const Type& other, Qualifiers qualifiers) : qualifiers{qualifiers}, index_{other.index_}, types_{other.types_} {}

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

   Type& addQualifier(token::Kind kind) {
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
      return *this;
   }

   bool operator==(const Type& other) const = default;
   bool operator!=(const Type& other) const = default;

   // todo: change to partial ordering
   std::strong_ordering operator<=>(const Type& other) const {
      return **this <=> *other;
   }

   operator bool() const {
      return index_ != 0 && types_ != nullptr;
   }

   operator ty_t*() const {
      return static_cast<ty_t*>(**this);
   }

   const Base* operator->() const {
      return &**this;
   }

   const Base& operator*() const {
      assert(types_ && "Type is not initialized");
      return (*types_)[index_];
   }

   Qualifiers qualifiers;

   private:
   // todo: maybe short is not enough
   unsigned short index_;
   std::vector<Base>* types_;
};
// ---------------------------------------------------------------------------
template <typename _EmitterT>
struct TaggedType {
   using Type = Type<_EmitterT>;
   using Base = Base<_EmitterT>;
   using ty_t = typename _EmitterT::ty_t;

   TaggedType(const TaggedType& other) = default;
   TaggedType(TaggedType&& other) = default;
   TaggedType& operator=(const TaggedType& other) = default;
   TaggedType& operator=(TaggedType&& other) = default;

   TaggedType(Ident name, Type ty) : name{name}, ty{ty} {}

   bool operator==(const TaggedType& other) const {
      return ty == other.ty && name == other.name;
   }

   auto operator<=>(const TaggedType& other) const {
      return ty <=> other.ty == 0 ? name <=> other.name : ty <=> other.ty;
   }

   operator bool() const {
      return ty;
   }

   const Base* operator->() const {
      return &**this;
   }

   const Base& operator*() const {
      return *ty;
   }

   operator ty_t*() const {
      return static_cast<ty_t*>(ty);
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
   os << *ty;
   return os;
}
// ---------------------------------------------------------------------------
} // namespace type
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TYPE_H