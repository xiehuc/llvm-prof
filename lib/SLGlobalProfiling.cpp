/* Store Load Instruction for Global Variable Profiling 
 * 
 * This Profiling would insert for all store instructions on global variable
 * bind the instruction id with the global variable address. a map like
 * <address,instr>
 *
 * Then insert for all load instruction on global variable, with the same
 * address, can find which stored instruction before.
 */

#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/InstIterator.h>
#include "ProfilingUtils.h"

using namespace std;
using namespace llvm;

class SLGlobalProfiling: public ModulePass
{
public:
   static char ID;
   SLGlobalProfiling():ModulePass(ID) {}
   bool runOnModule(Module& M);
};

char SLGlobalProfiling::ID = 0;
static RegisterPass<SLGlobalProfiling> X("insert-slg-profiling", "Insert StoreLoadGlobalVariable Profiling into Module", false, true);

/**
 * access whether a Instruction is reference global variable. 
 * if is, return the global variable
 * else return NULL
 */
static Value* access_global_variable(Instruction *I)
{
   Value* U = NULL;
   if(StoreInst* SI = dyn_cast<StoreInst>(I)){
      U = SI->getPointerOperand();
   } else if(LoadInst* LI = dyn_cast<LoadInst>(I)){
      U = LI->getPointerOperand();
   } else return NULL;
	while(ConstantExpr* CE = dyn_cast<ConstantExpr>(U)){
      Instruction*I = CE->getAsInstruction();
		if(isa<CastInst>(I))
         U = I->getOperand(0);
		else break;
	}
	if(GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(U))
      U = GEP->getOperand(GEP->getPointerOperandIndex());
	if(isa<GlobalVariable>(U)) return U;
	return NULL;
}

bool SLGlobalProfiling::runOnModule(Module &M)
{
   LLVMContext& C = M.getContext();
   Type* VoidTy = Type::getVoidTy(C);
   PointerType* VoidPtrTy = PointerType::getUnqual(VoidTy);
   Type* Int32Ty = IntegerType::getInt32Ty(C);
   APInt loadCount = APInt::getNullValue(32); // get a 32 bit zero
   APInt storeCount = APInt::getNullValue(32);
   Constant* StoreCall = M.getOrInsertFunction("llvm_profiling_trap_store", VoidTy, VoidPtrTy, 
         Int32Ty, NULL);
   Constant* LoadCall = M.getOrInsertFunction("llvm_profiling_trap_load", VoidTy, VoidPtrTy, 
         Int32Ty, NULL);
   llvm::Value* callArgs[2];

   for(Module::iterator F = M.begin(), ME = M.end(); F != ME; ++F){
      for(inst_iterator I = inst_begin(F), E = inst_end(F); I!=E; ++I){
         if(access_global_variable(&*I)){
            if(StoreInst* SI = dyn_cast<StoreInst>(&*I)){
               callArgs[0] = CastInst::CreatePointerCast(SI->getPointerOperand(), VoidPtrTy, "", SI);
               callArgs[1] = Constant::getIntegerValue(Int32Ty, storeCount++);
               CallInst::Create(StoreCall, callArgs, "", SI);
            }else if(LoadInst* LI = dyn_cast<LoadInst>(&*I)){
               callArgs[0] = CastInst::CreatePointerCast(LI->getPointerOperand(), VoidPtrTy, "", LI);
               callArgs[1] = Constant::getIntegerValue(Int32Ty, loadCount++);
               CallInst::Create(LoadCall, callArgs, "", LI);
            }
         }
      }
   }

   Type* CounterTy = ArrayType::get(Int32Ty, loadCount.getZExtValue());
   GlobalVariable* Counters = new GlobalVariable(M, CounterTy, false,
         GlobalVariable::InternalLinkage, Constant::getAllOnesValue(CounterTy),
         "SLGCounters");
   InsertProfilingInitCall(M.getFunction("main"), "llvm_start_slg_profiling", Counters);

   return true;
}
