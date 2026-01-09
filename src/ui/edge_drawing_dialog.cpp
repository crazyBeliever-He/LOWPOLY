#include "edge_drawing_dialog.h"
#include "ui_edge_drawing_dialog.h"

#include "edge_drawing_lib.h"
#include "params_util.h"

EDParamDialog::EDParamDialog(QWidget *parent)
    : BaseDialog(parent)
    , ui(new Ui::EDParamDialog)
{
    ui->setupUi(this);
    initDialogConnections();
}

QVariant EDParamDialog::getData()
{
    opencved::EDParams p;
    p.PFmode = ui->PFmode->isChecked();
    p.EdgeDetectionOperator = ui->EdgeDetectionOperator->currentIndex();
    p.GradientThresholdValue = ui->GradientThresholdValue->value();
    p.AnchorThresholdValue = ui->AnchorThresholdValue->value();
    p.ScanInterval = ui->ScanInterval->value();
    p.MinPathLength = ui->MinPathLength->value();
    p.Sigma = static_cast<float>(ui->Sigma->value());
    p.SumFlag = ui->SumFlag->isChecked();
    p.NFAValidation = ui->NFAValidation->isChecked();
    p.MinLineLength = ui->MinLineLength->value();
    p.MaxDistanceBetweenTwoLines = ui->MaxDistanceBetweenTwoLines->value();
    p.LineFitErrorThreshold = ui->LineFitErrorThreshold->value();
    p.MaxErrorThreshold = ui->MaxErrorThreshold->value();
    return QVariant::fromValue(p);
}

void EDParamDialog::setData(const QVariant &data)
{
    if (!data.canConvert<opencved::EDParams>()) return;
    opencved::EDParams p = data.value<opencved::EDParams>();
    // 使用 QSignalBlocker 阻止信号发射, 阻止整个 dialog 的信号发射, 防止更新 UI 时触发不必要的事件逻辑
    QSignalBlocker blocker(this);
    ui->PFmode->setChecked(p.PFmode);
    ui->EdgeDetectionOperator->setCurrentIndex(p.EdgeDetectionOperator);
    ui->GradientThresholdValue->setValue(p.GradientThresholdValue);
    ui->AnchorThresholdValue->setValue(p.AnchorThresholdValue);
    ui->ScanInterval->setValue(p.ScanInterval);
    ui->MinPathLength->setValue(p.MinPathLength);
    ui->Sigma->setValue(static_cast<double>(p.Sigma));
    ui->SumFlag->setChecked(p.SumFlag);
    ui->NFAValidation->setChecked(p.NFAValidation);
    ui->MinLineLength->setValue(p.MinLineLength);
    ui->MaxDistanceBetweenTwoLines->setValue(p.MaxDistanceBetweenTwoLines);
    ui->LineFitErrorThreshold->setValue(p.LineFitErrorThreshold);
    ui->MaxErrorThreshold->setValue(p.MaxErrorThreshold);
}

bool EDParamDialog::validate(const QVariant &data, QString &outMsg)
{
    return EDParamsUtil::validateEDParams(data.value<opencved::EDParams>(), outMsg);
}

QVariant EDParamDialog::getDefaultData()
{
    opencved::EDParams p;
    EDParamsUtil::getDefaultEDParams(p);
    return QVariant::fromValue(p);
}

void EDParamDialog::onSubmissionResult(bool success, const QString &message)
{
    // 处理主窗口反馈的结果
}

EDParamDialog::~EDParamDialog()
{
    delete ui;
}
