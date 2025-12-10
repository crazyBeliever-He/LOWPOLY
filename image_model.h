#ifndef IMAGE_MODEL_H
#define IMAGE_MODEL_H

#include <QImage>
#include <QString>
#include <QObject>
#include <QVector>

class ImageModel : public QObject
{

    Q_OBJECT

public:
    enum ImageType {
        TYPE_ORIGIN = 0,    // 原始图像
        TYPE_GRAY,          //
        TYPE_BLUR,          //
        TYPE_OUTLINE,       //
        TYPE_CUSTOM1,       //
        TYPE_CUSTOM2,       //
        NUM_TYPES           // 存储的图像数量
    };

public:
    explicit ImageModel(QObject *parent = nullptr);


    bool loadFromFile(const QString &path);
    bool saveToFile(const QString &path,
                    const QString &format = "PNG",
                    int quality = 100);

    /*获取指定类型的图像*/
    const QImage &getImage(ImageType type) const{return imageVector[type];}
    /*获取当前展示类型的图像*/
    const QImage &getcurrentImage() const{ return getImage(currentDisplayImageType);}
    ImageType getCurrentImageType() const{return currentDisplayImageType;}

    /*设置指定类型的图像*/
    void setImage(ImageType type, const QImage &img);
    /*切换当前展示图像类型*/
    void setCurrentImageType(ImageType type);

signals:
    /*切换被操作的图像(重新加载图片)*/
    void imageChanged(ImageType type, const QImage &newImage);
    /*当前显示图像变化*/
    void currentDisplayImageChanged(const QImage &newImage);
    /*切换打开文件的路径*/
    void filePathChanged(const QString &newPath);

private:
    QVector<QImage> imageVector;
    ImageType currentDisplayImageType = TYPE_ORIGIN;

    /*当前打开图像的路径*/
    QString currentPath;
    bool modified = false;
};


#endif // IMAGE_MODEL_H
