cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    imgv-cpp
    VERSION 0.1.0
    DESCRIPTION "media viewer based on libmpv"
    HOMEPAGE_URL "https://github.com/btmxh/imgv-cpp"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    imgv-cpp_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    imgv-cpp_lib ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(imgv-cpp_lib PUBLIC cxx_std_17)

# ---- Declare executable ----

add_executable(imgv-cpp_exe source/main.cpp)
add_executable(imgv-cpp::exe ALIAS imgv-cpp_exe)

set_property(TARGET imgv-cpp_exe PROPERTY OUTPUT_NAME imgv-cpp)

target_compile_features(imgv-cpp_exe PRIVATE cxx_std_17)

target_link_libraries(imgv-cpp_exe PRIVATE imgv-cpp_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT imgv-cpp_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of imgv-cpp"
  )
endif()

include(cmake/dev-mode.cmake)
