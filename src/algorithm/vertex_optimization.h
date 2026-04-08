#ifndef VERTEX_OPTIMIZATION_H
#define VERTEX_OPTIMIZATION_H

#include <QImage>
#include <QPoint>

#include "algorithm_params.h"

class VertexOptimization
{
public:
    std::vector<QPointF> optimizedPoints; // 存放优化后的高精度自由点
    VertexOptimization() = default;
    /**
     * @brief 顶点优化封装接口
     * @param width 图像宽度
     * @param height 图像高度
     * @param Nc 约束点总数
     * @param sampledPoints SaliencyDetection 产生的自由点
     * @param dpResults DouglasPeucker 产生的边缘点集（二维）
     * @param cornerPoints DouglasPeucker 产生的四角点
     * @param featureFlowMap JumpFlooding 产生的特征流场 F(x) 数组
     * @param iterations 迭代次数 (默认10)
     * @return true 成功，false 失败
     */
    bool vertexOptimizationApi(int width,int height, int Nc,
                               const std::vector<QPoint>& sampledPoints,
                               const std::vector<std::vector<QPoint>>& dpResults,
                               const std::vector<QPoint>& cornerPoints,
                               const std::vector<float>& featureFlowMap,
                               int iterations = 10);

    // 考虑以下两种变体:
    // 1. 特征流场叠加: 把 drawFeatureFlowMap 生成的单通道灰度图作为底图画上去
    // 2. 位移矢量图 (Before & After): 用 QPainter::drawLine 从原始位置画一条半透明的细线指向 optimizedPoints 的新位置
    QImage drawOptimizedVertices(int width, int height,
                                 const std::vector<std::vector<QPoint>>& dpResults,
                                 const std::vector<QPoint>& cornerPoints);
};


#endif // VERTEX_OPTIMIZATION_H
