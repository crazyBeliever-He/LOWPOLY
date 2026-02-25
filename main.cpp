#include "widget.h"

#include <QApplication>
#include <QTranslator>
#include <QMessageBox>

#include "logger.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString translation = "";
    // QTranslator translator;
    // const QStringList uiLanguages = QLocale::system().uiLanguages();
    // for (const QString &locale : uiLanguages)
    // {
    //     const QString baseName = "TestLowPoly04_" + QLocale(locale).name();
    //     if (translator.load(":/i18n/translation/" + baseName + ".qm"))
    //     {   //i18n 是 internationalization
    //         a.installTranslator(&translator);
    //         translation = baseName;
    //         break;
    //     }
    // }

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
    LOG_INFO << "Application started. Traslation: " << translation;

    int result = a.exec();
    logger::Logger::destroy();
    return result;
}
