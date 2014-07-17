#include "ProfileInfoWriter.h"
#include <llvm/Support/raw_ostream.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

using namespace llvm;

ProfileInfoWriter::ProfileInfoWriter(const char* ToolName, 
      const std::string& Filename)
{
   File = fopen(Filename.c_str(), "wb");

   if (File == 0) {
      errs() << ToolName << ": Error opening '" << Filename << "': ";
      perror(0);
      exit(1);
   }
   assert(File);
}

ProfileInfoWriter::~ProfileInfoWriter()
{
   if(File) fclose(File);
}

void ProfileInfoWriter::write(const std::string &cmd)
{
}

void ProfileInfoWriter::write(ProfilingType Type, const std::vector<unsigned int> &Counter)
{
   assert(Type != ValueInfo);
}

