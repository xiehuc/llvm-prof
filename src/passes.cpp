#include "passes.h"
#include <ProfileInfo.h>
#include <llvm/IR/Module.h>

using namespace llvm;

char ProfileInfoConverter::ID = 0;
void ProfileInfoConverter::getAnalysisUsage(AnalysisUsage &AU) const
{
   AU.addRequired<ProfileInfo>();
   AU.setPreservesAll();
}

bool ProfileInfoConverter::runOnModule(Module &M)
{
   ProfileInfo& PI = getAnalysis<ProfileInfo>();

   std::vector<unsigned> Counters;
   for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB){
         Counters.push_back(PI.getExecutionCount(BB));
      }
   }

   Writer.write(BlockInfo, Counters);

   return false;
}


bool ProfileInfoCompare::run()
{
#define CRITICAL_EQUAL(what) if(Lhs.get##what() != Rhs.get##what()){\
      errs()<<#what" differ\n";\
      return 0;\
   }
#define WARN_EQUAL(what) if(Lhs.getRaw##what() != Rhs.getRaw##what()){\
      errs()<<#what" differ\n";\
   }

   CRITICAL_EQUAL(NumExecutions);
   WARN_EQUAL(BlockCounts);
   WARN_EQUAL(EdgeCounts);
   WARN_EQUAL(FunctionCounts);
   WARN_EQUAL(ValueCounts);
   WARN_EQUAL(SLGCounts);
   if(Lhs.getRawValueCounts().size() == Rhs.getRawValueCounts().size()){
      for(uint i=0;i<Lhs.getRawValueCounts().size();++i){
         if(Lhs.getRawValueContent(i) != Rhs.getRawValueContent(i))
            errs()<<"ValueContent at "<<i<<" differ\n";
      }
   }
   return 0;
#undef CRITICAL_EQUAL
}
