// NativeUtils.h
#pragma once
#include <QRect>
#include <QPoint>
#include <QWidget>

class NativeUtils {
public:
    // 新增：只获取句柄，不计算矩形
    static void* getHwndUnderMouse(const QPoint& globalPos, double devicePixelRatio, WId skipId);

    // 增加 skipId 参数，传入我们自己的窗口句柄
    static QRect getWindowRectUnderMouse(const QPoint &globalPos, double devicePixelRatio, WId skipId);
};