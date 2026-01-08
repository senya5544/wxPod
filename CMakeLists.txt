cmake_minimum_required(VERSION 3.10.0)
project(wxPod VERSION 0.1.0 LANGUAGES C CXX)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

find_package(wxWidgets COMPONENTS base core REQUIRED)
find_package(ZLIB)
find_package(TagLib 2.1.1 REQUIRED)

include_directories(
    "/usr/include/gpod-1.0/gpod"
    ${GLIB_INCLUDE_DIRS}
)

add_executable(wxPod
    src/main.cpp
    src/device.cpp
    src/tracklist.cpp
    src/filters.cpp
    src/properties.cpp
)

target_link_libraries(wxPod
    wxWidgets::wxWidgets
    TagLib::tag
    udev
    gpod
    ${GLIB_LIBRARIES}
)

