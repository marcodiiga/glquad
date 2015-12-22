cmake_minimum_required (VERSION 3.1)
project (filter)
set (CMAKE_CXX_STANDARD 11)

set (SRCS src/main.cpp
          src/array_view.hpp
          src/shader_utils.hpp)

add_executable (filter ${SRCS})

add_definitions(-D_SCL_SECURE_NO_WARNINGS) # Suppress MSVC checked iterators warnings

# FreeGLUT dependency
target_include_directories (filter PUBLIC deps/FreeGLUT/include)
target_link_libraries (filter debug ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Debug/x64/freeglutd.lib optimized ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Release/x64/freeglut.lib)

# GLEW dependency
target_include_directories (filter PUBLIC deps/GLEW/include)
target_link_libraries (filter ${CMAKE_SOURCE_DIR}/deps/GLEW/bin/glew32.lib)

# Copy dlls
file (COPY  ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Debug/x64/freeglutd.dll ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Release/x64/freeglut.dll DESTINATION ${CMAKE_BINARY_DIR})

file (COPY  ${CMAKE_SOURCE_DIR}/deps/GLEW/bin/glew32.dll DESTINATION ${CMAKE_BINARY_DIR})
