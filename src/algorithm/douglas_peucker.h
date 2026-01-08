#ifndef DOUGLAS_PEUCKER_H
#define DOUGLAS_PEUCKER_H

#include <QPoint>
#include <vector>
#include "algorithm_params.h"

// 曲线简化算法, 采疏
class DouglasPeucker
{
public:
    // 构造函数：初始化论文参数
    DouglasPeucker(double epsilon = 1.5, double eta = 0.02);

    // 核心接口：输入底层结果，输出 Qt 坐标点集
    void simplify(const ScopedEDResults& edResults, int imgW, int imgH);
    QImage drawPointImage(int width, int height);
    QImage drawLineImage(int width, int height);


public:
    DPParams dpParams;
    // 二维容器: 外层vector是线(edge drawing算法中的一个EDEdgeSegment)的集合, 内层 vector 是该线上的点
    std::vector<std::vector<QPoint>> dpResults;
    // 四角点单独存放，因为它们不是"线", 而是算法约束点
    std::vector<QPoint> cornerPoints;
    // 论文中的 Li. 当前计算出的长度阈值
    double sampleInterval;

private:
    void processSegment(const std::vector<QPoint>& segment,
                        int start, int end, std::vector<int>& outIndices);
    void processSegment(const opencved::EDPoint* points, // 直接传入原始指针
                   int start, int end,
                        std::vector<int>& outIndices);

    // 辅助计算：点到线段距离
    double distToSegment(const QPoint& p, const QPoint& a, const QPoint& b);
    double distToSegment(const opencved::EDPoint& p, const opencved::EDPoint& a, const opencved::EDPoint& b);

    // 辅助计算：两点欧氏距离
    double distance(const QPoint& a, const QPoint& b);
    double distance(const opencved::EDPoint& a, const opencved::EDPoint& b);
};

#endif // DOUGLAS_PEUCKER_H
