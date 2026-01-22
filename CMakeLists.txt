cmake_minimum_required(VERSION 3.16)
project(QtScreenshotSolution LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# --- 自动检测架构并设置输出目录 ---
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_NAME "x64")
else()
    set(ARCH_NAME "x86")
endif()

# 将生成物放在 bin/x64 或 bin/x86 下，互不干扰
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${ARCH_NAME})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${ARCH_NAME})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${ARCH_NAME})

message(STATUS "Build Architecture: ${ARCH_NAME}")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# --- 查找 Qt ---
# 注意：这里不再设置 CMAKE_PREFIX_PATH，由外部 Presets 注入
find_package(Qt5 COMPONENTS Widgets Gui Core REQUIRED)

# 添加子目录
add_subdirectory(ScreenshotLib)
add_subdirectory(DemoApp)