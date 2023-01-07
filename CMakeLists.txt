add_library(MicroNetworkDevice INTERFACE)
add_subdirectory(Src/MicroNetwork/Device)
target_include_directories(MicroNetworkDevice INTERFACE Src)