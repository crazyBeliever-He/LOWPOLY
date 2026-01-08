#include "widget.h"
#include "ui_widget.h"

#include <QMenuBar>
#include <QMessageBox>
#include <QMetaType>

#include "image_widget.h"       // 图像显示
#include "image_model.h"        // 数据
#include "image_controller.h"   // 业务逻辑
#include "algorithm_params.h"       // 业务需要的结构体
#include "edge_drawing_dialog.h"    // edge drawing 算法参数窗口
#include "douglas_peucker_dialog.h" // douglas peucker 算法参数窗口

// 实现依赖（Implementation Dependency）放在 .cpp

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mainWidget)
{
    ui->setupUi(this);

    // image controller, model, widget
    ImageWidget *imageWidget = new ImageWidget(this->ui->middleWidget);
    imageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->middleWidget->layout()->addWidget(imageWidget);
    ImageModel *imageModel = new ImageModel();
    imageController = new ImageController(imageModel, imageWidget, this);

    // init widgets
    initMenuWidget();
    initContentWidget();
    initStatusWidget();
}

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
    initSettingBtnInMenu();

    //about menu
    QMenu *aboutMenu = new QMenu(this->ui->aboutButton);
    ui->aboutButton->setMenu(aboutMenu);
    QAction *aboutProgramAction = aboutMenu->addAction(tr("Program"));
    QAction *aboutAuthorAction = aboutMenu->addAction(tr("Auther"));
    connect(aboutProgramAction, &QAction::triggered, this, &Widget::showAboutProgram);
    connect(aboutAuthorAction, &QAction::triggered, this, &Widget::showAboutAuthor);
}

void Widget::initSettingBtnInMenu()
{
    QMenu *settingMenu = new QMenu(this->ui->settingButton);
    ui->settingButton->setMenu(settingMenu);

    // 1. 图像自适应窗口大小
    QAction *autoSizeAction = settingMenu->addAction(tr("Auto Size"));
    autoSizeAction->setCheckable(true);
    connect(autoSizeAction, &QAction::toggled, this, [this](bool checked) {
        imageController->onAutoSize(checked);
    });
    autoSizeAction->setChecked(true);

    // 2. Edge drawing 参数设置
    settingMenu->addAction(tr("Edge Drawing Settings"), this, [this](){
        openDialog<opencved::EDParams>(tr("Edge Drawin Params"));
    });

    // 3. douglas peucker 参数设置
    QAction *dpParamAction = settingMenu->addAction(tr("Douglas Peucker Settings"));
    connect(dpParamAction, &QAction::triggered, this, [this](){
        openDialog<DPParams>(tr("Douglas Peucker Params"));
    });
}

template<typename T>
void Widget::openDialog(const QString& title)
{
    QString typeName = QMetaType::fromType<T>().name();

    // 1. 懒加载：如果没创建过，则根据类型创建
    if (!dialogMap.contains(typeName))
    {
        BaseDialog* dialog = nullptr;
        if (typeName == "opencved::EDParams") dialog = new EDParamDialog(this);
        else if (typeName == "DPParams") dialog = new DPParamDialog(this);

        if (dialog)
        {
            dialog->setWindowTitle(title);
            dialogMap[typeName] = dialog;
            // 连接通路 1
            connect(dialog, &BaseDialog::dataSubmitted, this, &Widget::handleDialogData);
        }
    }

    // 2. 从 Controller 获取当前最新数据并存入 Dialog
    BaseDialog* dialog = dialogMap[typeName];
    if (typeName == "opencved::EDParams")
        dialog->setData(QVariant::fromValue(imageController->getEDParams()));
    else if (typeName == "DPParams")
        dialog->setData(QVariant::fromValue(imageController->getDPParams()));

    dialog->show();
    dialog->raise();
}

void Widget::handleDialogData(const QVariant &data)
{
    // 根据 data 存储的实际类型进行分发
    if (data.canConvert<opencved::EDParams>())
    {
        imageController->setEDParams(data.value<opencved::EDParams>());
    }
    else if (data.canConvert<DPParams>())
    {
        imageController->setDPParams(data.value<DPParams>());
    }
}

void Widget::initContentWidget()
{
    //判断字符串"源数据"太暴力,判断数字（0, 1, 2）又容易乱
    //可以给 ComboBox 的每一项绑定一个自定义数据.这样无论文字怎么变,逻辑都不会断
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
