#ifndef PARAMS_UTIL_H
#define PARAMS_UTIL_H

#include <QString>
#include "algorithm_params.h"
namespace params_util
{
constexpr double DEFAULT_EPSILON = 1e-9;

inline bool isEqual(double a, double b, double epsilon = DEFAULT_EPSILON) {
    return std::fabs(a - b) < epsilon;
}

}
//默认参数只能在函数声明中指定，不能在函数定义中重复
class EDParamsUtil
{
public:
    //校验参数合法性. errorMessage 如果校验失败,存储错误描述信息
    static bool validateEDParams(const opencved::EDParams &params, QString& errorMessage);
    static void setEDParams(opencved::EDParams& params, bool PFmode = false,
                            int EdgeDetectionOperator = 1, int GradientThresholdValue = 20,
                            int AnchorThresholdValue = 0, int ScanInterval = 1,
                            int MinPathLength = 10, float Sigma = 1.0f,
                            bool SumFlag = true, bool NFAValidation = true,
                            int MinLineLength = 10, double MaxDistanceBetweenTwoLines = 6.0,
                            double LineFitErrorThreshold = 1.0,double MaxErrorThreshold = 1.3);
    static bool compareEDParams(const opencved::EDParams &params1, const opencved::EDParams &params2);
    static void getDefaultEDParams(opencved::EDParams &p);
};

class DPParamsUtil
{
public:
    //校验参数合法性. errorMessage 如果校验失败,存储错误描述信息
    static bool validateDPParams(const DPParams &params, QString &errorMessage);
    static void setDPParams(DPParams &params, double epsilon = 1.5, double eta = 0.02);
    static bool compareDPParams(const DPParams &params1, const DPParams &params2);
    static void getDefaultDPParams(DPParams &p);

};
class SDParamsUtil
{
public:
    static bool validateSDParams(const SDParams &params, QString &errorMessage);
    static void setSDParams(SDParams &params,
                            int type = 0,
                            int userN = 0,
                            double lambda = 0.7,
                            uint8_t threshold = 128);
    static bool compareSDParams(const SDParams &params1, const SDParams &params2);
    static void getDefaultSDParams(SDParams &p);

};

#endif // PARAMS_UTIL_H
