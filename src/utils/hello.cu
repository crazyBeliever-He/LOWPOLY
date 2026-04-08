#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdio.h>
#include "test.h"

// __global__ 关键字告诉编译器，这个函数在 GPU 上执行，但可以被 CPU 调用
__global__ void helloFromGPU() {
    // threadIdx.x 是 CUDA 的内置变量，表示当前执行该代码的 GPU 线程编号
    printf("Hello from GPU! I am thread: %d\n", threadIdx.x);
}

// 这是一个普通的 C++ 包装函数，用于在主机 (CPU) 端启动 GPU 核函数
void runHelloCUDA() {
    printf("--- GPU Computation Started ---\n");
    
    // <<<1, 5>>> 表示启动 1 个线程块（Block），该块内有 5 个线程（Thread）
    // 这意味着 helloFromGPU 函数会被 GPU 同时并行执行 5 次
    helloFromGPU<<<1, 5>>>();
    
    // 强制 CPU 等待 GPU 执行完毕。
    // 如果没有这一句，CPU 跑得太快直接退出了，你可能就看不到 GPU 的打印信息了。
    cudaDeviceSynchronize(); 
    
    printf("--- GPU Computation Finished ---\n");
}
