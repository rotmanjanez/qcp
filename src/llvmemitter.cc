// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "basetype.h"
#include "llvmemitter.h"
#include "operator.h"
#include "type.h"
#include "typefactory.h"
// ---------------------------------------------------------------------------
#include <span>
#include <string>
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
using LLVMEmitter = qcp::emitter::LLVMEmitter;
using OpKind = qcp::op::Kind;
using Instr = llvm::Instruction;
using TY = typename LLVMEmitter::TY;
// ---------------------------------------------------------------------------
Instr::BinaryOps toLLVMIntegralBinOp(OpKind kind) {
   switch (kind) {
      case OpKind::ADD: return Instr::Add;
      case OpKind::SUB: return Instr::Sub;
      case OpKind::MUL: return Instr::Mul;
      case OpKind::DIV: return Instr::SDiv;
      case OpKind::REM: return Instr::SRem;
      case OpKind::L_AND: return Instr::And;
      case OpKind::L_OR: return Instr::Or;
      case OpKind::BW_XOR: return Instr::Xor;
      case OpKind::SHL: return Instr::Shl;
      case OpKind::SHR: return Instr::AShr;
      default: return llvm::Instruction::BinaryOps::BinaryOpsEnd;
   }
}
// ---------------------------------------------------------------------------
Instr::BinaryOps toLLVMFloatingBinOp(OpKind kind) {
   switch (kind) {
      case OpKind::ADD: return Instr::FAdd;
      case OpKind::SUB: return Instr::FSub;
      case OpKind::MUL: return Instr::FMul;
      case OpKind::DIV: return Instr::FDiv;
      case OpKind::REM: return Instr::FRem;
      default: return llvm::Instruction::BinaryOps::BinaryOpsEnd;
   }
}
// ---------------------------------------------------------------------------
OpKind decomposeAssignOp(OpKind kind) {
   switch (kind) {
      case OpKind::ADD_ASSIGN: return OpKind::ADD;
      case OpKind::SUB_ASSIGN: return OpKind::SUB;
      case OpKind::MUL_ASSIGN: return OpKind::MUL;
      case OpKind::DIV_ASSIGN: return OpKind::DIV;
      case OpKind::REM_ASSIGN: return OpKind::REM;
      case OpKind::BW_AND_ASSIGN: return OpKind::L_AND;
      case OpKind::BW_OR_ASSIGN: return OpKind::L_OR;
      case OpKind::BW_XOR_ASSIGN: return OpKind::BW_XOR;
      case OpKind::SHL_ASSIGN: return OpKind::SHL;
      case OpKind::SHR_ASSIGN: return OpKind::SHR;
      default: return OpKind::END;
   }
}
// ---------------------------------------------------------------------------
Instr::BinaryOps toLLVMRValBinOp(TY ty, OpKind kind) {
   if (ty.isIntegerTy() || ty.isPointerTy()) {
      return toLLVMIntegralBinOp(kind);
   } else if (ty.isFloatingTy()) {
      return toLLVMFloatingBinOp(kind);
   }
   return llvm::Instruction::BinaryOps::BinaryOpsEnd;
}
// ---------------------------------------------------------------------------
std::pair<bool, Instr::BinaryOps> toLLVMBinOp(TY ty, OpKind kind) {
   llvm::Instruction::BinaryOps binOp = llvm::Instruction::BinaryOps::BinaryOpsEnd;

   OpKind assignOp = decomposeAssignOp(kind);
   kind = assignOp != OpKind::END ? assignOp : kind;

   return {assignOp != OpKind::END, toLLVMRValBinOp(ty, kind)};
}
// ---------------------------------------------------------------------------
llvm::CmpInst::Predicate toLLVMSignedCmpOp(OpKind kind) {
   switch (kind) {
      case OpKind::EQ: return llvm::CmpInst::Predicate::ICMP_EQ;
      case OpKind::NE: return llvm::CmpInst::Predicate::ICMP_NE;
      case OpKind::LT: return llvm::CmpInst::Predicate::ICMP_SLT;
      case OpKind::LE: return llvm::CmpInst::Predicate::ICMP_SLE;
      case OpKind::GT: return llvm::CmpInst::Predicate::ICMP_SGT;
      case OpKind::GE: return llvm::CmpInst::Predicate::ICMP_SGE;
      default: return llvm::CmpInst::Predicate::BAD_ICMP_PREDICATE;
   }
}
// ---------------------------------------------------------------------------
llvm::CmpInst::Predicate toLLVMUnsignedCmpOp(OpKind kind) {
   switch (kind) {
      case OpKind::EQ: return llvm::CmpInst::Predicate::ICMP_EQ;
      case OpKind::NE: return llvm::CmpInst::Predicate::ICMP_NE;
      case OpKind::LT: return llvm::CmpInst::Predicate::ICMP_ULT;
      case OpKind::LE: return llvm::CmpInst::Predicate::ICMP_ULE;
      case OpKind::GT: return llvm::CmpInst::Predicate::ICMP_UGT;
      case OpKind::GE: return llvm::CmpInst::Predicate::ICMP_UGE;
      default: return llvm::CmpInst::Predicate::BAD_ICMP_PREDICATE;
   }
}
// ---------------------------------------------------------------------------
llvm::CmpInst::Predicate toLLVMFloatCmpOp(OpKind kind) {
   switch (kind) {
      case OpKind::EQ: return llvm::CmpInst::Predicate::FCMP_OEQ;
      case OpKind::NE: return llvm::CmpInst::Predicate::FCMP_UNE;
      case OpKind::LT: return llvm::CmpInst::Predicate::FCMP_OLT;
      case OpKind::LE: return llvm::CmpInst::Predicate::FCMP_OLE;
      case OpKind::GT: return llvm::CmpInst::Predicate::FCMP_OGT;
      case OpKind::GE: return llvm::CmpInst::Predicate::FCMP_OGE;
      default: return llvm::CmpInst::Predicate::BAD_FCMP_PREDICATE;
   }
}
// ---------------------------------------------------------------------------
llvm::CmpInst::Predicate toLLVMCmpOp(TY ty, OpKind kind) {
   if (ty.isIntegerTy()) {
      return ty.isSignedTy() ? toLLVMSignedCmpOp(kind) : toLLVMUnsignedCmpOp(kind);
   } else if (ty.isFloatingTy()) {
      return toLLVMFloatCmpOp(kind);
   }
   return llvm::CmpInst::Predicate::BAD_ICMP_PREDICATE;
}
// ---------------------------------------------------------------------------
std::pair<bool, int> decomposeIncDecOp(OpKind kind) {
   switch (kind) {
      case OpKind::POSTINC: return {true, 1};
      case OpKind::POSTDEC: return {true, -1};
      case OpKind::PREINC: return {false, 1};
      case OpKind::PREDEC: return {false, -1};
      default:
         assert(false && "Invalid inc/dec operator");
         return {false, 0};
   }
}
// ---------------------------------------------------------------------------
inline typename LLVMEmitter::ssa_t* asLLVMValue(const typename LLVMEmitter::value_t& val) {
   if (typename LLVMEmitter::ssa_t* const* ssa = std::get_if<typename LLVMEmitter::ssa_t*>(&val)) {
      return *ssa;
   } else if (typename LLVMEmitter::iconst_t* const* iconst = std::get_if<typename LLVMEmitter::iconst_t*>(&val)) {
      return *iconst;
   }
   return std::get<typename LLVMEmitter::const_t*>(val);
}
// ---------------------------------------------------------------------------
llvm::Instruction::CastOps toLLVMCastOp(qcp::type::Cast cast) {
   return static_cast<llvm::Instruction::CastOps>(static_cast<int>(llvm::Instruction::CastOps::Trunc) + static_cast<int>(cast));
}
// ---------------------------------------------------------------------------
llvm::Constant* toLLVMConstant(typename LLVMEmitter::const_or_iconst_t& val) {
   if (typename LLVMEmitter::const_t* const* c = std::get_if<typename LLVMEmitter::const_t*>(&val)) {
      return *c;
   }
   return static_cast<typename LLVMEmitter::const_t*>(std::get<typename LLVMEmitter::iconst_t*>(val));
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::const_or_iconst_t toQCPConstant(llvm::Constant* val) {
   if (llvm::ConstantInt* c = llvm::dyn_cast<llvm::ConstantInt>(val)) {
      return c;
   }
   return val;
}
// ---------------------------------------------------------------------------
llvm::Constant* llvmUint23T(llvm::LLVMContext& Ctx, uint32_t val) {
   return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Ctx), val);
}
// ---------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------
namespace qcp {
namespace emitter {
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitVoidTy() {
   return llvm::Type::getVoidTy(Ctx);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitIntTy(unsigned bits) {
   return llvm::IntegerType::get(Ctx, bits);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitFloatTy() {
   return llvm::Type::getFloatTy(Ctx);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitDoubleTy() {
   return llvm::Type::getDoubleTy(Ctx);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitPtrTo(TY ty) {
   return ty.emitterType()->getPointerTo();
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitFnTy(TY retTy, std::vector<TY> argTys, bool isVarArg) {
   std::vector<ty_t*> emitterArgTys;
   emitterArgTys.reserve(argTys.size());
   for (auto& argTy : argTys) {
      emitterArgTys.push_back(argTy.emitterType());
   }
   return llvm::FunctionType::get(retTy.emitterType(), emitterArgTys, isVarArg);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitArrayTy(TY ty, iconst_t* size) {
   return llvm::ArrayType::get(ty.emitterType(), size->getZExtValue());
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitStructTy(const std::vector<TY>& tys, Ident name) {
   std::vector<ty_t*> llvmTys;
   llvmTys.reserve(tys.size());
   for (const auto& ty : tys) {
      llvmTys.push_back(ty.emitterType());
   }
   return llvm::StructType::create(Ctx, llvmTys, static_cast<std::string>(name));
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitUndef() {
   return llvm::UndefValue::get(llvm::Type::getVoidTy(Ctx));
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitPoison() {
   return llvm::PoisonValue::get(llvm::Type::getVoidTy(Ctx));
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitGlobalVar(TY ty, Ident name) {
   // todo: bool to i8
   return new llvm::GlobalVariable(*Mod, ty.emitterType(), false, llvm::GlobalValue::ExternalLinkage, nullptr, static_cast<std::string>(name).c_str());
}
// ---------------------------------------------------------------------------
void LLVMEmitter::setInitValueGlobalVar(ssa_t* val, const_or_iconst_t init) {
   static_cast<llvm::GlobalVariable*>(val)->setInitializer(toLLVMConstant(init));
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::fn_t* LLVMEmitter::emitFnProto(TY fnTy, Ident name) {
   return llvm::Function::Create(static_cast<llvm::FunctionType*>(fnTy.emitterType()), llvm::Function::ExternalLinkage, static_cast<std::string>(name).c_str(), Mod);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::bb_t* LLVMEmitter::emitFn(fn_t* fnProto) {
   return llvm::BasicBlock::Create(Ctx, "entry", fnProto);
}
// ---------------------------------------------------------------------------
bool LLVMEmitter::isFnProto(fn_t* fn) {
   return fn->isDeclaration();
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::getParam(fn_t* fn, unsigned idx) {
   return fn->getArg(idx);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::bb_t* LLVMEmitter::emitBB(fn_t* fn, bb_t* insertBefore, Ident name) {
   return llvm::BasicBlock::Create(Ctx, static_cast<std::string>(name).c_str(), fn, insertBefore);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitLocalVar(fn_t* fn, bb_t* entry, TY ty, Ident name, bool insertAtBegin) {
   return emitAllocaImpl(entry, ty, nullptr, name, insertAtBegin);
}
// ---------------------------------------------------------------------------
void LLVMEmitter::setInitValueLocalVar(fn_t* fn, TY ty, ssa_t* val, value_t init) {
   emitStore(&fn->getEntryBlock(), ty, init, val);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitAllocaImpl(bb_t* bb, TY ty, ssa_t* size, Ident name, bool insertAtBegin) {
   // todo: performance is bad...
   auto it = bb->begin();
   while (!insertAtBegin && it != bb->end() && llvm::isa<llvm::AllocaInst>(it)) {
      ++it;
   }
   // insert the alloca instruction before the first non-alloca instruction
   Builder.SetInsertPoint(bb, it);
   llvm::Type* type = ty.emitterType();
   if (ty.isBoolTy()) {
      type = llvm::Type::getInt8Ty(Ctx);
   }

   return Builder.CreateAlloca(type, size, static_cast<std::string>(name).c_str());
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitAlloca(bb_t* bb, TY ty, ssa_t* size, Ident name) {
   return emitAllocaImpl(bb, ty, size, name, false);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitLoad(bb_t* bb, TY ty, ssa_t* ptr, Ident name) {
   Builder.SetInsertPoint(bb);
   if (ty.isBoolTy()) {
      auto result = Builder.CreateLoad(llvm::Type::getInt8Ty(Ctx), ptr, static_cast<std::string>(name).c_str());
      return llvm::CastInst::Create(llvm::CastInst::Trunc, result, llvm::Type::getInt1Ty(Ctx), "", bb);
   }
   return Builder.CreateLoad(ty.emitterType(), ptr, static_cast<std::string>(name).c_str());
}
// ---------------------------------------------------------------------------
void LLVMEmitter::emitStore(bb_t* bb, TY ty, value_t value, ssa_t* ptr) {
   Builder.SetInsertPoint(bb);
   auto llvmVal = asLLVMValue(value);
   if (ty.isBoolTy()) {
      llvmVal = llvm::CastInst::Create(llvm::CastInst::ZExt, llvmVal, llvm::Type::getInt8Ty(Ctx), "", bb);
   }
   Builder.CreateStore(llvmVal, ptr);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitJump(bb_t* bb, bb_t* target) {
   return llvm::BranchInst::Create(target, bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitBranch(bb_t* bb, bb_t* trueBB, bb_t* falseBB, value_t cond) {
   return llvm::BranchInst::Create(trueBB, falseBB, asLLVMValue(cond), bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitRet(bb_t* bb, value_t value) {
   return llvm::ReturnInst::Create(Ctx, asLLVMValue(value), bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::const_or_iconst_t LLVMEmitter::emitConst(TY ty, long value) {
   if (ty.isFloatingTy()) {
      return llvm::ConstantFP::get(ty.emitterType(), static_cast<double>(value));
   }
   return static_cast<iconst_t*>(llvm::ConstantInt::get(ty.emitterType(), value, true));
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::phi_t* LLVMEmitter::emitPhi(bb_t* bb, TY ty, Ident name) {
   return llvm::PHINode::Create(ty.emitterType(), 0, static_cast<std::string>(name).c_str(), bb);
}
// ---------------------------------------------------------------------------
void LLVMEmitter::addIncoming(phi_t* phi, ssa_t* value, bb_t* bb, Ident name) {}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitBinOp(bb_t* bb, TY ty, op::Kind kind, value_t lhs, value_t rhs, ssa_t* dest, Ident name) {
   ssa_t* lhs_ = asLLVMValue(lhs);
   ssa_t* rhs_ = asLLVMValue(rhs);
   if (auto [isAssign, binOp] = toLLVMBinOp(ty, kind); binOp != Instr::BinaryOps::BinaryOpsEnd) {
      auto* result = llvm::BinaryOperator::Create(binOp, lhs_, rhs_, static_cast<std::string>(name).c_str(), bb);
      if (isAssign) {
         Builder.CreateStore(result, dest);
      }
      return result;
   } else if (kind == OpKind::ASSIGN) {
      Builder.CreateStore(rhs_, dest);
      return lhs_;
   } else if (auto cmpOp = toLLVMCmpOp(ty, kind); cmpOp != llvm::CmpInst::Predicate::BAD_ICMP_PREDICATE) {
      auto cmpInst = ty.isFloatingTy() ? llvm::Instruction::FCmp : llvm::Instruction::ICmp;
      return llvm::CmpInst::Create(cmpInst, cmpOp, lhs_, rhs_, static_cast<std::string>(name).c_str(), bb);
   }

   assert(false && "not implemented");
   // else if (auto otherOp = toLLVMOtherOp(kind); otherOp != Instr::OtherOps::OtherOpsEnd) {
   //   return llvm::CmpInst::Create(otherOp, llvm::CmpInst::Predicate::ICMP_EQ, lhs, rhs, static_cast<std::string>(name).c_str(), bb);
   // }
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::const_or_iconst_t LLVMEmitter::emitConstBinOp([[maybe_unused]] bb_t* bb, TY ty, op::Kind kind, const_or_iconst_t lhs, const_or_iconst_t rhs, [[maybe_unused]] Ident name) {
   auto lhs_ = toLLVMConstant(lhs);
   auto rhs_ = toLLVMConstant(rhs);
   if (auto [isAssign, binOp] = toLLVMBinOp(ty, kind); binOp != Instr::BinaryOps::BinaryOpsEnd) {
      return toQCPConstant(llvm::ConstantExpr::get(binOp, lhs_, rhs_));
   } else if (auto cmpOp = toLLVMCmpOp(ty, kind); cmpOp != llvm::CmpInst::Predicate::BAD_ICMP_PREDICATE) {
      return static_cast<iconst_t*>(llvm::ConstantExpr::getCompare(cmpOp, lhs_, rhs_));
   }

   assert(false && "not implemented");
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitIncDecOp(bb_t* bb, TY ty, op::Kind kind, ssa_t* operand, Ident name) {
   auto [isPost, plusMinusOne] = decomposeIncDecOp(kind);
   ssa_t* value = emitLoad(bb, ty, operand, name);
   const_t* incDecVal = llvm::ConstantInt::get(ty.emitterType(), plusMinusOne);
   ssa_t* result = llvm::BinaryOperator::Create(Instr::Add, value, incDecVal, "", bb);
   Builder.CreateStore(result, operand);
   return isPost ? value : result;
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitNeg(bb_t* bb, TY ty, ssa_t* operand, Ident name) {
   if (ty.isFloatingTy()) {
      return llvm::UnaryOperator::Create(Instr::FNeg, operand, static_cast<std::string>(name).c_str(), bb);
   } else if (ty.kind() == type::Kind::BOOL) {
      llvm::ConstantInt* True = llvm::ConstantInt::getTrue(Ctx);
      return llvm::BinaryOperator::Create(Instr::Xor, operand, True, static_cast<std::string>(name).c_str(), bb);
   }
   const_t* zero = llvm::ConstantInt::get(ty.emitterType(), 0);
   return llvm::BinaryOperator::Create(Instr::Sub, zero, operand, static_cast<std::string>(name).c_str(), bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::const_or_iconst_t LLVMEmitter::emitConstNeg([[maybe_unused]] bb_t* bb, TY ty, const_or_iconst_t operand, [[maybe_unused]] Ident name) {
   // todo: check if this is correct
   return toQCPConstant(llvm::ConstantExpr::getNeg(toLLVMConstant(operand)));
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitBWNeg(bb_t* bb, TY ty, ssa_t* operand, Ident name) {
   const_t* allOnes = llvm::ConstantInt::getAllOnesValue(ty.emitterType());
   return llvm::BinaryOperator::Create(Instr::Xor, operand, allOnes, static_cast<std::string>(name).c_str(), bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::const_or_iconst_t LLVMEmitter::emitConstBWNeg([[maybe_unused]] bb_t* bb, TY ty, const_or_iconst_t operand, [[maybe_unused]] Ident name) {
   const_t* allOnes = llvm::ConstantInt::getAllOnesValue(ty.emitterType());
   return toQCPConstant(llvm::ConstantExpr::getXor(toLLVMConstant(operand), allOnes));
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::const_or_iconst_t LLVMEmitter::emitConstCast([[maybe_unused]] bb_t* bb, [[maybe_unused]] TY fromTy, const_or_iconst_t val, TY toTy, qcp::type::Cast cast) {
   switch (cast) {
      case qcp::type::Cast::TRUNC:
      case qcp::type::Cast::PTRTOINT:
      case qcp::type::Cast::INTTOPTR:
      case qcp::type::Cast::BITCAST:
         return toQCPConstant(llvm::ConstantExpr::getCast(toLLVMCastOp(cast), toLLVMConstant(val), toTy.emitterType()));
      case qcp::type::Cast::FPTRUNC:
      case qcp::type::Cast::FPEXT:
      case qcp::type::Cast::ZEXT:
      case qcp::type::Cast::SEXT:
      case qcp::type::Cast::UITOFP:
      case qcp::type::Cast::SITOFP:
      default:
         throw std::runtime_error("Invalid cast");
         // todo: assert(false && "Invalid cast");
   }
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitCast(bb_t* bb, [[maybe_unused]] TY fromTy, ssa_t* val, TY toTy, qcp::type::Cast cast) {
   return llvm::CastInst::Create(toLLVMCastOp(cast), val, toTy.emitterType(), "", bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitCall(bb_t* bb, fn_t* fn, const std::vector<value_t>& args, Ident name) {
   llvm::FunctionCallee callee{fn};
   return emitCall(bb, callee, args, name);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitCall(bb_t* bb, TY fnTy, ssa_t* fnPtr, const std::vector<value_t>& args, Ident name) {
   llvm::FunctionCallee fn{llvm::cast<llvm::FunctionType>(fnTy.emitterType()), fnPtr};
   return emitCall(bb, fn, args, name);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitCall(bb_t* bb, llvm::FunctionCallee fn, const std::vector<value_t>& args, Ident name) {
   std::vector<llvm::Value*> emitterArgs;
   emitterArgs.reserve(args.size());
   for (auto& arg : args) {
      emitterArgs.push_back(asLLVMValue(arg));
   }
   return llvm::CallInst::Create(fn, emitterArgs, static_cast<std::string>(name).c_str(), bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::iconst_t* LLVMEmitter::sizeOf(TY ty) {
   auto size = Mod->getDataLayout().getTypeAllocSize(ty.emitterType());
   return llvm::ConstantInt::get(llvm::Type::getInt64Ty(Ctx), size);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitGEP(bb_t* bb, TY ty, value_t ptr, value_t idx, Ident name) {
   return emitGEPImpl(bb, ty, asLLVMValue(ptr), {asLLVMValue(idx), nullptr, nullptr}, name);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitGEP(bb_t* bb, TY ty, value_t ptr, std::array<uint32_t, 2> idx, Ident name) {
   return emitGEPImpl(bb, ty, asLLVMValue(ptr), {llvmUint23T(Ctx, idx[0]), llvmUint23T(Ctx, idx[1]), nullptr}, name);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitGEP(bb_t* bb, TY ty, value_t ptr, std::array<uint32_t, 3> idx, Ident name) {
   return emitGEPImpl(bb, ty, asLLVMValue(ptr), {llvmUint23T(Ctx, idx[0]), llvmUint23T(Ctx, idx[1]), llvmUint23T(Ctx, idx[2])}, name);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitGEPImpl(bb_t* bb, TY ty, value_t ptr, std::array<ssa_t*, 3> idx, Ident name) {
   Builder.SetInsertPoint(bb);
   llvm::ArrayRef<llvm::Value*> indices{idx[0], idx[1], idx[2]};
   while (indices.size() > 0 && indices.back() == nullptr) {
      indices = indices.drop_back();
   }
   return Builder.CreateGEP(ty.emitterType(), asLLVMValue(ptr), indices, static_cast<std::string>(name).c_str());
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::sw_t* LLVMEmitter::emitSwitch(bb_t* bb, value_t value) {
   return llvm::SwitchInst::Create(asLLVMValue(value), nullptr, 0, bb);
}
// ---------------------------------------------------------------------------
void LLVMEmitter::addSwitchCase(sw_t* sw, iconst_t* value, bb_t* target) {
   // todo iconst instead of const
   sw->addCase(value, target);
}
// ---------------------------------------------------------------------------
void LLVMEmitter::addSwitchDefault(sw_t* sw, bb_t* target) {
   sw->setDefaultDest(target);
}
// ---------------------------------------------------------------------------
} // namespace emitter
} // namespace qcp
// ---------------------------------------------------------------------------