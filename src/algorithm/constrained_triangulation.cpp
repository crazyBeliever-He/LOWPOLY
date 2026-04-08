#include <QPainter>
#include <random>

#include "constrained_triangulation.h"
#include "logger.h"
#include "CDT/include/CDT.h" // 引入 artem-ogre/CDT 核心头文件

// 内部辅助结构，用于向 CDT 库传递约束边
struct ConstrainedEdge {
    std::size_t v1;
    std::size_t v2;
};

bool ConstrainedTriangulation::generateLowPolyMesh(
    const std::vector<QPointF>& optimizedPoints,
    const std::vector<std::vector<QPoint>>& dpResults,
    const std::vector<QPoint>& cornerPoints)
{
    allVertices.clear();
    triangles.clear();

    // 1. 使用 CDT 库原生结构存储点和边
    std::vector<CDT::V2d<float>> cdtVertices;
    std::vector<CDT::Edge> cdtEdges;

    std::size_t totalPoints = optimizedPoints.size() + cornerPoints.size();
    for (const auto& line : dpResults) {
        totalPoints += line.size();
    }
    cdtVertices.reserve(totalPoints);

    // 2. 压入优化后的自由点
    for (const auto& pt : optimizedPoints) {
        CDT::V2d<float> v;
        v.x = static_cast<float>(pt.x());
        v.y = static_cast<float>(pt.y());
        cdtVertices.push_back(v);
    }

    // 3. 压入四个角点
    for (const auto& pt : cornerPoints) {
        CDT::V2d<float> v;
        v.x = static_cast<float>(pt.x());
        v.y = static_cast<float>(pt.y());
        cdtVertices.push_back(v);
    }

    // 4. 构建约束边集合 (Edges List)
    for (const auto& line : dpResults) {
        if (line.size() < 2) continue;

        std::size_t startIndex = cdtVertices.size();
        for (std::size_t i = 0; i < line.size(); ++i) {
            CDT::V2d<float> v;
            v.x = static_cast<float>(line[i].x());
            v.y = static_cast<float>(line[i].y());
            cdtVertices.push_back(v);

            if (i > 0) {
                cdtEdges.push_back(CDT::Edge(CDT::VertInd(startIndex + i - 1), CDT::VertInd(startIndex + i)));
            }
        }
    }

    // 5. 剔除重叠点，重映射边索引
    CDT::RemoveDuplicatesAndRemapEdges(cdtVertices, cdtEdges);

    // 6. 执行 CDT 算法
    try {
        // 核心修复：在构造时开启相交边自动解析功能
        CDT::Triangulation<float> cdt(
            CDT::VertexInsertionOrder::Auto,
            CDT::IntersectingConstraintEdges::TryResolve,
            0.0f
            );

        cdt.insertVertices(
            cdtVertices.begin(),
            cdtVertices.end(),
            [](const CDT::V2d<float>& p) { return p.x; },
            [](const CDT::V2d<float>& p) { return p.y; }
            );

        cdt.insertEdges(
            cdtEdges.begin(),
            cdtEdges.end(),
            [](const CDT::Edge& e) { return e.v1(); },
            [](const CDT::Edge& e) { return e.v2(); }
            );

        cdt.eraseSuperTriangle();

        // 7. 保存生成的三角形拓扑 (索引指向的是 cdt.vertices)
        triangles.reserve(cdt.triangles.size());
        for (const auto& t : cdt.triangles) {
            LowPolyTriangle lpt;
            lpt.v[0] = t.vertices[0];
            lpt.v[1] = t.vertices[1];
            lpt.v[2] = t.vertices[2];
            triangles.push_back(lpt);
        }

        // 8. 由于 TryResolve 可能打断相交边并注入了新顶点，
        // 必须从 cdt.vertices 提取最终的物理坐标，同步到 allVertices 中
        allVertices.reserve(cdt.vertices.size());
        for (const auto& v : cdt.vertices) {
            allVertices.emplace_back(v.x, v.y);
        }

    } catch (const std::exception& e) {
        LOG_ERROR << "Constrained Triangulation failed: " << e.what();
        return false;
    }

    return true;
}

QImage ConstrainedTriangulation::drawTriangulationWireframe(int width, int height)
{
    // 1. 创建背景画布
    QImage canvas(width, height, QImage::Format_RGB888);
    canvas.fill(Qt::black);

    // 2. 初始化 QPainter
    QPainter painter(&canvas);
    // 开启反锯齿，消除线条的像素阶梯感，让网格极其平滑
    painter.setRenderHint(QPainter::Antialiasing);

    // 3. 设置画笔 (青蓝色，半透明，线宽 1.0)
    QPen meshPen(QColor(0, 200, 255, 180), 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(meshPen);

    // 只需要绘制线框，不需要填充内部
    painter.setBrush(Qt::NoBrush);

    // 4. 遍历并绘制所有的三角形
    for (const auto& tri : triangles) {
        // 通过索引从全局顶点池中获取 3 个顶点的真实亚像素坐标
        QPointF p1 = allVertices[tri.v[0]];
        QPointF p2 = allVertices[tri.v[1]];
        QPointF p3 = allVertices[tri.v[2]];

        // 构造 Qt 的浮点精度多边形对象
        QPolygonF polygon;
        polygon << p1 << p2 << p3;

        // 直接绘制该三角形的线框
        painter.drawPolygon(polygon);
    }

    // 5. (可选) 想突出显示顶点，可以再遍历一次 allVertices 画点
    /*
    QPen pointPen(QColor(255, 100, 100, 200), 2.0);
    painter.setPen(pointPen);
    painter.drawPoints(allVertices.data(), static_cast<int>(allVertices.size()));
    */

    painter.end();
    return canvas;
}

// =====================================================================
// 色彩空间转换辅助函数 (RGB <-> CIELAB)
// =====================================================================
namespace {
struct LabColor {
    float L, a, b;
};

// 标准 sRGB 到 CIELAB 转换 (参考 D65 白点)
LabColor rgb2lab(int R, int G, int B) {
    float r = R / 255.0f, g = G / 255.0f, b = B / 255.0f;

    r = (r > 0.04045f) ? std::pow((r + 0.055f) / 1.055f, 2.4f) : r / 12.92f;
    g = (g > 0.04045f) ? std::pow((g + 0.055f) / 1.055f, 2.4f) : g / 12.92f;
    b = (b > 0.04045f) ? std::pow((b + 0.055f) / 1.055f, 2.4f) : b / 12.92f;

    float x = (r * 0.4124564f + g * 0.3575761f + b * 0.1804375f) / 0.95047f;
    float y = (r * 0.2126729f + g * 0.7151522f + b * 0.0721750f) / 1.00000f;
    float z = (r * 0.0193339f + g * 0.1191920f + b * 0.9503041f) / 1.08883f;

    auto f = [](float t) {
        return (t > 0.008856f) ? std::cbrt(t) : (7.787f * t) + (16.0f / 116.0f);
    };

    float fx = f(x), fy = f(y), fz = f(z);
    return { 116.0f * fy - 16.0f, 500.0f * (fx - fy), 200.0f * (fy - fz) };
}

// CIELAB 到标准 sRGB 转换
void lab2rgb(float L, float a, float b, int& R, int& G, int& B) {
    float fy = (L + 16.0f) / 116.0f;
    float fx = a / 500.0f + fy;
    float fz = fy - b / 200.0f;

    auto invf = [](float t) {
        float t3 = t * t * t;
        return (t3 > 0.008856f) ? t3 : (t - 16.0f / 116.0f) / 7.787f;
    };

    float x = invf(fx) * 0.95047f;
    float y = invf(fy) * 1.00000f;
    float z = invf(fz) * 1.08883f;

    float r = x * 3.2404542f + y * -1.5371385f + z * -0.4985314f;
    float g = x * -0.9692660f + y * 1.8760108f + z * 0.0415560f;
    float bl= x * 0.0556434f + y * -0.2040259f + z * 1.0572252f;

    auto comp = [](float c) {
        return (c > 0.0031308f) ? 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f : 12.92f * c;
    };

    R = std::max(0, std::min(255, static_cast<int>(std::round(comp(r) * 255.0f))));
    G = std::max(0, std::min(255, static_cast<int>(std::round(comp(g) * 255.0f))));
    B = std::max(0, std::min(255, static_cast<int>(std::round(comp(bl)* 255.0f))));
}
}

QImage ConstrainedTriangulation::drawLowPolyResult(const QImage& originalImage)
{
    if (originalImage.isNull() || triangles.empty() || allVertices.empty()) {
        return QImage();
    }

    int width = originalImage.width();
    int height = originalImage.height();

    QImage srcImage = originalImage.convertToFormat(QImage::Format_RGB888);
    QImage result(width, height, QImage::Format_RGB888);
    result.fill(Qt::black);

    QPainter painter(&result);
    //painter.setRenderHint(QPainter::Antialiasing);

    // 随机数生成器用于颜色扰动 (论文中 L 分量加入 -10 到 10 的噪声)
    // TODO: 每张图都边缘都会出现黑线，目前未知原因，待修改
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> noiseDist(-9.9f, 9.9f);

    for (const auto& tri : triangles) {
        QPointF p1 = allVertices[tri.v[0]];
        QPointF p2 = allVertices[tri.v[1]];
        QPointF p3 = allVertices[tri.v[2]];

        QPolygonF polygon;
        polygon << p1 << p2 << p3;

        QRectF boundingRect = polygon.boundingRect();
        int minX = std::max(0, static_cast<int>(std::floor(boundingRect.left())));
        int minY = std::max(0, static_cast<int>(std::floor(boundingRect.top())));
        int maxX = std::min(width - 1, static_cast<int>(std::ceil(boundingRect.right())));
        int maxY = std::min(height - 1, static_cast<int>(std::ceil(boundingRect.bottom())));

        std::vector<LabColor> pixelColors;
        // 预分配足够的空间提升性能
        pixelColors.reserve((maxX - minX + 1) * (maxY - minY + 1));

        for (int y = minY; y <= maxY; ++y) {
            const uchar* scanLine = srcImage.constScanLine(y);
            for (int x = minX; x <= maxX; ++x) {
                if (polygon.containsPoint(QPointF(x + 0.5, y + 0.5), Qt::OddEvenFill)) {
                    int offset = x * 3;
                    // 将提取的 RGB 立即转换至 Lab 空间
                    pixelColors.push_back(rgb2lab(scanLine[offset], scanLine[offset + 1], scanLine[offset + 2]));
                }
            }
        }

        QColor fillColor;

        if (!pixelColors.empty()) {
            // 1. 消除伪影：按照 Lab 的 L (亮度) 分量对三角形内所有像素进行排序
            std::sort(pixelColors.begin(), pixelColors.end(), [](const LabColor& c1, const LabColor& c2) {
                return c1.L < c2.L;
            });

            // 2. 取中间部分：提取 40% 到 60% 区间，避免极端的亮部或暗部拉偏整体颜色
            std::size_t startIdx = static_cast<std::size_t>(pixelColors.size() * 0.40);
            std::size_t endIdx = static_cast<std::size_t>(pixelColors.size() * 0.60);
            if (endIdx <= startIdx) { endIdx = startIdx + 1; } // 防退化

            float sumL = 0.0f, sumA = 0.0f, sumB = 0.0f;
            float count = static_cast<float>(endIdx - startIdx);

            for (std::size_t i = startIdx; i < endIdx; ++i) {
                sumL += pixelColors[i].L;
                sumA += pixelColors[i].a;
                sumB += pixelColors[i].b;
            }

            float avgL = sumL / count;
            float avgA = sumA / count;
            float avgB = sumB / count;

            // 3. 颜色扰动：给 L 通道加入噪声，使得扁平的多边形也带有些微明暗起伏的艺术感
            float currentNoise = noiseDist(rng);

            // 如果区域极亮（接近纯白）或极暗（接近纯黑），直接取消噪声，防止画面变脏
            if (avgL > 95.0f || avgL < 5.0f) {
                currentNoise = 0.0f;
            }
            else
            {
                // 可选进阶: 平滑衰减, 越靠近边界, 噪声越弱, 避免硬截断带来的视觉割裂
                // 距离中心灰度(50)越远，衰减系数越小
                float distanceToCenter = std::abs(avgL - 50.0f);
                float attenuation = 1.0f - std::pow(distanceToCenter / 50.0f, 4.0f);
                currentNoise *= attenuation;
            }

            avgL += currentNoise;
            avgL = std::max(0.0f, std::min(100.0f, avgL)); // L 值域约束 [0, 100]

            // 还原回 RGB 颜色
            int R, G, B;
            lab2rgb(avgL, avgA, avgB, R, G, B);
            fillColor.setRgb(R, G, B);

            // 4. 后处理：提升饱和度使其更鲜艳 (论文中指出需提升饱和度以强化艺术感)
            int h, s, v;
            fillColor.getHsv(&h, &s, &v);
            // 设定一个艺术化缩放系数，例如提高 30% 的饱和度
            // 只有当本身具有一定饱和度时才进行放大，避免将接近灰/白的杂色放大为明显的色块
            if (s > 10) {
                s = std::min(255, static_cast<int>(s * 1.3));
            }
            fillColor.setHsv(h, s, v);

        } else {
            // 极其狭长的三角形的极限回退机制
            QPointF centroid = (p1 + p2 + p3) / 3.0;
            int cx = std::max(0, std::min(width - 1, static_cast<int>(centroid.x())));
            int cy = std::max(0, std::min(height - 1, static_cast<int>(centroid.y())));

            const uchar* scanLine = srcImage.constScanLine(cy);
            int offset = cx * 3;
            fillColor = QColor(scanLine[offset], scanLine[offset + 1], scanLine[offset + 2]);
        }

        // 依然保留缝隙消除技术 (利用 QPen 和 QBrush 重合画边)，保证在 Qt 渲染器下网格致密无黑边
        QPen pen(fillColor, 0.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(QBrush(fillColor));

        painter.drawPolygon(polygon);
    }

    painter.end();
    return result;
}

