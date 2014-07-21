#include "ProfileInfoMerge.h"


using namespace llvm;

ProfileInfoMerge::ProfileInfoMerge(std::string toolName,std::string fileName,llvm::ProfileInfoLoader& AHS){
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
void ProfileInfoMerge::addProfileInfo(llvm::ProfileInfoLoader &THS){
   //errs()<<THS.getFileName()<<"\n";
   //errs()<<THS.getNumExecutions()<<"\n";

#define MERGEVECTOR(what) assert(this->what.size() == THS.getRaw##what().size());\
   std::transform(this->what.begin(),this->what.end(),THS.getRaw##what().begin(),this->what.begin(),std::plus<unsigned>());
   MERGEVECTOR(BlockCounts);
   MERGEVECTOR(OptimalEdgeCounts);
   MERGEVECTOR(EdgeCounts);
   MERGEVECTOR(FunctionCounts);
   MERGEVECTOR(ValueCounts);
   MERGEVECTOR(SLGCounts);
   
   return;
}

void ProfileInfoMerge::writeTotalFile(){

   ProfileInfoWriter totalFile((this->Toolname.c_str()),this->Filename);
   //errs()<<"hello world!\n";
   totalFile.write(FunctionInfo, this->FunctionCounts);
   totalFile.write(BlockInfo, this->BlockCounts);
   totalFile.write(EdgeInfo, this->EdgeCounts);
   totalFile.write(OptEdgeInfo, this->OptimalEdgeCounts);
   totalFile.write(SLGInfo, this->SLGCounts);
   for(unsigned i = 0;i < this->CommandLines.size();i++){
      totalFile.write(this->CommandLines[i]);
   }
}
