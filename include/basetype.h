#ifndef QCP_BASE_TYPE_H
#define QCP_BASE_TYPE_H
// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "operator.h"
#include "token.h"
#include "type.h"
// ---------------------------------------------------------------------------
#include <compare>
#include <sstream>
#include <vector>
// ---------------------------------------------------------------------------
namespace qcp {
namespace type {
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
   using emitter_ty_t = typename _EmitterT::ty_t;
   using ssa_t = typename _EmitterT::ssa_t;
   using iconst_t = typename _EmitterT::iconst_t;

   unsigned CHAR_BITS = _EmitterT::CHAR_HAS_16_BIT ? 16 : 8;
   static constexpr unsigned SHORT_BITS = 16;
   static constexpr unsigned INT_BITS = _EmitterT::INT_HAS_64_BIT ? 64 : 32;
   static constexpr unsigned LONG_BITS = _EmitterT::LONG_HAS_64_BIT ? 64 : 32;
   static constexpr unsigned LONG_LONG_BITS = 64;

   Base() : kind{Kind::END} {}

   // todo: (jr) remove this constructor
   constexpr explicit Base(Kind kind) : kind{kind}, signedness{Signedness::UNSPECIFIED} {}

   Base(Kind integerKind, bool unsignedTy) : kind{integerKind}, signedness{unsignedTy ? Signedness::UNSIGNED : Signedness::SIGNED} {
      assert(kind >= Kind::BOOL && kind <= Kind::LONGLONG && kind != Kind::COMPLEX && "Invalid Base::Kind for integer");
   }

   // pointer type
   Base(TY ptrTy) : kind{Kind::PTR_T}, ptrTy{ptrTy} {}

   // array type
   Base(TY elemTy, iconst_t* size, bool unspecifiedSize = false) : kind{Kind::ARRAY_T}, arrayTy{elemTy, unspecifiedSize, false, {.fixedSize = size}} {}
   Base(TY elemTy, ssa_t* size, bool unspecifiedSize = false) : kind{Kind::ARRAY_T}, arrayTy{elemTy, unspecifiedSize, true, {.valueRef = size}} {}

   // function type
   // todo: replace with move
   Base(TY retTy, const std::vector<TY>& paramTys, bool isVarArg) : kind{Kind::FN_T}, fnTy{paramTys, retTy, isVarArg} {}

   // Struct or Union type
   Base(token::Kind tk, std::vector<TaggedTY> members, bool incomplete) : kind{tk == token::Kind::STRUCT ? Kind::STRUCT_T : Kind::UNION_T}, structOrUnionTy{incomplete, 0, members} {
      assert(tk == token::Kind::STRUCT || tk == token::Kind::UNION && "Invalid token::Kind for struct or union");
   }

   ~Base();
   // copy and move
   Base(const Base&) = delete;
   Base(Base&& other);
   Base& operator=(const Base&) = delete;
   Base& operator=(Base&& other);

   bool operator==(const Base& other) const;
   std::strong_ordering operator<=>(const Base& other) const;

   int rank() const;

   bool variablyModified() const {
      // todo: pointer and function types
      if (kind == Kind::ARRAY_T) {
         return arrayTy.varSize;
      } else if (kind == Kind::STRUCT_T || kind == Kind::UNION_T) {
         for (const auto& member : structOrUnionTy.members) {
            if (member.type.variablyModified()) {
               return true;
            }
         }
      }
      return false;
   }

   void populateEmitterType(_EmitterT& emitter);

   std::string str() const {
      std::stringstream ss;
      strImpl(ss);
      return ss.str();
   }

   void strImpl(std::stringstream& ss) const {
      static const char* names[] = {
#define ENUM_AS_STRING
#include "defs/types.def"
#undef ENUM_AS_STRING
      };
      if ((kind < Kind::FLOAT || (kind >= Kind::DECIMAL32 && kind <= Kind::DECIMAL128)) && signedness == Signedness::UNSIGNED) {
         ss << "unsigned ";
      }
      if (kind < Kind::NULLPTR_T) {
         ss << names[static_cast<int>(kind)];
      } else {
         std::stringstream prepend;
         switch (kind) {
            case Kind::PTR_T:
               ss << '*';
               ptrTy.base().strImpl(ss);
               break;
            case Kind::ARRAY_T:
               arrayTy.elemTy.base().strImpl(ss);
               if (arrayTy.varSize) {
                  ss << "[]";
               } else {
                  ss << '[' << arrayTy.fixedSize << ']';
               }
               break;
            case Kind::STRUCT_T:
            case Kind::UNION_T:
            case Kind::ENUM_T:
               assert(false && "Not implemented");
               break;
            case Kind::FN_T:
               prepend << fnTy.retTy;
               if (!ss.str().empty()) {
                  prepend << '(';
                  ss << ')';
               }
               ss << '(';
               for (size_t i = 0; i < fnTy.paramTys.size(); ++i) {
                  ss << fnTy.paramTys[i];
                  if (i + 1 < fnTy.paramTys.size()) {
                     ss << ", ";
                  }
               }
               if (fnTy.isVarArg) {
                  if (!fnTy.paramTys.empty()) {
                     ss << ", ";
                  }
                  ss << "...";
               }
               ss << ')';
               prepend << ss.str();
               ss.str(prepend.str());
               break;
            case Kind::END:
               ss << "undef";
               break;
            default:
               ss << "unknown(" << static_cast<int>(kind) << ')';
               break;
         }
      }
   }

   struct StructOrUnionTy {
      bool incomplete;
      unsigned tag;
      std::vector<TaggedTY> members;
      // todo: XXX attributes; // e.g. __attribute__((packed))
      // todo: XXX members; // might have flexible array member; might have bitfields; might be anonymous union or structures, might have alignas()
   };

   struct EnumTy {
      bool incomplete;
      unsigned tag;
      TY underlyingType;
   };

   struct FnTy {
      std::vector<TY> paramTys;
      TY retTy;
      bool isVarArg;
   };

   struct ArrayTy {
      TY elemTy;
      bool unspecifiedSize;
      bool varSize;
      union {
         iconst_t* fixedSize;
         // See also Example 5 in section 6.7.9.
         ssa_t* valueRef;
      };
   };

   Kind kind;
   emitter_ty_t* ref = nullptr;
   union {
      Signedness signedness;
      TY ptrTy;
      StructOrUnionTy structOrUnionTy;
      EnumTy enumTy;
      FnTy fnTy;
      ArrayTy arrayTy;
   };
};
// ---------------------------------------------------------------------------
// Base
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Base<_EmitterT>::~Base() {
   switch (kind) {
      case Kind::PTR_T:
         ptrTy.~TY();
         break;
      case Kind::STRUCT_T:
      case Kind::UNION_T:
         structOrUnionTy.~StructOrUnionTy();
         break;
      case Kind::ENUM_T:
         enumTy.~EnumTy();
         break;
      case Kind::FN_T:
         fnTy.~FnTy();
         break;
      case Kind::ARRAY_T:
         arrayTy.~ArrayTy();
         break;
      default:
         break;
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Base<_EmitterT>::Base(Base&& other) : kind{other.kind} {
   (*this) = std::move(other);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
Base<_EmitterT>& Base<_EmitterT>::operator=(Base&& other) {
   if (this == &other) {
      return *this;
   }

   kind = other.kind;
   ref = other.ref;
   switch (kind) {
      case Kind::PTR_T:
         ptrTy = other.ptrTy;
         break;
      case Kind::STRUCT_T:
      case Kind::UNION_T:
         // todo:
         structOrUnionTy = std::move(other.structOrUnionTy);
         break;
      case Kind::ENUM_T:
         enumTy = other.enumTy;
         break;
      case Kind::FN_T:
         fnTy.isVarArg = other.fnTy.isVarArg;
         fnTy.paramTys = std::move(other.fnTy.paramTys);
         fnTy.retTy = other.fnTy.retTy;
         break;
      case Kind::ARRAY_T:
         arrayTy = other.arrayTy;
         break;
      default:
         signedness = other.signedness;
         break;
   }

   return *this;
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
bool Base<_EmitterT>::operator==(const Base& other) const {
   if (kind != other.kind) {
      return false;
   }

   switch (kind) {
      case Kind::CHAR:
      case Kind::SHORT:
      case Kind::INT:
      case Kind::LONG:
      case Kind::LONGLONG:
      case Kind::FLOAT:
      case Kind::DOUBLE:
      case Kind::LONGDOUBLE:
      case Kind::DECIMAL32:
      case Kind::DECIMAL64:
      case Kind::DECIMAL128:
         return signedness == other.signedness;
      case Kind::BOOL:
      case Kind::NULLPTR_T:
      case Kind::VOID:
         return true;
      case Kind::PTR_T:
         return ptrTy == other.ptrTy;
      case Kind::STRUCT_T:
         return structOrUnionTy.tag == other.structOrUnionTy.tag;
      case Kind::ENUM_T:
         return enumTy.tag == other.enumTy.tag;
      case Kind::FN_T:
         return fnTy.retTy == other.fnTy.retTy && fnTy.isVarArg == other.fnTy.isVarArg && fnTy.paramTys == other.fnTy.paramTys;
      case Kind::ARRAY_T:
         return arrayTy.elemTy == other.arrayTy.elemTy && arrayTy.fixedSize == other.arrayTy.fixedSize;
      default:
         return false;
   }
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
std::strong_ordering Base<_EmitterT>::operator<=>(const Base& other) const {
   return rank() <=> other.rank();
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
int Base<_EmitterT>::rank() const {
   if (kind == Kind::ENUM_T) {
      return enumTy.underlyingType.base().rank();
   }
   if (kind == Kind::END) {
      // todo: (jr) how to handle this?
      return -1;
   }
   assert(kind >= Kind::BOOL && kind <= Kind::LONGLONG && kind != Kind::COMPLEX && "Invalid Base::Kind to rank()");

   return static_cast<int>(kind);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Base<_EmitterT>::populateEmitterType(_EmitterT& emitter) {
   assert(!ref && "Emitter type already populated");

   unsigned bits = 0;

   if (kind == Kind::PTR_T) {
      ref = emitter.emitPtrTo(ptrTy);
      return;
   } else if (kind == Kind::FN_T) {
      ref = emitter.emitFnTy(fnTy.retTy, fnTy.paramTys, fnTy.isVarArg);
      return;
   } else if (kind == Kind::ARRAY_T) {
      if (arrayTy.varSize) {
         ref = arrayTy.elemTy.emitterType();
      } else {
         ref = emitter.emitArrayTy(arrayTy.elemTy, arrayTy.fixedSize);
      }
      return;
   }
   // todo: struct, union, enum

   switch (kind) {
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
         ref = emitter.emitVoidTy();
         return;
      case Kind::FLOAT:
         ref = emitter.emitFloatTy();
         return;
      case Kind::DOUBLE:
         ref = emitter.emitDoubleTy();
         return;
      default:
         // todo: (jr) not implemented
         std::cerr << "Invalid Base::Kind to populateEmitterType(): " << *this << std::endl;
         assert(false && "Invalid Base::Kind to populateEmitterType()");
   }
   ref = emitter.emitIntTy(bits);
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