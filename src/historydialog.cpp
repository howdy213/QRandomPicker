#include "historydialog.h"
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

HistoryDialog::HistoryDialog(const QList<PickRecord> &history, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("抽取历史记录");
    resize(550, 450);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(8);

    auto *topBar = new QWidget(this);
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0, 0, 0, 0);
    auto *lblTitle = new QLabel("历史记录", this);
    topLayout->addWidget(lblTitle);
    topLayout->addStretch();
    m_lblCount = new QLabel("0 条", this);
    topLayout->addWidget(m_lblCount);
    layout->addWidget(topBar);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::MultiSelection);
    layout->addWidget(m_list);

    for (const auto &rec : history) {
        QString timeStr = rec.timestamp.toString("yyyy-MM-dd hh:mm:ss");
        QString modeStr = rec.mode == PickMode::Random ? "随机" : "公平";
        QString namesStr = rec.picked.join(", ");
        QString entry = QString("[%1] %2 抽取 %3 人：%4")
                            .arg(timeStr)
                            .arg(modeStr)
                            .arg(rec.count)
                            .arg(namesStr);
        m_list->addItem(entry);
    }
    m_lblCount->setText(QString("%1 条").arg(m_list->count()));

    auto *bottomLayout = new QHBoxLayout();
    m_btnCopy = new QPushButton("复制选中", this);
    m_btnCopy->setDisabled(m_list->count() == 0);
    bottomLayout->addWidget(m_btnCopy);
    bottomLayout->addStretch();
    auto *btnClear = new QPushButton("清空历史记录", this);
    btnClear->setDisabled(history.isEmpty());
    bottomLayout->addWidget(btnClear);
    auto *btnClose = new QPushButton("关闭", this);
    bottomLayout->addWidget(btnClose);
    layout->addLayout(bottomLayout);

    connect(m_btnCopy, &QPushButton::clicked, this, &HistoryDialog::onCopyClicked);
    connect(btnClear, &QPushButton::clicked, this, &HistoryDialog::onClearClicked);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
}

void HistoryDialog::onClearClicked() {
    m_clearRequested = true;
    m_list->clear();
    m_lblCount->setText("0 条");
    accept();
}

void HistoryDialog::onCopyClicked() {
    QStringList items = selectedItems();
    if (items.isEmpty())
        return;
    QApplication::clipboard()->setText(items.join("\n"));
    m_btnCopy->setText(QString("已复制 %1 条").arg(items.size()));
}

QStringList HistoryDialog::selectedItems() const {
    QStringList res;
    for (auto *item : m_list->selectedItems())
        res.append(item->text());
    return res;
}
