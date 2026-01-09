#include "image_model.h"

#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include "logger.h"

ImageModel::ImageModel(QObject *parent) : QObject(parent)
{
    imageVector.resize(typeToIdx(ImageType::Count));
}

int ImageModel::typeToIdx(ImageType type) const
{
    return static_cast<int>(type);
}

const QImage &ImageModel::getImage(ImageType type) const
{
    return imageVector[typeToIdx(type)];
}

const QImage &ImageModel::getcurrentImage() const
{
    return imageVector[typeToIdx(currentType)];
}

void ImageModel::setImage(ImageType type, const QImage &img)
{
    if (img.isNull()) return;
    imageVector[typeToIdx(type)] = img;
    if (type == currentType)
    {
        emit displayImageUpdated(img);
    }
}

void ImageModel::setCurrentImageType(ImageType type)
{
    currentType = type;
    emit displayImageUpdated(getImage(type));
}

bool ImageModel::loadImage(const QString &path)
{
    if (path.isEmpty())
    {
        LOG_WARNING << "Load failed: empty path.";
        return false;
    }
    QFileInfo fileInfo(path);
    if (!fileInfo.exists())
    {
        LOG_WARNING << "Load failed: file not found - " << path;
        return false;
    }
    QImage img(path);
    if (img.isNull())
    {
        LOG_ERROR << "Load failed: Failed to load image - " << path;
        return false;
    }
    if (img.format() == QImage::Format_Invalid)
    {
        LOG_ERROR << "Load failed: Invalid image format - " << path;
        return false;
    }
    imageVector[typeToIdx(ImageType::Origin)] = img;
    currentType = ImageType::Origin;
    currentPath = path;

    emit originImageChanged(img);
    return true;
}

bool ImageModel::saveImage(const QString &path, const QString &format, int quality)
{
    const QImage &img = getcurrentImage();
    if (img.isNull())
    {
        LOG_WARNING << "Save failed: No image to save.";
        return false;
    }
    QString saveFormat = format.isEmpty() ? "PNG" : format;
    QFileInfo fileInfo(path);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists() && !dir.mkpath("."))
    {
        LOG_ERROR << "Save failed: Failed to create directory - " << path;
        return false;
    }
    // bool QImage::save(1%, 2%, 3%), 其中2%: const char *format= nullptr
    bool success = img.save(path, saveFormat.toUtf8().constData(), quality);

    if (success)
    {
        currentPath = path;
    }
    else
    {
        LOG_WARNING << "Save failed: Failed to save image - " << path;
    }
    return success;
}
