//===- ProfileInfoLoad.cpp - Load profile information from disk -----------===//
//
//                      The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The ProfileInfoLoader class is used to load and represent profiling
// information read in from the dump file.
//
//===----------------------------------------------------------------------===//

#include "preheader.h"
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include "ProfileInfoLoader.h"
#include "ProfileInfoTypes.h"
#include <cstdio>
#include <cstdlib>
#include <assert.h>
#include <vector>
using namespace llvm;

raw_ostream &llvm::operator<<(raw_ostream &O, std::pair<const BasicBlock *,
                                                        const BasicBlock *> E) {
  O << "(";

  if (E.first)
    O << E.first->getName();
  else
    O << "0";

  O << ",";

  if (E.second)
    O << E.second->getName();
  else
    O << "0";

  return O << ")";
}

// ByteSwap - Byteswap 'Var' if 'Really' is true.
//
static inline unsigned ByteSwap(unsigned Var, bool Really) {
  if (!Really) return Var;
  return ((Var & (255U<< 0U)) << 24U) |
         ((Var & (255U<< 8U)) <<  8U) |
         ((Var & (255U<<16U)) >>  8U) |
         ((Var & (255U<<24U)) >> 24U);
}

static inline uint64_t ByteSwap(uint64_t Var, bool Really)
{
   if (!Really) return Var;
   return ((Var & (255UL <<  0U)) << 56U) |
          ((Var & (255UL <<  8U)) << 40U) |
          ((Var & (255UL << 16U)) << 24U) | 
          ((Var & (255UL << 24U)) <<  8U) |
          ((Var & (255UL << 32U)) >>  8U) | 
          ((Var & (255UL << 40U)) >> 24U) |
          ((Var & (255UL << 48U)) >> 40U) |
          ((Var & (255UL << 56U)) >> 56U);
}

static uint64_t AddCounts(uint64_t A, uint64_t B) {
  // If either value is undefined, use the other.
  if (A == ProfileInfoLoader::Uncounted) return B;
  if (B == ProfileInfoLoader::Uncounted) return A;
  return A + B;
}

template<class IntT>
static void ReadProfilingBlock(const char *ToolName, FILE *F,
                               bool ShouldByteSwap,
                               std::vector<IntT> &Data) {
  // Read the number of entries...
  IntT NumEntries;
  if (fread(&NumEntries, sizeof(IntT), 1, F) != 1) {
    errs() << ToolName << ": data packet truncated!\n";
    perror(0);
    exit(1);
  }
  NumEntries = ByteSwap(NumEntries, ShouldByteSwap);

  // Read the counts...
  std::vector<IntT> TempSpace(NumEntries);

  // Read in the block of data...
  if (fread(&TempSpace[0], sizeof(IntT)*NumEntries, 1, F) != 1) {
    errs() << ToolName << ": data packet truncated!\n";
    perror(0);
    exit(1);
  }

  // Make sure we have enough space... The space is initialised to -1 to
  // facitiltate the loading of missing values for OptimalEdgeProfiling.
  if (Data.size() < NumEntries)
    Data.resize(NumEntries, ProfileInfoLoader::Uncounted);

  // Accumulate the data we just read into the data.
  if (!ShouldByteSwap) {
    for (IntT i = 0; i != NumEntries; ++i) {
      Data[i] = AddCounts(TempSpace[i], Data[i]);
    }
  } else {
    for (IntT i = 0; i != NumEntries; ++i) {
      Data[i] = AddCounts(ByteSwap(TempSpace[i], true), Data[i]);
    }
  }
}

static void ReadValueProfilingContents(const char* ToolName, FILE* F, 
		bool ShouldByteSwap, const size_t Counts, 
		std::vector<std::vector<int> >& Data)
{
#define EXIT_IF_ERROR {\
    errs() << ToolName << ": data packet truncated!\n";\
    perror(0);\
    exit(1);}

	if(Data.size() < Counts)
		Data.resize(Counts);
   std::vector<int> TempSpace;
   for(unsigned i=0;i<Counts;++i){
		unsigned count = 0;
		if(fread(&count,sizeof(unsigned),1,F) != 1) EXIT_IF_ERROR;
		count = ByteSwap(count, ShouldByteSwap);
		if(count==0) continue;
		TempSpace.clear();
		TempSpace.resize(count);
		if(fread(&TempSpace[0],sizeof(int)*count,1,F) != 1){
			EXIT_IF_ERROR;
		}
		Data[i].resize(TempSpace.size());
      uint64_t (*BS)(uint64_t, bool) = ByteSwap;

		//tranverse TempSpace with ByteSwap and write to Data[i]
		std::transform(TempSpace.begin(), TempSpace.end(), Data[i].begin(),
				std::bind(BS, std::placeholders::_1, ShouldByteSwap));
	}
#undef EXIT_IF_ERROR
}

const uint64_t ProfileInfoLoader::Uncounted = ~0U;

// ProfileInfoLoader ctor - Read the specified profiling data file, exiting the
// program if the file is invalid or broken.
//
ProfileInfoLoader::ProfileInfoLoader(const char *ToolName,
                                     const std::string &Filename)
  : Filename(Filename) {
  FILE *F = fopen(Filename.c_str(), "rb");
  if (F == 0) {
    errs() << ToolName << ": Error opening '" << Filename << "': ";
    perror(0);
    exit(1);
  }
  if (0 == (fseek(F, 0, SEEK_END), ftell(F))) { 
    // end == begin == 0, then it is empty
    errs() << " Warnning '" << Filename << "' seems empty\n";
  }
  fseek(F, 0, SEEK_SET);
  std::vector<unsigned> TempCounters32;

  // Keep reading packets until we run out of them.
  unsigned PacketType;
  while (fread(&PacketType, sizeof(unsigned), 1, F) == 1) {
    // If the low eight bits of the packet are zero, we must be dealing with an
    // endianness mismatch.  Byteswap all words read from the profiling
    // information.
    bool ShouldByteSwap = (char)PacketType == 0;
    PacketType = ByteSwap(PacketType, ShouldByteSwap);

    switch (PacketType) {
    case ArgumentInfo: {
      unsigned ArgLength;
      if (fread(&ArgLength, sizeof(unsigned), 1, F) != 1) {
        errs() << ToolName << ": arguments packet truncated!\n";
        errs() << "  " << Filename << ": ";
        perror(0);
        exit(1);
      }
      ArgLength = ByteSwap(ArgLength, ShouldByteSwap);

      // Read in the arguments...
      std::vector<char> Chars(ArgLength+4);

      if (ArgLength)
        if (fread(&Chars[0], (ArgLength+3) & ~3, 1, F) != 1) {
          errs() << ToolName << ": arguments packet truncated!\n";
          perror(0);
          exit(1);
        }
      CommandLines.push_back(std::string(&Chars[0], &Chars[ArgLength]));
      break;
    }

    case FunctionInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, FunctionCounts);
      break;

    case BlockInfo:
       ReadProfilingBlock(ToolName, F, ShouldByteSwap, TempCounters32);
       if (BlockCounts.size() < TempCounters32.size())
          BlockCounts.resize(TempCounters32.size(), Uncounted);
       std::transform(TempCounters32.begin(), TempCounters32.end(),
                      BlockCounts.begin(), BlockCounts.begin(),
                      std::plus<uint64_t>());
       TempCounters32.clear();
      break;

    case EdgeInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, TempCounters32);
       if (EdgeCounts.size() < TempCounters32.size())
          EdgeCounts.resize(TempCounters32.size(), Uncounted);
       std::transform(TempCounters32.begin(), TempCounters32.end(),
                      EdgeCounts.begin(), EdgeCounts.begin(),
                      std::plus<uint64_t>());
       TempCounters32.clear();
      break;

    case OptEdgeInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, OptimalEdgeCounts);
      break;

    case BBTraceInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, BBTrace);
      break;

	case ValueInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, ValueCounts);
      ReadValueProfilingContents(ToolName, F, ShouldByteSwap, ValueCounts.size(), ValueContents);
      break;

   case SLGInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, SLGCounts);
      break;

   case MPInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, MPICounts);
      break;

   case MPIFullInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, MPIFullCounters);
      break;

   case BlockInfo64:
      ReadProfilingBlock<uint64_t>(ToolName, F, ShouldByteSwap, BlockCounts);
      break;

   case EdgeInfo64:
      ReadProfilingBlock<uint64_t>(ToolName, F, ShouldByteSwap, EdgeCounts);
      break;

   default:
      errs() << ToolName << ": Unknown packet type #" << PacketType << "!\n";
      errs() << "at position "<<ftell(F) <<"/";
      fseek(F,0,SEEK_END);
      errs() << ftell(F) <<"\n";
      fclose(F);
      exit(1);
    }
  }

  fclose(F);
}
