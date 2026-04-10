#include "triangle_color_dialog.h"
#include "ui_triangle_color_dialog.h" // 你自己去 Qt Designer 画这个 UI

TriangleColorDialog::TriangleColorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TriangleColorDialog),
    currentTriIndex(-1)
{
    ui->setupUi(this);

    // 核心设置: 使窗口成为非模态工具面板，可以一直悬浮，不阻塞主窗口
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(false);

    // 在 Qt Designer 里创建了以下控件：
    // 3个 QSpinBox: spinBoxR, spinBoxG, spinBoxB (取值范围设为 0-255)
    // 2个 QPushButton: confirmButton, resetButton
    // 1个 QLabel: colorPreviewLabel (用来展示颜色块)

    // 连接 RGB 调整信号 (注意 QSpinBox 的 valueChanged 有重载，需要用 QOverload 显式指定)
    connect(ui->spinBoxR, QOverload<int>::of(&QSpinBox::valueChanged), this, &TriangleColorDialog::onColorChanged);
    connect(ui->spinBoxG, QOverload<int>::of(&QSpinBox::valueChanged), this, &TriangleColorDialog::onColorChanged);
    connect(ui->spinBoxB, QOverload<int>::of(&QSpinBox::valueChanged), this, &TriangleColorDialog::onColorChanged);

    // 连接按钮
    connect(ui->confirmButton, &QPushButton::clicked, this, &TriangleColorDialog::onConfirmClicked);
    connect(ui->resetButton, &QPushButton::clicked, this, &TriangleColorDialog::onResetClicked);
}

TriangleColorDialog::~TriangleColorDialog()
{
    delete ui;
}

void TriangleColorDialog::closeEvent(QCloseEvent *event)
{
    emit dialogClosed(); // 发出信号
    QDialog::closeEvent(event); // 走原本的关闭逻辑
}

void TriangleColorDialog::setTarget(int triIndex, const QColor& color)
{
    currentTriIndex = triIndex;
    originalColor = color;
    currentColor = color;

    // 阻止信号发射，防止代码 setValue 时触发 onColorChanged 导致二次调用
    const QSignalBlocker blockerR(ui->spinBoxR);
    const QSignalBlocker blockerG(ui->spinBoxG);
    const QSignalBlocker blockerB(ui->spinBoxB);

    ui->spinBoxR->setValue(color.red());
    ui->spinBoxG->setValue(color.green());
    ui->spinBoxB->setValue(color.blue());

    updateColorPreviewBlock(color);

    // 更新标题提示
    setWindowTitle(tr("Edit Triangle #%1").arg(triIndex));
}

void TriangleColorDialog::onColorChanged()
{
    // 获取 UI 上的最新 RGB 值
    currentColor.setRgb(ui->spinBoxR->value(), ui->spinBoxG->value(), ui->spinBoxB->value());

    // 更新对话框自己的颜色块
    updateColorPreviewBlock(currentColor);

    // 【进阶玩法】：如果解开下面这行注释，并且在 Widget 里将其连接到 Controller，
    // 就可以实现“拖动数值，主图上的三角形实时跟着变色”的效果！
    // emit colorPreview(currentTriIndex, currentColor);
}

void TriangleColorDialog::updateColorPreviewBlock(const QColor& color)
{
    // 利用 Qt 的 StyleSheet 动态给 QLabel 刷背景色来做颜色预览
    if (ui->colorPreviewLabel) {
        QString styleSheet = QString("background-color: rgb(%1, %2, %3); border: 1px solid #ccc; border-radius: 4px;")
        .arg(color.red())
            .arg(color.green())
            .arg(color.blue());
        ui->colorPreviewLabel->setStyleSheet(styleSheet);
    }
}

void TriangleColorDialog::onConfirmClicked()
{
    if (currentTriIndex != -1) {
        // 通知 ImageController 正式提交颜色修改
        emit colorConfirmed(currentTriIndex, currentColor);
    }
}

void TriangleColorDialog::onResetClicked()
{
    if (currentTriIndex != -1) {
        // 重置为原本的颜色，这会连带更新 UI 数字和颜色预览块
        setTarget(currentTriIndex, originalColor);

        // 如果上面你开启了实时预览，重置的时候也要发个信号让主图恢复回去
        // emit colorPreview(currentTriIndex, originalColor);
    }
}
