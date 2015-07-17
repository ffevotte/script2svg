cmake_minimum_required (VERSION 2.6)
project (script2svg)
add_executable (script2svg main.cxx)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

string (REPLACE ":" ";" LD_RUN_PATH $ENV{LD_RUN_PATH})
foreach (path IN LISTS LD_RUN_PATH)
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,${path}")
endforeach (path)


# tsm
find_path (TSM_INCLUDE_DIR libtsm.h
  HINTS ENV CPATH)

find_library (TSM_LIBRARY tsm
  HINTS ENV LIBRARY_PATH)

if (TSM_INCLUDE_DIR)
  if (TSM_LIBRARY)
    set (TSM_FOUND "YES")
  endif (TSM_LIBRARY)
endif (TSM_INCLUDE_DIR)

if (NOT TSM_FOUND)
  message (SEND_ERROR "libtsm not found.")
endif (NOT TSM_FOUND)

message (STATUS "Found libtsm:")
message (STATUS "  include dir: ${TSM_INCLUDE_DIR}")
message (STATUS "  library:     ${TSM_LIBRARY}")
include_directories (${TSM_INCLUDE_DIR})
target_link_libraries (script2svg ${TSM_LIBRARY})
