#include "image_model.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

ImageModel::ImageModel(QObject *parent) : QObject(parent)
{
    imageVector.resize(NUM_TYPES);
}

bool ImageModel::loadFromFile(const QString &path)
{
    if (path.isEmpty()) {
        qWarning() << "Empty file path";
        return false;
    }

    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        qWarning() << "File does not exist:" << path;
        return false;
    }

    QImage img(path);
    if (img.isNull()) {
        qWarning() << "Failed to load image:" << path;
        return false;
    }

    imageVector[TYPE_ORIGIN] = img;
    currentDisplayImageType = TYPE_ORIGIN;
    currentPath = path;
    modified = false;
    emit imageChanged(TYPE_ORIGIN, img);
    emit currentDisplayImageChanged(img);
    emit filePathChanged(currentPath);

    return true;
}

bool ImageModel::saveToFile(const QString &path, const QString &format, int quality)
{
    const QImage &img = getImage(currentDisplayImageType);
    if (img.isNull()){
        qWarning() << "No image to save";
        return false;
    }

    QString saveFormat = format.isEmpty() ? "PNG" : format;

    QFileInfo fileInfo(path);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists() && !dir.mkpath(".")) {
        qWarning() << "Failed to create directory:" << dir.path();
        return false;
    }

    bool success = img.save(path, saveFormat.toUtf8().constData(), quality);

    if (success) {
        currentPath = path;
        modified = false;
        emit filePathChanged(currentPath);
    }
    return success;
}

void ImageModel::setImage(ImageType type, const QImage &img)
{
    if (img.isNull()) return;
    imageVector[type] = img;
    emit imageChanged(type, img);

    if(type == currentDisplayImageType){
        emit currentDisplayImageChanged(img);
    }
}

void ImageModel::setCurrentImageType(ImageType type)
{
    if (currentDisplayImageType == type) return;
    if (type >= imageVector.size()) return;

    currentDisplayImageType = type;
    const QImage &img = imageVector[type];
    emit currentDisplayImageChanged(img);
}
