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
template <typename _IRValueRef>
struct Base;
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
class BaseFactory;
// ---------------------------------------------------------------------------
enum class Kind {
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
   using Base = Base<_IRValueRef>;
   using Factory = BaseFactory<_IRValueRef>;
   using Token = token::Token;

   template <typename T>
   friend std::ostream& operator<<(std::ostream& os, const Type<T>& ty);

   explicit Type(unsigned short typeIndex) : typeIndex_{typeIndex} {}

   public:
   Type() : typeIndex_{0} {}

   explicit Type(Kind kind) : typeIndex_{Factory::construct(kind)} {}

   static Type discardQualifiers(const Type& other) {
      return Type(other.typeIndex_);
   }

   Type(Kind integerKind, bool unsignedTy) : typeIndex_{Factory::construct(integerKind, unsignedTy)} {}

   static Type ptrTo(const Type& other);

   static Type arrayOf(const Type& base) {
      return Type{Factory::construct(base, 0, true, false)};
   }

   static Type arrayOf(const Type& base, size_t size, bool varLen = false, bool unspecifiedSize = false) {
      return Type{Factory::construct(base, size, unspecifiedSize, varLen)};
   }

   // todo: replace with move
   static Type function(const Type& retTy, const std::vector<Type>& argTys, bool isVarArg = false) {
      return Type{Factory::construct(retTy, argTys, isVarArg)};
   }

   // todo: replace with move
   static Type structOrUnion(token::Kind tk, const std::vector<Type>& members, bool incomplete) {
      return Type{Factory::construct(tk, members, incomplete)};
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

   static Type fromToken(const Token& token);

   static Type fromConstToken(const Token& token);

   Type promote() const {
      // todo: assert(Base().kind <= Kind::LONGLONG && "Invalid Base::Kind to promote()");
      // todo: promote integers
      return Type(*this);
   }

   static Type commonRealType(op::Kind op, const Type& lhs, const Type& rhs);

   Base& base() const {
      return Factory::getTypes()[typeIndex_];
   }

   unsigned short typeIndex() const {
      return typeIndex_;
   }

   bool operator==(const Type<_IRValueRef>& other) const {
      return qualifiers == other.qualifiers && base() == other.base();
   }

   auto operator<=>(const Type<_IRValueRef>& other) const {
      return base().rank() <=> other.base().rank();
   }

   operator bool() const {
      return typeIndex_ != 0;
   }

   struct qualifiers {
      bool operator==(const qualifiers& other) const {
         return CONST == other.CONST && RESTRICT == other.RESTRICT && VOLATILE == other.VOLATILE;
      }

      bool CONST = false;
      bool RESTRICT = false;
      bool VOLATILE = false;
      // todo: bool ATOMIC = false;
   } qualifiers;

   private:
   // todo: maybe short is not enough
   unsigned short typeIndex_;
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
struct Base {
   using TY = Type<_IRValueRef>;

   Kind kind;

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
   Base(TY retTy, const std::vector<TY>& argTys, bool isVarArg) : kind{Kind::FN_T}, fnTy{argTys, retTy, isVarArg} {}

   // Struct or Union type
   Base(token::Kind tk, std::vector<TY> members, bool incomplete) : kind{tk == token::Kind::STRUCT ? Kind::STRUCT_T : Kind::UNION_T}, structOrUnionTy{incomplete, 0, members} {
      assert(tk == token::Kind::STRUCT || tk == token::Kind::UNION && "Invalid token::Kind for struct or union");
   }

   ~Base() {
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

   // copy constructor
   Base(const Base&) = delete;

   // move constructor
   Base(Base&& other) : kind{other.kind} {
      (*this) = std::move(other);
   }

   // copy assignment
   Base& operator=(const Base&) = delete;

   // move assignment
   Base& operator=(Base&& other) {
      if (this == &other) {
         return *this;
      }

      kind = other.kind;
      switch (kind) {
         case Kind::PTR_T:
            ptrTy = other.ptrTy;
            break;
         case Kind::STRUCT_T:
         case Kind::UNION_T:
            structOrUnionTy = std::move(other.structOrUnionTy);
            break;
         case Kind::ENUM_T:
            enumTy = other.enumTy;
            break;
         case Kind::FN_T:
            fnTy = std::move(other.fnTy);
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

   int rank() const;

   bool operator==(const Base& other) const;

   auto operator<=>(const Base& other) const {
      return rank() <=> other.rank();
   }

   struct StructOrUnionTy {
      bool incomplete;
      unsigned tag;
      std::vector<TY> members;
      // todo: XXX attributes; // e.g. __attribute__((packed))
      // todo: XXX members; // might have flexible array member; might have bitfields; might be anonymous union or structures, might have alignas()
   };

   struct EnumTy {
      bool incomplete;
      unsigned tag;
      TY underlyingType;
   };

   // todo: vector in union not possible
   struct FnTy {
      std::vector<TY> argTys;
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
         _IRValueRef* valueRef;
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
class BaseFactory {
   using Base = Base<_IRValueRef>;
   using TY = Type<_IRValueRef>;

   public:
   class DeclTypeBaseRef {
      public:
      DeclTypeBaseRef() : typeIndex_{0} {}
      DeclTypeBaseRef(unsigned short typeIndex) : typeIndex_{typeIndex} {}
      DeclTypeBaseRef(const TY& ty) : typeIndex_{ty.typeIndex()} {}

      TY& operator*() const {
         Base& base = BaseFactory::getTypes()[typeIndex_];
         switch (base.kind) {
            case Kind::PTR_T:
               return base.ptrTy;
            case Kind::ARRAY_T:
               return base.arrayTy.elemTy;
            case Kind::ENUM_T:
               return base.enumTy.underlyingType;
            case Kind::FN_T:
               return base.fnTy.retTy;
            default:
               std::cerr << "Invalid Base::Kind to dereference: " << base << "\n";
               assert(false && "Invalid Base::Kind to dereference\n");
         }
         // todo: what to return here?
      }

      void chain(TY ty) {
         DeclTypeBaseRef& _this = *this;

         if (_this) {
            *_this = ty;
            _this = DeclTypeBaseRef(*_this);
         } else {
            _this = DeclTypeBaseRef(ty);
         }
      }

      operator bool() const {
         return typeIndex_ != 0;
      }

      private:
      unsigned short typeIndex_;
   };

   template <typename... Args>
   static unsigned short construct(Args... args);

   static std::vector<Base>& getTypes();

   private:
   static std::vector<Base> types;
};
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
std::ostream& operator<<(std::ostream& os, const Base<_IRValueRef>& ty) {
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
            if (!ty.fnTy.argTys.empty()) {
               os << "taking ";
               for (const auto& argTy : ty.fnTy.argTys) {
                  os << argTy
                     << (argTy == ty.fnTy.argTys.back() ? ' ' : ',');
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
template <typename _IRValueRef>
std::ostream& operator<<(std::ostream& os, const Type<_IRValueRef>& ty) {
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
// Type
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
Type<_IRValueRef> Type<_IRValueRef>::fromToken(const Token& token) {
   token::Kind tk = token.getKind();
   if (tk >= token::Kind::ICONST && tk <= token::Kind::LDCONST) {
      return fromConstToken(token);
   }

   return Type();
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
Type<_IRValueRef> Type<_IRValueRef>::fromConstToken(const Token& token) {
   token::Kind tk = token.getKind();
   assert(tk >= token::Kind::ICONST && tk <= token::Kind::LDCONST && "Invalid token::Kind to create Type from");

   int tkVal = static_cast<int>(tk);
   bool hasSigness = tk <= token::Kind::ULL_ICONST;

   int tVal;
   if (hasSigness) {
      tkVal -= static_cast<int>(token::Kind::ICONST);
      // compensate for duplicate tokens with signess
      tVal = static_cast<int>(Kind::INT) + tkVal / 2;
   } else {
      tVal = static_cast<int>(Kind::FLOAT) + tkVal - static_cast<int>(token::Kind::FCONST);
   }

   return Type(static_cast<Kind>(tVal), hasSigness && (tkVal & 1));
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
Type<_IRValueRef> Type<_IRValueRef>::ptrTo(const Type<_IRValueRef>& other) {
   return Type{Factory::construct(other)};
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
Type<_IRValueRef> Type<_IRValueRef>::commonRealType(op::Kind kind, const Type& lhs, const Type& rhs) {
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
   const Type& lhsPromoted{lhs.promote()};
   const Type& rhsPromoted{rhs.promote()};

   if (lhsPromoted == rhsPromoted) {
      std::cout << "lhsPromoted == rhsPromoted\n";
      return Type::discardQualifiers(lhsPromoted);
   } else if (lhsPromoted.base().unsingedTy == rhsPromoted.base().unsingedTy) {
      std::cout << "same signess\n";
      return Type::discardQualifiers(lhsPromoted > rhsPromoted ? lhsPromoted : rhsPromoted);
   } else {
      const Type& unsignedTy = lhsPromoted.base().unsingedTy ? lhsPromoted : rhsPromoted;
      const Type& signedTy = lhsPromoted.base().unsingedTy ? rhsPromoted : lhsPromoted;
      if (unsignedTy >= signedTy) {
         std::cout << "unsignedTy >= signedTy\n";
         return Type::discardQualifiers(unsignedTy);
      } else if (signedTy > unsignedTy) {
         std::cout << "signedTy > unsignedTy\n";
         return Type::discardQualifiers(signedTy);
      } else {
         std::cout << "signed but unsigned\n";
         return Type(signedTy.base().kind, true);
      }
   }
}
// ---------------------------------------------------------------------------
// Base
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
bool Base<_IRValueRef>::operator==(const Base& other) const {
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
template <typename _IRValueRef>
int Base<_IRValueRef>::rank() const {
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
// BaseFactory
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
std::vector<Base<_IRValueRef>> BaseFactory<_IRValueRef>::types(1);
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
template <typename... Args>
unsigned short BaseFactory<_IRValueRef>::construct(Args... args) {
   if (types.empty()) {
      types.emplace_back(Kind::UNDEF);
   }

   auto it = std::find(types.begin(), types.end(), Base(args...));

   if (it != types.end()) {
      return std::distance(types.begin(), it);
   }
   types.emplace_back(args...);
   return types.size() - 1;
}
// ---------------------------------------------------------------------------
template <typename _IRValueRef>
std::vector<typename BaseFactory<_IRValueRef>::Base>& BaseFactory<_IRValueRef>::getTypes() {
   return types;
}
// ---------------------------------------------------------------------------
} // namespace type
} // namespace qcp
// ---------------------------------------------------------------------------
#endif // QCP_TYPE_H