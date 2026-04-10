#include <Qdebug>
#include "image_worker.h"
#include "image_controller.h"

ImageWorker::ImageWorker(ImageController *ctrl, QObject *parent)
    : QObject(parent), controller(ctrl)
{
}

void ImageWorker::processAll()
{
    // 按顺序调用 Controller 现有的算法逻辑，并周期性抛出进度
    emit progressUpdated(0, tr("Edge Drawing..."));
    controller->applyEdgeDrawing();

    emit progressUpdated(20, tr("Douglas Peucker..."));
    controller->applyDouglasPeucker();

    emit progressUpdated(40, tr("Saliency Detection..."));
    controller->applySaliencyDetection();

    emit progressUpdated(60, tr("Feature Flow..."));
    controller->applyJumpFloodingAndFeatureFlow();

    emit progressUpdated(80, tr("Vertex Optimization..."));
    controller->applyVertexOptimization();

    emit progressUpdated(90, tr("Constrained Triangulation..."));
    controller->applyConstrainedTriangulation();

    emit progressUpdated(100, tr("Finished!"));
    emit finished();
}
void ImageWorker::processAllTime()
{
    // 定义一个简单的计时并执行的 Lambda
    auto traceStep = [&](const QString& name, int progress, std::function<void()> func) {
        emit progressUpdated(progress, name + "...");

        // 获取开始时间
        auto start = std::chrono::high_resolution_clock::now();

        // 执行算法
        func();

        // 获取结束时间
        auto end = std::chrono::high_resolution_clock::now();

        // 计算耗时（使用 double 以获得最高精度，单位：毫秒）
        std::chrono::duration<double, std::milli> elapsed = end - start;

        qDebug().noquote() << QString("[%1] Elapsed time: %2 ms")
                                  .arg(name, QString::number(elapsed.count(), 'f'));
    };

    // --- 开始执行算法流 ---

    traceStep(tr("Edge Drawing"), 0, [this](){
        controller->applyEdgeDrawing();
    });

    traceStep(tr("Douglas Peucker"), 20, [this](){
        controller->applyDouglasPeucker();
    });

    traceStep(tr("Saliency Detection"), 40, [this](){
        controller->applySaliencyDetection();
    });

    traceStep(tr("Feature Flow"), 60, [this](){
        controller->applyJumpFloodingAndFeatureFlow();
    });

    traceStep(tr("Vertex Optimization"), 80, [this](){
        controller->applyVertexOptimization();
    });

    traceStep(tr("Constrained Triangulation"), 90, [this](){
        controller->applyConstrainedTriangulation();
    });

    emit progressUpdated(100, tr("Finished!"));
    emit finished();
}
