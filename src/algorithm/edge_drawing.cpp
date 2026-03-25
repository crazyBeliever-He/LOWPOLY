#include "edge_drawing.h"

#include <QtMath>
#include <stack>
#include <QPainter>

float EdgeDrawing::lut[LUT_SIZE] = {0};
bool EdgeDrawing::lutInitialized = false;

EdgeDrawing::EdgeDrawing()
{
    if (!lutInitialized) {
        initializeLUT();
    }
    useLut = true;
    scanInterval = 1;
    stdGradientThreshold = 30;
    stdAnchorThreshold = 10;
    shortEdgeLength = 2;
}

QImage EdgeDrawing::getEDImageGray16(const QImage &origin)
{
    if (origin.isNull()) return QImage();
    // 1. 转为浮点灰度空间
    QImage fImg = convertColorToGrayscale16(origin);
    // 2. 高斯滤波 (内核5, Sigma 1.0)
    QImage blurred = gaussianBlurGrayscale16(fImg);
    // 3. 计算梯度 (结果存入类成员 m_grad)
    gradient = computeGradientGrayscale16(blurred);
    // 4. 提取锚点
    std::vector<QPoint> anchors = extractAnchors();
    // 5. 智能路由 (结果存入 m_edgeMap)
    std::vector<EdgeChain> chains = graySmartRouting(anchors);
    // 6. 绘图输出
    return drawEdgeChains(origin.width(), origin.height(), chains);
}
QImage EdgeDrawing::getEDImageFloat(const QImage &origin)
{
    if (origin.isNull()) return QImage();
    // 1. 转为浮点灰度空间
    FloatImage fImg = convertColorToGrayFloat(origin);
    // 2. 高斯滤波 (内核5, Sigma 1.0)
    FloatImage blurred = gaussianBlurFloat(fImg);
    // 3. 计算梯度 (结果存入类成员 m_grad)
    gradient = computeGradientFloat(blurred);
    // 4. 提取锚点
    std::vector<QPoint> anchors = extractAnchors();
    // 5. 智能路由 (结果存入 m_edgeMap)
    std::vector<EdgeChain> chains = graySmartRouting(anchors);
    // 6. 绘图输出
    return drawEdgeChains(origin.width(), origin.height(), chains);
}

void EdgeDrawing::initializeLUT()
{   //预先计算并存储256个sRGB值到线性RGB值的转换结果，用于加速图像处理中的色彩空间转换。
    //sRGB色彩空间使用非线性编码(伽马≈2.2),而图像处理算法需要在线性色彩空间中进行计算。
    for (int i = 0; i < 256; ++i)
    {
        float d = i / 255.0f;
        lut[i] = (d >= 0.04045f) ? std::pow((d + 0.055f) / 1.055f, 2.4f) : (d / 12.92f);
    }
    lutInitialized = true;
}

/* 默认用 BT601 */
inline float EdgeDrawing::getGrayValue(float r, float g, float b, GrayMethod method)
{
    switch (method)
    {
    case METHOD_BT601:
        return 0.299f * r + 0.587f * g + 0.114f * b;
    case METHOD_BT709:
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    default:
        return (r + g + b) / 3.0f;
    }
}

inline float EdgeDrawing::getLinearValue(uchar value)
{   // 伽马校正LUT 或 归一化normaization. 如果使用 LUT,返回查表值. 否则直接线性归一化到 [0, 1]
    return useLut ? lut[value] : (value / 255.0f);
}

inline float EdgeDrawing::calculateInternalThreshold(int stdThreshold, DataMode mode)
{   // 将 0-255 的标准梯度阈值转换为适应数据流的梯度阈值
    return (mode == MODE_FLOAT) ? (stdThreshold / 255.0f) : (stdThreshold * 257.0f);
}

QImage EdgeDrawing::convertColorToGrayscale16(const QImage &image)
{
    if (image.isNull() || image.format() == QImage::Format_Grayscale16)
        return image;

    QImage grayImage(image.size(), QImage::Format_Grayscale16);
    int w = image.width();
    int h = image.height();

    if (image.format() == QImage::Format_Grayscale8)
    {
        for (int y = 0; y < h; ++y)
        {
            ushort *destLine = reinterpret_cast<ushort*>(grayImage.scanLine(y));
            const uchar *srcLine = image.constScanLine(y);
            for (int x = 0; x < w; ++x)
            {
                float linearV = getLinearValue(srcLine[x]);
                destLine[x] = static_cast<ushort>(qBound(0.0f, linearV * 65535.0f, 65535.0f));
            }
        }
        return grayImage;
    }

    for (int y = 0; y < image.height(); ++y)
    {
        ushort *destLine = reinterpret_cast<ushort*>(grayImage.scanLine(y));
        const uchar *srcLine = image.constScanLine(y);

        for (int x = 0; x < image.width(); ++x)
        {
            const uchar* p = srcLine + (x * 3);
            float r = getLinearValue(p[0]);
            float g = getLinearValue(p[1]);
            float b = getLinearValue(p[2]);
            float grayVal = getGrayValue(r, g, b);
            destLine[x] = static_cast<ushort>(qBound(0.0f, grayVal * 65535.0f, 65535.0f));
        }
    }
    return grayImage;
}

FloatImage EdgeDrawing::convertColorToGrayFloat(const QImage &input)
{
    if (input.isNull()) return {0, 0, {}};

    FloatImage output;
    output.width = input.width();
    output.height = input.height();
    output.data.resize(output.width * output.height);

    int w = input.width();
    int h = input.height();

    const QImage::Format format = input.format();
    if (format == QImage::Format_Grayscale16)
    {   // 处理 16 位灰度图
        for (int y = 0; y < h; ++y)
        {
            const uint16_t *srcLine = reinterpret_cast<const uint16_t*>(input.constScanLine(y));
            float* destRow = &output.data[y*w];
            for (int x = 0; x < w; ++x)
            {   // 16位转为 [0.0, 1.0], 这里按线性处理，不做伽马校正
                destRow[x] = srcLine[x] / 65535.0f;
            }
        }
    }
    else if (format == QImage::Format_Grayscale8)
    {   // 处理 8 位灰度图
        for (int y = 0; y < h; ++y)
        {
            float* destRow = &output.data[y*w];
            const uchar *srcLine = input.constScanLine(y);
            for (int x = 0; x < w; ++x)
            {   // 8位灰度图通常也是 sRGB 编码的
                destRow[x] = getLinearValue(srcLine[x]);
            }
        }
    }
    else if (format == QImage::Format_RGB888)
    {   // 处理 RGB888
        for (int y = 0; y < h; ++y)
        {
            const uchar* srcLine = input.constScanLine(y);
            float* destRow = &output.data[y * w];
            for (int x = 0; x < w; ++x)
            {
                const uchar* pixel = srcLine + (x * 3);
                float r = getLinearValue(pixel[0]);
                float g = getLinearValue(pixel[1]);
                float b = getLinearValue(pixel[2]);
                destRow[x] = getGrayValue(r, g, b);
            }
        }
    }
    return output;
}

QImage EdgeDrawing::gaussianBlurGrayscale16(const QImage &image, int kSize, float sigma)
{   // defalut kernel size is 5, sigma = 1.0f, use BORDER_REFLECT_101(镜像边界)
    if(kSize <= 1 || sigma <= 0.0f) return image;
    if (image.isNull() || image.format() != QImage::Format_Grayscale16)
        return image;

    int width = image.width();
    int height = image.height();
    QImage result(image.size(), QImage::Format_Grayscale16);

    //一维高斯函数的标准正态分布公式, 因为常数项在归一化的过程中被约掉,所以无需实现常数项部分.
    std::vector<float> kernel(kSize);
    float sum = 0.0f;
    int half = kSize / 2;
    for (int i = 0; i < kSize; ++i)
    {    // 生成一维高斯核(可分离卷积特性)
        float x = static_cast<float>(i - half);
        kernel[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        sum += kernel[i];
    }
    // 归一化核,保证滤波后图像整体亮度不变
    for (int i = 0; i < kSize; ++i) kernel[i] /= sum;

    // 临时缓冲区(存储横向平滑后的浮点值，避免连续两次舍入误差)
    std::vector<float> temp(width * height);
    // 横向平滑(Horizontal Pass) + 镜像边界
    for (int y = 0; y < height; ++y)
    {
        const ushort *srcLine = reinterpret_cast<const ushort*>(image.constScanLine(y));
        float* tempRow = &temp[y * width];
        for (int x = 0; x < width; ++x)
        {
            float res = 0.0f;
            for (int i = -half; i <= half; ++i)
            {
                int ix = x + i;
                if (static_cast<unsigned int>(ix) >= static_cast<unsigned int>(width))
                {   // 边界检查,只有当 ix < 0 或 ix >= w 时才会进入这里（无符号转化的技巧）
                    if (ix < 0) ix = -ix;
                    else ix = 2 * width - ix - 2;
                    // 最后的保险, 当kSize > image.width
                    ix = (ix < 0) ? 0 : (ix >= width ? width - 1 : ix);
                }
                res += static_cast<float>(srcLine[ix]) * kernel[i + half];
            }
            tempRow[x] = res;
        }
    }
    // 纵向平滑(Vertical Pass) + 镜像边界 + 转回 ushort(16位)
    for (int y = 0; y < height; ++y)
    {
        ushort *destLine = reinterpret_cast<ushort*>(result.scanLine(y));
        for (int x = 0; x < width; ++x)
        {
            float res = 0.0f;
            for (int i = -half; i <= half; ++i)
            {
                int iy = y + i;
                if (static_cast<unsigned int>(iy) >= static_cast<unsigned int>(height))
                {
                    if (iy < 0) iy = -iy;
                    else iy = 2 * height - iy - 2;
                    iy = (iy < 0) ? 0 : (iy >= height ? height - 1 : iy);
                }
                res += temp[iy * width + x] * kernel[i + half];
            }
            destLine[x] = static_cast<ushort>(qBound(0.0f, res, 65535.0f));
        }
    }
    return result;
}


FloatImage EdgeDrawing::gaussianBlurFloat(const FloatImage &input, int kSize, float sigma)
{   // defalut kernel size is 5, sigma = 1.0f, use BORDER_REFLECT_101(镜像边界)
    if (input.data.empty() || kSize <= 1 || sigma <= 0.0f)
        return input;

    int width = input.width;
    int height = input.height;
    FloatImage output = {width, height, std::vector<float>(width * height)};

    std::vector<float> kernel(kSize);
    float sum = 0.0f;
    int half = kSize / 2;
    for (int i = 0; i < kSize; ++i)
    {   // 生成一维高斯核(可分离卷积特性)
        float x = static_cast<float>(i - half);
        kernel[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        sum += kernel[i];
    }
    // 归一化核,保证滤波后图像整体亮度不变
    for (int i = 0; i < kSize; ++i) kernel[i] /= sum;

    // 临时缓冲区用于存储横向卷积后的结果
    std::vector<float> temp(width * height);
    // 横向平滑(Horizontal Pass) + 镜像边界处理
    for (int y = 0; y < height; ++y)
    {
        const float* srcRow = &input.data[y * width];
        float* tempRow = &temp[y * width];
        for (int x = 0; x < width; ++x)
        {
            float res = 0.0f;
            for (int i = -half; i <= half; ++i)
            {
                int ix = x + i;
                if (static_cast<unsigned int>(ix) >= static_cast<unsigned int>(width))
                {
                    if (ix < 0) ix = -ix;
                    else ix = 2 * width - ix - 2;
                    ix = (ix < 0) ? 0 : (ix >= width ? width - 1 : ix);
                }
                res += srcRow[ix] * kernel[i + half];
            }
            tempRow[x] = res;
        }
    }
    // 纵向平滑(Vertical Pass) + 镜像边界处理
    for (int y = 0; y < height; ++y)
    {
        float* destRow = &output.data[y * width];
        for (int x = 0; x < width; ++x)
        {
            float res = 0.0f;
            for (int i = -half; i <= half; ++i)
            {
                int iy = y + i;
                if (static_cast<unsigned int>(iy) >= static_cast<unsigned int>(height))
                {
                    if (iy < 0) iy = -iy;
                    else iy = 2 * height - iy - 2;
                    iy = (iy < 0) ? 0 : (iy >= height ? height - 1 : iy);
                }
                res += temp[iy * width + x] * kernel[i + half];
            }
            destRow[x] = res;
        }
    }
    return output;
}

GradientInfo EdgeDrawing::computeGradientGrayscale16(const QImage &image)
{   // 梯度计算 Prewitt operator
    int w = image.width();
    int h = image.height();
    // 根据不同的数据流, 使用不同的特化参数
    gradientThreshold = calculateInternalThreshold(stdGradientThreshold, MODE_GRAY16);
    anchorThreshold = calculateInternalThreshold(stdAnchorThreshold, MODE_GRAY16);
    gradient = {w, h,std::vector<float>(w * h, 0.0f),std::vector<uchar>(w * h, 0)};

    // 跳过四周 1 像素边缘以防越界
    for (int y = 1; y < h - 1; ++y)
    {
        const ushort* r0 = reinterpret_cast<const ushort*>(image.constScanLine(y - 1)); // 上一行
        const ushort* r1 = reinterpret_cast<const ushort*>(image.constScanLine(y));     // 当前行
        const ushort* r2 = reinterpret_cast<const ushort*>(image.constScanLine(y + 1)); // 下一行

        float* magRow = &gradient.magnitude[y * w];
        uchar* dirRow = &gradient.direction[y * w];

        for (int x = 1; x < w - 1; ++x)
        {
            // 16位计算时，gx, gy 的范围会比较大
            // Prewitt 算子 - 水平梯度 Gx (检测垂直边缘)
            // [-1 0 1]
            // [-1 0 1]
            // [-1 0 1]
            int gx = (r0[x+1] - r0[x-1]) + (r1[x+1] - r1[x-1]) + (r2[x+1] - r2[x-1]);
            // Prewitt 算子 - 垂直梯度 Gy (检测水平边缘)
            // [-1 -1 -1]
            // [ 0  0  0]
            // [ 1  1  1]
            int gy = (r2[x-1] + r2[x] + r2[x+1]) - (r0[x-1] + r0[x] + r0[x+1]);

            float absGx = static_cast<float>(std::abs(gx));
            float absGy = static_cast<float>(std::abs(gy));
            // ed论文公式: Magnitude = |Gx| + |Gy|(或用Magnitude = sqrt((Gx * Gx + (Gy * Gy)))
            magRow[x] = absGx + absGy;
            // 论文逻辑: 如果 |Gx| >= |Gy|，则为垂直边缘
            dirRow[x] = (absGx >= absGy) ? EDGE_VERTICAL : EDGE_HORIZONTAL;
        }
    }
    return gradient;
}

GradientInfo EdgeDrawing::computeGradientFloat(const FloatImage &input)
{
    int w = input.width;
    int h = input.height;
    // 根据不同的数据流, 使用不同的特化参数
    gradientThreshold = calculateInternalThreshold(stdGradientThreshold, MODE_FLOAT);
    anchorThreshold = calculateInternalThreshold(stdAnchorThreshold, MODE_FLOAT);
    GradientInfo grad = {w, h, std::vector<float>(w * h, 0.0f), std::vector<uchar>(w * h, 0)};

    // 跳过四周 1 像素边缘以防越界
    for (int y = 1; y < h - 1; ++y)
    {
        const float* r0 = &input.data[(y - 1) * w]; // 上一行
        const float* r1 = &input.data[y * w];       // 当前行
        const float* r2 = &input.data[(y + 1) * w]; // 下一行

        float* magRow = &grad.magnitude[y * w];
        uchar* dirRow = &grad.direction[y * w];

        for (int x = 1; x < w - 1; ++x)
        {   // Prewitt 算子
            float gx = (r0[x+1] - r0[x-1]) + (r1[x+1] - r1[x-1]) + (r2[x+1] - r2[x-1]);
            float gy = (r2[x-1] + r2[x] + r2[x+1]) - (r0[x-1] + r0[x] + r0[x+1]);
            float absGx = std::abs(gx);
            float absGy = std::abs(gy);
            magRow[x] = absGx + absGy;
            // 如果 |Gx| >= |Gy|，则为垂直边缘
            dirRow[x] = (absGx >= absGy) ? EDGE_VERTICAL : EDGE_HORIZONTAL;
        }
    }
    return grad;
}

std::vector<QPoint> EdgeDrawing::extractAnchors()
{   // get anchors
    std::vector<QPoint> anchors;
    int w = gradient.width;
    int h = gradient.height;

    for (int y = scanInterval; y < h - scanInterval; y += scanInterval)
    {
        for (int x = scanInterval; x < w - scanInterval; x += scanInterval)
        {
            int idx = y * w + x;
            float mag = gradient.magnitude[idx];
            // 只有超过梯度阈值的像素才能作为潜在锚点
            if (mag <= gradientThreshold) continue;
            // 非极大值抑制 (Non-Maximum Suppression)
            int neighbor1, neighbor2;
            if (gradient.direction[idx] == EDGE_VERTICAL)
            {// 垂直边缘：左右邻居
                neighbor1 = idx - 1;
                neighbor2 = idx + 1;
            }
            else {// 水平边缘：上下邻居
                neighbor1 = idx - w;
                neighbor2 = idx + w;
            }
            if (mag > (gradient.magnitude[neighbor1] + anchorThreshold) &&
                mag > (gradient.magnitude[neighbor2] + anchorThreshold)) {
                anchors.push_back(QPoint(x, y));
            }
        }
    }
    return anchors;
}

std::vector<EdgeChain> EdgeDrawing::graySmartRouting(const std::vector<QPoint> &anchors)
{   // smart route
    int w = gradient.width;
    int h = gradient.height;
    // 初始化 EdgeMap, 使用 edgeMap 记录已访问像素
    edgeMap = std::vector<uchar>(w * h, 0);
    std::vector<EdgeChain> allChains;

    for (size_t i = 0; i < anchors.size(); ++i)
    {
        const QPoint& anchor = anchors[i];
        EdgeChain chain;
        routeStack(chain, anchor.x(), anchor.y());
        if (chain.points.size() > shortEdgeLength)
        {   // 论文建议：过滤掉过短的边缘
            allChains.push_back(chain);
        }
    }
    return allChains;
}

void EdgeDrawing::route(EdgeChain &chain, int x, int y)
{   // route. 目前路由的缺点: 生成的边缘图像物理的边缘有 1 像素的空隙. 卷积算子Prewitt也有边缘缩进情况.
    int w = gradient.width;
    int h = gradient.height;
    int idx = y * w + x;

    // 1. 基础检查
    if (x <= 0 || y <= 0 || x >= w - 1 || y >= h - 1) return;
    if (gradient.magnitude[idx] <= gradientThreshold || edgeMap[idx]) return;

    // 2. 标记当前点
    chain.points.push_back(QPoint(x, y));
    edgeMap[idx] = 255; // 标记已访问

    // 3. 根据当前像素的边缘方向决定生长路径
    if (gradient.direction[idx] == EDGE_HORIZONTAL)
    {   // --- 水平边缘：向左和向右生长 ---
        // 尝试向左 (Go Left: x-1)
        if (!edgeMap[(y-1)*w + (x-1)] && !edgeMap[y*w + (x-1)] && !edgeMap[(y+1)*w + (x-1)])
        {
            float gLeftUp   = gradient.magnitude[(y - 1) * w + (x - 1)];
            float gLeftMid  = gradient.magnitude[y * w + (x - 1)];
            float gLeftDown = gradient.magnitude[(y + 1) * w + (x - 1)];
            if (gLeftUp > gLeftMid && gLeftUp > gLeftDown)
                route(chain, x - 1, y - 1);
            else if (gLeftDown > gLeftMid && gLeftDown > gLeftUp)
                route(chain, x - 1, y + 1);
            else
                route(chain, x - 1, y);
        }
        // 尝试向右 (Go Right: x+1)
        if (!edgeMap[(y-1)*w + (x+1)] && !edgeMap[y*w + (x+1)] && !edgeMap[(y+1)*w + (x+1)])
        {
            float gRightUp   = gradient.magnitude[(y - 1) * w + (x + 1)];
            float gRightMid  = gradient.magnitude[y * w + (x + 1)];
            float gRightDown = gradient.magnitude[(y + 1) * w + (x + 1)];
            if (gRightUp > gRightMid && gRightUp > gRightDown)
                route(chain, x + 1, y - 1);
            else if (gRightDown > gRightMid && gRightDown > gRightUp)
                route(chain, x + 1, y + 1);
            else
                route(chain, x + 1, y);
        }
    }
    else
    {   // --- 垂直边缘：向上和向下生长 ---
        // 尝试向上 (Go Top: y-1)
        if (!edgeMap[(y-1)*w + (x-1)] && !edgeMap[(y-1)*w + x] && !edgeMap[(y-1)*w + (x+1)])
        {
            float gTopLeft  = gradient.magnitude[(y - 1) * w + (x - 1)];
            float gTopMid   = gradient.magnitude[(y - 1) * w + x];
            float gTopRight = gradient.magnitude[(y - 1) * w + (x + 1)];
            if (gTopLeft > gTopMid && gTopLeft > gTopRight)
                route(chain, x - 1, y - 1);
            else if (gTopRight > gTopLeft && gTopRight > gTopMid)
                route(chain, x + 1, y - 1);
            else
                route(chain, x, y - 1);
        }
        // 尝试向下 (Go Down: y+1)
        if (!edgeMap[(y+1)*w + (x-1)] && !edgeMap[(y+1)*w + x] && !edgeMap[(y+1)*w + (x+1)])
        {
            float gDownLeft  = gradient.magnitude[(y + 1) * w + (x - 1)];
            float gDownMid   = gradient.magnitude[(y + 1) * w + x];
            float gDownRight = gradient.magnitude[(y + 1) * w + (x + 1)];
            if (gDownLeft > gDownMid && gDownLeft > gDownRight)
                route(chain, x - 1, y + 1);
            else if (gDownRight > gDownLeft && gDownRight > gDownMid)
                route(chain, x + 1, y + 1);
            else
                route(chain, x, y + 1);
        }
    }
}

void EdgeDrawing::routeStack(EdgeChain &chain, int startX, int startY)
{
    int w = gradient.width;
    int h = gradient.height;

    // 显式栈，存储待访问的点
    std::stack<QPoint> s;
    s.push(QPoint(startX, startY));

    while (!s.empty())
    {
        QPoint current = s.top();
        s.pop();
        int x = current.x();
        int y = current.y();
        int idx = y * w + x;

        // 1. 基础检查
        if (x <= 0 || y <= 0 || x >= w - 1 || y >= h - 1) continue;
        if (gradient.magnitude[idx] <= gradientThreshold || edgeMap[idx]) continue;

        // 2. 标记并保存
        edgeMap[idx] = 255;
        chain.points.push_back(current);

        // 3. 根据方向探测邻居并入栈
        if (gradient.direction[idx] == EDGE_HORIZONTAL)
        {   // --- 处理水平边缘 (向左和向右) ---
            // 右侧探测 (后入栈，先处理)
            if (!edgeMap[(y-1)*w + (x+1)] && !edgeMap[y*w + (x+1)] && !edgeMap[(y+1)*w + (x+1)])
                s.push(findBestNeighbor(x + 1, y, true)); // true 表示垂直扫描三个邻居
            // 左侧探测
            if (!edgeMap[(y-1)*w + (x-1)] && !edgeMap[y*w + (x-1)] && !edgeMap[(y+1)*w + (x-1)])
                s.push(findBestNeighbor(x - 1, y, true));

        }
        else
        {   // --- 处理垂直边缘 (向上和向下) ---
            // 下方探测
            if (!edgeMap[(y+1)*w + (x-1)] && !edgeMap[(y+1)*w + x] && !edgeMap[(y+1)*w + (x+1)])
                s.push(findBestNeighbor(x, y + 1, false)); // false 表示水平扫描三个邻居
            // 上方探测
            if (!edgeMap[(y-1)*w + (x-1)] && !edgeMap[(y-1)*w + x] && !edgeMap[(y-1)*w + (x+1)])
                s.push(findBestNeighbor(x, y - 1, false));
        }
    }
}

QPoint EdgeDrawing::findBestNeighbor(int targetX, int targetY, bool isVerticalScan)
{
    int w = gradient.width;
    float maxG = -1.0f;
    QPoint bestP(targetX, targetY);

    if (isVerticalScan)
    {   // 固定 X，扫描 Y-1, Y, Y+1
        for (int oy = -1; oy <= 1; ++oy)
        {
            float g = gradient.magnitude[(targetY + oy) * w + targetX];
            if (g > maxG)
            {
                maxG = g;
                bestP = QPoint(targetX, targetY + oy);
            }
        }
    }
    else
    {   // 固定 Y，扫描 X-1, X, X+1
        for (int ox = -1; ox <= 1; ++ox)
        {
            float g = gradient.magnitude[targetY * w + (targetX + ox)];
            if (g > maxG)
            {
                maxG = g;
                bestP = QPoint(targetX + ox, targetY);
            }
        }
    }
    return bestP;
}

QImage EdgeDrawing::drawEdgeChains(int width, int height, const std::vector<EdgeChain> &chains)
{
    // 创建一个纯黑底色的 8 位图像
    QImage result(width, height, QImage::Format_RGB32);
    result.fill(Qt::black);
    // 预定义一些颜色，方便区分不同的 Chain
    QColor colors[] = {Qt::green, Qt::red, Qt::yellow, Qt::cyan, Qt::magenta, Qt::blue};
    int colorCount = 6;
    for (size_t i = 0; i < chains.size(); ++i)
    {   // 每个 Chain 用不同的颜色绘制，方便观察是否断裂
        QColor drawColor = colors[i % colorCount];
        for (const QPoint &pt : chains[i].points)
        {   // 确保坐标在图像范围内
            if (pt.x() >= 0 && pt.x() < width && pt.y() >= 0 && pt.y() < height)
            {
                result.setPixelColor(pt.x(), pt.y(), drawColor);
            }
        }
    }
    return result;
}

/********************************************************************************/
// edge drawing from opencv
/********************************************************************************/

// edge drawing form opencv contrib
void EdgeDrawing::edgeDrawingInLib(const QImage &originalImage)
{
    QImage workImage;
    int channels = 0;
    // Edge Drawing 支持 CV_8UC1 CV_8UC3 CV_8UC4
    switch (originalImage.format())
    {
    case QImage::Format_Grayscale8:
    case QImage::Format_Indexed8:
        workImage = originalImage;
        channels = 1;
        break;

    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGBA8888:
        // 若有透明通道, 转为 BGRA (对应 CV_8UC4). Qt 无 BGRA8888 格式.
        {   // 统一将透明背景下的颜色设为白色, 为了消除伪影. 目标像素（Dst'） = (1 - Alpha) * Dst + Alpha * Src
            // QImage::Format_ARGB32 和 QImage::Format_RGBA8888 在存储上有很大的区别的(含大小端问题), 注意区分.
            // 利用 RGBA8888 字节流格式 + rgbSwapped 得到固定的 BGRA
            workImage = QImage(originalImage.size(), QImage::Format_RGBA8888);
            workImage.fill(Qt::white);
            QPainter painter(&workImage);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.drawImage(0, 0, originalImage);
            painter.end();

            workImage = workImage.rgbSwapped(); // [Byte0:B, Byte1:G, Byte2:R, Byte3:A]
            channels = 4;
        }
        break;

    default:
        // 其余一律转为 BGR (对应 CV_8UC3). Format_BGR888 是字节流格式, 在内存中按 B, G, R 顺序排列.
        workImage = originalImage.convertToFormat(QImage::Format_BGR888);
        channels = 3;
        break;
    }

    edResults.reset(RunEdgeDrawingFull(workImage.bits(), workImage.width(),
                                       workImage.height(), workImage.bytesPerLine(),
                                       channels, edParams));
}

QImage EdgeDrawing::drawImage(int width, int height)
{
    QImage edgeMap(width, height, QImage::Format_RGB888);
    edgeMap.fill(Qt::black);

    // 预先获取所有行的首地址
    QVector<uchar*> linePtrs(height);
    for (int y = 0; y < height; ++y)
    {
        linePtrs[y] = edgeMap.scanLine(y);
    }

    for (int i = 0; i < edResults.data.segmentCount; ++i)
    {
        opencved::EDEdgeSegment& seg = edResults.data.segments[i];
        // 动态计算颜色: 根据索引 i 平分 360 度色相环，保证相邻边缘颜色差异最大
        int hue = (i * 137) % 360; // 137 是黄金角度偏移，能让颜色分布更均匀(完全剩余系)
        QColor segmentColor = QColor::fromHsv(hue, 255, 255);

        for (int j = 0; j < seg.count; ++j)
        {
            opencved::EDPoint& pt = seg.points[j];
            // 确保坐标不越界
            if (pt.x >= 0 && pt.x < edgeMap.width() && pt.y >= 0 && pt.y < edgeMap.height())
            {
                uchar* pixel = linePtrs[pt.y] + (pt.x * 3);
                pixel[0] = segmentColor.red();
                pixel[1] = segmentColor.green();
                pixel[2] = segmentColor.blue();
                // pixel[0] = 255;
                // pixel[1] = 255;
                // pixel[2] = 255;
            }
        }
    }
    return edgeMap;
}
