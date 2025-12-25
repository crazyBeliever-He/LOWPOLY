#include "widget.h"
#include "ui_widget.h"

#include <QMenuBar>
#include <QMessageBox>
#include <QMetaType>

#include "edge_drawing_lib.h"

#include "image_widget.h"       // 图像显示
#include "image_model.h"        // 数据
#include "image_controller.h"   // 业务逻辑
#include "ed_param_dialog.h"    // edge drawing 算法参数窗口

// 实现依赖（Implementation Dependency）放在 .cpp

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mainWidget)
{
    ui->setupUi(this);

    // 注册结构体到Qt元对象系统
    qRegisterMetaType<opencved::EDParams>("EDParams");

    // image controller, model, widget
    ImageWidget *imageWidget = new ImageWidget(this->ui->middleWidget);
    imageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->middleWidget->layout()->addWidget(imageWidget);

    ImageModel *imageModel = new ImageModel();
    imageController = new ImageController(imageModel, imageWidget, this);

    // main widget
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
    QAction *openImageAction = fileMenu->addAction(tr("Open"));
    QAction *saveImageAction = fileMenu->addAction(tr("Save"));
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
    QAction *aboutProgramAction = aboutMenu->addAction(tr("Program"));
    QAction *aboutAuthorAction = aboutMenu->addAction(tr("Auther"));
    connect(aboutProgramAction, &QAction::triggered, this, &Widget::showAboutProgram);
    connect(aboutAuthorAction, &QAction::triggered, this, &Widget::showAboutAuthor);
}

void Widget::initSettingBtnInMenuWidget()
{
    QMenu *settingMenu = new QMenu(this->ui->settingButton);
    ui->settingButton->setMenu(settingMenu);

    // 图像自适应窗口大小
    QAction *autoSizeAction = settingMenu->addAction(tr("Auto Size"));
    autoSizeAction->setCheckable(true);
    connect(autoSizeAction, &QAction::toggled, this, [this](bool checked) {
        imageController->onAutoSize(checked);
    });
    autoSizeAction->setChecked(true);

    //Edge drawing 参数设置
    edParamDialog = new EdParamDialog(this);
    connect(edParamDialog, &BaseDialog::dataSubmitted, this, [this](const QVariant &data) {
        imageController->setEDParams(data.value<opencved::EDParams>());
    });

    QAction *edParamAction = settingMenu->addAction(tr("Edge Drawing Params"));
    connect(edParamAction, &QAction::triggered, this, [this]() {
        if (edParamDialog) {
            // 将主窗口当前存储的数据同步给子窗口, 显示窗口
            edParamDialog->setData(QVariant::fromValue(imageController->edParams));
            edParamDialog->show();
            edParamDialog->raise();
        }
    });

}

void Widget::initToolWidget()
{
    connect(ui->comboBox, &QComboBox::currentIndexChanged,
            imageController, &ImageController::onImageTypeSelected);
    // 如果有其他因为切换源图需要同步的ui组件,可以把“同步 UI 状态”写成一个成员函数, 目前就ui->comboBox需要同步
    connect(imageController, &ImageController::requestUiReset, this, [this](){
        QSignalBlocker blocker(ui->comboBox);
        ui->comboBox->setCurrentIndex(0);
    });
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
