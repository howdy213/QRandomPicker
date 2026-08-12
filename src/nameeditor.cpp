#include "nameeditor.h"
#include <QFileDialog>
#include <QFile>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

NameListEditor::NameListEditor(const QStringList &names, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("编辑名单");
    resize(400, 500);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(8);

    auto *topBar = new QWidget(this);
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(6);

    auto *btnImport = new QPushButton("从文件导入...", this);
    auto *btnClear = new QPushButton("清空", this);
    topLayout->addWidget(btnImport);
    topLayout->addWidget(btnClear);
    topLayout->addStretch();

    m_lblCount = new QLabel("0 人", this);
    topLayout->addWidget(m_lblCount);

    layout->addWidget(topBar);

    m_editor = new QTextEdit(this);
    m_editor->setFont(QFont("Consolas", 10));
    m_editor->setPlaceholderText("每行一个名字...");
    layout->addWidget(m_editor, 1);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    auto *btnOk = new QPushButton("确定", this);
    auto *btnCancel = new QPushButton("取消", this);
    bottomLayout->addWidget(btnOk);
    bottomLayout->addWidget(btnCancel);
    layout->addLayout(bottomLayout);

    m_editor->setPlainText(names.join("\n"));
    onTextChanged();

    connect(btnImport, &QPushButton::clicked, this, &NameListEditor::onImportClicked);
    connect(btnClear, &QPushButton::clicked, this, [this]() {
        m_editor->clear();
    });
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_editor, &QTextEdit::textChanged, this, &NameListEditor::onTextChanged);
}

QStringList NameListEditor::names() const {
    QStringList result;
    const auto lines = m_editor->toPlainText().split('\n', Qt::SkipEmptyParts);
    for (const auto &line : lines) {
        QString t = line.trimmed();
        if (!t.isEmpty() && !result.contains(t))
            result << t;
    }
    return result;
}

void NameListEditor::onImportClicked() {
    QString path = QFileDialog::getOpenFileName(this, "选择名单文件",
                                                 QString(),
                                                 "文本文件 (*.txt);;所有文件 (*.*)");
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    QStringList existing = names();
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !existing.contains(line))
            existing << line;
    }
    m_editor->setPlainText(existing.join("\n"));
}

void NameListEditor::onClearClicked() {
}

void NameListEditor::onTextChanged() {
    int count = names().size();
    m_lblCount->setText(QString("%1 人").arg(count));
}
