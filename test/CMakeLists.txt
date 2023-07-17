# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(imgv-cppTests LANGUAGES CXX)

# ---- Tests ----

add_executable(imgv-cpp_test source/imgv-cpp_test.cpp)
target_link_libraries(imgv-cpp_test PRIVATE imgv-cpp_lib)
target_compile_features(imgv-cpp_test PRIVATE cxx_std_17)

add_test(NAME imgv-cpp_test COMMAND imgv-cpp_test)

# ---- End-of-file commands ----

add_folders(Test)
