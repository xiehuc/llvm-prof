#include "ProfileInfoMerge.h"


using namespace llvm;
template <typename ty>
void MergeVector(std::vector<ty>& Ahstmp, std::vector<ty>& Thstmp){
   for(int i = 0;i < Ahstmp.size(); i++){
      Ahstmp[i]=Ahstmp[i]+Thstmp[i];
   }
}

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
   for(unsigned i = 0;i < AHS.getRawValueCounts().size();i++){
      std::vector<int> tmp = AHS.getRawValueContent(i);
      this->ValueContents.push_back(tmp);
   }
  // errs()<<this->Toolname<<"\n";
  // errs()<<this->Filename<<"\n";
  // errs()<<this->FunctionCounts.size()<<"\n";
  // errs()<<this->OptimalEdgeCounts.size()<<"\n";
  // errs()<<this->SLGCounts.size()<<"\n";
  // errs()<<this->EdgeCounts.size()<<"\n";
  // errs()<<this->BlockCounts.size()<<"\n";
  // errs()<<this->ValueCounts.size()<<"\n";
  // errs()<<this->CommandLines.size()<<"\n";

}
void ProfileInfoMerge::addProfileInfo(llvm::ProfileInfoLoader &THS){
   //errs()<<THS.getFileName()<<"\n";
   //errs()<<THS.getNumExecutions()<<"\n";

   std::vector<unsigned> Thstmp = THS.getRawBlockCounts();
   MergeVector(this->BlockCounts,Thstmp);

   Thstmp = THS.getRawOptimalEdgeCounts();
   MergeVector(this->OptimalEdgeCounts,Thstmp);

   Thstmp = THS.getRawEdgeCounts();
   MergeVector(this->EdgeCounts,Thstmp);

   Thstmp = THS.getRawFunctionCounts();
   MergeVector(this->FunctionCounts,Thstmp);

   Thstmp = THS.getRawValueCounts();
   MergeVector(this->ValueCounts,Thstmp);

   Thstmp = THS.getRawSLGCounts();
   MergeVector(this->SLGCounts,Thstmp);

   for(unsigned i = 0; i < THS.getRawValueCounts().size(); i++){
      std::vector<int> tmp = THS.getRawValueContent(i);
      MergeVector(this->ValueContents[i],tmp);
   }

   return;
}

void ProfileInfoMerge::writeTotleFile(){

   ProfileInfoWriter totleFile((this->Toolname.c_str()),this->Filename);
   //errs()<<"hello world!\n";
   totleFile.write(FunctionInfo, this->FunctionCounts);
   totleFile.write(BlockInfo, this->BlockCounts);
   totleFile.write(EdgeInfo, this->EdgeCounts);
   totleFile.write(OptEdgeInfo, this->OptimalEdgeCounts);
   totleFile.write(SLGInfo, this->SLGCounts);
}
