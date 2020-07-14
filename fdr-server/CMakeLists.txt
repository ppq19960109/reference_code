cmake_minimum_required(VERSION 3.10)
project(fdrserver)
#set(CMAKE_C_STANDARD 11)#C11
#set(CMAKE_CXX_STANDARD 17)#C17
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER   arm-xm-linux-gcc)
set(CMAKE_CXX_COMPILER arm-xm-linux-g++)

set(CMAKE_C_FLAGS "-Wall -Werror -O2 -s -fsigned-char -fopenmp")
set(CMAKE_CXX_FLAGS " -Wall -Werror -O2 -s -fsigned-char -fopenmp")
add_definitions(-D_GLIBCXX_USE_C99=1)

set(TARGET_NAME fdrserver)
add_definitions(-DXM6XX)
add_definitions(-DXM650)
add_definitions(-DUI_LANG=0)
add_definitions(-D_GNU_SOURCE -D__USE_XOPEN)

# LCD_SIZE_43_INCH    0
# LCD_SIZE_50_INCH    1
# LCD_SIZE_70_INCH    2
add_definitions(-DLCD_SIZE_TYPE=LCD_SIZE_50_INCH)

#TEST MACRO
#add_definitions(-DTEST_VERSION)

#add_definitions(-DZHONGTIE)



#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

#find_package(SDL2 REQUIRED SDL2)
#include_directories(${SDL2_INCLUDE_DIRS})
link_directories("/home/heyan/Downloads/SDL2-2.0.10/_install/lib")
link_directories("/home/heyan/Downloads/freetype-2.10.1/_install/lib")
link_directories("/home/heyan/Downloads/zlib-1.2.11/_install/lib")
link_directories("/home/heyan/git_workspace/fdrsdk-xm630/libs")
link_directories("/home/heyan/git_workspace/fdrsdk-xm630/libs/isp")
link_directories("/home/heyan/git_workspace/fdrsdk-xm630/libs/IA")
link_directories("/home/heyan/git_workspace/fdrsdk-xm630/libs/third-party")


include_directories("/home/heyan/Downloads/SDL2-2.0.10/_install/include")
include_directories("/home/heyan/Downloads/freetype-2.10.1/_install/include/freetype2")
include_directories("/home/heyan/Downloads/zlib-1.2.11/_install/include")
include_directories("/home/heyan/git_workspace/fdrsdk-xm630/include/bashi")
include_directories("/home/heyan/git_workspace/fdrsdk-xm630/include/mpp")
include_directories("/home/heyan/git_workspace/fdrsdk-xm630/include/smart")
include_directories("/home/heyan/git_workspace/fdrsdk-xm630/include/smart/ia")
include_directories("/home/heyan/git_workspace/fdrsdk-xm630/include/third-party")
include_directories("/home/heyan/git_workspace/fdrsdk-xm630/include/third-party/ZXing")
include_directories("/home/heyan/git_workspace/fdrsdk-xm630/include/third-party/event")

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/thirdparts/misc)

file(GLOB_RECURSE INCLUDES "include/*.h"  "thirdparts/misc/*.h")

file(GLOB_RECURSE SOURCES
	"src/bisp-xm6xx.c"
	"src/fdr.c"
	"src/fdr-cp.c"
	"src/fdr-cp-xmui.c"
	"src/fdr-dbs.c"
	"src/fdr-dbs-config.c"
	"src/fdr-dbs-feature.c"
	"src/fdr-dbs-guest.c"
	"src/fdr-dbs-user.c"
	"src/fdr-discover.c"
	"src/fdr-license.c"
	"src/fdr-handler.c"
	"src/fdr-handler-logger.c"
	"src/fdr-handler-user.c"
	"src/fdr-logger.c"
	"src/fdr-mp.c"
	"src/fdr-mp-cloud.c"
	"src/fdr-mp-local.c"
	"src/fdr-network.c"
	"src/fdr-qrcode.c"
	"src/fdr-util.c"
	"src/fdr-version.c"
	"src/logger.c"
	)

file(GLOB_RECURSE MISC
	"thirdparts/misc/base58.c"
	"thirdparts/misc/cJSON.c"
	"thirdparts/misc/ecc.c"
	"thirdparts/misc/lodepng.c"
	"thirdparts/misc/sha256.c"
	"thirdparts/misc/sqlite3.c"
	)

set(XM650_LIBS
	libh264.a
	libintel.a
	libmpi.a
	libmve_instr.a
	libmve_kwrap.a
	libmve_log.a
	libmve_mem.a
	libmveomxunit.a
	libmveutils.a
	libsmart.a
	libVenc.a
	libxmaudio3a.a
	libae.a
	libawb.a
	libfvideo.a
	libisp.a
	libispapp_XM530.a
	libjson.so
	libphyvi_XM530.a
	libsns_X123_XM530.a
	libvenctx_XM530.a
	libxmisp_XM530.a
)

# do not change the link sequence
set(XMIA_LIBS
	libxmia.a
	libfdr.a
	libfr.a
	libfacnn.a
	libfpcnn.a
	libstf.a
	libxmia_fd_cnn.a
	libxmia_comm.a
)

set(FDRSDK_LIBS bsm650fv100.a)
set(FREETYPE2_LIBS libfreetype.a)
set(ZLIB_LIBS libz.a)
set(ZXING_LIBS libZXingCore.a)
set(EVENT_LIBS libevent.a libevent_core.a libevent_extra.a libevent_pthreads.a)

set(TARGET_LIBS
	${FDRSDK_LIBS}
	${XM650_LIBS}
	${XMIA_LIBS}
	${FDRSDK_LIBS}
	${FREETYPE2_LIBS}
	${ZLIB_LIBS}
	${ZXING_LIBS}
	${EVENT_LIBS}
	misc
	fdr
	stdc++
	pthread
	json
	dl
	m)

add_library(misc STATIC ${MISC} ${INCLUDES})
add_library(fdr STATIC ${SOURCES} ${INCLUDES})
#add_library(fdr SHARED ${SOURCES} ${INCLUDES})
add_executable(${TARGET_NAME} src/main.c)
target_link_libraries(${TARGET_NAME} PRIVATE -Wl,--start-group ${TARGET_LIBS} -Wl,--end-group)

install(TARGETS ${TARGET_NAME} misc fdr
  RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/install
  ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/libs
  LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/libs
)
