cmake_minimum_required(VERSION 3.16)

project(VedioPlay VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)
# FFmpeg 路径设置
set(FFMPEG_DIR "E:/lib/ffmpeg5-1-2")
include_directories(${FFMPEG_DIR}/include)
link_directories(${FFMPEG_DIR}/lib)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(VedioPlay
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        readthread.h readthread.cpp
        videodecoder.h videodecoder.cpp
        playimage.h playimage.cpp

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET VedioPlay APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(VedioPlay SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(VedioPlay
            ${PROJECT_SOURCES}
        )
    endif()
endif()
# set(FFMPEG_DIR "E:/lib/ffmpeg5-1-2")
# include_directories(${FFMPEG_DIR}/include)
# link_directories(${FFMPEG_DIR}/lib)

target_link_libraries(VedioPlay PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
    avcodec
    avfilter
    avformat
    swscale
    avutil
    swresample
    avdevice
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.VedioPlay)
endif()
set_target_properties(VedioPlay PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS VedioPlay
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(VedioPlay)
endif()
