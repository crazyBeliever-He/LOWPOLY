#include "image_controller.h"

#include <QFileDialog>
#include <QPainter>
#include "logger.h"
#include "params_util.h"

ImageController::ImageController(ImageModel *model,
                                 ImageWidget *view,
                                 QObject *parent)
    : QObject(parent),          // 用于对话框的父窗口,传递给基类，参与内存管理
    imageModel(model),
    imageWidget(view)
{
    // 1. edge drawing 参数初始化
    edgeDrawing = new EdgeDrawing();
    EDParamsUtil::getDefaultEDParams(edgeDrawing->edParams);

    // 2. douglas peucker 参数初始化
    douglasPeucker = new DouglasPeucker();
    DPParamsUtil::getDefaultDPParams(douglasPeucker->dpParams);

    // n. 实现connect
    // 切换源图
    connect(imageModel, &ImageModel::originImageChanged,
            this, &ImageController::applyAllImageProcess);
    // ImageModel 和 ImageWidget 解耦, 切换源图和其他图片分别执行不同的绘制操作
    connect(imageModel, &ImageModel::originImageChanged,
            imageWidget, &ImageWidget::setOriginImage);
    connect(imageModel, &ImageModel::displayTypeUpdated,
            imageWidget, &ImageWidget::setImage);
}

void ImageController::applyAllImageProcess()
{
    // emit requestUiReset();
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
    QImage originImage = imageModel->getImage(ImageModel::TYPE_ORIGIN);
    edgeDrawing->edgeDrawingInLib(originImage);
    QImage edgeMap = edgeDrawing->drawImage(originImage.width(), originImage.height());
    imageModel->setImageData(ImageModel::TYPE_EDGEDRAWING, edgeMap);

    // int totalPoints = 0;
    // for(int i = 0; i < edgeDrawing->edResults.data.segmentCount; i++)
    // {
    //     totalPoints +=  edgeDrawing->edResults.data.segments[i].count;
    // }
    // qDebug() << QString("points after edge drawing : %1").arg(totalPoints);
}

void ImageController::applyDouglasPeucker()
{
    QImage originImage = imageModel->getImage(ImageModel::TYPE_ORIGIN);
    int w = originImage.width();
    int h = originImage.height();
    douglasPeucker->simplify(edgeDrawing->edResults, w, h);
    QImage pointMap = douglasPeucker->drawPointImage(w, h);
    imageModel->setImageData(ImageModel::TYPE_DOUGLASPOINT, pointMap);
    QImage lineMap = douglasPeucker->drawLineImage(w, h);
    imageModel->setImageData(ImageModel::TYPE_DOUGLASLINE, lineMap);

    // int totalPoints = 0;
    // for(std::vector<QPoint>& i : douglasPeucker->dpResults)
    // {
    //     totalPoints += static_cast<int>(i.size());
    // }
    // qDebug() << QString("points after douglas peucker : %1").arg(totalPoints);
}

void ImageController::setEDParams(const opencved::EDParams& newParams)
{
    if (!EDParamsUtil::compareEDParams(edgeDrawing->edParams, newParams))
    {
        edgeDrawing->edParams = newParams;
        // ED 参数变了，当前进度重置为 None
        currentStage = ProcessStage::None;
        applyAllImageProcess();
    }
}

void ImageController::setDPParams(const DPParams& newParams)
{
    if(newParams.epsilon != douglasPeucker->dpParams.epsilon ||
        newParams.eta != douglasPeucker->dpParams.eta)
    {
        douglasPeucker->dpParams.epsilon = newParams.epsilon;
        douglasPeucker->dpParams.eta = newParams.eta;
        // 如果 DP 参数变了，进度最多只能维持在 EdgeDrawing 完成状态
        if (currentStage > ProcessStage::EdgeDrawingDone)
        {
            currentStage = ProcessStage::EdgeDrawingDone;
            applyAllImageProcess();
        }
    }
}

void ImageController::openImageWithDialog(QWidget *parent)
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
    //emit statusMessage(tr("Loading..."));
    if (imageModel->loadFromFile(path))
    {
        emit statusMessage(tr("Image loaded"));
        LOG_INFO << "Load: " << path;
        return;
    }
    emit errorOccurred(tr("Failed to load image from: ") + path);
}

void ImageController::saveImageWithDialog(QWidget *parent)
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
    //emit statusMessage(tr("Saving..."));
    if(imageModel->saveToFile(path))
    {
        emit statusMessage(tr("Image saved"));
        LOG_INFO << "Save: " << path;
        return;
    }
    emit errorOccurred(tr("Failed to save image to: ") + path);
}

void ImageController::onImageTypeSelected(int intType)
{
    ImageModel::ImageType imageType = ImageModel::TYPE_ORIGIN;
    if (intType >= 0 && intType < ImageModel::NUM_TYPES)
    {
        imageType = static_cast<ImageModel::ImageType>(intType);
        imageModel->setDisplayType(imageType);
    }
}

ImageController::~ImageController()
{
    // 确保 imageWidget 不是 nullptr 且未被其他父控件删除
    if (imageWidget != nullptr && !imageWidget->parent())
    {
        delete imageWidget;  // 如果 imageWidget 没有父控件，手动删除它
    }
    delete imageModel;  // 删除 imageModel
}
