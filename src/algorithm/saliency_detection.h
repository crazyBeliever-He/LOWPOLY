#ifndef SALIENCY_DETECTION_H
#define SALIENCY_DETECTION_H

#include <QImage>
#include "algorithm_params.h"

struct ColorBin {
    int r, g, b;            // 颜色的 RGB 值
    float probability;      // 出现概率 (像素数 / 总像素)
    float saliency;         // 最终显著性值

    // 用于 CSD (空间分布) 计算
    double sumX, sumY;      // 坐标和
    double sumX2, sumY2;    // 坐标平方和
    int count;              // 属于该 Bin 的像素数量

    ColorBin() : r(0), g(0), b(0), probability(0), saliency(0),
        sumX(0), sumY(0), sumX2(0), sumY2(0), count(0) {}
};

/********************************************************************************/
// Zhu W, Liang S, Wei Y, et al.
// Saliency optimization from robust background detection[C]
// Proceedings of the IEEE conference on computer vision and pattern recognition. 2014: 2814-2821.
/********************************************************************************/

//
class SaliencyDetection
{
public:
    SDParams sDParams;
    SaliencyDetection();
    // 统一的外部调用接口
    QImage saliencyDetectionInterface(const QImage &input);

    // 显著性采样, 在显著性检测之后使用，获取 N-Nc 个随机点
    std::vector<QPoint> generateNonUniformSamples(
        int Lw, int Lh,                 // 图像宽高
        int Nc,                         // 已知约束点数量（特征点 + 4个角点）
        double eta,                     // 采样间隔控制参数
        const QImage& saliencyMap      // 显著性/掩码图 (二维数组)
        );

//----------论文作者提供版saliency detection----------
public:
    QImage saliencyDetectionApi(const QImage &input);
    QImage saliencyDetectionExe(const QImage &input);
//----------简化版 saliency detection----------
public:
    // 主函数：输入 QImage，输出灰度显著性图
    static QImage getSaliencyDetection(const QImage &input);
private:
    // 计算两个颜色的欧氏距离
    static float colorDist(const ColorBin &c1, const ColorBin &c2);
};

#endif // SALIENCY_DETECTION_H
