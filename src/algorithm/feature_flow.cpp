#include "feature_flow.h"
#include "logger.h"
#include <algorithm>
#include <cfloat>
#include <QDebug>

// cpu版本函数
namespace
{
bool jump_flooding_cpu_api(
    const JFAFF::JFAPoint* input_seed_map,
    JFAFF::JFAPoint* output_map,
    int width,
    int height,
    char* error_msg,
    int max_error_len
    ) {
    // 基础的合法性检测
    if (width <= 0 || height <= 0 || input_seed_map == nullptr || output_map == nullptr) {
        if (error_msg && max_error_len > 0) {
            snprintf(error_msg, max_error_len, "Invalid arguments: null pointers or invalid dimensions.");
        }
        return false;
    }

    int total_pixels = width * height;

    // 分配 CPU 端的 Ping-Pong 缓冲区
    std::vector<JFAFF::JFAPoint> buffer_A(input_seed_map, input_seed_map + total_pixels);
    std::vector<JFAFF::JFAPoint> buffer_B(total_pixels);

    JFAFF::JFAPoint* buf_in = buffer_A.data();
    JFAFF::JFAPoint* buf_out = buffer_B.data();

    // 计算初始步长 k: 从 max_dim 的向下取整的 2 的次幂开始
    int max_dim = std::max(width, height);
    int step = 1;
    while (step < max_dim) {
        step *= 2;
    }

    // JFA 迭代逻辑
    while (step >= 1) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;

                JFAFF::JFAPoint best_seed = buf_in[idx];
                float best_dist = FLT_MAX; // 初始化为极大值

                // 计算当前位置到自身已知种子的距离
                if (best_seed.x != -1) {
                    float dx0 = static_cast<float>(x - best_seed.x);
                    float dy0 = static_cast<float>(y - best_seed.y);
                    best_dist = dx0 * dx0 + dy0 * dy0;
                }

                // 遍历周围 9 个邻居 (步长为 step)
                for (int j = -1; j <= 1; ++j) {
                    int ny = y + j * step;

                    if (ny >= 0 && ny < height) {
                        for (int i = -1; i <= 1; ++i) {
                            int nx = x + i * step;

                            if (nx >= 0 && nx < width) {
                                int n_idx = ny * width + nx;
                                JFAFF::JFAPoint neighbor_seed = buf_in[n_idx];

                                if (neighbor_seed.x != -1) {
                                    float dx = static_cast<float>(x - neighbor_seed.x);
                                    float dy = static_cast<float>(y - neighbor_seed.y);
                                    float dist = dx * dx + dy * dy;

                                    // 如果找到更近的种子，更新它
                                    if (dist < best_dist) {
                                        best_dist = dist;
                                        best_seed = neighbor_seed;
                                    }
                                }
                            }
                        }
                    }
                }

                // 将当前轮次的最优结果写入输出缓冲区
                buf_out[idx] = best_seed;
            }
        }

        // Ping-Pong 交换指针
        std::swap(buf_in, buf_out);
        step /= 2;
    }

    // 注意：由于循环最后执行了 swap，此时最新的结果存放在 buf_in 中
    // 将结果拷贝回调用方预分配的内存
    for (int i = 0; i < total_pixels; ++i) {
        output_map[i] = buf_in[i];
    }

    if (error_msg && max_error_len > 0) {
        snprintf(error_msg, max_error_len, "Jump Flooding completed successfully.");
    }

    return true;
}

bool feature_flow_cpu_api(
    const JFAFF::JFAPoint* nearest_seed_map,
    float* output_flow_map,
    int width,
    int height,
    float m,
    char* error_msg,
    int max_error_len
    ) {
    // 参数验证（简化版，仅必要的检查）
    if (width <= 0 || height <= 0) {
        if (error_msg && max_error_len > 0) {
            snprintf(error_msg, max_error_len, "Invalid dimensions: width=%d, height=%d", width, height);
        }
        return false;
    }
    if (nearest_seed_map == nullptr || output_flow_map == nullptr) {
        if (error_msg && max_error_len > 0) {
            snprintf(error_msg, max_error_len, "Null pointer: input or output buffer is null");
        }
        return false;
    }
    if (m <= 0.0f) {
        if (error_msg && max_error_len > 0) {
            snprintf(error_msg, max_error_len, "Invalid m: must be > 0");
        }
        return false;
    }

    const float inv_m = 1.0f / m;

    // 遍历每个像素
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int idx = y * width + x;
            const JFAFF::JFAPoint seed = nearest_seed_map[idx];

            if (seed.x != -1 && seed.y != -1) {
                // 计算欧氏距离
                const float dx = static_cast<float>(x - seed.x);
                const float dy = static_cast<float>(y - seed.y);
                const float d_x = sqrtf(dx * dx + dy * dy);

                // 计算周期特征流值
                const float ratio = d_x * inv_m;
                const float floor_f = floorf(ratio);
                const float frac = ratio - floor_f;
                const int floor_val = static_cast<int>(floor_f);

                output_flow_map[idx] = ((floor_val & 1) == 0)
                                           ? (255.0f * frac)
                                           : (255.0f * (1.0f - frac));
            } else {
                output_flow_map[idx] = 0.0f;   // 无种子情况（理论上不会发生）
            }
        }
    }

    // 成功信息（可选）
    if (error_msg && max_error_len > 0) {
        snprintf(error_msg, max_error_len, "Feature Flow completed successfully.");
    }
    return true;
}

}

// 函数对外接口

//
bool FeatureFlow::jumpFloodingCUDAApi(const ScopedEDResults& edResults,
                                       int imgWidth, int imgHeight)
{
    // 1. 基础参数校验
    if (imgWidth <= 0 || imgHeight <= 0)
    {
        LOG_ERROR << "Jump Flooding cuda 启动失败: 参数无效";
        return false;
    }

    int total_pixels = imgWidth * imgHeight;

    // 2. 初始化输入和输出缓冲区
    // 规定 {-1, -1} 表示该像素没有种子
    std::vector<JFAFF::JFAPoint> input_seed_map(total_pixels, {-1, -1});
    // 直接初始化并分配类成员变量的内存，用于接收 DLL 的输出
    jFResults.assign(total_pixels, JFAFF::JFAPoint{-1, -1});

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
    // 5. 调用 DLL 中的 C 接口, 直接将数据写进类成员 jFResults
    bool success = jump_flooding_api(
        input_seed_map.data(),
        jFResults.data(),
        imgWidth,
        imgHeight,
        dll_status_msg,
        sizeof(dll_status_msg)
        );
    // 6. 处理结果
    if (!success)
    {
        //LOG_ERROR <<  "Jump Flooding CUDA API Failed, DLL最后状态/异常日志: " << dll_status_msg;
        // 失败时清空成员变量，防止外界读取到无效或残留的数据
        jFResults.clear();
        return false;
    }
    LOG_INFO <<  "Jump Flooding CUDA API success, DLL最后状态/日志: " << dll_status_msg;
    return true;
}

bool FeatureFlow::featureFlowCUDAApi(const std::vector<JFAFF::JFAPoint>& nearestSeedMap,
                                      int imgWidth, int imgHeight, float m)
{
    // 1. 基础参数校验
    if (nearestSeedMap.empty() || imgWidth <= 0 || imgHeight <= 0 || m <= 0.0f)
    {
        qDebug()<<nearestSeedMap.empty() <<"   "<<imgWidth<<"   "<<imgHeight<<"   "<<m;
        LOG_ERROR << "Feature Flow cuda 启动失败: 参数无效 (m 必须大于 0 且种子图不能为空)";
        return false;
    }

    int total_pixels = imgWidth * imgHeight;

    // 2. 直接初始化并分配类成员变量的内存，用于接收 DLL 输出的浮点权重
    fFResults.assign(total_pixels, 0.0f);

    // 3. 准备错误日志缓冲区
    char dll_status_msg[512] = {0};

    // 4. 调用 DLL 中的 C 接口
    bool success = feature_flow_api(
        nearestSeedMap.data(),
        fFResults.data(),
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
        fFResults.clear();
        return false;
    }
    LOG_ERROR << "Feature Flow CUDA API success, DLL最后状态/日志: " << dll_status_msg;
    return true;
}


bool FeatureFlow::jumpFloodingCPUApi(const ScopedEDResults& edResults,
                                      int imgWidth, int imgHeight)
{
    // 1. 基础参数校验
    if (imgWidth <= 0 || imgHeight <= 0)
    {
        LOG_ERROR << "Jump Flooding cpu 启动失败: 参数无效";
        return false;
    }

    int total_pixels = imgWidth * imgHeight;

    // 2. 初始化输入和输出缓冲区
    // 规定 {-1, -1} 表示该像素没有种子
    std::vector<JFAFF::JFAPoint> input_seed_map(total_pixels, {-1, -1});
    // 直接初始化并分配类成员变量的内存，用于接收 DLL 的输出
    jFResults.assign(total_pixels, JFAFF::JFAPoint{-1, -1});

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
    // 5. 调用 DLL 中的 C 接口, 直接将数据写进类成员 jFResults
    bool success = jump_flooding_cpu_api(
        input_seed_map.data(),
        jFResults.data(),
        imgWidth,
        imgHeight,
        dll_status_msg,
        sizeof(dll_status_msg)
        );
    // 6. 处理结果
    if (!success)
    {
        LOG_ERROR <<  "Jump Flooding cpu API Falied, DLL最后状态/异常日志: " << dll_status_msg;
        // 失败时清空成员变量，防止外界读取到无效或残留的数据
        jFResults.clear();
        return false;
    }
    return true;
}

bool FeatureFlow::featureFlowCPUApi(const std::vector<JFAFF::JFAPoint>& nearestSeedMap,
                                     int imgWidth, int imgHeight, float m)
{
    // 1. 基础参数校验
    if (nearestSeedMap.empty() || imgWidth <= 0 || imgHeight <= 0 || m <= 0.0f)
    {
        qDebug()<<nearestSeedMap.empty() <<"   "<<imgWidth<<"   "<<imgHeight<<"   "<<m;
        LOG_ERROR << "Feature Flow cpu 启动失败: 参数无效 (m 必须大于 0 且种子图不能为空)";
        return false;
    }

    int total_pixels = imgWidth * imgHeight;

    // 2. 直接初始化并分配类成员变量的内存，用于接收 DLL 输出的浮点权重
    fFResults.assign(total_pixels, 0.0f);

    // 3. 准备错误日志缓冲区
    char dll_status_msg[512] = {0};

    // 4. 调用 DLL 中的 C 接口
    bool success = feature_flow_cpu_api(
        nearestSeedMap.data(),
        fFResults.data(),
        imgWidth,
        imgHeight,
        m,
        dll_status_msg,
        sizeof(dll_status_msg)
        );

    // 5. 处理结果
    if (!success)
    {
        LOG_ERROR << "Feature Flow CPU API Failed, DLL最后状态/异常日志: " << dll_status_msg;
        // 失败时清空成员变量，防止外界读取到无效或残留的脏数据
        fFResults.clear();
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
