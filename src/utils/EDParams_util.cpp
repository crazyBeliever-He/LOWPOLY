#include <QCoreApplication>
#include "EDParams_util.h"

bool EDParamsUtil::validateParams(const opencved::EDParams& params, QString& errorMessage)
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
