#pragma once
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <functional>

class SelectionBar : public QWidget {
    Q_OBJECT
public:
    explicit SelectionBar(QWidget *parent = nullptr);

    // 核心扩展接口：添加一个功能按钮
    void addTool(const QString &text, const QString &tooltip, std::function<void()> onClick);

private:
    QHBoxLayout *m_layout;
};