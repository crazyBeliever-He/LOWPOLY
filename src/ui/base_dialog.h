#ifndef BASE_DIALOG_H
#define BASE_DIALOG_H

#include <QDialog>
#include <QVariant>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

// ---------------------------------------------------------
// 抽象基类 BaseDialog
// ---------------------------------------------------------
class BaseDialog : public QDialog
{
    Q_OBJECT

private:
    QLabel* label_Status = nullptr;

public:
    explicit BaseDialog(QWidget *parent = nullptr) : QDialog(parent){}
    virtual ~BaseDialog() {}
signals:
    void dataSubmitted(const QVariant &data);

// --- 子类必须实现的方法 ---
public:
    // 设置当前界面上的数据
    virtual void setData(const QVariant &data) = 0;
protected:
    // 获取当前界面上的数据
    virtual QVariant getData() = 0;
    // 判断数据是否合法
    virtual bool validate(const QVariant &data, QString &outMsg) = 0;
    // 获取默认设置的参数
    virtual QVariant getDefaultData() = 0;
    // 用于主窗口处理完后，更新子窗口状态(如清空输入或显示提示)
    virtual void onSubmissionResult(bool success, const QString &message) = 0;

// --- 公用实现 ---
protected:
    // 统一初始化: 在子类 setupUi 之后调用
    void initDialogConnections()
    {
        // 设置窗口属性: 关闭时不销毁, 只是隐藏(实现单例复用)
        this->setAttribute(Qt::WA_QuitOnClose, false);
        // 将焦点给窗口背景, 这样没有任何输入控件会被默认选中.
        this->setFocus();
        // 自动查找并连接按钮(前提是 ObjectName 统一)
        QPushButton* confirmBtn = findChild<QPushButton*>("confirmButton");
        QPushButton* resetBtn = findChild<QPushButton*>("resetButton");

        if (confirmBtn) connect(confirmBtn, &QPushButton::clicked, this, &BaseDialog::onConfirmButtonClicked);
        if (resetBtn) connect(resetBtn, &QPushButton::clicked, this, &BaseDialog::onResetButtonClicked);
    }
    // confirm Button 操作
    void onConfirmButtonClicked()
    {
        QVariant currentData = getData();
        QString validationMsg;
        // 1. 调用子类的校验逻辑
        bool isValid = validate(currentData, validationMsg);
        if (!isValid)
        {   // 使用 critical 错误图标, 用户必须修改, 不发出信号
            QMessageBox::critical(this, tr("Parameter Error"), validationMsg);
            updateStatus(tr("Error. Please fix parameters."), "red");
            return; // 用户不能提交
        }
        if (!validationMsg.isEmpty())
        {   // 使用 warning 警告图标, 询问用户是否坚持提交
            QString question = validationMsg + "\n\n" + tr("Do you want to continue?");
            auto reply = QMessageBox::warning(this, tr("Parameter Warning"),
                                              question, QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No)
            {
                updateStatus(tr("Canceled by user."), "orange");
                return; // 用户选择返回修改
            }
        }
        // 2. 校验通过, 发射信号
        updateStatus(tr("Parameters submitted."), "green");
        emit dataSubmitted(currentData);
    }
    // reset Button 操作
    void onResetButtonClicked()
    {
        setData(getDefaultData());
        updateStatus(tr("Parameters reset to default."), "blue");
    }
    // 设置状态栏文本及颜色
    void updateStatus(const QString &text, const QString &color = "black")
    {
        if (label_Status)
        {
            label_Status->setText(text);
            label_Status->setStyleSheet(QString("color: %1;").arg(color));
        }
    }
};

#endif // BASE_DIALOG_H
