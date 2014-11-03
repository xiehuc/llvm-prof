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
   explicit ProfileInfoMerge(std::string toolName,std::string fileName,ProfileInfoLoader& AHS) {
      this->Toolname = toolName;
      this->Filename = fileName;
      this->FunctionCounts = AHS.getRawFunctionCounts();
      this->OptimalEdgeCounts = AHS.getRawOptimalEdgeCounts();
      this->SLGCounts = AHS.getRawSLGCounts();
      this->EdgeCounts = AHS.getRawEdgeCounts();
      this->BlockCounts = AHS.getRawBlockCounts();
      this->ValueCounts = AHS.getRawValueCounts();
      for(unsigned i = 0;i < AHS.getNumExecutions();i++){
         std::string tmp = AHS.getExecution(i);
         this->CommandLines.push_back(tmp);
      }
   }
   /*Merge the file*/
   template<class F>
   void  addProfileInfo(ProfileInfoLoader& THS, F acc) {
#define MERGEVECTOR(what) assert(this->what.size() == THS.getRaw##what().size());\
      std::transform(this->what.begin(),this->what.end(),THS.getRaw##what().begin(),this->what.begin(), acc);
      MERGEVECTOR(BlockCounts);
      MERGEVECTOR(OptimalEdgeCounts);
      MERGEVECTOR(EdgeCounts);
      MERGEVECTOR(FunctionCounts);
      MERGEVECTOR(ValueCounts);
      MERGEVECTOR(SLGCounts);
#undef MERGEVECTOR
   }
   void addProfileInfo(ProfileInfoLoader& THS) {
      addProfileInfo(THS, std::plus<unsigned>());
   }
   /*write totle merging file*/
   template<class F>
   void writeTotalFile(F distribution) {
      ProfileInfoWriter totalFile((this->Toolname.c_str()),this->Filename);
      std::vector<unsigned> counts;
#define WRITEVECTOR(info, what) \
      counts.resize(what.size()); \
      std::transform(what.begin(), what.end(), counts.begin(), distribution); \
      totalFile.write(info, counts);
      WRITEVECTOR(FunctionInfo, FunctionCounts);
      WRITEVECTOR(BlockInfo, BlockCounts);
      WRITEVECTOR(EdgeInfo, EdgeCounts);
      WRITEVECTOR(OptEdgeInfo, OptimalEdgeCounts);
      WRITEVECTOR(SLGInfo, SLGCounts);
      for(unsigned i = 0;i < this->CommandLines.size();i++){
         totalFile.write(this->CommandLines[i]);
      }
#undef WRITEVECTOR
   }
   void writeTotalFile() {
      /** x + 0 --> x ; it doesn't make any distribution **/
      writeTotalFile(std::bind2nd(std::plus<unsigned>(), 0));
   }
};

}

#endif
