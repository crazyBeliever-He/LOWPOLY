#define VERTEX_OPT_EXPORTS

#include "VertexOptimization.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstdio>

#define UPDATE_STATUS(err_buf, err_len, ...) \
    do { \
        if ((err_buf) != nullptr && (err_len) > 0) { \
            snprintf((err_buf), (err_len), __VA_ARGS__); \
        } \
    } while(0)

namespace VertexOpt {

// 辅助函数：根据中心点匹配，寻找其在 free_points 中的索引
// OpenCV 的 Subdiv2D 返回的面片中心点可能存在微小的浮点误差
int findFreePointIndex(const cv::Point2f& center, OptPoint* free_points, int num_free_points) {
    float min_dist_sq = 1e6f;
    int best_idx = -1;
    for (int i = 0; i < num_free_points; ++i) {
        float dx = center.x - free_points[i].x;
        float dy = center.y - free_points[i].y;
        float dist_sq = dx * dx + dy * dy;
        // 阈值设置为一个极小值，判定为同一个点
        if (dist_sq < 1e-4f && dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            best_idx = i;
        }
    }
    return best_idx;
}

extern "C" {

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
    int max_error_len) 
{
    if (!free_points || !constrained_points || !feature_flow_map || width <= 0 || height <= 0) {
        UPDATE_STATUS(error_msg, max_error_len, "Invalid pointers or dimensions.");
        return false;
    }

    cv::Rect rect(0, 0, width, height);

    for (int iter = 0; iter < iterations; ++iter) {
        cv::Subdiv2D subdiv(rect);

        // 1. 插入所有点
        for (int i = 0; i < num_free_points; ++i) 
        {
            subdiv.insert(cv::Point2f(free_points[i].x, free_points[i].y));
        }
        for (int i = 0; i < num_constrained_pts; ++i) 
        {
            subdiv.insert(cv::Point2f(constrained_points[i].x, constrained_points[i].y));
        }

        // 2. 获取 Voronoi 区域 (Facets)
        std::vector<std::vector<cv::Point2f>> facets;
        std::vector<cv::Point2f> centers;
        subdiv.getVoronoiFacetList(std::vector<int>(), facets, centers);

        // 用于临时存放本轮计算出的新坐标，避免在迭代中途破坏拓扑
        std::vector<cv::Point2f> new_positions(num_free_points);
        std::vector<bool> point_updated(num_free_points, false);

        // 3. 遍历每个 Voronoi 单元
        for (size_t i = 0; i < facets.size(); i++) 
        {
            int free_idx = findFreePointIndex(centers[i], free_points, num_free_points);
            
            // 如果这个单元的中心不是自由点（说明它是约束点），则跳过 
            if (free_idx == -1) continue;

            // 获取该面片的包围盒 (Bounding Box)，优化像素遍历范围
            cv::Rect bbox = cv::boundingRect(facets[i]);
            bbox &= cv::Rect(0, 0, width, height); // 确保不越界

            // 将浮点多边形转换为 OpenCV 绘制/判断用的整型多边形
            std::vector<cv::Point> facet_int;
            facet_int.reserve(facets[i].size());
            for (const auto& pt : facets[i]) 
            {
                facet_int.push_back(cv::Point(cvRound(pt.x), cvRound(pt.y)));
            }

            float sum_x = 0.0f, sum_y = 0.0f, sum_w = 0.0f;

            // 4. 遍历多边形内的像素并积分
            for (int y = bbox.y; y < bbox.y + bbox.height; ++y) 
            {
                for (int x = bbox.x; x < bbox.x + bbox.width; ++x) 
                {
                    // 判断像素 (x, y) 是否在 Voronoi 单元多边形内 (>=0 表示在内部或边缘)
                    if (cv::pointPolygonTest(facet_int, cv::Point2f(x, y), false) >= 0) 
                    {
                        float f_val = feature_flow_map[y * width + x];
                        float w = f_val / 255.0f; 
                        
                        // 防止除以零
                        if (w < 1e-6f) w = 1e-6f; 

                        sum_x += w * static_cast<float>(x);
                        sum_y += w * static_cast<float>(y);
                        sum_w += w;     
                    }
                }
            }

            // 5. 更新质心
            if (sum_w > 0.0f) 
            {
                new_positions[free_idx] = cv::Point2f(sum_x / sum_w, sum_y / sum_w); 
                point_updated[free_idx] = true;
            }
        }

        // 6. 应用新坐标及边界限制 
        for (int i = 0; i < num_free_points; ++i) 
        {
            if (point_updated[i]) {
                // 如果原始点在边界上，只能沿边界移动 
                bool is_on_left = (free_points[i].x == 0.0f);
                bool is_on_right = (free_points[i].x == static_cast<float>(width - 1));
                bool is_on_top = (free_points[i].y == 0.0f);
                bool is_on_bottom = (free_points[i].y == static_cast<float>(height - 1));

                float nx = new_positions[i].x;
                float ny = new_positions[i].y;

                if (is_on_left) nx = 0.0f;
                if (is_on_right) nx = static_cast<float>(width - 1);
                if (is_on_top) ny = 0.0f;
                if (is_on_bottom) ny = static_cast<float>(height - 1);

                // 全局越界保护
                nx = std::min(std::max(nx, 0.0f), static_cast<float>(width - 1));
                ny = std::min(std::max(ny, 0.0f), static_cast<float>(height - 1));

                free_points[i].x = nx;
                free_points[i].y = ny;
            }
        }
    }
    UPDATE_STATUS(error_msg, max_error_len, "Optimization completed successfully.");
    return true;
}

} // extern "C"
} // namespace VertexOpt