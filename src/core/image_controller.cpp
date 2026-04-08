#include "image_controller.h"

#include <QFileDialog>
#include <QPainter>
#include "logger.h"
#include "param_util.h"
// 包含所有前向声明过的完整定义
#include "image_model.h"
#include "image_widget.h"
#include "algorithm_params.h"
#include "edge_drawing.h"
#include "douglas_peucker.h"
#include "saliency_detection.h"
#include "feature_flow.h"
#include "vertex_optimization.h"
#include "constrained_triangulation.h"

ImageController::ImageController(ImageModel *model,
                                 ImageWidget *view,
                                 QObject *parent)
    : QObject(parent),          // 用于对话框的父窗口,传递给基类，参与内存管理
    imageModel(model),
    imageWidget(view)
{
    // 1. edge drawing 参数初始化
    edgeDrawing = std::make_unique<EdgeDrawing>();
    EDParamUtil::getDefaultEDParams(edgeDrawing->edParams);

    // 2. douglas peucker 参数初始化
    douglasPeucker = std::make_unique<DouglasPeucker>();
    DPParamsUtil::getDefaultDPParams(douglasPeucker->dpParams);

    // 3. saliency detection 参数初始化
    saliencyDetection = std::make_unique<SaliencyDetection>();
    SDParamsUtil::getDefaultSDParams(saliencyDetection->sDParams);

    // 4. jump flooding 和 feature flow 参数初始化
    featureFlow = std::make_unique<FeatureFlow>();

    // 5. vertex optimization 初始化
    vertexOptimization = std::make_unique<VertexOptimization>();

    // 6. constrained triangulation 初始化
    constrainedTriangulation = std::make_unique<ConstrainedTriangulation>();

    // n. 实现connect
    // 切换源图
    connect(imageModel, &ImageModel::originImageChanged,
            this, &ImageController::onOriginImageChanged);
    // ImageModel 和 ImageWidget 解耦
    connect(imageModel, &ImageModel::displayImageUpdated,
            imageWidget, &ImageWidget::updateImageKeepView);
}

ImageController::~ImageController() = default;

void ImageController::onOriginImageChanged(const QImage& img)
{
    // TODO: 根据传入的图片修改算法的参数, 某些算法的参数也要重置


    // 重置widget ui
    emit requestUiReset();
    //currentStage = ProcessStage::None;

    // 执行算法
    applyAllImageProcess();
    // 通知 ImageWidget 展示源图
    imageWidget->updateImageResetView(img);
}

void ImageController::onImageTypeSelected(int intType)
{
    ImageModel::ImageType imageType = ImageModel::ImageType::Origin;
    if (intType >= 0 && intType < imageModel->typeToIdx(ImageModel::ImageType::Count))
    {
        //qDebug() << intType;
        imageType = static_cast<ImageModel::ImageType>(intType);
        imageModel->setCurrentImageType(imageType);
    }
}

/********************************************************************************/
// apply algorithms
/********************************************************************************/

// apply alogrithms
void ImageController::applyAllImageProcess()
{
    // TODO: 开启一或两个工作线程(Worker Thread)执行该函数
    // TODO: 根据阶段的不同(修改某一步的参数导致的), 使用switch判断从哪一步开始
    // TODO: 增加交互功能

    // 阶段 1: Edge Drawing. 如果当前 ED 没做,则执行它
    //if (currentStage < ProcessStage::EdgeDrawingDone)
    {
    applyEdgeDrawing();
        //currentStage = ProcessStage::EdgeDrawingDone;
    }

    // 阶段 2: Douglas Peucker. 如果当前没做 DP,或者 ED 重做导致 DP 失效
    //if (currentStage < ProcessStage::DouglasPeuckerDone)
    {
    applyDouglasPeucker();
        //currentStage = ProcessStage::DouglasPeuckerDone;
    }

    // 阶段 3: 只有在边缘和约束点都计算完毕后，才能执行显著性检测与采样
    applySaliencyDetection();

    // 阶段 4: 特征流场
    applyJumpFloodingAndFeatureFlow();

    // 阶段 5: 顶点优化
    applyVertexOptimization();

    // 阶段 6: 三角剖分和颜色处理
    applyConstrainedTriangulation();

}

void ImageController::applyEdgeDrawing()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);

    bool success = edgeDrawing->edgeDrawingInLib(originImage);
    if(!success)
    {
        LOG_ERROR << "edge drawing form opencv failed";
    }
    QImage edgeMap = edgeDrawing->drawImage(originImage.width(), originImage.height());

    imageModel->setImage(ImageModel::ImageType::EdgeDrawing, edgeMap);

    QPair<int, int> edInfo = edgeDrawing->getEDPointsNumber();
    qDebug()<<"Edge Drawing Info: segmentCount is " <<edInfo.first<<", totalPoints is " <<edInfo.second;
}

void ImageController::applyDouglasPeucker()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();

    bool success = douglasPeucker->simplify(edgeDrawing->edResults, w, h);
    if(!success)
    {
        LOG_ERROR << "douglas peucker simplfy failed";
    }
    QImage pointMap = douglasPeucker->drawPointImage(w, h);
    QImage lineMap = douglasPeucker->drawLineImage(w, h);

    imageModel->setImage(ImageModel::ImageType::DouglasPoint, pointMap);
    imageModel->setImage(ImageModel::ImageType::DouglasLine, lineMap);

    int dpInfo = douglasPeucker->getDPPointsNumber();
    qDebug()<<"Douglas Peucker Info: totalPoints is " <<dpInfo;
}

void ImageController::applySaliencyDetection()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();

    QImage result = saliencyDetection->saliencyDetectionInterface(originImage);
    if(result.isNull())
    {
        LOG_ERROR << "saliency detection failed";
    }
    imageModel->setImage(ImageModel::ImageType::SaliencyDetection, result);

    // TODO: douglas peucker 算法生成的约束点较多, 一般大于默认的约束点总数, 要优化
    int sdInfo =saliencyDetection->generateNonUniformSamples(w, h, douglasPeucker->getDPPointsNumber(), douglasPeucker->dpParams.eta);

    qDebug()<<"saliency detection Info: samples is " <<sdInfo;
}

void ImageController::applyJumpFloodingAndFeatureFlow()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();
    bool successJFA =  featureFlow->jumpFloodingCUDAApi(edgeDrawing->edResults,w,h);
    if(!successJFA)
    {
        // TODO: 调用非cuda版本
        LOG_ERROR << " jfa and ff fail";
    }
    QImage resultJFA = featureFlow->drawJumpFloodingMap(w,h,featureFlow->jFCUDAResults,edgeDrawing->edResults);
    imageModel->setImage(ImageModel::ImageType::JumpFlooding, resultJFA);

    float Li = douglasPeucker->dpParams.eta*(w+h);
    bool successFF = featureFlow->featureFlowCUDAApi(featureFlow->jFCUDAResults,w,h,Li);
    if(!successFF)
    {
        // TODO: 调用非cuda版本
        LOG_ERROR << " jfa and ff fail";
    }
    QImage resultFF = featureFlow->drawFeatureFlowMap(w,h,featureFlow->fFCUDAResults);
    imageModel->setImage(ImageModel::ImageType::FeatureFlow, resultFF);
}

void ImageController::applyVertexOptimization()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();
    bool success = vertexOptimization->vertexOptimizationApi(w, h,
                                                             douglasPeucker->getDPPointsNumber(),
                                                             saliencyDetection->sampledPoints,
                                                             douglasPeucker->dpResults,
                                                             douglasPeucker->cornerPoints,
                                                             featureFlow->fFCUDAResults);
    if(!success)
    {
        LOG_ERROR << " vertex optimization fail";
        return;
    }
    QImage result = vertexOptimization->drawOptimizedVertices(w, h,
                                                              douglasPeucker->dpResults,
                                                              douglasPeucker->cornerPoints);
    imageModel->setImage(ImageModel::ImageType::VertexOptimization, result);
}

void ImageController::applyConstrainedTriangulation()
{

    bool success = constrainedTriangulation->generateLowPolyMesh(vertexOptimization->optimizedPoints,
                                                  douglasPeucker->dpResults,
                                                  douglasPeucker->cornerPoints);
    if(!success)
    {
        LOG_ERROR << " constrained triangulation fail";
    }
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();
    QImage result = constrainedTriangulation->drawTriangulationWireframe(w, h);
    imageModel->setImage(ImageModel::ImageType::TriangulationWireframe, result);
    QImage result2 = constrainedTriangulation->drawLowPolyResult(originImage);
    imageModel->setImage(ImageModel::ImageType::LowPoly, result2);
}


/********************************************************************************/
// get/set params from dialogs
/********************************************************************************/

// get params uniform inerface
QVariant ImageController::getParams(const QString& typeName) const
{
    // 在这里维护类型映射    comtroller -> dialogs
    if (typeName == QMetaType::fromType<opencved::EDParams>().name())
        return QVariant::fromValue(edgeDrawing->edParams);
    else if (typeName == QMetaType::fromType<DPParams>().name())
        return QVariant::fromValue(douglasPeucker->dpParams);
    else if (typeName == QMetaType::fromType<SDParams>().name())
        return QVariant::fromValue(saliencyDetection->sDParams);
    return QVariant();
}

// set params uniform inerface
void ImageController::setParams(const QString& typeName, const QVariant& data)
{
    // 在这里维护类型映射    dialogs -> controller
    bool success = true;
    QString feedbackMsg = tr("Success");

    try {
        if (typeName == QMetaType::fromType<opencved::EDParams>().name()) {
            setEDParams(data.value<opencved::EDParams>());
        }
        else if (typeName == QMetaType::fromType<DPParams>().name()) {
            setDPParams(data.value<DPParams>());
        }
        else if(typeName == QMetaType::fromType<SDParams>().name()){
            setSDParams(data.value<SDParams>());
        }
        else {
            success = false;
            feedbackMsg = tr("Unknown parameter type");
        }
    } catch (...) {
        // 如果底层算法抛出异常，这里可以捕获
        success = false;
        feedbackMsg = tr("Algorithm execution failed");
    }

    //如果未来 setEDParams 内部把任务丢给了工作线程, 可以等工作线程的 finished 信号触发后, 再发出 paramsApplyFinished 信号
    // 无论成功还是失败，都把结果发射出去
    emit paramsApplyFinished(typeName, success, feedbackMsg);
}

opencved::EDParams ImageController::getEDParams() const
{
    return edgeDrawing->edParams;
}
void ImageController::setEDParams(const opencved::EDParams& newParams){
    if (!EDParamUtil::compareEDParam(edgeDrawing->edParams, newParams))
    {
        edgeDrawing->edParams = newParams;
        // ED 参数变了，当前进度重置为 None
        //currentStage = ProcessStage::None;
        applyAllImageProcess();
    }
}

DPParams ImageController::getDPParams() const
{
    return douglasPeucker->dpParams;
}
void ImageController::setDPParams(const DPParams& newParams)
{
    if(!DPParamsUtil::compareDPParams(douglasPeucker->dpParams, newParams))
    {
        douglasPeucker->dpParams = newParams;
        // 如果 DP 参数变了，进度最多只能维持在 EdgeDrawing 完成状态
        //if (currentStage > ProcessStage::EdgeDrawingDone)
        {
            //currentStage = ProcessStage::EdgeDrawingDone;
            applyAllImageProcess();
        }
    }
}

SDParams ImageController::getSDParams() const
{
    return saliencyDetection->sDParams;
}
void ImageController::setSDParams(const SDParams& newParams)
{
    if (!SDParamsUtil::compareSDParams(saliencyDetection->sDParams, newParams))
    {
        saliencyDetection->sDParams = newParams;
        // 显著性检测独立于 edge drawing 和 douglas peucker, 直接重新应用即可
        applySaliencyDetection();
    }
}

void ImageController::onAutoSize(bool isAuto)
{
    imageWidget->autoSize = isAuto;
}

/********************************************************************************/
// open/save image
/********************************************************************************/

// open&load image from sotrage
void ImageController::onOpenImage(QWidget *parent)
{
    QString path = QFileDialog::getOpenFileName(parent,
                                                tr("Open"),
                                                "C:/Users/h7286/Desktop/image",
                                                "Images (*.jpg *.jpeg *.png *.bmp *.tif *.tiff)");
    if (path.isEmpty())
    {
        emit statusMessage(tr("Open canceled"));
        return;
    }
    if (imageModel->loadImage(path))
    {
        emit statusMessage(tr("Image loaded"));
        LOG_INFO << "Load: " << path;
        return;
    }
    emit errorOccurred(tr("Failed to load image from: ") + path);
}

// save image to storage
void ImageController::onSaveImage(QWidget *parent)
{
    if (imageModel->getcurrentImage().isNull())
    {
        emit errorOccurred(tr("No image to save"));
        return;
    }
    QString path = QFileDialog::getSaveFileName(parent,
                                                tr("Save"),
                                                "",
                                                "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp);;TIFF(*.tiff *.tif)");
    if (path.isEmpty())
    {
        emit statusMessage(tr("Save canceled"));
        return;
    }
    if(imageModel->saveImage(path))
    {
        emit statusMessage(tr("Image saved"));
        LOG_INFO << "Save: " << path;
        return;
    }
    emit errorOccurred(tr("Failed to save image to: ") + path);
}
