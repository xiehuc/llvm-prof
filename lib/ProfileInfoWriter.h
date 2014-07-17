#ifndef LLVM_ANALYSIS_PROFILE_INFO_WRITER
#define LLVM_ANALYSIS_PROFILE_INFO_WRITER

#include "ProfileInfoLoader.h"
#include "ProfileInfoTypes.h"

namespace llvm {

class ProfileInfoWriter {
   FILE* File;
   public:
   /* open a Filename and prepare for write */
   ProfileInfoWriter(const char* ToolName, const std::string& Filename);
   /* close file and release memory */
   ~ProfileInfoWriter();
   /* write the execution argument type */
   void write(const std::string& cmd);
   /* write the counters
    * @param Type: set type of Counters
    *              !NOTE! Doesn't support ValueInfo Now
    * @param Counter: a Array of unsigned Counter
    */
   void write(ProfilingType Type, const std::vector<unsigned>& Counter);
};

}

#endif
