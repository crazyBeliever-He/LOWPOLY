#include "image_controller.h"
#include <QFileDialog>
#include "logger.h"

ImageController::ImageController(ImageModel *model,
                                 ImageWidget *view,
                                 QObject *parent)
    : QObject(parent),          // 用于对话框的父窗口,传递给基类，参与内存管理
    imageModel(model),
    imageWidget(view)
{
    connect(imageModel, &ImageModel::currentDisplayImageChanged,imageWidget, &ImageWidget::setImage);
    connect(imageModel, &ImageModel::imageChanged, this, &ImageController::applyAllImageProcess);

}

void ImageController::openImageWithDialog(QWidget *parent)
{
    QString path = QFileDialog::getOpenFileName(parent,
                                                tr("Open"),
                                                "",
                                                "Images (*.png *.jpg *.jpeg *.bmp)");

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
                                                "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)");

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

void ImageController::applyAllImageProcess()
{
    // TODO: 如果有多种不相关的图像处理，考虑并行处理
    applyOutline();
    applyGray();
}

void ImageController::applyOutline()
{
    QImage src = imageModel->getImage(ImageModel::TYPE_ORIGIN);
    QImage dst = src.convertToFormat(QImage::Format_RGB888);  // 转为 RGB 格式

    // 假设这里用 Sobel 算法实现简单的边缘检测
    for (int y = 1; y < dst.height() - 1; ++y) {
        for (int x = 1; x < dst.width() - 1; ++x) {
            QColor color1 = QColor(src.pixel(x - 1, y - 1));
            QColor color2 = QColor(src.pixel(x + 1, y + 1));
            QColor color3 = QColor(src.pixel(x + 1, y - 1));
            QColor color4 = QColor(src.pixel(x - 1, y + 1));

            // 假设做一个简单的边缘效果：计算颜色差异（实际算法可以更复杂）
            int r = qAbs(color1.red() - color2.red());
            int g = qAbs(color3.green() - color4.green());
            int b = qAbs(color1.blue() - color4.blue());

            dst.setPixel(x, y, qRgb(r, g, b));
        }
    }

    imageModel->setImage(ImageModel::TYPE_OUTLINE, dst); // 保存处理后的图像
}

void ImageController::applyGray()
{
    QImage src = imageModel->getImage(ImageModel::TYPE_ORIGIN);
    QImage dst = src.convertToFormat(QImage::Format_RGB888);

    for (int y = 0; y < dst.height(); ++y) {
        for (int x = 0; x < dst.width(); ++x) {
            QColor color = QColor(src.pixel(x, y));
            int gray = qGray(color.red(), color.green(), color.blue());
            dst.setPixel(x, y, qRgb(gray, gray, gray));
        }
    }

    imageModel->setImage(ImageModel::TYPE_GRAY, dst);   // 保存处理后的图像
}

void ImageController::applyBlur()
{
    QImage src = imageModel->getImage(ImageModel::TYPE_ORIGIN);
    QImage dst = src;

    // 简单进行一个平均模糊
    for (int y = 1; y < dst.height() - 1; ++y) {
        for (int x = 1; x < dst.width() - 1; ++x) {
            QColor c1 = QColor(src.pixel(x - 1, y - 1));
            QColor c2 = QColor(src.pixel(x + 1, y - 1));
            QColor c3 = QColor(src.pixel(x - 1, y + 1));
            QColor c4 = QColor(src.pixel(x + 1, y + 1));

            int r = (c1.red() + c2.red() + c3.red() + c4.red()) / 4;
            int g = (c1.green() + c2.green() + c3.green() + c4.green()) / 4;
            int b = (c1.blue() + c2.blue() + c3.blue() + c4.blue()) / 4;

            dst.setPixel(x, y, qRgb(r, g, b));
        }
    }

    imageModel->setImage(ImageModel::TYPE_BLUR, dst);
}
