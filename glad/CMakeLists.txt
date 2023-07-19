add_library(glad gl.c)
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_target_properties(glad PROPERTIES CXX_CLANG_TIDY "")
