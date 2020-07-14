
CROSS = 
ARCH = amd64

CC = $(CROSS)gcc

vpath 	%.h 	../include 
vpath 	%.c 	../src ../deps/misc
vpath 	%.o 	../objs

LVGL = ../deps/littlevgl-6.0

INCLUDE = -I. -I../include -I../deps/misc -I$(LVGL) -I./include -I$(LVGL)/lvgl

CFLAGS = -g -O2 -Wall $(INCLUDE)

LIBS = -llvgl -pthread -levent_pthreads -levent -ldl -lm  -lSDL2  -lfreetype 

LDFLAGS =  -L$(LVGL)  -L$(LVGL) $(LIBS)

genkey-objs =               \
		ecc.o				\
		genkey.o

x2bmp-objs = 				\
		lodepng.o 			\
		lvgui-yuv.o			\
		x2bmp.o

TARGET = genkey x2bmp

%.o : %.c
	$(CC) $(CFLAGS) -c -o ../objs/$@  $<

all: $(TARGET)  $(genkey-objs) $(x2bmp-objs)

genkey : $(genkey-objs)
x2bmp : $(x2bmp-objs)

$(TARGET) : $(genkey-objs) $(x2bmp-objs)
	$(CC)  -o genkey  $(addprefix ../objs/, $(genkey-objs)) $(LDFLAGS)
	$(CC)  -o x2bmp  $(addprefix ../objs/, $(x2bmp-objs)) $(LDFLAGS)

clean:
	rm -f $(addprefix ../objs/, $(genkey-objs) $(x2bmp-objs) $(TARGET))
	rm -f $(TARGET)


lv_gc.o :
	cp $(LVGL)/lv_gc.o  ../objs/lv_gc.o
