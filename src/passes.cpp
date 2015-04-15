#include "passes.h"
#include <ProfileInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <fstream>
#include <iterator>
#include <float.h>

using namespace llvm;

namespace {
#ifndef NDEBUG
   cl::opt<bool> TimingDebug("timing-debug", cl::desc("print more detail for timing mode"));
#endif
   cl::opt<std::string> TimingIgnore("timing-ignore",
                                     cl::desc("ignore list for timing mode"),
                                     cl::init(""));
};

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


char ProfileTimingPrint::ID = 0;
void ProfileTimingPrint::getAnalysisUsage(AnalysisUsage &AU) const
{
   AU.setPreservesAll();
   AU.addRequired<ProfileInfo>();
}

bool ProfileTimingPrint::runOnModule(Module &M)
{
   ProfileInfo& PI = getAnalysis<ProfileInfo>();
   double AbsoluteTiming = 0.0, BlockTiming = 0.0, MpiTiming = 0.0;
   for(auto S : Sources){
      if(BlockTiming < DBL_EPSILON){ // BlockTiming is Zero
         for(Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F){
            if(Ignore.count(F->getName())) continue;
#ifndef NDEBUG
            double FuncTiming = 0.;
            size_t MaxTimes = 0;
            double MaxCount = 0.;
            double MaxProd = 0.;
            StringRef MaxName;
            for(Function::iterator BB = F->begin(), BBE = F->end(); BB != BBE; ++BB){
               size_t exec_times = PI.getExecutionCount(BB);
               double exec_count = S->count(*BB);
               double timing = exec_times * exec_count;
               if(timing > MaxProd){
                  MaxProd = timing;
                  MaxCount = exec_count;
                  MaxTimes = exec_times;
                  MaxName = BB->getName();
               }
               FuncTiming += timing; // 基本块频率×基本块时间
            }
            if (TimingDebug)
              outs() << FuncTiming << "\t"
                     << "max=" << MaxTimes << "*" << MaxCount << "\t" << MaxName
                     << "\t" << F->getName() << "\n";
            BlockTiming += FuncTiming;
#else
            for(Function::iterator BB = F->begin(), BBE = F->end(); BB != BBE; ++BB){
               BlockTiming += PI.getExecutionCount(BB) * S->count(*BB);
            }
#endif
         }
      }
      if(MpiTiming < DBL_EPSILON){ // MpiTiming is Zero
         for(auto I : PI.getAllTrapedValues(MPInfo)){
            const CallInst* CI = cast<CallInst>(I);
            const BasicBlock* BB = CI->getParent();
            double timing = S->count(*I, PI.getExecutionCount(BB), PI.getExecutionCount(CI)); // IO 模型
            MpiTiming += timing;
         }
      }
   }
   AbsoluteTiming = BlockTiming + MpiTiming;
   outs()<<"Block Timing: "<<BlockTiming<<" ns\n";
   outs()<<"MPI Timing: "<<MpiTiming<<" ns\n";
   outs()<<"Timing: "<<AbsoluteTiming<<" ns\n";
   return false;
}

ProfileTimingPrint::ProfileTimingPrint(std::vector<TimingSource*>&& TS,
      std::vector<std::string>& Files):ModulePass(ID), Sources(TS)
{
   if(Sources.size() > Files.size()){
      errs()<<"No Enough File to initialize Timing Source\n";
      exit(-1);
   }
   if(TimingIgnore!=""){
      std::ifstream IgnoreFile(TimingIgnore);
      if(!IgnoreFile.is_open()){
         errs()<<"Couldn't open ignore file: "<<TimingIgnore<<"\n";
         exit(-1);
      }
      std::copy(std::istream_iterator<std::string>(IgnoreFile),
                std::istream_iterator<std::string>(),
                std::inserter(Ignore, Ignore.end()));
      IgnoreFile.close();
   }
   for(unsigned i = 0; i < Sources.size(); ++i){
      if(auto MPB = dyn_cast<MPBenchTiming>(Sources[i]))
         MPB->init_with_file(Files[i].c_str());
      else
         Sources[i]->init_with_file(Files[i].c_str());
#ifndef NDEBUG
      if(TimingDebug){
         outs()<<"parsed "<<Files[i]<<" file's content:\n";
         Sources[i]->print(outs());
      }
#endif
   }
}

ProfileTimingPrint::~ProfileTimingPrint()
{
   for(auto S : Sources)
      delete S;
}
