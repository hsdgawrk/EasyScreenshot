#include "CanvasWidget.h"

#include "SelectionBar.h" // 引入工具栏
#include "NativeUtils.h"

#include <QApplication>
#include <QGuiApplication>
#include <QDebug>
#include <QWindow>

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent)
{
    // 无边框、置顶、跳过任务栏
    setWindowFlags(Qt::FramelessWindowHint /*| Qt::WindowStaysOnTopHint | Qt::Tool*/);
    // 开启鼠标追踪以便实现悬停吸附
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    // 初始化 UIA 扫描器
    m_uiaScanner = std::make_unique<UIAScanner>();
    m_uiaScanner->init(); // 可以在这里初始化，或者在第一次探测时初始化

    // 初始化工具栏 (隐藏状态)
    m_toolbar = new SelectionBar(this);
    m_toolbar->hide();

    // --- 在这里扩展功能 ---
    // 添加“确认”按钮
    m_toolbar->addTool("完成", "保存截图到剪贴板", [this]()
                       {
        if (!m_selectionRect.isNull()) {
            emit finished(m_fullScreenPix.copy(m_selectionRect));
            close();
        } });
}

void CanvasWidget::setBackground(const QPixmap &pix)
{
    m_fullScreenPix = pix;
    // 设置窗口大小覆盖全屏
    setGeometry(QGuiApplication::primaryScreen()->geometry());
    update();
}

// --- 核心：8点碰撞检测 ---
int CanvasWidget::getHandleAt(const QPoint &pos)
{
    if (m_state != Selected)
        return None;

    QRect r = m_selectionRect;
    int HS = HANDLE_SIZE; // 手柄大小
    int HS2 = HS * 2;     // 扩展点击区域，让用户更容易点中

    // 构建8个关键点的矩形区域进行测试
    // Top Left
    if (QRect(r.left() - HS, r.top() - HS, HS2, HS2).contains(pos))
        return TopLeft;
    // Top Right
    if (QRect(r.right() - HS, r.top() - HS, HS2, HS2).contains(pos))
        return TopRight;
    // Bottom Left
    if (QRect(r.left() - HS, r.bottom() - HS, HS2, HS2).contains(pos))
        return BottomLeft;
    // Bottom Right
    if (QRect(r.right() - HS, r.bottom() - HS, HS2, HS2).contains(pos))
        return BottomRight;

    // Top
    if (QRect(r.left() + HS, r.top() - HS, r.width() - HS2, HS2).contains(pos))
        return Top;
    // Bottom
    if (QRect(r.left() + HS, r.bottom() - HS, r.width() - HS2, HS2).contains(pos))
        return Bottom;
    // Left
    if (QRect(r.left() - HS, r.top() + HS, HS2, r.height() - HS2).contains(pos))
        return Left;
    // Right
    if (QRect(r.right() - HS, r.top() + HS, HS2, r.height() - HS2).contains(pos))
        return Right;

    // Body (移动整个选区)
    if (r.contains(pos))
        return Body;

    return None;
}

void CanvasWidget::updateCursorShape(const QPoint &pos)
{
    if (m_state == Idle)
    {
        setCursor(Qt::CrossCursor);
        return;
    }

    if (m_state == Selected)
    {
        int handle = getHandleAt(pos);
        switch (handle)
        {
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case Top:
        case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case Left:
        case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case Body:
            setCursor(Qt::SizeAllCursor);
            break; // 移动手势
        default:
            setCursor(Qt::ArrowCursor);
            break;
        }
    }
}

// --- 核心：工具栏跟随逻辑 ---
void CanvasWidget::updateToolbarPosition()
{
    if (m_state != Selected || !m_toolbar)
        return;

    // 默认放在选区右下角外部
    int x = m_selectionRect.right() - m_toolbar->width();
    int y = m_selectionRect.bottom() + 10; // 间距 10px

    // 边界检查：如果底部空间不够，就放到选区内部上方
    if (y + m_toolbar->height() > this->height())
    {
        y = m_selectionRect.bottom() - m_toolbar->height() - 10;
    }

    // 边界检查：如果右边空间不够，靠左一点
    if (x < 0)
        x = 10;

    m_toolbar->move(x, y);
    m_toolbar->show();
    m_toolbar->raise(); // 确保在遮罩层之上
}

void CanvasWidget::performAutoSnap(const QPoint &globalPos)
{
    // --- 自动吸附逻辑 ---
    // 获取 DPI 比例
    qreal dpr = windowHandle()->devicePixelRatio();

    // --- 优先尝试 UIA 探测 ---
    // 1. 先用 WinAPI 穿透遮罩，找到底下的“真身” HWND
    void *targetHwnd = NativeUtils::getHwndUnderMouse(globalPos, dpr, this->winId());

    QRect snapped;
    if (targetHwnd)
    {
        // 2. 将真身 HWND 交给 UIA 进行内部解剖
        snapped = m_uiaScanner->detectElementRect(targetHwnd, globalPos, dpr);
    }

    // 3. 如果 UIA 失败（比如目标窗口不支持 UIA），回退到 WinAPI 矩形
    if (snapped.isNull() || !snapped.isValid())
    {
        // 这里调用之前的旧方法作为兜底
        snapped = NativeUtils::getWindowRectUnderMouse(globalPos, dpr, this->winId());
    }

    // 坐标系转换：WinAPI 返回的是全局坐标，需要转换为当前 Widget 的局部坐标
    // 但因为我们是全屏 Widget，全局坐标通常等于局部坐标（除非有多屏偏移）
    QPoint offset = this->mapFromGlobal(globalPos);
    // 简单的校准逻辑 (这里假设单屏或主屏，多屏需更复杂的 mapFromGlobal)

    if (!snapped.isNull())
    {
        // 将全局坐标映射回 Widget 坐标系统
        QPoint topLeft = this->mapFromGlobal(snapped.topLeft());
        m_selectionRect = QRect(topLeft, snapped.size());
    }
    /*else
    {
        m_selectionRect = QRect();
    }*/
    update();
}

void CanvasWidget::changeStateTo(State new_state)
{
    m_state = new_state;

    switch (m_state)
    {
    case Idle:
        m_selectionRect = QRect();
        m_toolbar->hide();
        break;
    case Selecting:
        m_toolbar->hide(); // 调整大小时隐藏工具栏
        break;
    case Selected:
        break;
    default:
        break;
    }

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

        // 6. [新增] 如果是 Selected 状态，画 8 个控制点小方块
        if (m_state == Selected)
        {
            p.setBrush(Qt::white);
            p.setPen(QColor(0, 120, 215));
            int r = HANDLE_SIZE / 2; // 半径

            // 定义 lambda 方便画点
            auto drawHandle = [&](QPoint pt)
            {
                p.drawRect(pt.x() - r, pt.y() - r, HANDLE_SIZE, HANDLE_SIZE);
            };

            // 四角
            drawHandle(m_selectionRect.topLeft());
            drawHandle(m_selectionRect.topRight());
            drawHandle(m_selectionRect.bottomLeft());
            drawHandle(m_selectionRect.bottomRight());

            // 四边中点
            drawHandle(QPoint(m_selectionRect.center().x(), m_selectionRect.top()));
            drawHandle(QPoint(m_selectionRect.center().x(), m_selectionRect.bottom()));
            drawHandle(QPoint(m_selectionRect.left(), m_selectionRect.center().y()));
            drawHandle(QPoint(m_selectionRect.right(), m_selectionRect.center().y()));
        }
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event)
{
    updateCursorShape(event->pos()); // 实时更新光标

    if (m_state == Idle)
    {
        // 计算移动距离
        int distance = (event->pos() - m_dragStartPos).manhattanLength();

        if ((event->buttons() & Qt::LeftButton) && distance > 5)
        {
            changeStateTo(Selecting);
            m_selectionRect = QRect();
            m_dragStartPos = event->pos();
        }
        else
        {
            performAutoSnap(event->globalPos());
        }
    }
    else if (m_state == Selecting)
    {
        // --- 拖拽/调整逻辑 ---
        if (m_dragHandle == None)
        {
            // 第一次创建选区 (橡皮筋)
            m_selectionRect = QRect(m_dragStartPos, event->pos()).normalized();
        }
        else
        {
            // 调整已有选区
            QPoint delta = event->pos() - m_dragStartPos;
            QRect r = m_dragStartRect;

            switch (m_dragHandle)
            {
            case TopLeft:
                r.setTopLeft(r.topLeft() + delta);
                break;
            case Top:
                r.setTop(r.top() + delta.y());
                break;
            case TopRight:
                r.setTopRight(r.topRight() + delta);
                break;
            case Right:
                r.setRight(r.right() + delta.x());
                break;
            case BottomRight:
                r.setBottomRight(r.bottomRight() + delta);
                break;
            case Bottom:
                r.setBottom(r.bottom() + delta.y());
                break;
            case BottomLeft:
                r.setBottomLeft(r.bottomLeft() + delta);
                break;
            case Left:
                r.setLeft(r.left() + delta.x());
                break;
            case Body:
                r.translate(delta);
                break;
            default:
                break;
            }
            m_selectionRect = r.normalized(); // 避免负宽高
        }
        update();
    }
}

// --- 事件处理 ---

void CanvasWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // 保存按下时的坐标，用于判断是否发生拖拽
        m_dragStartPos = event->pos();

        if (m_state == Idle)
        {
            // Idle 状态下按下，什么都不做，等待 Move 判断是否拖拽，或者 Release 判断是否点击
            // 保持当前吸附的矩形不变
            m_toolbar->hide(); // 隐藏工具栏
        }
        else if (m_state == Selected)
        {
            // 检查点击的是哪里
            m_dragHandle = (Handle)getHandleAt(event->pos());
            if (m_dragHandle != None)
            {
                changeStateTo(Selecting);// 暂时切回 Selecting 状态以复用重绘逻辑，或者保持 Selected 并增加一个 Resizing 状态
                // 这里为了简单，我们引入一个临时变量标记我们在调整
                m_dragStartPos = event->pos();
                m_dragStartRect = m_selectionRect; // 记录原始矩形
                
            }
            else
            {
                // 点中选区外部
                changeStateTo(Selecting);
            }
        }
    }
    else if (event->button() == Qt::RightButton)
    {
        if (m_state == Selecting || m_state == Selected)
        {
            // 右键取消选区，回到 Idle
            changeStateTo(Idle);
        }
        else
        {
            // Idle 状态下右键退出
            emit cancelled();
            close();
        }
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (m_state == Selecting)
        {
            // 拖拽结束，进入 Selected 状态
            changeStateTo(Selected);
            m_dragHandle = None; // 重置句柄

            // 确保矩形有效
            if (m_selectionRect.width() <= 2 || m_selectionRect.height() <= 2)
            {
                changeStateTo(Idle);// 太小了，视为误触
            }
            else
            {
                updateToolbarPosition(); // 显示工具栏
                update();
            }
        }
        else if (m_state == Idle)
        {
            // 【核心修改】
            // 如果松开鼠标时还是 Idle，说明没有触发拖拽逻辑
            // 这意味着用户想要“确认当前吸附的矩形”

            if (!m_selectionRect.isNull() && m_selectionRect.width() > 1)
            {
                changeStateTo(Selected);// 直接进入选中状态
                updateToolbarPosition(); // 显示工具栏
            }
            // 如果当前没有吸附到任何东西 (Rect 为空)，则依然保持 Idle
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
