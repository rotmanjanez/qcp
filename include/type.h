#ifndef QCP_TYPE_H
#define QCP_TYPE_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "operator.h"
#include "token.h"
// ---------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <variant>
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
   PTRTOINT, INTTOPTR, BITCAST, END
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
class Type {
   using BaseType = Base<_EmitterT>;
   using Token = token::Token;
   using ty_t = typename _EmitterT::ty_t;

   template <typename T>
   friend std::ostream& operator<<(std::ostream& os, const Type<T>& ty);

   friend TypeFactory<_EmitterT>;
   // sfriend typename TypeFactory::DeclTypeBaseRef;
   // friend typename Base::const_member_iterator;

   explicit Type(std::pair<unsigned short, std::vector<BaseType>&> ini) : index_{ini.first}, types_{&ini.second} {}
   Type(std::vector<BaseType>& types, unsigned short index) : index_{index}, types_{&types} {}

   public:
   // todo: change to bit flags
   struct Qualifiers {
      bool operator==(const Qualifiers& other) const = default;
      bool operator!=(const Qualifiers& other) const = default;

      bool operator<(const Qualifiers& other) const {
         return std::tie(CONST, RESTRICT, VOLATILE) < std::tie(other.CONST, other.RESTRICT, other.VOLATILE);
      }
      bool operator>(const Qualifiers& other) const {
         return std::tie(CONST, RESTRICT, VOLATILE) > std::tie(other.CONST, other.RESTRICT, other.VOLATILE);
      }

      char CONST : 1 = false,
                   RESTRICT : 1 = false,
                   VOLATILE : 1 = false;
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

   bool isCompatibleWith(const Type& other) const {
      if (!*this || !other) {
         return false;
      }
      if (qualifiers != other.qualifiers) {
         return false;
      }
      return (*this)->isCompatibleWith(*other);
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

   bool operator==(const Type& other) const {
      return *this && other && **this == *other;
   }

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

   const BaseType* operator->() const {
      return &**this;
   }

   const BaseType& operator*() const {
      assert(types_ && "Type is not initialized");
      return (*types_)[index_];
   }

   Qualifiers qualifiers;

   private:
   // todo: maybe short is not enough
   unsigned short index_;
   std::vector<BaseType>* types_;
};
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