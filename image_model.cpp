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

    imageVector[TYPE_ORIGIN] = img;
    currentDisplayImageType = TYPE_ORIGIN;
    currentPath = path;

    emit imageChanged(TYPE_ORIGIN, img);
    emit currentDisplayImageChanged(img);

    return true;
}

bool ImageModel::saveToFile(const QString &path, const QString &format, int quality)
{
    const QImage &img = getImage(currentDisplayImageType);
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

void ImageModel::setCurrentImageType(ImageType type)
{
    if (currentDisplayImageType == type) return;
    if (type >= imageVector.size()) return;

    currentDisplayImageType = type;
    const QImage &img = imageVector[type];
    emit currentDisplayImageChanged(img);
}
