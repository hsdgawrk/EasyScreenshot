#pragma once
#include "UIAScanner.h" // 引入头文件

#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QMouseEvent>
#include <QScreen>

#include <memory>

class SelectionBar; // 前置声明

class CanvasWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CanvasWidget(QWidget *parent = nullptr);
    void setBackground(const QPixmap &pix);

signals:
    void finished(const QPixmap &capture);
    void cancelled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum State
    {
        Idle,
        Selecting,
        Selected
    };
    State m_state = Idle;

    // 拖拽模式定义
    enum Handle
    {
        None,
        TopLeft,
        Top,
        TopRight,
        Left,
        Right,
        BottomLeft,
        Bottom,
        BottomRight,
        Body
    };
    Handle m_dragHandle = None;

    QPixmap m_fullScreenPix; // 全屏原图
    QRect m_selectionRect;   // 当前选区
    QPoint m_dragStartPos;   // 鼠标按下时的坐标
    QRect m_dragStartRect;   // 拖拽开始时的矩形（用于计算偏移）

    // 增加成员变量
    std::unique_ptr<UIAScanner> m_uiaScanner;
    SelectionBar* m_toolbar = nullptr; // 工具栏实例

    const int HANDLE_SIZE = 8; // 手柄触点大小

private:
    // 更新光标形状
    void updateCursorShape(const QPoint &pos);
    // 获取鼠标位置对应的操作手柄
    int getHandleAt(const QPoint &pos);
    // 更新工具栏位置
    void updateToolbarPosition();
    // 提取自动吸附逻辑，避免代码重复
    void performAutoSnap(const QPoint& globalPos);
    void changeStateTo(State new_state);
};