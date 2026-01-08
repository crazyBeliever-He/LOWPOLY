#include "douglas_peucker_dialog.h"
#include "ui_douglas_peucker_dialog.h"

#include "params_util.h"

DPParamDialog::DPParamDialog(QWidget *parent)
    : BaseDialog(parent)
    , ui(new Ui::DPParamDialog)
{
    ui->setupUi(this);
    initDialogConnections();
}

QVariant DPParamDialog::getData()
{
    DPParams p;
    p.epsilon = ui->Epsilon->value();
    p.eta = ui->Eta->value();
    return QVariant::fromValue(p);
}
void DPParamDialog::setData(const QVariant &data)
{
    if (!data.canConvert<DPParams>()) return;
    DPParams p = data.value<DPParams>();
    // 使用 QSignalBlocker 阻止信号发射, 阻止整个 dialog 的信号发射, 防止更新 UI 时触发不必要的事件逻辑
    QSignalBlocker blocker(this);
    ui->Epsilon->setValue(p.epsilon);
    ui->Eta->setValue(p.eta);
}

bool DPParamDialog::validate(const QVariant &data, QString &outMsg)
{
    return DPParamsUtil::validateDPParams(data.value<DPParams>(), outMsg);
}

QVariant DPParamDialog::getDefaultData()
{
    DPParams p;
    DPParamsUtil::getDefaultDPParams(p);
    return QVariant::fromValue(p);
}

void DPParamDialog::onSubmissionResult(bool success, const QString &message)
{
    // 处理主窗口反馈的结果
}

DPParamDialog::~DPParamDialog()
{
    delete ui;
}
