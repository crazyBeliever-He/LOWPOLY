#include "image_controller.h"

#include <QFileDialog>
#include <QPainter>
#include "logger.h"
#include "params_util.h"
// 包含所有前向声明过的完整定义
#include "image_model.h"
#include "image_widget.h"
#include "algorithm_params.h"
#include "edge_drawing.h"
#include "douglas_peucker.h"
#include "saliency_detection.h"

ImageController::ImageController(ImageModel *model,
                                 ImageWidget *view,
                                 QObject *parent)
    : QObject(parent),          // 用于对话框的父窗口,传递给基类，参与内存管理
    imageModel(model),
    imageWidget(view)
{
    // 1. edge drawing 参数初始化
    edgeDrawing = std::make_unique<EdgeDrawing>();
    EDParamsUtil::getDefaultEDParams(edgeDrawing->edParams);

    // 2. douglas peucker 参数初始化
    douglasPeucker = std::make_unique<DouglasPeucker>();
    DPParamsUtil::getDefaultDPParams(douglasPeucker->dpParams);

    // 3. saliency detection 参数初始化
    saliencyDetection = std::make_unique<SaliencyDetection>();
    SDParamsUtil::getDefaultSDParams(saliencyDetection->sDParams);

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
    // 重置widget ui
    emit requestUiReset();
    currentStage = ProcessStage::None;
    // TODO: 某些算法的参数也要重置

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
        imageType = static_cast<ImageModel::ImageType>(intType);
        imageModel->setCurrentImageType(imageType);
    }
}

/********************************************************************************/
// apply algorithms
/********************************************************************************/
void ImageController::applyAllImageProcess()
{   // 后续改进: 开启一个工作线程(Worker Thread)执行该函数
    // 阶段 1: Edge Drawing. 如果当前 ED 没做,则执行它
    if (currentStage < ProcessStage::EdgeDrawingDone)
    {
        applyEdgeDrawing();
        currentStage = ProcessStage::EdgeDrawingDone;
    }
    // 阶段 2: Douglas Peucker. 如果当前没做 DP,或者 ED 重做导致 DP 失效
    if (currentStage < ProcessStage::DouglasPeuckerDone)
    {
        applyDouglasPeucker();
        currentStage = ProcessStage::DouglasPeuckerDone;
    }

    applySaliencyDetection();
}

void ImageController::applyEdgeDrawing()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    edgeDrawing->edgeDrawingInLib(originImage);
    QImage edgeMap = edgeDrawing->drawImage(originImage.width(), originImage.height());
    imageModel->setImage(ImageModel::ImageType::EdgeDrawingA, edgeMap);
}

void ImageController::applyDouglasPeucker()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();
    douglasPeucker->simplify(edgeDrawing->edResults, w, h);
    QImage pointMap = douglasPeucker->drawPointImage(w, h);
    imageModel->setImage(ImageModel::ImageType::DouglasPoint, pointMap);
    QImage lineMap = douglasPeucker->drawLineImage(w, h);
    imageModel->setImage(ImageModel::ImageType::DouglasLine, lineMap);
}
void ImageController::applySaliencyDetection()
{
    QImage ogrigin = imageModel->getImage(ImageModel::ImageType::Origin);
    QImage result = saliencyDetection->saliencyDetectionInterface(ogrigin);
    imageModel->setImage(ImageModel::ImageType::SaliencyDetectionA, result);
}
/********************************************************************************/
// get/set params from dialogs
/********************************************************************************/
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
    if (!EDParamsUtil::compareEDParams(edgeDrawing->edParams, newParams))
    {   // ED 参数变了，当前进度重置为 None
        edgeDrawing->edParams = newParams;
        currentStage = ProcessStage::None;
        applyAllImageProcess();
    }
}

DPParams ImageController::getDPParams() const
{
    return douglasPeucker->dpParams;
}
void ImageController::setDPParams(const DPParams& newParams)
{
    if(newParams.epsilon != douglasPeucker->dpParams.epsilon ||
        newParams.eta != douglasPeucker->dpParams.eta)
    {   // 如果 DP 参数变了，进度最多只能维持在 EdgeDrawing 完成状态
        douglasPeucker->dpParams.epsilon = newParams.epsilon;
        douglasPeucker->dpParams.eta = newParams.eta;
        if (currentStage > ProcessStage::EdgeDrawingDone)
        {
            currentStage = ProcessStage::EdgeDrawingDone;
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
