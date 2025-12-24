#ifndef EDPARAMS_UTIL_H
#define EDPARAMS_UTIL_H

#include <cstring>
#include "edge_drawing_lib.h"

class EDParamsUtil {
public:
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
};


#endif // EDPARAMS_UTIL_H
