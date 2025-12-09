#ifndef IMAGE_CONTROLLER_H
#define IMAGE_CONTROLLER_H

#include <QObject>
#include "image_model.h"
#include "image_widget.h"

class ImageController : public QObject
{
    Q_OBJECT

signals:
    void imageOpened(const QImage &image);                  // 打开图像后的信号
    void imageSaved(bool success);                          // 保存图像后的信号
    void errorOccurred(const QString &message);             // 错误信号
    void processingStarted();                               // 处理开始的信号
    void processingCompleted(ImageModel::ImageType type);   // 处理完成的信号

public:
    //
    ImageController(ImageModel *model,
                    ImageWidget *view,
                    QObject *parent = nullptr);             // Qt对象父对象

public slots:
    void openImageWithDialog(QWidget *parent = nullptr);     // 打开图像的函数
    void saveImageWithDialog(QWidget *parent = nullptr);     // 保存图像的函数

private:
    //其他图像处理函数，但不是现在的这些
    void applyOutline();                            // 应用描边处理
    void applyGray();                               // 应用灰度处理
    void applyBlur();                               // 应用模糊处理



private:
    ImageModel *imageModel;
    ImageWidget *imageWidget;
};

#endif // IMAGE_CONTROLLER_H
