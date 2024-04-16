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
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
struct BaseType;
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
class BaseTypeFactory;
// ---------------------------------------------------------------------------
enum class BaseTypeKind {
   // clang-format off
   BOOL, CHAR, SHORT, INT, LONG, LONGLONG,
   
   FLOAT, DOUBLE, LONGDOUBLE,

   DECIMAL32, DECIMAL64, DECIMAL128,

   NULLPTR_T,
   VOID,

   PTR_T, ARRAY_T, STRUCT_T, ENUM_T, UNION_T, FN_T,
   UNDEF,
   // clang-format on
};
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
class Type {
   using BTY = BaseType<_IRValueRef>;
   using Factory = BaseTypeFactory<_IRValueRef>;
   using Token = token::Token;

   template <typename T>
   friend std::ostream& operator<<(std::ostream& os, const Type<T>& ty);

   Type(unsigned short typeIndex) : typeIndex_{typeIndex}, qualifiers{false, false, false} {}

   public:
   Type() : typeIndex_{0}, qualifiers{false, false, false} {}

   explicit Type(BaseTypeKind kind) : typeIndex_{Factory::constructKind(kind)}, qualifiers{false, false, false} {}

   static Type discardQualifiers(const Type& other) {
      return Type(other.typeIndex_);
   }

   Type(BaseTypeKind integerKind, bool unsignedTy) : typeIndex_{Factory::constructKind(integerKind, unsignedTy)}, qualifiers{false, false, false} {}

   static Type ptrTo(const Type& other);

   static Type fromToken(const Token& token);

   static Type fromConstToken(const Token& token);

   Type promote() const {
      // todo: assert(baseType().kind <= BaseTypeKind::LONGLONG && "Invalid BaseType::Kind to promote()");
      // todo: promote integers
      return Type(*this);
   }

   static Type commonRealType(op::Kind op, const Type& lhs, const Type& rhs);

   const BTY& baseType() const {
      return Factory::getTypes()[typeIndex_];
   }

   bool operator==(const Type<_IRValueRef>& other) const {
      return baseType() == other.baseType();
   }

   auto operator<=>(const Type<_IRValueRef>& other) const {
      return baseType().rank() <=> other.baseType().rank();
   }

   private:
   // todo: maybe short is not enough
   unsigned short typeIndex_;

   struct qualifiers {
      bool CONST;
      bool RESTRICT;
      bool ATOMIC;
   } qualifiers;
};
// ---------------------------------------------------------------------------
// todo: _BitInt(N)
// todo: _Complex float
// todo: _Imaginary float
// todo: _Complex double
// todo: _Imaginary double
// todo: _Complex long double
// todo: _Imaginary long double
template <typename _IRValueRef>
struct BaseType {
   using TY = Type<_IRValueRef>;

   static BaseType ptr_t(const TY& ty) {
      BaseType t{BaseTypeKind::PTR_T};
      t.ptrTy = ty;
      return t;
   }

   BaseTypeKind kind;

   BaseType() : kind{BaseTypeKind::UNDEF} {}

   explicit BaseType(BaseTypeKind kind) : kind{kind} {}
   BaseType(BaseTypeKind integerKind, bool unsignedTy) : kind{integerKind}, unsingedTy{unsignedTy} {
      assert(kind >= BaseTypeKind::BOOL && kind <= BaseTypeKind::DECIMAL128 && "Invalid BaseType::Kind for integer");
   }

   int rank() const;

   bool operator==(const BaseType& other) const;

   auto operator<=>(const BaseType& other) const {
      return rank() <=> other.rank();
   }

   struct StructOrUnionTy {
      bool incomplete;
      bool isStruct;
      unsigned tag;
      // todo: XXX attributes; // e.g. __attribute__((packed))
      // todo: XXX members; // might have flexible array member; might have bitfields; might be anonymous union or structures, might have alignas()
   };

   struct EnumTy {
      bool incomplete;
      unsigned tag;
      TY underlyingType;
   };

   // todo: vector in union not possible
   // std::vector<TY> argTys;
   struct FnTy {
      TY retTy;
      bool isVarArg;
   };

   struct ArrayTy {
      TY elemTy;
      union {
         size_t fixedSize;
         // See also Example 5 in section 6.7.9.
         _IRValueRef* varSize;
         bool unspecifiedSize;
         bool sizeNotPresent;
      };
   };

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
template <typename _IRValueRef>
class BaseTypeFactory {
   using BTY = BaseType<_IRValueRef>;

   public:
   static BTY* ptrTo(const Type<_IRValueRef>& other);

   static unsigned short constructKind(BaseTypeKind kind);

   static unsigned short constructKind(BaseTypeKind integerKind, bool unsignedTy);

   static const std::vector<BTY>& getTypes();

   private:
   static std::vector<BTY> types;
};
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
std::ostream& operator<<(std::ostream& os, const BaseType<_IRValueRef>& ty) {
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
   if ((ty.kind < BaseTypeKind::FLOAT || (ty.kind >= BaseTypeKind::DECIMAL32 && ty.kind <= BaseTypeKind::DECIMAL128)) && ty.unsingedTy) {
      os << "unsigned ";
   }
   if (ty.kind <= BaseTypeKind::VOID) {
      os << names[static_cast<int>(ty.kind)];
   } else {
      switch (ty.kind) {
         case BaseTypeKind::PTR_T:
            os << ty.ptrTy << "*";
            break;
         case BaseTypeKind::ARRAY_T:
            // todo: differnt array sizes
            os << ty.arrayTy.elemTy << "[" << ty.arrayTy.fixedSize << "]";
            break;
         case BaseTypeKind::STRUCT_T:
            // todo: name / anonymous
            os << "struct";
            break;
         case BaseTypeKind::ENUM_T:
            // todo: name / anonymous
            os << "enum";
            break;
         case BaseTypeKind::UNION_T:
            // todo: name / anonymous
            os << "union";
            break;
         case BaseTypeKind::FN_T:
            // todo: varargs
            os << ty.fnTy.retTy << " fn(";
            // todo: for (auto& arg : ty.fnTy.argTys) {
            // todo:    os << arg << ", ";
            // todo: }
            os << "missing args";
            os << ")";
            break;
         case BaseTypeKind::UNDEF:
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
template <typename _IRValueRef>
std::ostream& operator<<(std::ostream& os, const Type<_IRValueRef>& ty) {
   if (ty.qualifiers.CONST) {
      os << "const ";
   }
   if (ty.qualifiers.RESTRICT) {
      os << "restrict ";
   }
   if (ty.qualifiers.ATOMIC) {
      os << "_Atomic ";
   }
   os << ty.baseType();
   return os;
}
// ---------------------------------------------------------------------------
// Type
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
Type<_IRValueRef> Type<_IRValueRef>::fromToken(const Token& token) {
   token::Kind tk = token.getType();
   if (tk >= token::Kind::ICONST && tk <= token::Kind::LDCONST) {
      return fromConstToken(token);
   }

   return Type();
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
Type<_IRValueRef> Type<_IRValueRef>::fromConstToken(const Token& token) {
   token::Kind tk = token.getType();
   assert(tk >= token::Kind::ICONST && tk <= token::Kind::LDCONST && "Invalid token::Kind to create Type from");

   int tkVal = static_cast<int>(tk);
   bool hasSigness = tk <= token::Kind::ULL_ICONST;

   int tVal;
   if (hasSigness) {
      tkVal -= static_cast<int>(token::Kind::ICONST);
      // compensate for duplicate tokens with signess
      tVal = static_cast<int>(BaseTypeKind::INT) + tkVal / 2;
   } else {
      tVal = static_cast<int>(BaseTypeKind::FLOAT) + tkVal - static_cast<int>(token::Kind::FCONST);
   }

   return Type(static_cast<BaseTypeKind>(tVal), hasSigness && (tkVal & 1));
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
Type<_IRValueRef> Type<_IRValueRef>::ptrTo(const Type<_IRValueRef>& other) {
   return {BTY::ptr_t(other)};
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
Type<_IRValueRef> Type<_IRValueRef>::commonRealType(op::Kind kind, const Type& lhs, const Type& rhs) {
   // todo: undef not necessary in when type of identifier is known
   if (lhs.baseType().kind == BaseTypeKind::UNDEF || rhs.baseType().kind == BaseTypeKind::UNDEF) {
      return Type();
   }

   // Usual arithmetic conversions
   // todo: If one operand has decimal floating type, the other operand shall not have standard floating, complex, or imaginary type.

   if (kind <= op::Kind::ALIGNOF) {
      // todo: differtent unary operators yield different types. replace by a switch statement
      return Type::discardQualifiers(lhs);
   }

   const Type& higher = lhs > rhs ? lhs : rhs;

   if (higher.baseType().kind >= BaseTypeKind::FLOAT) {
      return Type::discardQualifiers(higher);
   }
   // todo: enums

   // integer promotion
   // todo: what happens with qualifiers?
   const Type& lhsPromoted{lhs.promote()};
   const Type& rhsPromoted{rhs.promote()};

   if (lhsPromoted == rhsPromoted) {
      std::cout << "lhsPromoted == rhsPromoted\n";
      return Type::discardQualifiers(lhsPromoted);
   } else if (lhsPromoted.baseType().unsingedTy == rhsPromoted.baseType().unsingedTy) {
      std::cout << "same signess\n";
      return Type::discardQualifiers(lhsPromoted > rhsPromoted ? lhsPromoted : rhsPromoted);
   } else {
      const Type& unsignedTy = lhsPromoted.baseType().unsingedTy ? lhsPromoted : rhsPromoted;
      const Type& signedTy = lhsPromoted.baseType().unsingedTy ? rhsPromoted : lhsPromoted;
      if (unsignedTy >= signedTy) {
         std::cout << "unsignedTy >= signedTy\n";
         return Type::discardQualifiers(unsignedTy);
      } else if (signedTy > unsignedTy) {
         std::cout << "signedTy > unsignedTy\n";
         return Type::discardQualifiers(signedTy);
      } else {
         std::cout << "signed but unsigned\n";
         return Type(signedTy.baseType().kind, true);
      }
   }
}
// ---------------------------------------------------------------------------
// BaseType
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
bool BaseType<_IRValueRef>::operator==(const BaseType& other) const {
   if (kind != other.kind) {
      return false;
   }

   switch (kind) {
      case BaseTypeKind::CHAR:
      case BaseTypeKind::SHORT:
      case BaseTypeKind::INT:
      case BaseTypeKind::LONG:
      case BaseTypeKind::LONGLONG:
      case BaseTypeKind::FLOAT:
      case BaseTypeKind::DOUBLE:
      case BaseTypeKind::LONGDOUBLE:
      case BaseTypeKind::DECIMAL32:
      case BaseTypeKind::DECIMAL64:
      case BaseTypeKind::DECIMAL128:
         return unsingedTy == other.unsingedTy;
      case BaseTypeKind::BOOL:
      case BaseTypeKind::NULLPTR_T:
      case BaseTypeKind::VOID:
         return true;
      case BaseTypeKind::PTR_T:
         return ptrTy == other.ptrTy;
      case BaseTypeKind::STRUCT_T:
         return structOrUnionTy.tag == other.structOrUnionTy.tag;
      case BaseTypeKind::ENUM_T:
         return enumTy.tag == other.enumTy.tag;
      case BaseTypeKind::FN_T:
         return fnTy.retTy == other.fnTy.retTy && fnTy.isVarArg == other.fnTy.isVarArg;
      case BaseTypeKind::ARRAY_T:
         return arrayTy.elemTy == other.arrayTy.elemTy && arrayTy.fixedSize == other.arrayTy.fixedSize;
      default:
         return false;
   }
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
int BaseType<_IRValueRef>::rank() const {
   if (kind == BaseTypeKind::ENUM_T) {
      return enumTy.underlyingType.baseType().rank();
   }
   if (kind == BaseTypeKind::UNDEF) {
      // todo: (jr) how to handle this?
      return -1;
   }
   assert(kind <= BaseTypeKind::DECIMAL128 && "Invalid BaseType::Kind to rank()");

   return static_cast<int>(kind);
}
// ---------------------------------------------------------------------------
// BaseTypeFactory
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
std::vector<BaseType<_IRValueRef>> BaseTypeFactory<_IRValueRef>::types{BTY()};
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
BaseTypeFactory<_IRValueRef>::BTY* BaseTypeFactory<_IRValueRef>::ptrTo(const Type<_IRValueRef>& other) {
   auto it = std::find_if(types.begin(), types.end(), [&other](const BTY& t) { return t.kind == BaseTypeKind::PTR_T && t.ptrTy == other; });
   if (it != types.end()) {
      return &*it;
   }
   types.push_back(BTY::ptr_t(other));
   return &types.back();
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
unsigned short BaseTypeFactory<_IRValueRef>::constructKind(BaseTypeKind kind) {
   auto it = std::find_if(types.begin(), types.end(), [kind](const BTY& t) { return t.kind == kind; });
   if (it != types.end()) {
      return std::distance(types.begin(), it);
   }
   BTY t{kind};
   types.push_back(t);
   return types.size() - 1;
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
unsigned short BaseTypeFactory<_IRValueRef>::constructKind(BaseTypeKind integerKind, bool unsignedTy) {
   assert(integerKind >= BaseTypeKind::BOOL && integerKind <= BaseTypeKind::DECIMAL128 && "Invalid BaseType::Kind for integer");

   auto it = std::find_if(types.begin(), types.end(), [integerKind, unsignedTy](const BTY& t) { return t.kind == integerKind && t.unsingedTy == unsignedTy; });
   if (it != types.end()) {
      return std::distance(types.begin(), it);
   }
   types.push_back(BTY(integerKind, unsignedTy));
   return types.size() - 1;
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
const std::vector<typename BaseTypeFactory<_IRValueRef>::BTY>& BaseTypeFactory<_IRValueRef>::getTypes() {
   return types;
}
// ---------------------------------------------------------------------------
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TYPE_H