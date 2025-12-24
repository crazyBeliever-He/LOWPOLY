#ifndef IMAGE_CONTROLLER_H
#define IMAGE_CONTROLLER_H

#include <QObject>
#include "image_model.h"
#include "image_widget.h"
#include "edge_drawing_lib.h"

class ImageController : public QObject
{
    Q_OBJECT

signals:
    void errorOccurred(const QString &message);             // 错误
    void statusMessage(const QString &message);             // 状态
    void displayTypeRequested(ImageModel::ImageType type);

public:
    //POD (Plain Old Data) 结构体（不含虚函数、构造函数、容器等）
    opencved::EDParams edParams;

public:
    ImageController(ImageModel *model,
                    ImageWidget *view,
                    QObject *parent = nullptr);             // Qt对象父对象
    ~ImageController();

    // set edge drawing params 注意 成员顺序 和 内存对齐问题(Padding)
    void setEdParams(bool PFmode = false,
                     int EdgeDetectionOperator = 1,
                     int GradientThresholdValue = 20,
                     int AnchorThresholdValue = 0,
                     int ScanInterval = 1,
                     int MinPathLength = 10,
                     float Sigma = 1.0f,
                     bool SumFlag = true,
                     bool NFAValidation = true,
                     int MinLineLength = 10,
                     double MaxDistanceBetweenTwoLines = 6.0,
                     double LineFitErrorThreshold = 1.0,
                     double MaxErrorThreshold = 1.3);
    void setEDParams(const opencved::EDParams& newParams);

    void edgeDrawing();
    void applyAllImageProcess();                            // 执行所有图像处理方法


public slots:
    void openImageWithDialog(QWidget *parent = nullptr);    // 打开图像的函数
    void saveImageWithDialog(QWidget *parent = nullptr);    // 保存图像的函数
    void onImageTypeSelected(int type);
    void onAutoSize(bool isAuto){ imageWidget->autoSize = isAuto;};


private:
    ImageModel *imageModel;
    ImageWidget *imageWidget;
    //GrayEdgeDrawing *grayEdgeDrawing;

};

#endif // IMAGE_CONTROLLER_H
