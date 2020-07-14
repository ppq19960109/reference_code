# desc
 描述依赖的第三方库，包括代码路径，交叉编译等。

# zxing-cpp
	git clone https://gitee.com/Lucifer3502/zxing-cpp.git
	mkdir build
	cmake ..
	make
	make install

# libevent
	git clone https://gitee.com/Lucifer3502/libevent.git
	./autogen.sh
	./configure --prefix=`pwd`/_install  CC=arm-xm-linux-gcc CXX=arm-xm-linux-g++ --host=arm-xm-linux
	make
	make install

# ntpdate
	mkdir -p _install
	./configure --prefix=`pwd`/_install --host=arm-xm-linux CC=arm-xm-linux-gcc --with-yielding-select=yes
	make
	make install

# zlib
	export CC=arm-xm-linux-gcc
	./configure --prefix=$PWD/_install
	make
	make install

# freetype-2.10.1.tar.gz
	tar zxvf freetype-2.10.1.tar.gz
	cd freetype-2.10.1
	./configure --prefix=`pwd`/_install  CC=arm-xm-linux-gcc CXX=arm-xm-linux-g++ --host=arm-xm-linux
	make
	make install

# lvgl
	git clone https://gitee.com/Lucifer3502/pc_simulator_sdl_eclipse.git --recurse-submodules
	mkdir build
	cd build
	cmake ..
	make

