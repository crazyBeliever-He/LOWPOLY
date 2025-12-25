#include "image_controller.h"
#include <QFileDialog>
#include "logger.h"
#include "EDParams_util.h"

ImageController::ImageController(ImageModel *model,
                                 ImageWidget *view,
                                 QObject *parent)
    : QObject(parent),          // 用于对话框的父窗口,传递给基类，参与内存管理
    imageModel(model),
    imageWidget(view)
{
    //grayEdgeDrawing = new GrayEdgeDrawing();
    // 初始化自己持有的 edge drawing 函数的参数
    std::memset(&edParams, 0, sizeof(edParams));
    setEdParams();
    // 切换源图
    connect(imageModel, &ImageModel::originImageChanged,
            this, &ImageController::applyAllImageProcess);

    // connect(this, &ImageController::displayTypeRequested,
    //         imageModel, &ImageModel::setDisplayType);
    // ImageModel 和 ImageWidget 解耦
    connect(imageModel, &ImageModel::displayTypeUpdated,
            imageWidget, &ImageWidget::setImage);
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
    if (imageModel->loadFromFile(path)) {
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

void ImageController::applyAllImageProcess()
{
    emit requestUiReset();
    edgeDrawing();
}

void ImageController::setEdParams(bool PFmode,
                                  int EdgeDetectionOperator,
                                  int GradientThresholdValue,
                                  int AnchorThresholdValue,
                                  int ScanInterval,
                                  int MinPathLength,
                                  float Sigma,
                                  bool SumFlag,
                                  bool NFAValidation,
                                  int MinLineLength,
                                  double MaxDistanceBetweenTwoLines,
                                  double LineFitErrorThreshold,
                                  double MaxErrorThreshold)
{
    opencved::EDParams newParams;
    EDParamsUtil::setEDParams(newParams,
                              PFmode, EdgeDetectionOperator, GradientThresholdValue,
                              AnchorThresholdValue, ScanInterval, MinPathLength,
                              Sigma, SumFlag,
                              NFAValidation,
                              MinLineLength, MaxDistanceBetweenTwoLines,
                              LineFitErrorThreshold, MaxErrorThreshold);
    // 如果参数修改就重新处理
    if (!EDParamsUtil::compareEDParams(this->edParams, newParams))
    {
        this->edParams = newParams;
        applyAllImageProcess();
    }
}

void ImageController::setEDParams(const opencved::EDParams& newParams)
{
    if (!EDParamsUtil::compareEDParams(this->edParams, newParams))
    {
        this->edParams = newParams;
        applyAllImageProcess();
    }
}

void ImageController::edgeDrawing()
{
    // 1. 准备图像格式, Edge Drawing 支持灰度图(CV_8UC1)或彩色图 (CV_8UC3)
    QImage originalImage = imageModel->getImage(ImageModel::TYPE_ORIGIN);
    QImage workImage;
    if (originalImage.format() == QImage::Format_Grayscale8) {
        workImage = originalImage.copy(); // 已经是灰度 8bit, 直接用
    }else{
        // OpenCV 默认彩色处理通常基于BGR. 或用函数QImage::rgbSwapped();
        workImage = originalImage.convertToFormat(QImage::Format_BGR888);
    }
    // 2. 调用 DLL 函数获取边缘链
    opencved::EDResults results = RunEdgeDrawingFull(
        workImage.bits(),
        workImage.width(),
        workImage.height(),
        workImage.bytesPerLine(),
        (workImage.format() != QImage::Format_Grayscale8), // isColor = true
        edParams
        );
    // 3. 将结果绘制到一张新图中
    QImage edgeMap(workImage.width(), workImage.height(), QImage::Format_RGB888);
    edgeMap.fill(Qt::black);
    // 遍历所有边缘段并画点
    for (int i = 0; i < results.segmentCount; ++i)
    {
        opencved::EDEdgeSegment& seg = results.segments[i];
        // 动态计算颜色: 根据索引 i 平分 360 度色相环，保证相邻边缘颜色差异最大
        int hue = (i * 137) % 360; // 137 是黄金角度偏移，能让颜色分布更均匀(完全剩余系)
        QColor segmentColor = QColor::fromHsv(hue, 255, 255);
        for (int j = 0; j < seg.count; ++j)
        {
            opencved::EDPoint& pt = seg.points[j];
            // 确保坐标不越界
            if (pt.x >= 0 && pt.x < edgeMap.width() && pt.y >= 0 && pt.y < edgeMap.height())
            {
                uchar* line = edgeMap.scanLine(pt.y);
                uchar* pixel = line + (pt.x * 3);
                pixel[0] = segmentColor.red();
                pixel[1] = segmentColor.green();
                pixel[2] = segmentColor.blue();
            }
        }
    }
    // 4. !!! 非常重要：释放 DLL 内部申请的内存 !!!
    opencved::FreeEDResults(results);
    // 5. 更新模型
    imageModel->setImage(ImageModel::TYPE_OUTLINE, edgeMap);
    emit statusMessage(tr("Edge Drawing segments detected: %1").arg(results.segmentCount));
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
