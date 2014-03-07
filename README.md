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

argument
---------

* `-list-all`      : print out all outputs
* `-value-content` : print out traped value detail content instead of brief report

note
-----

you need add `$PKG_CONFIG_PATH` to let pkg-config recognise llvm-prof.pc
