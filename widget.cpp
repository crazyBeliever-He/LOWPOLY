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

    // 连接报错和状态的信号和槽
    connect(imageController, &ImageController::errorOccurred,
            this, &Widget::showErrorMessage);
    connect(imageController, &ImageController::statusMessage,
            this, &Widget::showStatusMessage);

    initMenus();
    initToolWidget();
}

/*初始化界面组件*/
void Widget::initMenus()
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
    //help menu
    QMenu *helpMenu = new QMenu(this->ui->helpButton);
    ui->helpButton->setMenu(helpMenu);
    QAction *guideAction = helpMenu->addAction("Guide");
    QAction *defaultSettingsAction = helpMenu->addAction("DefaultSettings");
    // Settings with checkable options
    QAction *settingAAction = helpMenu->addAction("SettingA");
    settingAAction->setCheckable(true); // Make it checkable
    settingAAction->setChecked(true);   // Set the initial state (checked or unchecked)

    QAction *settingBAction = helpMenu->addAction("SettingB");
    settingBAction->setCheckable(true); // Make it checkable
    settingBAction->setChecked(false);  // Set the initial state (checked or unchecked)

    //about menu
    QMenu *settingMenu = new QMenu(this->ui->aboutButton);
    ui->aboutButton->setMenu(settingMenu);
    QAction *aboutProgramAction = settingMenu->addAction("Program");
    QAction *aboutAuthorAction = settingMenu->addAction("Auther");
    connect(aboutProgramAction, &QAction::triggered, this, &Widget::showAboutProgram);
    connect(aboutAuthorAction, &QAction::triggered, this, &Widget::showAboutAuthor);
}

void Widget::initToolWidget()
{
    connect(ui->comboBox, &QComboBox::currentIndexChanged,
            imageController, &ImageController::onImageTypeSelected);
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
                             "Version 1.0");
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
}
