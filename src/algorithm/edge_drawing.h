#ifndef EDGE_DRAWING_H
#define EDGE_DRAWING_H

#include <vector>
#include <QImage>
#include "algorithm_params.h"

// 输入图片->灰度图->高斯滤波->智能路由
// TODO: 修改自实现的edge drawing gray 算法, 改成gray和color两个版本都能用
struct FloatImage {
    int width;
    int height;
    std::vector<float> data;
    float at(int x, int y) const { return data[y * width + x]; }
};

struct GradientInfo {
    int width;
    int height;
    std::vector<float> magnitude;
    std::vector<uchar> direction;
};

struct EdgeChain {
    std::vector<QPoint> points;
};

class EdgeDrawing
{
// ---------- opencv-contrib edge drawing ----------
public:
    opencved::EDParams edParams;    // 参数
    ScopedEDResults edResults;  // 结果
    // edge drawing from opencv-contrib
    void edgeDrawingInLib(const QImage &originalImage);
    // 绘制
    QImage drawImage(int width, int height);

// ---------- my edge drawing ----------
public:
    enum GrayMethod { METHOD_BT601 = 0, METHOD_BT709, NUM_METHODS };
    enum DataMode { MODE_FLOAT = 0, MODE_GRAY16 };
    enum EdgeDirection { EDGE_HORIZONTAL = 0, EDGE_VERTICAL = 1 };
public:
    bool useLut;    // 是否使用伽马校正
    int stdGradientThreshold;   // 梯度阈值, 忽略梯度小于该值的点(弱像素)
    int  stdAnchorThreshold;    // 锚点选取阈值，该阈值越大锚点越少
    int scanInterval;   // 扫描第 n *scanInterval行寻找锚点
    int shortEdgeLength;    // 忽略的短边长度
private:
    float gradientThreshold;    // 锚点备选点的梯度阈值
    float anchorThreshold;  // 选锚点时的非极大值抑制参数
    std::vector<uchar> edgeMap; // 0: 未访问, 255: 已确定为边缘
    GradientInfo gradient;  // 原始梯度信息, 不做处理

public:
    EdgeDrawing();
    QImage getEDImageGray16(const QImage &origin);
    QImage getEDImageFloat(const QImage &origin);

    inline float getGrayValue(float r, float g, float b, GrayMethod method = METHOD_BT601);
    inline float getLinearValue(uchar value);
    inline float calculateInternalThreshold(int stdThreshold, DataMode mode);

    // 1. QImage::Format_Grayscale16版本
    QImage convertColorToGrayscale16(const QImage &input);  // 灰度图
    QImage gaussianBlurGrayscale16(const QImage &image, int kSize = 5, float sigma = 1.0f); // 高斯模糊
    GradientInfo computeGradientGrayscale16(const QImage &image);   // 梯度计算

    // 1. FloatImage(自定义)版本
    FloatImage convertColorToGrayFloat(const QImage &img);  // 灰度图
    FloatImage gaussianBlurFloat(const FloatImage &input, int kSize = 5, float sigma = 1.0f);   // 高斯模糊
    GradientInfo computeGradientFloat(const FloatImage &input); // 梯度计算

    // 2. 选取锚点 + 智能路由
    std::vector<QPoint> extractAnchors();   // 选取锚点
    std::vector<EdgeChain> graySmartRouting(const std::vector<QPoint> &anchors);    // 智能路由
    void route(EdgeChain &chain, int x, int y); // 路由
    void routeStack(EdgeChain &chain, int startX, int startY);      // 路由的栈版本
    QPoint findBestNeighbor(int targetX, int targetY, bool isVerticalScan); // 路由选择的方式
    QImage drawEdgeChains(int width, int height, const std::vector<EdgeChain> &chains); // 绘制结果

// constexpr 常量表达式在编译时就能计算确定的值
private:
    static constexpr int LUT_SIZE = 256;
    static float lut[LUT_SIZE];
    static bool lutInitialized;
    static void initializeLUT();    // 初始化sRGB到线性RGB的查找表(LUT)
};

#endif // EDGE_DRAWING_H
