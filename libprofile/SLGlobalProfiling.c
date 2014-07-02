#include "Profiling.h"
#include "tree.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#define malloc0(sz) memset(malloc(sz),0,sz)
#define SLG_SPEED_UP 1 // only trap at first time

typedef struct addr_map_v {
   void* ptr;
   int32_t idx;
   RB_ENTRY(addr_map_v) entry;
} addr_map_v;
typedef RB_HEAD(addr_map_tree, addr_map_v) addr_map_tree;

static addr_map_tree addr_map;
static unsigned *ArrayStart;
static unsigned *StoreFlags;
static unsigned NumElements;

// compare by address
int addr_map_cmp(addr_map_v* lhs, addr_map_v* rhs)
{ return (char*)lhs->ptr - (char*)rhs->ptr; }

RB_GENERATE(addr_map_tree, addr_map_v, entry, addr_map_cmp);


void SLGProfAtExitHandler(void)
{
   write_profiling_data(SLGInfo, ArrayStart, NumElements);
   free(StoreFlags);
   addr_map_v* next, *v;
   RB_FOREACH_SAFE(v, addr_map_tree, &addr_map, next){
      RB_REMOVE(addr_map_tree, &addr_map, v);
      free(v);
   }
}

void llvm_profiling_trap_store(int32_t idx, void* ptr)
{
#ifdef SLG_SPEED_UP
   if(StoreFlags[idx]==1) return;
#endif
   addr_map_v* v = (addr_map_v*)malloc0(sizeof(*v)), *h = NULL;
   v->ptr = ptr;
   v->idx = idx;
   if((h = RB_INSERT(addr_map_tree, &addr_map, v))){
      //already have ptr in tree
      h->idx = idx;
      free(v);
   }
#ifdef SLG_SPEED_UP
   else{
      StoreFlags[idx]=1;
   }
#endif
}

void llvm_profiling_trap_load(int32_t idx, void* ptr)
{
#ifdef SLG_SPEED_UP
   if(ArrayStart[idx] !=-1 && ArrayStart[idx] != 0) return;
#endif

   addr_map_v item = {ptr,0}, *v = NULL;
   v = RB_FIND(addr_map_tree, &addr_map, &item);
   if(v==NULL){
      ArrayStart[idx] = 0;
      return;
   }
#if 0
   if(ArrayStart[idx]!=-1 && ArrayStart[idx]!=0){
      assert(ArrayStart[idx] == v->idx); /* last load-store dependencies should
                                            keep same */
      return;
   }
#endif
   ArrayStart[idx] = v->idx;
}
void llvm_start_slg_setting(unsigned numStores) 
{
  StoreFlags = malloc0(sizeof(unsigned)*numStores);
}
int llvm_start_slg_profiling(int argc, const char **argv, unsigned* arrayStart,
      unsigned numLoads) {
  int Ret = save_arguments(argc, argv);
  RB_INIT(&addr_map);

  ArrayStart = arrayStart;
  NumElements = numLoads;
  memset(ArrayStart,-1,sizeof(int32_t)*NumElements);

  atexit(SLGProfAtExitHandler);
  return Ret;
}
