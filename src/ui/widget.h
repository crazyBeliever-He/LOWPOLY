#ifndef WIDGET_H
#define WIDGET_H

#include <QMap>
#include <QWidget>

#include "base_dialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {      // 命名空间
class mainWidget;       // ui_widget.h文件里面定义的类，前向声明
}
QT_END_NAMESPACE

// 类型依赖(Type Dependency)放在.h 加快编译速度，降低模块耦合，隐藏实现细节
// 前向声明，减少头文件依赖
class ImageController;
class EDParamDialog;

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
    EDParamDialog *edParamDialog;

private:
    void initMenuWidget();
    void initContentWidget();
    void initStatusWidget();
    void initSettingBtnInMenu();

private:
    // 存储已经创建过的 Dialog，实现单例复用
    QMap<QString, BaseDialog*> dialogMap;
    // 处理所有 Dialog 提交的数据 (通路 1) 将数据从Dialog写回Controller
    void handleDialogData(const QVariant &data);
    // 打开并同步数据的通用函数 (通路 2) 从Controller取数据并同步给Dialog
    template<typename T>
    void openDialog(const QString& title);


private slots:
    void showAboutProgram();
    void showAboutAuthor();

};

#endif // WIDGET_H
