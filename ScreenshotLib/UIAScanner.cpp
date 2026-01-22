#include "UIAScanner.h"
#include <QDebug>
#include <Windows.h>

UIAScanner::UIAScanner() {
    // 构造函数里不做耗时操作，显式调用 init
}

UIAScanner::~UIAScanner() {
    // ComPtr 会自动 Release，不需要手动处理
    // 但要在析构时清理 COM 环境，通常由宿主程序管理，这里暂时略过 CoUninitialize
}

bool UIAScanner::init() {
    if (m_initialized) return true;

    // 1. 初始化 COM 库 (Qt 主线程通常已经是 STA，这里尝试初始化一下也没坏处)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        // 如果已经初始化过，且模式不同，可能会失败，视情况处理
    }

    // 2. 创建 UIA 实例
    hr = CoCreateInstance(CLSID_CUIAutomation, nullptr,
                          CLSCTX_INPROC_SERVER, IID_IUIAutomation,
                          reinterpret_cast<void**>(m_automation.GetAddressOf()));

    if (FAILED(hr)) {
        qWarning() << "Failed to create UIAutomation instance. Error:" << hr;
        return false;
    }

    m_initialized = true;
    return true;
}

QRect UIAScanner::detectElementRect(const QPoint &globalPos, double devicePixelRatio) {
    if (!m_initialized || !m_automation) return QRect();

    POINT pt;
    // UIA 使用的是物理坐标，所以要乘以 DPI
    pt.x = static_cast<LONG>(globalPos.x() * devicePixelRatio);
    pt.y = static_cast<LONG>(globalPos.y() * devicePixelRatio);

    ComPtr<IUIAutomationElement> element;
    
    // 3. 核心魔法：从点获取元素 (ElementFromPoint)
    // 这一步在某些复杂应用上可能会有 10ms-50ms 的阻塞，注意性能
    HRESULT hr = m_automation->ElementFromPoint(pt, &element);
    
    if (FAILED(hr) || !element) {
        return QRect();
    }

    // 4. 获取元素的包围盒 (Bounding Rectangle)
    RECT rect;
    hr = element->get_CurrentBoundingRectangle(&rect);
    if (FAILED(hr)) {
        return QRect();
    }

    // 5. 坐标转回 Qt 逻辑坐标
    return QRect(
        static_cast<int>(rect.left / devicePixelRatio),
        static_cast<int>(rect.top / devicePixelRatio),
        static_cast<int>((rect.right - rect.left) / devicePixelRatio),
        static_cast<int>((rect.bottom - rect.top) / devicePixelRatio)
    );
}