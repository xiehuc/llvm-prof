#ifndef LLVM_PROF_PASSES_H_H
#define LLVM_PROF_PASSES_H_H
#include <llvm/Pass.h>
#include <TimingSource.h>
#include <ProfileInfoWriter.h>
#include <set>
namespace llvm{
   enum TimingMode {
      TIMING_NONE,
      TIMING_LMBENCH
   };
   /// ProfileInfoPrinterPass - Helper pass to dump the profile information for
   /// a module.
   //
   class ProfileInfoPrinterPass : public ModulePass {
      ProfileInfoLoader &PIL;
      public:
      static char ID; // Class identification, replacement for typeinfo.
      explicit ProfileInfoPrinterPass(ProfileInfoLoader &_PIL) 
         : ModulePass(ID), PIL(_PIL) {}

      void getAnalysisUsage(AnalysisUsage &AU) const;

      bool runOnModule(Module &M);
      void printExecutionCommands();
      void printFunctionCounts(std::vector<std::pair<Function*, double> >&
            FunctionCounts);
      std::set<Function*> 
         printBasicBlockCounts( std::vector<std::pair<BasicBlock*, double> >&
               Counts);
      void printAnnotatedCode(std::set<Function*>& FunctionToPrint,Module& M);
      void printValueCounts();
      void printValueContent();
      void printSLGCounts();
      virtual const char* getPassName() const {
         return "Print Profile Info";
      }
   };
   class ProfileInfoConverter: public ModulePass
   {
      ProfileInfoWriter& Writer;
      public:
      static char ID;
      ProfileInfoConverter(ProfileInfoWriter& PIW):ModulePass(ID), Writer(PIW) {}
      void getAnalysisUsage(AnalysisUsage& AU) const;
      bool runOnModule(Module& M);
   };
   class ProfileInfoCompare
   {
      ProfileInfoLoader& Lhs;
      ProfileInfoLoader& Rhs;
      public:
      explicit ProfileInfoCompare(ProfileInfoLoader& LHS,ProfileInfoLoader& RHS)
         :Lhs(LHS), Rhs(RHS) {}
      bool run();
   };
   class ProfileTimingPrint: public ModulePass
   {
      TimingSource Source;
      public:
      static char ID;
      ProfileTimingPrint(TimingMode T, std::string File);
      void getAnalysisUsage(AnalysisUsage& AU) const override;
      bool runOnModule(Module& M) override;
   };
}
#endif
