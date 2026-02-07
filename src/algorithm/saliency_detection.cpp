#include "saliency_detection.h"
#include <map>
#include <vector>

// 辅助函数：将 RGB 量化为索引 key
// 论文中提到将每个通道分为 12 份
inline uint32_t quantifyColor(QRgb color) {
    int r = qRed(color) / 21; // 256 / 12 ≈ 21.3
    int g = qGreen(color) / 21;
    int b = qBlue(color) / 21;
    // 限制在 0-11 范围内
    if (r > 11) r = 11;
    if (g > 11) g = 11;
    if (b > 11) b = 11;
    // 打包成一个 int key
    return (r << 8) | (g << 4) | b;
}

float SaliencyDetection::colorDist(const ColorBin &c1, const ColorBin &c2) {
    return std::sqrt(std::pow(c1.r - c2.r, 2) +
                     std::pow(c1.g - c2.g, 2) +
                     std::pow(c1.b - c2.b, 2));
}

QImage SaliencyDetection::getSaliencyMap(const QImage &input) {
    if (input.isNull()) return QImage();

    QImage img = input.convertToFormat(QImage::Format_RGB888);
    int width = img.width();
    int height = img.height();
    int totalPixels = width * height;

    // 1. 直方图量化 (Histogram Quantization)
    // ---------------------------------------------------------
    // 使用 map 暂存量化后的颜色
    std::map<uint32_t, ColorBin> hist;

    // 同时也需要保存每个像素对应的 Bin 索引，以便最后生成图像
    std::vector<uint32_t> pixelLabels(totalPixels);

    const uchar *bits = img.bits();
    int bytesPerLine = img.bytesPerLine();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int offset = y * bytesPerLine + x * 3;
            int r = bits[offset];
            int g = bits[offset + 1];
            int b = bits[offset + 2];
            QRgb color = qRgb(r, g, b);

            uint32_t key = quantifyColor(color);
            pixelLabels[y * width + x] = key;

            ColorBin &bin = hist[key];
            // 累加颜色值，最后取平均
            bin.r += r;
            bin.g += g;
            bin.b += b;
            bin.count++;

            // 累加空间坐标，用于计算 CSD [cite: 153-156]
            bin.sumX += x;
            bin.sumY += y;
            bin.sumX2 += (double)x * x;
            bin.sumY2 += (double)y * y;
        }
    }

    // 将 map 转为 vector 方便遍历，并计算平均颜色
    std::vector<ColorBin> bins;
    std::map<uint32_t, int> keyToVectorIndex; // 原始 key 到 vector 下标的映射

    int index = 0;
    for (auto &pair : hist) {
        ColorBin &bin = pair.second;
        if (bin.count > 0) {
            bin.r /= bin.count;
            bin.g /= bin.count;
            bin.b /= bin.count;
            bin.probability = (float)bin.count / totalPixels;
            bins.push_back(bin);
            keyToVectorIndex[pair.first] = index++;
        }
    }

    // 论文  提到可以丢弃覆盖率 < 95% 之外的颜色以加速
    // 但在这个简化实现中，为了保证所有像素都有值，我们保留所有 Bin。
    // 由于量化为 12x12x12，Bin 的数量本身就不多（通常 < 100）。

    // 2. 计算显著性 (Global Uniqueness + Spatial Distribution)
    // ---------------------------------------------------------
    float maxSaliency = 0.0f;

    for (size_t i = 0; i < bins.size(); ++i) {
        float globalContrast = 0.0f;

        // 计算 CSD (Color Spatial Distribution) [cite: 153-161]
        // 空间方差 V(c) = Vh(c) + Vv(c)
        double meanX = bins[i].sumX / bins[i].count;
        double meanY = bins[i].sumY / bins[i].count;
        double varX = (bins[i].sumX2 / bins[i].count) - (meanX * meanX);
        double varY = (bins[i].sumY2 / bins[i].count) - (meanY * meanY);
        float spatialVariance = std::sqrt(varX + varY);

        // 归一化空间方差 (简单归一化到 0-1，假设最大方差是半对角线)
        float normalizedVariance = spatialVariance / (std::sqrt(width*width + height*height) / 2.0);
        if (normalizedVariance > 1.0f) normalizedVariance = 1.0f;
        float spatialWeight = 1.0f - normalizedVariance; // 方差越小越显著

        // 计算 GU (Global Uniqueness)
        // 论文使用 GMM 距离，这里我们使用直方图对比度简化替代，效果接近且无需 OpenCV
        for (size_t j = 0; j < bins.size(); ++j) {
            if (i == j) continue;
            float dist = colorDist(bins[i], bins[j]);
            // 累加：距离 * 概率
            globalContrast += dist * bins[j].probability;
        }

        // 融合 GU 和 CSD
        // 这里采用简单的乘法融合，模拟论文中 "Integration" 的效果
        // S(c) = Contrast * (1 - Variance)
        bins[i].saliency = globalContrast * spatialWeight;

        if (bins[i].saliency > maxSaliency) {
            maxSaliency = bins[i].saliency;
        }
    }

    // 3. 生成显著性图 (Generate Map)
    // ---------------------------------------------------------
    QImage result(width, height, QImage::Format_Grayscale8);

    // 归一化参数
    float scale = (maxSaliency > 0) ? (255.0f / maxSaliency) : 0;

    for (int y = 0; y < height; ++y) {
        uchar *line = result.scanLine(y);
        for (int x = 0; x < width; ++x) {
            uint32_t key = pixelLabels[y * width + x];
            // 找到对应的 Bin
            if (keyToVectorIndex.find(key) != keyToVectorIndex.end()) {
                int binIdx = keyToVectorIndex[key];
                int val = (int)(bins[binIdx].saliency * scale);
                line[x] = (val > 255) ? 255 : val;
            } else {
                line[x] = 0;
            }
        }
    }

    return result;
}
