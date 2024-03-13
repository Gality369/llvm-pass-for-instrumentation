//
// Created by Gality on 2024/3/11.
//
#include "tools.h"

#include <stdio.h>
#include <stdlib.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

class Coverage : public ModulePass {
public:
    static char ID;

    Coverage() : ModulePass(ID) {}

    bool runOnModule(Module &M) override;
};


char Coverage::ID = 0;

bool Coverage::runOnModule(Module &M) {
  LLVMContext &ctx = M.getContext();

  IntegerType *Int8Ty = IntegerType::getInt8Ty(ctx);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(ctx);

  /* Decide instrumentation ratio */
  unsigned int inst_ratio = 100;

  // 指向 "用于输出控制流覆盖次数的共享内存" 的指针(其实就是一个hashtable, 继续看下面就知道了)
  // * 注意，GlobalVariable::ExternalLinkage指定了该全局变量是一个外部链接，在编译时会由链接器替换成真实变量(定义在llvm-rt.o.c中)
  auto *mapPtr =
          new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                             GlobalVariable::ExternalLinkage, 0, "__area_ptr");
  // __prev_loc is thread-local, 表示前一个基本块的编号
  auto *prevLoc = new GlobalVariable(
          M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__prev_loc",
          0, GlobalVariable::GeneralDynamicTLSModel, 0, false);

  /* Instrument all the things! */
  int inst_blocks = 0;
  for (auto &F: M)
    for (auto &BB: F) {
      BasicBlock::iterator IP = BB.getFirstInsertionPt();
      IRBuilder<> IRB(&(*IP));

      // 控制插桩覆盖率
      if (Rand(100) >= inst_ratio) continue;

      /* Make up cur_loc */
      // 当前的基础块编号, 随机获取
      unsigned int cur_loc = Rand(MAP_SIZE);
      ConstantInt *CurLoc = ConstantInt::get(Int32Ty, cur_loc);

      /* Load prev_loc */
      // 加载上一个基础块的编号
      LoadInst *PrevLoc = IRB.CreateLoad(prevLoc);
      // 增加调试信息(元数据)
      PrevLoc->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(ctx, None));
      // 零扩展 将PrevLoc转为32位无符号整数，代表前一个基础块编号
      Value *PrevLocCasted = IRB.CreateZExt(PrevLoc, IRB.getInt32Ty());

      // 获取指向共享内存的指针
      LoadInst *MapPtr = IRB.CreateLoad(mapPtr);
      MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(ctx, None));
      // 根据当前基础块与上一个基础块的编号，计算出MapPtr中的某个地址(索引)
      Value *MapPtrIdx = IRB.CreateGEP(MapPtr, IRB.CreateXor(PrevLocCasted, CurLoc));

      /* Update bitmap */
      // 获取索引值
      LoadInst *Counter = IRB.CreateLoad(MapPtrIdx);
      Counter->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(ctx, None));
      // 索引值 += 1
      Value *Incr = IRB.CreateAdd(Counter, ConstantInt::get(Int8Ty, 1));
      IRB.CreateStore(Incr, MapPtrIdx)
              ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(ctx, None));

      /* Set prev_loc to cur_loc >> 1 */
      // 当前块编号右移一位存储，作为 "下一个基本块" 的 "上一基本块"编号
      StoreInst *Store =
              IRB.CreateStore(ConstantInt::get(Int32Ty, cur_loc >> 1), prevLoc);
      Store->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(ctx, None));

      // 已插桩数 ++
      inst_blocks++;
    }

  if (!inst_blocks) WARNF("No instrumentation targets found.");
  else
    OKF("Instrumented %u locations (%s mode, ratio %u%%).",
        inst_blocks, getenv("HARDEN") ? "hardened" :
                     ((getenv("USE_ASAN") || getenv("USE_MSAN")) ?
                      "ASAN/MSAN" : "non-hardened"), inst_ratio);

  return true;
}

// 向Pass管理器组册Coverage
static void registerPass(const PassManagerBuilder &, legacy::PassManagerBase &PM) {
  PM.add(new Coverage());
}

// 注册为模块级优化器的回调
static RegisterStandardPasses RegisterMyPass(
        PassManagerBuilder::EP_ModuleOptimizerEarly, registerPass);
// 注册为优化级别为 0 时的回调函数
static RegisterStandardPasses RegisterMyPass0(
        PassManagerBuilder::EP_EnabledOnOptLevel0, registerPass);