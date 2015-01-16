#include "preheader.h"
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include "ProfilingUtils.h"
#include "InitializeProfilerPass.h"
#include "ProfileInstrumentations.h"

namespace {
   class MPIProfiler : public llvm::ModulePass
   {
      public:
      static char ID;
      MPIProfiler():ModulePass(ID) {}
      bool runOnModule(llvm::Module&) override;
   };
}

using namespace llvm;
char MPIProfiler::ID = 0;
static RegisterPass<MPIProfiler> X("insert-mpi-profiling", 
      "insert profiling for mpi communication instruction", false, false)

bool MPIProfiler::runOnModule(llvm::Module &M)
{
  Function *Main = M.getFunction("main");
  if (Main == 0) {
    errs() << "WARNING: cannot insert edge profiling into a module"
           << " with no main function!\n";
    return false;  // No main, no instrumentation!
  }

  size_t MPICount = 0;
  for(auto F = M.begin(), E = M.end(); F!=E; ++F){
  }
}
