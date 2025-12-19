#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {      // 命名空间
class mainWidget;   // ui_widget.h文件里面定义的类，前向声明
}
QT_END_NAMESPACE

// 类型依赖(Type Dependency)放在.h 加快编译速度，降低模块耦合，隐藏实现细节
// 前向声明，减少头文件依赖
class ImageController;

class Widget : public QWidget
{
    Q_OBJECT    //宏，使用Qt信号与槽机制必备的

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

public slots:
    void showErrorMessage(const QString &msg);
    void showStatusMessage(const QString &msg);

private:
    Ui::mainWidget *ui;
    ImageController *imageController;

private:
    void initMenus();
    void initToolWidget();

private slots:

    void showAboutProgram();
    void showAboutAuthor();

};

#endif // WIDGET_H
