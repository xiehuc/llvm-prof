#ifndef LLVM_ANALYSIS_PROFILE_INFO_MERGE
#define LLVM_ANALYSIS_PROFILE_INFO_MERGE

#include "ProfileInfoLoader.h"
#include "ProfileInfoTypes.h"
#include "ProfileInfoWriter.h"
#include <llvm/Support/raw_ostream.h>

namespace llvm {


class ProfileInfoMerge
{
   std::string Filename;
   std::string Toolname;
   std::vector<std::string> CommandLines;
   std::vector<unsigned> FunctionCounts;
   std::vector<unsigned> BlockCounts;
   std::vector<unsigned> EdgeCounts;
   std::vector<unsigned> OptimalEdgeCounts;
   std::vector<unsigned> BBTrace;
   std::vector<unsigned> ValueCounts;
   std::vector<unsigned> SLGCounts;
   std::vector<std::vector<int> > ValueContents; 
   public:
   /*Create a initial total data*/
   explicit ProfileInfoMerge(std::string toolName,std::string fileName,ProfileInfoLoader& AHS);
   /*Merge the file*/
   void  addProfileInfo(ProfileInfoLoader& THS); 
   /*write totle merging file*/
   void writeTotalFile();
};

}

#endif
