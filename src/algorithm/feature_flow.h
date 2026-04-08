#ifndef FEATURE_FLOW_H
#define FEATURE_FLOW_H

#include <vector>
#include <QPoint>
#include <QImage>

#include "JumpFlooding_FeatureFlow.h"
#include "algorithm_params.h"

/********************************************************************************/
// Rong G, Tan T S.
// Jump flooding in GPU with applications to Voronoi diagram and distance transform[C]
// Proceedings of the 2006 symposium on Interactive 3D graphics and games. 2006: 109-116.
/********************************************************************************/

/********************************************************************************/
// qt6+cmake+msvc已经可以很好的支持cuda，如果采用上述配置，完全可以在qt creator中进行cuda编程。
// 将cuda代码编译成dll进行隐式链接，可以独立构建、调试CUDA代码，可以节省时间，平台兼容性也会更好。
/********************************************************************************/

// 注: JFA是近似计算, 可能存在部分不正确值
// TODO: 1. CPU版本的JFA     2. CUDA版本的完备计算(朴素的遍历所有seeds)
// TODO: 实现JFA的变体如JFA+1等，使其效果更高

//
class FeatureFlow
{
public:
    std::vector<JFAFF::JFAPoint> jFCUDAResults;
    std::vector<float> fFCUDAResults;
    // CUDA版本JFA, 考虑将seeds参数更换为douglas peucker的结果(点图但是连线版本)
    bool jumpFloodingCUDAApi(const ScopedEDResults& edResults,
                             int imgWidth,
                             int imgHeight);
    bool jumpFloodingCPU(const ScopedEDResults& edResults,
                         int imgWidth,
                         int imgHeight);

    // nearestSeedMap 即 jFCUDAResult. m 为控制流场通道宽度的参数, 按论文建议传入 Li / 2.0f
    bool featureFlowCUDAApi(const std::vector<JFAFF::JFAPoint>& nearestSeedMap,
                            int imgWidth,
                            int imgHeight,
                            float m);
    QImage drawJumpFloodingMap(int width, int height, const std::vector<JFAFF::JFAPoint>& jfaOutput, const ScopedEDResults& edResults);
    QImage drawFeatureFlowMap(int width, int height, const std::vector<float>& ffOutput);

public:
    FeatureFlow(){};
};

#endif // FEATURE_FLOW_H
