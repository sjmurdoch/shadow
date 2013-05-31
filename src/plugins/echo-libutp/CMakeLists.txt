## example echo server/client plug-in

find_package(RT REQUIRED)
find_package(DL REQUIRED)
find_package(GLIB REQUIRED)

get_filename_component(CURRENT_LIBUTP_DIR "${CMAKE_BINARY_DIR}/../libutp" ABSOLUTE)
include_directories(${CURRENT_LIBUTP_DIR})
include_directories(${CURRENT_LIBUTP_DIR}/utp_config_lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPOSIX -fno-exceptions -fno-rtti -Wall -g")

## executable that can run outside of shadow
add_executable(shadow-echo-libutp shd-echo-libutp-main.c shd-echo-libutp-tcp.c
    shd-echo-libutp-udp.c shd-echo-libutp-utp.c shd-echo-libutp-pipe.c
    ${CURRENT_LIBUTP_DIR}/utp.cpp ${CURRENT_LIBUTP_DIR}/utp_utils.cpp)
target_link_libraries(shadow-echo-libutp ${GLIB_LIBRARIES} ${RT_LIBRARIES} ${DL_LIBRARIES})
install(TARGETS shadow-echo-libutp DESTINATION bin)

## create and install a shared library that can plug into shadow
add_bitcode(shadow-plugin-echo-libutp-bitcode 
    shd-echo-libutp-plugin.c shd-echo-libutp-tcp.c shd-echo-libutp-udp.c
    shd-echo-libutp-utp.c shd-echo-libutp-pipe.c
    ${CURRENT_LIBUTP_DIR}/utp.cpp ${CURRENT_LIBUTP_DIR}/utp_utils.cpp)
add_plugin(shadow-plugin-echo-libutp shadow-plugin-echo-libutp-bitcode)
target_link_libraries(shadow-plugin-echo-libutp ${GLIB_LIBRARIES} ${RT_LIBRARIES} ${DL_LIBRARIES})
install(TARGETS shadow-plugin-echo-libutp DESTINATION plugins)
