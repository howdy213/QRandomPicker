#include "historydialog.h"
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
    auto *btnClear = new QPushButton("清空历史记录", this);
    btnClear->setDisabled(history.isEmpty());
    bottomLayout->addWidget(btnClear);
    bottomLayout->addStretch();
    auto *btnClose = new QPushButton("关闭", this);
    bottomLayout->addWidget(btnClose);
    layout->addLayout(bottomLayout);

    connect(btnClear, &QPushButton::clicked, this, &HistoryDialog::onClearClicked);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
}

void HistoryDialog::onClearClicked() {
    m_clearRequested = true;
    m_list->clear();
    m_lblCount->setText("0 条");
    accept();
}
