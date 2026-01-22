add_compile_definitions(SCREENSHOT_LIBRARY)

# 必须开启 AUTOMOC
set(CMAKE_AUTOMOC ON) 

set(SOURCES
    screenshot_global.h
    ScreenCaptureManager.h ScreenCaptureManager.cpp
    NativeUtils.h NativeUtils.cpp
    CanvasWidget.h CanvasWidget.cpp
	UIAScanner.h UIAScanner.cpp
	SelectionBar.h SelectionBar.cpp
)

add_library(ScreenshotLib SHARED ${SOURCES})

target_link_libraries(ScreenshotLib PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core)

if(WIN32)
    # MinGW 编译器链接 Windows 系统库的标准写法
    # user32: 窗口管理
    # dwmapi: 窗口特效(去边框)
    # gdi32: 绘图相关
    target_link_libraries(ScreenshotLib PRIVATE user32 dwmapi Ole32 OleAut32)
endif()