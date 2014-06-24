llvm-prof
===========

this was copied form llvm release 3.3 include llvm-prof libLLVMProfiling
libprofile\_rt.so 

because since llvm release 3.4 they remove profiling related code totaly, so
this project would help you get the function back.

build
------

	$ mkdir build
	$ cd build
	$ cmake .. 
	$ make 
	$ sudo make install

option
-------

*  `LLVM_RECOMMAND_VERSION` : select which llvm version to build
*  `OUTPUT_HASPID`          : does llvmprof.out contain a pid for mpi program
*  `DYNAMIC_LINK`           : dynamic link LLVM single big shared object

argument
---------

* `-list-all`      : print out all outputs
* `-value-content` : print out traped value detail content instead of brief report
* `-unsort`        : print out outputs without sort
* `-diff`          : compare two output file and report whether they are different
                     example: ``llvm-prof -diff a.out b.out``

extra profilings
-----------------

* *ValueProfiling*    : provide value profiling, could trap some value.
* *SLGlobalProfiling* : provide store load profiling on global variables, could
                        find dynamic data denpendencies.

note
-----

you need add `$PKG_CONFIG_PATH` to let pkg-config recognise llvm-prof.pc
