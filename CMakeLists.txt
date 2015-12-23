cmake_minimum_required (VERSION 3.1)
project (filter)
set (CMAKE_CXX_STANDARD 11)

find_package(OpenGL REQUIRED)

# Put all binaries in the /bin dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
add_subdirectory(deps/glfw)

add_subdirectory(deps/glxw)

add_subdirectory(deps/zlib)
set(ZLIB_LIBRARY debug;${CMAKE_BINARY_DIR}/deps/zlib/Debug/zlibd.lib;optimized;${CMAKE_BINARY_DIR}/deps/zlib/Release/zlib.lib)
set (ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/deps/zlib ${CMAKE_BINARY_DIR}/deps/zlib)
add_subdirectory(deps/libpng)

include_directories(deps/glfw/include)
include_directories(${CMAKE_BINARY_DIR}/deps/glxw/include)
include_directories(deps/libpng ${CMAKE_BINARY_DIR}/deps/libpng ${ZLIB_INCLUDE_DIR})

set(LIBRARIES glfw glxw png16 ${GLFW_LIBRARIES} ${GLXW_LIBRARY} ${OPENGL_LIBRARY} ${CMAKE_DL_LIBS})

link_directories (${OPENGLEXAMPLES_BINARY_DIR}/bin)

if(MSVC)
  add_definitions (-D_SCL_SECURE_NO_WARNINGS) # Suppress MSVC checked iterators warnings
endif()

set (SRCS src/main.cpp
          src/array_view.hpp
          src/shader_utils.hpp
          src/image_utils.hpp)


add_executable (filter ${SRCS})

target_link_libraries(filter ${LIBRARIES})


# Copy assets
file (COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
