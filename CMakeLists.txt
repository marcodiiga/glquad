cmake_minimum_required (VERSION 3.1)
project (filter)
set (CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory (${CMAKE_SOURCE_DIR}/deps/zlib-1.2.8)
set (ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/deps/zlib-1.2.8 ${CMAKE_BINARY_DIR}/deps/zlib-1.2.8)
set (ZLIB_LIBRARY_DEBUG ${CMAKE_BINARY_DIR}/deps/zlib-1.2.8/Debug/zlibd.lib)
set (ZLIB_LIBRARY_RELEASE ${CMAKE_BINARY_DIR}/deps/zlib-1.2.8/Release/zlib.lib)
add_subdirectory (${CMAKE_SOURCE_DIR}/deps/lpng1620)

set (SRCS src/main.cpp
  src/array_view.hpp
  src/shader_utils.hpp
  src/image_utils.hpp)

add_executable (filter ${SRCS})

add_definitions (-D_SCL_SECURE_NO_WARNINGS) # Suppress MSVC checked iterators warnings

# FreeGLUT dependency
target_include_directories (filter PUBLIC deps/FreeGLUT/include)
if (WIN32)
  target_link_libraries (filter debug ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Debug/x64/freeglutd.lib optimized ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Release/x64/freeglut.lib)
endif()

# GLEW dependency
target_include_directories (filter PUBLIC deps/GLEW/include)
if (WIN32)
  target_link_libraries (filter ${CMAKE_SOURCE_DIR}/deps/GLEW/bin/glew32.lib)
endif()

# libPNG dependency
target_include_directories (filter PUBLIC ${CMAKE_SOURCE_DIR}/deps/lpng1620 ${CMAKE_BINARY_DIR}/deps/lpng1620 ${ZLIB_INCLUDE_DIR})
target_link_libraries (filter png16)

# Copy dlls
if (WIN32)
  file (COPY  ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Debug/x64/freeglutd.dll ${CMAKE_SOURCE_DIR}/deps/FreeGLUT/bin/Release/x64/freeglut.dll DESTINATION ${CMAKE_BINARY_DIR})

  file (COPY  ${CMAKE_SOURCE_DIR}/deps/GLEW/bin/glew32.dll DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Copy assets
file (COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
