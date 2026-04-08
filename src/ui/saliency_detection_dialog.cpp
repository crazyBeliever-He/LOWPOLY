#include "saliency_detection_dialog.h"
#include "ui_saliency_detection_dialog.h"

#include "algorithm_params.h"
#include "param_util.h"
#include "dialogs_factory.h"

SDParamDialog::SDParamDialog(QWidget *parent)
    : BaseDialog(parent)
    , ui(new Ui::SDParamDialog)
{
    ui->setupUi(this);
    initDialogConnections();
}

QVariant SDParamDialog::getData()
{
    SDParams p;
    // 从 QComboBox 获取当前的选项索引 (0, 1, 或 2)
    p.type = ui->SaliencyDetectionType->currentIndex();
    p.lambda = ui->Lambda->value();
    p.threshold = ui->Threshold->value();
    p.userN = ui->SampleNumber->value();
    return QVariant::fromValue(p);
}

void SDParamDialog::setData(const QVariant &data)
{
    if (!data.canConvert<SDParams>()) return;
    SDParams p = data.value<SDParams>();

    // 使用 QSignalBlocker 阻止信号发射
    QSignalBlocker blocker(this);
    ui->SaliencyDetectionType->setCurrentIndex(p.type);
    ui->Lambda->setValue(p.lambda);
    ui->Threshold->setValue(p.threshold);
    ui->SampleNumber->setValue(p.userN);
}

bool SDParamDialog::validate(const QVariant &data, QString &outMsg)
{
    // 调用 ParamsUtil 中对应的校验方法
    return SDParamsUtil::validateSDParams(data.value<SDParams>(), outMsg);
}

QVariant SDParamDialog::getDefaultData()
{
    SDParams p;
    // 调用 ParamsUtil 中对应的获取默认值方法
    SDParamsUtil::getDefaultSDParams(p);
    return QVariant::fromValue(p);
}

void SDParamDialog::onSubmissionResult(bool success, const QString &message)
{
    // 处理主窗口反馈的结果
}

SDParamDialog::~SDParamDialog()
{
    delete ui;
}

// 注册当前 Dialog 到工厂中
REGISTER_DIALOG(SDParams, SDParamDialog);
