#include "image_widget.h"

#include <QPainter>
#include "logger.h"

ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent)
{
    autoSize = true;
    //editFunc = false;
    setMouseTracking(true); // 启用鼠标跟踪（不按下按钮也能接收移动事件）
}

void ImageWidget::updateImageKeepView(const QImage &img)
{
    if(img.isNull())
    {
        LOG_ERROR << "The image to be displayed is empty";
        return;
    }
    presentImage = img;
    // 不重置缩放值, 但应该缓存失效, 否则会继续绘制旧图片的缓存
    cachedScale = -1.0;
    // 重绘
    update();
}

void ImageWidget::updateImageResetView(const QImage &img)
{
    if(img.isNull())
    {
        LOG_ERROR << "The image to be displayed is empty";
        return;
    }
    presentImage = img;
    // 重置显示状态
    scale = 1.0;
    offset = QPoint(0, 0);
    cachedScale = 0.0;
    // 初始加载时适应窗口
    if (autoSize)
        adjustToWindowSize();
    // 重绘
    update();
}

void ImageWidget::adjustToWindowSize()
{
    // 获取当前窗口的大小
    QSize newSize = size();
    // 计算实际的缩放比例,选择最小的缩放比例，确保图像保持原始宽高比
    double widthScale = static_cast<double>(newSize.width()) / static_cast<double>(presentImage.width());
    double heightScale = static_cast<double>(newSize.height()) / static_cast<double>(presentImage.height());
    scale = qMin(widthScale, heightScale);
    cachedScale = scale;
    //Qt::SmoothTransformation会进行双线性过滤,像素边缘会变模糊,产生渐变.
    //Qt::FastTransformation直接取最近的像素颜色.当放大倍数很大时,看到一个个完美的正方形像素点,边界极其清晰.
    cachedPixmap = QPixmap::fromImage(presentImage.scaled(newSize, Qt::KeepAspectRatio, Qt::FastTransformation));
}

QPointF ImageWidget::widgetToImage(const QPointF &wpos) const
{
    return (wpos - offset) / scale;
}

void ImageWidget::updateCachedPixmap()
{
    if (presentImage.isNull() || cachedScale == scale)
        return;

    QSize scaledSize = presentImage.size() * scale;

    cachedPixmap = QPixmap::fromImage(
        presentImage.scaled(
            scaledSize,
            Qt::KeepAspectRatio,
            Qt::FastTransformation)
        );
    cachedScale = scale;
}

void ImageWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::transparent);
    //painter.fillRect(rect(), Qt::white);

    if (presentImage.isNull())
        return;

    updateCachedPixmap();

    painter.drawPixmap(offset, cachedPixmap);
    // 绘制半透明的蓝色选择框
    if (isSelecting)
    {
        painter.setPen(QPen(QColor(0, 120, 215), 1, Qt::DashLine));
        painter.setBrush(QColor(0, 120, 215, 50));
        painter.drawRect(QRectF(selectionStart, selectionEnd).normalized());
    }

    // 绘制选中的三角形高亮框
    if (!highlightedTriangle.isEmpty()) {
        // 将原图坐标映射到窗口坐标
        QPolygonF mappedPoly;
        // 关于警告: clazy-range-loop-detach
        // 1: 使用 C++17 的 std::as_const. for (const QPointF& pt : std::as_const(highlightedTriangle))
        // 2: 使用 Qt 自带的 qAsConst. for (const QPointF& pt : qAsConst(highlightedTriangle))
        // 3: 使用最基础的 C++11 const auto& 别名(兼容性最强).
        // const auto& constPoly = highlightedTriangle;
        // for (const QPointF& pt : constPoly)
        for (const QPointF& pt : std::as_const(highlightedTriangle)) {
            mappedPoly << (pt * scale + offset);
        }

        QPen highlightPen(QColor(0, 255, 255), 2.0, Qt::DashLine); // 青色虚线
        painter.setPen(highlightPen);
        painter.setBrush(QColor(0, 255, 255, 50)); // 半透明填充
        painter.drawPolygon(mappedPoly);
    }
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() & Qt::ControlModifier) {
            // Ctrl + 左键：开始框选删除
            isSelecting = true;
            selectionStart = event->pos();
            selectionEnd = event->pos();
        } else if (event->modifiers() & Qt::AltModifier) {
            // Alt + 左键：增加点 (将 Widget 坐标转换为原图坐标)
            emit requestAddPoint(widgetToImage(event->pos()));
        } else {
            // 普通拖拽漫游
            lastMousePos = event->pos();
        }
    }
    else if (event->button() == Qt::RightButton) {
        // 右键直接触发选中三角形
        emit requestSelectTriangle(widgetToImage(event->pos()));
        event->accept(); // 吞掉这个事件，防止冒泡触发外层可能的上下文菜单
    }
    // 如果右键点击后，除了选中三角形还触发了其他的上下文菜单（如果有的话），
    // 可能需要在 else if (event->button() == Qt::RightButton) 里面加一句 event->accept(); 来吞掉这个事件，防止它继续传递。
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isSelecting)
    {
        // 更新框选终点并重绘
        selectionEnd = event->pos();
        update();
    }
    else if (event->buttons() & Qt::LeftButton)
    {
        QPointF delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
        update();
    }
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isSelecting)
    {
        isSelecting = false;

        // 获取正规化的框选区域 (左上角和右下角)
        QRectF widgetRect(selectionStart, selectionEnd);
        widgetRect = widgetRect.normalized();

        // 将 Widget 的区域转换为真实图像的区域
        QPointF topLeft = widgetToImage(widgetRect.topLeft());
        QPointF bottomRight = widgetToImage(widgetRect.bottomRight());
        QRectF imageArea(topLeft, bottomRight);

        // 发出删除信号
        emit requestDeletePointsInArea(imageArea.normalized());
        update();
    }
}

void ImageWidget::wheelEvent(QWheelEvent *event)
{
    if (presentImage.isNull())
        return;

    QPointF mousePos = event->position();
    QPointF imgPosBefore = widgetToImage(mousePos);
    // 缩放
    double factor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;
    scale *= factor;
    // 限制范围
    scale = qBound(0.05, scale, 20.0);
    // 使缩放中心在鼠标位置
    QPointF imgPosAfter = imgPosBefore * scale + offset;
    offset += (mousePos - imgPosAfter);

    update();
}
