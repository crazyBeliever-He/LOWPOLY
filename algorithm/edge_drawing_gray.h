#ifndef EDGE_DRAWING_GRAY_H
#define EDGE_DRAWING_GRAY_H

#include <vector>
#include <QImage>

// opencv实现了edge drawing算法
// 输入图片->灰度图->高斯滤波(a 5 5 Gaussian kernel with r = 1 is used)
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

class GrayEdgeDrawing
{
public:
    enum GrayMethod { METHOD_BT601 = 0, METHOD_BT709, NUM_METHODS };
    enum DataMode { MODE_FLOAT = 0, MODE_GRAY16 };
    enum EdgeDirection { EDGE_HORIZONTAL = 0, EDGE_VERTICAL = 1 };

    GrayEdgeDrawing();
    QImage getEDImageGray16(const QImage &origin);
    QImage getEDImageFloat(const QImage &origin);

    inline float getGrayValue(float r, float g, float b, GrayMethod method = METHOD_BT601);
    inline float getLinearValue(uchar value);
    inline float calculateInternalThreshold(int stdThreshold, DataMode mode);

    //QImage::QImage::Format_Grayscale16版本
    QImage convertColorToGrayscale16(const QImage &input);
    QImage gaussianBlurGrayscale16(const QImage &image, int kSize = 5, float sigma = 1.0f);
    GradientInfo computeGradientGrayscale16(const QImage &image);

    // FloatImage(自定义)版本
    FloatImage convertColorToGrayFloat(const QImage &img);
    FloatImage gaussianBlurFloat(const FloatImage &input, int kSize = 5, float sigma = 1.0f);
    /* 梯度计算 Prewitt operator */
    GradientInfo computeGradientFloat(const FloatImage &input);


    // 选取锚点 + 智能路由
    std::vector<QPoint> extractAnchors();
    std::vector<EdgeChain> graySmartRouting(const std::vector<QPoint> &anchors);
    void route(EdgeChain &chain, int x, int y);
    // 栈
    void routeStack(EdgeChain &chain, int startX, int startY);
    QPoint findBestNeighbor(int targetX, int targetY, bool isVerticalScan);

    // 绘制结果
    QImage drawEdgeChains(int width, int height, const std::vector<EdgeChain> &chains);

public:
    /* 是否使用伽马校正 */
    bool useLut;
    /* 梯度阈值, 忽略梯度小于该值的点(弱像素) */
    int stdGradientThreshold;
    /* 锚点选取阈值，该阈值越大锚点越少 */
    int  stdAnchorThreshold;
    /* 扫描第 n *scanInterval行  寻找锚点 */
    int scanInterval;
    /* 忽略短边 */
    int shortEdgeLength;

private:
    /* 锚点备选点的梯度阈值 */
    float gradientThreshold;
    /* 选锚点时的非极大值抑制参数 */
    float anchorThreshold;
    /* 0: 未访问, 255: 已确定为边缘 */
    std::vector<uchar> edgeMap;
    /* 原始梯度信息, 不做处理 */
    GradientInfo gradient;

    static constexpr int LUT_SIZE = 256;
    static float lut[LUT_SIZE];
    static bool lutInitialized;
    static void initializeLUT();

    // static constexpr 成员可以直接在类定义中初始化
    // static 非 constexpr 成员必须在类外进行初始化(即使它们是数组或普通变量)
    // reinterpret_cast可能存在潜在风险

};

#endif // EDGE_DRAWING_GRAY_H
