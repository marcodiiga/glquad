cmake_minimum_required (VERSION 3.1)
project (filter)
set (CMAKE_CXX_STANDARD 14)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

find_package(OpenGL REQUIRED)

set(FREEGLUT_BUILD_DEMOS OFF)
add_subdirectory (deps/FreeGLUT/freeglut/freeglut)
add_subdirectory (deps/glxw)
find_package(zlib)
# If ZLIB_LIBRARY is not defined or it is set as ZLIB_LIBRARY-NOTFOUND
if(NOT DEFINED ZLIB_LIBRARY OR NOT ZLIB_LIBRARY)
  message(STATUS "Zlib not installed, using local copy")
  add_subdirectory (deps/zlib)
  if(WIN32)
    set(ZLIB_LIBRARY optimized;${CMAKE_BINARY_DIR}/bin/zlib.lib;debug;${CMAKE_BINARY_DIR}/bin/zlibd.lib)
  endif()
  set(ZLIB_INCLUDE_DIR deps/zlib ${CMAKE_BINARY_DIR}/deps/zlib)
  include_directories(${ZLIB_INCLUDE_DIR})
else()
  message(STATUS "Zlib found: " ${ZLIB_LIBRARY})
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
  
SET(LIBRARIES freeglut glxw png16 ${FREEGLUT_LIBRARIES} ${GLXW_LIBRARY} ${OPENGL_LIBRARY} ${CMAKE_DL_LIBS})

if(MSVC)
  add_definitions (-D_SCL_SECURE_NO_WARNINGS) # Suppress MSVC checked iterators warnings
endif()

add_executable (filter ${SRCS})
target_link_libraries(filter ${LIBRARIES} )


# Copy assets
file (COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
