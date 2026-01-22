#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QDebug>
#include "../ScreenshotLib/ScreenCaptureManager.h" // 引用 DLL 头文件

int main(int argc, char *argv[])
{
    // 重要：开启高分屏支持，否则坐标计算会错乱
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    QApplication a(argc, argv);

    QWidget w;
    w.setWindowTitle("Qt Screenshot Component Demo");
    w.resize(800, 600);

    QVBoxLayout *layout = new QVBoxLayout(&w);

    QPushButton *btnCapture = new QPushButton("Start Screen Capture (Click Me)", &w);
    btnCapture->setMinimumHeight(50);
    
    QLabel *lblPreview = new QLabel("Screenshot will appear here", &w);
    lblPreview->setAlignment(Qt::AlignCenter);
    lblPreview->setStyleSheet("border: 2px dashed gray; background: #f0f0f0;");
    lblPreview->setScaledContents(true); // 让图片自适应 Label 大小

    layout->addWidget(btnCapture);
    layout->addWidget(lblPreview);

    // 调用 DLL 能力
    QObject::connect(btnCapture, &QPushButton::clicked, [&](){
        // 为了演示效果，延迟 200ms 隐藏主窗口（模仿 QQ 截图时隐藏当前窗口）
        // w.hide(); 
        
        auto& manager = ScreenCaptureManager::instance();
        manager.startCapture();
    });

    // 接收结果
    auto& manager = ScreenCaptureManager::instance();
    QObject::connect(&manager, &ScreenCaptureManager::captureFinished, [&](const QPixmap &pix){
        w.showNormal(); // 如果之前隐藏了，这里恢复
        lblPreview->setPixmap(pix);
        qDebug() << "Captured size:" << pix.size();
    });

    QObject::connect(&manager, &ScreenCaptureManager::captureCancelled, [&](){
        w.showNormal();
        qDebug() << "Capture cancelled";
    });

    w.show();
    return a.exec();
}