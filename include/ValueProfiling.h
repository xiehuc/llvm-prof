#ifndef LLVM_VALUE_PROFILER_H_H
#define LLVM_VALUE_PROFILER_H_H
#include <llvm/Pass.h>
#include <llvm/IR/Value.h>
#include <vector>


/**
    #.  ValueProfiling的输出格式

        首先一般通常的Profiling就是由一个定长大数组组成的,数组元素是某个统计的
        计数器,而ValueProfiling为了记录每次的值,除了需要大数组记录经过次数之外,
        还需要在其尾部追加很多段不定长的数组,用于记录捕获到的每个值的内容,例如
        下面的定义形式(不是最终的格式)::

            {|3|2|1|}{|9|8|7|}{|5|4|}{|2|}

        表示记录的三个Value,分别捕获到的次数是3,2,1,其中第一个数的完整内容是
        9,8,7,第二个数的内容是5,4,以此类推.由于大数组中的和指定了所有数的实际长
        度,所以可以通过循环来读取所有完整的内容.
        
        实际上,每个数捕获到的次数可能会很多,所以还是会造成可观的内存消耗,因此每
        个捕获的类型选用int32型而不是int64类型,从而降低内存压力.

        首先,捕获的次数使用Profiling的 `标准格式 <profiling-format_>`_ 

        对捕获的值的内容,我们使用自定义的格式,为了保证能够顺利的切分出不同的数
        组 --- 先写入数组长度,再写入flag,再写入数组内容(捕获到的值).因此整个的
        组织是::

            {...Counters...} //记录捕获的次数
            {|count|flag|{... content ...}   //Value1的内容
             |count|flag|{... content ...}   //Value2的内容
             |count|flag|{... content ...}}  //Value3的内容

        其中长度是包含flag和内容数组的长度(即不包含自身).flag用于区分一些特殊标
        记,如数据压缩格式等等.

        数据压缩是一个很重要的内容,有效的压缩形式不仅节省内存占用,还加快CPU处理
        速度,是十分必要的. 现在使用两种压缩形式:

        +  *CONSTANT_COMPRESS* : 对于常数,我们不需要记录它的值数组,因为能够从源
           代码中找到,只需要记录捕获的次数,就能够重新生成整个数组.

        +  *RUN_LENGTH_COMPRESS* : 游程编码,默认启用,可以在编译时关闭.由于很多
           捕获到的数据都是不变的只有一个值,这个时候使用游程编码则非常的简单有
           效,在content中首先写入实际的值,再写入重复的次数,就完成了游程编码了.
           如果对于经常变化的游程为1的数据,游程编码的浪费率为100%,但是除此之外
           都能够保证至少不浪费,考虑到大量的重复次数很大的数都是不变的,所以还是
           非常可靠的.

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

        插入方向是头插,每个元素只有一个int型的内容和next指针.
        
        通常链表还是有很大弊端,1是内容浪费比较严重,next指针长度就是int的2倍,导
        致了浪费了200%的内存空间.并且频繁的调用malloc也非常的不好. 所以这里使用
        了 *trunk* 来解决这种问题,其思路非常简单. 申请空间按块大小(默认100)来
        做,每 *trunk_size* 个数据共用这一个块,写满了再去申请下一个块,所以这个时
        候next指针的浪费率为 :math:`\frac 1 {trunk\_size}` 已经完全無問題了.

        当程序执行完了,就在内存中建立了一个非常大的链表,并且方向是反的,我们在退
        出的时候会触发一个函数,在这时由于已经有了每个Value的次数了.所以就可以建
        立足够长的buffer,并且将内容逆序赋值到buffer中,再写完了长度和flag之后一
        次性的写入内存,从而将顺序颠倒回来,并且增加写入时的io效率.

        对于数据压缩则在trunk内写入压缩后的格式,常数压缩则不写入,游程压缩则每次
        写入2个数.

    #.  ValueProfiling的读取

        和其它Profiling不同,ValueProfiling不是全局有效的,所以不能像其他
        Profiling一样通过相同的遍历顺序来建立映射,因此,它是通过试图找寻
        ``llvm_profiling_trap_value`` 这个函数调用的指令,来建立的映射,因为该指
        令的第一个参数是index,第二个参数是Value,就可以建立[index] |srarr| inst 
        这样的映射了.然后加上llvmprof.out中的数据,其格式为[index] |srarr|
        {num,contents},因此就可以联立起来消除index,从而建立[inst] |srarr|
        {num,contents}的数据结构,于是读取完成.
        
        为了方便叙述,需要先定义一些名称:

        +  *trapedValue* : 指的 [inst] ,是那条Call语句,作为主键,能够方便的找到
           其他内容(index,value等),还能够通过 getParent() 找到所在的位置.
        +  *trapedTarget* : 指的是 [value], 是经过 *castoff* [#]_ 之后的结果.有
           些value是常数,在LLVM中相等的常数指向同一个对象,所以以前用它作为map的
           主键时导致了数据冲突从而覆盖了一些数据.造成数据不完整,现在改用
           trapedValue 做主键之后则没有这些问题了.
        +  *trapedIndex* : 指 [index]

    #.  ValueProfiling的访问时内存结构

        根据上面的内容,使用一个map<CallInst*,ValueInfo>来记录所有的信息,其中
        ValueInfo是一个结构体,包含了num,contents. contents的类型是vector<int>记
        录了压缩格式的捕获的所有的值的内容. 常数的contents为空.当需要使用的时候
        使用 ``getValueContent()`` 来临时生成完整的数组以便使用.
        
        判断value是否为常数可以使用 ``isa<Constant>(getTrapedTarget())`` 必须使
        用getTrapedTarget,因为它返回castoff之后的结果,在castoff之前可能不是指向
        真正的常数而是只想的cast语句(一定是一个变量),造成判断失误.

.. [#] 因为在写入ir的时候需要将其它类型 转换(cast) 到i32 类型.castoff则是反向过
   程,脱掉cast语句,因为cast语义是不改变值内容.所以在大部分情况下castoff过程都是
   安全的.

**/


namespace llvm{
	class ValueProfiler: public ModulePass
	{
		static int numTrapedValues;

		GlobalVariable* Counters;
		typedef std::vector<std::pair<int,BasicBlock*> > ConstantTrapTy;
		ConstantTrapTy ConstantTraps;
		public:
		static char ID;
		ValueProfiler();
		//insert a value profiler, return inserted call instruction
		static Instruction* insertValueTrap(Value* value,BasicBlock* InsertTail);
		static Instruction* insertValueTrap(Value* value,Instruction* InsertBefore);
		//insert initial profiling for module
		bool runOnModule(Module& M);
		int getNumTrapedValues() const { return numTrapedValues;}
		virtual const char* getPassName() const{
			return "ValueProfiler";
		}
	};
}
#endif
