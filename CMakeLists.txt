cmake_minimum_required(VERSION 3.16)

project(TestLowPoly04 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/i18n/translation/TestLowPoly04_zh_CN.ts)

set(PROJECT_SOURCES
    main.cpp
    resources.qrc
    ${SRC_DIR}/ui/widget.cpp
    ${SRC_DIR}/ui/widget.h
    ${SRC_DIR}/ui/widget.ui

    ${SRC_DIR}/core/image_controller.h
    ${SRC_DIR}/core/image_controller.cpp
    ${SRC_DIR}/core/image_model.h
    ${SRC_DIR}/core/image_model.cpp
    ${SRC_DIR}/core/image_widget.h
    ${SRC_DIR}/core/image_widget.cpp

    ${SRC_DIR}/algorithm/edge_drawing_gray.h
    ${SRC_DIR}/algorithm/edge_drawing_gray.cpp
    ${SRC_DIR}/algorithm/edge_drawing_color.h
    ${SRC_DIR}/algorithm/edge_drawing_color.cpp

    ${SRC_DIR}/utils/logger.h
    ${SRC_DIR}/utils/logger.cpp

    ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(TestLowPoly04
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        src/ui/base_dialog.h
        src/ui/ed_param_dialog.h
        src/ui/ed_param_dialog.cpp
        src/ui/ed_param_dialog.ui
        src/utils/EDParams_util.h
        src/utils/EDParams_util.cpp





    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET TestLowPoly04 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES
        ${CMAKE_SOURCE_DIR}
        ${TS_FILES}
    )

else()
    if(ANDROID)
        add_library(TestLowPoly04 SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(TestLowPoly04
            ${PROJECT_SOURCES}
        )
    endif()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

# target_include_directories: 将头文件搜索路径添加到指定的目标
# PRIVATE: 表示这些目录只会在编译目标时使用，其他依赖于该目标的目标不会受到影响
target_include_directories(TestLowPoly04 PRIVATE
    ${SRC_DIR}/core
    ${SRC_DIR}/ui
    ${SRC_DIR}/algorithm
    ${SRC_DIR}/utils
    ${CMAKE_SOURCE_DIR}/exlib
)

target_link_libraries(TestLowPoly04 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    "${CMAKE_CURRENT_SOURCE_DIR}/exlib/Dll2.lib"
)

# # cuda external library
# target_include_directories(TestLowPoly04 PRIVATE
#     "C:/Users/h7286/Desktop/exlib"
# )
# target_link_libraries(TestLowPoly04 PRIVATE
#     "C:/Users/h7286/Desktop/exlib/Dll1.lib"
# )




# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.TestLowPoly04)
endif()
set_target_properties(TestLowPoly04 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS TestLowPoly04
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(TestLowPoly04)
endif()
