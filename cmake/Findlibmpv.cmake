find_package(PkgConfig QUIET)

pkg_search_module(PKGCONFIG_LIBMPV QUIET mpv)

set(libmpv_VERSION ${PKGCONFIG_LIBMPV_VERSION})

find_path(libmpv_INCLUDE_DIRS
    NAMES client.h
    PATH_SUFFIXES mpv
    HINTS ${PKGCONFIG_LIBMPV_INCLUDEDIR}
)

find_library(libmpv_LIBRARIES
    NAMES mpv
    HINTS ${PKGCONFIG_LIBMPV_LIBRARIES}
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    libmpv
    FOUND_VAR libmpv_FOUND
    REQUIRED_VARS libmpv_LIBRARIES libmpv_INCLUDE_DIRS
    VERSION_VAR libmpv_VERSION
)

if(libmpv_FOUND AND NOT TARGET libmpv::libmpv)
    add_library(libmpv::libmpv UNKNOWN IMPORTED libmpv)
    add_library(libmpv ALIAS libmpv::libmpv)
    set_target_properties(libmpv::libmpv PROPERTIES
        IMPORTED_LOCATION "${libmpv_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${libmpv_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(libmpv_LIBRARIES libmpv_LIBRARIES)

include(FeatureSummary)
set_package_properties(libmpv PROPERTIES
    URL "https://mpv.io/"
    DESCRIPTION "interfacing API to the mpv media player"
)
