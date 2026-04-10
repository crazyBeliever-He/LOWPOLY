#include "widget.h"
#include "ui_widget.h"

#include <QMenuBar>
#include <QMessageBox>
#include <QMetaType>

#include "logger.h"
#include "image_widget.h"       // 图像显示
#include "image_model.h"        // 数据
#include "image_controller.h"   // 业务逻辑
#include "algorithm_params.h"       // 业务需要的结构体
#include "dialogs_factory.h"
#include "triangle_color_dialog.h"

// 实现依赖（Implementation Dependency）放在 .cpp

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mainWidget)
{
    ui->setupUi(this);

    // 1. ImageWidget: 由 Qt 父子树管理 (middleWidget 负责释放)
    ImageWidget *imageWidget = new ImageWidget(this->ui->middleWidget);
    imageWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->middleWidget->layout()->addWidget(imageWidget);

    // 2. ImageModel: 指定一个父对象 Widget. 当 Widget 销毁时,imageModel 也会被自动释放.
    ImageModel *imageModel = new ImageModel(this);
    // 3. ImageController 所有权归属: Widget. 释放机制: Qt 对象树 (Parent-Child).
    imageController = new ImageController(imageModel, imageWidget, this);

    // init widgets
    initMenuWidget();
    initContentWidget();
    initStatusWidget();

    // 实例化非模态颜色调节面板
    triangleColorDialog = new TriangleColorDialog(this);
    initInteractiveConnections();
}

void Widget::initMenuWidget()
{
    //file menu
    QMenu *fileMenu = new QMenu(this->ui->fileButton);
    ui->fileButton->setMenu(fileMenu);
    QAction *openImageAction = fileMenu->addAction(tr("Open"));
    QAction *saveImageAction = fileMenu->addAction(tr("Save"));
    connect(openImageAction, &QAction::triggered, this, [this]() {
        imageController->onOpenImage(this);
    });
    connect(saveImageAction, &QAction::triggered, this, [this]() {
        imageController->onSaveImage(this);
    });

    //setting menu
    initSettingBtnInMenu();

    //about menu
    QMenu *aboutMenu = new QMenu(this->ui->aboutButton);
    ui->aboutButton->setMenu(aboutMenu);
    QAction *aboutProgramAction = aboutMenu->addAction(tr("Program"));
    QAction *aboutAuthorAction = aboutMenu->addAction(tr("Auther"));
    connect(aboutProgramAction, &QAction::triggered, this, &Widget::onShowAboutProgram);
    connect(aboutAuthorAction, &QAction::triggered, this, &Widget::onShowAboutAuthor);
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

    // 4. saliency detection 参数设置
    QAction *sdParamAction = settingMenu->addAction(tr("Saliency Detection Settings"));
    connect(sdParamAction, &QAction::triggered, this, [this](){
        openDialog<SDParams>(tr("Saliency Detection Params"));
    });
}

template<typename T>
void Widget::openDialog(const QString& title)
{
    QString typeName = QMetaType::fromType<T>().name();

    // 1. 懒加载：如果没创建过，则根据类型创建
    if (!dialogMap.contains(typeName))
    {
        BaseDialog* dialog = DialogFactory::instance().createDialog(typeName, this);
        if (dialog)
        {
            dialog->setWindowTitle(title);
            dialogMap[typeName] = dialog;
            // 闭包捕获 typeName，盲目地将 Dialog 提交的 QVariant 塞给 Controller
            connect(dialog, &BaseDialog::dataSubmitted, this, [this, typeName](const QVariant& data) {
                imageController->setParams(typeName, data);
            });
        }
        else
        {
            LOG_ERROR << typeName << " Not Registered" ;
            return;
        }
    }

    // 2. 从 Controller 获取当前最新数据并存入 Dialog
    BaseDialog* dialog = dialogMap[typeName];
    dialog->setData(imageController->getParams(typeName));

    dialog->show();
    dialog->raise();
}

void Widget::initContentWidget()
{
    //判断字符串"源数据"太暴力,判断数字 0, 1, 2 又容易乱
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
    connect(imageController, &ImageController::errorOccurred, this, &Widget::onShowErrorMessage);
    connect(imageController, &ImageController::statusMessage, this, &Widget::onShowStatusMessage);

    // Widget 监听 ImageController 的信号, 然后根据 typeName 去 dialogMap 里找对应的窗口,把结果塞进去
    connect(imageController, &ImageController::paramsApplyFinished,
            this, [this](const QString& typeName, bool success, const QString& msg) {
                // 如果字典里有这个类型的 Dialog 实例
                if (dialogMap.contains(typeName)) {
                    BaseDialog* dialog = dialogMap[typeName];
                    // 把结果传递给 Dialog
                    dialog->receiveSubmissionFeedback(success, msg);

                    // 可选：如果成功了, 可以在这里统一决定是否自动隐藏 Dialog
                    // if (success) dialog->hide();
                }
            });


    // 接收进度并更新 UI
    connect(imageController, &ImageController::progressUpdated,
            this, [this](int percent, const QString &stepName) {

                // 如果有进度条组件：
                // ui->progressBar->setValue(percent);

                // 直接将进度格式化到现有的 statusLabel 里：
                QString statusText = QString("%1 (%2%)").arg(stepName).arg(percent);
                ui->statusLabel->setText(statusText);
            });

    // 接收处理完成信号，恢复界面
    connect(imageController, &ImageController::processFinished,
            this, [this]() {
                ui->statusLabel->setText(tr("Ready"));
                // 如果在处理前禁用了某些按钮，可以在这里重新启用
            });
}

void Widget::onShowErrorMessage(const QString &msg)
{
    QMessageBox::warning(this, "Error", msg);
}

void Widget::onShowStatusMessage(const QString &msg)
{
    ui->statusLabel->setText(msg);
}

void Widget::onShowAboutProgram()
{//Qt 提供的静态函数 QMessageBox::about
    // 1. 使用 static 函数，一行代码搞定创建和销毁。
    // 2. 使用 HTML 标签 <h3> 加粗加大标题，<p> 控制段落。
    // 3. style='color: gray;' 让版本号这种次要信息稍微淡一点，增加层次感。
    QString content = R"(
        <div style='text-align: center;'>
            <h3>Image Processing Demo</h3>
            <p>A lightweight tool for image manipulation.</p>
            <p style='color: gray; font-size: 12px;'>Version 0.1</p>
        </div>
    )";
    QMessageBox::about(this, tr("About Program"), content);
}

void Widget::onShowAboutAuthor()
{//C++11 的 Raw String Literal (R"(...)"), 好用,多用
    QString content = R"(
        <div style='text-align: center;'>
            <p>Program developed by:</p>
            <h3>crazyBeliever_he</h3>
            <p><a href='https://github.com/crazyBeliever-He' style='text-decoration: none; color: #0078d7;'>Visit GitHub Repository</a></p>
        </div>
    )";
    QMessageBox::about(this, tr("About Author"), content);
}

// 专门用于处理交互式面板的信号连接
void Widget::initInteractiveConnections()
{

    connect(triangleColorDialog, &TriangleColorDialog::dialogClosed,
            imageController, &ImageController::clearTriangleSelection);

    // 通路 1: Controller 通知 Widget 弹窗并更新数据
    // 当用户在画布上按住 Ctrl+左键 选中三角形时触发
    connect(imageController, &ImageController::openColorDialogRequested,
            this, [this](int triIndex, QColor color) {
                // 更新面板里的目标索引和颜色
                triangleColorDialog->setTarget(triIndex, color);

                // 如果是因为切换视图回来的，就把位置重置到主窗口中心
                if (this->needResetColorDialogPos)
                {
                    // 计算居中位置 (相对于 Widget 所在的整个主窗口)
                    QPoint centerPos = this->window()->geometry().center() - triangleColorDialog->rect().center();
                    triangleColorDialog->move(centerPos);
                    this->needResetColorDialogPos = false; // 消费掉标志位
                }

                // 显示面板
                triangleColorDialog->show();
                // 确保面板在最上层
                triangleColorDialog->raise();
            });

    // 通路 2: 面板通知 Controller 提交颜色修改
    // 当用户在面板上点击 Confirm 按钮时触发
    connect(triangleColorDialog, &TriangleColorDialog::colorConfirmed,
            imageController, &ImageController::updateTriangleColor);

    // (可选) 通路 3: 如果你在面板里实现了实时预览的信号 colorPreview，可以在这里接上
    // connect(triangleColorDialog, &TriangleColorDialog::colorPreview,
    //         imageController, &ImageController::updateTriangleColor);

    // 监听 Controller 发来的关闭请求
    connect(imageController, &ImageController::closeColorDialogRequested, this, [this]() {
        // 安全检查：指针有效且面板当前是显示状态，才执行隐藏
        if (this->triangleColorDialog && this->triangleColorDialog->isVisible()) {
            this->triangleColorDialog->hide(); // 只是隐藏（hide），不销毁（delete），下次切回 Test 还能继续复用
        }
        // 只要切了视图，就标记下次弹窗需要重置位置
        this->needResetColorDialogPos = true;
    });
}

Widget::~Widget()
{
    delete ui;
}
