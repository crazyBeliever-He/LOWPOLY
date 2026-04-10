#ifndef TRIANGLE_COLOR_DIALOG_H
#define TRIANGLE_COLOR_DIALOG_H

#include <QDialog>
#include <QColor>

QT_BEGIN_NAMESPACE
namespace Ui {
class TriangleColorDialog;
}
QT_END_NAMESPACE

class TriangleColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TriangleColorDialog(QWidget *parent = nullptr);
    ~TriangleColorDialog();

    // 暴露给外界的接口：当主图选中新三角形时，调用此函数更新面板数据
    void setTarget(int triIndex, const QColor& color);

signals:
    // 确认修改时发出的信号，交由 ImageController 处理
    void colorConfirmed(int triIndex, QColor newColor);

    // 通知外部窗口已关闭
    void dialogClosed();

    // (可选体验升级) 如果想要拖动滑动条时主图实时变色，可以发出此信号
    void colorPreview(int triIndex, QColor previewColor);

protected:
    // 重写关闭事件
    void closeEvent(QCloseEvent *event) override;

private slots:
    // UI 绑定的槽函数
    void onConfirmClicked();
    void onResetClicked();
    void onColorChanged(); // 当 RGB 数值改变时，更新面板里的颜色预览块

private:
    void updateColorPreviewBlock(const QColor& color);

    Ui::TriangleColorDialog *ui;
    int currentTriIndex;   // 当前正在编辑的三角形索引
    QColor originalColor;  // 点进来时的初始颜色（用于重置）
    QColor currentColor;   // 当前在面板上调节出的颜色
};

#endif // TRIANGLE_COLOR_DIALOG_H
