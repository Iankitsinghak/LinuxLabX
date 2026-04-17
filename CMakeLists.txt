cmake_minimum_required(VERSION 3.21)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" LINUXLABX_VERSION_RAW)
string(STRIP "${LINUXLABX_VERSION_RAW}" LINUXLABX_VERSION)
if (NOT LINUXLABX_VERSION)
    set(LINUXLABX_VERSION "0.1.0")
endif()

project(LinuxLabX
    VERSION ${LINUXLABX_VERSION}
    DESCRIPTION "LinuxLabX beginner-friendly virtual Linux terminal"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

string(TIMESTAMP LINUXLABX_BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS deployment target" FORCE)
endif()

if (APPLE)
    find_program(QMAKE_EXECUTABLE NAMES qmake qmake6)
    if (QMAKE_EXECUTABLE)
        execute_process(
            COMMAND ${QMAKE_EXECUTABLE} -query QT_INSTALL_LIBS
            OUTPUT_VARIABLE QT_INSTALL_LIBS_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        set(QT_CORE_FRAMEWORK "${QT_INSTALL_LIBS_DIR}/QtCore.framework/QtCore")
        if (EXISTS "${QT_CORE_FRAMEWORK}")
            execute_process(
                COMMAND file "${QT_CORE_FRAMEWORK}"
                OUTPUT_VARIABLE QT_CORE_FILE_INFO
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            if (QT_CORE_FILE_INFO MATCHES "x86_64" AND NOT QT_CORE_FILE_INFO MATCHES "arm64")
                message(STATUS "Qt appears to be x86_64-only; forcing CMAKE_OSX_ARCHITECTURES=x86_64")
                set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architecture inferred from Qt" FORCE)
            endif()
        endif()
    endif()
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets Network QUIET)
if (Qt6_FOUND)
    set(QT_LIBS Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)
else()
    find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
    set(QT_LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/Version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
    @ONLY
)

set(APP_ICON_MACOS "${CMAKE_SOURCE_DIR}/assets/icon.icns")
set(APP_ICON_WINDOWS "${CMAKE_SOURCE_DIR}/assets/icon.ico")

set(LINUXLABX_SOURCES
    src/main.cpp
    src/parser.cpp
    src/command_registry.cpp
    src/errors.cpp
    src/filesystem.cpp
    src/executor.cpp
    src/suggestions.cpp
    src/ui.cpp
    src/logging.cpp
)

if (APPLE AND EXISTS "${APP_ICON_MACOS}")
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND LINUXLABX_SOURCES ${APP_ICON_MACOS})
endif()

if (WIN32 AND EXISTS "${APP_ICON_WINDOWS}")
    set(WINDOWS_ICON_PATH "${APP_ICON_WINDOWS}")
    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/windows-icon.rc.in"
        "${CMAKE_BINARY_DIR}/generated/windows-icon.rc"
        @ONLY
    )
    list(APPEND LINUXLABX_SOURCES "${CMAKE_BINARY_DIR}/generated/windows-icon.rc")
endif()

add_executable(LinuxLabX ${LINUXLABX_SOURCES})

set_target_properties(LinuxLabX PROPERTIES
    OUTPUT_NAME "LinuxLabX"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "LinuxLabX"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.linuxlabx.app"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    WIN32_EXECUTABLE TRUE
)

if (APPLE AND EXISTS "${APP_ICON_MACOS}")
    set_target_properties(LinuxLabX PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "icon.icns"
    )
endif()

target_include_directories(LinuxLabX
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(LinuxLabX PRIVATE ${QT_LIBS})
target_compile_definitions(LinuxLabX PRIVATE LINUXLABX_HAS_QT_WIDGETS=1)

install(TARGETS LinuxLabX
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif (WIN32)
    set(CPACK_GENERATOR "NSIS")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
endif()

set(CPACK_PACKAGE_NAME "LinuxLabX")
set(CPACK_PACKAGE_VENDOR "LinuxLabX")
set(CPACK_PACKAGE_CONTACT "maintainers@linuxlabx.example")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LinuxLabX virtual Linux learning terminal")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "LinuxLabX-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")

set(CPACK_NSIS_DISPLAY_NAME "LinuxLabX")
set(CPACK_NSIS_PACKAGE_NAME "LinuxLabX")
if (EXISTS "${CMAKE_SOURCE_DIR}/assets/icon.ico")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/icon.ico")
endif()

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "LinuxLabX")
set(CPACK_DEBIAN_PACKAGE_SECTION "education")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

include(CPack)
