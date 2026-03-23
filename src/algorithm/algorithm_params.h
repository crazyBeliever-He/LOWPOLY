#ifndef ALGORITHM_PARAMS_H
#define ALGORITHM_PARAMS_H

#include <QMetaType>
#include "edge_drawing_lib.h"

//POD (Plain Old Data) 结构体(不含虚函数、构造函数、容器等)

struct DPParams {
    double epsilon; // Douglas-Peucker 的误差容限
    double eta; // Gai & Wang 论文中 Li 计算的比例因子
};

// RAII 思想封装原始的opencved::EDResults
struct ScopedEDResults {
    opencved::EDResults data = {nullptr, 0};

    ScopedEDResults() = default;

    // 重置方法, 处理从 DLL 接收的新数据
    void reset(opencved::EDResults newData = {nullptr, 0})
    {
        if (data.segments)
        {
            opencved::FreeEDResults(data);
        }
        data = newData;
    }

    // 析构时自动释放
    ~ScopedEDResults() { reset(); }

    // 禁止拷贝, 允许移动 (Move)
    ScopedEDResults(const ScopedEDResults&) = delete;
    ScopedEDResults& operator=(const ScopedEDResults&) = delete;
    ScopedEDResults& operator=(ScopedEDResults&& other) noexcept
    {
        if (this != &other)
        {
            if (data.segments) opencved::FreeEDResults(data);
            data = other.data;
            other.data = {nullptr, 0};
        }
        return *this;
    }
};

struct SDParams{
    int type = 0;                   // 使用的显著性检测类型
    int userN = 0;                  // 用户指定的总采样数 (为0则使用论文公式)
    double lambda = 0.7;            // 前景密度控制参数
    uint8_t threshold = 128;        // 区分前景/背景的灰度阈值
};

// 在编译时声明一个类型，允许该类型与 QVariant 一起使用. 使 QVariant 能够处理该类型.
Q_DECLARE_METATYPE(opencved::EDParams)
Q_DECLARE_METATYPE(DPParams)
Q_DECLARE_METATYPE(SDParams)

// 在运行时将类型注册到 Qt 的元对象系统,允许该类型用于信号槽和跨线程传递.
// 使得类型能够跨线程或信号槽机制传递, 必须在信号槽、跨线程等使用该类型时调用该方法.
// qRegisterMetaType<opencved::EDParams>("EDParams");

#endif // ALGORITHM_PARAMS_H
