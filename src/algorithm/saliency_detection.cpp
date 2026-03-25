#include "saliency_detection.h"
#include "saliency_api.h"
#include "logger.h"

#include <vector>
#include <random>
#include <QProcess>
#include <QDir>
#include <QUuid>
#include <QCoreApplication>

SaliencyDetection::SaliencyDetection()
{
    sDParams.type = 0;
}

// 这个函数根据 UI 传过来的参数（sDParams.type），决定使用哪种算法
QImage SaliencyDetection::saliencyDetectionInterface(const QImage &input)
{
    if (input.isNull()) return QImage();

    // 对应 UI 中 QComboBox 的索引：
    // 0 -> "type1", 1 -> "type2", 2 -> "type3"
    switch (sDParams.type) {
    case 0:// 选项 1: 跨进程通信(IPC), 完整的文作者提供版
        return saliencyDetectionExe(input);
    case 1:// 选项 2: 调用DLL, 文作者提供版但无CSD
        return saliencyDetectionApi(input);
    case 2:// 选项 3: 自实现版, 效果差
        return getSaliencyDetection(input);
    default:
        return getSaliencyDetection(input);
    }
}

/********************************************************************************/
// saliency detection simplified version
/********************************************************************************/

// 辅助函数：将 RGB 量化为索引 key
// 论文中提到将每个通道分为 12 份
inline uint32_t quantifyColor(QRgb color)
{
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

float SaliencyDetection::colorDist(const ColorBin &c1, const ColorBin &c2)
{
    return std::sqrt(std::pow(c1.r - c2.r, 2) +
                     std::pow(c1.g - c2.g, 2) +
                     std::pow(c1.b - c2.b, 2));
}

QImage SaliencyDetection::getSaliencyDetection(const QImage &input)
{
    if (input.isNull()) return QImage();

    QImage img = input.convertToFormat(QImage::Format_RGB888);
    int width = img.width();
    int height = img.height();
    int totalPixels = width * height;

    // 1. 直方图量化 (Histogram Quantization)
    // 使用 map 暂存量化后的颜色
    std::map<uint32_t, ColorBin> hist;
    // 同时也需要保存每个像素对应的 Bin 索引，以便最后生成图像
    std::vector<uint32_t> pixelLabels(totalPixels);

    const uchar *bits = img.bits();
    int bytesPerLine = img.bytesPerLine();

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
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
    for (auto &pair : hist)
    {
        ColorBin &bin = pair.second;
        if (bin.count > 0)
        {
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
    float maxSaliency = 0.0f;

    for (size_t i = 0; i < bins.size(); ++i)
    {
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
        for (size_t j = 0; j < bins.size(); ++j)
        {
            if (i == j) continue;
            float dist = colorDist(bins[i], bins[j]);
            // 累加：距离 * 概率
            globalContrast += dist * bins[j].probability;
        }

        // 融合 GU 和 CSD
        // 这里采用简单的乘法融合，模拟论文中 "Integration" 的效果
        // S(c) = Contrast * (1 - Variance)
        bins[i].saliency = globalContrast * spatialWeight;

        if (bins[i].saliency > maxSaliency)
        {
            maxSaliency = bins[i].saliency;
        }
    }
    // 3. 生成显著性图 (Generate Map)
    QImage result(width, height, QImage::Format_Grayscale8);
    // 归一化参数
    float scale = (maxSaliency > 0) ? (255.0f / maxSaliency) : 0;
    for (int y = 0; y < height; ++y)
    {
        uchar *line = result.scanLine(y);
        for (int x = 0; x < width; ++x)
        {
            uint32_t key = pixelLabels[y * width + x];
            // 找到对应的 Bin
            if (keyToVectorIndex.find(key) != keyToVectorIndex.end())
            {
                int binIdx = keyToVectorIndex[key];
                int val = (int)(bins[binIdx].saliency * scale);
                line[x] = (val > 255) ? 255 : val;
            }
            else
            {
                line[x] = 0;
            }
        }
    }
    return result;
}

/********************************************************************************/
// saliency detection from paper auther
/********************************************************************************/

// saliency detection by dll
QImage SaliencyDetection::saliencyDetectionApi(const QImage &input)
{
    if (input.isNull()) return QImage();

    //LOG_INFO << "[显著性检测] 开始处理图像";
    int w = input.width();
    int h = input.height();
    QImage workImage = input.convertToFormat(QImage::Format_BGR888);

    // 1. 创建一个绝对连续的内存缓冲区
    std::vector<unsigned char> continuous_img_data(w * h * 3);
    for (int y = 0; y < h; ++y)
    {// 逐行拷贝数据，避开 QImage 在行末可能附加的 Padding
        std::memcpy(continuous_img_data.data() + y * w * 3, workImage.scanLine(y), w * 3);
    }
    // 2. 准备输出缓冲区 (8位灰度图，所以大小是 width * height)
    std::vector<unsigned char> out_saliency(w * h, 0);
    // 3. 准备用于接收 DLL 状态的缓冲区
    char dll_status_msg[256] = {0};
    // 4. 调用封装的黑盒 DLL
    int result = compute_saliency_mask(continuous_img_data.data(),
                                       w, h, 3,
                                       out_saliency.data(),
                                       dll_status_msg, sizeof(dll_status_msg));
    // 5. 根据结果进行处理
    if (result == 0)
    {   //LOG_INFO << "[显著性检测] 计算成功完成！";
         QImage maskImage(w, h, QImage::Format_Grayscale8);
        for (int y = 0; y < h; ++y)
        {   // 逐行深拷贝回 QImage，交由 Qt 安全接管内存
            std::memcpy(maskImage.scanLine(y), out_saliency.data() + y * w, w);
        }
        return maskImage;
    }
    else
    {   // 失败时打印调试信息（错误码 + DLL内部传出的最后遗言/异常信息）
        LOG_ERROR << "[显著性检测失败] 错误码: " << result
                  << ", DLL最后状态/异常日志: " << dll_status_msg;
        // 失败时返回空图像，避免返回包含内存脏数据（雪花噪点）的异常图像
        return QImage();
    }
}

// saliency detection by exe
QImage SaliencyDetection::saliencyDetectionExe(const QImage &input)
{
    if (input.isNull()) return QImage();
    //LOG_INFO << "[显著性检测] 启动跨进程 EXE 模式";

    // 1. 生成唯一安全的临时文件路径
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString tempDir = QCoreApplication::applicationDirPath();
    QString inPath = tempDir + "/sal_in_" + uuid + ".png";
    QString outPath = tempDir + "/sal_out_" + uuid + ".png";

    // 2. 将 Qt 的图片先保存到硬盘
    if (!input.save(inPath))
    {
        LOG_ERROR << "[显著性检测] 无法保存临时输入文件";
        return QImage();
    }

    // 3. 配置 QProcess
    QProcess process;
    // 使用 CMake 注入的源码目录宏, 宏 PROJECT_SOURCE_DIR 在编译时会被替换成类似 "D:/Code/Qt/TestLowPoly04" 的字符串
    QString sourceDir = QString(PROJECT_SOURCE_DIR);
    QString exePath = sourceDir + "/exlib/saliency/SaliencyICCV13.exe";
    // 检查 EXE 是否存在
    if (!QFile::exists(exePath))
    {
        LOG_ERROR << "[显著性检测] 找不到核心组件: " << exePath;
        QFile::remove(inPath);
        return QImage();
    }
    // 填入命令行参数
    QStringList args;
    args << inPath << outPath;

    // 4. 执行 EXE 并阻塞等待
    process.start(exePath, args);
    if (!process.waitForStarted())
    {
        LOG_ERROR << "[显著性检测] 无法启动 32 位 EXE 进程";
        QFile::remove(inPath);
        return QImage();
    }
    // 等待算法执行完成(设置最大超时时间为 30 秒，防止死锁)
    if (!process.waitForFinished(30000))
    {
        LOG_ERROR << "[显著性检测] EXE 进程超时或崩溃";
        process.kill();
        QFile::remove(inPath);
        return QImage();
    }

    // 5. 检查执行结果
    int exitCode = process.exitCode();
    if (exitCode == 0)
    {   // 成功时，读取 stdout (捕捉 EXE 中的 cout)
        // QString successLog = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        //LOG_INFO << "[显著性检测] 跨进程返回: " << successLog;

        // 6. 读取算好的显著性结果图
        QImage result(outPath);
        if (result.isNull())
        {
            LOG_ERROR << "[显著性检测] 无法读取输出文件, 可能 EXE 内部生成失败";
        }/*else{   // 强制确保格式为 8 位灰度
            result = result.convertToFormat(QImage::Format_Grayscale8);
        }*/

        // 7. 环保清理: 删掉临时图片
        QFile::remove(inPath);
        QFile::remove(outPath);

        return result;
    }
    else
    {   // 失败时，读取 stderr (捕捉 EXE 中的 cerr)
        QString errorLog = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        LOG_ERROR << "[显著性检测失败] 错误码: " << exitCode << ", 原因: " << errorLog;
        // 环保清理
        QFile::remove(inPath);
        // 尝试删除outPath(出错时大概率没有生成)
        QFile::remove(outPath);

        return QImage();
    }
}

/********************************************************************************/
// generate nonuniform samples
/********************************************************************************/

// generate external nonuniform samples
std::vector<QPoint> SaliencyDetection::generateNonUniformSamples(
    int Lw, int Lh,                 // 图像宽高
    int Nc,                         // 已知约束点数量(特征点 + 4个角点)
    double eta,                     // 采样间隔控制参数
    const QImage& saliencyMap       // 显著性/掩码图 (二维数组)
    )
{
    if(Lw <=0 || Lh <= 0 || eta <=0 || Nc <= 0 || saliencyMap.format() != QImage::Format_Grayscale8)
    {
        LOG_ERROR << "Saliency Detection: sampling params error";
        return {};
    }
    // 1. 计算总采样数 N
    int N = sDParams.userN;
    if (N <= 0) {
        // 根据论文公式计算采样间隔 Li
        double Li = eta * (Lw + Lh);
        // 默认总点数公式
        N = static_cast<int>(std::floor(Lw / Li) * std::floor(Lh / Li));
    }

    // 2. 计算需要采样的自由点数
    int remainingPoints = N - Nc;
    if (remainingPoints <= 0) {
        return {}; // 约束点已够, 无需额外撒点
    }

    // 3. 计算前景(Ns)和背景(Nb)的采样数量
    int Ns = static_cast<int>(std::round(sDParams.lambda * remainingPoints));
    if (Ns < 0) Ns = 0;
    if (Ns > remainingPoints) Ns = remainingPoints;
    int Nb = remainingPoints - Ns;

    // 4. 将所有像素划分为前景候选池和背景候选池
    std::vector<QPoint> salientPool;
    std::vector<QPoint> backgroundPool;
    // 预分配内存以提高性能
    salientPool.reserve((Lw * Lh) / 2);
    backgroundPool.reserve((Lw * Lh) / 2);
    for (int y = 0; y < Lh; ++y)
    {
        const uchar* scan = saliencyMap.constScanLine(y);
        for (int x = 0; x < Lw; ++x)
        {
            if (scan[x] >= sDParams.threshold) {
                salientPool.emplace_back(x, y);
            } else {
                backgroundPool.emplace_back(x, y);
            }
        }
    }

    // 5. 随机抽取
    std::vector<QPoint> sampledPoints;
    sampledPoints.reserve(remainingPoints);

    std::random_device rd;
    std::mt19937 gen(rd());

    // 辅助 Lambda：从候选池中打乱并抽取指定数量的点
    // C++17 替代 std::shuffle 的写法: 使用std::sample
    auto extractSamples = [&](std::vector<QPoint>& pool, int count)
    {
        if (count <= 0 || pool.empty())
            return;
        std::shuffle(pool.begin(), pool.end(), gen);
        int actualCount = std::min(count, static_cast<int>(pool.size()));
        sampledPoints.insert(sampledPoints.end(), pool.begin(), pool.begin() + actualCount);
    };

    extractSamples(salientPool, Ns);
    extractSamples(backgroundPool, Nb);

    return sampledPoints;
}
