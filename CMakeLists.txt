cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    imgv-cpp
    VERSION 0.1.0
    DESCRIPTION "media viewer based on libmpv"
    HOMEPAGE_URL "https://github.com/btmxh/imgv-cpp"
    LANGUAGES C CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# ---- Dependencies ----

find_package(glfw3 REQUIRED)
find_package(nfd REQUIRED)
find_package(Freetype REQUIRED)
find_package(fmt REQUIRED)
find_package(Boxer REQUIRED)
find_package(libmpv REQUIRED)
find_package(GIF REQUIRED)
find_package(WebP REQUIRED)

## EasyGifReader (Build from source since it's a small dependency)
include(FetchContent)

FetchContent_Declare(
  EasyGifReader
  GIT_REPOSITORY https://github.com/Chlumsky/EasyGifReader
)

FetchContent_MakeAvailable(EasyGifReader)

FetchContent_GetProperties(
  EasyGifReader
  SOURCE_DIR EasyGifReader_SOURCE_DIR
)

add_library(EasyGifReader
  "${EasyGifReader_SOURCE_DIR}/EasyGifReader.cpp"
)

target_include_directories(EasyGifReader PUBLIC ${EasyGifReader_SOURCE_DIR})
target_link_libraries(EasyGifReader PRIVATE GIF::GIF)
set_target_properties(EasyGifReader PROPERTIES CXX_CLANG_TIDY "")

add_subdirectory(glad)

# ---- Declare library ----

add_library(
  imgv-cpp_lib OBJECT
  source/context.cpp
  source/events.cpp
  source/root_window.cpp
  source/mpv_window.cpp
  source/window.cpp
)

target_include_directories(
    imgv-cpp_lib ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(imgv-cpp_lib PUBLIC cxx_std_17)

target_link_libraries(imgv-cpp_lib PUBLIC
  glad
  EasyGifReader
  glfw
  nfd
  libmpv::libmpv
  Freetype::Freetype
  fmt::fmt
  Boxer::Boxer
)

# ---- Declare executable ----

add_executable(imgv-cpp_exe
  source/main.cpp
)
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
