#include "preheader.h"
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <unordered_map>

#include "ValueUtils.h"
#include "ProfilingUtils.h"
#include "InitializeProfilerPass.h"
#include "ProfileInstrumentations.h"

namespace {
   class MPIProfiler : public llvm::ModulePass
   {
      std::unordered_map<std::string, unsigned> Attributes;

      public:
      static char ID;
      MPIProfiler();
      bool runOnModule(llvm::Module&) override;
   };
}

using namespace llvm;
char MPIProfiler::ID = 0;
static RegisterPass<MPIProfiler> X("insert-mpi-profiling", 
      "insert profiling for mpi communication instruction", false, false)

static void IncrementMPICounter(Value* V, GlobalVariable* Counters, IRBuilder<>& Builder)
{
   LLVMContext &Context = Inc->getContext();

   // Create the getelementptr constant expression
   std::vector<Constant*> Indices(2);
   Indices[0] = Constant::getNullValue(Type::getInt32Ty(Context));
   Indices[1] = ConstantInt::get(Type::getInt32Ty(Context), Index);
   Constant *ElementPtr =
      ConstantExpr::getGetElementPtr(Counters, Indices);

   // Load, increment and store the value back.
   Value* OldVal = Builder.CreateLoad(ElementPtr, "OldBlockCounter");
   Value* NewVal = Builder.CreateAdd(OldVal, Inc, "NewBlockCounter");
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

  size_t MPICount = 0;
  std::vector<std::pair<CallInst*,unsigned> > Traped;
  for(auto F = M.begin(), E = M.end(); F!=E; ++F){
     for(auto I = inst_begin(*F), IE = inst_end(*F); I!=IE; ++I){
        CallInst* CI = dyn_cast<CallInst>(&*I);
        if(CI==NULL) continue;
        Function* Called = castoff(CI->getCalledValue());
        auto Found = Attributes.find(Called->getName());
        if(Found == Attributes.end()) continue;
        Traped.emplace_back(CI, Found->second);
     }
  }

  IRBuilder<> Builder(M.getContext());
  Type*ATy = ArrayType::get(Type::getInt32Ty(M.getContext()), Traped.size());
  GlobalVariable* Counters = new GlobalVariable(M, ATy, false,
        GlobalVariable::InternalLinkage, Constant::getNullValue(ATy),
        "MPICounters");

  unsigned I=0;
  for(auto P : Traped){
     Builder.SetInsertPoint(P.first);
     IncrementMPICounter(P.first->getArgOperand(P.second), I++, Counters, Builder);
  }
}

MPIProfiler::MPIProfiler():ModulePass(ID)
{
   Attributes["mpi_bcast_"] = 1;
}
