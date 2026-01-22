// NativeUtils.h
#pragma once
#include <QRect>
#include <QPoint>
#include <QWidget>

class NativeUtils {
public:
    // 增加 skipId 参数，传入我们自己的窗口句柄
    static QRect getWindowRectUnderMouse(const QPoint &globalPos, double devicePixelRatio, WId skipId);
};