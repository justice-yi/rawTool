cmake_minimum_required(VERSION 3.16)
project(ImageToolQt VERSION 0.36 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Widgets REQUIRED)
endif()

# ---- 核心库：纯 C，不依赖 Qt ----
add_library(itcore STATIC
    src/core/itypes.c
    src/core/raw_loader.c
    src/core/bayer.c
    src/core/isp_pipeline.c
    src/core/analysis.c
    src/core/it_persist.c
    src/core/imgio.c
)
target_include_directories(itcore PUBLIC src)

if (MSVC)
    target_compile_options(itcore PRIVATE /W4)
else()
    target_compile_options(itcore PRIVATE -Wall -Wextra)
endif()
if (NOT WIN32)
    target_link_libraries(itcore PUBLIC m)   # MinGW 下数学函数在 msvcrt
endif()

# ---- GUI 壳：Qt Widgets ----
add_executable(ImageToolQt
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/ImageView.cpp
    src/gui/RawFormatDialog.cpp
    src/gui/IspPanelDialog.cpp
    src/gui/AnalysisDialog.cpp
)
if (Qt6_FOUND)
    target_link_libraries(ImageToolQt PRIVATE Qt6::Widgets)
else()
    target_link_libraries(ImageToolQt PRIVATE Qt5::Widgets)
endif()
target_link_libraries(ImageToolQt PRIVATE itcore)

# Windows 目标：GUI 子系统（不弹控制台）+ 静态链接 MinGW 运行库（免 DLL）
if (WIN32)
    set_target_properties(ImageToolQt PROPERTIES WIN32_EXECUTABLE TRUE)
    if (NOT MSVC)
        target_link_options(ImageToolQt PRIVATE -static)
    endif()
endif()
if (MSVC)
    target_compile_options(ImageToolQt PRIVATE /W3)
else()
    target_compile_options(ImageToolQt PRIVATE -Wall -Wextra)
endif()

# ---- 静态 Qt：显式注册平台与图片格式插件（否则静态插件不初始化）----
if (Qt6_FOUND AND QT_IS_STATIC_LIB)
    qt_import_plugins(ImageToolQt
        INCLUDE Qt6::QWindowsIntegrationPlugin
                Qt6::QJpegPlugin
                Qt6::QGifPlugin
                Qt6::QIcoPlugin)
endif()

# ---- 冒烟测试（核心库端到端验证，非 GUI）----
enable_testing()
add_executable(it_smoke_test tools/smoke_test.c)
target_link_libraries(it_smoke_test PRIVATE itcore)
add_test(NAME smoke COMMAND it_smoke_test
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
