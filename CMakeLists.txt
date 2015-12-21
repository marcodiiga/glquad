cmake_minimum_required (VERSION 3.1)
project (filter)
set (CMAKE_CXX_STANDARD 11)

set (SRCS src/main.cpp)

add_executable (filter ${SRCS})

# FreeGLUT dependency
target_include_directories (filter PUBLIC deps/FreeGLUT/include)
target_link_libraries (filter debug ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Debug/x64/freeglutd.lib optimized ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Release/x64/freeglut.lib)

# Copy dlls
file (COPY  ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Debug/x64/freeglutd.dll ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Release/x64/freeglut.dll DESTINATION ${CMAKE_BINARY_DIR})
