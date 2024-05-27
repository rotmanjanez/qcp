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
   using emitter_ssa_t = typename _EmitterT::ssa_t;

   Base() : kind{Kind::UNDEF} {}

   // todo: (jr) remove this constructor
   explicit Base(Kind kind) : kind{kind}, unsingedTy{false} {}

   Base(Kind integerKind, bool unsignedTy) : kind{integerKind}, unsingedTy{unsignedTy} {
      assert(kind >= Kind::BOOL && kind <= Kind::DECIMAL128 && "Invalid Base::Kind for integer");
   }

   // pointer type
   Base(TY ptrTy) : kind{Kind::PTR_T}, ptrTy{ptrTy} {}

   // array type
   Base(TY elemTy, size_t size, bool unspecifiedSize = false, bool varLen = false) : kind{Kind::ARRAY_T}, arrayTy{elemTy, unspecifiedSize, varLen, {size}} {}

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

   void populateEmitterType(_EmitterT& emitter);

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
         size_t fixedSize;
         // See also Example 5 in section 6.7.9.
         emitter_ssa_t* valueRef;
      };
   };

   Kind kind;
   emitter_ty_t* ref;
   union {
      bool unsingedTy;
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
         unsingedTy = other.unsingedTy;
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
         return unsingedTy == other.unsingedTy;
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
         return fnTy.retTy == other.fnTy.retTy && fnTy.isVarArg == other.fnTy.isVarArg;
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
   if (kind == Kind::UNDEF) {
      // todo: (jr) how to handle this?
      return -1;
   }
   assert(kind <= Kind::DECIMAL128 && "Invalid Base::Kind to rank()");

   return static_cast<int>(kind);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
void Base<_EmitterT>::populateEmitterType(_EmitterT& emitter) {
   assert(!ref && "Emitter type already populated");

   unsigned bits = 0;

   if (kind == Kind::PTR_T) {
      ref = emitter.emitPtrTo(ptrTy.emitterType());
      return;
   } else if (kind == Kind::FN_T) {
      std::vector<emitter_ty_t*> paramTys;
      for (const auto& paramTy : fnTy.paramTys) {
         // todo: (jr) very inefficient
         paramTys.push_back(paramTy.emitterType());
      }
      ref = emitter.emitFnTy(fnTy.retTy.emitterType(), paramTys);
      return;
   } // todo: struct, union, enum

   switch (kind) {
      case Kind::BOOL:
         bits = 1;
         break;
      case Kind::CHAR:
         bits = _EmitterT::CHAR_BITS;
         break;
      case Kind::SHORT:
         bits = _EmitterT::SHORT_BITS;
         break;
      case Kind::INT:
         bits = _EmitterT::INT_BITS;
         break;
      case Kind::LONG:
         bits = _EmitterT::LONG_BITS;
         break;
      case Kind::LONGLONG:
         bits = _EmitterT::LONG_LONG_BITS;
         break;
      default:
         assert(false && "Invalid Base::Kind to populateEmitterType");
         // todo: (jr) not implemented
   }
   ref = emitter.emitIntTy(bits);
}
// ---------------------------------------------------------------------------
template <typename _EmitterT>
std::ostream& operator<<(std::ostream& os, const Base<_EmitterT>& ty) {
   static const char* names[] = {
       // clang-format off
      "bool", "char", "short", "int", "long", "long long",
      "float", "double", "long double",
      "decimal32", "decimal64", "decimal128",
      "nullptr_t",
      "void",
      "undef",
       // clang-format on
   };
   if ((ty.kind < Kind::FLOAT || (ty.kind >= Kind::DECIMAL32 && ty.kind <= Kind::DECIMAL128)) && ty.unsingedTy) {
      os << "unsigned ";
   }
   if (ty.kind <= Kind::VOID) {
      os << names[static_cast<int>(ty.kind)];
   } else {
      switch (ty.kind) {
         case Kind::PTR_T:
            os << "ptr to " << ty.ptrTy;
            break;
         case Kind::ARRAY_T:
            os << "array of " << ty.arrayTy.elemTy;
            break;
         case Kind::STRUCT_T:
         case Kind::UNION_T:
            // todo: name / anonymous
            if (ty.kind == Kind::STRUCT_T) {
               os << "struct { ";
            } else {
               os << "union { ";
            }
            for (const auto& member : ty.structOrUnionTy.members) {
               os << member << "; ";
            }
            os << '}';
            break;
         case Kind::ENUM_T:
            // todo: name / anonymous
            os << "enum";
            break;
         case Kind::FN_T:
            // todo: varargs
            // todo: args
            os << "function ";
            if (!ty.fnTy.paramTys.empty()) {
               os << "taking ";
               for (const auto& paramTy : ty.fnTy.paramTys) {
                  os << paramTy
                     << (paramTy == ty.fnTy.paramTys.back() ? ' ' : ',');
               }
               os << "and ";
            }
            if (ty.fnTy.isVarArg) {
               os << "accepting any number of arguments and ";
            }
            os << "returning " << ty.fnTy.retTy;
            break;
         case Kind::UNDEF:
            os << "undef";
            break;
         default:
            os << "unknown(" << static_cast<int>(ty.kind) << ')';
            break;
      }
   }
   return os;
}
// ---------------------------------------------------------------------------
} // namespace type
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_BASE_TYPE_H