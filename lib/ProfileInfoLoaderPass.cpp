//===- ProfileInfoLoaderPass.cpp - LLVM Pass to load profile info ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a concrete implementation of profiling information that
// loads the information from a profile dump file.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "profile-loader"
#include "llvm/Analysis/Passes.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "ProfileInfo.h"
#include "ProfileInfoLoader.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include "InitializeProfilerPass.h"
#include "ProfileInstrumentations.h"
#include "ProfilingUtils.h"
#include <llvm/Support/InstIterator.h>
#include <llvm/IR/Constants.h>
#include <set>
#include <vector>
#include <numeric>
using namespace llvm;

STATISTIC(NumEdgesRead, "The # of edges read.");

static cl::opt<std::string>
ProfileInfoFilename("profile-info-file", cl::init("llvmprof.out"),
                    cl::value_desc("filename"),
                    cl::desc("Profile file loaded by -profile-loader"));

namespace {
  class LoaderPass : public ModulePass, public ProfileInfo {
    std::string Filename;
    std::set<Edge> SpanningTree;
    std::set<const BasicBlock*> BBisUnvisited;
    unsigned ReadCount;
  public:
    static char ID; // Class identification, replacement for typeinfo
    explicit LoaderPass(const std::string &filename = "")
		: ModulePass(ID), Filename(filename) {
			initializeLoaderPassPass(*PassRegistry::getPassRegistry());
			initializeProfileInfoAnalysisGroup(*PassRegistry::getPassRegistry());
			if (filename.empty()) Filename = ProfileInfoFilename;
    }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
    }

    virtual const char *getPassName() const {
      return "Profiling information loader";
    }

    // recurseBasicBlock() - Calculates the edge weights for as much basic
    // blocks as possbile.
    virtual void recurseBasicBlock(const BasicBlock *BB);
    virtual void readEdgeOrRemember(Edge, Edge&, unsigned &, double &);
    virtual void readEdge(ProfileInfo::Edge, std::vector<unsigned>&);

    /// getAdjustedAnalysisPointer - This method is used when a pass implements
    /// an analysis interface through multiple inheritance.  If needed, it
    /// should override this to adjust the this pointer as needed for the
    /// specified pass info.
    virtual void *getAdjustedAnalysisPointer(AnalysisID PI) {
      if (PI == &ProfileInfo::ID)
        return (ProfileInfo*)this;
      return this;
    }
    
    /// run - Load the profile information from the specified file.
    virtual bool runOnModule(Module &M);
  };
}  // End of anonymous namespace

char LoaderPass::ID = 0;
INITIALIZE_AG_PASS(LoaderPass, ProfileInfo, "profile-loader",
              "Load profile information from llvmprof.out", false, true, false)

//static RegisterPass<LoaderPass> X("profile-loader",
//              "Load profile information from llvmprof.out", false, true);

char &llvm::ProfileLoaderPassID = LoaderPass::ID;

ModulePass *llvm::createProfileLoaderPass() { return new LoaderPass(); }

/// createProfileLoaderPass - This function returns a Pass that loads the
/// profiling information for the module from the specified filename, making it
/// available to the optimizers.
Pass *llvm::createProfileLoaderPass(const std::string &Filename) {
  return new LoaderPass(Filename);
}

void LoaderPass::readEdgeOrRemember(Edge edge, Edge &tocalc, 
                                    unsigned &uncalc, double &count) {
  double w;
  if ((w = getEdgeWeight(edge)) == MissingValue) {
    tocalc = edge;
    uncalc++;
  } else {
    count+=w;
  }
}

// recurseBasicBlock - Visits all neighbours of a block and then tries to
// calculate the missing edge values.
void LoaderPass::recurseBasicBlock(const BasicBlock *BB) {

  // break recursion if already visited
  if (BBisUnvisited.find(BB) == BBisUnvisited.end()) return;
  BBisUnvisited.erase(BB);
  if (!BB) return;

  for (succ_const_iterator bbi = succ_begin(BB), bbe = succ_end(BB);
       bbi != bbe; ++bbi) {
    recurseBasicBlock(*bbi);
  }
  for (const_pred_iterator bbi = pred_begin(BB), bbe = pred_end(BB);
       bbi != bbe; ++bbi) {
    recurseBasicBlock(*bbi);
  }

  Edge tocalc;
  if (CalculateMissingEdge(BB, tocalc)) {
    SpanningTree.erase(tocalc);
  }
}

void LoaderPass::readEdge(ProfileInfo::Edge e,
                          std::vector<unsigned> &ECs) {
  if (ReadCount < ECs.size()) {
    double weight = ECs[ReadCount++];
    if (weight != ProfileInfoLoader::Uncounted) {
      // Here the data realm changes from the unsigned of the file to the
      // double of the ProfileInfo. This conversion is save because we know
      // that everything thats representable in unsinged is also representable
      // in double.
      EdgeInformation[getFunction(e)][e] += (double)weight;

      DEBUG(dbgs() << "--Read Edge Counter for " << e
                   << " (# "<< (ReadCount-1) << "): "
                   << (unsigned)getEdgeWeight(e) << "\n");
    } else {
      // This happens only if reading optimal profiling information, not when
      // reading regular profiling information.
      SpanningTree.insert(e);
    }
  }
}

/** signal max **/
inline unsigned sig_max(unsigned acc, unsigned b)
{
   // if b == -1; always return acc
   if(b == ~0U) return acc;
   else return std::max(acc,b);
}

bool LoaderPass::runOnModule(Module &M) {
  ProfileInfoLoader PIL("profile-loader", Filename);

  EdgeInformation.clear();
  std::vector<unsigned> Counters = PIL.getRawEdgeCounts();
  if (Counters.size() > 0) {
    ReadCount = 0;
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      DEBUG(dbgs() << "Working on " << F->getName() << "\n");
      readEdge(getEdge(0,&F->getEntryBlock()), Counters);
      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
        TerminatorInst *TI = BB->getTerminator();
        for (unsigned s = 0, e = TI->getNumSuccessors(); s != e; ++s) {
          readEdge(getEdge(BB,TI->getSuccessor(s)), Counters);
        }
      }
    }
    if (ReadCount != Counters.size()) {
      errs() << "WARNING: profile information is inconsistent with "
             << "the current program!\n";
    }
    NumEdgesRead = ReadCount;
  }

  Counters = PIL.getRawOptimalEdgeCounts();
  if (Counters.size() > 0) {
    ReadCount = 0;
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      DEBUG(dbgs() << "Working on " << F->getName() << "\n");
      readEdge(getEdge(0,&F->getEntryBlock()), Counters);
      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
        TerminatorInst *TI = BB->getTerminator();
        if (TI->getNumSuccessors() == 0) {
          readEdge(getEdge(BB,0), Counters);
        }
        for (unsigned s = 0, e = TI->getNumSuccessors(); s != e; ++s) {
          readEdge(getEdge(BB,TI->getSuccessor(s)), Counters);
        }
      }
      while (SpanningTree.size() > 0) {

        unsigned size = SpanningTree.size();

        BBisUnvisited.clear();
        for (std::set<Edge>::iterator ei = SpanningTree.begin(),
             ee = SpanningTree.end(); ei != ee; ++ei) {
          BBisUnvisited.insert(ei->first);
          BBisUnvisited.insert(ei->second);
        }
        while (BBisUnvisited.size() > 0) {
          recurseBasicBlock(*BBisUnvisited.begin());
        }

        if (SpanningTree.size() == size) {
          DEBUG(dbgs()<<"{");
          for (std::set<Edge>::iterator ei = SpanningTree.begin(),
               ee = SpanningTree.end(); ei != ee; ++ei) {
            DEBUG(dbgs()<< *ei <<",");
          }
          assert(0 && "No edge calculated!");
        }

      }
    }
    if (ReadCount != Counters.size()) {
      errs() << "WARNING: profile information is inconsistent with "
             << "the current program!\n";
    }
    NumEdgesRead = ReadCount;
  }

  BlockInformation.clear();
  Counters = PIL.getRawBlockCounts();
  if (Counters.size() > 0) {
    ReadCount = 0;
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB)
        if (ReadCount < Counters.size())
          // Here the data realm changes from the unsigned of the file to the
          // double of the ProfileInfo. This conversion is save because we know
          // that everything thats representable in unsinged is also
          // representable in double.
          BlockInformation[F][BB] = (double)Counters[ReadCount++];
    }
    if (ReadCount != Counters.size()) {
      errs() << "WARNING: profile information is inconsistent with "
             << "the current program!\n";
    }
  }

  FunctionInformation.clear();
  Counters = PIL.getRawFunctionCounts();
  if (Counters.size() > 0) {
    ReadCount = 0;
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      if (ReadCount < Counters.size())
        // Here the data realm changes from the unsigned of the file to the
        // double of the ProfileInfo. This conversion is save because we know
        // that everything thats representable in unsinged is also
        // representable in double.
        FunctionInformation[F] = (double)Counters[ReadCount++];
    }
    if (ReadCount != Counters.size()) {
      errs() << "WARNING: profile information is inconsistent with "
             << "the current program!\n";
    }
  }

  ValueInformation.clear();
  Counters = PIL.getRawValueCounts();
  if(Counters.size() > 0) {
	  ReadCount = 0;
	  for(Module::iterator F = M.begin(),E = M.end(); F!= E; ++F) {
		  if (F->isDeclaration()) continue;
		  for(inst_iterator I = inst_begin(F),IE = inst_end(F); I!=IE; ++I){
			  CallInst* Call = dyn_cast<CallInst>(&*I);
           if(!Call) continue;
			  if(Call->getCalledValue()->getName() != "llvm_profiling_trap_value") continue;
			  unsigned index = getTrapedIndex(Call);
			  ValueCounts Ins;
			  Ins.Nums = Counters[index];
			  const std::vector<int>& content = PIL.getRawValueContent(index);
			  Ins.flags = (ProfilingFlags)content.front();
			  Ins.Contents.resize(content.size()-1);
			  copy(content.begin()+1,content.end(),Ins.Contents.begin());
			  //should NOT insert two values into one cell.
			  ValueInformation[Call] = Ins;
		  }
	  }
  }

  SLGInformation.clear();
  Counters = PIL.getRawSLGCounts();
  if(Counters.size() > 0) {
     unsigned MaxStore = std::accumulate(Counters.begin(), Counters.end(), 0, sig_max);
     std::vector<const Instruction*> Cache(MaxStore+1);
     ReadCount = 0; 
     unsigned load_idx = 0, store_idx = 1;
     for(Module::iterator F = M.begin(), E = M.end(); F!=E; ++F){
        for(inst_iterator I = inst_begin(F), IE = inst_end(F); I!=IE; ++I){

           if(access_global_variable(&*I)){
              unsigned index;
              Instruction* SLI = &*I;
              if(isa<StoreInst>(&*I)){
                 index = store_idx++;
              }else if(LoadInst* LI = dyn_cast<LoadInst>(&*I)){
                 SLGInformation[LI] = std::make_pair(load_idx, (Instruction*)NULL);
                 index = Counters[load_idx++];
                 if(index == 0 || index == ~0U/*unsigned -1*/){ 
                    continue;
                 }
              }
              if(index >= Cache.size()) continue;
              if(Cache[index]){
                 const Instruction* SLJ = Cache[index];
                 if(isa<StoreInst>(SLJ) && isa<LoadInst>(SLI)) SLGInformation[SLI].second = SLJ;
                 else if(isa<StoreInst>(SLI) && isa<LoadInst>(SLJ)) SLGInformation[SLJ].second = SLI;
                 else
                    assert(0 && "It shouldn't happen");
              }else
                 Cache[index] = &*I;
           }
        }
     }
  }

  return false;
}
