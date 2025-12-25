#ifndef BASE_DIALOG_H
#define BASE_DIALOG_H

#include <QDialog>

// ---------------------------------------------------------
// 抽象基类 BaseDialog
// ---------------------------------------------------------
class BaseDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BaseDialog(QWidget *parent = nullptr) : QDialog(parent) {}
    virtual ~BaseDialog() {}

    // 纯虚函数：子类必须实现，用于获取/设置当前界面上的数据
    virtual QVariant getData() = 0;
    virtual void setData(const QVariant &data) = 0;

    // 用于主窗口处理完后，更新子窗口状态（如清空输入或显示提示）
    virtual void onSubmissionResult(bool success, const QString &message) = 0;

signals:
    void dataSubmitted(const QVariant &data);

};

// // ---------------------------------------------------------
// // 子类 EdParamsDialog
// // ---------------------------------------------------------
// // 前向声明 UI 类（与你创建 .ui 文件时生成的名称一致）
// namespace Ui { class EdParamsDialog; }
// class EdParamsDialog : public BaseDialog
// {
//     Q_OBJECT
// public:
//     explicit EdParamsDialog(QWidget *parent = nullptr);
//     ~EdParamsDialog();
//     QVariant getData() const override;
//     void onSubmissionResult(bool success, const QString &message) override;
// private:
//     Ui::EdParamsDialog *ui; // 子类持有自己的 UI
// };

// // ---------------------------------------------------------
// // 子类 NextDialog
// // ---------------------------------------------------------
// namespace Ui { class NextDialog; }
// class NextDialog : public BaseDialog
// {
//     Q_OBJECT
// public:
//     explicit NextDialog(QWidget *parent = nullptr);
//     ~NextDialog();
//     QVariant getData() const override;
//     void onSubmissionResult(bool success, const QString &message) override;

// private:
//     Ui::NextDialog *ui;
// };

#endif // BASE_DIALOG_H
