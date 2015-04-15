#include <string.h>
#include <math.h>

#include <map>
#include <string>
#include <functional>

#include "ValueUtils.h"
#include "FreeExpression.h"

#include <llvm/Support/raw_ostream.h>


static std::map<llvm::StringRef, std::function<FreeExpression *()>> FEntries;

void FreeExpression::Register_(const char* Name, std::function<FreeExpression*()>&& f)
{
   FEntries[Name] = f;
}

FreeExpression* FreeExpression::Construct(const std::string& Name)
{
   try{
      return FEntries.at(Name.c_str())();
   }catch(...){
      return NULL;
   }
}

unsigned FreeExpression::init_param(const std::string &para_str)
{
   Param* P_beg = (Param*)((char*)this + sizeof(FreeExpression));// point begin of sub class
   unsigned n = 0, b = 0;
   char Name[32]={0};
   double Val = 0.;
   unsigned succ = 0;
   while (n < para_str.size()
          && sscanf(para_str.c_str() + n, "%31[^=]=%lf%n", Name, &Val, &b)
             == 2) {
      for (Param* P = P_beg; P->Name != NULL; ++P)
         if (strcmp(Name, P->Name) == 0) {
            P->Val = Val;
            ++succ;
            break;
         }
      n += b + 1;
   }
   return succ;
}

double LogisticLog::operator()(double X) const
{
   return L.Val/(1+k.Val/pow(X, u.Val));
}

void LogisticLog::print(llvm::raw_ostream& OS) const
{
   OS<<"L/(1+k/x^u) with L="<<L.Val<<"\tk="<<k.Val<<"\tu="<<u.Val;
}

void Linear::print(llvm::raw_ostream& OS) const
{
   OS<<"k*x+b with k="<<k.Val<<"\tb="<<b.Val;
}

const char* Linear::Name = FreeExpression::Register<Linear>("linear");
const char* LogisticLog::Name = FreeExpression::Register<LogisticLog>("logistic-log");
