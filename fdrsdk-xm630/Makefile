# xm510 sample Makefile
CROSS	= arm-xm-linux-
CC	= @echo " GCC	$@"; $(CROSS)gcc
CPP	= @echo " G++	$@"; $(CROSS)g++ -std=c++11
LD	= @echo " LD	$@"; $(CROSS)ld
AR	= @echo " AR	$@"; $(CROSS)ar
RM	= rm -f



TOPDIR := .
REL_LIB_PATH = $(TOPDIR)/libs
ISP_LIB_PATH = $(TOPDIR)/libs/isp
AI_LIB_PATH = $(TOPDIR)/libs/IA
THIRD_PARTY_PATH = $(TOPDIR)/libs/third-party

SDKINC_PATH = $(TOPDIR)/include
SDKSRC_PATH = $(TOPDIR)/src

STATIC_SDKLIB_NAME = libfdr_sdk.a
SHARED_SDKLIB_NAME = libfdr_sdk.so

CFLAGS	+= -I$(SDKINC_PATH)/mpp
CFLAGS	+= -I$(SDKINC_PATH)/smart
CFLAGS	+= -I$(SDKINC_PATH)/smart/ia
CFLAGS	+= -I$(SDKINC_PATH)/bashi
CFLAGS	+= -I$(SDKINC_PATH)/third-party/ZXing
CFLAGS	+= -I$(SDKINC_PATH)/ZXing-wrapper
CFLAGS 	+= -I.
CFLAGS 	+= -Wall -Os
CFLAGS 	+= -D_REENTRANT 
CFLAGS 	+= -DXM650
CFLAGS 	+= -mcpu=cortex-a5 -mtune=cortex-a5 -fopenmp
LDFLAGS += -ljson -lpthread -lstdc++ -lm -gc-sections -L./libs/isp

MPI_LIBS += $(REL_LIB_PATH)/*.a
MPI_LIBS += $(ISP_LIB_PATH)/*.a
MPI_LIBS += $(AI_LIB_PATH)/libxmia.a
MPI_LIBS += $(AI_LIB_PATH)/libfacnn.a
MPI_LIBS += $(AI_LIB_PATH)/libfdr.a
MPI_LIBS += $(AI_LIB_PATH)/libfpcnn.a
MPI_LIBS += $(AI_LIB_PATH)/libfr.a
MPI_LIBS += $(AI_LIB_PATH)/libstf.a
MPI_LIBS += $(AI_LIB_PATH)/libxmia_fd_cnn.a
MPI_LIBS += $(AI_LIB_PATH)/libxmia_comm.a
MPI_LIBS += $(THIRD_PARTY_PATH)/libZXingCore.a
MPI_LIBS += $(REL_LIB_PATH)/$(STATIC_SDKLIB_NAME)


# target source
#SRC += $(wildcard $(SRC_PATH)/*.c)
SRC += $(TOPDIR)/main.c
SDKLIB_SRC += $(wildcard $(SDKSRC_PATH)/*.c)
SDKLIB_SRC_CPP += $(wildcard $(TOPDIR)/third-party/ZXing/*.cpp)

OBJ += $(SRC:%.c=%.o)
SDKLIB_OBJ += $(SDKLIB_SRC:%.c=%.o)
SDKLIB_OBJ += $(SDKLIB_SRC_CPP:%.cpp=%.o)


%.o : %.cpp
	$(CPP) $(CFLAGS) -c -o $@  $<

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@  $<
	
$(STATIC_SDKLIB_NAME): $(SDKLIB_OBJ)
	$(AR) -rcs  $(STATIC_SDKLIB_NAME) $(SDKLIB_OBJ)
	cp $(STATIC_SDKLIB_NAME) $(REL_LIB_PATH)/

TARGET := fdr_demo
.PHONY : clean all

all: $(TARGET) $(STATIC_SDKLIB_NAME)

$(TARGET):$(OBJ) $(SDKLIB_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^  -Xlinker "-(" $(MPI_LIBS) -Xlinker "-)"

debug:
	@echo $(SRC)
	@echo $(SDKLIB_SRC)
	@echo $(SDKSRC_PATH)
	@echo $(OBJ)
	@echo $(SDKLIB_OBJ)
	
clean:
	$(RM) $(TARGET)
	$(RM) $(OBJ) $(SDKLIB_OBJ)
	
