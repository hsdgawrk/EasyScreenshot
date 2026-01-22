#include "CanvasWidget.h"
#include "NativeUtils.h"

#include <QApplication>
#include <QGuiApplication>
#include <QDebug>
#include <QWindow>

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent)
{
    // 无边框、置顶、跳过任务栏
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    // 开启鼠标追踪以便实现悬停吸附
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    // 初始化 UIA 扫描器
    m_uiaScanner = std::make_unique<UIAScanner>();
    m_uiaScanner->init(); // 可以在这里初始化，或者在第一次探测时初始化
}

void CanvasWidget::setBackground(const QPixmap &pix)
{
    m_fullScreenPix = pix;
    // 设置窗口大小覆盖全屏
    setGeometry(QGuiApplication::primaryScreen()->geometry());
    update();
}

void CanvasWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    // 1. 绘制底层原图
    p.drawPixmap(0, 0, m_fullScreenPix);

    // 2. 绘制半透明黑色遮罩
    p.fillRect(rect(), QColor(0, 0, 0, 100));

    // 3. 挖空选区 (CompositionMode_Clear 会把目标区域变透明，露出底下的原图)
    // 注意：这里不能直接用 Clear，因为我们是在 Widget 上画。
    // 技巧：我们使用 Source 模式把原图的对应区域再画一遍，覆盖掉半透明遮罩
    if (!m_selectionRect.isNull())
    {
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawPixmap(m_selectionRect, m_fullScreenPix, m_selectionRect);

        // 4. 绘制边框
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        p.setPen(QPen(Qt::cyan, 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(m_selectionRect);

        // 绘制尺寸文字
        QString sizeText = QString("%1 x %2").arg(m_selectionRect.width()).arg(m_selectionRect.height());
        p.setPen(Qt::white);
        p.drawText(m_selectionRect.topLeft() - QPoint(0, 5), sizeText);
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_state == Idle)
    {
        // --- 自动吸附逻辑 ---
        // 获取 DPI 比例
        qreal dpr = windowHandle()->devicePixelRatio();

        // --- 优先尝试 UIA 探测 ---
        QRect snapped = m_uiaScanner->detectElementRect(event->globalPos(), dpr);
        // UIA 有时候会返回整个 Desktop 的大矩形，我们通常不需要吸附整个桌面
        // 简单判断：如果吸附区域等于屏幕大小，则忽略（或者你也可以保留）
        if (snapped.isValid() && snapped.size() == this->size())
        {
            snapped = QRect(); // 视为无效，或者降级处理
        }

        if (snapped.isNull())
        {
            snapped = NativeUtils::getWindowRectUnderMouse(event->globalPos(), dpr, this->winId());
        }

        // 坐标系转换：WinAPI 返回的是全局坐标，需要转换为当前 Widget 的局部坐标
        // 但因为我们是全屏 Widget，全局坐标通常等于局部坐标（除非有多屏偏移）
        QPoint offset = this->mapFromGlobal(event->globalPos());
        // 简单的校准逻辑 (这里假设单屏或主屏，多屏需更复杂的 mapFromGlobal)

        if (!snapped.isNull())
        {
            // 将全局坐标映射回 Widget 坐标系统
            QPoint topLeft = this->mapFromGlobal(snapped.topLeft());
            m_selectionRect = QRect(topLeft, snapped.size());
        }
        else
        {
            m_selectionRect = QRect();
        }
        update();
    }
    else if (m_state == Selecting)
    {
        // --- 橡皮筋拖拽逻辑 ---
        QRect rect(m_startPos, event->pos());
        m_selectionRect = rect.normalized(); // 处理负宽高
        update();
    }
}

void CanvasWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_state = Selecting;
        m_startPos = event->pos();
        m_selectionRect = QRect(); // 清除吸附框，开始自由选取
    }
    else if (event->button() == Qt::RightButton)
    {
        // 右键退出
        emit cancelled();
        close();
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_state = Selected;

        // 简单起见，松开鼠标直接完成截图
        // 实际项目中这里应该弹出工具栏让用户确认
        if (!m_selectionRect.isNull() && m_selectionRect.width() > 1 && m_selectionRect.height() > 1)
        {
            // 从原图中切出选区
            QPixmap result = m_fullScreenPix.copy(m_selectionRect);
            emit finished(result);
            close();
        }
        else
        {
            // 如果选区太小（比如误触），重置回 Idle
            m_state = Idle;
            update();
        }
    }
}

void CanvasWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit cancelled();
        close();
    }
}
