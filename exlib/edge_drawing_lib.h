#ifndef OPENCVEDLIB_H  
#define OPENCVEDLIB_H  

#ifdef OPENCVEDLIB_EXPORTS
#define ED_API __declspec(dllexport)
#else
#define ED_API __declspec(dllimport)
#endif

namespace opencved
{   // 使用 extern "C" 包含结构体定义, 确保 ABI 兼容性并消除警告
    extern "C" {
        /**
         * @brief 基础点结构体
         * 用于跨 DLL 边界传输像素坐标, 避免直接依赖 OpenCV 的 cv::Point
         */
        struct EDPoint {
            int x;  // 横坐标
            int y;  // 纵坐标
        };

        /**
         * @brief 边缘链结构体
         * 代表一条连续的, 1像素宽的边缘段(由一系列像素点组成)
         */
        struct EDEdgeSegment {
            EDPoint* points;    // 指向像素点数组的指针
            int count;          // 当前边缘段包含的像素点数量
        };

        /**
         * @brief 边缘检测结果集
         * 包含图像中检测到的所有边缘段
         */
        struct EDResults {
            EDEdgeSegment* segments;    // 指向边缘段数组的指针
            int segmentCount;           // 检测到的边缘段总数
        };

        /**
         * @brief ColorED 算法参数配置结构体
         * 包含从底层梯度计算到高层几何拟合的所有控制变量
         */
        struct EDParams {
            // --- 核心边缘检测 (Edge Drawing) 参数 ---

            //! Parameter Free mode will be activated when this value is set as true. Default value is false.
            // 参数自由模式: 如果设为 true, 算法会启用基于 NFA(虚警数)的验证步骤, 自动调整部分内部逻辑以减少误检.
            bool PFmode;

            /** @brief indicates the operator used for gradient calculation.

            one of the flags cv::ximgproc::EdgeDrawing::GradientOperator. Default value is PREWITT.

            梯度算子: 0:PREWITT, 1:SOBEL, 2:SCHARR, 3:LSD.
            */
            int EdgeDetectionOperator;

            //! threshold value of gradiential difference between pixels. Used to create gradient image. Default value is 20.
            // 梯度阈值: 只有梯度幅值大于此值的像素才会被视为潜在边缘像素. 若值<1则自动修正为1, 建议20-40.
            int GradientThresholdValue;

            //! threshold value used to select anchor points. Default value is 0.
            //锚点阈值: 在提取锚点(边缘骨架点)时, 局部极大值点需超过邻域梯度值的差值. 设为 0 表示只要是局部极大值即可.
            int AnchorThresholdValue;

            //! Default value is 1
            // 扫描间隔: 在寻找锚点时跳过的像素行/列数. 值越大处理速度越快, 但可能遗漏细小边缘.
            int ScanInterval;

            /** @brief minimun connected pixels length processed to create an edge segment.

            in gradient image, minimum connected pixels length processed to create an edge segment. pixels having upper value than GradientThresholdValue
            will be processed. Default value is 10

             最小路径长度: 检测出的边缘段"像素链"必须包含的最小像素数. 短于此值的链将被舍弃.
            */
            int MinPathLength;

            //! sigma value for internal GaussianBlur() function. Default value is 1.0
            // 高斯平滑系数: 用于预处理去噪. 如果 "<1.0" 则不平滑; 如果是彩色图像, 会应用于 L/a/b 三个通道.
            float Sigma;

            // 梯度计算方式: true使用简易求和(近似梯度|gx|+|gy|), false使用平方根. Default value is true.
            bool SumFlag;

            // --- 几何形状检测 (Line/Ellipse) 参数 ---

            //! Default value is true. indicates if NFA (Number of False Alarms) algorithm will be used for line and ellipse validation.
            // 是否开启基于 Helmholtz 原理的边缘验证, 能有效滤除纹理产生的噪声边缘. 通过概率模型过滤掉噪声产生的虚假线段和圆.
            bool NFAValidation;

            //! minimun line length to detect. Default value is -1
            // 最小直线长度：检测到的直线段若短于此值则舍弃. 若设为 -1, 算法会根据图像尺寸自动计算一个理论最小值.
            int MinLineLength;

            //! Default value is 6.0
            // 线段合并阈值：两条线段距离小于此间距时尝试合并.
            double MaxDistanceBetweenTwoLines;

            //! Default value is 1.0
            // 直线拟合误差：最小二乘法拟合时的平均偏差限制. 直线拟合的最大距离误差, 控制像素点偏离理想直线的程度.
            double LineFitErrorThreshold;

            //! Default value is 1.3
            // 圆/椭圆拟合最大误差阈值：像素点到拟合圆/椭圆边界的最大允许距离.
            double MaxErrorThreshold;
        };

        /**
         * @brief 基础边缘检测(ColorED)
         * @param data 图像数据指针(8-bit unsigned char), 支持灰度(1通道)或彩色(3通道 - BGR顺序)
         * @param width 图像宽度
         * @param height 图像高度
         * @param step 图像行跨度 (stride/pitch)相邻两行像素首地址之间的字节数差值
         * @param isColor 是否为彩色。
         * - true: 3通道。建议输入BGR顺序(OpenCV标准), 算法会转为 Lab 空间
         * - false: 单通道灰度图
         * @param p 算法参数
         * * @return EDResults 包含所有检测到的像素链
         * 注意: 返回的内存是在 DLL 堆上分配的, 必须调用 FreeEDResults 释放, 否则会内存泄漏.
         */
        ED_API struct EDResults RunEdgeDrawingFull(unsigned char* data, int width, int height, int step, bool isColor, EDParams p);

        /**
         * @brief 直线段检测 (EDLines)
         * @details 在 detectEdges 的基础上提取矢量化的直线段
         * @param linesData [输出] 浮点数组, 每4个元素代表一条线 [x1, y1, x2, y2]. 原点(0,0)位于图像左上角.
         * @param maxLines 外部缓冲区允许存储的最大直线数量
         * @return 实际检测到的直线数量
         */
        ED_API int RundetectLines(unsigned char* data, int width, int height, int step, bool isColor, EDParams p, float* linesData, int maxLines);

        /**
         * @brief 圆和椭圆检测 (EDCircles)
         * @details 在 detectEdges 的基础上提取圆和椭圆信息
         * @param ellipseData [输出] 双精度数组, 每6个元素代表一个椭圆 [cX, cY, a, b, angle, ...]. 原点(0,0)位于图像左上角.
         * @param maxEllipses 外部缓冲区允许存储的最大椭圆数量
         * @return 实际检测到的圆/椭圆数量
         */
        ED_API int RundetectEllipses(unsigned char* data, int width, int height, int step, bool isColor, EDParams p, double* ellipseData, int maxEllipses);

        /**
         * @brief 内存释放辅助函数
         * @details 必须调用此函数来释放由 RunEdgeDrawingFull 在堆上分配的像素链内存.
         */
        ED_API void FreeEDResults(EDResults results);

    };// extern "C"

};// namespace opencved

#endif // OPENCVEDLIB_H