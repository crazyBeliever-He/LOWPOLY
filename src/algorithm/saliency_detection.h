#ifndef SALIENCY_DETECTION_H
#define SALIENCY_DETECTION_H

#include <QImage>

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

class SaliencyDetection{

public:
    // 主函数：输入 QImage，输出灰度显著性图
    static QImage getSaliencyMap(const QImage &input);

private:
    // 计算两个颜色的欧氏距离
    static float colorDist(const ColorBin &c1, const ColorBin &c2);

};

#endif // SALIENCY_DETECTION_H
