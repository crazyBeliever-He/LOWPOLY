#include "widget.h"
#include "./ui_widget.h"

#include <QMenuBar>
#include <QMessageBox>

#include "image_widget.h"       // 图像显示
#include "image_model.h"        // 数据
#include "image_controller.h"   // 业务逻辑
//#include "logger.h"           // 日志

// 实现依赖（Implementation Dependency）放在 .cpp

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mainWidget)
{
    ui->setupUi(this);

    ImageWidget *imageWidget = new ImageWidget(this->ui->middleWidget);
    imageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->middleWidget->layout()->addWidget(imageWidget);

    ImageModel *imageModel = new ImageModel();
    imageController = new ImageController(imageModel, imageWidget, this);

    initMenuWidget();
    initToolWidget();
    initStatusWidget();
}

/*初始化界面组件*/
void Widget::initMenuWidget()
{
    //file menu
    QMenu *fileMenu = new QMenu(this->ui->fileButton);
    ui->fileButton->setMenu(fileMenu);
    QAction *openImageAction = fileMenu->addAction("Open");
    QAction *saveImageAction = fileMenu->addAction("Save");
    connect(openImageAction, &QAction::triggered, this, [this]() {
        imageController->openImageWithDialog(this);
    });
    connect(saveImageAction, &QAction::triggered, this, [this]() {
        imageController->saveImageWithDialog(this);
    });

    //setting menu
    initSettingBtnInMenuWidget();

    //about menu
    QMenu *aboutMenu = new QMenu(this->ui->aboutButton);
    ui->aboutButton->setMenu(aboutMenu);
    QAction *aboutProgramAction = aboutMenu->addAction("Program");
    QAction *aboutAuthorAction = aboutMenu->addAction("Auther");
    connect(aboutProgramAction, &QAction::triggered, this, &Widget::showAboutProgram);
    connect(aboutAuthorAction, &QAction::triggered, this, &Widget::showAboutAuthor);
}

void Widget::initSettingBtnInMenuWidget()
{
    // 图像自适应窗口大小
    QMenu *settingMenu = new QMenu(this->ui->settingButton);
    ui->settingButton->setMenu(settingMenu);
    QAction *autoSizeAction = settingMenu->addAction("Auto Size");
    autoSizeAction->setCheckable(true);
    connect(autoSizeAction, &QAction::toggled, this, [this](bool checked) {
        imageController->onAutoSize(checked);
    });
    autoSizeAction->setChecked(true);

    //
}

void Widget::initToolWidget()
{
    connect(ui->comboBox, &QComboBox::currentIndexChanged,
            imageController, &ImageController::onImageTypeSelected);
}
void Widget::initStatusWidget()
{
    // 连接报错和状态信息
    connect(imageController, &ImageController::errorOccurred, this, &Widget::showErrorMessage);
    connect(imageController, &ImageController::statusMessage, this, &Widget::showStatusMessage);

}

void Widget::showErrorMessage(const QString &msg)
{
    QMessageBox::warning(this, "Error", msg);
}

void Widget::showStatusMessage(const QString &msg)
{
    ui->statusLabel->setText(msg);
}

void Widget::showAboutProgram()
{
    QMessageBox::information(this,
                             "About Program",
                             "This program is a demo for image processing.\n"
                             "Version 0.1");
}

void Widget::showAboutAuthor()
{
    // 带有超链接的HTML内容
    QString aboutText = "<div style=\"text-align: center;\">"
                        "Program developed by: crazyBeliever_he<br>"
                        "<a href=\"https://github.com/crazyBeliever-He\">Auther GitHub</a>.<br>"
                        "</div>";

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("About Auther");
    msgBox.setText(aboutText);
    // 设置消息框的文本格式为 RichText，这样可以支持 HTML 格式
    msgBox.setTextFormat(Qt::RichText);
    msgBox.exec();

}

Widget::~Widget()
{
    delete ui;
    delete imageController;
}
