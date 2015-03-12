#include "libtiming.c"
#include <unistd.h>

#define REPNUM 10000
#define INSNUM 2000
#define ALLOCA_NUM 100

//ref+=inst_template(TEMPLATE,VAR);
#define REPEAT(TEMPLATE, REP, VAR...)                                          \
  {                                                                            \
    for (int i = 0; i < REP; ++i) {                                            \
      beg = timing();                                                          \
      ref += inst_template(TEMPLATE, ##VAR);                                   \
      end = timing();                                                          \
      sum += end - beg - t_err;                                                \
    }                                                                          \
    sum /= REP;                                                                \
    ref /= REP;                                                                \
    double ins_cycles = (double)sum / INSNUM;                                  \
    printf(TEMPLATE ":\t%lf nanoseconds,\t%lf cycles\n",                       \
           ins_cycles *cycle_time, ins_cycles);                                \
  }
#define REPEAT_INST(TEMPLATE, VAR...) REPEAT(TEMPLATE, REPNUM, ##VAR)
#define REPEAT_ALLOCA_INST(TEMPLATE, VAR...) REPEAT(TEMPLATE, ALLOCA_NUM, ##VAR)
#define REPEAT_GETELE_INST(TEMPLATE, VAR...) REPEAT(TEMPLATE, REPNUM, element, ##VAR)

static int element[1][INSNUM][2];
static double cycle_time;         
double calcu_cycle_time(){
   uint64_t t_res = timing_res();
   uint64_t t_err = timing_err();
   unsigned long beg = 0, end = 0, sum = 0, ref = 0;
   beg = timing();
   sleep(1);
   end = timing();
   sum += (end-beg-t_err);
   beg = timing();
   sleep(0);
   end = timing();
   unsigned long sleep_err = end - beg - t_err;
   sum = sum-sleep_err;
   double cyc_tim = (1/(double)sum)*1E9;
   printf("CPU freq: %lf GHz\n",(double)sum/1E9);
   return cyc_tim;
}

int main()
{
   //Here we can also read system file to obtain the hightest CPU frequency  
   cycle_time = calcu_cycle_time();
   volatile int* var = malloc(sizeof(int));
   uint64_t beg, end, sum = 0;
   int ref = 0;
   uint64_t t_err = timing_err();
   int integer = 333333;
   double real = 333.333;

   for(unsigned k = 0;k<1;k++){
      for(unsigned i = 0;i < INSNUM;i++){
         for(unsigned j = 0; j < 2;j++){
            element[k][i][j] = k+i+j;
         }
      }
   }

   REPEAT_INST("load",var);
   REPEAT_INST("store",var);
   REPEAT_INST("fix_add",var);
   REPEAT_INST("float_add",var);
   REPEAT_INST("fix_mul",var);
   REPEAT_INST("float_mul",var);
   REPEAT_INST("fix_sub",var);
   REPEAT_INST("float_sub",var);
   REPEAT_INST("u_div",var);
   REPEAT_INST("s_div",var);
   REPEAT_INST("float_div",var);
   REPEAT_INST("u_rem",var);
   REPEAT_INST("s_rem",var);
   REPEAT_INST("float_rem",var);
   REPEAT_INST("shl",var);
   REPEAT_INST("lshr",var);
   REPEAT_INST("ashr",var);
   REPEAT_INST("and",var);
   REPEAT_INST("or",var);
   REPEAT_INST("xor",var);
   REPEAT_ALLOCA_INST("alloca",var);
   REPEAT_GETELE_INST("getelementptr",var);
   REPEAT_INST("trunc_to",var);
   REPEAT_INST("zext_to",var);
   REPEAT_INST("sext_to",var);
   REPEAT_INST("fptrunc_to",var);
   REPEAT_INST("fpext_to",var);
   REPEAT_INST("fptoui_to",var);
   REPEAT_INST("fptosi_to",var);
   REPEAT_INST("uitofp_to",var);
   REPEAT_INST("sitofp_to",var);
   REPEAT_INST("ptrtoint_to",var);
   REPEAT_INST("inttoptr_to",var);
   REPEAT_INST("bitcast_to",var);
   REPEAT_INST("icmp",var,integer);
   REPEAT_INST("fcmp",var,real);
   REPEAT_INST("select",var);
   return 0;
}
