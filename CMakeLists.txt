cmake_minimum_required(VERSION 3.16)

project(SSH-Explorer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Find libssh headers and library
find_path(LIBSSH_INCLUDE_DIR libssh/libssh.h
    PATHS ${CMAKE_PREFIX_PATH}/include
)
find_library(LIBSSH_LIBRARY ssh
    PATHS ${CMAKE_PREFIX_PATH}/lib
)

# Find OpenSSL headers and libs
find_path(OPENSSL_INCLUDE_DIR openssl/ssl.h
    PATHS ${CMAKE_PREFIX_PATH}/include
)
find_library(OPENSSL_SSL_LIBRARY ssl
    PATHS ${CMAKE_PREFIX_PATH}/lib
)
find_library(OPENSSL_CRYPTO_LIBRARY crypto
    PATHS ${CMAKE_PREFIX_PATH}/lib
)

# Find zlib
find_path(ZLIB_INCLUDE_DIR zlib.h
    PATHS ${CMAKE_PREFIX_PATH}/include
)
find_library(ZLIB_LIBRARY z
    PATHS ${CMAKE_PREFIX_PATH}/lib
)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)

    qt_add_executable(SSH-Explorer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}

        remotefilesystem.h remotefilesystem.cpp
        sshwrapper.h sshwrapper.cpp

        connectiondialog.h connectiondialog.cpp connectiondialog.ui
        connectionmanager.h connectionmanager.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET SSH-Explorer APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(SSH-Explorer SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(SSH-Explorer
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(SSH-Explorer PRIVATE
    ${LIBSSH_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

target_link_libraries(SSH-Explorer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${LIBSSH_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${ZLIB_LIBRARY}
)
target_link_libraries(SSH-Explorer PRIVATE Qt6::Core)
target_link_libraries(SSH-Explorer PRIVATE Qt6::Core)
target_link_libraries(SSH-Explorer PRIVATE Qt6::Core)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
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
