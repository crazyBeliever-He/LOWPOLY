#ifndef IMAGE_CONTROLLER_H
#define IMAGE_CONTROLLER_H

#include <QObject>
#include "image_model.h"
#include "image_widget.h"

#include "algorithm_params.h"
#include "douglas_peucker.h"
#include "edge_drawing.h"

class ImageController : public QObject
{
    Q_OBJECT

public:
    EdgeDrawing *edgeDrawing;
    DouglasPeucker *douglasPeucker;

private:
    enum class ProcessStage {
        None,
        EdgeDrawingDone,
        DouglasPeuckerDone
    };
    ProcessStage currentStage = ProcessStage::None;
    ImageModel *imageModel;
    ImageWidget *imageWidget;

public:
    ImageController(ImageModel *model,
                    ImageWidget *view,
                    QObject *parent = nullptr);             // Qt对象父对象
    ~ImageController();

    opencved::EDParams getEDParams() const {return edgeDrawing->edParams;}
    void setEDParams(const opencved::EDParams& newParams);

    DPParams getDPParams() const {return douglasPeucker->dpParams;}
    void setDPParams(const DPParams& newParams);

    void applyEdgeDrawing();
    void applyDouglasPeucker();
    void applyAllImageProcess();                            // 执行所有图像处理方法

signals:
    void errorOccurred(const QString &message);             // 错误
    void statusMessage(const QString &message);             // 状态
    void requestUiReset();      // 请求重置ui的一些设置
    //void displayTypeRequested(ImageModel::ImageType type);

public slots:
    void openImageWithDialog(QWidget *parent = nullptr);    // 打开图像的函数
    void saveImageWithDialog(QWidget *parent = nullptr);    // 保存图像的函数
    void onImageTypeSelected(int type);
    void onAutoSize(bool isAuto){ imageWidget->autoSize = isAuto;}

};

#endif // IMAGE_CONTROLLER_H
