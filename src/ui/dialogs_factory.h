#ifndef DIALOGS_FACTORY_H
#define DIALOGS_FACTORY_H
#include <QMap>
#include <QString>
#include <functional>
#include "base_dialog.h"

class DialogFactory
{
public:
    using CreatorFn = std::function<BaseDialog*(QWidget*)>;

    // 单例模式 - instance()
    static DialogFactory& instance() {
        static DialogFactory factory; // 保证了局部静态变量初始化的线程安全
        return factory;
    }

    void registerDialog(const QString& typeName, CreatorFn fn) {
        m_creators[typeName] = fn;
    }

    BaseDialog* createDialog(const QString& typeName, QWidget* parent = nullptr) {
        if (m_creators.contains(typeName)) {
            return m_creators[typeName](parent);
        }
        return nullptr;
    }

private:
    DialogFactory() = default;
    QMap<QString, CreatorFn> m_creators;
};

// 静态自动注册 Static Auto-Registration, 用于在具体的 cpp 文件中实现自动注册
// ## 拼接符: 传入 DPParamDialog，编译器会把它变成 struct DialogRegistrar_DPParamDialog，保证名字唯一
// 匿名命名空间 namespace {}：限制了这个结构体和变量的作用域只在当前 .cpp 文件内有效，防止多个 Dialog 文件中出现名字冲突（链接错误）
#define REGISTER_DIALOG(ParamType, DialogClass) \
namespace { \
    struct DialogRegistrar_##DialogClass { \
        DialogRegistrar_##DialogClass() { \
            DialogFactory::instance().registerDialog( \
                                 QMetaType::fromType<ParamType>().name(), \
                                 [](QWidget* parent) { return new DialogClass(parent); } \
                    ); \
        } \
    } registrar_##DialogClass; \
}

#endif // DIALOGS_FACTORY_H
