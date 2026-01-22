#pragma once

// 1. 必须先包含 Windows.h
#include <Windows.h>

// 2. 修正：使用 UIAutomationClient.h 而不是 UIAutomation.h
#include <UIAutomationClient.h>

// 3. WRL 智能指针
#include <wrl/client.h> 

#include <QRect>
#include <QPoint>

// 使用 Microsoft::WRL 命名空间下的 ComPtr
using Microsoft::WRL::ComPtr;

class UIAScanner {
public:
    UIAScanner();
    ~UIAScanner();

    bool init();
    QRect detectElementRect(void* hwnd, const QPoint& globalPos, double devicePixelRatio);

private:
    ComPtr<IUIAutomation> m_automation;
    ComPtr<IUIAutomationTreeWalker> m_rawWalker; // 新增：原始视图遍历器
    bool m_initialized = false;
};
