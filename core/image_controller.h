#ifndef IMAGE_CONTROLLER_H
#define IMAGE_CONTROLLER_H

#include <QObject>
#include "image_model.h"
#include "image_widget.h"
#include "edge_drawing_gray.h"

class ImageController : public QObject
{
    Q_OBJECT

signals:
    void errorOccurred(const QString &message);             // 错误
    void statusMessage(const QString &message);             // 状态
    void displayTypeRequested(ImageModel::ImageType type);

public:
    ImageController(ImageModel *model,
                    ImageWidget *view,
                    QObject *parent = nullptr);             // Qt对象父对象

public slots:
    void onImageTypeSelected(int type);
    void openImageWithDialog(QWidget *parent = nullptr);    // 打开图像的函数
    void saveImageWithDialog(QWidget *parent = nullptr);    // 保存图像的函数
    void onApplyAllImageProcess();                          // 执行所有图像处理函数

private:
    //其他图像处理函数，但不是现在的这些
    QImage processImageWithED(const QImage &sourceImage);

private:
    ImageModel *imageModel;
    ImageWidget *imageWidget;
    GrayEdgeDrawing *grayEdgeDrawing;
};

#endif // IMAGE_CONTROLLER_H
