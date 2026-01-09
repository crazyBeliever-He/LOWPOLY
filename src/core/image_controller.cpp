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

    // n. 实现connect
    // 切换源图
    connect(imageModel, &ImageModel::originImageChanged,
            this, &ImageController::onOriginImageChanged);
    // ImageModel 和 ImageWidget 解耦
    connect(imageModel, &ImageModel::displayImageUpdated,
            imageWidget, &ImageWidget::updateImageKeepView);
}

void ImageController::onOriginImageChanged(const QImage& img)
{
    // 重置widget ui
    emit requestUiReset();
    currentStage = ProcessStage::None;
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
}

void ImageController::applyEdgeDrawing()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    edgeDrawing->edgeDrawingInLib(originImage);
    QImage edgeMap = edgeDrawing->drawImage(originImage.width(), originImage.height());
    imageModel->setImage(ImageModel::ImageType::EdgeDrawing, edgeMap);
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

opencved::EDParams ImageController::getEDParams() const
{
    return edgeDrawing->edParams;
}

void ImageController::setEDParams(const opencved::EDParams& newParams)
{
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

ImageController::~ImageController() = default;
