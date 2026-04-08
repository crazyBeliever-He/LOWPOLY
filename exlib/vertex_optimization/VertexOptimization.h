#pragma once

#ifdef VERTEX_OPT_EXPORTS
    #define VERTEX_OPT_API __declspec(dllexport)
#else
    #define VERTEX_OPT_API __declspec(dllimport)
#endif

namespace VertexOpt {

    extern "C" {

        /**
         * @brief 亚像素精度的点，用于 Lloyd 松弛迭代
         */
        struct OptPoint {
            float x;
            float y;
        };

        /**
         * @brief 执行基于特征流场引导的 Voronoi 顶点优化 (Lloyd Relaxation)
         * @param free_points          自由活动点数组 (显著性采样点)，计算完成后会被就地更新为新坐标
         * @param num_free_points      自由活动点的数量
         * @param constrained_points   绝对静止点数组 (Douglas-Peucker结果 + 4个角点)
         * @param num_constrained_pts  绝对静止点的数量
         * @param feature_flow_map     特征流场 F(x) 数组 (大小: width * height)
         * @param width                图像宽度
         * @param height               图像高度
         * @param iterations           迭代次数 (论文推荐 10 次)
         * @param error_msg            错误信息写入缓冲区
         * @param max_error_len        错误信息缓冲区的最大长度
         * @return true                计算成功
         * @return false               计算失败
         */
        VERTEX_OPT_API bool optimize_vertices_api(
            OptPoint* free_points,
            int num_free_points,
            const OptPoint* constrained_points,
            int num_constrained_pts,
            const float* feature_flow_map,
            int width,
            int height,
            int iterations,
            char* error_msg,
            int max_error_len
        );
    }

} // namespace VertexOpt