#pragma once

#include <map>
#include <string>
#include <vector>
#include <QString>

#include <QDialog>

class QFormLayout;
class QLineEdit;

namespace ui {

struct FieldSpec {
    enum class Type {
        Text,
        Port,
        IpAddress,
    };

    std::string key;
    QString label;
    QString value;
    QString placeholder;
    Type type = Type::Text;
};

class ModuleConfigDialog : public QDialog {
    Q_OBJECT

public:
    ModuleConfigDialog(const QString& title, std::vector<FieldSpec> fields, QWidget* parent = nullptr);

    std::map<std::string, QString> values() const;

private:
    void build_form(QFormLayout* layout, const std::vector<FieldSpec>& fields);

    std::map<std::string, QLineEdit*> inputs_;
};

}  // namespace ui

