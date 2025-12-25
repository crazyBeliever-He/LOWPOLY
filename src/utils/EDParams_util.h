#ifndef EDPARAMS_UTIL_H
#define EDPARAMS_UTIL_H

#include <QString>
#include "edge_drawing_lib.h"

class EDParamsUtil {
public:
    /**
     * @brief 校验参数合法性
     * @param params 待校验的结构体
     * @param errorMessage 如果校验失败，存储错误描述信息
     * @return 符合规范返回 true，否则返回 false
     */
    static bool validateParams(const opencved::EDParams& params, QString& errorMessage);


    static void setEDParams(opencved::EDParams &params,
                            bool PFmode = false,
                            int EdgeDetectionOperator = 1,
                            int GradientThresholdValue = 20,
                            int AnchorThresholdValue = 0,
                            int ScanInterval = 1,
                            int MinPathLength = 10,
                            float Sigma = 1.0f,
                            bool SumFlag = true,
                            bool NFAValidation = true,
                            int MinLineLength = 10,
                            double MaxDistanceBetweenTwoLines = 6.0,
                            double LineFitErrorThreshold = 1.0,
                            double MaxErrorThreshold = 1.3) {
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
    static bool compareEDParams(const opencved::EDParams &params1, const opencved::EDParams &params2)
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
               params1.MaxDistanceBetweenTwoLines == params2.MaxDistanceBetweenTwoLines &&
               params1.LineFitErrorThreshold == params2.LineFitErrorThreshold &&
               params1.MaxErrorThreshold == params2.MaxErrorThreshold;
    }

    static opencved::EDParams getDefaultParams()
    {
        opencved::EDParams p;
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
        return p;
    }
};


#endif // EDPARAMS_UTIL_H
