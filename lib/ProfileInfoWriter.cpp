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
   unsigned ArgLength = cmd.length();
   ProfilingType Type = ArgumentInfo;
   if(ArgLength <= 0) return;
   fwrite(&Type,sizeof(unsigned), 1, this->File);
   fwrite(&ArgLength, sizeof(unsigned), 1, this->File);
   fwrite(&cmd[0], (ArgLength+3) & ~3, 1, this->File);
}

void ProfileInfoWriter::write(ProfilingType Type, const std::vector<unsigned int> &Counter)
{
   assert(Type != ValueInfo);
   //errs()<<"store counter\n";

   unsigned NumEntries = Counter.size();
   if(NumEntries <= 0) return;
   fwrite(&Type, sizeof(unsigned), 1, this->File);
   fwrite(&NumEntries, sizeof(unsigned), 1, this->File);
   fwrite(&Counter[0], sizeof(unsigned)*NumEntries, 1, this->File);
     // errs()<<"store over!\n";
}

