#include "Profiling.h"
#include <sys/queue.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#define malloc0(sz) memset(malloc(sz),0,sz)
#define trunk_size 100

static unsigned *ArrayStart;
static unsigned NumElements;
//record true write value content count.

typedef struct ValueItem{
	SLIST_ENTRY(ValueItem) next;
	int value[trunk_size];
}ValueItem;
typedef SLIST_HEAD(ValueEntry,ValueItem) ValueEntry;

typedef struct ValueHead{
	unsigned count;//real write bits length
	enum ProfilingFlags flags; //should only use under 32bit
	int pos; //current write position in first trunk
#ifdef ENABLE_COMPRESS
	struct {
		int last_value;
	}compress;
#endif
	ValueEntry entry;
}ValueHead;

static ValueHead* ValueLink = NULL;

void ValueProfAtExitHandler(void)
{
#define EXIT_ON_ERROR {\
	fprintf(stderr,"error: unable to write to output file.");\
	exit(0); }

	int* buffer = NULL;
	int OutFile = getOutFile();
	write_profiling_data(ValueInfo, ArrayStart, NumElements);
	int i=0;
	for(i=0;i<NumElements;i++){
		unsigned writeCount = ValueLink[i].count+1;// extra flags size;
		int flags = ValueLink[i].flags;// on 64bit enum is long type
		write(OutFile,&writeCount,sizeof(unsigned));
		write(OutFile,&flags,sizeof(int));
		size_t len = sizeof(int)*ValueLink[i].count;
		if(len==0) continue;
		buffer = malloc0(len);
		ValueItem* item = NULL;
		ValueEntry* entry = &ValueLink[i].entry;
		int* w = buffer+ValueLink[i].count;
		SLIST_FOREACH(item, entry, next){
			if(item == SLIST_FIRST(entry)){
				int size = ValueLink[i].pos;
				memcpy(w-size,item->value,sizeof(int)*size);
				w-=size;
			}else{
				memcpy(w-trunk_size,item->value,sizeof(int)*trunk_size);
				w-=trunk_size;
			}
		}
		if(write(OutFile,buffer,len)<0)
			EXIT_ON_ERROR;
		free(buffer);
	}
	close(OutFile);
#undef EXIT_ON_ERROR
}

void llvm_profiling_trap_value(int index,int value,int isConstant)
{
	++ArrayStart[index];
	if(isConstant) return;
	int* _pos = &ValueLink[index].pos;
#define pos (*_pos)
	ValueEntry* entry = &ValueLink[index].entry;
	if(pos >= trunk_size || SLIST_EMPTY(entry)){
		ValueItem* ins = malloc0(sizeof(ValueItem));
		SLIST_INSERT_HEAD(entry, ins, next);
		pos = 0;
	}
	ValueItem* item = SLIST_FIRST(entry);
#ifdef ENABLE_COMPRESS
	if(pos == 0||item->value[pos-2]!=value||item->value[pos-1]==INT_MAX){
		item->value[pos]=value;
		item->value[pos+1]=1;
		pos+=2;
		ValueLink[index].count+=2;
	}else 
		++item->value[pos-1];
#else
	++ValueLink[index].count;
	item->value[pos] = value;
	++pos;
#endif
#undef pos
}

int llvm_start_value_profiling(int argc, const char **argv,
                              unsigned *arrayStart, unsigned numElements) {
  int Ret = save_arguments(argc, argv);
  ArrayStart = arrayStart;
  NumElements = numElements;
  ValueLink = malloc0(sizeof(*ValueLink)*NumElements);
  int i=0;
  for(i=0;i<NumElements;++i){
	  ValueLink[i].flags |= CONSTANT_COMPRESS;
#ifdef ENABLE_COMPRESS
	  ValueLink[i].flags |= RUN_LENGTH_COMPRESS;
#endif
  }
  atexit(ValueProfAtExitHandler);
  return Ret;
}
