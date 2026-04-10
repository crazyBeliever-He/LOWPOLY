#include <QPainter>
#include <Qdebug>

#include "vertex_optimization.h"
#include "VertexOptimization.h"
#include "logger.h"

bool VertexOptimization::vertexOptimizationApi(int width, int height, int Nc,
    const std::vector<QPoint>& sampledPoints,
    const std::vector<std::vector<QPoint>>& dpResults,
    const std::vector<QPoint>& cornerPoints,
    const std::vector<float>& featureFlowMap,
    int iterations)
{

    // 1. 基础参数校验
    if (width <= 0 || height <= 0 || sampledPoints.empty())
    {
        //qDebug()<<width<< height<< sampledPoints.empty();
        LOG_ERROR << "Vertex Optimization 启动失败: 图像尺寸无效或没有自由采样点";
        return false;
    }

    if (featureFlowMap.size() != static_cast<size_t>(width * height))
    {
        LOG_ERROR << "Vertex Optimization 启动失败: 特征流场大小与图像尺寸不匹配";
        return false;
    }

    // 2. 准备自由点 (Free Points)
    std::vector<VertexOpt::OptPoint> freePointsBuf;
    freePointsBuf.reserve(sampledPoints.size());
    for (const auto& pt : sampledPoints)
    {
        freePointsBuf.push_back({static_cast<float>(pt.x()), static_cast<float>(pt.y())});
    }

    // 3. 准备约束点 (Constrained Points)
    std::vector<VertexOpt::OptPoint> constrainedPointsBuf;
    constrainedPointsBuf.reserve(static_cast<size_t>(Nc));
    // 压入四个角点
    for (const auto& pt : cornerPoints)
    {
        constrainedPointsBuf.push_back({static_cast<float>(pt.x()), static_cast<float>(pt.y())});
    }
    // 压入 Douglas-Peucker 提取的边缘关键点 (展平二维数组)
    for (const auto& line : dpResults)
    {
        for (const auto& pt : line)
        {
            constrainedPointsBuf.push_back({static_cast<float>(pt.x()), static_cast<float>(pt.y())});
        }
    }

    // 4. 调用 DLL 接口
    char dll_status_msg[512] = {0};

    bool success = VertexOpt::optimize_vertices_api(
        freePointsBuf.data(),
        static_cast<int>(freePointsBuf.size()),
        constrainedPointsBuf.data(),
        static_cast<int>(constrainedPointsBuf.size()),
        featureFlowMap.data(),
        width,
        height,
        iterations,
        dll_status_msg,
        sizeof(dll_status_msg)
        );

    // 5. 处理结果
    if (!success)
    {
        LOG_ERROR << "Vertex Optimization DLL 调用失败: " << dll_status_msg;
        return false;
    }

    // 6. 将优化后的结果写回类成员
    optimizedPoints.clear();
    optimizedPoints.reserve(freePointsBuf.size());
    for (const auto& optPt : freePointsBuf)
    {
        // 使用 QPointF 保存亚像素精度的坐标，方便后续 Constrained Delaunay Triangulation 使用
        optimizedPoints.emplace_back(optPt.x, optPt.y);
    }

    return true;
}

QImage VertexOptimization::drawOptimizedVertices(int width, int height,
                                                 const std::vector<std::vector<QPoint>>& dpResults,
                                                 const std::vector<QPoint>& cornerPoints)
{
    // 1. 创建背景画布
    QImage canvas(width, height, QImage::Format_RGB888);
    canvas.fill(Qt::black);

    QPainter painter(&canvas);
    // 开启反锯齿，让点和线更平滑
    //painter.setRenderHint(QPainter::Antialiasing);

    // 2. 绘制受约束的特征边缘 (Douglas-Peucker 结果)
    // 使用青色 (Cyan)，线宽 1.0，表现骨架感
    QPen edgePen(QColor(0,255,255), 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(edgePen);
    for (const auto& line : dpResults)
    {
        if (!line.empty())
        {
            //painter.drawPolyline(line.data(), static_cast<int>(line.size()));
            // 只绘制受约束的特征边缘上的点 (将原来的线改为点)
            painter.drawPoints(line.data(), static_cast<int>(line.size()));
        }
    }

    // 3. 绘制受约束的角点
    // 使用白色，稍微加粗
    QPen cornerPen(Qt::white, 1.0, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(cornerPen);
    if (!cornerPoints.empty())
    {
        painter.drawPoints(cornerPoints.data(), static_cast<int>(cornerPoints.size()));
    }

    // 4. 绘制优化后的自由点 (亚像素精度 QPointF)
    // 使用亮橙红 (Tomato/Coral)，大小适中，与冷色的约束边形成强烈对比
    QPen freePointPen(Qt::red, 1.0, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(freePointPen);
    if (!optimizedPoints.empty())
    {
        // QPainter 支持直接传入 QPointF 数组进行亚像素级别的绘制
        painter.drawPoints(optimizedPoints.data(), static_cast<int>(optimizedPoints.size()));
    }

    painter.end();
    return canvas;
}
