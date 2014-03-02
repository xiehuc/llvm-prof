#ifndef LLVM_VALUE_PROFILER_H_H
#define LLVM_VALUE_PROFILER_H_H
#include <llvm/Pass.h>
#include <llvm/IR/Value.h>
#include <vector>


/**
    #.  ValueProfiling的输出格式

        首先一般通常的Profiling就是由一个定长大数组组成的,数组元素是某个统计的
        计数器,而ValueProfiling为了记录每次的值,除了需要大数组记录经过次数之外,
        还需要在其尾部追加很多段不定长的数组,用于记录捕获到的每个值的内容,例如::

            {|3|2|1|}{|9|8|7|}{|5|4|}{|2|}

        表示记录的三个Value,分别捕获到的次数是3,2,1,其中第一个数的完整内容是
        9,8,7,第二个数的内容是5,4,以此类推.由于大数组中的和指定了所有数的实际长
        度,所以可以通过循环来读取所有完整的内容.
        
        实际上,每个数捕获到的次数可能会很多,所以还是会造成可观的内存消耗,因此每
        个捕获的类型选用int32型而不是int64类型,从而降低内存压力.

        另外,对于常数,我们也没有必要完整的记录所有信息,我们可以保留它的捕获次数
        ,而将它对应的内容标记为-1,只占用一个单位,我们就知道这个Value是一个常数,
        当然,无论何时它的值我们也是能够推导出来的.

    #.  ValueProfiling的插桩
    
        ValueProfiling的插桩分为两个部分,首先是需要 *选中* 那些感兴趣的Value,使
        用 ``insertValueTrap`` 函数插入一个函数调用,保证在运行时将value的值复制
        出来保存到内存中,同时还保持了一个计数器从而统计一共有多少要捕获的Value,
        另外一个部分则是使用 ``runOnModule`` 在main函数中插入初始化函数,这里正
        好使用第一步统计出来的数量信息,直接在ir中创建一个大数组(GlobalValue).它
        的实现并不困难,可以参考LLVM中的源码.
        
        需要额外说明的是这里有两种思路,一种是通过LLVM的一点一点的创建ir,把实际
        的代码直接写入bitcode中,另外一种是使用c语言事先在libprofile中写好.实际
        上从难以程度来将后者完爆前者,c语言自然是容易的.从实际的运算结果上来看两
        者也都是正确的.但是LLVM的一些profiling选择使用前者来作为实现基础,是从效
        率方面考虑的,毕竟有些就是测量执行时间,如果是加入了太多的函数调用,其影响
        是比较可观的,而直接 *内联* 在ir中自然快许多.

        在LLVM的Profiling框架中都会包含一个在ir中内联的全局大数组变量,并且是不
        可少的(长度信息从它读取),至于Profiling的其它部分的实现,我们为了简化,就
        选择了事先在libprofile里面写好的方式.因为里面要操作链表,直接用LLVM构建
        ir则会非常困难,并且我们的目的是捕获Value,即使拖慢了程序的执行速度也没有
        关系,反正数值还是一样的.

        对于常数,我们不能插入函数调用,同时还要保持计数器的增长,所以只需要调用
        LLVM内置的 ``IncrementCounterInBlock`` 即可,它会写入ir的add指令增加计数
        器,同时不会捕获Value的内容.

    #.  ValueProfiling的内存组织

        前面插桩完成之后,就可以编译连接,动态运行了,前面说道信息有两部分,第一部
        分大数组是定长的,没有问题.关键是后面是一个不定长的,这里使用了链表来实现
        内存组织,对每个要捕获的Value都建立一个链表::

            |3|--->|7|->|8|->|9|
            |2|--->|4|->|5|
            |1|--->|2|

        插入方向是头插,每个元素只有一个int型的内容和next指针,这里选用链表其实还
        是有很大弊端的,1是内容浪费比较严重,next指针长度就是int的2倍,导致了浪费
        了200%的内存空间.并且频繁的调用malloc也非常的不好,就只有先这样了.

        当程序执行完了,就在内存中建立了一个非常大的链表,并且方向是反的,我们在退
        出的时候会触发一个函数,在这时由于已经有了每个Value的次数了.所以就可以建
        立足够长的buffer,并且将内容逆序赋值到buffer中,再一次性的写入内存,从而将
        顺序颠倒回来,并且增加写入时的io效率.

        对于常数,由于不会捕获内容,所以链表为空.

*/


namespace llvm{
	class ValueProfiler: public ModulePass
	{
		GlobalVariable* Counters;
		int numTrapedValues;
		typedef std::vector<std::pair<int,BasicBlock*> > ConstantTrapTy;
		ConstantTrapTy ConstantTraps;
		public:
		static char ID;
		ValueProfiler();
		//insert a value profiler, return inserted call instruction
		Instruction* insertValueTrap(Value* value,BasicBlock* InsertTail);
		Instruction* insertValueTrap(Value* value,Instruction* InsertBefore);
		//insert initial profiling for module
		bool runOnModule(Module& M);
		int getNumTrapedValues() const { return numTrapedValues;}
		virtual const char* getPassName() const{
			return "ValueProfiler";
		}
	};
}
#endif
