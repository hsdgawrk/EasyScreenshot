// NativeUtils.cpp
#include "NativeUtils.h"
#include <Windows.h>
#include <dwmapi.h> // 需要在 CMake 中链接 Dwmapi

// MinGW 有时需要定义这个宏才能使用某些 DWM API
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// 辅助函数：判断窗口类名是否属于某些特殊的不建议吸附的类（可选）
bool isSpecialWindow(HWND hwnd) {
    char className[256];
    if (GetClassNameA(hwnd, className, 256)) {
        // 比如排除 Windows 任务栏的开始按钮等特殊装饰
        // if (strcmp(className, "Shell_TrayWnd") == 0) return true;
    }
    return false;
}

QRect NativeUtils::getWindowRectUnderMouse(const QPoint &globalPos, double devicePixelRatio, WId skipId) {
    POINT pt;
    pt.x = static_cast<LONG>(globalPos.x() * devicePixelRatio);
    pt.y = static_cast<LONG>(globalPos.y() * devicePixelRatio);

    // 1. 初步探测 (可能会探测到我们自己的全屏遮罩)
    HWND hwnd = WindowFromPoint(pt);

    // 2. 穿透遮罩层 (这部分逻辑保持不变)
    if (hwnd == (HWND)skipId) {
        HWND next = GetWindow(hwnd, GW_HWNDNEXT);
        while (next) {
            if (IsWindowVisible(next)) {
                RECT bounds;
                GetWindowRect(next, &bounds);
                if (PtInRect(&bounds, pt)) {
                    hwnd = next;
                    break;
                }
            }
            next = GetWindow(next, GW_HWNDNEXT);
        }
    }

    if (!hwnd || hwnd == (HWND)skipId) return QRect();

    // 3. 【核心修改】尝试钻取更深层的子控件 (High Granularity)
    // WindowFromPoint 有时比较懒，只返回包含点的 GroupBox，而不返回里面的 Button
    // 我们可以用 ChildWindowFromPointEx 尝试在当前 hwnd 里面再找一层
    POINT ptClient = pt;
    ScreenToClient(hwnd, &ptClient);
    HWND child = ChildWindowFromPointEx(hwnd, ptClient, CWP_ALL); // CWP_ALL = 不跳过任何子窗口
    if (child && child != hwnd) {
        // 确认一下这个子窗口是不是真的可见
        if (IsWindowVisible(child)) {
             hwnd = child;
        }
    }
    
    // 4. 【已移除】 GetAncestor(hwnd, GA_ROOT); 
    // 不再强制找父窗口，保留当前找到的“最深”控件

    // 5. 获取几何矩形
    RECT r;
    
    // 获取窗口样式
    LONG_PTR styles = GetWindowLongPtr(hwnd, GWL_STYLE);
    bool isChild = (styles & WS_CHILD); // 检查是否是子控件

    bool success = false;

    // 如果是顶层窗口，尝试用 DWM 去除阴影
    if (!isChild) {
        if (DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(RECT)) == S_OK) {
            success = true;
        }
    }

    // 如果是子控件，或者 DWM 失败，直接获取物理矩形
    if (!success) {
        GetWindowRect(hwnd, &r);
    }

    // 6. 坐标转换
    return QRect(
        static_cast<int>(r.left / devicePixelRatio),
        static_cast<int>(r.top / devicePixelRatio),
        static_cast<int>((r.right - r.left) / devicePixelRatio),
        static_cast<int>((r.bottom - r.top) / devicePixelRatio)
    );
}