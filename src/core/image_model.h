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
    enum class ImageType {
        Origin = 0,
        EdgeDrawing,
        DouglasPoint,
        DouglasLine,
        SaliencyDetection,
        Count
    };
    Q_ENUM(ImageType) // 允许 Qt 元系统识别

private:
    QVector<QImage> imageVector;    // 存储不同类型的图像
    ImageType currentType = ImageType::Origin;    // 展示的图像
    QString currentPath;    // 当前打开图像的路径
    //bool modified = false;    // 图像是否变化

public:
    explicit ImageModel(QObject *parent = nullptr);

    // 辅助函数: type转换 int 函数
    int typeToIdx(ImageType type) const;
    const QImage &getImage(ImageType type) const;
    const QImage &getcurrentImage() const;
    void setImage(ImageType type, const QImage &img);
    void setCurrentImageType(ImageType type);

    bool loadImage(const QString &path);
    bool saveImage(const QString &path, const QString &format = "PNG", int quality = 100);

signals:
    void originImageChanged(const QImage &newImage);    // 切换源图像
    void displayImageUpdated(const QImage &newImage);   // 当前显示图像变化
    //void filePathChanged(const QString &newPath); // 切换打开文件的路径
};

#endif // IMAGE_MODEL_H
