#pragma once
#include "UIAScanner.h" // 引入头文件

#include <QWidget>
#include <QPixmap>
#include <QPainter>
#include <QMouseEvent>
#include <QScreen>

#include <memory>

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

    QPixmap m_fullScreenPix; // 全屏原图
    QRect m_selectionRect;   // 当前选区
    QPoint m_startPos;       // 拖拽起始点

    // 增加成员变量
    std::unique_ptr<UIAScanner> m_uiaScanner;
};