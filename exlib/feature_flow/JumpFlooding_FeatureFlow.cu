// 定义导出宏，激活 dllexport
#define JFA_FF_EXPORTS

#include "JumpFlooding_FeatureFlow.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cmath>
#include <algorithm>

namespace JFAFF {

// 遵循 C99/C++11 标准的跨平台写法
// 更高的代码质量，把这两个变量显式传给宏，UPDATE_STATUS(err_buf, max_len, ...)
#define UPDATE_STATUS(...) \
    do { \
        if (error_msg != nullptr && max_error_len > 0) { \
            snprintf(error_msg, max_error_len, __VA_ARGS__); \
        } \
    } while(0)

// =====================================================================
// 独立验证模块
// =====================================================================
// 检查 CUDA 环境是否可用
bool CheckCudaAvailable(char* error_msg, int max_error_len) 
{
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);

    if (err != cudaSuccess || deviceCount == 0) 
    {
        UPDATE_STATUS("Cannot use CUDA: No CUDA-capable devices found or driver error. (err code: %d, %s)",
            (int)err, cudaGetErrorString(err));
        return false;
    }
    return true;
}

// 检查输入参数和内存缓冲区的合法性
bool ValidateBuffers(const JFAPoint * in, JFAPoint * out, int width, int height, char* error_msg, int max_error_len) 
{
    if (width <= 0 || height <= 0) 
    {
        UPDATE_STATUS("Invalid dimensions: width (%d) and height (%d) must be > 0.", width, height);
        return false;
    }
    if (in == nullptr || out == nullptr) 
    {
        UPDATE_STATUS("Invalid buffers: input or output pointers cannot be null.");
        return false;
    }
    return true;
}

// 针对 FeatureFlow 的独立验证模块
bool ValidateFeatureFlowBuffers(const JFAPoint* in, float* out, int width, int height, float m, char* error_msg, int max_error_len) 
{
    if (width <= 0 || height <= 0) 
    {
        UPDATE_STATUS("Invalid dimensions: width (%d) and height (%d) must be > 0.", width, height);
        return false;
    }
    if (in == nullptr || out == nullptr) 
    {
        UPDATE_STATUS("Invalid buffers: input or output pointers cannot be null.");
        return false;
    }
    if (m <= 0.0f) 
    {
        UPDATE_STATUS("Invalid parameter: 'm' must be strictly greater than 0.");
        return false;
    }
    return true;
}

// =====================================================================
// Jump Flooding Algorithm CUDA Kernel
// =====================================================================

__global__ void jfa_step_kernel(const JFAPoint* __restrict__ input_map, JFAPoint* __restrict__ output_map, int step, int width, int height)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    int idx = y * width + x;

    // 使用 __restrict__ 关键字向编译器保证指针不混叠，这会自动触发硬件的只读数据缓存 (Texture Cache) 加速读取
    JFAPoint best_seed = input_map[idx];
    float best_dist = FLT_MAX; 

    // 计算当前位置到已知种子的距离
    if (best_seed.x != -1) 
    {
        float dx0 = (float)(x - best_seed.x);
        float dy0 = (float)(y - best_seed.y);
        best_dist = dx0 * dx0 + dy0 * dy0;
    }

    // 遍历周围 9 个邻居 (步长为 step)
    // 使用 #pragma unroll 完全展开固定边界 (-1 到 1) 的循环，消除循环控制开销
    #pragma unroll
    for (int j = -1; j <= 1; ++j) 
    {
        int ny = y + j * step;
        // 将 Y 轴的越界检查提到外层循环，减少内层 3 次无意义的判断
        if (ny >= 0 && ny < height)
        {
        #pragma unroll
            for (int i = -1; i <= 1; ++i)
            {
                int nx = x + i * step;
                // 越界检查
                if (nx >= 0 && nx < width)
                {
                    int n_idx = ny * width + nx;
                    JFAPoint neighbor_seed = input_map[n_idx];

                    if (neighbor_seed.x != -1)
                    {
                        float dx = static_cast<float>(x - neighbor_seed.x);
                        float dy = static_cast<float>(y - neighbor_seed.y);
                        float dist = dx * dx + dy * dy;

                        // 如果找到更近的种子，更新它
                        if (dist < best_dist)
                        {
                            best_dist = dist;
                            best_seed = neighbor_seed;
                        }
                    }
                }
            }
        }
    }
    // 写入当前轮次的最优结果
    output_map[idx] = best_seed;
}

// =====================================================================
// Feature Flow CUDA Kernel
// =====================================================================
__global__ void feature_flow_kernel(const JFAPoint* __restrict__ seed_map, float* __restrict__ flow_map, int width, int height, float inv_m)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    int idx = y * width + x;
    JFAPoint seed = seed_map[idx];

    if (seed.x != -1)
    {
        // 1. 计算当前像素到最近种子的真实欧氏距离 D(x)
        float dx = static_cast<float>(x - seed.x);
        float dy = static_cast<float>(y - seed.y);
        float d_x = sqrtf(dx * dx + dy * dy);

        // 2. 计算 F(x) 经过优化的版本(另: 论文公式出现笔误)
        // 可知 fmodf(d_x, m) == d_x - floor(d_x / m) * m == m * (d_x / m -  floor(d_x / m))
        // 将传入的 m 改为从 CPU 传入 inv_m (即 1.0f / m)，化除法为乘法
        float ratio = d_x * inv_m;

        // 3. 提取整数部分和小数部分
        float floor_f = floorf(ratio);  //floorf 向下取整
        float frac = ratio - floor_f; // 范围 [0, 1)，等价于推导的优化
        int floor_val = static_cast<int>(floor_f);

        // 4. 位运算判断奇偶 + 三元运算符消除分支发散
        flow_map[idx] = ((floor_val & 1) == 0) ? (255.0f * frac) : (255.0f * (1.0f - frac));
    }
    else 
    {
        flow_map[idx] = 0.0f; // 没有种子的情况 (理论上JFA执行完不会存在)
    }
}

// =====================================================================
// 导出的 C 接口
// =====================================================================
extern "C" JFA_FF_API bool jump_flooding_api(
    const JFAPoint* input_seed_map,
    JFAPoint* output_map,
    int width,
    int height,
    char* error_msg,
    int max_error_len
) {
    // 1. 独立检查模块拦截
    if (!CheckCudaAvailable(error_msg, max_error_len)) return false;
    if (!ValidateBuffers(input_seed_map, output_map, width, height, error_msg, max_error_len)) return false;

    size_t total_pixels = (size_t)width * height;
    size_t map_size_bytes = total_pixels * sizeof(JFAPoint);

    // 2. 申请显存 (Ping-Pong 缓冲区)
    JFAPoint* d_map_in = nullptr;
    JFAPoint* d_map_out = nullptr;

    cudaError_t err1 = cudaMalloc(&d_map_in, map_size_bytes);
    cudaError_t err2 = cudaMalloc(&d_map_out, map_size_bytes);

    if (err1 != cudaSuccess || err2 != cudaSuccess)
    {
        cudaError_t actual_err = (err1 != cudaSuccess) ? err1 : err2;
        UPDATE_STATUS("CUDA memory allocation failed: %s (code: %d)", cudaGetErrorString(actual_err), (int)actual_err);
        if (d_map_in) cudaFree(d_map_in);
        if (d_map_out) cudaFree(d_map_out);
        return false;
    }

    // 3. 将初始种子图拷贝到显存
    cudaError_t memcpy_err = cudaMemcpy(d_map_in, input_seed_map, map_size_bytes, cudaMemcpyHostToDevice);
    if (memcpy_err != cudaSuccess)
    {
        UPDATE_STATUS("Host to Device memory copy failed: %s (code: %d)", cudaGetErrorString(memcpy_err), (int)memcpy_err);
        cudaFree(d_map_in);
        cudaFree(d_map_out);
        return false;
    }

    // 4. 动态获取针对 jfa_step_kernel 获取最优的 Kernel 启动配置 (Occupancy API)
    int minGridSize = 0;
    int bestBlockSize1D = 0;

    cudaError_t occErr = cudaOccupancyMaxPotentialBlockSize(
        &minGridSize,
        &bestBlockSize1D,
        jfa_step_kernel,
        0,
        0
    );

    if (occErr != cudaSuccess)
    {
        UPDATE_STATUS("Failed to calculate optimal block size: %s (code: %d)", cudaGetErrorString(occErr), (int)occErr);
        cudaFree(d_map_in);
        cudaFree(d_map_out);
        return false;
    }

    int deviceId;
    cudaGetDevice(&deviceId);
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, deviceId);

    int blockX = 32;
    if (blockX > deviceProp.maxThreadsDim[0])
        blockX = deviceProp.maxThreadsDim[0];

    int blockY = bestBlockSize1D / blockX;
    if (blockY > deviceProp.maxThreadsDim[1])
        blockY = deviceProp.maxThreadsDim[1];
    if (blockY <= 0) blockY = 1;

    dim3 blockSize(blockX, blockY);
    dim3 gridSize(
        (width + blockSize.x - 1) / blockSize.x,
        (height + blockSize.y - 1) / blockSize.y
    );

    // 5. JFA 迭代逻辑
    int max_dim = std::max(width, height);
    int step = 1;
    while (step < max_dim) {
        step *= 2;
    }

    // === 开始纯内核计时 ===
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    while (step >= 1)
    {
        jfa_step_kernel << <gridSize, blockSize >> > (d_map_in, d_map_out, step, width, height);

        cudaError_t kernel_launch_err = cudaGetLastError();
        if (kernel_launch_err != cudaSuccess)
        {
            UPDATE_STATUS("CUDA kernel launch failed at step %d: %s (code: %d)", step, cudaGetErrorString(kernel_launch_err), (int)kernel_launch_err);
            cudaFree(d_map_in);
            cudaFree(d_map_out);
            cudaEventDestroy(start);
            cudaEventDestroy(stop);
            return false;
        }

        cudaError_t kernel_sync_err = cudaDeviceSynchronize();
        if (kernel_sync_err != cudaSuccess)
        {
            UPDATE_STATUS("CUDA kernel execution (sync) failed at step %d: %s (code: %d)", step, cudaGetErrorString(kernel_sync_err), (int)kernel_sync_err);
            cudaFree(d_map_in);
            cudaFree(d_map_out);
            cudaEventDestroy(start);
            cudaEventDestroy(stop);
            return false;
        }

        std::swap(d_map_in, d_map_out);
        step /= 2;
    }

    // === 结束纯内核计时 ===
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float jfa_ms = 0;
    cudaEventElapsedTime(&jfa_ms, start, stop);

    // 销毁事件对象防止显存泄漏
    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    // 6. 将结果拷贝回调用方预分配的内存
    cudaError_t memcpy_back_err = cudaMemcpy(output_map, d_map_in, map_size_bytes, cudaMemcpyDeviceToHost);
    if (memcpy_back_err != cudaSuccess)
    {
        UPDATE_STATUS("Device to Host memory copy failed: %s (code: %d)", cudaGetErrorString(memcpy_back_err), (int)memcpy_back_err);
        cudaFree(d_map_in);
        cudaFree(d_map_out);
        return false;
    }

    // 7. 释放显存
    cudaFree(d_map_in);
    cudaFree(d_map_out);

    // 8. 返回成功消息并携带运行时间
    UPDATE_STATUS("Jump Flooding completed successfully. Kernel time: %.3f ms", jfa_ms);
    return true;
}

extern "C" JFA_FF_API bool feature_flow_api(
    const JFAPoint* nearest_seed_map,
    float* output_flow_map,
    int width,
    int height,
    float m,
    char* error_msg,
    int max_error_len
) {
    // 1. 独立检查拦截
    if (!CheckCudaAvailable(error_msg, max_error_len)) return false;
    if (!ValidateFeatureFlowBuffers(nearest_seed_map, output_flow_map, width, height, m, error_msg, max_error_len)) return false;

    size_t total_pixels = (size_t)width * height;
    size_t input_size_bytes = total_pixels * sizeof(JFAPoint);
    size_t output_size_bytes = total_pixels * sizeof(float);

    // 2. 申请显存
    JFAPoint* d_seed_map = nullptr;
    float* d_flow_map = nullptr;

    cudaError_t err1 = cudaMalloc(&d_seed_map, input_size_bytes);
    cudaError_t err2 = cudaMalloc(&d_flow_map, output_size_bytes);

    if (err1 != cudaSuccess || err2 != cudaSuccess) {
        cudaError_t actual_err = (err1 != cudaSuccess) ? err1 : err2;
        UPDATE_STATUS("CUDA memory allocation failed: %s (code: %d)", cudaGetErrorString(actual_err), (int)actual_err);
        if (d_seed_map) cudaFree(d_seed_map);
        if (d_flow_map) cudaFree(d_flow_map);
        return false;
    }

    // 3. 将 Host 的最近种子图传入 Device
    cudaError_t memcpy_err = cudaMemcpy(d_seed_map, nearest_seed_map, input_size_bytes, cudaMemcpyHostToDevice);
    if (memcpy_err != cudaSuccess)
    {
        UPDATE_STATUS("Host to Device memory copy failed: %s (code: %d)", cudaGetErrorString(memcpy_err), (int)memcpy_err);
        cudaFree(d_seed_map);
        cudaFree(d_flow_map);
        return false;
    }

    // 4. Occupancy API 计算最佳线程块
    int minGridSize;
    int bestBlockSize1D;
    cudaError_t occErr = cudaOccupancyMaxPotentialBlockSize(
        &minGridSize,
        &bestBlockSize1D,
        feature_flow_kernel,
        0,
        0
    );

    if (occErr != cudaSuccess)
    {
        UPDATE_STATUS("Failed to calc block size: %s (code: %d)", cudaGetErrorString(occErr), (int)occErr);
        cudaFree(d_seed_map);
        cudaFree(d_flow_map);
        return false;
    }

    int deviceId;
    cudaGetDevice(&deviceId);
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, deviceId);

    int blockX = 32;
    if (blockX > deviceProp.maxThreadsDim[0])
        blockX = deviceProp.maxThreadsDim[0];

    int blockY = bestBlockSize1D / blockX;
    if (blockY > deviceProp.maxThreadsDim[1])
        blockY = deviceProp.maxThreadsDim[1];
    if (blockY <= 0)
        blockY = 1;

    dim3 blockSize(blockX, blockY);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
        (height + blockSize.y - 1) / blockSize.y);

    // 5. 启动 Kernel
    float inv_m = 1.0f / m;

    // === 开始纯内核计时 ===
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    feature_flow_kernel << <gridSize, blockSize >> > (d_seed_map, d_flow_map, width, height, inv_m);

    cudaError_t kernel_launch_err = cudaGetLastError();
    if (kernel_launch_err != cudaSuccess)
    {
        UPDATE_STATUS("FeatureFlow kernel launch failed: %s (code: %d)", cudaGetErrorString(kernel_launch_err), (int)kernel_launch_err);
        cudaFree(d_seed_map);
        cudaFree(d_flow_map);
        cudaEventDestroy(start);
        cudaEventDestroy(stop);
        return false;
    }

    cudaError_t kernel_sync_err = cudaDeviceSynchronize();
    if (kernel_sync_err != cudaSuccess)
    {
        UPDATE_STATUS("FeatureFlow kernel execution (sync) failed: %s (code: %d)", cudaGetErrorString(kernel_sync_err), (int)kernel_sync_err);
        cudaFree(d_seed_map);
        cudaFree(d_flow_map);
        cudaEventDestroy(start);
        cudaEventDestroy(stop);
        return false;
    }

    // === 结束纯内核计时 ===
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float ff_ms = 0;
    cudaEventElapsedTime(&ff_ms, start, stop);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    // 6. 拷贝结果回 Host
    cudaError_t memcpy_back_err = cudaMemcpy(output_flow_map, d_flow_map, output_size_bytes, cudaMemcpyDeviceToHost);
    if (memcpy_back_err != cudaSuccess)
    {
        UPDATE_STATUS("Device to Host memory copy failed: %s (code: %d)", cudaGetErrorString(memcpy_back_err), (int)memcpy_back_err);
        cudaFree(d_seed_map);
        cudaFree(d_flow_map);
        return false;
    }

    // 7. 收尾清理
    cudaFree(d_seed_map);
    cudaFree(d_flow_map);

    // 8. 返回成功消息并携带运行时间
    UPDATE_STATUS("Feature Flow completed successfully. Kernel time: %.3f ms", ff_ms);
    return true;
}

} // namespace JFAFF