#include <memory>
#include <QLocale>
#include "ui/module_config_dialog.hpp"

#include <QValidator>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QIntValidator>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

namespace ui {

namespace {

std::unique_ptr<QValidator> make_validator(FieldSpec::Type type, QObject* parent) {
    switch (type) {
    case FieldSpec::Type::Port: {
        auto validator = std::make_unique<QIntValidator>(0, 65535, parent);
        validator->setLocale(QLocale::C);
        return validator;
    }
    case FieldSpec::Type::IpAddress: {
        QRegularExpression regex(
            R"((25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)\."
            R"((25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)\."
            R"((25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d)\."
            R"((25[0-5]|2[0-4]\d|1\d{2}|[1-9]?\d))"
        );
        auto validator = std::make_unique<QRegularExpressionValidator>(regex, parent);
        validator->setLocale(QLocale::C);
        return validator;
    }
    case FieldSpec::Type::Text:
    default:
        return nullptr;
    }
}

}  // namespace

ModuleConfigDialog::ModuleConfigDialog(const QString& title, std::vector<FieldSpec> fields, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(title);
    auto* layout = new QVBoxLayout(this);

    auto* form_layout = new QFormLayout();
    build_form(form_layout, fields);
    layout->addLayout(form_layout);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ModuleConfigDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ModuleConfigDialog::reject);
    layout->addWidget(buttons);
}

void ModuleConfigDialog::build_form(QFormLayout* layout, const std::vector<FieldSpec>& fields) {
    for (const auto& field : fields) {
        auto* line_edit = new QLineEdit(this);
        line_edit->setText(field.value);
        line_edit->setPlaceholderText(field.placeholder);
        if (auto validator = make_validator(field.type, line_edit)) {
            line_edit->setValidator(validator.release());
        }
        layout->addRow(field.label, line_edit);
        inputs_[field.key] = line_edit;
    }
}

std::map<std::string, QString> ModuleConfigDialog::values() const {
    std::map<std::string, QString> result;
    for (const auto& [key, edit] : inputs_) {
        result[key] = edit->text();
    }
    return result;
}

}  // namespace ui

