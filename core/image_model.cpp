#include "image_model.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include "logger.h"

ImageModel::ImageModel(QObject *parent) : QObject(parent)
{
    imageVector.resize(NUM_TYPES);
}

bool ImageModel::loadFromFile(const QString &path)
{
    if (path.isEmpty()) {
        LOG_WARNING << "Load failed: empty path.";
        return false;
    }

    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        LOG_WARNING << "Load failed: file not found - " << path;
        return false;
    }

    QImage img(path);
    if (img.isNull()) {
        LOG_ERROR << "Load failed: Failed to load image - " << path;
        return false;
    }
    if (img.format() == QImage::Format_Invalid) {
        LOG_ERROR << "Load failed: Invalid image format - " << path;
        return false;
    }
    // 转换为 RGB888 格式
    if (img.format() != QImage::Format_RGB888 &&
        img.format() != QImage::Format_Grayscale8 &&
        img.format() != QImage::Format_Grayscale16)
    {
        img = img.convertToFormat(QImage::Format_RGB888);
    }

    imageVector[TYPE_ORIGIN] = img;
    displayType = TYPE_ORIGIN;
    currentPath = path;

    emit originImageChanged(TYPE_ORIGIN, img);
    emit displayTypeUpdated(img);

    return true;
}

bool ImageModel::saveToFile(const QString &path, const QString &format, int quality)
{
    const QImage &img = getcurrentImage();
    if (img.isNull()){
        LOG_WARNING << "Save failed: No image to save.";
        return false;
    }

    QString saveFormat = format.isEmpty() ? "PNG" : format;

    QFileInfo fileInfo(path);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists() && !dir.mkpath(".")) {
        LOG_ERROR << "Save failed: Failed to create directory - " << path;
        return false;
    }
    // bool QImage::save(1%, 2%, 3%), 其中2%: const char *format= nullptr
    bool success = img.save(path, saveFormat.toUtf8().constData(), quality);

    if (success) {
        currentPath = path;
    } else{
        LOG_WARNING << "Save failed: Failed to save image - " << path;
    }

    return success;
}

void ImageModel::setImage(ImageType type, const QImage &img)
{
    if (img.isNull()) return;
    imageVector[type] = img;
}

void ImageModel::setDisplayType(ImageType type)
{
    if (displayType == type) return;
    if (type >= imageVector.size()) return;

    displayType = type;
    const QImage &img = imageVector[type];
    emit displayTypeUpdated(img);
}
