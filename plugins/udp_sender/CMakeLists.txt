project(udp_sender)

add_library(udp_sender MODULE main.cpp)

target_link_libraries(udp_sender PRIVATE satdump_core)

install(TARGETS udp_sender DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/satdump/plugins)