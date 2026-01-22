
#include "ScreenCaptureManager.h"
#include "CanvasWidget.h"

#include <QGuiApplication>
#include <QScreen>
#include <QTimer>

#include <memory>

struct ScreenCaptureManager::Impl {
    CanvasWidget* canvas = nullptr;
};

ScreenCaptureManager::ScreenCaptureManager(QObject *parent)
    : QObject(parent), m_impl(new Impl) {}

ScreenCaptureManager::~ScreenCaptureManager() = default;

ScreenCaptureManager& ScreenCaptureManager::instance() {
    static ScreenCaptureManager inst;
    return inst;
}

void ScreenCaptureManager::startCapture() {
    // 避免重复启动
    if (m_impl->canvas) return;

    // 1. 截取全屏
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    QPixmap fullPix = screen->grabWindow(0);

    // 2. 创建画布 (设为 DeleteOnClose，但我们需要手动管理指针置空)
    m_impl->canvas = new CanvasWidget(nullptr);
    m_impl->canvas->setAttribute(Qt::WA_DeleteOnClose);
    m_impl->canvas->setBackground(fullPix);

    // 3. 连接信号
    connect(m_impl->canvas, &CanvasWidget::finished, this, [this](const QPixmap &p){
        emit captureFinished(p);
        m_impl->canvas = nullptr;
    });

    connect(m_impl->canvas, &CanvasWidget::cancelled, this, [this](){
        emit captureCancelled();
        m_impl->canvas = nullptr;
    });
    
    // 监听销毁信号以防意外关闭
    connect(m_impl->canvas, &QObject::destroyed, this, [this](){
        if(m_impl->canvas) m_impl->canvas = nullptr;
    });

    // 4. 显示
    m_impl->canvas->show();
}
