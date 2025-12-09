#include "image_widget.h"
#include <QPainter>

ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true); // 启用鼠标跟踪（不按下按钮也能接收移动事件）
}


void ImageWidget::setImage(const QImage &img)
{
    presentImage = img;

    // 重置显示状态
    scale = 1.0;
    offset = QPoint(0, 0);
    cachedScale = 0.0;

    // 初始加载时适应窗口
    adjustToWindowSize();

    //重绘,如果后续进行不同处理结果切换时出现不显示的bug，取消注释
    //update();
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

    // 根据窗口大小调整图像缩放
    cachedPixmap = QPixmap::fromImage(presentImage.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

}
/*将控件坐标映射到原图坐标*/
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
            Qt::SmoothTransformation)
        );

    cachedScale = scale;
}

void ImageWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    if (presentImage.isNull())
        return;

    updateCachedPixmap();

    painter.drawPixmap(offset, cachedPixmap);
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        lastMousePos = event->pos();
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {

        QPointF delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
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
