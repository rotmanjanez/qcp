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
#include <sstream>
#include <variant>
#include <vector>
#include <cstdint>
// ---------------------------------------------------------------------------
#define VARIANT_ACCESS_METHODS(name, type, member) \
   type& name() { return std::get<type>(data_); }  \
   const type& name() const { return std::get<type>(data_); }
// ---------------------------------------------------------------------------
namespace qcp {
namespace type {
// ---------------------------------------------------------------------------
template <typename _EmitterT>
class TypeFactory;
// ---------------------------------------------------------------------------
// todo: _BitInt(N)
// todo: _Complex float
// todo: _Imaginary float
// todo: _Complex double
// todo: _Imaginary double
// todo: _Complex long double
// todo: _Imaginary long double
template <typename _EmitterT>
struct Base {
   using Type = Type<_EmitterT>;
   using TaggedType = tagged<Type>;
   using ty_t = typename _EmitterT::ty_t;
   using ssa_t = typename _EmitterT::ssa_t;
   using iconst_t = typename _EmitterT::iconst_t;

   unsigned CHAR_BITS = _EmitterT::CHAR_HAS_16_BIT ? 16 : 8;
   static constexpr unsigned SHORT_BITS = 16;
   static constexpr unsigned INT_BITS = _EmitterT::INT_HAS_64_BIT ? 64 : 32;
   static constexpr unsigned LONG_BITS = _EmitterT::LONG_HAS_64_BIT ? 64 : 32;
   static constexpr unsigned LONG_LONG_BITS = 64;

   friend TypeFactory<_EmitterT>;

   template <typename T>
   friend std::ostream& operator<<(std::ostream& os, const Base<T>& ty);

   Base() : kind_{Kind::END} {}

   // todo: (jr) remove this constructor
   constexpr explicit Base(Kind kind) : kind_{kind}, data_{Signedness(Signedness::UNSPECIFIED)} {}

   Base(Kind integerKind, bool unsignedTy) : kind_{integerKind}, data_{Signedness(unsignedTy ? Signedness::UNSIGNED : Signedness::SIGNED)} {
      assert(kind_ >= Kind::BOOL && kind_ <= Kind::LONGLONG && kind_ != Kind::COMPLEX && "Invalid Base::Kind for integer");
   }

   // pointer type
   explicit Base(Type ptrTy) : kind_{Kind::PTR_T}, data_{ptrTy} {}

   // array type
   Base(Type elemTy, std::size_t size, bool unspecifiedSize = false) : kind_{Kind::ARRAY_T}, data_{ArrayTy{elemTy, unspecifiedSize, size}} {}
   Base(Type elemTy, ssa_t* size, bool unspecifiedSize = false) : kind_{Kind::ARRAY_T}, data_{ArrayTy{elemTy, unspecifiedSize, size}} {}

   // function type
   // todo: replace with move
   Base(Type retTy, const std::vector<Type>& paramTys, bool isVarArgFnTy) : kind_{Kind::FN_T}, data_{FnTy{paramTys, retTy, isVarArgFnTy}} {}

   // Struct or Union type
   Base(token::Kind tk, std::vector<tagged<Type>> members, bool incomplete, Ident tag) : kind_{tk == token::Kind::STRUCT ? Kind::STRUCT_T : Kind::UNION_T}, data_{StructOrUnionTy{incomplete, tag, members}} {
      assert(tk == token::Kind::STRUCT || tk == token::Kind::UNION && "Invalid token::Kind for struct or union");
   }

   // Enum type
   Base(Type underlyingType, bool fixedUnerlyingTy, Ident tag) : kind_{Kind::ENUM_T}, data_{EnumTy{tag, fixedUnerlyingTy, underlyingType}} {}

   Base(const Base&) = default;
   Base(Base&&) = default;
   Base& operator=(const Base&) = delete;
   Base& operator=(Base&&) = default;

   bool operator==(const Base& other) const {
      return kind_ == other.kind_ && data_ == other.data_;
   }

   std::strong_ordering operator<=>(const Base&) const;

   operator ty_t*() const {
      return ref_;
   }

   int rank() const;

   bool isVoidTy() const { return kind_ == Kind::VOID; }
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
   bool isSignedTy() const;
   bool isSignedCharlikeTy() const;
   bool isVarArgFnTy() const;
   bool isFixedSizeArrayTy() const;
   bool isArrayTy() const;
   bool isUnionTy() const;
   bool isFnTy() const;
   bool isStructTy() const;
   bool isEnumTy() const;

   Ident getTag() const {
      if (kind_ == Kind::STRUCT_T || kind_ == Kind::UNION_T) {
         return structOrUnionTy().tag;
      } else if (kind_ == Kind::ENUM_T) {
         return enumTy().tag;
      }
      return Ident();
   }

   bool isCompatibleWith(const Base& other) const {
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
         if (!getElemTy().isCompatibleWith(other.getElemTy())) {
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

   Type getRetTy() const;
   const std::vector<Type>& getParamTys() const;
   Type getElemTy() const;
   Type getPointedToTy() const;
   const std::vector<tagged<Type>>& getMembers() const;
   std::size_t getArraySize() const;
   Type getUnderlyingTy() const { return enumTy().underlyingType; }

   Kind kind() const { return kind_; }

   class const_member_iterator {
      public:
      using value_type = const tagged<Type>;
      using reference = const tagged<Type>&;
      using pointer = const tagged<Type>*;
      using iterator_category = std::input_iterator_tag;
      using difference_type = std::ptrdiff_t;

      const_member_iterator() : root{nullptr}, indices_{0, 0}, parentMemberSize{0}, memberIt_{} {}
      const_member_iterator(const Base& root) : root{&root}, indices_{0, 0}, parentMemberSize{root.getMembers().size()}, memberIt_{root.getMembers().begin()} {
         const Base* parent = &root;
         if (memberIt_ == parent->getMembers().end()) {
            return;
         }
         indices_.push_back(0);
         while (canTransparentlyStepInto()) {
            parentMemberSize = parent->getMembers().size();
            indices_.push_back(0);
            parent = &*parent->getMembers().front();
         }
         memberIt_ = parent->getMembers().begin();
      }

      const_member_iterator&
      operator++() {
         if (canTransparentlyStepInto()) {
            Type parent = *memberIt_;
            while (canTransparentlyStepInto()) {
               indices_.push_back(0);
               parent = parent->getMembers().front();
               parentMemberSize = parent->getMembers().size();
            }
         } else if (indices_.back() < parentMemberSize) {
            ++indices_.back();
            ++memberIt_;
         } else {
            while (indices_.back() == parentMemberSize) {
               assert(indices_.size() > 2 && "Cannot increment past the end iterator");
               indices_.pop_back();
               ++indices_.back();
               const Base* parent = root;
               for (unsigned i = 2; i < indices_.size(); ++i) {
                  parent = &*parent->getMembers()[indices_[i]];
               }
               parentMemberSize = parent->getMembers().size();
            }
         }
         return *this;
      }

      bool operator==(const const_member_iterator& other) const {
         return (indices_.size() == 2 && other.indices_.size() == 2) || std::tie(root, indices_) == std::tie(other.root, other.indices_);
      }

      const_member_iterator operator++(int) {
         const_member_iterator tmp = *this;
         ++(*this);
         return tmp;
      }

      reference operator*() const {
         return *memberIt_;
      }

      pointer operator->() const {
         return &*memberIt_;
      }

      const std::vector<std::uint32_t>& GEPDerefValues() const {
         return indices_;
      }

      private:
      bool canTransparentlyStepInto() const {
         return ((**this)->isStructTy() || (**this)->isUnionTy()) && !(**this)->structOrUnionTy().tag && (**this)->getMembers().size() > 0;
      }

      const Base* root;
      std::vector<std::uint32_t> indices_;
      // these are denormalizations for performance reasons
      std::size_t parentMemberSize;
      std::vector<tagged<Type>>::const_iterator memberIt_;
   };

   // iterator over members that can be accessed by member access or dereference operator
   const_member_iterator membersBegin() const {
      if (kind_ == Kind::STRUCT_T || kind_ == Kind::UNION_T) {
         return const_member_iterator{*this};
      }
      return const_member_iterator{};
   }

   const_member_iterator membersEnd() const {
      return const_member_iterator{};
   }

   struct StructOrUnionTy {
      bool operator==(const StructOrUnionTy& other) const = default;
      bool operator!=(const StructOrUnionTy& other) const = default;

      bool incomplete;
      Ident tag;
      std::vector<tagged<Type>> members;
      // todo: XXX attributes; // e.g.warn on gnu style attribute __attribute__((packed))
      // todo: warn on any attribute
      // todo: XXX members; // might have flexible array member; might have bitfields; might be anonymous union or structures, might have alignas()
   };

   struct EnumTy {
      bool operator==(const EnumTy& other) const = default;
      bool operator!=(const EnumTy& other) const = default;

      Ident tag;
      bool fixedUnerlyingTy;
      Type underlyingType;
   };

   struct FnTy {
      bool operator==(const FnTy& other) const = default;
      bool operator!=(const FnTy& other) const = default;

      std::vector<Type> paramTys{};
      Type retTy{};
      bool isVarArgFnTy;
   };

   struct ArrayTy {
      bool operator==(const ArrayTy& other) const = default;
      bool operator!=(const ArrayTy& other) const = default;

      Type elemTy;
      bool unspecifiedSize;
      std::variant<std::monostate, std::size_t, ssa_t*> size;
      // See also Example 5 in section 6.7.9.
   };

   VARIANT_ACCESS_METHODS(signedness, Signedness, data_)
   VARIANT_ACCESS_METHODS(ptrTy, Type, data_)
   VARIANT_ACCESS_METHODS(structOrUnionTy, StructOrUnionTy, data_)
   VARIANT_ACCESS_METHODS(enumTy, EnumTy, data_)
   VARIANT_ACCESS_METHODS(fnTy, FnTy, data_)
   VARIANT_ACCESS_METHODS(arrayTy, ArrayTy, data_)

   private:
   void populateEmitterType(TypeFactory<_EmitterT>& factory, _EmitterT& emitter);

   std::string str() const {
      std::stringstream ss;
      strImpl(ss);
      return ss.str();
   }

   void strImpl(std::stringstream& ss) const;

   Type* derivedFrom() {
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

   const Type* derivedFrom() const {
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
   ty_t* ref_ = nullptr;
   std::variant<Signedness, Type, StructOrUnionTy, EnumTy, FnTy, ArrayTy> data_;
};
// ---------------------------------------------------------------------------
// Base
// ---------------------------------------------------------------------------
template <typename _EmitterT>
std::strong_ordering Base<_EmitterT>::operator<=>(const Base& other) const {
   return rank() <=> other.rank();
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
int Base<_EmitterT>::rank() const {
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
      default:
         assert(false && "Invalid Base::Kind to rank()");
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isVariablyModifiedTy() const {
   // todo: pointer and function types
   if (kind_ == Kind::ARRAY_T) {
      return std::holds_alternative<ssa_t*>(arrayTy().size);
   } else if (kind_ == Kind::STRUCT_T || kind_ == Kind::UNION_T) {
      for (const auto& member : structOrUnionTy().members) {
         if (member.type.isVariablyModifiedTy()) {
            return true;
         }
      }
   }
   return false;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isBoolTy() const {
   return kind_ == Kind::BOOL;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isIntegerTy() const {
   // todo: enum types
   switch (kind_) {
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
template <typename _EmitterT>
bool Base<_EmitterT>::isFloatingTy() const {
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
template <typename _EmitterT>
bool Base<_EmitterT>::isDecimalTy() const {
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
template <typename _EmitterT>
bool Base<_EmitterT>::isRealFloatingTy() const {
   return isFloatingTy() || isDecimalTy();
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isBasicTy() const {
   return isRealFloatingTy() || isIntegerTy() || kind_ == Kind::CHAR;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isCharacterTy() const {
   return kind_ == Kind::CHAR;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isArithmeticTy() const {
   return isIntegerTy() || isRealFloatingTy();
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isCompleteTy() const {
   switch (kind_) {
      case Kind::VOID:
         return false;
      case Kind::ARRAY_T:
         return !arrayTy().unspecifiedSize && getElemTy()->isCompleteTy();
      case Kind::STRUCT_T:
      case Kind::UNION_T:
         if (structOrUnionTy().incomplete) {
            return false;
         }
         for (const auto& member : getMembers()) {
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
template <typename _EmitterT>
bool Base<_EmitterT>::isScalarTy() const {
   return isArithmeticTy() || isPointerTy() || kind_ == Kind::NULLPTR_T;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isAggregateTy() const {
   return kind_ == Kind::STRUCT_T || kind_ == Kind::ARRAY_T;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isPointerTy() const {
   return kind_ == Kind::PTR_T;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isSignedTy() const {
   return isIntegerTy() && signedness() == Signedness::SIGNED;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isSignedCharlikeTy() const {
   return isCharacterTy() && (isSignedTy() || (signedness() == Signedness::UNSPECIFIED && _EmitterT::CHAR_IS_SIGNED));
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isVarArgFnTy() const {
   return kind_ == Kind::FN_T && fnTy().isVarArgFnTy;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Base<_EmitterT>::Type Base<_EmitterT>::getRetTy() const {
   return fnTy().retTy;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
const std::vector<typename Base<_EmitterT>::Type>& Base<_EmitterT>::getParamTys() const {
   return fnTy().paramTys;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Base<_EmitterT>::Type Base<_EmitterT>::getElemTy() const {
   return arrayTy().elemTy;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isFixedSizeArrayTy() const {
   if (kind_ == Kind::ARRAY_T) {
      return !std::holds_alternative<ssa_t*>(arrayTy().size);
   }
   return false;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isArrayTy() const {
   return kind_ == Kind::ARRAY_T;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isUnionTy() const {
   return kind_ == Kind::UNION_T;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isFnTy() const {
   return kind_ == Kind::FN_T;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isStructTy() const {
   return kind_ == Kind::STRUCT_T;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isEnumTy() const {
   return kind_ == Kind::ENUM_T;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
std::size_t Base<_EmitterT>::getArraySize() const {
   if (const std::size_t* size = std::get_if<std::size_t>(&arrayTy().size)) {
      return *size;
   }
   assert(false && "Array size is not a constant");
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Base<_EmitterT>::Type Base<_EmitterT>::getPointedToTy() const {
   return ptrTy();
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
const std::vector<tagged<Type<_EmitterT>>>& Base<_EmitterT>::getMembers() const {
   return structOrUnionTy().members;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Base<_EmitterT>::populateEmitterType(TypeFactory<_EmitterT>& factory, _EmitterT& emitter) {
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
      if (const std::size_t* size = std::get_if<std::size_t>(&arrayTy().size)) {
         ref_ = emitter.emitArrayTy(arrayTy().elemTy, emitter.emitIConst(factory.sizeTy(), *size));
      } else {
         ref_ = static_cast<ty_t*>(arrayTy().elemTy);
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
      std::vector<Type> members;
      for (const auto& member : structOrUnionTy().members) {
         members.push_back(member);
      }
      ref_ = emitter.emitStructTy(members, structOrUnionTy().incomplete, structOrUnionTy().tag.prefix("struct."));
   } else if (kind_ == Kind::UNION_T) {
      Type max = structOrUnionTy().members.front();
      for (const auto& member : structOrUnionTy().members) {
         if (emitter.getIntegerValue(emitter.sizeOf(member)) > emitter.getIntegerValue(emitter.sizeOf(max))) {
            max = member;
         }
      }
      //  std::max_element(structOrUnionTy().members.begin(), structOrUnionTy().members.end(), [&emitter](const tagged<Type>& a, const tagged<Type>& b) {            return emitter.getIntegerValue(emitter.sizeOf(a.ty)) < emitter.getIntegerValue(emitter.sizeOf(b.ty));})->ty;
      std::vector<Type> members{max};

      ref_ = emitter.emitStructTy(members, structOrUnionTy().tag.prefix("union."));
   } else if (kind_ == Kind::ENUM_T) {
      ref_ = enumTy().underlyingType->ref_;
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
template <typename _EmitterT>
void Base<_EmitterT>::strImpl(std::stringstream& ss) const {
   static const char* names[] = {
#define ENUM_AS_STRING
#include "defs/types.def"
#undef ENUM_AS_STRING
   };
   if ((kind_ < Kind::FLOAT || (kind_ >= Kind::DECIMAL32 && kind_ <= Kind::DECIMAL128)) && signedness() == Signedness::UNSIGNED) {
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
            ss << getElemTy();
            if (getElemTy()->kind() == Kind::FN_T) {
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
               for (const auto& member : structOrUnionTy().members) {
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
template <typename _EmitterT>
std::ostream& operator<<(std::ostream& os, const Base<_EmitterT>& ty) {
   os << ty.str();
   return os;
}
// ---------------------------------------------------------------------------
} // namespace type
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_BASE_TYPE_H