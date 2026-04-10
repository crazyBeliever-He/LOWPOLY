#ifndef IMAGE_WORKER_H
#define IMAGE_WORKER_H

#include <QObject>

class ImageController; // 前向声明，避免交叉包含

class ImageWorker : public QObject
{
    Q_OBJECT
public:
    explicit ImageWorker(ImageController *ctrl, QObject *parent = nullptr);

public slots:
    // 核心工作槽函数，将在后台线程的事件循环中被调用
    void processAll();
    void processAllTime();

signals:
    // 异步抛出进度和状态信息
    void progressUpdated(int percent, const QString &stepName);
    void finished();

private:
    ImageController *controller;
};

#endif // IMAGE_WORKER_H
