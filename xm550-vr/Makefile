# xm510 sample Makefile
CROSS	= arm-xm-linux-
CC	= @echo " GCC	$@"; $(CROSS)gcc
CPP	= @echo " G++	$@"; $(CROSS)g++ -std=c++11
LD	= @echo " LD	$@"; $(CROSS)ld
AR	= @echo " AR	$@"; $(CROSS)ar
STRIP = @echo " STRIP	$@"; $(CROSS)strip
RM	= rm -f



TOPDIR := .
REL_LIB_PATH = $(TOPDIR)/libs
ISP_LIB_PATH = $(TOPDIR)/libs/isp
AI_LIB_PATH = $(TOPDIR)/libs/IA
THIRD_PARTY_PATH = $(TOPDIR)/libs/third-party

PKGTOOL_PATH = $(TOPDIR)/xm6xx-tools
PKGTOOL_NAME = xm6xx_pkg.sh
INSTALL = $(TOPDIR)/install

LIB_PATH = $(TOPDIR)/libs
INC_PATH = $(TOPDIR)/include
LIBSRC_PATH = $(TOPDIR)/libsrc
APPSRC_PATH = $(TOPDIR)/app
STATIC_HALLIB_NAME = libhal.a


#CFLAGS 	+= -DLOG_DEBUG
CFLAGS	+= -I$(INC_PATH)/mpp
CFLAGS	+= -I$(INC_PATH)/smart
CFLAGS	+= -I$(INC_PATH)/smart/ia
CFLAGS	+= -I$(INC_PATH)/bashi
CFLAGS	+= -I$(INC_PATH)/third-party/ZXing
CFLAGS	+= -I$(INC_PATH)/third-party/event
CFLAGS	+= -I$(INC_PATH)/ZXing-wrapper
CFLAGS	+= -I$(INC_PATH)/vr
CFLAGS 	+= -I.
CFLAGS 	+= -Wall -Os -Werror
CFLAGS 	+= -D_REENTRANT
CFLAGS 	+= -mcpu=cortex-a5 -mtune=cortex-a5 -fopenmp
LDFLAGS += -lpthread -lstdc++ -lm -ljson -gc-sections -L$(ISP_LIB_PATH)

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
MPI_LIBS += $(THIRD_PARTY_PATH)/*.a
MPI_LIBS += $(LIB_PATH)/vr/*.a


LIBSRC_FILES += $(wildcard $(LIBSRC_PATH)/*.c)
APPSRC_FILES += $(wildcard $(APPSRC_PATH)/*.c)
LIB_OBJS += $(LIBSRC_FILES:%.c=%.o)
APP_OBJS += $(APPSRC_FILES:%.c=%.o)
OBJS += $(LIB_OBJS)
OBJS += $(APP_OBJS)



%.o : %.cpp
	$(CPP) $(CFLAGS) -c -o $@  $<

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@  $<
	

TARGET := vrapp
.PHONY : clean all

all: $(TARGET) $(STATIC_HALLIB_NAME)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^  -Xlinker "-(" $(MPI_LIBS) -Xlinker "-)"
	$(STRIP) $(TARGET)
	mkdir -p $(INSTALL)
	cp $(TARGET) $(INSTALL)/
	$(PKGTOOL_PATH)/$(PKGTOOL_NAME)

$(STATIC_HALLIB_NAME): $(LIB_OBJS)
	$(AR) -rcs  $(STATIC_HALLIB_NAME) $(LIB_OBJS)

debug:
	@echo "LIBSRC_PATH: " $(LIBSRC_PATH)
	@echo "APPSRC_PATH: " $(APPSRC_PATH)
	@echo "SRCS: " $(LIBSRC_FILES) $(APPSRC_FILES)
	@echo "OBJS: " $(OBJS)
	@echo "LIBS: " $(DEP_LIBS)
	@echo "TARGET: " $(TARGET)

install:

clean:
	$(RM) $(TARGET) $(OBJS) $(STATIC_HALLIB_NAME)
	
