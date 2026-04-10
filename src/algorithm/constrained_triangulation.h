#ifndef CONSTRAINED_TRIANGULATION_H
#define CONSTRAINED_TRIANGULATION_H

#include <vector>
#include <QPoint>
#include <QPointF>
#include <QImage>

// 定义最终的低多边形三角形结构，存储其在顶点池中的 3 个索引
struct LowPolyTriangle {
    std::size_t v[3];
};

class ConstrainedTriangulation
{
public:
    // 存储用于最终渲染的全部顶点 (自由点 + 角点 + DP特征点)
    std::vector<QPointF> allVertices;

    // 存储最终生成的三角形拓扑结构
    std::vector<LowPolyTriangle> triangles;

    ConstrainedTriangulation() = default;
    ~ConstrainedTriangulation() = default;

    /**
     * @brief 执行受约束三角剖分生成 Low Poly 网格
     * @param optimizedPoints 顶点优化后的高精度自由点
     * @param dpResults Douglas-Peucker 简化的特征边缘点集
     * @param cornerPoints 图像的四个角点
     * @return true 成功，false 失败
     */
    bool generateLowPolyMesh(
        const std::vector<QPointF>& optimizedPoints,
        const std::vector<std::vector<QPoint>>& dpResults,
        const std::vector<QPoint>& cornerPoints
        );

    /**
     * @brief 绘制未上色的三角剖分线框图 (Wireframe)
     * @param width 图像宽度
     * @param height 图像高度
     * @return 包含三角网格线框的 QImage
     */
    QImage drawTriangulationWireframe(int width, int height);

    /**
     * @brief 绘制最终的 Low Poly 艺术风格图像
     * @param originalImage 原始输入图像（用于提取颜色）
     * @return 渲染完毕的低多边形图像
     */
    QImage drawLowPolyResult(const QImage& originalImage);


//
public:
    // 缓存每个三角形算好的颜色，用于极速重绘
    std::vector<QColor> cachedColors;
    // 用户自定义的颜色覆盖字典 (Key: 三角形索引, Value: 颜色)
    std::unordered_map<int, QColor> customColors;
    // 交互支持接口
    int findTriangleAt(QPointF pt) const; // 根据坐标找三角形索引，找不到返回 -1
    QPolygonF getTrianglePolygon(int triIndex) const; // 获取三角形的多边形对象
    QImage redrawLowPolyFast(const QImage& originalImage); // 使用 cachedColors 和 customColors 极速重绘
};

#endif // CONSTRAINED_TRIANGULATION_H
