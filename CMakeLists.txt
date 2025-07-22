cmake_minimum_required(VERSION 3.16)

project(SSH-Explorer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(libssh CONFIG REQUIRED)


set(PROJECT_SOURCES
    main.cpp

    mainwindow.h
    mainwindow.cpp
    mainwindow.ui

    remotefilesystem.h
    remotefilesystem.cpp

    sshwrapper.h
    sshwrapper.cpp

    connectiondialog.h
    connectiondialog.cpp
    connectiondialog.ui

    connectionmanager.h
    connectionmanager.cpp

    texteditor.h
    texteditor.cpp
    texteditor.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(SSH-Explorer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        vcpkg.json
    )
else()
    if(ANDROID)
        add_library(SSH-Explorer SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(SSH-Explorer
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(SSH-Explorer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SSH-Explorer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
)

target_link_libraries(SSH-Explorer PRIVATE ssh)

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.SSH-Explorer)
endif()

set_target_properties(SSH-Explorer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS SSH-Explorer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SSH-Explorer)
endif()
