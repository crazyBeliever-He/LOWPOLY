#include <QMessageBox>

#include "ed_param_dialog.h"
#include "ui_ed_param_dialog.h"
#include "EDParams_util.h"

EdParamDialog::EdParamDialog(QWidget *parent)
    : BaseDialog(parent)
    , ui(new Ui::EDParamDialog)
{
    ui->setupUi(this);
    // 设置窗口属性: 关闭时不销毁, 只是隐藏(实现复用)
    this->setAttribute(Qt::WA_QuitOnClose, false);
    connect(ui->confirmButton, &QPushButton::clicked, this, &EdParamDialog::onConfirmButtonClicked);
    connect(ui->resetButton, &QPushButton::clicked, this, [this]() {
        setData(QVariant::fromValue(EDParamsUtil::getDefaultParams()));
    });
}
QVariant EdParamDialog::getData()
{
    // 将 UI 控件的值读入结构体
    //opencved::EDParams edParams;
    edParams.PFmode = ui->PFmode->isChecked();
    edParams.EdgeDetectionOperator = ui->EdgeDetectionOperator->currentIndex();
    edParams.GradientThresholdValue = ui->GradientThresholdValue->value();
    edParams.AnchorThresholdValue = ui->AnchorThresholdValue->value();
    edParams.ScanInterval = ui->ScanInterval->value();
    edParams.MinPathLength = ui->MinPathLength->value();
    edParams.Sigma = static_cast<float>(ui->Sigma->value());
    edParams.SumFlag = ui->SumFlag->isChecked();
    edParams.NFAValidation = ui->NFAValidation->isChecked();
    edParams.MinLineLength = ui->MinLineLength->value();
    edParams.MaxDistanceBetweenTwoLines = ui->MaxDistanceBetweenTwoLines->value();
    edParams.LineFitErrorThreshold = ui->LineFitErrorThreshold->value();
    edParams.MaxErrorThreshold = ui->MaxErrorThreshold->value();
    return QVariant::fromValue(edParams);
}

void EdParamDialog::setData(const QVariant &data)
{
    if (!data.canConvert<opencved::EDParams>()) return;
    edParams = data.value<opencved::EDParams>();
    // 阻止信号发射，防止更新 UI 时触发不必要的事件逻辑
    const bool blocked = this->blockSignals(true);
    // qDebug() << "PFmode:" << edParams.PFmode;
    ui->PFmode->setChecked(edParams.PFmode);
    ui->EdgeDetectionOperator->setCurrentIndex(edParams.EdgeDetectionOperator);
    ui->GradientThresholdValue->setValue(edParams.GradientThresholdValue);
    ui->AnchorThresholdValue->setValue(edParams.AnchorThresholdValue);
    ui->ScanInterval->setValue(edParams.ScanInterval);
    ui->MinPathLength->setValue(edParams.MinPathLength);
    ui->Sigma->setValue(static_cast<double>(edParams.Sigma));
    ui->SumFlag->setChecked(edParams.SumFlag);
    ui->NFAValidation->setChecked(edParams.NFAValidation);
    ui->MinLineLength->setValue(edParams.MinLineLength);
    ui->MaxDistanceBetweenTwoLines->setValue(edParams.MaxDistanceBetweenTwoLines);
    ui->LineFitErrorThreshold->setValue(edParams.LineFitErrorThreshold);
    ui->MaxErrorThreshold->setValue(edParams.MaxErrorThreshold);
    this->blockSignals(blocked);
}

void EdParamDialog::onConfirmButtonClicked()
{
    // 此处可以加入数据校验逻辑
    QString validationMsg;
    opencved::EDParams currentParams = this->getData().value<opencved::EDParams>();
    bool isValid = EDParamsUtil::validateParams(currentParams, validationMsg);
    // 3. 处理校验结果
    if (!isValid) {
        // --- 情况 A: 存在致命错误 ---
        // 使用 critical 错误图标，用户必须修改，不发出信号
        QMessageBox::critical(this,
                              QCoreApplication::translate("EdParamDialog", "Parameter Error"),
                              validationMsg);

        ui->label_Status->setText(tr("Status: Error. Please fix parameters."));
        ui->label_Status->setStyleSheet("color: red;");
        return;
    }
    else if (!validationMsg.isEmpty()) {
        // --- 情况 B: 校验通过，但有警告或算法修正提示 ---
        // 使用 warning 警告图标，询问用户是否坚持提交
        validationMsg += "\n" + QCoreApplication::translate("EdParamDialog", "Do you want to continue?");

        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(this,
                                     QCoreApplication::translate("EdParamDialog", "Parameter Warning"),
                                     validationMsg,
                                     QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No) {
            ui->label_Status->setText(tr("Status: Canceled by user."));
            ui->label_Status->setStyleSheet("color: orange;");
            return; // 用户选择返回修改
        }
    }
    // 发出数据，主窗口收到后会传给 Controller
    emit dataSubmitted(this->getData());
}

void EdParamDialog::onSubmissionResult(bool success, const QString &message)
{
    // 处理主窗口反馈的结果
}

EdParamDialog::~EdParamDialog()
{
    delete ui;
}

