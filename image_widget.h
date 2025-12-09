#ifndef IMAGE_WIDGET_H
#define IMAGE_WIDGET_H

#include <QWidget>
#include <QImage>
#include <QPointF>
#include <QMouseEvent>
#include <QWheelEvent>

class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageWidget(QWidget *parent = nullptr);

    /*设置并显示图像*/
    void setImage(const QImage &img);
    //TODO: 提供从外部直接拖放图片进入操作窗口的功能

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    /*计算自适应缩放, 只在初次展示某图片时调用*/
    void adjustToWindowSize();
    void updateCachedPixmap();
    QPointF widgetToImage(const QPointF &p) const;

private:
    QImage presentImage;        // 当前显示的图像源数据
    QPixmap cachedPixmap;       // 展示的图像数据
    double scale = 1.0;         // 缩放比例
    double cachedScale = 0.0;   // 缓存的缩放比例
    QPointF offset;             // 偏移量
    QPointF lastMousePos;       //
};

#endif // IMAGE_WIDGET_H
