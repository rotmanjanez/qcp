// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "basetype.h"
#include "llvmemitter.h"
#include "operator.h"
#include "type.h"
#include "typefactory.h"
// ---------------------------------------------------------------------------
#include <vector>
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
using OpKind = qcp::op::Kind;
using Instr = llvm::Instruction;
using TY = typename qcp::emitter::LLVMEmitter::TY;
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
   if (ty.isIntegerType() || ty.isPointerType()) {
      return toLLVMIntegralBinOp(kind);
   } else if (ty.isFloatingType()) {
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
      case OpKind::NE: return llvm::CmpInst::Predicate::FCMP_ONE;
      case OpKind::LT: return llvm::CmpInst::Predicate::FCMP_OLT;
      case OpKind::LE: return llvm::CmpInst::Predicate::FCMP_OLE;
      case OpKind::GT: return llvm::CmpInst::Predicate::FCMP_OGT;
      case OpKind::GE: return llvm::CmpInst::Predicate::FCMP_OGE;
      default: return llvm::CmpInst::Predicate::BAD_FCMP_PREDICATE;
   }
}
// ---------------------------------------------------------------------------
llvm::CmpInst::Predicate toLLVMCmpOp(TY ty, OpKind kind) {
   if (ty.isIntegerType()) {
      return ty.isSigned() ? toLLVMSignedCmpOp(kind) : toLLVMUnsignedCmpOp(kind);
   } else if (ty.isFloatingType()) {
      return toLLVMFloatCmpOp(kind);
   }
   return llvm::CmpInst::Predicate::BAD_ICMP_PREDICATE;
}
// ---------------------------------------------------------------------------
Instr::UnaryOps toLLVMUnOp(OpKind kind) {
   switch (kind) {
         // case OpKind::BW_NOT:
      default: return llvm::Instruction::UnaryOps::UnaryOpsEnd;
   }
}
// ---------------------------------------------------------------------------
Instr::OtherOps toLLVMOtherOp(OpKind kind) {
   switch (kind) {
      case OpKind::EQ:
      case OpKind::NE:
      case OpKind::LT:
      case OpKind::LE:
      case OpKind::GT:
      case OpKind::GE: return Instr::ICmp;
      default: return llvm::Instruction::OtherOps::OtherOpsEnd;
   }
}
// ---------------------------------------------------------------------------
std::pair<bool, OpKind> decomposeIncDecOp(OpKind kind) {
   switch (kind) {
      case OpKind::POSTINC: return {true, OpKind::ADD};
      case OpKind::POSTDEC: return {true, OpKind::SUB};
      case OpKind::PREINC: return {false, OpKind::ADD};
      case OpKind::PREDEC: return {false, OpKind::SUB};
      default: return {false, OpKind::END};
   }
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
   for (auto& argTy : argTys) {
      emitterArgTys.push_back(argTy.emitterType());
   }
   return llvm::FunctionType::get(retTy.emitterType(), emitterArgTys, isVarArg);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitArrayTy(TY ty, std::size_t size) {
   return llvm::ArrayType::get(ty.emitterType(), size);
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
   return new llvm::GlobalVariable(*Mod, ty.emitterType(), false, llvm::GlobalValue::ExternalLinkage, nullptr, static_cast<std::string>(name).c_str());
}
// ---------------------------------------------------------------------------
void LLVMEmitter::setInitValueGlobalVar(ssa_t* val, const_t* init) {
   static_cast<llvm::GlobalVariable*>(val)->setInitializer(init);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::fn_t* LLVMEmitter::emitFnProto(Ident name, TY fnTy) {
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
typename LLVMEmitter::bb_t* LLVMEmitter::emitBB(Ident name, fn_t* fn, bb_t* insertBefore) {
   return llvm::BasicBlock::Create(Ctx, static_cast<std::string>(name).c_str(), fn, insertBefore);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitLocalVar(fn_t* fn, bb_t* bb, TY ty, Ident name, ssa_t* init) {
   bb_t* entry = &fn->getEntryBlock();
   ssa_t* alloca = emitAlloca(entry, ty, name);
   if (init) {
      emitStore(entry, init, alloca);
   }
   return alloca;
}
// ---------------------------------------------------------------------------
void LLVMEmitter::setInitValueLocalVar(fn_t* fn, ssa_t* val, ssa_t* init) {
   emitStore(&fn->getEntryBlock(), init, val);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitAlloca(bb_t* bb, TY ty, Ident name) {
   // find the first instruction in the basic block that is not an alloca
   auto it = bb->begin();
   while (it != bb->end() && llvm::isa<llvm::AllocaInst>(it)) {
      ++it;
   }
   // insert the alloca instruction before the first non-alloca instruction
   Builder.SetInsertPoint(bb, it);

   return Builder.CreateAlloca(ty.emitterType(), nullptr, static_cast<std::string>(name).c_str());
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitLoad(bb_t* bb, Ident name, TY ty, ssa_t* ptr) {
   Builder.SetInsertPoint(bb);
   return Builder.CreateLoad(ty.emitterType(), ptr, static_cast<std::string>(name).c_str());
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitStore(bb_t* bb, ssa_t* value, ssa_t* ptr) {
   Builder.SetInsertPoint(bb);
   return Builder.CreateStore(value, ptr);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitJump(bb_t* bb, bb_t* target) {
   return llvm::BranchInst::Create(target, bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitBranch(bb_t* bb, bb_t* trueBB, bb_t* falseBB, ssa_t* cond) {
   return llvm::BranchInst::Create(trueBB, falseBB, cond, bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitRet(bb_t* bb, ssa_t* value) {
   return llvm::ReturnInst::Create(Ctx, value, bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::const_t* LLVMEmitter::emitConst(TY ty, Ident name, long value) {
   return llvm::ConstantInt::get(ty.emitterType(), value, true);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::phi_t* LLVMEmitter::emitPhi(bb_t* bb, Ident name, TY ty) {
   return llvm::PHINode::Create(ty.emitterType(), 0, static_cast<std::string>(name).c_str(), bb);
}
// ---------------------------------------------------------------------------
void LLVMEmitter::addIncoming(Ident name, phi_t* phi, ssa_t* value, bb_t* bb) {}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitBinOp(bb_t* bb, TY ty, Ident name, op::Kind kind, ssa_t* lhs, ssa_t* rhs, ssa_t* dest) {
   if (auto [isAssign, binOp] = toLLVMBinOp(ty, kind); binOp != Instr::BinaryOps::BinaryOpsEnd) {
      auto* result = llvm::BinaryOperator::Create(binOp, lhs, rhs, static_cast<std::string>(name).c_str(), bb);
      if (isAssign) {
         Builder.CreateStore(result, dest);
      }
      return result;
   } else if (kind == OpKind::ASSIGN) {
      Builder.CreateStore(rhs, dest);
      return lhs;
   } else if (auto cmpOp = toLLVMCmpOp(ty, kind); cmpOp != llvm::CmpInst::Predicate::BAD_ICMP_PREDICATE) {
      return llvm::CmpInst::Create(llvm::Instruction::ICmp, cmpOp, lhs, rhs, static_cast<std::string>(name).c_str(), bb);
   }

   assert(false && "not implemented");
   // else if (auto otherOp = toLLVMOtherOp(kind); otherOp != Instr::OtherOps::OtherOpsEnd) {
   //   return llvm::CmpInst::Create(otherOp, llvm::CmpInst::Predicate::ICMP_EQ, lhs, rhs, static_cast<std::string>(name).c_str(), bb);
   // }
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitUnOp(bb_t* bb, TY ty, Ident name, op::Kind kind, ssa_t* operand) {
   if (auto [postInc, op] = decomposeIncDecOp(kind); op != OpKind::END) {
      Instr::BinaryOps binOp = op == OpKind::ADD ? Instr::Add : Instr::Sub;
      ssa_t* value = emitLoad(bb, name, ty, operand);
      auto* one = llvm::ConstantInt::get(ty.emitterType(), 1);
      ssa_t* result = llvm::BinaryOperator::Create(binOp, value, one, "", bb);
      Builder.CreateStore(result, operand);
      return postInc ? operand : result;
   }
   assert(false && "not implemented");
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitCast(bb_t* bb, ssa_t* val, TY toTy, qcp::type::Cast cast) {
   llvm::Instruction::CastOps op = static_cast<llvm::Instruction::CastOps>(
       static_cast<int>(llvm::Instruction::CastOps::Trunc) + static_cast<int>(cast));
   return llvm::CastInst::Create(op, val, toTy.emitterType(), "", bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitCall(bb_t* bb, Ident name, fn_t* fn, const std::vector<ssa_t*>& args) {
   return llvm::CallInst::Create(fn, args, static_cast<std::string>(name).c_str(), bb);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitCall(bb_t* bb, Ident name, TY fnTy, ssa_t* fnPtr, const std::vector<ssa_t*>& args) {
   llvm::FunctionCallee fn{llvm::cast<llvm::FunctionType>(fnTy.emitterType()), fnPtr};
   return llvm::CallInst::Create(fn, args, static_cast<std::string>(name).c_str(), bb);
}
// ---------------------------------------------------------------------------
} // namespace emitter
} // namespace qcp
// ---------------------------------------------------------------------------