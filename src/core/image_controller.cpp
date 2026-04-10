#include "image_controller.h"

#include <QFileDialog>
#include <QPainter>
#include <QThread>
#include <QImageReader>
#include "logger.h"
#include "param_util.h"
// 包含所有前向声明过的完整定义
#include "image_model.h"
#include "image_widget.h"
#include "algorithm_params.h"
#include "edge_drawing.h"
#include "douglas_peucker.h"
#include "saliency_detection.h"
#include "feature_flow.h"
#include "vertex_optimization.h"
#include "constrained_triangulation.h"

#include "image_worker.h"

ImageController::ImageController(ImageModel *model,
                                 ImageWidget *view,
                                 QObject *parent)
    : QObject(parent),          // 用于对话框的父窗口,传递给基类，参与内存管理
    imageModel(model),
    imageWidget(view)
{
    // 1. edge drawing 参数初始化
    edgeDrawing = std::make_unique<EdgeDrawing>();
    EDParamUtil::getDefaultEDParams(edgeDrawing->edParams);

    // 2. douglas peucker 参数初始化
    douglasPeucker = std::make_unique<DouglasPeucker>();
    DPParamsUtil::getDefaultDPParams(douglasPeucker->dpParams);

    // 3. saliency detection 参数初始化
    saliencyDetection = std::make_unique<SaliencyDetection>();
    SDParamsUtil::getDefaultSDParams(saliencyDetection->sDParams);

    // 4. jump flooding 和 feature flow 参数初始化
    featureFlow = std::make_unique<FeatureFlow>();

    // 5. vertex optimization 初始化
    vertexOptimization = std::make_unique<VertexOptimization>();

    // 6. constrained triangulation 初始化
    constrainedTriangulation = std::make_unique<ConstrainedTriangulation>();

    // 7. 初始化多线程架构
    workerThread = new QThread(this);
    // 注意：Worker 不能传入 parent，否则 moveToThread 会报错！
    worker = new ImageWorker(this, nullptr);
    worker->moveToThread(workerThread);
    // 内部通路：Controller 触发 Worker
    connect(this, &ImageController::requestWorkerStart, worker, &ImageWorker::processAllTime);

    // 外部通路：Worker 进度传递回 Controller，再转发给 UI (或直接转发)
    connect(worker, &ImageWorker::progressUpdated, this, &ImageController::progressUpdated);

    // 收尾工作：任务完成后重置状态锁
    connect(worker, &ImageWorker::finished, this, [this](){
        this->isProcessing = false;
        emit processFinished();
    });

    // 启动子线程的事件循环
    workerThread->start();



    // n. 实现connect
    // 切换源图
    connect(imageModel, &ImageModel::originImageChanged,
            this, &ImageController::onOriginImageChanged);
    // ImageModel 和 ImageWidget 解耦
    connect(imageModel, &ImageModel::displayImageUpdated,
            imageWidget, &ImageWidget::updateImageKeepView);
    // 绑定用户交互信号
    connect(imageWidget, &ImageWidget::requestAddPoint,
            this, &ImageController::onAddPointRequested);
    connect(imageWidget, &ImageWidget::requestDeletePointsInArea,
            this, &ImageController::onDeletePointsRequested);
    connect(imageWidget, &ImageWidget::requestSelectTriangle,
            this, &ImageController::onTriangleSelected);
}

ImageController::~ImageController()
{
    // 安全退出线程的标准写法
    if (workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait();
    }
    delete worker;
}

void ImageController::onOriginImageChanged(const QImage& img)
{
    // 重置widget ui
    emit requestUiReset();
    //currentStage = ProcessStage::None;

    // 执行算法
    applyAllImageProcess();
    // 通知 ImageWidget 展示源图
    imageWidget->updateImageResetView(img);
}

void ImageController::onImageTypeSelected(int intType)
{
    ImageModel::ImageType imageType = ImageModel::ImageType::Origin;
    if (intType >= 0 && intType < imageModel->typeToIdx(ImageModel::ImageType::Count))
    {
        //qDebug() << intType;
        imageType = static_cast<ImageModel::ImageType>(intType);
        imageModel->setCurrentImageType(imageType);

        // ==========================================
        // 视图切换时的交互状态清理
        if (imageType != ImageModel::ImageType::Test)
        {
            // 1. 清除 ImageWidget 画布上的高亮三角形
            // 因为 Controller 拥有 imageWidget 的指针，所以由它来直接调用最合适
            if (imageWidget)
            {
                imageWidget->setHighlightedTriangle(QPolygonF());
            }
            // 2. 发出信号，通知 Widget 隐藏颜色面板
            emit closeColorDialogRequested();
        }
    }
}

/********************************************************************************/
// apply algorithms
/********************************************************************************/

// apply alogrithms
void ImageController::applyAllImageProcess()
{

    // // 阶段 1: Edge Drawing. 如果当前 ED 没做,则执行它
    // //if (currentStage < ProcessStage::EdgeDrawingDone)
    // {
    // applyEdgeDrawing();
    //     //currentStage = ProcessStage::EdgeDrawingDone;
    // }

    // // 阶段 2: Douglas Peucker. 如果当前没做 DP,或者 ED 重做导致 DP 失效
    // //if (currentStage < ProcessStage::DouglasPeuckerDone)
    // {
    // applyDouglasPeucker();
    //     //currentStage = ProcessStage::DouglasPeuckerDone;
    // }

    // // 阶段 3: 只有在边缘和约束点都计算完毕后，才能执行显著性检测与采样
    // applySaliencyDetection();

    // // 阶段 4: 特征流场
    // applyJumpFloodingAndFeatureFlow();

    // // 阶段 5: 顶点优化
    // applyVertexOptimization();

    // // 阶段 6: 三角剖分和颜色处理
    // applyConstrainedTriangulation();




    // 防抖与防并发锁：如果正在处理，则拒绝新的处理请求
    if (isProcessing) {
        LOG_INFO << "Worker is busy, ignoring new request.";
        return;
    }

    isProcessing = true;

    // 异步触发后台线程工作
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    QSize size = originImage.size();
    qDebug() << "CUDA\nOrigin image size:" << size;
    emit requestWorkerStart();

}

void ImageController::applyEdgeDrawing()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);

    bool success = edgeDrawing->edgeDrawingInLib(originImage);
    if(!success)
    {
        LOG_ERROR << "edge drawing form opencv failed";
    }
    QImage edgeMap = edgeDrawing->drawImage(originImage.width(), originImage.height());

    imageModel->setImage(ImageModel::ImageType::EdgeDrawing, edgeMap);

    //QPair<int, int> edInfo = edgeDrawing->getEDPointsNumber();
    //qDebug()<<"Edge Drawing Info: segmentCount is " <<edInfo.first<<", totalPoints is " <<edInfo.second;
}

void ImageController::applyDouglasPeucker()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();

    bool success = douglasPeucker->simplify(edgeDrawing->edResults, w, h);
    if(!success)
    {
        LOG_ERROR << "douglas peucker simplfy failed";
    }
    QImage pointMap = douglasPeucker->drawPointImage(w, h);
    QImage lineMap = douglasPeucker->drawLineImage(w, h);

    imageModel->setImage(ImageModel::ImageType::DouglasPoint, pointMap);
    imageModel->setImage(ImageModel::ImageType::DouglasLine, lineMap);

    //int dpInfo = douglasPeucker->getDPPointsNumber();
    //qDebug()<<"Douglas Peucker Info: totalPoints is " <<dpInfo;
}

void ImageController::applySaliencyDetection()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();

    QImage result = saliencyDetection->saliencyDetectionInterface(originImage);
    if(result.isNull())
    {
        LOG_ERROR << "saliency detection failed";
    }
    imageModel->setImage(ImageModel::ImageType::SaliencyDetection, result);

    // TODO: douglas peucker 算法生成的约束点较多, 一般大于默认的约束点总数, 要优化
    int sdInfo =saliencyDetection->generateNonUniformSamples(w, h, douglasPeucker->getDPPointsNumber(), douglasPeucker->dpParams.eta);

    //qDebug()<<"saliency detection Info: samples is " <<sdInfo;
}

void ImageController::applyJumpFloodingAndFeatureFlow()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();
    bool successJFA =  featureFlow->jumpFloodingCUDAApi(edgeDrawing->edResults,w,h);
    //bool successJFA =false;
    if(!successJFA)
    {
        LOG_INFO <<"use CPU jfa";
        successJFA =  featureFlow->jumpFloodingCPUApi(edgeDrawing->edResults,w,h);
        if(!successJFA)
        {
            LOG_ERROR << "both CUDA and CPU jfa fail";
        }
    }
    QImage resultJFA = featureFlow->drawJumpFloodingMap(w,h,featureFlow->jFResults,edgeDrawing->edResults);
    imageModel->setImage(ImageModel::ImageType::JumpFlooding, resultJFA);

    float Li = douglasPeucker->dpParams.eta*(w+h);
    bool successFF = featureFlow->featureFlowCUDAApi(featureFlow->jFResults,w,h,Li);
    //bool successFF =false;
    if(!successFF)
    {
        LOG_INFO <<"use CPU ff";
        successFF =  featureFlow->featureFlowCPUApi(featureFlow->jFResults,w,h,Li);
        if(!successFF)
        {
            LOG_ERROR << "both CUDA and CPU ff  fail";
        }
    }
    QImage resultFF = featureFlow->drawFeatureFlowMap(w,h,featureFlow->fFResults);
    imageModel->setImage(ImageModel::ImageType::FeatureFlow, resultFF);
}

void ImageController::applyVertexOptimization()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();
    bool success = vertexOptimization->vertexOptimizationApi(w, h,
                                                             douglasPeucker->getDPPointsNumber(),
                                                             saliencyDetection->sampledPoints,
                                                             douglasPeucker->dpResults,
                                                             douglasPeucker->cornerPoints,
                                                             featureFlow->fFResults);
    if(!success)
    {
        LOG_ERROR << " vertex optimization fail";
        return;
    }
    QImage result = vertexOptimization->drawOptimizedVertices(w, h,
                                                              douglasPeucker->dpResults,
                                                              douglasPeucker->cornerPoints);
    imageModel->setImage(ImageModel::ImageType::VertexOptimization, result);
}

void ImageController::applyConstrainedTriangulation()
{

    bool success = constrainedTriangulation->generateLowPolyMesh(vertexOptimization->optimizedPoints,
                                                  douglasPeucker->dpResults,
                                                  douglasPeucker->cornerPoints);
    if(!success)
    {
        LOG_ERROR << " constrained triangulation fail";
    }
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();
    QImage result = constrainedTriangulation->drawTriangulationWireframe(w, h);
    imageModel->setImage(ImageModel::ImageType::TriangulationWireframe, result);
    QImage result2 = constrainedTriangulation->drawLowPolyResult(originImage);
    imageModel->setImage(ImageModel::ImageType::LowPoly, result2);
    imageModel->setImage(ImageModel::ImageType::Test, result2);
}

/********************************************************************************/
// get/set params from dialogs
/********************************************************************************/

// get params uniform inerface
QVariant ImageController::getParams(const QString& typeName) const
{
    // 在这里维护类型映射    comtroller -> dialogs
    if (typeName == QMetaType::fromType<opencved::EDParams>().name())
        return QVariant::fromValue(edgeDrawing->edParams);
    else if (typeName == QMetaType::fromType<DPParams>().name())
        return QVariant::fromValue(douglasPeucker->dpParams);
    else if (typeName == QMetaType::fromType<SDParams>().name())
        return QVariant::fromValue(saliencyDetection->sDParams);
    return QVariant();
}

// set params uniform inerface
void ImageController::setParams(const QString& typeName, const QVariant& data)
{
    // 在这里维护类型映射    dialogs -> controller
    bool success = true;
    QString feedbackMsg = tr("Success");

    try {
        if (typeName == QMetaType::fromType<opencved::EDParams>().name()) {
            setEDParams(data.value<opencved::EDParams>());
        }
        else if (typeName == QMetaType::fromType<DPParams>().name()) {
            setDPParams(data.value<DPParams>());
        }
        else if(typeName == QMetaType::fromType<SDParams>().name()){
            setSDParams(data.value<SDParams>());
        }
        else {
            success = false;
            feedbackMsg = tr("Unknown parameter type");
        }
    } catch (...) {
        // 如果底层算法抛出异常，这里可以捕获
        success = false;
        feedbackMsg = tr("Algorithm execution failed");
    }

    //如果未来 setEDParams 内部把任务丢给了工作线程, 可以等工作线程的 finished 信号触发后, 再发出 paramsApplyFinished 信号
    // 无论成功还是失败，都把结果发射出去
    emit paramsApplyFinished(typeName, success, feedbackMsg);
}

opencved::EDParams ImageController::getEDParams() const
{
    return edgeDrawing->edParams;
}
void ImageController::setEDParams(const opencved::EDParams& newParams){
    if (!EDParamUtil::compareEDParam(edgeDrawing->edParams, newParams))
    {
        edgeDrawing->edParams = newParams;
        // ED 参数变了，当前进度重置为 None
        //currentStage = ProcessStage::None;
        applyAllImageProcess();
    }
}

DPParams ImageController::getDPParams() const
{
    return douglasPeucker->dpParams;
}
void ImageController::setDPParams(const DPParams& newParams)
{
    if(!DPParamsUtil::compareDPParams(douglasPeucker->dpParams, newParams))
    {
        douglasPeucker->dpParams = newParams;
        // 如果 DP 参数变了，进度最多只能维持在 EdgeDrawing 完成状态
        //if (currentStage > ProcessStage::EdgeDrawingDone)
        {
            //currentStage = ProcessStage::EdgeDrawingDone;
            applyAllImageProcess();
        }
    }
}

SDParams ImageController::getSDParams() const
{
    return saliencyDetection->sDParams;
}
void ImageController::setSDParams(const SDParams& newParams)
{
    if (!SDParamsUtil::compareSDParams(saliencyDetection->sDParams, newParams))
    {
        saliencyDetection->sDParams = newParams;
        // 显著性检测独立于 edge drawing 和 douglas peucker, 直接重新应用即可
        applySaliencyDetection();
    }
}

void ImageController::onAutoSize(bool isAuto)
{
    imageWidget->autoSize = isAuto;
}

/********************************************************************************/
// open/save image
/********************************************************************************/

// open&load image from sotrage
void ImageController::onOpenImage(QWidget *parent)
{
    QString path = QFileDialog::getOpenFileName(parent,
                                                tr("Open"),
                                                "C:/Users/h7286/Desktop/testimage",
                                                "Images (*.jpg *.jpeg *.png *.bmp *.tif *.tiff)");
    if (path.isEmpty())
    {
        emit statusMessage(tr("Open canceled"));
        return;
    }
    QImageReader::setAllocationLimit(8192);
    if (imageModel->loadImage(path))
    {
        emit statusMessage(tr("Image loaded"));
        LOG_INFO << "Load: " << path;
        return;
    }
    emit errorOccurred(tr("Failed to load image from: ") + path);
}

// save image to storage
void ImageController::onSaveImage(QWidget *parent)
{
    if (imageModel->getcurrentImage().isNull())
    {
        emit errorOccurred(tr("No image to save"));
        return;
    }
    QString path = QFileDialog::getSaveFileName(parent,
                                                tr("Save"),
                                                "",
                                                "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp);;TIFF(*.tiff *.tif)");
    if (path.isEmpty())
    {
        emit statusMessage(tr("Save canceled"));
        return;
    }
    if(imageModel->saveImage(path))
    {
        emit statusMessage(tr("Image saved"));
        LOG_INFO << "Save: " << path;
        return;
    }
    emit errorOccurred(tr("Failed to save image to: ") + path);
}

/********************************************************************************/
// user interaction
/********************************************************************************/

void ImageController::onAddPointRequested(QPointF imgPos)
{
    auto currentType = imageModel->getCurrentImageType();
    if (currentType != ImageModel::ImageType::VertexOptimization &&
        currentType != ImageModel::ImageType::TriangulationWireframe)
    {
        return; // 不在指定状态，拒绝交互
    }

    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    // 越界保护
    if (imgPos.x() < 0 || imgPos.y() < 0 || imgPos.x() >= originImage.width() || imgPos.y() >= originImage.height()) {
        return;
    }

    // 直接作为优化好的自由点加入
    vertexOptimization->optimizedPoints.push_back(imgPos);

    // 局部重绘并刷新下游阶段
    updateAfterInteraction();
}

void ImageController::onDeletePointsRequested(QRectF imgArea)
{
    auto currentType = imageModel->getCurrentImageType();
    if (currentType != ImageModel::ImageType::VertexOptimization &&
        currentType != ImageModel::ImageType::TriangulationWireframe)
    {
        return; // 不在指定状态，拒绝交互
    }

    // 1. 从自由点集合中删除
    auto& optPoints = vertexOptimization->optimizedPoints;
    optPoints.erase(std::remove_if(optPoints.begin(), optPoints.end(),
                                   [&imgArea](const QPointF& pt) { return imgArea.contains(pt); }), optPoints.end());

    // 2. 从约束点 (Douglas-Peucker 结果) 中删除
    auto& dpLines = douglasPeucker->dpResults;
    for (auto& line : dpLines) {
        line.erase(std::remove_if(line.begin(), line.end(),
                                  [&imgArea](const QPoint& pt) { return imgArea.contains(QPointF(pt)); }), line.end());
    }

    // 如果想连四个角点也能删除，可以对 douglasPeucker->cornerPoints 执行同样操作。
    // 但通常保留角点有利于三角剖分覆盖全图，建议不删。

    // 局部重绘并刷新下游阶段
    updateAfterInteraction();
}

void ImageController::updateAfterInteraction()
{
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    int w = originImage.width();
    int h = originImage.height();

    // 1. 刷新 VertexOptimization 的渲染图
    QImage voResult = vertexOptimization->drawOptimizedVertices(w, h,
                                                                douglasPeucker->dpResults,
                                                                douglasPeucker->cornerPoints);
    imageModel->setImage(ImageModel::ImageType::VertexOptimization, voResult);

    // 2. 重新跑 Constrained Triangulation
    applyConstrainedTriangulation();

    // 3. UI 焦点保持: 由于重新 setImage 会触发 displayImageUpdated,
    // ImageWidget 会自动刷新当前视图，无需特殊干预。
}

void ImageController::clearTriangleSelection()
{
    if (imageWidget) {
        imageWidget->setHighlightedTriangle(QPolygonF());
    }
}

void ImageController::onTriangleSelected(QPointF imgPos)
{
    auto currentType = imageModel->getCurrentImageType();
    if (currentType != ImageModel::ImageType::Test) {
        // 如果不是 Test 模式，清理高亮框并退出
        imageWidget->setHighlightedTriangle(QPolygonF());
        return;
    }

    int triIndex = constrainedTriangulation->findTriangleAt(imgPos);
    if (triIndex != -1) {
        // 1. 让 Widget 画高亮框
        QPolygonF poly = constrainedTriangulation->getTrianglePolygon(triIndex);
        imageWidget->setHighlightedTriangle(poly);

        // 2. 获取当前颜色 (优先取自定义，其次取缓存)
        QColor currentColor = constrainedTriangulation->cachedColors[triIndex];
        if (constrainedTriangulation->customColors.count(triIndex)) {
            currentColor = constrainedTriangulation->customColors[triIndex];
        }

        // 3. 通知主窗口弹出或更新颜色调节面板
        emit openColorDialogRequested(triIndex, currentColor);
    } else {
        imageWidget->setHighlightedTriangle(QPolygonF());
    }
}

void ImageController::updateTriangleColor(int triIndex, QColor newColor)
{
    // 保存自定义颜色
    constrainedTriangulation->customColors[triIndex] = newColor;

    // 极速重绘
    QImage originImage = imageModel->getImage(ImageModel::ImageType::Origin);
    QImage newResult = constrainedTriangulation->redrawLowPolyFast(originImage);

    // 更新 Test 和 LowPoly 图层
    imageModel->setImage(ImageModel::ImageType::Test, newResult);
    imageModel->setImage(ImageModel::ImageType::LowPoly, newResult);
}
