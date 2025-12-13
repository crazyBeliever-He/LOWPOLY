#ifndef IMAGE_CONTROLLER_H
#define IMAGE_CONTROLLER_H

#include <QObject>
#include "image_model.h"
#include "image_widget.h"

class ImageController : public QObject
{
    Q_OBJECT

signals:
    void errorOccurred(const QString &message);             // 错误
    void statusMessage(const QString &message);             // 状态

public:
    ImageController(ImageModel *model,
                    ImageWidget *view,
                    QObject *parent = nullptr);             // Qt对象父对象

public slots:
    void openImageWithDialog(QWidget *parent = nullptr);    // 打开图像的函数
    void saveImageWithDialog(QWidget *parent = nullptr);    // 保存图像的函数
    void applyAllImageProcess();                            // 执行所有图像处理函数

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
