#include "Profiling.h"
#include <bsd/sys/tree.h>

#define malloc0(sz) memset(malloc(sz),0,sz)

typedef struct addr_map_v {
   void* ptr;
   int32_t idx;
   RB_ENTRY(addr_map_v) entry;
} addr_map_v;
typedef RB_HEAD(addr_map_tree, addr_map_v) addr_map_tree;

static addr_map_tree addr_map;
static unsigned *ArrayStart;
static unsigned NumElements;

// compare by address
int addr_map_cmp(addr_map_v* lhs, addr_map_v* rhs)
{ return lhs->ptr - rhs->ptr; }

RB_GENERATE_STATIC(addr_map_tree, addr_map_v, entry, addr_map_cmp);


void ValueProfAtExitHandler(void)
{
   addr_map_v* next, v;
   RB_FOREACH_SAFE(v, addr_map_tree, &addr_map, next){
      RB_REMOVE(addr_map_tree, &addr_map, v);
      free(v);
   }
   write_profiling_data(SLGInfo, ArrayStart, NumElements);
}

void llvm_profiling_trap_store(void* ptr, int32_t idx)
{
   addr_map_v* v = malloc0(sizeof(*v)), *h = NULL;
   v->ptr = ptr;
   v->idx = idx;
   if(h = RB_INSERT(addr_map_tree, &addr_map, v)){
      //already have ptr in tree
      h->idx = idx;
      free(v);
   }
}

void llvm_profiling_trap_load(void* ptr, int32_t idx)
{
   addr_map_v item = {ptr,0}, *v = NULL;
   v = RB_FIND(addr_map_tree, &addr_map, &item);
   assert(v); // should first store global value
   if(ArrayStart[idx]){
      assert(ArrayStart[idx] == v->idx); /* last load-store dependencies should
                                            keep same */
      return;
   }
   ArrayStart[idx] = v->idx;
}

int llvm_start_slg_profiling(int argc, const char **argv,
                              unsigned *arrayStart, unsigned numElements) {
  int Ret = save_arguments(argc, argv);
  RB_INIT(&map_tree);

  ArrayStart = arrayStart;
  NumElements = numElements;

  atexit(SLGProfAtExitHandler);
  return Ret;
}
