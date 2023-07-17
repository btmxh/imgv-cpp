install(
    TARGETS imgv-cpp_exe
    RUNTIME COMPONENT imgv-cpp_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
