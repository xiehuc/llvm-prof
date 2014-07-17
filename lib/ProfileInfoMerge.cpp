#include "ProfileInfoMerge.h"


using namespace llvm;

void MergeVector(std::vector<unsigned>& Ahstmp, std::vector<unsigned>& Thstmp){
   for(int i = 0;i < Ahstmp.size(); i++){
      Ahstmp[i]=Ahstmp[i]+Thstmp[i];
   }
}

ProfileInfoMerge::ProfileInfoMerge(llvm::ProfileInfoLoader& AHS){
   this->Filename = AHS.getFileName();
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
   errs()<<this->Filename<<"\n";
   errs()<<this->FunctionCounts.size()<<"\n";
   errs()<<this->OptimalEdgeCounts.size()<<"\n";
   errs()<<this->SLGCounts.size()<<"\n";
   errs()<<this->EdgeCounts.size()<<"\n";
   errs()<<this->BlockCounts.size()<<"\n";
   errs()<<this->ValueCounts.size()<<"\n";
   errs()<<this->CommandLines.size()<<"\n";

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


   return;
}

