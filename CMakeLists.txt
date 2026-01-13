cmake_minimum_required(VERSION 3.10)

project(editor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra")

add_executable(editor main.cpp scene.cpp tools.cpp grid.cpp panda3d_imgui.cpp)
target_include_directories(editor PUBLIC "/usr/include/panda3d" "/usr/include/eigen3")
target_link_directories(editor PUBLIC "${CMAKE_SOURCE_DIR}/nativefiledialog/build/lib/Release/x64" "/usr/lib/panda3d")
target_link_libraries(editor gtk-3 p3framework panda pandaexpress p3dtool p3dtoolconfig p3direct glib-2.0 imgui nfd)
