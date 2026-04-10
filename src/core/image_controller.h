#ifndef IMAGE_CONTROLLER_H
#define IMAGE_CONTROLLER_H

#include <QObject>
#include <memory>
#include <QColor>
#include "algorithm_params.h"

/**
 * @brief 类型前向声明 (Forward Declaration)
 * * [好处 / 目的]:
 * 1. 解开耦合 (Decoupling): Controller 仅持有指针，无需知道类的实现细节。
 * 2. 编译加速 (Compilation Speed):
 * - 修改 ImageModel.h 时，只有 image_controller.cpp 会重编。
 * - 包含此头文件(image_controller.h)的其他文件(如 Widget.h, main.cpp)无需重编。
 * 3. 避免循环引用 (Circular Dependency): 防止 A 包含 B 且 B 包含 A 导致的编译错误。
 * 4. 隐藏实现: 外部模块通过此头文件无法感知底层算法库(如 OpenCV)的存在。
 * * [注意]:
 * - 必须在对应的 .cpp 文件中 #include 完整头文件以访问成员或调用析构。
 * - 对于 std::unique_ptr 成员，必须在 .cpp 中显式定义析构函数 (~ClassName() = default;)。
 */
class ImageModel;
class ImageWidget;
class EdgeDrawing;
class DouglasPeucker;
class SaliencyDetection;
class FeatureFlow;
class VertexOptimization;
class ConstrainedTriangulation;

class ImageWorker;

class ImageController : public QObject
{
    Q_OBJECT

public:
    // enum class ProcessStage {
    //     None,
    //     AutoProcessing, // 正在执行 ED -> DP -> SD -> FF -> VO
    //     ReadyForEdit    // 阶段5完成，可以开始手动精修点，并随时重跑阶段6
    // };
    // Q_ENUM(ProcessStage)
    // ProcessStage currentStage = ProcessStage::None;

    // 防止重入并发引发崩溃
    bool isProcessing = false;

    ImageModel *imageModel;
    ImageWidget *imageWidget;

    std::unique_ptr<EdgeDrawing> edgeDrawing;
    std::unique_ptr<DouglasPeucker> douglasPeucker;
    std::unique_ptr<SaliencyDetection> saliencyDetection;
    std::unique_ptr<FeatureFlow> featureFlow;
    std::unique_ptr<VertexOptimization> vertexOptimization;
    std::unique_ptr<ConstrainedTriangulation> constrainedTriangulation;

public:
    ImageController(ImageModel *model,
                    ImageWidget *view,
                    QObject *parent = nullptr);
    ~ImageController();

    // 统一的数据存取接口
    QVariant getParams(const QString& typeName) const;
    void setParams(const QString& typeName, const QVariant& data);

    void setEDParams(const opencved::EDParams& newParams);
    opencved::EDParams getEDParams() const;
    void setDPParams(const DPParams& newParams);
    DPParams getDPParams() const;   // 因为返回的是值(Value)，编译器需要 algorithm_params.h 里的结构体定义
    void setSDParams(const SDParams& newParams);
    SDParams getSDParams() const;

    void applyEdgeDrawing();
    void applyDouglasPeucker();
    void applySaliencyDetection();
    void applyJumpFloodingAndFeatureFlow();
    void applyVertexOptimization();
    void applyConstrainedTriangulation();
    // 执行所有图像处理方法
    void applyAllImageProcess();

    // 交互增加删除点
    void onAddPointRequested(QPointF imgPos);
    void onDeletePointsRequested(QRectF imgArea);
    void updateAfterInteraction();

    // 交互修改指定三角颜色
    void onTriangleSelected(QPointF imgPos);
    void updateTriangleColor(int triIndex, QColor newColor);
    void clearTriangleSelection();

    // 响应 ImageModel 发出的通知, 执行切换源图后需要的所有操作
    void onOriginImageChanged(const QImage& img);
    // 响应 Widget 发出的通知, 用户申请查看某种结果图
    void onImageTypeSelected(int type);
    void onAutoSize(bool isAuto);
    void onOpenImage(QWidget *parent = nullptr);
    void onSaveImage(QWidget *parent = nullptr);

signals:
    void errorOccurred(const QString &message);
    void statusMessage(const QString &message);
    // 请求重置ui的一些设置
    void requestUiReset();
    // 向外界反馈特定类型的参数应用结果
    void paramsApplyFinished(const QString& typeName, bool success, const QString& message);
    //void displayTypeRequested(ImageModel::ImageType type);

    // 通知 Widget 弹窗并带上当前颜色和索引
    void openColorDialogRequested(int triIndex, QColor currentColor);
    void closeColorDialogRequested();

    // processing时对外抛给UI的进度信号
    void progressUpdated(int percent, const QString &stepName);
    void processFinished();
    // 内部通知 Worker 开始干活的信号
    void requestWorkerStart();

private:
    QThread *workerThread;
    ImageWorker *worker;
};

#endif // IMAGE_CONTROLLER_H
