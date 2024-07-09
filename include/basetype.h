#ifndef QCP_BASE_TYPE_H
#define QCP_BASE_TYPE_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "emittertraits.h"
#include "operator.h"
#include "token.h"
#include "type.h"
// ---------------------------------------------------------------------------
#include <algorithm>
#include <compare>
#include <cstdint>
#include <span>
#include <sstream>
#include <variant>
#include <vector>
// ---------------------------------------------------------------------------
namespace qcp {
namespace type {
// ---------------------------------------------------------------------------
template <typename T>
class TypeFactory;
// ---------------------------------------------------------------------------
// todo: _BitInt(N)
// todo: _Complex float
// todo: _Imaginary float
// todo: _Complex double
// todo: _Imaginary double
// todo: _Complex long double
// todo: _Imaginary long double
template <typename T>
struct Base {
   using Ty = Type<T>;
   using ty_t = typename T::ty_t;
   using ssa_t = typename T::ssa_t;
   using iconst_t = typename T::iconst_t;

   unsigned CHAR_BITS = T::CHAR_HAS_16_BIT ? 16 : 8;
   static constexpr unsigned SHORT_BITS = 16;
   static constexpr unsigned INT_BITS = T::INT_HAS_64_BIT ? 64 : 32;
   static constexpr unsigned LONG_BITS = T::LONG_HAS_64_BIT ? 64 : 32;
   static constexpr unsigned LONG_LONG_BITS = 64;

   friend TypeFactory<T>;

   template <typename U>
   friend std::ostream &operator<<(std::ostream &os, const Base<U> &ty);

   Base() : kind_{Kind::END} {}

   constexpr explicit Base(Kind kind) : kind_{kind}, data_{Sign(Sign::UNSPECIFIED)} {}

   Base(Kind integerKind, Sign signedness) : kind_{integerKind}, data_{signedness} {
      assert(kind_ >= Kind::BOOL && kind_ <= Kind::LONGLONG && kind_ != Kind::COMPLEX && "Invalid Base::Kind for integer");
   }

   // pointer type
   explicit Base(Ty ptrTy) : kind_{Kind::PTR_T}, data_{ptrTy} {}

   // array type
   Base(Ty elemTy, std::size_t size, bool unspecifiedSize = false) : kind_{Kind::ARRAY_T}, data_{ArrayTy{elemTy, unspecifiedSize, size}} {}
   Base(Ty elemTy, ssa_t *size, bool unspecifiedSize = false) : kind_{Kind::ARRAY_T}, data_{ArrayTy{elemTy, unspecifiedSize, size}} {}

   // function type
   // todo: replace with move
   Base(Ty retTy, const std::vector<Ty> &paramTys, bool isVarArgFnTy) : kind_{Kind::FN_T}, data_{FnTy{paramTys, retTy, isVarArgFnTy}} {}

   // Struct or Union type
   Base(token::Kind tk, std::vector<tagged<Ty>> members, bool incomplete, Ident tag) : kind_{tk == token::Kind::STRUCT ? Kind::STRUCT_T : Kind::UNION_T}, data_{StructOrUnionTy{incomplete, tag, members}} {
      assert((tk == token::Kind::STRUCT || tk == token::Kind::UNION) && "Invalid token::Kind for struct or union");
   }

   // Enum type
   Base(Ty underlyingType, bool fixedUnerlyingTy, Ident tag) : kind_{Kind::ENUM_T}, data_{EnumTy{tag, fixedUnerlyingTy, underlyingType}} {}

   Base(const Base &) = default;
   Base(Base &&) = default;
   Base &operator=(const Base &) = delete;
   Base &operator=(Base &&) = default;

   bool operator==(const Base &other) const {
      return kind_ == other.kind_ && data_ == other.data_;
   }

   std::strong_ordering operator<=>(const Base &) const;

   operator ty_t *() const {
      return ref_;
   }

   int rank() const;

   bool isVoidTy() const { return kind_ == Kind::VOID; }
   bool isNullPointerTy() const { return kind_ == Kind::NULLPTR_T; }
   bool isVariablyModifiedTy() const;
   bool isBoolTy() const;
   bool isIntegerTy() const;
   bool isFloatingTy() const;
   bool isDecimalTy() const;
   bool isRealFloatingTy() const;
   bool isBasicTy() const;
   bool isCharacterTy() const;
   bool isArithmeticTy() const;
   bool isCompleteTy() const;
   bool isScalarTy() const;
   bool isAggregateTy() const;
   bool isPointerTy() const;
   bool isVoidPointerTy() const { return isPointerTy() && getPointedToTy()->isVoidTy(); }
   bool isSignedTy() const;
   bool isSignedCharlikeTy() const;
   bool isVarArgFnTy() const;
   bool isFixedSizeArrayTy() const;
   bool isArrayTy() const;
   bool isUnionTy() const;
   bool isFnTy() const;
   bool isStructTy() const;
   bool isEnumTy() const;
   bool isPtrTy() const { return kind_ == Kind::PTR_T; }
   bool isRealTy() const { return isIntegerTy() || isRealFloatingTy(); }

   Ident getTag() const {
      if (kind_ == Kind::STRUCT_T || kind_ == Kind::UNION_T) {
         return structOrUnionTy().tag;
      } else if (kind_ == Kind::ENUM_T) {
         return enumTy().tag;
      }
      return Ident();
   }

   bool isCompatibleWith(const Base &other) const {
      if (kind_ == Kind::END || other.kind_ == Kind::END) {
         return false;
      }
      if (isIntegerTy() && other.isIntegerTy()) {
         return signedness() == other.signedness(); // todo: how to handle char here?
      }
      if (kind_ != other.kind_) {
         return false;
      }

      if (kind_ == Kind::PTR_T) {
         return getPointedToTy().isCompatibleWith(other.getPointedToTy());
      } else if (kind_ == Kind::ARRAY_T) {
         if (!getElementTy().isCompatibleWith(other.getElementTy())) {
            return false;
         }
         if (isFixedSizeArrayTy() && other.isFixedSizeArrayTy()) {
            return getArraySize() == other.getArraySize();
         }
      } else if (kind_ == Kind::FN_T) {
         if (!getRetTy().isCompatibleWith(other.getRetTy()) ||
             getParamTys().size() != other.getParamTys().size() ||
             isVarArgFnTy() != other.isVarArgFnTy()) {
            return false;
         }
         for (size_t i = 0; i < getParamTys().size(); ++i) {
            if (!getParamTys()[i].isCompatibleWith(other.getParamTys()[i])) {
               return false;
            }
         }
      } else if (kind_ == Kind::STRUCT_T || kind_ == Kind::UNION_T) {
         if (structOrUnionTy().incomplete || other.structOrUnionTy().incomplete) {
            return structOrUnionTy().tag == other.structOrUnionTy().tag;
         }
         if (structOrUnionTy().members.size() != other.structOrUnionTy().members.size()) {
            return false;
         }
         for (size_t i = 0; i < structOrUnionTy().members.size(); ++i) {
            auto thisMember = structOrUnionTy().members[i];
            auto otherMember = other.structOrUnionTy().members[i];
            if (!thisMember.isCompatibleWith(otherMember)) {
               return false;
            }
            // todo: alignemt specifier
            if (thisMember.name() || otherMember.name()) {
               return thisMember.name() == otherMember.name();
            }
         }
      }
      return true;
   }

   Ty getRetTy() const;
   const std::vector<Ty> &getParamTys() const;
   Ty getElementTy() const;
   Ty getPointedToTy() const;
   const std::vector<tagged<Ty>> &getMembers() const;
   std::size_t getArraySize() const;
   Ty getUnderlyingTy() const { return enumTy().underlyingType; }

   Kind kind() const { return kind_; }

   struct StructOrUnionTy {
      bool operator==(const StructOrUnionTy &other) const = default;
      bool operator!=(const StructOrUnionTy &other) const = default;

      bool incomplete;
      Ident tag;
      std::vector<tagged<Ty>> members;
      // todo: XXX attributes; // e.g.warn on gnu style attribute __attribute__((packed))
      // todo: warn on any attribute
      // todo: XXX members; // might have flexible array member; might have bitfields; might be anonymous union or structures, might have alignas()
   };

   struct EnumTy {
      bool operator==(const EnumTy &other) const = default;
      bool operator!=(const EnumTy &other) const = default;

      Ident tag;
      bool fixedUnerlyingTy;
      Ty underlyingType;
   };

   struct FnTy {
      bool operator==(const FnTy &other) const = default;
      bool operator!=(const FnTy &other) const = default;

      std::vector<Ty> paramTys{};
      Ty retTy{};
      bool isVarArgFnTy;
   };

   struct ArrayTy {
      bool operator==(const ArrayTy &other) const = default;
      bool operator!=(const ArrayTy &other) const = default;

      Ty elemTy;
      bool unspecifiedSize;
      std::variant<std::monostate, std::size_t, ssa_t *> size;
      // See also Example 5 in section 6.7.9.
   };

#define VARIANT_ACCESS_METHODS(name, type, member) \
   type &name() { return std::get<type>(data_); }  \
   const type &name() const { return std::get<type>(data_); }

   VARIANT_ACCESS_METHODS(signedness, Sign, data_)
   VARIANT_ACCESS_METHODS(ptrTy, Ty, data_)
   VARIANT_ACCESS_METHODS(structOrUnionTy, StructOrUnionTy, data_)
   VARIANT_ACCESS_METHODS(enumTy, EnumTy, data_)
   VARIANT_ACCESS_METHODS(fnTy, FnTy, data_)
   VARIANT_ACCESS_METHODS(arrayTy, ArrayTy, data_)

#undef VARIANT_ACCESS_METHODS

   private:
   void populateEmitterType(TypeFactory<T> &factory, T &emitter);

   std::string str() const {
      std::stringstream ss;
      strImpl(ss);
      return ss.str();
   }

   void strImpl(std::stringstream &ss) const;

   Ty *derivedFrom() {
      switch (kind_) {
         case Kind::PTR_T:
            return &ptrTy();
         case Kind::ARRAY_T:
            return &arrayTy().elemTy;
         case Kind::FN_T:
            return &fnTy().retTy;
         default:
            return nullptr;
      }
   }

   const Ty *derivedFrom() const {
      switch (kind_) {
         case Kind::PTR_T:
            return &ptrTy();
         case Kind::ARRAY_T:
            return &arrayTy().elemTy;
         case Kind::FN_T:
            return &fnTy().retTy;
         default:
            return nullptr;
      }
   }

   Kind kind_;
   ty_t *ref_ = nullptr;
   std::variant<Sign, Ty, StructOrUnionTy, EnumTy, FnTy, ArrayTy> data_;
};
// ---------------------------------------------------------------------------
// Base
// ---------------------------------------------------------------------------
template <typename T>
std::strong_ordering Base<T>::operator<=>(const Base &other) const {
   return rank() <=> other.rank();
}
// ---------------------------------------------------------------------------
template <typename T>
int Base<T>::rank() const {
   if (kind_ == Kind::ENUM_T) {
      return enumTy().underlyingType->rank();
   }
   if (kind_ == Kind::END) {
      // todo: (jr) how to handle this?
      return -1;
   }
   switch (kind_) {
      case Kind::BOOL:
      case Kind::CHAR:
      case Kind::SHORT:
      case Kind::INT:
         return static_cast<int>(kind_) - 1;
      case Kind::LONG:
         return 4;
      case Kind::LONGLONG:
         return 5;
      case Kind::FLOAT:
         return 6;
      case Kind::DOUBLE:
         return 7;
      case Kind::LONGDOUBLE:
         return 8;
      case Kind::DECIMAL32:
         return 9;
      case Kind::DECIMAL64:
         return 10;
      case Kind::DECIMAL128:
         return 11;
      case Kind::ENUM_T:
         return enumTy().underlyingType->rank();
      default:
         assert(false && "Invalid Base::Kind to rank()");
   }
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isVariablyModifiedTy() const {
   // todo: pointer and function types
   if (kind_ == Kind::ARRAY_T) {
      return std::holds_alternative<ssa_t *>(arrayTy().size);
   } else if (kind_ == Kind::STRUCT_T || kind_ == Kind::UNION_T) {
      for (const auto &member : structOrUnionTy().members) {
         if (member.type.isVariablyModifiedTy()) {
            return true;
         }
      }
   }
   return false;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isBoolTy() const {
   return kind_ == Kind::BOOL;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isIntegerTy() const {
   // todo: enum types
   switch (kind_) {
      case Kind::BOOL:
      case Kind::CHAR:
      case Kind::SHORT:
      case Kind::INT:
      case Kind::LONG:
      case Kind::LONGLONG:
      case Kind::ENUM_T:
         return true;
      default:
         return false;
   }
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isFloatingTy() const {
   switch (kind_) {
      case Kind::FLOAT:
      case Kind::DOUBLE:
      case Kind::LONGDOUBLE:
         return true;
      default:
         return false;
   }
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isDecimalTy() const {
   switch (kind_) {
      case Kind::DECIMAL32:
      case Kind::DECIMAL64:
      case Kind::DECIMAL128:
         return true;
      default:
         return false;
   }
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isRealFloatingTy() const {
   return isFloatingTy() || isDecimalTy();
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isBasicTy() const {
   return isRealFloatingTy() || isIntegerTy() || kind_ == Kind::CHAR;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isCharacterTy() const {
   return kind_ == Kind::CHAR;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isArithmeticTy() const {
   return isIntegerTy() || isRealFloatingTy();
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isCompleteTy() const {
   switch (kind_) {
      case Kind::VOID:
         return false;
      case Kind::ARRAY_T:
         return !arrayTy().unspecifiedSize && getElementTy()->isCompleteTy();
      case Kind::STRUCT_T:
      case Kind::UNION_T:
         if (structOrUnionTy().incomplete) {
            return false;
         }
         for (const auto &member : getMembers()) {
            if (!member->isCompleteTy()) {
               return false;
            }
         }
         return true;
      case Kind::ENUM_T:
         return enumTy().underlyingType && enumTy().underlyingType->isCompleteTy();
      default:
         return true;
   }
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isScalarTy() const {
   return isArithmeticTy() || isPointerTy() || kind_ == Kind::NULLPTR_T;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isAggregateTy() const {
   return kind_ == Kind::STRUCT_T || kind_ == Kind::ARRAY_T;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isPointerTy() const {
   return kind_ == Kind::PTR_T;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isSignedTy() const {
   return isIntegerTy() && signedness() == Sign::SIGNED;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isSignedCharlikeTy() const {
   return isCharacterTy() && (isSignedTy() || (signedness() == Sign::UNSPECIFIED && T::CHAR_IS_SIGNED));
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isVarArgFnTy() const {
   return kind_ == Kind::FN_T && fnTy().isVarArgFnTy;
}
// ---------------------------------------------------------------------------
template <typename T>
Type<T> Base<T>::getRetTy() const {
   return fnTy().retTy;
}
// ---------------------------------------------------------------------------
template <typename T>
const std::vector<Type<T>> &Base<T>::getParamTys() const {
   return fnTy().paramTys;
}
// ---------------------------------------------------------------------------
template <typename T>
Type<T> Base<T>::getElementTy() const {
   return arrayTy().elemTy;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isFixedSizeArrayTy() const {
   if (kind_ == Kind::ARRAY_T) {
      return !std::holds_alternative<ssa_t *>(arrayTy().size);
   }
   return false;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isArrayTy() const {
   return kind_ == Kind::ARRAY_T;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isUnionTy() const {
   return kind_ == Kind::UNION_T;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isFnTy() const {
   return kind_ == Kind::FN_T;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isStructTy() const {
   return kind_ == Kind::STRUCT_T;
}
// ---------------------------------------------------------------------------
template <typename T>
bool Base<T>::isEnumTy() const {
   return kind_ == Kind::ENUM_T;
}
// ---------------------------------------------------------------------------
template <typename T>
std::size_t Base<T>::getArraySize() const {
   if (const std::size_t *size = std::get_if<std::size_t>(&arrayTy().size)) {
      return *size;
   }
   assert(false && "Array size is not a constant");
}
// ---------------------------------------------------------------------------
template <typename T>
Type<T> Base<T>::getPointedToTy() const {
   return ptrTy();
}
// ---------------------------------------------------------------------------
template <typename T>
const std::vector<tagged<Type<T>>> &Base<T>::getMembers() const {
   return structOrUnionTy().members;
}
// ---------------------------------------------------------------------------
template <typename T>
void Base<T>::populateEmitterType(TypeFactory<T> &factory, T &emitter) {
   assert((!ref_ || ((kind_ == Kind::STRUCT_T || kind_ == Kind::UNION_T) && structOrUnionTy().incomplete)) && "Emitter type already populated");

   if (kind_ == Kind::PTR_T) {
      if (!getPointedToTy()->isCompleteTy()) {
         ref_ = emitter.emitPtrTo(factory.voidTy());
      } else {
         ref_ = emitter.emitPtrTo(ptrTy());
      }
      return;
   } else if (kind_ == Kind::FN_T) {
      ref_ = emitter.emitFnTy(fnTy().retTy, fnTy().paramTys, fnTy().isVarArgFnTy);
      return;
   } else if (kind_ == Kind::ARRAY_T) {
      if (const std::size_t *size = std::get_if<std::size_t>(&arrayTy().size)) {
         ref_ = emitter.emitArrayTy(arrayTy().elemTy, emitter.emitIConst(factory.sizeTy(), *size));
      } else {
         ref_ = static_cast<ty_t *>(arrayTy().elemTy);
      }
      return;
   } else if (kind_ == Kind::VOID) {
      ref_ = emitter.emitVoidTy();
   } else if (kind_ == Kind::FLOAT) {
      ref_ = emitter.emitFloatTy();
   } else if (kind_ == Kind::DOUBLE) {
      ref_ = emitter.emitDoubleTy();
   } else if (kind_ == Kind::STRUCT_T) {
      if (structOrUnionTy().incomplete) {
         return;
      }
      std::vector<Ty> members;
      for (const auto &member : structOrUnionTy().members) {
         members.push_back(member);
      }
      static Ident ANON {"anon"};
      Ident tag = structOrUnionTy().tag ? structOrUnionTy().tag : ANON;
      ref_ = emitter.emitStructTy(members, structOrUnionTy().incomplete, tag.prefix("struct."));
   } else if (kind_ == Kind::UNION_T) {
      Ty max = structOrUnionTy().members.front();
      for (const auto &member : structOrUnionTy().members) {
         if (emitter.getIntegerValue(emitter.sizeOf(member)) > emitter.getIntegerValue(emitter.sizeOf(max))) {
            max = member;
         }
      }
      //  std::max_element(structOrUnionTy().members.begin(), structOrUnionTy().members.end(), [&emitter](const tagged<Ty>& a, const tagged<Ty>& b) {            return emitter.getIntegerValue(emitter.sizeOf(a.ty)) < emitter.getIntegerValue(emitter.sizeOf(b.ty));})->ty;
      std::vector<Ty> members{max};

      ref_ = emitter.emitStructTy(members, structOrUnionTy().tag.prefix("union."));
   } else if (kind_ == Kind::ENUM_T) {
      if (enumTy().underlyingType) {
         ref_ = enumTy().underlyingType->ref_;
      }
   } else {
      unsigned bits = 0;
      switch (kind_) {
         case Kind::BOOL:
            bits = 1;
            break;
         case Kind::CHAR:
            bits = CHAR_BITS;
            break;
         case Kind::SHORT:
            bits = SHORT_BITS;
            break;
         case Kind::INT:
            bits = INT_BITS;
            break;
         case Kind::LONG:
            bits = LONG_BITS;
            break;
         case Kind::LONGLONG:
            bits = LONG_LONG_BITS;
            break;
         case Kind::VOID:
            ref_ = emitter.emitVoidTy();
            return;
         case Kind::FLOAT:
            ref_ = emitter.emitFloatTy();
            return;
         case Kind::DOUBLE:
            ref_ = emitter.emitDoubleTy();
            return;
         case Kind::LONGDOUBLE:
            ref_ = emitter.emitLongDoubleTy();
            return;
         default:
            // todo: (jr) not implemented
            std::cerr << "Invalid Base::Kind to populateEmitterType(): " << *this << std::endl;
            assert(false && "Invalid Base::Kind to populateEmitterType()");
      }
      ref_ = emitter.emitIntTy(bits);
   }
}
// ---------------------------------------------------------------------------
template <typename T>
void Base<T>::strImpl(std::stringstream &ss) const {
   static const char *names[] = {
#define ENUM_AS_STRING
#include "defs/types.def"
#undef ENUM_AS_STRING
   };
   if ((kind_ < Kind::FLOAT || (kind_ >= Kind::DECIMAL32 && kind_ <= Kind::DECIMAL128)) && signedness() == Sign::UNSIGNED) {
      ss << "unsigned ";
   }
   if (kind_ < Kind::NULLPTR_T) {
      ss << names[static_cast<int>(kind_)];
   } else {
      std::stringstream prepend;
      switch (kind_) {
         case Kind::PTR_T:
            ss << getPointedToTy() << '*';
            if (getPointedToTy()->kind() == Kind::ARRAY_T || getPointedToTy()->kind() == Kind::FN_T) {
               prepend << '(';
               ss << ')';
            }
            break;
         case Kind::ARRAY_T:
            ss << getElementTy();
            if (getElementTy()->kind() == Kind::FN_T) {
               prepend << '(';
               ss << ')';
            }
            ss << '[';
            if (isFixedSizeArrayTy()) {
               ss << std::to_string(getArraySize());
            } else {
               ss << "/* variable size */"; // todo: (jr) how to handle this?
            }
            ss << ']';
            break;
         case Kind::STRUCT_T:
         case Kind::UNION_T:
            ss << (kind_ == Kind::STRUCT_T ? "struct" : "union");
            if (structOrUnionTy().tag) {
               ss << ' ' << structOrUnionTy().tag;
            }
            if (!structOrUnionTy().incomplete && !structOrUnionTy().tag) {
               ss << " { ";
               for (const auto &member : structOrUnionTy().members) {
                  ss << member << ' ' << member.name() << "; ";
               }
               ss << '}';
            }
            break;
         case Kind::ENUM_T:
            ss << "enum";
            if (enumTy().tag) {
               ss << ' ' << enumTy().tag;
            }
            ss << " : " << enumTy().underlyingType;
            break;
         case Kind::FN_T:
            ss << getRetTy();
            if (getRetTy()->kind() == Kind::ARRAY_T || getRetTy()->kind() == Kind::FN_T) {
               prepend << '(';
               ss << ')';
            }
            ss << "(";
            for (size_t i = 0; i < fnTy().paramTys.size(); ++i) {
               ss << fnTy().paramTys[i];
               if (i + 1 < fnTy().paramTys.size()) {
                  ss << ", ";
               }
            }
            if (isVarArgFnTy()) {
               if (!getParamTys().empty()) {
                  ss << ", ";
               }
               ss << "...";
            }
            ss << ")";
            break;
         case Kind::END:
            ss << "undef";
            break;
         default:
            ss << "unknown(" << static_cast<int>(kind_) << ')';
            break;
      }
   }
}
// ---------------------------------------------------------------------------
template <typename T>
std::ostream &operator<<(std::ostream &os, const Base<T> &ty) {
   os << ty.str();
   return os;
}
// ---------------------------------------------------------------------------
} // namespace type
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_BASE_TYPE_H