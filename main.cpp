#include "widget.h"

#include <QApplication>
#include <QTranslator>
#include <QMessageBox>

#include "logger.h"
void test();

/********************************************************************************/
// Gai M, Wang G.
// Artistic low poly rendering for images[J].
// The visual computer, 2016, 32(4): 491-500.
/********************************************************************************/

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

    //test();

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

void test()
{

    struct SDParams1 {  // 原始版本
        int type = 0;
        int userN = 0;
        double lambda = 0.7;
        uint8_t threshold = 128;
    };

    struct SDParams2 {  // 优化版本
        double lambda = 0.7;
        uint8_t threshold = 128;
        int type = 0;
        int userN = 0;
    };

    struct SDParams3 {  // 分组版本
        double lambda = 0.7;
        int type = 0;
        int userN = 0;
        uint8_t threshold = 128;
    };

        //QString tempDir = QDir::tempPath(); // 系统临时目录 (如 C:\Users\xxx\AppData\Local\Temp)
        //qDebug()<< " tempDir"<< tempDir;
        qDebug() << "sizeof(SDParams1) = " << sizeof(SDParams1) << " bytes\n";
        qDebug() << "sizeof(SDParams2) = " << sizeof(SDParams2) << " bytes\n";
        qDebug() << "sizeof(SDParams3) = " << sizeof(SDParams3) << " bytes\n";

        qDebug() << "\n偏移量:\n";
        qDebug() << "SDParams2::lambda   偏移: " << offsetof(SDParams2, lambda) << "\n";
        qDebug() << "SDParams2::threshold偏移: " << offsetof(SDParams2, threshold) << "\n";
        qDebug() << "SDParams2::type     偏移: " << offsetof(SDParams2, type) << "\n";
        qDebug() << "SDParams2::userN    偏移: " << offsetof(SDParams2, userN) << "\n";

}
