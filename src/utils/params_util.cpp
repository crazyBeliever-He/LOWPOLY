#include "params_util.h"

#include <QCoreApplication>

/********************************************************************************/
// edge drawing params
/********************************************************************************/
bool EDParamsUtil::validateEDParams(const opencved::EDParams& params, QString& errorMessage)
{
    errorMessage = "";
    bool isValid = true;
    // --- 边缘检测相关参数 (5个) ---
    // 1. GradientThresholdValue (梯度阈值)
    // 源码逻辑：if (gradThresh < 1) gradThresh = 1;
    if (params.GradientThresholdValue < 1) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Error: Gradient Threshold must be at least 1.\n");
        isValid = false;
    } else if (params.GradientThresholdValue > 255) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Gradient Threshold value recommend 1 ~ 255.\n");
    }
    // 2. AnchorThresholdValue (锚点阈值)
    // 源码逻辑：if (anchorThresh < 0) anchorThresh = 0;
    if (params.AnchorThresholdValue < 0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Error: Anchor Threshold Value cannot be negative.\n");
        isValid = false;
    } else if (params.AnchorThresholdValue > 255) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Anchor Threshold Value > 255 may result in very few anchors.\n");
    }
    // 3. ScanInterval (扫描间隔)
    // 源码逻辑：用于循环步长，若为0会导致死循环或崩溃
    if (params.ScanInterval <= 0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Error: Scan Interval must be greater than 0.\n");
        isValid = false;
    } else if (params.ScanInterval > 10) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Scan Interval > 10 may result in very sparse edges.\n");
    }
    // 4. MinPathLength (最小路径长度)
    // 源码逻辑：if (params.MinPathLength < 3) params.MinPathLength = 3;
    if (params.MinPathLength < 1) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Error: Min Path Length must be positive.\n");
        isValid = false;
    } else if (params.MinPathLength < 3) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Min Path Length < 3 will be forced to 3 by algorithm.\n");
    } else if (params.MinPathLength > 1000) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Min Path Length > 1000 may result in very sparse edges.\n");
    }
    // 5. Sigma (高斯平滑系数)
    if (params.Sigma < 0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Error: Sigma cannot be negative.\n");
        isValid = false;
    } else if (params.Sigma > 0 && params.Sigma < 0.5) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Sigma < 0.5 provides almost no smoothing effect.\n");
    } else if (params.Sigma > 10.0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Sigma > 10.0 might blur the image excessively.\n");
    }

    // --- 几何检测相关参数 (4个) ---
    // 6. MinLineLength (最小直线长度)
    // 源码逻辑：-1为自动计算，若 > 0 则强制至少为 9
    if (params.MinLineLength < -1 || params.MinLineLength == 0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Error: Min Line Length must be -1 (Auto) or a positive value.\n");
        isValid = false;
    } else if (params.MinLineLength > 0 && params.MinLineLength < 9) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Min Line Length < 9 will be forced to 9 by algorithm.\n");
    }
    // 7. MaxDistanceBetweenTwoLines (线段合并最大距离)
    if (params.MaxDistanceBetweenTwoLines < 0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Error: Max Distance cannot be negative.\n");
        isValid = false;
    } else if (params.MaxDistanceBetweenTwoLines > 20.0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Max Distance > 20.0 may incorrectly merge separate lines.\n");
    }
    // 8. LineFitErrorThreshold (直线拟合误差)
    if (params.LineFitErrorThreshold < 0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Error: Line Fit Error Threshold cannot be negative.\n");
        isValid = false;
    } else if (params.LineFitErrorThreshold > 10.0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Line Fit Error > 10.0 may cause inaccurate line detection.\n");
    }
    // 9. MaxErrorThreshold (圆/椭圆拟合误差)
    if (params.MaxErrorThreshold < 0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Error: Max Error Threshold cannot be negative.\n");
        isValid = false;
    } else if (params.MaxErrorThreshold > 10.0) {
        errorMessage += QCoreApplication::translate("EDParamsUtil",
                                                    "Warning: Max Error > 10.0 may result in poor ellipse fitting.\n");
    }
    return isValid;
}

void EDParamsUtil::setEDParams(opencved::EDParams &params,
                        bool PFmode,
                        int EdgeDetectionOperator,
                        int GradientThresholdValue,
                        int AnchorThresholdValue,
                        int ScanInterval,
                        int MinPathLength,
                        float Sigma,
                        bool SumFlag,
                        bool NFAValidation,
                        int MinLineLength,
                        double MaxDistanceBetweenTwoLines,
                        double LineFitErrorThreshold,
                        double MaxErrorThreshold) {
    params.PFmode = PFmode;
    params.EdgeDetectionOperator = EdgeDetectionOperator;
    params.GradientThresholdValue = GradientThresholdValue;
    params.AnchorThresholdValue = AnchorThresholdValue;
    params.ScanInterval = ScanInterval;
    params.MinPathLength = MinPathLength;
    params.Sigma = Sigma;
    params.SumFlag = SumFlag;
    params.NFAValidation = NFAValidation;
    params.MinLineLength = MinLineLength;
    params.MaxDistanceBetweenTwoLines = MaxDistanceBetweenTwoLines;
    params.LineFitErrorThreshold = LineFitErrorThreshold;
    params.MaxErrorThreshold = MaxErrorThreshold;
}

bool EDParamsUtil::compareEDParams(const opencved::EDParams &params1, const opencved::EDParams &params2)
{
    return params1.PFmode == params2.PFmode &&
           params1.EdgeDetectionOperator == params2.EdgeDetectionOperator &&
           params1.GradientThresholdValue == params2.GradientThresholdValue &&
           params1.AnchorThresholdValue == params2.AnchorThresholdValue &&
           params1.ScanInterval == params2.ScanInterval &&
           params1.MinPathLength == params2.MinPathLength &&
           params1.Sigma == params2.Sigma &&
           params1.SumFlag == params2.SumFlag &&
           params1.NFAValidation == params2.NFAValidation &&
           params1.MinLineLength == params2.MinLineLength &&
           params_util::isEqual(params1.MaxDistanceBetweenTwoLines, params2.MaxDistanceBetweenTwoLines) &&
           params_util::isEqual(params1.LineFitErrorThreshold, params2.LineFitErrorThreshold) &&
           params_util::isEqual(params1.MaxErrorThreshold, params2.MaxErrorThreshold);
}

void EDParamsUtil::getDefaultEDParams(opencved::EDParams &p)
{
    p.PFmode = false;
    p.EdgeDetectionOperator = 1;
    p.GradientThresholdValue = 20;
    p.AnchorThresholdValue = 0;
    p.ScanInterval = 1;
    p.MinPathLength = 10;
    p.Sigma = 1.0f;
    p.SumFlag = true;
    p.NFAValidation = true;
    p.MinLineLength = 10;
    p.MaxDistanceBetweenTwoLines = 6.0;
    p.LineFitErrorThreshold = 1.0;
    p.MaxErrorThreshold = 1.3;
}
/********************************************************************************/
// douglas peucker params
/********************************************************************************/
bool DPParamsUtil::validateDPParams(const DPParams &params, QString &errorMessage)
{
    errorMessage = "";
    bool isValid = true;
    // 1. Epsilon (容差阈值)
    // 决定点到直线的距离超过多少时保留该点. 像素坐标系下通常 > 0. 推荐值范围：0.1 ~ 10.0 过小则没简化.过大则丢失形状.
    if (params.epsilon < 0.0) {
        errorMessage += QCoreApplication::translate("DPParamsUtil",
                                                    "Error: Epsilon (tolerance) cannot be negative.\n");
        isValid = false;
    } else if (params.epsilon < 0.1) {
        errorMessage += QCoreApplication::translate("DPParamsUtil",
                                                    "Warning: Epsilon is very small; simplification might be ineffective.\n");
    } else if (params.epsilon > 20.0) {
        errorMessage += QCoreApplication::translate("DPParamsUtil",
                                                    "Warning: Epsilon is very large; edge features may be severely distorted.\n");
    }

    // 2. Eta (长度比例系数)
    // 论文逻辑: Li = eta * (W + H), 论文推荐值为 0.02. eta 过大, 长边不会被打断, Low Poly 感减弱; 如果过小, 点会过于密集.
    if (params.eta <= 0.0) {
        errorMessage += QCoreApplication::translate("DPParamsUtil",
                                                    "Error: Eta (length factor) must be greater than 0.\n");
        isValid = false;
    } else if (params.eta > 0.5) {
        // 如果 eta > 0.5, 意味着采样间隔超过了图像尺寸的一半，显然不合理
        errorMessage += QCoreApplication::translate("DPParamsUtil",
                                                    "Error: Eta is too large; maximum segment length exceeds image proportions.\n");
        isValid = false;
    } else if (params.eta < 0.005) {
        errorMessage += QCoreApplication::translate("DPParamsUtil",
                                                    "Warning: Eta is very small; may generate excessive constrained points.\n");
    }
    return isValid;
}

void DPParamsUtil::setDPParams(DPParams &params, double epsilon, double eta)
{
    params.epsilon = epsilon;
    params.eta = eta;
}

bool DPParamsUtil::compareDPParams(const DPParams &params1, const DPParams &params2)
{
    return params_util::isEqual(params1.epsilon, params2.epsilon) &&
           params_util::isEqual(params1.eta, params2.eta);
}

void DPParamsUtil::getDefaultDPParams(DPParams &p)
{
    p.epsilon = 1.5;
    p.eta = 0.02;
}
/********************************************************************************/
// saliency detection params
/********************************************************************************/
bool SDParamsUtil::validateSDParams(const SDParams &params, QString &errorMessage)
{
    errorMessage = "";
    bool isValid = true;

    // QComboBox 的索引应该是
    if (params.type < 0 || params.type > 2) {
        errorMessage += QCoreApplication::translate("SDParamsUtil",
                                                    "Error: Invalid Saliency Detection Type.\n");
        isValid = false;
    }

    return isValid;
}

void SDParamsUtil::setSDParams(SDParams &params,
                               int type,
                               int userN,
                               double lambda,
                               uint8_t threshold)
{
    params.type = type;
    params.userN = userN;
    params.lambda = lambda;
    params.threshold = threshold;
}

bool SDParamsUtil::compareSDParams(const SDParams &params1, const SDParams &params2)
{
    return params1.type == params2.type &&
        params_util::isEqual(params1.lambda, params2.lambda) &&
        params1.threshold == params2.threshold &&
        params1.userN == params2.userN;
}

void SDParamsUtil::getDefaultSDParams(SDParams &p)
{
    p.type = 0;         // 默认选中第一个选项 "type1"
    p.lambda = 0.7;
    p.threshold = 128;
    p.userN = 0;        // 用户指定的总采样数 (为0则使用论文公式)
}
