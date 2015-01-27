#include "libtiming.c"

#define REPEAT 10000
#define INSNUM 2000
#define REPEAT_CAST_INST(INS,VAR,INSTY)\
   for(int i=0;i<REPEAT;++i){\
      beg = timing();\
      inst_template(INS,VAR);\
      end = timing();\
      sum += end-beg-t_err;\
   }\
   sum /= REPEAT;\
   ref /= REPEAT;\
   printf(INSTY,(double)sum/INSNUM);

#define REPEAT_ALLOCA_INST(INS,VAR,INSTY)\
   for(int i=0;i<100;++i){\
      beg = timing();\
      ref+=inst_template(INS,VAR);\
      end = timing();\
      sum += end-beg-t_err;\
   }\
   sum /= 100;\
   ref /= 100;\
   printf(INSTY,(double)sum/INSNUM);

#define REPEAT_INST(INS,VAR,INSTY)\
   for(int i=0;i<REPEAT;++i){\
      beg = timing();\
      ref+=inst_template(INS,VAR);\
      end = timing();\
      sum += end-beg-t_err;\
   }\
   sum /= REPEAT;\
   ref /= REPEAT;\
   printf(INSTY,(double)sum/INSNUM);

#define REPEAT_GETELE_INST(INS,VAR,INSTY)\
   int element[1][INSNUM][2];\
   for(int k = 0;k <1;k++){\
      for(int i = 0;i < INSNUM;i++){\
         for(int j = 0; j < 2;j++){\
            element[k][i][j] = 1;\
         }\
      }\
   }\
   for(int i=0;i<REPEAT;++i){\
      beg = timing();\
      ref+=inst_template(INS,element,VAR);\
      end = timing();\
      sum += end-beg-t_err;\
   }\
   sum /= REPEAT;\
   ref /= REPEAT;\
   printf(INSTY,(double)sum/INSNUM);

enum INSTY{
   LOAD,
   STORE,
   FIX_ADD,
   FLOAT_ADD,
   FIX_MUL,
   FLOAT_MUL,
   FIX_SUB,
   FLOAT_SUB,
   U_DIV,
   S_DIV,
   FLOAT_DIV,
   U_REM,
   S_REM,
   FLOAT_REM,
   SHL,
   LSHR,
   ASHR,
   AND,
   OR,
   XOR,
   ALLOCA,
   GETELEMENTPTR,
   TRUNC,
   ZEXT,
   SEXT,
   FPTRUNC,
   FPEXT,
   FPTOUI,
   FPTOSI,
   UITOFP,
   SITOFP,
   PTRTOINT,
   INTTOPTR,
   BITCAST,
   ADDRSPACECAST,
   ICMP,
   FCMP,
   SELECT
};
/* use a template to generate instruction */
int inst_template(const char* templ, ...);
void test_otherOp(enum INSTY ty){

   volatile int* var = malloc(sizeof(int));
   uint64_t beg, end, sum = 0;
   int ref = 0;
   uint64_t t_err = timing_err();
   if(ty == LOAD){
      REPEAT_INST("load",var,"load : %f cycles\n");
   }
   if(ty == STORE){
      REPEAT_INST("store",var,"store : %f cycles\n");
   }
   if(ty == FIX_ADD){
      REPEAT_INST("fix_add",var,"add : %f cycles\n");
   }
   if(ty == FLOAT_ADD){
      REPEAT_INST("float_add",var,"fadd : %f cycles\n");
   }
   if(ty == FIX_MUL){
      REPEAT_INST("fix_mul",var,"mul : %f cycles\n");
   }
   if(ty == FLOAT_MUL){
      REPEAT_INST("float_mul",var,"fmul : %f cycles\n");
   }
   if(ty == FIX_SUB){
      REPEAT_INST("fix_sub",var,"sub : %f cycles\n");
   }
   if(ty == FLOAT_SUB){
      REPEAT_INST("float_sub",var,"fsub : %f cycles\n");
   }
   if(ty == U_DIV){
      REPEAT_INST("u_div",var,"udiv : %f cycles\n");
   }
   if(ty == S_DIV){
      REPEAT_INST("s_div",var,"sdiv : %f cycles\n");
   }
   if(ty == FLOAT_DIV){
      REPEAT_INST("float_div",var,"fdiv : %f cycles\n")
   }
   if(ty == U_REM){
      REPEAT_INST("u_rem",var,"urem : %f cycles\n");
   }
   if(ty == S_REM){
      REPEAT_INST("s_rem",var,"srem : %f cycles\n");
   }
   if(ty == FLOAT_REM){
      REPEAT_INST("float_rem",var,"frem : %f cycles\n");
   }
   if(ty == SHL){
      REPEAT_INST("shl",var,"shl : %f cycles\n");
   }
   if(ty == LSHR){
      REPEAT_INST("lshr",var,"lshr : %f cycles\n");
   }
   if(ty == ASHR){
      REPEAT_INST("ashr",var,"ashr : %f cycles\n");
   }
   if(ty == AND){
      REPEAT_INST("and",var,"and : %f cycles\n");
   }
   if(ty == OR){
      REPEAT_INST("or",var,"or : %f cycles\n");
   }
   if(ty == XOR){
      REPEAT_INST("xor",var,"xor : %f cycles\n");
   }
   if(ty == ALLOCA){
      REPEAT_ALLOCA_INST("alloca",var,"alloca : %f cycles\n");
   }
   if(ty == GETELEMENTPTR){
      REPEAT_GETELE_INST("getelementptr",var,"getelementptr : %f cycles\n");
   }
   if(ty == TRUNC){
      REPEAT_CAST_INST("trunc_to",var,"trunc_to : %f cycles\n");
   }
   if(ty == ZEXT){
      REPEAT_CAST_INST("zext_to",var,"zext_to : %f cycles\n");
   }
   if(ty == SEXT){
      REPEAT_CAST_INST("sext_to",var,"sext_to : %f cycles\n");
   }
   if(ty == FPTRUNC){
      REPEAT_CAST_INST("fptrunc_to",var,"fptrunc_to : %f cycles\n");
   }
   if(ty == FPEXT){
      REPEAT_CAST_INST("fpext_to",var,"fpext_to : %f cycles\n");
   }
   if(ty == FPTOUI){
      REPEAT_CAST_INST("fptoui_to",var,"fptoui_to : %f cycles\n");
   }
   if(ty == FPTOSI){
      REPEAT_CAST_INST("fptosi_to",var,"fptosi_to : %f cycles\n");
   }
   if(ty == UITOFP){
      REPEAT_CAST_INST("uitofp_to",var,"uitofp_to : %f cycles\n");
   }
   if(ty == SITOFP){
      REPEAT_CAST_INST("sitofp_to",var,"sitofp_to : %f cycles\n");
   }
   if(ty == PTRTOINT){
      REPEAT_CAST_INST("ptrtoint_to",var,"ptrtoint_to : %f cycles\n");
   }
   if(ty == INTTOPTR){
      REPEAT_CAST_INST("inttoptr_to",var,"inttoptr_to : %f cycles\n");
   }
   if(ty == BITCAST){
      REPEAT_CAST_INST("bitcast_to",var,"bitcast_to : %f cycles\n");
   }
   //if(ty == ADDRSPACECAST){
   //   REPEAT_INST("addrspacecast_to",var,"addrespacecast_to : %f cycles\n");
   //}
   if(ty == ICMP){
      //REPEAT_INST("icmp",var,"icmp : %f cycles\n");
      for(int i = 0;i < REPEAT;++i){
         int tmp = i;
         beg = timing();
         ref += inst_template("icmp",var,tmp);
         end = timing();
         sum += end - beg -t_err;
      }
      sum /= REPEAT;
      ref /= REPEAT;
      printf("icmp : %f cycles\n",(double)sum/INSNUM);
   }
   if(ty == FCMP){
      //REPEAT_INST("fcmp",var,"fcmp : %f cycles\n");
      for(int i = 0;i < REPEAT;++i){
         float tmp = (float)i+0.01;
         beg = timing();
         ref += inst_template("fcmp",var,tmp);
         end = timing();
         sum += end - beg -t_err;
      }
      sum /= REPEAT;
      ref /= REPEAT;
      printf("fcmp : %f cycles\n",(double)sum/INSNUM);
   }
   if(ty == SELECT){
      REPEAT_INST("select",var,"select : %f cycles\n");
   }
}
int main()
{
   enum INSTY insty;
   test_otherOp(insty = LOAD);
   test_otherOp(insty = STORE);
   test_otherOp(insty = FIX_SUB);
   test_otherOp(insty = FLOAT_SUB);
   test_otherOp(insty = FIX_MUL);
   test_otherOp(insty = FLOAT_MUL);
   test_otherOp(insty = FIX_ADD);
   test_otherOp(insty = FLOAT_ADD);
   test_otherOp(insty = U_REM);
   test_otherOp(insty = S_REM);
   test_otherOp(insty = FLOAT_REM);
   test_otherOp(insty = U_DIV);
   test_otherOp(insty = S_DIV);
   test_otherOp(insty = FLOAT_DIV);
   test_otherOp(insty = ASHR);
   test_otherOp(insty = LSHR);
   test_otherOp(insty = SHL);
   test_otherOp(insty = XOR);
   test_otherOp(insty = OR);
   test_otherOp(insty = AND);
   test_otherOp(insty = ALLOCA);
   test_otherOp(insty = GETELEMENTPTR);
   test_otherOp(insty = TRUNC);
   test_otherOp(insty = ZEXT);
   test_otherOp(insty = SEXT);
   test_otherOp(insty = FPTRUNC);
   test_otherOp(insty = FPEXT);
   test_otherOp(insty = FPTOUI);
   test_otherOp(insty = FPTOSI);
   test_otherOp(insty = UITOFP);
   test_otherOp(insty = SITOFP);
   test_otherOp(insty = PTRTOINT);
   test_otherOp(insty = INTTOPTR);
   test_otherOp(insty = BITCAST);
   //test_otherOp(insty = ADDRSPACECAST);
   test_otherOp(insty = ICMP);
   test_otherOp(insty = FCMP);
   test_otherOp(insty = SELECT);
   return 0;
}

