#include "SelectionBar.h"
#include <QPainter>
#include <QStyleOption>

SelectionBar::SelectionBar(QWidget *parent) : QWidget(parent) {
    // 设置工具栏样式：白色背景、圆角、阴影
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground); // 允许圆角透明

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(10, 5, 10, 5);
    m_layout->setSpacing(8);

    // 设置固定高度，方便计算位置
    setFixedHeight(40); 
}

void SelectionBar::addTool(const QString &text, const QString &tooltip, std::function<void()> onClick) {
    QPushButton *btn = new QPushButton(text, this);
    btn->setToolTip(tooltip);
    btn->setCursor(Qt::PointingHandCursor);
    
    // 简单的样式表美化
    btn->setStyleSheet(
        "QPushButton { "
        "  background-color: #0078d7; color: white; border: none; "
        "  padding: 5px 15px; border-radius: 4px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #0063b1; }"
        "QPushButton:pressed { background-color: #004f8b; }"
    );

    connect(btn, &QPushButton::clicked, this, onClick);
    m_layout->addWidget(btn);
}

// 重写 Paint 以支持圆角背景绘制 (因为 setAttribute(WA_TranslucentBackground) 后默认不画背景)
// 也可以直接在 CanvasWidget 里作为子控件管理，这里简化处理，假设它是一个普通的子 Widget
/* 如果你想让它有阴影，最好重写 paintEvent 画一个圆角矩形 */