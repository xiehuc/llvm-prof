#include "preheader.h"
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <unordered_map>

#include "ValueUtils.h"
#include "ProfilingUtils.h"
#include "ProfileInstrumentations.h"
#include "ProfileDataTypes.h"

namespace {
   class MPIProfiler : public llvm::ModulePass
   {
      public:
      static char ID;
      MPIProfiler():ModulePass(ID) {};
      bool runOnModule(llvm::Module&) override;
   };
}

using namespace llvm;
using namespace lle;
char MPIProfiler::ID = 0;
static RegisterPass<MPIProfiler> X("insert-mpi-profiling", 
      "insert profiling for mpi communication instruction", false, false);

static void IncrementMPICounter(Value* Inc, unsigned Index, GlobalVariable* Counters, IRBuilder<>& Builder)
{
   LLVMContext &Context = Inc->getContext();

   // Create the getelementptr constant expression
   std::vector<Constant*> Indices(2);
   Indices[0] = Constant::getNullValue(Type::getInt32Ty(Context));
   Indices[1] = ConstantInt::get(Type::getInt32Ty(Context), Index);
   Constant *ElementPtr =
      ConstantExpr::getGetElementPtr(Counters, Indices);

   // Load, increment and store the value back.
   Value* OldVal = Builder.CreateLoad(ElementPtr, "OldMPICounter");
   Value* NewVal = Builder.CreateAdd(OldVal, Inc, "NewMPICounter");
   Builder.CreateStore(NewVal, ElementPtr);
}

bool MPIProfiler::runOnModule(llvm::Module &M)
{
  Function *Main = M.getFunction("main");
  if (Main == 0) {
    errs() << "WARNING: cannot insert edge profiling into a module"
           << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  std::vector<std::pair<CallInst*,unsigned> > Traped;
  for(auto F = M.begin(), E = M.end(); F!=E; ++F){
     for(auto I = inst_begin(*F), IE = inst_end(*F); I!=IE; ++I){
        CallInst* CI = dyn_cast<CallInst>(&*I);
        if(CI == NULL) continue;
        if(unsigned idx = get_mpi_count_idx(CI)) // idx > 0
           Traped.emplace_back(CI, idx);
     }
  }

  IRBuilder<> Builder(M.getContext());
  Type* I32Ty = Type::getInt32Ty(M.getContext());
  // last 128 is datatype map array
  Type*ATy = ArrayType::get(I32Ty, Traped.size() + FORTRAN_DATATYPE_MAP_SIZE);
  GlobalVariable* Counters = new GlobalVariable(M, ATy, false,
        GlobalVariable::InternalLinkage, Constant::getNullValue(ATy),
        "MPICounters");
  Value* Zero = ConstantInt::get(I32Ty, 0);
  Value* Length = ConstantInt::get(I32Ty, Traped.size());

  unsigned I=0;
  for(auto P : Traped){
     Builder.SetInsertPoint(P.first);
     Value* FortranDT = Builder.CreateLoad(P.first->getArgOperand(P.second+1));
     Value* Idx[] = {Zero, Builder.CreateAdd(Length, FortranDT)};// get offset of global array
     Value* DataSize = Builder.CreateLoad(Builder.CreateGEP(Counters, Idx));
     Value* Count = Builder.CreateLoad(P.first->getArgOperand(P.second));
     //trap for count * datasize
     IncrementMPICounter(Builder.CreateMul(Count, DataSize), I++, Counters, Builder);
  }

  InsertProfilingInitCall(Main, "llvm_start_mpi_profiling", Counters);
  return true;
}

