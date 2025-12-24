#ifndef ED_PARAM_DIALOG_H
#define ED_PARAM_DIALOG_H

#include "base_dialog.h"
#include "edge_drawing_lib.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class EDParamDialog;
}
QT_END_NAMESPACE


class EdParamDialog : public BaseDialog
{
    Q_OBJECT

public:
    explicit EdParamDialog(QWidget *parent = nullptr);
    ~EdParamDialog();

    QVariant getData() override;
    void setData(const QVariant &data) override;

    void onSubmissionResult(bool success, const QString &message) override;

private slots:
    // 响应界面提交按钮的槽
    void onConfirmButtonClicked();

private:
    Ui::EDParamDialog *ui;
    opencved::EDParams edParams;

};

#endif // ED_PARAM_DIALOG_H
