cmake_minimum_required (VERSION 3.1)
project (filter)
set (CMAKE_CXX_STANDARD 14)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(OpenGL REQUIRED)

set(FREEGLUT_BUILD_DEMOS OFF)
add_subdirectory (deps/FreeGLUT/freeglut/freeglut)
add_subdirectory (deps/glxw)
find_package(zlib)
if(NOT DEFINED ZLIB_LIBRARY)
  add_subdirectory (deps/zlib)
endif()
set(PNG_TESTS OFF)
add_subdirectory (deps/libpng)

include_directories(deps/FreeGLUT/freeglut/freeglut/include)
include_directories(${CMAKE_BINARY_DIR}/deps/libpng deps/libpng)
include_directories(${CMAKE_BINARY_DIR}/deps/glxw/include)

set (SRCS src/main.cpp
  src/array_view.hpp
  src/shader_utils.hpp
  src/image_utils.hpp)
  
SET(LIBRARIES freeglut glxw png16 zlib ${FREEGLUT_LIBRARIES} ${GLXW_LIBRARY} ${OPENGL_LIBRARY} ${CMAKE_DL_LIBS})

add_executable (filter ${SRCS})
target_link_libraries(filter ${LIBRARIES} )


# Copy assets
file (COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
