#pragma once

#ifdef JFA_FF_EXPORTS
    #define JFA_FF_API __declspec(dllexport)
#else
    #define JFA_FF_API __declspec(dllimport)
#endif

namespace JFAFF {

extern "C" {

    /**
     * @brief 定义在 DLL 边界传输的像素点结构，当 x = -1 且 y = -1 时，表示该位置没有种子 (Empty)
     */
    struct JFAPoint {
        int x;
        int y;
    };
    /**
     * @brief 执行 CUDA 版本的跳跃泛洪算法 (JFA)
     * * @param input_seed_map 调用方预先准备好的种子图，大小为 width * height。
     * 如果是种子点，存入其 {x, y}；如果是背景点，必须存入 {-1, -1}。
     * @param output_map     调用方预先申请好的输出空间，大小为 width * height。
     * 计算成功后，每个像素将存储距离其最近的种子点的 {x, y}。
     * @param width          图像宽度
     * @param height         图像高度
     * @param error_msg      错误信息写入缓冲区
     * @param max_error_len  错误信息缓冲区的最大长度
     * @return true          计算成功
     * @return false         计算失败（如不支持 CUDA，内存不足等），具体原因见 error_msg
     */
    JFA_FF_API bool jump_flooding_api(
        const JFAPoint* input_seed_map,
        JFAPoint* output_map,
        int width, 
        int height,
        char* error_msg, 
        int max_error_len
    );

    /**
     * @brief 根据 JFA 生成的最近种子图，计算特征流场 F(x)
     * @param nearest_seed_map  由 JumpFlooding 算出的最近种子图 (大小: width * height)
     * @param output_flow_map   输出的特征流场权重图，值范围 0~255 (由调用方分配的 float 数组，大小: width * height)
     * @param width             图像宽度
     * @param height            图像高度
     * @param m                 控制流场通道间隔宽度的参数 (论文建议: m = Li / 2)
     * @param error_msg         用于接收错误/状态信息的字符缓冲区 (由调用方分配)
     * @param max_error_len     error_msg 缓冲区的最大长度
     * @return true 成功, false 失败
     */
    JFA_FF_API bool feature_flow_api(
        const JFAPoint* nearest_seed_map,
        float* output_flow_map,
        int width,
        int height,
        float m,
        char* error_msg,
        int max_error_len
    );
}

} // namespace JFAFF