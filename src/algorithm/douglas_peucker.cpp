#include "douglas_peucker.h"

#include <algorithm>
#include <qimage.h>
#include <QPainter>

DouglasPeucker::DouglasPeucker(double epsilon, double eta)
{
    dpParams.epsilon = epsilon;
    dpParams.eta = eta;
}

void DouglasPeucker::simplify(const ScopedEDResults& edResults, int width, int height)
{
    // 1. 清理旧结果并预分配空间, 也可以考虑粗略预分配空间
    dpResults.clear();
    cornerPoints.clear();

    // 2. 按照论文公式计算离散化的整数步长 Li
    sampleInterval = static_cast<int>(std::round(dpParams.eta * (width + height)));
    sampleInterval = sampleInterval > 1 ? sampleInterval : 1;

    // 3. 遍历每一个边缘段(Segment)
    for (int i = 0; i < edResults.data.segmentCount; ++i)
    {
        const auto& seg = edResults.data.segments[i];
        if (seg.count < 2) continue;

        // --- 每一条线独立处理 ---
        std::vector<int> keptIndices;

// --- 选择编译逻辑开始 ---
#define USE_ZERO_COPY_OPTIMIZATION 1
// 优先使用const opencved::EDPoint* points.
// 当: 异步处理或多线程, 数据需要频繁"原地修改", 数据持久化需求. 使用情况B: const std::vector<QPoint>& segment
#if USE_ZERO_COPY_OPTIMIZATION
        // 情况 A: 原始指针版本 - 追求极致性能，零拷贝
        processSegment(seg.points, 0, seg.count - 1, keptIndices);
#else
        // 情况 B: Qt 容器版本 - 适用于异步处理或需要原地修改数据的场景
        将 C 结构体转换为 std::vector<QPoint>，方便使用 Qt 特性
        std::vector<QPoint> currentSegment;
        currentSegment.reserve(seg.count);
        for (int j = 0; j < seg.count; ++j)
        {   //emplace_back 和 push_back 有差异
            currentSegment.emplace_back(seg.points[j].x, seg.points[j].y);
        }
        processSegment(currentSegment, 0, currentSegment.size() - 1, keptIndices);
#endif
// --- 选择编译逻辑结束 ---

        // 4. 排序并去重索引
        //std::unique 会将相邻的重复元素移到容器的末尾, 并返回一个迭代器, 指向第一个需要移除的元素的位置
        std::sort(keptIndices.begin(), keptIndices.end());
        keptIndices.erase(std::unique(keptIndices.begin(), keptIndices.end()), keptIndices.end());

        // 5. 将当前段的简化点存入一个独立的 vector
        std::vector<QPoint> simplifiedLine;
        simplifiedLine.reserve(keptIndices.size());
        for (int idx : keptIndices)
        {   // 将结果转为 QPoint 存入 finalPoints
            simplifiedLine.emplace_back(seg.points[idx].x, seg.points[idx].y);
        }
        // 存入二维容器
        dpResults.push_back(std::move(simplifiedLine));
    }

    // 6. 四角点存入独立容器(不混入线段中)
    cornerPoints.push_back(QPoint(0, 0));
    cornerPoints.push_back(QPoint(width - 1, 0));
    cornerPoints.push_back(QPoint(0, height - 1));
    cornerPoints.push_back(QPoint(width - 1, height - 1));
}

int DouglasPeucker::getDPPointsNumber()
{
    int Nc = 4;
    for (const auto& line : dpResults) {
        Nc += static_cast<int>(line.size());
    }
    return Nc;
}

void DouglasPeucker::processSegment(const std::vector<QPoint>& segment,
                                    int start, int end, std::vector<int>& outIndices)
{
    if (start >= end) return;

    // 寻找最远点
    double maxDistance = -1.0;
    int farthestIdx = start;
    for (int i = start + 1; i < end; ++i)
    {
        double d = distToSegment(segment[i], segment[start], segment[end]);
        if (d > maxDistance)
        {
            maxDistance = d;
            farthestIdx = i;
        }
    }

    double segmentLength = distance(segment[start], segment[end]);

    // 结合 Gai & Wang 论文的判定条件
    if (maxDistance > dpParams.epsilon)
    {   // 弯曲度太大 -> 分裂
        processSegment(segment, start, farthestIdx, outIndices);
        processSegment(segment, farthestIdx, end, outIndices);
    }
    else if (segmentLength > sampleInterval)
    {   // 太直但太长 -> 在索引中点强制分裂
        int mid = (start + end) / 2;
        processSegment(segment, start, mid, outIndices);
        processSegment(segment, mid, end, outIndices);
    }
    else
    {   // 满足简化条件
        outIndices.push_back(start);
        outIndices.push_back(end);
    }
}

void DouglasPeucker::processSegment(const opencved::EDPoint* points, // 直接传入原始指针
                                    int start, int end,
                                    std::vector<int>& outIndices)
{
    if (start >= end) return;

    // 寻找最远点
    double maxDistance = -1.0;
    int farthestIdx = start;

    // 使用指针访问
    const auto& pStart = points[start];
    const auto& pEnd = points[end];

    for (int i = start + 1; i < end; ++i)
    {
        double d = distToSegment(points[i], pStart, pEnd);
        if (d > maxDistance)
        {
            maxDistance = d;
            farthestIdx = i;
        }
    }

    double segmentLength = distance(pStart, pEnd);

    // 结合 Gai & Wang 论文的判定条件
    if (maxDistance > dpParams.epsilon)
    {
        // 弯曲度太大 -> 分裂
        processSegment(points, start, farthestIdx, outIndices);
        processSegment(points, farthestIdx, end, outIndices);
    }
    else if (segmentLength > sampleInterval)
    {
        // 太直但太长 -> 在索引中点强制分裂
        int mid = (start + end) / 2;
        processSegment(points, start, mid, outIndices);
        processSegment(points, mid, end, outIndices);
    }
    else
    {
        // 满足简化条件
        outIndices.push_back(start);
        outIndices.push_back(end);
    }
}

double DouglasPeucker::distToSegment(const QPoint& p, const QPoint& a, const QPoint& b)
{
    double l2 = std::pow(a.x()-b.x(), 2) + std::pow(a.y()-b.y(), 2);
    // 如果线段两点重合，直接计算点到点的距离
    if (l2 == 0.0) return distance(p, a);
    // 计算投影参数 t (点 P 在直线 AB 上的投影位置)
    double t = ((p.x() - a.x()) * (b.x() - a.x()) + (p.y() - a.y()) * (b.y() - a.y())) / l2;
    // 将 t 限制在 [0, 1] 范围内，使其落在水平线段内
    t = std::max(0.0, std::min(1.0, t));
    // 计算并返回投影点坐标并求距离
    return distance(p, QPoint(a.x() + t * (b.x() - a.x()), a.y() + t * (b.y() - a.y())));
}

double DouglasPeucker::distance(const QPoint& a, const QPoint& b)
{
    // 利用算术重载实现简洁的距离计算
    QPoint diff = a - b;
    return std::sqrt(diff.x() * diff.x() + diff.y() * diff.y());
}

double DouglasPeucker::distToSegment(const opencved::EDPoint& p, const opencved::EDPoint& a, const opencved::EDPoint& b)
{
    double dx_ba = static_cast<double>(b.x) - a.x;
    double dy_ba = static_cast<double>(b.y) - a.y;

    // 线段长度的平方 (L2 norm squared)
    double l2 = dx_ba * dx_ba + dy_ba * dy_ba;

    // 如果线段两点重合，直接计算点到点的距离
    if (l2 == 0.0) return distance(p, a);

    // 计算投影参数 t (点 P 在直线 AB 上的投影位置)
    double dx_pa = static_cast<double>(p.x) - a.x;
    double dy_pa = static_cast<double>(p.y) - a.y;

    double t = (dx_pa * dx_ba + dy_pa * dy_ba) / l2;

    // 将 t 限制在 [0, 1] 范围内，使其落在水平线段内
    t = std::max(0.0, std::min(1.0, t));

    // 计算投影点坐标并求距离
    double projX = a.x + t * dx_ba;
    double projY = a.y + t * dy_ba;

    double finalDX = p.x - projX;
    double finalDY = p.y - projY;

    return std::sqrt(finalDX * finalDX + finalDY * finalDY);
}

double DouglasPeucker::distance(const opencved::EDPoint& a, const opencved::EDPoint& b)
{
    double dx = static_cast<double>(a.x) - b.x;
    double dy = static_cast<double>(a.y) - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

QImage DouglasPeucker::drawPointImage(int width, int height)
{
    QImage edgeMap(width, height, QImage::Format_RGB888);
    edgeMap.fill(Qt::black);

    // 预先获取所有行的首地址
    QVector<uchar*> linePtrs(height);
    for (int y = 0; y < height; ++y)
    {
        linePtrs[y] = edgeMap.scanLine(y);
    }

    // 遍历所有简化的线段
    for (size_t i = 0; i < dpResults.size(); ++i)
    {
        const std::vector<QPoint>& line = dpResults[i];
        // 为每一条"简化线"分配颜色
        int hue = (i * 137) % 360;
        QColor segmentColor = QColor::fromHsv(hue, 255, 255);
        int r = segmentColor.red(), g = segmentColor.green(), b = segmentColor.blue();
        // 遍历该线段上的所有点
        for (const QPoint& pt : line)
        {
            if (pt.x() >= 0 && pt.x() < width && pt.y() >= 0 && pt.y() < height)
            {
                uchar* pixel = linePtrs[pt.y()] + (pt.x() * 3);
                pixel[0] = r;
                pixel[1] = g;
                pixel[2] = b;
            }
        }
    }
    // 最后画四角点(例如用白色)
    // for (const QPoint& pt : cornerPoints)
    // {
    //     if (pt.x() >= 0 && pt.x() < width && pt.y() >= 0 && pt.y() < height)
    //     {
    //         uchar* pixel = linePtrs[pt.y()] + (pt.x() * 3);
    //         pixel[0] = 255; pixel[1] = 255; pixel[2] = 255;
    //     }
    // }
    return edgeMap;
}
QImage DouglasPeucker::drawLineImage(int width, int height)
{
    // 1. 创建画布
    QImage edgeMap(width, height, QImage::Format_RGB888);
    edgeMap.fill(Qt::black);

    // 2. 初始化 Painter
    QPainter painter(&edgeMap);
    // 开启反锯齿, 这会让折线在视觉上非常丝滑,消除像素阶梯感
    painter.setRenderHint(QPainter::Antialiasing);

    // 3. 遍历每一条简化的线段
    for (size_t i = 0; i < dpResults.size(); ++i)
    {
        const std::vector<QPoint>& linePoints = dpResults[i];
        if (linePoints.empty()) continue;

        // 4. 动态计算颜色
        int hue = (i * 137) % 360;
        QColor color = QColor::fromHsv(hue, 255, 255);

        // 设置画笔: 颜色, 线宽, 线段连接样式(style线条, cap端点, join连接)
        QPen pen(color, 1.0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
        painter.setPen(pen);

        // 5. 绘制折线
        // 直接利用 Qt 的 drawPolyline 接口, 它能一次性画出 vector 里的连续线段
        painter.drawPolyline(linePoints.data(), static_cast<int>(linePoints.size()));
    }

    // 6. 绘制四角点(可选,作为参考特征点)
    painter.setPen(QPen(Qt::white, 2)); // 白色加粗点
    painter.drawPoints(cornerPoints.data(), static_cast<int>(cornerPoints.size()));

    painter.end();
    return edgeMap;
}
