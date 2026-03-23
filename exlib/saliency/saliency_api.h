#pragma once
#ifndef SALIENCY_API_H
#define SALIENCY_API_H

// 工业级 DLL 导出/导入宏控制
// 在编译此 DLL 的工程属性中，需定义宏 SALIENCY_EXPORTS (预处理器定义)
// 外部项目直接包含此头文件时，会自动切换为 dllimport
#ifdef _WIN32
    #ifdef SALIENCY_EXPORTS
        #define SALIENCY_API __declspec(dllexport)
    #else
        #define SALIENCY_API __declspec(dllimport)
    #endif
#else
    #define SALIENCY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief 显著性检测状态码枚举
     */
    typedef enum {
        SALIENCY_SUCCESS = 0,               // 计算成功
        SALIENCY_ERR_INVALID_ARGS = -1,     // 无效的输入参数 (如空指针、尺寸错误)
        SALIENCY_ERR_OPENCV = -2,           // OpenCV 内部异常崩溃
        SALIENCY_ERR_STD = -3,              // C++ 标准异常
        SALIENCY_ERR_UNKNOWN = -4           // 未知致命错误
    } SaliencyStatus;

    /**
     * @brief 计算 RGB/BGR 图像的显著性掩码 (Low Poly 适用版本)
     * * @details
     * 该接口通过计算全局颜色对比度 (Global Uniqueness, GU) 来提取图像的显著性图。
     * 剔除了基于 CSD (颜色空间分布) 的计算分支，解除了对外部 32 位 apclusterwin.dll 的依赖，
     * * @param img_data      [in] 指向原始图像像素数据的连续内存指针。
     * 必须是每像素 3 通道 (8-bit) 的交错排列数据。
     * 注意：内存必须绝对连续，不允许存在行末对齐填充 (Padding)。
     * 由于 OpenCV 内部默认使用 BGR，建议传入 BGR888 格式。
     * @param width         [in] 图像的宽度 (像素)。
     * @param height        [in] 图像的高度 (像素)。
     * @param channels      [in] 图像的通道数，当前版本必须固定传入 3。
     * @param out_saliency  [out] 用于接收输出掩码的预分配内存指针。
     * 大小必须严格等于 width * height 字节。
     * 输出结果为 8 位单通道灰度图 (0-255，值越大越显著)。
     * @param error_msg     [out] (可选) 用于接收执行状态或崩溃日志的字符缓冲区指针。
     * 如果不关心错误信息，可传入 NULL。
     * @param max_error_len [in]  error_msg 缓冲区的最大长度 (包含 \0)。
     * * @return SaliencyStatus 0 表示成功，非 0 表示对应类型的错误码。
     */
    SALIENCY_API int __cdecl compute_saliency_mask(
        const unsigned char* img_data,
        int width,
        int height,
        int channels,
        unsigned char* out_saliency,
        char* error_msg,
        int max_error_len
    );

#ifdef __cplusplus
}
#endif

#endif // SALIENCY_API_H