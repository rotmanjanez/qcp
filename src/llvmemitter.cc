// ---------------------------------------------------------------------------
// qcp
// ---------------------------------------------------------------------------
#include "llvmemitter.h"
#include "operator.h"
#include "type.h"
// ---------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------
using OpKind = qcp::op::Kind;
using Instr = llvm::Instruction;
// ---------------------------------------------------------------------------
Instr::BinaryOps toLLVMBinOp(OpKind kind) {
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
Instr::UnaryOps toLLVMUnOp(OpKind kind) {
   switch (kind) {
         // case OpKind::BW_NOT:
      default: return llvm::Instruction::UnaryOps::UnaryOpsEnd;
   }
}
// ---------------------------------------------------------------------------
Instr::OtherOps toLLVMOtherOp(OpKind kind) {
   switch (kind) {
      case OpKind::EQ: return Instr::ICmp;
      case OpKind::NE: return Instr::ICmp;
      case OpKind::LT: return Instr::ICmp;
      case OpKind::LE: return Instr::ICmp;
      case OpKind::GT: return Instr::ICmp;
      case OpKind::GE: return Instr::ICmp;
      default: return llvm::Instruction::OtherOps::OtherOpsEnd;
   }
}
// ---------------------------------------------------------------------------
Instr::BinaryOps decomposeAssignOp(OpKind kind) {
   switch (kind) {
      case OpKind::ADD_ASSIGN: return Instr::Add;
      case OpKind::SUB_ASSIGN: return Instr::Sub;
      case OpKind::MUL_ASSIGN: return Instr::Mul;
      case OpKind::DIV_ASSIGN: return Instr::SDiv;
      case OpKind::REM_ASSIGN: return Instr::SRem;
      case OpKind::SHL_ASSIGN: return Instr::Shl;
      case OpKind::SHR_ASSIGN: return Instr::AShr;
      case OpKind::BW_AND_ASSIGN: return Instr::And;
      case OpKind::BW_XOR_ASSIGN: return Instr::Xor;
      case OpKind::BW_OR_ASSIGN: return Instr::Or;
      default: return llvm::Instruction::BinaryOps::BinaryOpsEnd;
   }
}
// ---------------------------------------------------------------------------
std::pair<bool, Instr::BinaryOps> decomposeIncDecOp(OpKind kind) {
   switch (kind) {
      case OpKind::POSTINC: return {true, Instr::Add};
      case OpKind::POSTDEC: return {true, Instr::Sub};
      case OpKind::PREINC: return {false, Instr::Add};
      case OpKind::PREDEC: return {false, Instr::Sub};
      default: return {false, llvm::Instruction::BinaryOps::BinaryOpsEnd};
   }
}
// ---------------------------------------------------------------------------
} // namespace
// ---------------------------------------------------------------------------
namespace qcp {
namespace emitter {
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitPtrTo(ty_t* ty) {
   return ty->getPointerTo();
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitIntTy(unsigned bits) {
   return llvm::IntegerType::get(Ctx, bits);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ty_t* LLVMEmitter::emitFnTy(ty_t* retTy, std::vector<ty_t*> argTys) {
   return llvm::FunctionType::get(retTy, argTys, false);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitGlobalVar(ty_t* ty, Ident name, const_t* init) {
   return new llvm::GlobalVariable(*Mod, ty, false, llvm::GlobalValue::ExternalLinkage, init, static_cast<std::string>(name).c_str());
}
// ---------------------------------------------------------------------------
void LLVMEmitter::setInitValueGlobalVar(ssa_t* val, const_t* init) {
   static_cast<llvm::GlobalVariable*>(val)->setInitializer(init);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::fn_t* LLVMEmitter::emitFnProto(Ident name, ty_t* fnTy) {
   return llvm::Function::Create(static_cast<llvm::FunctionType*>(fnTy), llvm::Function::ExternalLinkage, static_cast<std::string>(name).c_str(), Mod);
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
typename LLVMEmitter::bb_t* LLVMEmitter::emitBB(Ident name, fn_t* fn) {
   return llvm::BasicBlock::Create(Ctx, static_cast<std::string>(name).c_str(), fn);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitLocalVar(fn_t* fn, bb_t* bb, ty_t* ty, Ident name, ssa_t* init) {
   bb_t* entry = &fn->getEntryBlock();
   ssa_t* alloca = emitAlloca(entry, ty, name);
   if (init) {
      emitStore(entry, init, alloca);
   }
   return alloca;
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitAlloca(bb_t* bb, ty_t* ty, Ident name) {
   Builder.SetInsertPoint(bb, bb->begin());
   return Builder.CreateAlloca(ty, nullptr, static_cast<std::string>(name).c_str());
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitLoad(bb_t* bb, Ident name, ty_t* ty, ssa_t* ptr) {
   Builder.SetInsertPoint(bb);
   return Builder.CreateLoad(ty, ptr, static_cast<std::string>(name).c_str());
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
typename LLVMEmitter::ssa_t* LLVMEmitter::emitConst(bb_t* bb, ty_t* ty, Ident name, long value) {
   return llvm::ConstantInt::get(ty, value, true);
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::phi_t* LLVMEmitter::emitPhi(bb_t* bb, Ident name, ty_t* ty) {
   return llvm::PHINode::Create(ty, 0, static_cast<std::string>(name).c_str(), bb);
}
// ---------------------------------------------------------------------------
void LLVMEmitter::addIncoming(Ident name, phi_t* phi, ssa_t* value, bb_t* bb) {}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitBinOp(bb_t* bb, Ident name, op::Kind kind, ssa_t* lhs, ssa_t* rhs) {
   if (auto binOp = toLLVMBinOp(kind); binOp != Instr::BinaryOps::BinaryOpsEnd) {
      return llvm::BinaryOperator::Create(binOp, lhs, rhs, static_cast<std::string>(name).c_str(), bb);
   } else if (decomposeAssignOp(kind) != Instr::BinaryOps::BinaryOpsEnd) {
      auto* result = llvm::BinaryOperator::Create(decomposeAssignOp(kind), lhs, rhs, "", bb);
      Builder.CreateStore(result, lhs);
      return result;
   } else if (kind == OpKind::ASSIGN) {
      Builder.CreateStore(rhs, lhs);
      return lhs;
   }
   assert(false && "not implemented");
   // else if (auto otherOp = toLLVMOtherOp(kind); otherOp != Instr::OtherOps::OtherOpsEnd) {
   //   return llvm::CmpInst::Create(otherOp, llvm::CmpInst::Predicate::ICMP_EQ, lhs, rhs, static_cast<std::string>(name).c_str(), bb);
   // }
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitUnOp(bb_t* bb, Ident name, op::Kind kind, ssa_t* operand) {
   if (auto [postInc, op] = decomposeIncDecOp(kind); op != Instr::BinaryOps::BinaryOpsEnd) {
      auto* result = llvm::BinaryOperator::Create(op, operand, llvm::ConstantInt::get(operand->getType(), 1), "", bb);
      Builder.CreateStore(result, operand);
      return postInc ? operand : result;
   }
   assert(false && "not implemented");
}
// ---------------------------------------------------------------------------
typename LLVMEmitter::ssa_t* LLVMEmitter::emitCall(bb_t* bb, Ident name, fn_t* fn, const std::vector<ssa_t*>& args) {
   return llvm::CallInst::Create(fn, args, static_cast<std::string>(name).c_str(), bb);
}
// ---------------------------------------------------------------------------
} // namespace emitter
} // namespace qcp
// ---------------------------------------------------------------------------