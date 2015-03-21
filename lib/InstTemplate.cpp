#include <llvm/Pass.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <unordered_map>
#define Assert(a,b) // an empty Assert macro , because this code is copied from
                    // other project

namespace lle{
   class InstTemplate: public llvm::ModulePass
   {
      public:
      static char ID;
      InstTemplate():ModulePass(ID) {}
      bool runOnModule(llvm::Module& M) override;
      private:
      typedef llvm::Value* (*TemplateFunc)(llvm::Instruction* InsPoint);
      static std::unordered_map<std::string, TemplateFunc> ImplMap;
      llvm::Value* implyTemplate(llvm::CallInst* Template) const;
   };
};

using namespace llvm;
char lle::InstTemplate::ID = 0;
static RegisterPass<lle::InstTemplate> X("InstTemplate", "InstTemplate");
static std::string FunctyStr = "";

bool lle::InstTemplate::runOnModule(Module &M)
{
   Function* F = M.getFunction("timing");
   Assert(F, "unable find @timing function");
   F->addFnAttr(Attribute::AlwaysInline);

   F = M.getFunction("timing_res");
   Assert(F, "unable find @timing_res function");
   F->addFnAttr(Attribute::AlwaysInline);

   if((F = M.getFunction("inst_template")))
   {
	   for(auto Ite = F->user_begin(), E = F->user_end(); Ite!=E; ++Ite){
		  CallInst* Template = dyn_cast<CallInst>(*Ite);
		  Value* R = implyTemplate(Template);
		  Template->replaceAllUsesWith(R);
		  Template->eraseFromParent();
	   }
   }
   else 
      Assert(F, "can not find @inst_template");  
   return true;
}

Value* lle::InstTemplate::implyTemplate(CallInst *Template) const
{
   Assert(Template,"");
   ConstantExpr* Arg0 = dyn_cast<ConstantExpr>(Template->getArgOperand(0));
   Assert(Arg0,"");
   GlobalVariable* Global = dyn_cast<GlobalVariable>(Arg0->getOperand(0));
   Assert(Global,"");
   ConstantDataArray* Const = dyn_cast<ConstantDataArray>(Global->getInitializer());
   Assert(Const,"");
   StringRef Selector = Const->getAsCString();

   FunctyStr = Selector.str();
   auto Found = ImplMap.find(Selector.str());
   //AssertRuntime(Found != ImplMap.end(), "unknow template keyword: "<<Selector);

   return Found->second(Template);
}

#define REPEAT 2000
static Value* fix_add(Instruction *InsPoint)
{
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());
   Value* One = ConstantInt::get(I32Ty, 1);
   Value* Lhs = One;
   for(int i=0;i<REPEAT;++i){
      Lhs = BinaryOperator::CreateAdd(Lhs, One, "", InsPoint);
   }
   return Lhs;
}

static Value* float_add(Instruction* InsPoint)
{
   Type* FTy = Type::getDoubleTy(InsPoint->getContext());
   Value* One = ConstantFP::get(FTy, 1.003L);
   Value* Lhs = One;
   for(int i=0;i<REPEAT;++i){
      Lhs = BinaryOperator::Create(Instruction::FAdd, Lhs, One, "", InsPoint);
   }
   return CastInst::Create(CastInst::FPToSI, Lhs,
         Type::getInt32Ty(InsPoint->getContext()), "", InsPoint);
}

static Value* fix_sub(Instruction* InsPoint){
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());
   Value* One = ConstantInt::get(I32Ty, 10);
   Value* Lhs = ConstantInt::get(I32Ty, 20*REPEAT);
   for(int i = 0;i < REPEAT; ++i){
      Lhs = BinaryOperator::Create(Instruction::Sub, Lhs, One,"",InsPoint);
   }
   return Lhs;
}
static Value* float_sub(Instruction* InsPoint){
   Type* FTy = Type::getDoubleTy(InsPoint->getContext());
   Value* One = ConstantFP::get(FTy,10.1L);
   Value* Lhs = ConstantFP::get(FTy,20L*REPEAT);
   for(int i = 0;i < REPEAT;++i){
      Lhs = BinaryOperator::Create(Instruction::FSub,Lhs,One,"",InsPoint);
   }
   return CastInst::Create(CastInst::FPToSI,Lhs,Type::getInt32Ty(InsPoint->getContext()),"",InsPoint);
}

static Value* fix_mul(Instruction* InsPoint){

   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());
   Value* One = ConstantInt::get(I32Ty,11);
   Value* Lhs = One;
   for(int i = 0;i < REPEAT;++i){
      Lhs = BinaryOperator::Create(Instruction::Mul,Lhs,One,"",InsPoint);
   }
   return Lhs;
}
static Value* float_mul(Instruction* InsPoint){
   Type* FTy = Type::getDoubleTy(InsPoint->getContext());
   Value* One = ConstantFP::get(FTy,1.03L);
   Value* Lhs = One;
   for(int i = 0;i < REPEAT;++i){
      One = ConstantFP::get(FTy,(double)(i+0.03));
      Lhs = BinaryOperator::Create(Instruction::FMul,Lhs,One,"",InsPoint);
   }
   return CastInst::Create(CastInst::FPToSI,Lhs,Type::getInt32Ty(InsPoint->getContext()),"",InsPoint);
}
static Value* us_div_rem(Instruction* InsPoint){
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());
   Value* One = ConstantInt::get(I32Ty,10);
   Value* Lhs = One;
   for(int i = 0;i < REPEAT;++i){
      One = ConstantInt::get(I32Ty,i+3);
      if(FunctyStr == "u_div"){
         Lhs = BinaryOperator::Create(Instruction::UDiv,Lhs,One,"",InsPoint);
      }
      else if(FunctyStr == "s_div"){
         Lhs = BinaryOperator::Create(Instruction::SDiv,Lhs,One,"",InsPoint);
      }
      else if(FunctyStr == "u_rem"){
         Lhs = BinaryOperator::Create(Instruction::URem,Lhs,One,"",InsPoint);
      }
      else if(FunctyStr == "s_rem"){
         Lhs = BinaryOperator::Create(Instruction::SRem,Lhs,One,"",InsPoint);
      }
   }
   return Lhs;
}
static Value* float_div_rem(Instruction* InsPoint){
   Type* FTy = Type::getDoubleTy(InsPoint->getContext());
   Value* One = ConstantFP::get(FTy,(double)10.03);
   Value* Lhs = One;
   for(int i = 0;i < REPEAT; ++i){
      One = ConstantFP::get(FTy, (double)(i+1.03));
      if(FunctyStr == "float_div")
         Lhs = BinaryOperator::Create(Instruction::FDiv,One,Lhs,"",InsPoint);
      else if(FunctyStr == "float_rem")
         Lhs = BinaryOperator::Create(Instruction::FRem,One,Lhs,"",InsPoint);

   }
   return CastInst::Create(CastInst::FPToSI,Lhs,Type::getInt32Ty(InsPoint->getContext()),"",InsPoint);
}
static Value* binary_op(Instruction* InsPoint){
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());
   Value* One = ConstantInt::get(I32Ty,3);
   Value* Lhs = One;
   for(int i = 0;i < REPEAT; ++i){
      if(FunctyStr == "shl"){
         Lhs = BinaryOperator::Create(Instruction::Shl,Lhs,One,"",InsPoint);
      }
      else if(FunctyStr == "lshr"){
         Lhs = BinaryOperator::Create(Instruction::LShr,Lhs,One,"",InsPoint);
      }
      else if(FunctyStr == "ashr"){
         Lhs = BinaryOperator::Create(Instruction::AShr,Lhs,One,"",InsPoint);
      }
      else if(FunctyStr == "and"){
         Lhs = BinaryOperator::Create(Instruction::And,Lhs,One,"",InsPoint);
      }
      else if(FunctyStr == "or"){
         Lhs = BinaryOperator::Create(Instruction::Or,Lhs,One,"",InsPoint);
      }
      else if(FunctyStr == "xor"){
         Lhs = BinaryOperator::Create(Instruction::Xor,Lhs,One,"",InsPoint);
      }
   }
   return Lhs;
}

static Value* alloca_op(Instruction* InsPoint){
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());
   AllocaInst* a;
   //Add the StoreInst to the End of BasicBlock,in order to prevent the optimization of target instructions. 
   StoreInst* s;
   BasicBlock* BB = InsPoint->getParent();
   for(int i = 0;i < REPEAT;++i){
      a = new AllocaInst(I32Ty,"",InsPoint);
      s = new StoreInst(ConstantInt::get(I32Ty,i),a,"",BB->getTerminator());
      s->setVolatile(true);
   }
   return CastInst::Create(CastInst::PtrToInt,a,I32Ty,"",InsPoint);
}
static Value* getelementptr_op(Instruction* InsPoint){
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());

   CallInst* Template = dyn_cast<CallInst>(InsPoint);
   Value* var = Template->getArgOperand(1);
   Value* stovar = Template->getArgOperand(2);

   Value* Lhs = ConstantInt::get(I32Ty,1);
   Value* Zero = ConstantInt::get(I32Ty,0);
   Value* Cast = Lhs;

   BasicBlock* BB = InsPoint->getParent();
   //Add the StoreInst to the End of BasicBlock,in order to prevent the optimization of target instructions. 
   StoreInst* s;
   for(int i = 0;i < REPEAT;++i){
      Value* Arg[] = { Zero, ConstantInt::get(I32Ty, i), ConstantInt::get(I32Ty, 1) };
      Lhs = GetElementPtrInst::Create(var, Arg, "", InsPoint);
      Cast = CastInst::Create(CastInst::PtrToInt,Lhs,I32Ty,"",BB->getTerminator());
      s = new StoreInst(Cast, stovar,"",BB->getTerminator());
      s->setVolatile(true);
   }
   return CastInst::Create(CastInst::PtrToInt,Lhs,I32Ty,"",InsPoint);
}
static Value* convert_op(Instruction* InsPoint){
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());
   Type* I64Ty = Type::getInt64Ty(InsPoint->getContext());
   Type* DoubleTy = Type::getDoubleTy(InsPoint->getContext());
   Type* FloatTy = Type::getFloatTy(InsPoint->getContext());
   Type* DoublePtrTy = Type::getDoublePtrTy(InsPoint->getContext());
   Type* FloatPtrTy = Type::getFloatPtrTy(InsPoint->getContext());
   Type* I64PtrTy = Type::getInt64PtrTy(InsPoint->getContext());


   Value* Lhs = ConstantInt::get(I64Ty, 33333333);
   Value* Con64 = Lhs;
   Value* Con32 = ConstantInt::get(I32Ty, 3333333);
   Value* Con32sign = ConstantInt::get(I32Ty,-333333);

   Value* ConDouble = ConstantFP::get(DoubleTy,-33.3333);
   Value* ConFloat = ConstantFP::get(FloatTy,-33.3333);

   BasicBlock* BB = InsPoint->getParent();

   CallInst* Template = dyn_cast<CallInst>(InsPoint);
   Value* var = Template->getArgOperand(1);
   //Add the StoreInst to the End of BasicBlock,in order to prevent the optimization of target instructions. 
   StoreInst* s;
   CastInst* Cast;

   for(int i = 0;i < REPEAT;++i){
      if(FunctyStr == "trunc_to"){
         Con64 = ConstantInt::get(I64Ty,i*333);
         Lhs = CastInst::Create(CastInst::Trunc,Con64,I32Ty,"",InsPoint);
         s = new StoreInst(Lhs, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "zext_to"){
         Con32 = ConstantInt::get(I32Ty,i*1111);
         Lhs = CastInst::Create(CastInst::ZExt,Con32,I64Ty,"",InsPoint);

         Cast = CastInst::Create(CastInst::Trunc,Lhs,I32Ty,"",BB->getTerminator());
         s = new StoreInst(Cast, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "sext_to"){
         Con32sign = ConstantInt::get(I32Ty,-i*1234);
         Lhs = CastInst::Create(CastInst::SExt,Con32sign,I64Ty,"",InsPoint);


         Cast = CastInst::Create(CastInst::Trunc,Lhs,I32Ty,"",BB->getTerminator());
         s = new StoreInst(Cast, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "fptrunc_to"){
         ConDouble = ConstantFP::get(DoubleTy,i*333.33);
         Lhs = CastInst::Create(CastInst::FPTrunc,ConDouble,FloatTy,"",InsPoint);

         Cast = CastInst::Create(CastInst::FPToSI,Lhs,I32Ty,"",BB->getTerminator());
         s = new StoreInst(Cast, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "fpext_to"){
         ConFloat = ConstantFP::get(FloatTy,-i*33.023);
         Lhs = CastInst::Create(CastInst::FPExt,ConFloat,DoubleTy,"",InsPoint);

         Cast = CastInst::Create(CastInst::FPToSI,Lhs,I32Ty,"",BB->getTerminator());
         s = new StoreInst(Cast, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "fptoui_to"){
         ConDouble = ConstantFP::get(DoubleTy,-i*333.33);
         Lhs = CastInst::Create(CastInst::FPToSI,ConDouble,I32Ty,"",InsPoint);
         s = new StoreInst(Lhs, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "fptosi_to"){
         ConDouble = ConstantFP::get(DoubleTy,-i*333.33);
         Lhs = CastInst::Create(CastInst::FPToSI,ConDouble,I32Ty,"",InsPoint);
         s = new StoreInst(Lhs, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "uitofp_to"){
         Con64 = ConstantInt::get(I64Ty,i*333);
         Lhs = CastInst::Create(CastInst::UIToFP,Con64,DoubleTy,"",InsPoint);

         Cast = CastInst::Create(CastInst::FPToSI,Lhs,I32Ty,"",BB->getTerminator());
         s = new StoreInst(Cast, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "sitofp_to"){
         Con32sign = ConstantInt::get(I32Ty,-i*1234);
         Lhs = CastInst::Create(CastInst::SIToFP,Con32sign,DoubleTy,"",InsPoint);

         Cast = CastInst::Create(CastInst::FPToSI,Lhs,I32Ty,"",BB->getTerminator());
         s = new StoreInst(Cast, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "ptrtoint_to"){
         Lhs = CastInst::Create(CastInst::PtrToInt,var,I32Ty,"",InsPoint);
         s = new StoreInst(Lhs, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "inttoptr_to"){
         Con32 = ConstantInt::get(I32Ty,i*1111);
         Lhs = CastInst::Create(CastInst::IntToPtr,Con32,DoublePtrTy,"",InsPoint);

         Cast = CastInst::Create(CastInst::PtrToInt,Lhs,I32Ty,"",BB->getTerminator());
         s = new StoreInst(Cast, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "bitcast_to"){
         Lhs = CastInst::Create(CastInst::BitCast,var,FloatPtrTy,"",InsPoint);

         Cast = CastInst::Create(CastInst::PtrToInt,Lhs,I32Ty,"",BB->getTerminator());
         s = new StoreInst(Cast, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
      else if(FunctyStr == "addrspacecast_to"){
         
         Lhs = CastInst::Create(CastInst::AddrSpaceCast,var,I64PtrTy,"",InsPoint);

         Cast = CastInst::Create(CastInst::PtrToInt,Lhs,I32Ty,"",BB->getTerminator());
         s = new StoreInst(Cast, var,"",BB->getTerminator());
         s->setVolatile(true);
      }
   }
   return ConstantInt::get(I32Ty, 0);
}

static Value* cmp_op(Instruction* InsPoint){
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());
   Type* DoubleTy = Type::getDoubleTy(InsPoint->getContext());
   CallInst* Template = dyn_cast<CallInst>(InsPoint);
   Value* var = Template->getArgOperand(1);

   Value* Lhs = new LoadInst(var, "", InsPoint);
   if(FunctyStr == "icmp"){
      for(unsigned i=0;i<REPEAT;++i){
         Lhs = new ICmpInst(InsPoint, CmpInst::ICMP_SLT, Lhs, ConstantInt::get(I32Ty, i));
         //Here we add BitCastInst after IcmpInst, to prevent the Optimization of IcmpInst.
         Lhs = CastInst::CreateSExtOrBitCast(Lhs, I32Ty, "", InsPoint);
      }
      return Lhs;
   }else if(FunctyStr == "fcmp"){
      for(unsigned i=0;i<REPEAT;++i){
         Lhs = new FCmpInst(InsPoint, CmpInst::FCMP_OLT, Lhs, ConstantFP::get(DoubleTy, i*1.1));
         //Here we add BitCastInst after IcmpInst, to prevent the Optimization of IcmpInst.
         Lhs = new UIToFPInst(Lhs, DoubleTy, "", InsPoint);
      }
      return new FPToSIInst(Lhs, I32Ty, "", InsPoint);
   }else{
      Assert(0, "unknow operate");
      return ConstantInt::get(I32Ty, 0); 
   }
}

static Value* select_op(Instruction* InsPoint){
   Type* I1Ty = Type::getInt1Ty(InsPoint->getContext());
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());

   CallInst* Template = dyn_cast<CallInst>(InsPoint);
   Value* var = Template->getArgOperand(1);

   Value* Lhs = new LoadInst(var, "", InsPoint);
   bool sel = false;
   for(int i = 0;i < REPEAT;++i){
     Lhs = SelectInst::Create(ConstantInt::get(I1Ty, sel = !sel), Lhs,
                              ConstantInt::get(I32Ty, i), "", InsPoint);
   }
   return Lhs;
}

static Value* mix_add(Instruction* InsPoint)
{
   Type* FTy = Type::getDoubleTy(InsPoint->getContext());
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());
   Value* F_One = ConstantFP::get(FTy, 1.0L);
   Value* I_One = ConstantInt::get(I32Ty, 1);
   Value* F_Lhs = F_One, *I_Lhs = I_One;
   for(int i=0;i<REPEAT;++i){
      I_Lhs = BinaryOperator::CreateAdd(I_Lhs, I_One, "", InsPoint);
      F_Lhs = BinaryOperator::CreateFAdd(F_Lhs, F_One, "", InsPoint);
   }
   Value* I_Rhs = CastInst::Create(CastInst::FPToSI, F_Lhs, I32Ty, "", InsPoint);
   return BinaryOperator::CreateAdd(I_Lhs, I_Rhs, "", InsPoint);
}

static Value* rand_add(Instruction* InsPoint)
{
   Type* FTy = Type::getDoubleTy(InsPoint->getContext());
   Type* I32Ty = Type::getInt32Ty(InsPoint->getContext());
   Value* F_One = ConstantFP::get(FTy, 1.0L);
   Value* I_One = ConstantInt::get(I32Ty, 1);
   Value* F_Lhs = F_One, *I_Lhs = I_One;
   std::random_device rd;
   std::mt19937 gen(rd());
   std::bernoulli_distribution d(0.5);
   for(int i=0;i<REPEAT;++i){
      if(d(gen))
         I_Lhs = BinaryOperator::CreateAdd(I_Lhs, I_One, "", InsPoint);
      else
         F_Lhs = BinaryOperator::CreateFAdd(F_Lhs, F_One, "", InsPoint);
   }
   Value* I_Rhs = CastInst::Create(CastInst::FPToSI, F_Lhs, I32Ty, "", InsPoint);
   return BinaryOperator::CreateAdd(I_Lhs, I_Rhs, "", InsPoint);
}

static Value* load(Instruction* InsPoint)
{
   CallInst* Template = dyn_cast<CallInst>(InsPoint);
   Value* var = Template->getArgOperand(1);
   LoadInst* l;

	for(int i=0;i<REPEAT;++i)
	{
		l = new LoadInst(var,"",InsPoint);
      l->setVolatile(true);
	}	
	return l;
}
static Value* store(Instruction* InsPoint){
   CallInst* Template = dyn_cast<CallInst>(InsPoint);
   Value* var = Template->getArgOperand(1);
   StoreInst* s;

   for(int i = 0;i < REPEAT; ++i){
      Value* one = ConstantInt::get(InsPoint->getType(), i);
      s = new StoreInst(one, var,"",InsPoint);
      s->setVolatile(true);
   }
   return CastInst::Create(CastInst::PtrToInt,var,Type::getInt32Ty(InsPoint->getContext()),"",InsPoint);
}

std::unordered_map<std::string, lle::InstTemplate::TemplateFunc> 
lle::InstTemplate::ImplMap = {
   {"fix_add",       fix_add}, 
   {"float_add",     float_add},
   {"mix_add",       mix_add},
   {"rand_add",      rand_add},
   {"load",          load},
   {"store",         store},
   {"fix_sub",       fix_sub},
   {"float_sub",     float_sub},
   {"fix_mul",       fix_mul},
   {"float_mul",     float_mul},
   {"u_div",         us_div_rem},
   {"s_div",         us_div_rem},
   {"float_div",     float_div_rem},
   {"u_rem",         us_div_rem},
   {"s_rem",         us_div_rem},
   {"float_rem",     float_div_rem},
   {"shl",           binary_op},
   {"lshr",          binary_op},
   {"ashr",          binary_op},
   {"and",           binary_op},
   {"or",            binary_op},
   {"xor",           binary_op},
   {"alloca",        alloca_op},
   {"getelementptr", getelementptr_op},
   {"trunc_to",      convert_op},
   {"zext_to",       convert_op},
   {"sext_to",       convert_op},
   {"fptrunc_to",    convert_op},
   {"fpext_to",      convert_op},
   {"fptoui_to",     convert_op},
   {"fptosi_to",     convert_op},
   {"uitofp_to",     convert_op},
   {"sitofp_to",     convert_op},
   {"ptrtoint_to",   convert_op},
   {"inttoptr_to",   convert_op},
   {"bitcast_to",    convert_op},
   //{"addrspacecast_to",convert_op},
   {"icmp",          cmp_op},
   {"fcmp",          cmp_op},
   {"select",        select_op}
};
