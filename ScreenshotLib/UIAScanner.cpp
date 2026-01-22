#include "UIAScanner.h"
#include <QDebug>
#include <Windows.h>

UIAScanner::UIAScanner() {}
UIAScanner::~UIAScanner() {}

bool UIAScanner::init() {
    if (m_initialized) return true;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    hr = CoCreateInstance(CLSID_CUIAutomation, nullptr,
        CLSCTX_INPROC_SERVER, IID_IUIAutomation,
        reinterpret_cast<void**>(m_automation.GetAddressOf()));

    if (FAILED(hr)) return false;

    // 【关键】预先获取 RawViewWalker (原始视图遍历器)
    // 只有 RawView 才能看到托盘里的那些小图标
    hr = m_automation->get_RawViewWalker(m_rawWalker.GetAddressOf());
    if (FAILED(hr)) return false;

    m_initialized = true;
    return true;
}

QRect UIAScanner::detectElementRect(void* hwnd, const QPoint& globalPos, double devicePixelRatio) {
    if (!m_initialized || !m_automation || !m_rawWalker || !hwnd) return QRect();

    POINT pt;
    pt.x = static_cast<LONG>(globalPos.x() * devicePixelRatio);
    pt.y = static_cast<LONG>(globalPos.y() * devicePixelRatio);

    ComPtr<IUIAutomationElement> element;

    // 1. 【关键变化】从 HWND 开始，而不是从 Point 开始
    // 这样就完美绕过了全屏遮罩层的遮挡问题
    HRESULT hr = m_automation->ElementFromHandle((UIA_HWND)hwnd, &element);
    if (FAILED(hr) || !element) return QRect();

    // 2. 向下钻取逻辑 (Deep Drill-Down)
    // 逻辑与之前类似，但起点已经是目标窗口（如托盘栏），只要往下找即可
    int depthSafety = 0; // 防止死循环
    while (depthSafety++ < 50) {
        ComPtr<IUIAutomationElement> child;

        // 获取第一个子元素
        hr = m_rawWalker->GetFirstChildElement(element.Get(), &child);
        if (FAILED(hr) || !child) break; // 无子节点，当前就是叶子

        bool foundDeeper = false;
        ComPtr<IUIAutomationElement> sibling = child;

        // 遍历兄弟节点
        while (sibling) {
            RECT childRect;
            if (SUCCEEDED(sibling->get_CurrentBoundingRectangle(&childRect))) {
                // 必须严格包含鼠标点
                if (PtInRect(&childRect, pt)) {
                    // 找到目标，更新 element，准备下一层循环
                    element = sibling;
                    foundDeeper = true;
                    break;
                }
            }

            ComPtr<IUIAutomationElement> next;
            m_rawWalker->GetNextSiblingElement(sibling.Get(), &next);
            sibling = next;
        }

        if (!foundDeeper) {
            // 如果所有子节点都不包含鼠标（可能鼠标在子控件之间的缝隙），
            // 那就认为当前的父容器是最佳选择
            break;
        }
    }

    // 3. 获取最终矩形
    RECT rect;
    hr = element->get_CurrentBoundingRectangle(&rect);
    if (FAILED(hr)) return QRect();

    // 过滤掉全屏大小的结果 (防止误吸附到整个桌面)
    // 如果 rect 几乎等于屏幕大小，可能需要丢弃

    return QRect(
        static_cast<int>(rect.left / devicePixelRatio),
        static_cast<int>(rect.top / devicePixelRatio),
        static_cast<int>((rect.right - rect.left) / devicePixelRatio),
        static_cast<int>((rect.bottom - rect.top) / devicePixelRatio)
    );
}