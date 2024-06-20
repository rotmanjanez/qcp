#ifndef QCP_BASE_TYPE_H
#define QCP_BASE_TYPE_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "operator.h"
#include "token.h"
#include "type.h"
// ---------------------------------------------------------------------------
#include <algorithm>
#include <compare>
#include <sstream>
#include <variant>
#include <vector>
// ---------------------------------------------------------------------------
namespace qcp {
namespace type {
// ---------------------------------------------------------------------------
template <typename _EmitterT>
class TypeFactory;
// ---------------------------------------------------------------------------
#define VARIANT_ACCESS_METHODS(name, type, member) \
   type& name() { return std::get<type>(data_); }  \
   const type& name() const { return std::get<type>(data_); }
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
   using TY = Type<_EmitterT>;
   using TaggedTY = TaggedType<_EmitterT>;
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
   Base(TY ptrTy) : kind_{Kind::PTR_T}, data_{ptrTy} {}

   // array type
   Base(TY elemTy, iconst_t* size, bool unspecifiedSize = false) : kind_{Kind::ARRAY_T}, data_{ArrayTy{elemTy, unspecifiedSize, size}} {}
   Base(TY elemTy, ssa_t* size, bool unspecifiedSize = false) : kind_{Kind::ARRAY_T}, data_{ArrayTy{elemTy, unspecifiedSize, size}} {}

   // function type
   // todo: replace with move
   Base(TY retTy, const std::vector<TY>& paramTys, bool isVarArg) : kind_{Kind::FN_T}, data_{FnTy{paramTys, retTy, isVarArg}} {}

   // Struct or Union type
   Base(token::Kind tk, std::vector<TaggedTY> members, bool incomplete, Ident tag) : kind_{tk == token::Kind::STRUCT ? Kind::STRUCT_T : Kind::UNION_T}, data_{StructOrUnionTy{incomplete, tag, members}} {
      assert(tk == token::Kind::STRUCT || tk == token::Kind::UNION && "Invalid token::Kind for struct or union");
   }

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

   bool variablyModified() const;
   bool isBoolTy() const;
   bool isIntegerTy() const;
   bool isFloatingTy() const;
   bool isDecimalTy() const;
   bool isRealFloatingTy() const;
   bool isBasicTy() const;
   bool isCharacterTy() const;
   bool isArithmeticTy() const;
   bool isCompleteType() const;
   bool isScalarTy() const;
   bool isAggregateTy() const;
   bool isPointerTy() const;
   bool isSignedTy() const;
   bool isSignedCharlikeTy() const;
   bool isVarArg() const;

   TY getRetTy() const;
   const std::vector<TY>& getParamTys() const;
   TY getElemTy() const;
   TY getPointedToTy() const;
   const std::vector<TaggedTY>& getMembers() const;

   Kind kind() const { return kind_; }

   struct StructOrUnionTy {
      bool operator==(const StructOrUnionTy& other) const = default;
      bool operator!=(const StructOrUnionTy& other) const = default;

      bool incomplete;
      Ident tag;
      std::vector<TaggedTY> members;
      // todo: XXX attributes; // e.g. __attribute__((packed))
      // todo: XXX members; // might have flexible array member; might have bitfields; might be anonymous union or structures, might have alignas()
   };

   struct EnumTy {
      bool operator==(const EnumTy& other) const = default;
      bool operator!=(const EnumTy& other) const = default;

      bool incomplete;
      unsigned tag;
      TY underlyingType;
   };

   struct FnTy {
      bool operator==(const FnTy& other) const = default;
      bool operator!=(const FnTy& other) const = default;

      std::vector<TY> paramTys{};
      TY retTy{};
      bool isVarArg;
   };

   struct ArrayTy {
      bool operator==(const ArrayTy& other) const = default;
      bool operator!=(const ArrayTy& other) const = default;

      TY elemTy;
      bool unspecifiedSize;
      std::variant<std::monostate, iconst_t*, ssa_t*> size;
      // See also Example 5 in section 6.7.9.
   };

   VARIANT_ACCESS_METHODS(signedness, Signedness, data_)
   VARIANT_ACCESS_METHODS(ptrTy, TY, data_)
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

   TY* derivedFrom() {
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

   const TY* derivedFrom() const {
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
   std::variant<Signedness, TY, StructOrUnionTy, EnumTy, FnTy, ArrayTy> data_;
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
   assert(kind_ >= Kind::BOOL && kind_ <= Kind::LONGLONG && kind_ != Kind::COMPLEX && "Invalid Base::Kind to rank()");

   return static_cast<int>(kind_);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::variablyModified() const {
   // todo: pointer and function types
   if (kind_ == Kind::ARRAY_T) {
      return std::holds_alternative<ssa_t*>(arrayTy().size);
   } else if (kind_ == Kind::STRUCT_T || kind_ == Kind::UNION_T) {
      for (const auto& member : structOrUnionTy().members) {
         if (member.type.variablyModified()) {
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
   return isIntegerTy() || isRealFloatingTy(); // todo: this is wrong
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::isCompleteType() const {
   switch (kind_) {
      case Kind::VOID:
         return false;
      case Kind::ARRAY_T:
         return getElemTy()->isCompleteType();
      case Kind::STRUCT_T:
      case Kind::UNION_T:
         if (structOrUnionTy().incomplete) {
            return false;
         }
         for (const auto& member : getMembers()) {
            if (!member->isCompleteType()) {
               return false;
            }
         }
         return true;
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
bool Base<_EmitterT>::isVarArg() const {
   return kind_ == Kind::FN_T && fnTy().isVarArg;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Base<_EmitterT>::TY Base<_EmitterT>::getRetTy() const {
   return fnTy().retTy;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
const std::vector<typename Base<_EmitterT>::TY>& Base<_EmitterT>::getParamTys() const {
   return fnTy().paramTys;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Base<_EmitterT>::TY Base<_EmitterT>::getElemTy() const {
   return arrayTy().elemTy;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
typename Base<_EmitterT>::TY Base<_EmitterT>::getPointedToTy() const {
   return ptrTy();
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
const std::vector<TaggedType<_EmitterT>>& Base<_EmitterT>::getMembers() const {
   return structOrUnionTy().members;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Base<_EmitterT>::populateEmitterType(TypeFactory<_EmitterT>& factory, _EmitterT& emitter) {
   assert((!ref_ || ((kind_ == Kind::STRUCT_T || kind_ == Kind::UNION_T) && structOrUnionTy().incomplete)) && "Emitter type already populated");

   unsigned bits = 0;

   // todo: union, enum
   if (kind_ == Kind::PTR_T) {
      if (!getPointedToTy()->isCompleteType()) {
         ref_ = emitter.emitPtrTo(factory.voidTy());
      } else {
         ref_ = emitter.emitPtrTo(ptrTy());
      }
      return;
   } else if (kind_ == Kind::FN_T) {
      ref_ = emitter.emitFnTy(fnTy().retTy, fnTy().paramTys, fnTy().isVarArg);
      return;
   } else if (kind_ == Kind::ARRAY_T) {
      if (iconst_t** size = std::get_if<iconst_t*>(&arrayTy().size)) {
         ref_ = emitter.emitArrayTy(arrayTy().elemTy, *size);
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
      std::vector<TY> members;
      for (const auto& member : structOrUnionTy().members) {
         members.push_back(member.ty);
      }
      ref_ = emitter.emitStructTy(members, structOrUnionTy().incomplete, structOrUnionTy().tag.prefix("struct."));
   } else if (kind_ == Kind::UNION_T) {
      TY max = structOrUnionTy().members.front().ty;
      for (const auto& member : structOrUnionTy().members) {
         if (emitter.getIntegerValue(emitter.sizeOf(member.ty)) > emitter.getIntegerValue(emitter.sizeOf(max))) {
            max = member.ty;
         }
      }
      //  std::max_element(structOrUnionTy().members.begin(), structOrUnionTy().members.end(), [&emitter](const TaggedTY& a, const TaggedTY& b) {            return emitter.getIntegerValue(emitter.sizeOf(a.ty)) < emitter.getIntegerValue(emitter.sizeOf(b.ty));})->ty;
      std::vector<TY> members{max};

      ref_ = emitter.emitStructTy(members, structOrUnionTy().tag.prefix("union."));
   } else {
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
            ptrTy()->strImpl(ss);
            ss << '*';
            break;
         case Kind::ARRAY_T:
            getElemTy()->strImpl(ss);
            if (iconst_t* const* size = std::get_if<iconst_t*>(&arrayTy().size)) {
               ss << '[' << *size << ']';
            } else {
               ss << "[]";
            }
            break;
         case Kind::STRUCT_T:
         case Kind::UNION_T:
            ss << (kind_ == Kind::STRUCT_T ? "struct" : "union");
            if (structOrUnionTy().tag) {
               ss << ' ' << structOrUnionTy().tag;
            }
            if (structOrUnionTy().incomplete) {
               ss << " /* incomplete */";
            }
            ss << " { ";
            for (const auto& member : structOrUnionTy().members) {
               ss << member.ty << ' ' << member.name << "; ";
            }
            ss << '}';
            break;
         case Kind::ENUM_T:
            assert(false && "Not implemented");
            break;
         case Kind::FN_T:
            ss << " function taking (";
            for (size_t i = 0; i < fnTy().paramTys.size(); ++i) {
               ss << fnTy().paramTys[i];
               if (i + 1 < fnTy().paramTys.size()) {
                  ss << ", ";
               }
            }
            if (fnTy().isVarArg) {
               if (!fnTy().paramTys.empty()) {
                  ss << ", ";
               }
               ss << "...";
            }
            ss << ") returning " << fnTy().retTy;
            break;
            // prepend << fnTy().retTy;
            // if (!ss.str().empty()) {
            //    prepend << '(';
            //    ss << ')';
            // }
            // ss << '(';
            // for (size_t i = 0; i < fnTy().paramTys.size(); ++i) {
            //    ss << fnTy().paramTys[i];
            //    if (i + 1 < fnTy().paramTys.size()) {
            //       ss << ", ";
            //    }
            // }
            // if (fnTy().isVarArg) {
            //    if (!fnTy().paramTys.empty()) {
            //       ss << ", ";
            //    }
            //    ss << "...";
            // }
            // ss << ')';
            // prepend << ss.str();
            // ss.str(prepend.str());
            // break;
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