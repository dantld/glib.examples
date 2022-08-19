cmake_minimum_required(VERSION 3.12)
project(glib-examples)
find_package(PkgConfig REQUIRED)

PKG_CHECK_MODULES(GLIB REQUIRED glib-2.0)
PKG_CHECK_MODULES(GIO REQUIRED gio-2.0)
PKG_CHECK_MODULES(GIOUNIX REQUIRED gio-unix-2.0)

include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    ${GIOUNIX_INCLUDE_DIRS}
    )

link_directories(
    ${GLIB_LIBRARY_DIRS}
    ${GIO_LIBRARY_DIRS}
    ${GIOUNIX_LIBRARY_DIRS}
    )

add_subdirectory(gio-smtp-server)
