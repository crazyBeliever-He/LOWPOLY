cmake_minimum_required(VERSION 3.16)

project(TestLowPoly04 VERSION 0.1 LANGUAGES CXX)
# cmake原生支持cuda，所以只要用cmake + MSVC编译器，即可简单实现在qt中使用cuda
#project(TestLowPoly04 VERSION 0.1 LANGUAGES CXX CUDA)
#set(CMAKE_CUDA_ARCHITECTURES "native")

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

    ${SRC_DIR}/utils/logger.h
    ${SRC_DIR}/utils/logger.cpp

    ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(TestLowPoly04
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        src/ui/base_dialog.h
        src/ui/edge_drawing_dialog.h
        src/ui/edge_drawing_dialog.cpp
        src/ui/edge_drawing_dialog.ui
        src/utils/param_util.h
        src/utils/param_util.cpp
        src/algorithm/douglas_peucker.h
        src/algorithm/douglas_peucker.cpp
        src/ui/douglas_peucker_dialog.h
        src/ui/douglas_peucker_dialog.cpp
        src/ui/douglas_peucker_dialog.ui
        src/algorithm/algorithm_params.h
        src/algorithm/edge_drawing.h
        src/algorithm/edge_drawing.cpp
        src/algorithm/saliency_detection.h
        src/algorithm/saliency_detection.cpp

        exlib/edge_drawing/edge_drawing_lib.h
        exlib/saliency/saliency_api.h
        exlib/feature_flow/JumpFlooding_FeatureFlow.h
        exlib/vertex_optimization/VertexOptimization.h

        src/utils/test.h
        src/utils/test.cpp
        src/ui/dialogs_factory.h
        src/ui/saliency_detection_dialog.h
        src/ui/saliency_detection_dialog.cpp
        src/ui/saliency_detection_dialog.ui
        src/algorithm/feature_flow.h
        src/algorithm/feature_flow.cpp
        src/algorithm/vertex_optimization.h
        src/algorithm/vertex_optimization.cpp
        src/algorithm/constrained_triangulation.h
        src/algorithm/constrained_triangulation.cpp
        src/utils/hello.cu


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
    ${CMAKE_SOURCE_DIR}/exlib/edge_drawing
    ${CMAKE_SOURCE_DIR}/exlib/saliency
    ${CMAKE_SOURCE_DIR}/exlib/feature_flow
    ${CMAKE_SOURCE_DIR}/exlib/vertex_optimization
    ${CMAKE_SOURCE_DIR}/exlib/CDT
)

# CDT 要求 C++11 标准兼容
target_compile_features(TestLowPoly04 PRIVATE cxx_std_11)

target_link_libraries(TestLowPoly04 PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    "${CMAKE_CURRENT_SOURCE_DIR}/exlib/edge_drawing/edge_drawing_lib.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/exlib/saliency/SaliencyICCV13.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/exlib/feature_flow/JumpFlooding_FeatureFlow.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/exlib/vertex_optimization/VertexOptimization.lib"
)
# =====================================================================
# 将 CMake 的源码根目录路径作为宏 (PROJECT_SOURCE_DIR) 注入到 C++ 中
# =====================================================================
target_compile_definitions(TestLowPoly04 PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
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



#[[# =====================================================================
# 自动拷贝 exlib 目录下的所有 .dll 文件到可执行文件同级目录
# =====================================================================

# 1. 递归查找 exlib 文件夹（包含子文件夹）下的所有 .dll 文件
file(GLOB_RECURSE EXLIB_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/exlib/*.dll")

# 2. 遍历找到的每一个 .dll 文件
foreach(DLL_FILE IN LISTS EXLIB_DLLS)
    # 3. 添加一个构建后的自定义命令 (POST_BUILD)
    add_custom_command(TARGET TestLowPoly04 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DLL_FILE}"
        "$<TARGET_FILE_DIR:TestLowPoly04>"
        COMMENT "正在拷贝外部依赖: ${DLL_FILE} 到输出目录..."
    )
endforeach()]]
