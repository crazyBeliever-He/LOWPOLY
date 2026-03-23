#ifndef SALIENCY_DETECTION_DIALOG_H
#define SALIENCY_DETECTION_DIALOG_H

#include "base_dialog.h"
#include "algorithm_params.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class SDParamDialog;
}
QT_END_NAMESPACE

class SDParamDialog : public BaseDialog
{
    Q_OBJECT

public:
    explicit SDParamDialog(QWidget *parent = nullptr);
    ~SDParamDialog();
    // 负责 结构体 -> UI
    void setData(const QVariant &data) override;

protected:
    // 负责 UI -> 结构体
    QVariant getData() override;
    // 负责调用对应的 Util 校验
    bool validate(const QVariant &data, QString &outMsg) override;
    // 负责调用对应的 Util 获取默认值
    QVariant getDefaultData() override;
    // 处理主窗口反馈的结果
    void onSubmissionResult(bool success, const QString &message) override;

private:
    Ui::SDParamDialog *ui;
};

// 注册 Traits，绑定数据结构和 UI 窗口
template <>
struct DialogTraits<SDParams>{
    using DialogType = SDParamDialog;
};

#endif // SALIENCY_DETECTION_DIALOG_H
