//===- llvm-prof.cpp - Read in and process llvmprof.out data files --------===//
//
//                      The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This tools is meant for use with the various LLVM profiling instrumentation
// passes.  It reads in the data file produced by executing an instrumented
// program, and outputs a nice report.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/LLVMContext.h"
#include "llvm/Analysis/Passes.h"
#include "ProfileInfo.h"
#include "ProfileInfoLoader.h"
#include "ProfileInfoMerge.h"
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/system_error.h"
#include <llvm/IR/Instructions.h>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>

using namespace llvm;

namespace {
  cl::opt<std::string>
  BitcodeFile(cl::Positional, cl::desc("<program bitcode file>"),
              cl::Required);

  cl::opt<std::string>
  ProfileDataFile(cl::Positional, cl::desc("<llvmprof.out file>"),
                  cl::Optional, cl::init("llvmprof.out"));

  cl::opt<bool>
  PrintAnnotatedLLVM("annotated-llvm",
                     cl::desc("Print LLVM code with frequency annotations"));
  cl::alias PrintAnnotated2("A", cl::desc("Alias for --annotated-llvm"),
                            cl::aliasopt(PrintAnnotatedLLVM));
  cl::opt<bool> ListAll("list-all", cl::desc("List all blocks"));
  cl::opt<bool> Unsort("unsort",cl::desc("Directly print without sort order"));
  cl::opt<bool> DiffMode("diff",cl::desc("Compare two out file"));
  cl::opt<bool> ValueContentPrint("value-content", 
        cl::desc("Print detailed value content in value profiling"));
  cl::opt<bool> PrintAllCode("print-all-code", 
        cl::desc("Print annotated code for the entire program"));
  ///////////////////
  cl::opt<bool> Merge("merge",cl::desc("Merge the Profile info"));
  //cl::opt<std::string> TotleFile(cl::Positional,cl::desc("<Totle file>"),cl::Optional,cl::init("llvmprof_Totle.out"));
  cl::list<std::string> MergeFile(cl::Positional,cl::desc("<Merge file list>"),cl::ZeroOrMore);
  /////////////////////
}

// PairSecondSort - A sorting predicate to sort by the second element of a pair.
template<class T>
struct PairSecondSortReverse
  : public std::binary_function<std::pair<T, double>,
                                std::pair<T, double>, bool> {
  bool operator()(const std::pair<T, double> &LHS,
                  const std::pair<T, double> &RHS) const {
    return LHS.second > RHS.second;
  }
};

static double ignoreMissing(double w) {
  if (w == ProfileInfo::MissingValue) return 0;
  return w;
}

namespace {
  class ProfileAnnotator : public AssemblyAnnotationWriter {
    ProfileInfo &PI;
  public:
    ProfileAnnotator(ProfileInfo &pi) : PI(pi) {}

    virtual void emitFunctionAnnot(const Function *F,
                                   formatted_raw_ostream &OS) {
      double w = PI.getExecutionCount(F);
      if (w != ProfileInfo::MissingValue) {
        OS << ";;; %" << F->getName() << " called "<<(unsigned)w
           <<" times.\n;;;\n";
      }
    }
    virtual void emitBasicBlockStartAnnot(const BasicBlock *BB,
                                          formatted_raw_ostream &OS) {
      double w = PI.getExecutionCount(BB);
      if (w != ProfileInfo::MissingValue) {
        if (w != 0) {
          OS << "\t;;; Basic block executed " << (unsigned)w << " times.\n";
        } else {
          OS << "\t;;; Never executed!\n";
        }
      }
    }

    virtual void emitBasicBlockEndAnnot(const BasicBlock *BB,
                                        formatted_raw_ostream &OS) {
      // Figure out how many times each successor executed.
      std::vector<std::pair<ProfileInfo::Edge, double> > SuccCounts;

      const TerminatorInst *TI = BB->getTerminator();
      for (unsigned s = 0, e = TI->getNumSuccessors(); s != e; ++s) {
        BasicBlock* Succ = TI->getSuccessor(s);
        double w = ignoreMissing(PI.getEdgeWeight(std::make_pair(BB, Succ)));
        if (w != 0)
          SuccCounts.push_back(std::make_pair(std::make_pair(BB, Succ), w));
      }
      if (!SuccCounts.empty()) {
        OS << "\t;;; Out-edge counts:";
        for (unsigned i = 0, e = SuccCounts.size(); i != e; ++i)
          OS << " [" << (SuccCounts[i]).second << " -> "
             << (SuccCounts[i]).first.second->getName() << "]";
        OS << "\n";
      }
    }
  };
}

namespace {
  /// ProfileInfoPrinterPass - Helper pass to dump the profile information for
  /// a module.
  //
  // FIXME: This should move elsewhere.
  class ProfileInfoPrinterPass : public ModulePass {
    ProfileInfoLoader &PIL;
  public:
    static char ID; // Class identification, replacement for typeinfo.
    explicit ProfileInfoPrinterPass(ProfileInfoLoader &_PIL) 
      : ModulePass(ID), PIL(_PIL) {}

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
      AU.addRequired<ProfileInfo>();
    }

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

  class ProfileInfoCompare
  {
     ProfileInfoLoader& Lhs;
     ProfileInfoLoader& Rhs;
     public:
     explicit ProfileInfoCompare(ProfileInfoLoader& LHS,ProfileInfoLoader& RHS)
        :Lhs(LHS), Rhs(RHS) {}
     bool run();
  };

 
}

char ProfileInfoPrinterPass::ID = 0;

void ProfileInfoPrinterPass::printValueContent() 
{
	ProfileInfo &PI = getAnalysis<ProfileInfo>();
	std::vector<const Instruction*> Calls = PI.getAllTrapedValues();
	for(std::vector<const Instruction*>::const_iterator I = Calls.begin(), E = Calls.end(); I!=E; ++I){
      const CallInst* CI = dyn_cast<CallInst>(*I);
		std::vector<int> Contents = PI.getValueContents(CI);
		const Value* traped = PI.getTrapedTarget(CI);
		if(isa<Constant>(traped))outs()<<"Constant";
		else outs()<<"Variable";
		outs()<<"("<<(unsigned)PI.getExecutionCount(CI)<<"):\t";
		std::vector<int>::iterator II = Contents.begin(),EE=Contents.end(),BND;
		while(II!=EE){
			BND = std::upper_bound(II, EE, *II);
			size_t len = BND-II;
			if (len>5)
				outs()<<*II<<"<repeat "<<len<<" times>,";
			else
				for(unsigned i=0;i<len;i++)
					outs()<<*II<<",";
			II=BND;
		}
		outs()<<"\n";
	}
}

void ProfileInfoPrinterPass::printExecutionCommands()
{
	// Output a report. Eventually, there will be multiple reports selectable on
	// the command line, for now, just keep things simple.
	outs() << "===" << std::string(73, '-') << "===\n"
		<< "LLVM profiling output for execution";
	if (PIL.getNumExecutions() != 1) outs() << "s";
	outs() << ":\n";

	for (unsigned i = 0, e = PIL.getNumExecutions(); i != e; ++i) {
		outs() << "  ";
		if (e != 1) outs() << i+1 << ". ";
		outs() << PIL.getExecution(i) << "\n";
	}
}

void ProfileInfoPrinterPass::printFunctionCounts( 
		std::vector<std::pair<Function*, double> >& FunctionCounts)
{
	// Sort by the frequency, backwards.
	if(!Unsort)
		sort(FunctionCounts.begin(), FunctionCounts.end(),
				PairSecondSortReverse<Function*>());

	double TotalExecutions = 0;
	for (unsigned i = 0, e = FunctionCounts.size(); i != e; ++i)
		TotalExecutions += FunctionCounts[i].second;

	if(TotalExecutions == 0) return;

	outs() << "\n===" << std::string(73, '-') << "===\n";
	outs() << "Function execution frequencies:\n\n";

	// Print out the function frequencies...
	outs() << " ##   Frequency\n";
	for (unsigned i = 0, e = FunctionCounts.size(); i != e; ++i) {
		if (!Unsort && FunctionCounts[i].second == 0) {
			outs() << "\n  NOTE: " << e-i << " function" 
				<< (e-i-1 ? "s were" : " was") << " never executed!\n";
			break;
		}

		outs() << format("%3d", i+1) << ". "
			<< format("%5.2g", FunctionCounts[i].second) << "/"
			<< format("%g", TotalExecutions) << " "
			<< FunctionCounts[i].first->getName() << "\n";
	}
}

std::set<Function*> 
ProfileInfoPrinterPass::printBasicBlockCounts(
		std::vector<std::pair<BasicBlock *, double> > &Counts)
{
	std::set<Function*> FunctionsToPrint;

	double TotalExecutions = 0;
	for (unsigned i = 0, e = Counts.size(); i != e; ++i)
		TotalExecutions += Counts[i].second;

	if(TotalExecutions == 0) return FunctionsToPrint;

	// Sort by the frequency, backwards.
	if(!Unsort)
		sort(Counts.begin(), Counts.end(),
				PairSecondSortReverse<BasicBlock*>());

	outs() << "\n===" << std::string(73, '-') << "===\n";
	if(!ListAll)
		outs() << "Top 20 most frequently executed basic blocks:\n\n";
	else
		outs() << "Sorted executed basic blocks:\n\n";

	// Print out the function frequencies...
	outs() <<" ##      %% \tFrequency\n";
	unsigned BlocksToPrint = Counts.size();
	if (!ListAll && BlocksToPrint > 20) BlocksToPrint = 20;
	for (unsigned i = 0; i != BlocksToPrint; ++i) {
		if (!Unsort && Counts[i].second == 0) break;
		Function *F = Counts[i].first->getParent();
		outs() << format("%3d", i+1) << ". "
			<< format("%5g", Counts[i].second/(double)TotalExecutions*100)<<"% "
			<< format("%5.0f", Counts[i].second) << "/"
			<< format("%g", TotalExecutions) << "\t"
			<< F->getName() << "() - "
			<< Counts[i].first->getName() << "\n";
		FunctionsToPrint.insert(F);
	}
	return FunctionsToPrint;
}

void ProfileInfoPrinterPass::printValueCounts()
{
	ProfileInfo& PI = getAnalysis<ProfileInfo>();
	std::vector<const Instruction*> trapes = PI.getAllTrapedValues();
	if(trapes.empty()) return;

	std::vector<std::pair<const CallInst*,double> > ValueCounts;
	double TotalExecutions = 0;
	for(int i=0,e=trapes.size();i!=e;++i){
      const CallInst* CI = dyn_cast<CallInst>(trapes[i]);
      if(!CI) continue;
		ValueCounts.push_back(std::make_pair(CI, PI.getExecutionCount(CI)));
		TotalExecutions += PI.getExecutionCount(CI);
	}

	if(TotalExecutions == 0) return;

	if(!Unsort)
		sort(ValueCounts.begin(),ValueCounts.end(),PairSecondSortReverse<const CallInst*>());

	outs() << "\n===" << std::string(73, '-') << "===\n";
	if(!ListAll)
		outs() << "Top 20 most frequently executed value trapes:\n\n";
	else
		outs() << "Sorted executed value trapes:\n\n";
	outs() <<" ##      Frequency\t\tValue\t\t\tPosition\n";
	unsigned ValuesToPrint = ValueCounts.size();
	if (!ListAll && ValuesToPrint > 20) ValuesToPrint = 20;
	for (unsigned i = 0; i != ValuesToPrint; ++i) {
		if (!Unsort && ValueCounts[i].second == 0) break;
		const BasicBlock* BB = ValueCounts[i].first->getParent();
		const Function* F = BB->getParent();
		unsigned idx = PI.getTrapedIndex(ValueCounts[i].first);
		outs() << format("%3d", idx) << ". "
			<< format("%5.0f", ValueCounts[i].second)  <<"\t"
			<< *PI.getTrapedTarget(ValueCounts[i].first) <<"\t"
			<< F->getName()<<":\""
			<< BB->getName() <<"\"\t"
			<< "\n";
	}
}

void ProfileInfoPrinterPass::printSLGCounts()
{
	ProfileInfo& PI = getAnalysis<ProfileInfo>();
	std::vector<const Instruction*> trapes = PI.getAllTrapedValues();
   bool first = true;

   for(unsigned i=0;i<trapes.size();i++){
      const LoadInst* LI = dyn_cast<LoadInst>(trapes[i]);
      if(!LI) continue;

      if(first){
         outs() << "\n===" << std::string(73, '-') << "===\n";
         outs() << "store load global profiling information:\n\n";
         outs() <<" ##      Load\t\tStore\t\t\tStore Position\n";
         first = false;
      }

      unsigned idx = PI.getTrapedIndex(LI);
      const Value* T = PI.getTrapedTarget(LI);
		outs() << format("%3d", idx) << ". "
			<< *LI  <<"\n  -->";
      if(T){
         const StoreInst* SI = dyn_cast<StoreInst>(T);
         assert(SI);
         const BasicBlock* BB = SI->getParent();
         const Function* F = BB->getParent();
         outs() << *SI <<"\t\t ## "
            << F->getName()<<":\""
            << BB->getName() <<"\"\t"
            << "\n";
      }else{
         outs() << "  unknow\n";
      }
   }
}

void ProfileInfoPrinterPass::printAnnotatedCode(
		std::set<Function *> &FunctionsToPrint,
		Module& M)
{
	ProfileInfo &PI = getAnalysis<ProfileInfo>();
	if (PrintAnnotatedLLVM || PrintAllCode) {
		outs() << "\n===" << std::string(73, '-') << "===\n";
		outs() << "Annotated LLVM code for the module:\n\n";

		ProfileAnnotator PA(PI);

		if (FunctionsToPrint.empty() || PrintAllCode)
			M.print(outs(), &PA);
		else
			// Print just a subset of the functions.
			for (std::set<Function*>::iterator I = FunctionsToPrint.begin(),
					E = FunctionsToPrint.end(); I != E; ++I)
				(*I)->print(outs(), &PA);
	}
}

bool ProfileInfoPrinterPass::runOnModule(Module &M) {
	ProfileInfo &PI = getAnalysis<ProfileInfo>();
	std::set<Function*> FunctionToPrint;

	if(ValueContentPrint){
		printValueContent();
		return false;
	}

	std::vector<std::pair<Function*, double> > FunctionCounts;
	std::vector<std::pair<BasicBlock*, double> > Counts;
	for (Module::iterator FI = M.begin(), FE = M.end(); FI != FE; ++FI) {
		if (FI->isDeclaration()) continue;
		double w = ignoreMissing(PI.getExecutionCount(FI));
		FunctionCounts.push_back(std::make_pair(FI, w));
		for (Function::iterator BB = FI->begin(), BBE = FI->end(); 
				BB != BBE; ++BB) {
			double w = ignoreMissing(PI.getExecutionCount(BB));
			Counts.push_back(std::make_pair(BB, w));
		}
	}

	printExecutionCommands();
	// Emit the most frequent function table...
	printFunctionCounts(FunctionCounts);
	FunctionToPrint = printBasicBlockCounts(Counts);
	printValueCounts();
   printSLGCounts();
	printAnnotatedCode(FunctionToPrint,M);

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


int main(int argc, char **argv) {
  // Print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);

  LLVMContext &Context = getGlobalContext();
  llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.
  
  cl::ParseCommandLineOptions(argc, argv, "llvm profile dump decoder\n");

  // Read in the bitcode file...
  std::string ErrorMessage;
  OwningPtr<MemoryBuffer> Buffer;
  error_code ec;
  Module *M = 0;
  if(DiffMode) {
     ProfileInfoLoader PIL1(argv[0], BitcodeFile);
     ProfileInfoLoader PIL2(argv[0], ProfileDataFile);
     
     ProfileInfoCompare Compare(PIL1,PIL2);
     Compare.run();
     return 0;
  }
  if(Merge) {

     // Add the ProfileDataFile arg to MergeFile, it blongs to the MergeFile
     MergeFile.push_back(std::string(ProfileDataFile.getValue()));
     if(MergeFile.size()==0){
        errs()<<"No merge file!";
        return 0;
     }

     //Initialize the ProfileInfoMerge class using one of merge files
     ProfileInfoLoader AHS(argv[0], *(MergeFile.end()-1));
     ProfileInfoMerge MergeClass(std::string(argv[0]),BitcodeFile,AHS);

     for(std::vector<std::string>::iterator merIt = MergeFile.begin(),END = MergeFile.end()-1;merIt!=END;++merIt){
        //errs()<<*merIt<<"\n";
        ProfileInfoLoader THS(argv[0], *merIt);
        MergeClass.addProfileInfo(THS);
     }
     MergeClass.writeTotalFile();

  }
  if (!(ec = MemoryBuffer::getFileOrSTDIN(BitcodeFile, Buffer))) {
     M = ParseBitcodeFile(Buffer.get(), Context, &ErrorMessage);
  } else
     ErrorMessage = ec.message();
  if (M == 0) {
     errs() << argv[0] << ": " << BitcodeFile << ": "
        << ErrorMessage << "\n";
     return 1;
  }

  // Read the profiling information. This is redundant since we load it again
  // using the standard profile info provider pass, but for now this gives us
  // access to additional information not exposed via the ProfileInfo
  // interface.
  ProfileInfoLoader PIL(argv[0], ProfileDataFile);

  // Run the printer pass.
  PassManager PassMgr;
  PassMgr.add(createProfileLoaderPass(ProfileDataFile));
  PassMgr.add(new ProfileInfoPrinterPass(PIL));
  PassMgr.run(*M);

  return 0;
}
