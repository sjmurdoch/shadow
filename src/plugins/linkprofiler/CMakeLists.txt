## link profiler server/client plug-in

## executable that can run outside of shadow
add_executable(shadow-linkprofiler shd-linkprofiler-main.c shd-linkprofiler-udp.c)
target_link_libraries(shadow-linkprofiler ${GLIB_LIBRARIES})
install(TARGETS shadow-linkprofiler DESTINATION bin)

## create and install a shared library that can plug into shadow
add_bitcode(shadow-plugin-linkprofiler-bitcode 
    shd-linkprofiler-plugin.c shd-linkprofiler-udp.c)
add_plugin(shadow-plugin-linkprofiler shadow-plugin-linkprofiler-bitcode)
target_link_libraries(shadow-plugin-linkprofiler ${GLIB_LIBRARIES})
install(TARGETS shadow-plugin-linkprofiler DESTINATION plugins)
