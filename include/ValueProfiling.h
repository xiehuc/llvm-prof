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
        ,这个的目的是为了减少内存占用,如果不用记录所有的常量的话,减小的记录的量
        还是非常可观的.

        因此,我们另外使用一组计数器,记录实际写入的数据数量,因此整个的组织是::

            ValueInfo    ---> {...Counters...} //记录捕获的次数
            ValueContent ---> {...Counters...} //记录实际写入数据的数量
                              {{... content ...}   //Value1的内容
                               {... content ...}   //Value2的内容
                               {... content ...}}  //Value3的内容
        
        这样的格式能够有效应对其它内存压缩方式,比如说将相同的数合并到一起,附加
        记录重复次数等等. 最后压缩后的数据应该能够还原回完整的捕获的内容.

        .. tip::
            
            过去使用-1作为间隔,不使用第二组计数器,但是在cgpop中捕获到大量的-1才
            导致更改为现在的格式.至于为什么cgpop捕获到-1则是以后要去思考原因的
            话题了.

    #.  ValueProfiling的插桩
    
        ValueProfiling的插桩分为两个部分,首先是需要 *选中* 那些感兴趣的Value,使
        用 ``insertValueTrap`` 函数插入一个函数调用
        ``llvm_profiling_trap_value`` ,该函数的作用是保证在运行时将value的值复
        制出来保存到内存中,同时还保持了一个计数器从而统计一共有多少要捕获的
        Value,另外一个部分则是使用 ``runOnModule`` 在main函数中插入初始化函数,
        这里正好使用第一步统计出来的数量信息,直接在ir中创建一个大数组
        (GlobalValue).它的实现并不困难,可以参考LLVM中的源码.
        
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

        对于常数,我们需要保持计数器的增长,但是不用捕获其内容,所以在
        ``llvm_profiling_trap_value`` 中添加了一个参数用于标识是否为常数,如果是
        的话,只增长其计数器而不执行捕获部分的代码.因此计数器的数大于等于实际写
        入的数据.为了正确的读取数据的确定长度需要使用另外一组计数器和实际写入的
        数据保持一致.

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

    #.  ValueProfiling的读取

        和其它Profiling不同,ValueProfiling不是全局有效的,所以不能像其他
        Profiling一样通过相同的遍历顺序来建立映射,因此,它是通过试图找寻
        ``llvm_profiling_trap_value`` 这个函数调用的指令,来建立的映射,因为该指
        令的第一个参数是index,第二个参数是Value,就可以建立[index] |srarr| value
        这样的映射了.然后加上llvmprof.out中的数据,其格式为[index] |srarr|
        {num,contents},因此就可以联立起来消除index,从而建立[value] |srarr|
        {inst,num,contents}的数据结构,于是读取完成.其中inst是那条call指令的指针
        ,因为有一些Value是常数,是没有getParent()来找到所在的位置的,只有
        Instruction才有getParent()来返回所在的BasicBlock.同时inst还包含了index
        值以便不时之需.

    #.  ValueProfiling的访问时内存结构

        根据上面的内容,使用一个map<Value*,ValueInfo>来记录所有的信息,其中
        ValueInfo是一个结构体,包含了inst,num,contents. contents的类型是
        vector<int>记录了捕获的所有的值的内容. 常数的contents为空.判断value是否
        为常数有多种方法,不一一赘述.

**/


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
