#include "widget.h"
#include <QApplication>
#include "logger.h"
#include <QMessageBox>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    try {
        logger::Logger::init();
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(nullptr,
                              "日志初始化失败",
                              QString("无法初始化日志系统: %1\n程序将退出。").arg(e.what()));
        return 1;
    } catch (std::exception& e) {
        QMessageBox::critical(nullptr,
                              "日志初始化失败",
                              QString("未知错误: %1\n程序将退出。").arg(e.what()));
        return 1;
    }

    Widget w;
    w.show();
    LOG_INFO << "Application started";

    int result = a.exec();
    logger::Logger::destroy();
    return result;
}
