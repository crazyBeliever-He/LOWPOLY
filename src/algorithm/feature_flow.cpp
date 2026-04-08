#include "feature_flow.h"
#include "logger.h"
#include <algorithm>

bool FeatureFlow::jumpFloodingCUDAApi(const ScopedEDResults& edResults,
                                       int imgWidth, int imgHeight)
{
    // 1. 基础参数校验
    if (imgWidth <= 0 || imgHeight <= 0)
    {
        LOG_ERROR << "Jump Flooding 启动失败: 参数无效";
        return false;
    }

    int total_pixels = imgWidth * imgHeight;

    // 2. 初始化输入和输出缓冲区
    // 规定 {-1, -1} 表示该像素没有种子
    std::vector<JFAFF::JFAPoint> input_seed_map(total_pixels, {-1, -1});
    // 直接初始化并分配类成员变量的内存，用于接收 DLL 的输出
    jFCUDAResults.assign(total_pixels, JFAFF::JFAPoint{-1, -1});

    // 3. 将 ScopedEDResults 中的边缘点填充到 input_seed_map 中
    if (edResults.data.segments != nullptr)
    {
        for (int i = 0; i < edResults.data.segmentCount; ++i)
        {
            const auto& segment = edResults.data.segments[i];
            for (int j = 0; j < segment.count; ++j)
            {
                int px = segment.points[j].x;
                int py = segment.points[j].y;
                // 安全的越界检查
                if (px >= 0 && px < imgWidth && py >= 0 && py < imgHeight)
                {
                    int idx = py * imgWidth + px;
                    input_seed_map[idx] = {px, py};
                }
            }
        }
    }

    // 4. 准备错误日志缓冲区
    char dll_status_msg[512] = {0};
    // 5. 调用 DLL 中的 C 接口, 直接将数据写进类成员 jFCUDAResults
    bool success = jump_flooding_api(
        input_seed_map.data(),
        jFCUDAResults.data(),
        imgWidth,
        imgHeight,
        dll_status_msg,
        sizeof(dll_status_msg)
        );
    // 6. 处理结果
    if (!success)
    {
        LOG_ERROR <<  ", DLL最后状态/异常日志: " << dll_status_msg;
        // 失败时清空成员变量，防止外界读取到无效或残留的数据
        jFCUDAResults.clear();
        // 返回 false 让调用方决定如何降级
        return false;
    }
    return true;
}

bool FeatureFlow::featureFlowCUDAApi(const std::vector<JFAFF::JFAPoint>& nearestSeedMap,
                                      int imgWidth, int imgHeight, float m)
{
    // 1. 基础参数校验
    if (nearestSeedMap.empty() || imgWidth <= 0 || imgHeight <= 0 || m <= 0.0f)
    {
        LOG_ERROR << "Feature Flow 启动失败: 参数无效 (m 必须大于 0 且种子图不能为空)";
        return false;
    }

    int total_pixels = imgWidth * imgHeight;

    // 2. 直接初始化并分配类成员变量的内存，用于接收 DLL 输出的浮点权重
    fFCUDAResults.assign(total_pixels, 0.0f);

    // 3. 准备错误日志缓冲区
    char dll_status_msg[512] = {0};

    // 4. 调用 DLL 中的 C 接口
    bool success = feature_flow_api(
        nearestSeedMap.data(),
        fFCUDAResults.data(),
        imgWidth,
        imgHeight,
        m,
        dll_status_msg,
        sizeof(dll_status_msg)
        );

    // 5. 处理结果
    if (!success)
    {
        LOG_ERROR << "Feature Flow CUDA API Failed, DLL最后状态/异常日志: " << dll_status_msg;
        // 失败时清空成员变量，防止外界读取到无效或残留的脏数据
        fFCUDAResults.clear();
        // 返回 false 让调用方决定如何降级
        return false;
    }
    return true;
}

QImage FeatureFlow::drawJumpFloodingMap(int width, int height,
                                        const std::vector<JFAFF::JFAPoint>& jfaOutput,
                                        const ScopedEDResults& edResults)
{
    // 1. 初始化输出图像并填充为黑色
    QImage jfaMap(width, height, QImage::Format_RGB888);
    jfaMap.fill(Qt::black);

    // 2. 建立“种子坐标”到“颜色”的映射表 (大小与图像像素数相同)
    // 这样在使用 JFA 输出的 (x, y) 时，能以 O(1) 的时间复杂度查到对应边缘段的颜色
    std::vector<QColor> seedColorMap(width * height, Qt::black);
    for (int i = 0; i < edResults.data.segmentCount; ++i)
    {
        opencved::EDEdgeSegment& seg = edResults.data.segments[i];
        // 动态计算该边缘段的专属颜色
        int hue = (i * 137) % 360;
        QColor segmentColor = QColor::fromHsv(hue, 255, 255);
        for (int j = 0; j < seg.count; ++j)
        {
            opencved::EDPoint& pt = seg.points[j];
            // 确保坐标不越界
            if (pt.x >= 0 && pt.x < width && pt.y >= 0 && pt.y < height)
                seedColorMap[pt.y * width + pt.x] = segmentColor;
        }
    }

    // 3. 预先获取所有行的首地址，加速写入操作
    QVector<uchar*> linePtrs(height);
    for (int y = 0; y < height; ++y)
    {
        linePtrs[y] = jfaMap.scanLine(y);
    }

    // 4. 遍历整个屏幕，根据 JFA 结果对每个像素进行着色
    for (int y = 0; y < height; ++y)
    {
        uchar* row = linePtrs[y];
        for (int x = 0; x < width; ++x)
        {
            int pixelIdx = y * width + x;
            const JFAFF::JFAPoint& nearestSeed = jfaOutput[pixelIdx];
            // 检查该像素是否找到了有效的最近种子 (-1, -1 为无效/无种子)
            if (nearestSeed.x >= 0 && nearestSeed.x < width &&
                nearestSeed.y >= 0 && nearestSeed.y < height)
            {
                // 去我们刚才建立的 Lookup Table 里查这个种子是什么颜色的
                int seedIdx = nearestSeed.y * width + nearestSeed.x;
                const QColor& color = seedColorMap[seedIdx];
                // 写入 RGB 数据
                int byteOffset = x * 3;
                row[byteOffset]     = color.red();
                row[byteOffset + 1] = color.green();
                row[byteOffset + 2] = color.blue();
            }
            // 如果没找到有效种子，由于前面已经 fill(Qt::black)，所以自然是黑色，无需额外处理
        }
    }
    return jfaMap;
}
QImage FeatureFlow::drawFeatureFlowMap(int width, int height, const std::vector<float>& ffOutput)
{
    // 1. 初始化单通道灰度图格式 (Format_Grayscale8)，比 RGB888 更节省内存且契合数据含义
    QImage ffMap(width, height, QImage::Format_Grayscale8);
    ffMap.fill(Qt::black);

    if (ffOutput.empty() || ffOutput.size() != static_cast<size_t>(width * height))
    {
        return ffMap;
    }

    // 2. 预先获取所有行的首地址，加速写入操作
    QVector<uchar*> linePtrs(height);
    for (int y = 0; y < height; ++y)
    {
        linePtrs[y] = ffMap.scanLine(y);
    }

    // 3. 遍历浮点数组，将其转换为灰度像素
    for (int y = 0; y < height; ++y)
    {
        uchar* row = linePtrs[y];
        for (int x = 0; x < width; ++x)
        {
            int pixelIdx = y * width + x;
            float weight = ffOutput[pixelIdx];

            // 安全截断：防止浮点数计算微小误差导致其越出 [0, 255] 范围，转换后直接赋给 uchar
            uchar grayValue = static_cast<uchar>(std::clamp(weight, 0.0f, 255.0f));

            // Format_Grayscale8 每像素占 1 个字节，无需乘以 3
            row[x] = grayValue;
        }
    }

    return ffMap;
}
