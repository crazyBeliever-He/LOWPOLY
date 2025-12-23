#include "widget.h"
#include <QApplication>
#include <QTranslator>
#include "logger.h"
#include <QMessageBox>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "TestLowPoly04_" + QLocale(locale).name();
        LOG_INFO << "Translation:" + baseName;
        if (translator.load(":/translation/" + baseName + ".qm")) {
            a.installTranslator(&translator);
            break;
        }
    }

    try {
        logger::Logger::init();
    } catch (const std::runtime_error& e) {
        QString errorMessage = QObject::tr("Failed to initialize log system: %1\nThe program will exit.").arg(e.what());
        QMessageBox::critical(nullptr,
                              QObject::tr("Logger Initialization Failed"),
                              errorMessage);
        return 1;
    } catch (std::exception& e) {
        QString errorMessage = QObject::tr("Unknown error: %1\nThe program will exit.").arg(e.what());
        QMessageBox::critical(nullptr,
                              QObject::tr("Logger Initialization Failed"),
                              errorMessage);
        return 1;
    }
    Widget w;
    w.show();
    LOG_INFO << "Application started";

    int result = a.exec();
    logger::Logger::destroy();
    return result;
}
