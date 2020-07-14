cmake_minimum_required(VERSION 3.10)
project(fdrsdk)
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

set(TARGET_NAME fdrsdk)

add_definitions(-D_GNU_SOURCE -D__USE_XOPEN)


#CONFIG_BSBOARD
# BSM650FV100
# BSM650FV101
# BSM650LV100
add_definitions(-DCONFIG_BSBOARD=BSM650FV100)

# "bsm650fv100" or "bsm650fv101" or "bsm650lv100"
set(FDRSDK_LIB "bsm650fv100")

# LCD_SIZE_43_INCH
# LCD_SIZE_50_INCH
# LCD_SIZE_70_INCH
add_definitions(-DLCD_SIZE_TYPE=LCD_SIZE_50_INCH)

# VO_COLOR_MODE
# VO_BLACK_MODE
# VO_NONE_MODE
add_definitions(-DVO_MODE=VO_COLOR_MODE)

#add_definitions(-DTEMP_TEST)


#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin) 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)


link_directories("${PROJECT_SOURCE_DIR}/libs")
link_directories("${PROJECT_SOURCE_DIR}/libs/isp")
link_directories("${PROJECT_SOURCE_DIR}/libs/IA")
link_directories("${PROJECT_SOURCE_DIR}/libs/third-party")

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories("${PROJECT_SOURCE_DIR}/include/bashi")
include_directories("${PROJECT_SOURCE_DIR}/include/mpp")
include_directories("${PROJECT_SOURCE_DIR}/include/smart")
include_directories("${PROJECT_SOURCE_DIR}/include/smart/ia")
include_directories("${PROJECT_SOURCE_DIR}/include/third-party")
include_directories("${PROJECT_SOURCE_DIR}/include/third-party/ZXing")
include_directories("${PROJECT_SOURCE_DIR}/include/third-party/event")
include_directories("${PROJECT_SOURCE_DIR}/include/ZXing-wrapper")

file(GLOB_RECURSE INCLUDES "include/*.h")

file(GLOB_RECURSE SOURCES "src/*.c" "third-party/ZXing/*.cpp")


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

set(ZXING_LIBS libZXingCore.a)


set(TARGET_LIBS 
	${XM650_LIBS}
	${XMIA_LIBS}
	${ZXING_LIBS}
	${EVENT_LIBS}
    ${FDRSDK_LIB}
	stdc++
	pthread
	json
	dl
	m)


add_library(${FDRSDK_LIB} STATIC ${SOURCES} ${INCLUDES})
#add_library(${FDRSDK_LIB} SHARED ${SOURCES} ${INCLUDES})
add_executable(A1050 A1050.c)
add_executable(A1000 A1000.c)
target_link_libraries(A1050 PRIVATE -Wl,--start-group ${TARGET_LIBS} -Wl,--end-group)
target_link_libraries(A1000 PRIVATE -Wl,--start-group ${TARGET_LIBS} -Wl,--end-group)

install(TARGETS ${FDRSDK_LIB}
  RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/install
  ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/libs
  LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/libs
)
