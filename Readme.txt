1. autogen.sh is used to generate the automake related file. The content is the below command.

aclocal
autoheader
automake --add-missing --copy
autoconf

2. cross_compile.sh is used to generate cross compile makefile. It's the command as below.

./configure --host=arm-linux-gnueabihf

If the host is equal to arm-linux-gnueabi, there may be some problems. I encountered that when I ran 
cupdi in the target system, it will implied that libc.so.6 can't be found. 
There're two ways to solve the problem. one is to use arm-linux-gnueabihf. Another is to use CFLAGS = -static 
in all the makefile, so all the lib is statically linked and the size of executed file will be a little large.


