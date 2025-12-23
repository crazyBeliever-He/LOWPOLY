#include "image_controller.h"
#include <QFileDialog>
#include "logger.h"
#include "opencvEDLib.h"

ImageController::ImageController(ImageModel *model,
                                 ImageWidget *view,
                                 QObject *parent)
    : QObject(parent),          // 用于对话框的父窗口,传递给基类，参与内存管理
    imageModel(model),
    imageWidget(view)
{
    grayEdgeDrawing = new GrayEdgeDrawing();
    connect(imageModel, &ImageModel::originImageChanged,
            this, &ImageController::onApplyAllImageProcess);

    connect(this, &ImageController::displayTypeRequested,
            imageModel, &ImageModel::setDisplayType);
    connect(imageModel, &ImageModel::displayTypeUpdated,
            imageWidget, &ImageWidget::setImage);
}

void ImageController::openImageWithDialog(QWidget *parent)
{
    QString path = QFileDialog::getOpenFileName(parent,
                                                tr("Open"),
                                                "C:/Users/h7286/Desktop/image",
                                                "Images (*.jpg *.jpeg *.png *.bmp *.tif *.tiff)");

    if (path.isEmpty()){
        emit statusMessage(tr("Open canceled"));
        return;
    }

    emit statusMessage(tr("Loading..."));

    if (imageModel->loadFromFile(path)) {
        emit statusMessage(tr("Image loaded"));
        LOG_INFO << "Load: " << path;
        return;
    }

    emit errorOccurred(tr("Failed to load image from: ") + path);

}

void ImageController::saveImageWithDialog(QWidget *parent)
{
    if (imageModel->getcurrentImage().isNull()) {
        emit errorOccurred(tr("No image to save"));
        return;
    }

    QString path = QFileDialog::getSaveFileName(parent,
                                                tr("Save"),
                                                "",
                                                "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp);;TIFF(*.tiff *.tif)");

    if (path.isEmpty()){
        emit statusMessage(tr("Save canceled"));
        return;
    }

    emit statusMessage(tr("Saving..."));

    if(imageModel->saveToFile(path)){
        emit statusMessage(tr("Image saved"));
        LOG_INFO << "Save: " << path;
        return;
    }

    emit errorOccurred(tr("Failed to save image to: ") + path);
}

void ImageController::onImageTypeSelected(int intType)
{
    ImageModel::ImageType imageType;
    if (intType >= 0 && intType < ImageModel::NUM_TYPES) {
        imageType = static_cast<ImageModel::ImageType>(intType);
    } else{
        LOG_ERROR << "Undefine image type.";
        return;
    }
    emit displayTypeRequested(imageType);
}

void ImageController::onApplyAllImageProcess()
{
    // 1. 准备图像格式, Edge Drawing 支持灰度图(CV_8UC1)或彩色图 (CV_8UC3)
    QImage originalImage = imageModel->getcurrentImage();
    QImage workImage;
    if (originalImage.format() == QImage::Format_Grayscale8) {
        workImage = originalImage.copy(); // 已经是灰度 8bit，直接用
    }else{
        // OpenCV 默认彩色处理通常基于BGR. 或用函数QImage::rgbSwapped();
        workImage = originalImage.convertToFormat(QImage::Format_BGR888);
    }

    // 2. 配置参数
    EDParams p;
    p.PFmode = false;
    p.EdgeDetectionOperator = 1; // SOBEL
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

    // 3. 调用 DLL 函数获取边缘链
    EDResults results = RunEdgeDrawingFull(
        workImage.bits(),
        workImage.width(),
        workImage.height(),
        workImage.bytesPerLine(),
        true, // isColor = true
        p
        );

    // 4. 将结果绘制到一张新的二值图中（或者直接显示边缘）
    // 创建一个全黑的图像用于绘制边缘像素
    QImage edgeMap(workImage.width(), workImage.height(), QImage::Format_Grayscale8);
    edgeMap.fill(Qt::black);
    // 遍历所有边缘段并画点
    for (int i = 0; i < results.segmentCount; ++i) {
        EDEdgeSegment& seg = results.segments[i];
        for (int j = 0; j < seg.count; ++j) {
            EDPoint& pt = seg.points[j];
            // 确保坐标不越界
            if (pt.x >= 0 && pt.x < edgeMap.width() && pt.y >= 0 && pt.y < edgeMap.height()) {
                edgeMap.setPixel(pt.x, pt.y, qRgb(255, 255, 255));
            }
        }
    }

    // 5. !!! 非常重要：释放 DLL 内部申请的内存 !!!
    FreeEDResults(results);

    // 6. 更新模型
    imageModel->setImage(ImageModel::TYPE_GRAY, edgeMap);
    emit statusMessage(tr("Edge Drawing segments detected: %1").arg(results.segmentCount));
}

QImage ImageController::processImageWithED(const QImage &sourceImage)
{
    //return grayEdgeDrawing->getEDImageGray16(sourceImage);
    return grayEdgeDrawing->getEDImageFloat(sourceImage);
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
