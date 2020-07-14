# ARCH = x64, arm 
ARCH = arm
TARGET = linux-xm650


LUAJIT = ./deps/LuaJIT-2.1.0-beta3/luajit

TARGETDIR = ./targets/$(TARGET)

appbins = 	Utils.bin			\
			Basedb.bin			\
			Logrdb.bin			\
			LogrGen.bin			\
			CPAdapter.bin 		\
			OSAdapter.bin		\
			CloudApp.bin		\
			WebApp.bin			\
			WalOS.bin

luabins = $(appbins) $(testbins)

all : $(luabins) 

vpath %.lua lua
vpath %.bin targets/$(TARGET)/app


%.bin:%.lua 
	$(LUAJIT) -b -g -a $(ARCH) $< $(TARGETDIR)/app/$@


clean :
	rm -f $(addprefix $(TARGETDIR)/app/, $(appbins))
	rm -f $(addprefix obj/, $(objs))
	rm -f $(TARGETDIR)/walnuts
	rm -f walnuts

