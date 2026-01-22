#pragma once
#include <QObject>
#include <QPixmap>
#include <memory>
#include "screenshot_global.h"

class SCREENSHOT_EXPORT ScreenCaptureManager : public QObject
{
    Q_OBJECT
public:
    static ScreenCaptureManager& instance();
    void startCapture();

signals:
    void captureFinished(const QPixmap &pixmap);
    void captureCancelled();

private:
    explicit ScreenCaptureManager(QObject *parent = nullptr);
    ~ScreenCaptureManager();
    Q_DISABLE_COPY(ScreenCaptureManager)

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};