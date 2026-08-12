#include "sessionpickerdialog.h"
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

SessionPickerDialog::SessionPickerDialog(const QStringList &availableNames, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("打开会话");
    resize(400, 450);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(8);

    auto *topBar = new QWidget(this);
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(6);

    auto *lblTitle = new QLabel("选择要打开的会话：", this);
    topLayout->addWidget(lblTitle);
    topLayout->addStretch();

    auto *btnSelectAll = new QPushButton("全选", this);
    auto *btnInvert = new QPushButton("反选", this);
    topLayout->addWidget(btnSelectAll);
    topLayout->addWidget(btnInvert);
    layout->addWidget(topBar);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    for (const auto &name : availableNames)
        m_list->addItem(name);
    layout->addWidget(m_list);

    auto *bottomLayout = new QHBoxLayout();
    m_lblCount = new QLabel("已选 0 项", this);
    bottomLayout->addWidget(m_lblCount);
    bottomLayout->addStretch();
    auto *btnOpen = new QPushButton("打开", this);
    auto *btnCancel = new QPushButton("取消", this);
    bottomLayout->addWidget(btnOpen);
    bottomLayout->addWidget(btnCancel);
    layout->addLayout(bottomLayout);

    connect(btnSelectAll, &QPushButton::clicked, this, &SessionPickerDialog::onSelectAll);
    connect(btnInvert, &QPushButton::clicked, this, &SessionPickerDialog::onInvert);
    connect(btnOpen, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_list, &QListWidget::itemSelectionChanged, this, &SessionPickerDialog::onSelectionChanged);

    onSelectionChanged();
}

QStringList SessionPickerDialog::selectedNames() const {
    QStringList names;
    for (auto *item : m_list->selectedItems())
        names << item->text();
    return names;
}

void SessionPickerDialog::onSelectionChanged() {
    int count = m_list->selectedItems().size();
    m_lblCount->setText(QString("已选 %1 项").arg(count));
}

void SessionPickerDialog::onSelectAll() {
    m_list->selectAll();
}

void SessionPickerDialog::onInvert() {
    QItemSelectionModel *sm = m_list->selectionModel();
    if (!sm) return;
    QItemSelection inverted;
    QModelIndex topLeft = m_list->model()->index(0, 0);
    QModelIndex bottomRight = m_list->model()->index(m_list->count() - 1, 0);
    inverted.select(topLeft, bottomRight);
    sm->select(inverted, QItemSelectionModel::Toggle);
}
