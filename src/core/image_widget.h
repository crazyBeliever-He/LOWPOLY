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
    bool autoSize;  // 是否自适应窗口大小
private:
    QImage presentImage;        // 当前图像的源数据
    QPixmap cachedPixmap;       // 经过缩放偏的移缓存数据
    double scale = 1.0;         // 缩放比例
    double cachedScale = 0.0;   // 缓存的缩放比例
    QPointF offset;             // 偏移量
    QPointF lastMousePos;       // 鼠标最新位置

public:
    explicit ImageWidget(QWidget *parent = nullptr);
    // 更新图像内容. 在算法处理结果之间切换显示.
    void updateImageKeepView(const QImage &img);
    // 加载新图像并重置所有显示参数. 打开新图片文件.
    void updateImageResetView(const QImage &img);
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
private:
    // 根据窗口大小计算自适应缩放
    void adjustToWindowSize();
    // 更新缓存的图像数据
    void updateCachedPixmap();
    // 将控件坐标映射到原图坐标
    QPointF widgetToImage(const QPointF &p) const;

// 交互增删点
signals:
    void requestAddPoint(QPointF imgPos);
    void requestDeletePointsInArea(QRectF imgArea);
private:
    bool isSelecting = false;      // 是否正在框选
    QPointF selectionStart;        // 框选起点（Widget坐标）
    QPointF selectionEnd;          // 框选终点（Widget坐标）
protected:
    void mouseReleaseEvent(QMouseEvent *event) override; // 增加鼠标释放事件


// 交互对指定三角重新上色
private:
    QPolygonF highlightedTriangle; // 记录当前高亮的三角形
public:
    void setHighlightedTriangle(const QPolygonF& poly) { highlightedTriangle = poly; update(); }
signals:
    // 请求选中三角形
    void requestSelectTriangle(QPointF imgPos);
};

#endif // IMAGE_WIDGET_H
