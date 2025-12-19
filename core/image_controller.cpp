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
    //imageModel->setImage(ImageModel::TYPE_GRAY, processImageWithED(imageModel->getcurrentImage()));
    imageModel->setImage(ImageModel::TYPE_OUTLINE, processImageWithED(imageModel->getcurrentImage()));

    emit statusMessage(tr("image processed"));
}

QImage ImageController::processImageWithED(const QImage &sourceImage)
{
    //return grayEdgeDrawing->getEDImageGray16(sourceImage);
    return grayEdgeDrawing->getEDImageFloat(sourceImage);
}


